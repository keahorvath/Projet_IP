#include "ColGenModel.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

#include "Heuristics.hpp"
#include "Instance.hpp"
#include "Solution.hpp"
#include "gurobi_c++.h"
using namespace std;

ColGenModel::ColGenModel(const Instance& inst_, PricingMethod pricing_method_, ColumnStrategy column_strategy_, Stabilization stabilization_,
                         bool verbose_)
    : inst(inst_), pricing_method(pricing_method_), column_strategy(column_strategy_), stabilization(stabilization_), verbose(verbose_) {
    env = new GRBEnv(true);
    if (!verbose) {
        env->set(GRB_IntParam_LogToConsole, 0);
    }
    env->start();
    model = new GRBModel(*env);

    // CONSTRAINTS
    // Each customer is assigned to one facility
    pi_constrs.resize(inst.nb_customers);
    for (int c = 0; c < inst.nb_customers; c++) {
        pi_constrs[c] = model->addConstr(0, GRB_EQUAL, 1, "assign once customer " + to_string(c));
    }
    // Don't use more than p columns
    theta_constr = model->addConstr(0, GRB_LESS_EQUAL, inst.nb_max_open_facilities, "no more than p columns");

    // Create an initial valid solution
    vector<Column> cols = Heuristics::closestCustomersCols(inst);
    for (Column col : cols) {
        addColumn(col);
    }
    optimize();

    // Initialize stabilization
    theta_center = getTheta();
    pi_center = getPi();
    best_LB = -numeric_limits<double>::infinity();
}

void ColGenModel::addColumn(Column col) {
    GRBColumn grb_col;
    for (int c : col.customers) {
        grb_col.addTerm(1, pi_constrs[c]);
    }
    grb_col.addTerm(1, theta_constr);

    lambda.push_back(model->addVar(0, 1, col.cost(inst), GRB_CONTINUOUS, grb_col));
}

double ColGenModel::getTheta() {
    return theta_constr.get(GRB_DoubleAttr_Pi);
}

double ColGenModel::getStabilizedTheta() {
    return stab_alpha * theta_center + (1 - stab_alpha) * getTheta();
}

vector<double> ColGenModel::getPi() {
    vector<double> duals;
    for (auto& constr : pi_constrs) {
        duals.push_back(constr.get(GRB_DoubleAttr_Pi));
    }
    return duals;
}

vector<double> ColGenModel::getStabilizedPi() {
    vector<double> stab_pi(inst.nb_customers);
    vector<double> current_pi = getPi();
    for (int c = 0; c < inst.nb_customers; c++) {
        stab_pi[c] = stab_alpha * pi_center[c] + (1 - stab_alpha) * current_pi[c];
    }
    return stab_pi;
}

double ColGenModel::obj() {
    return model->get(GRB_DoubleAttr_ObjVal);
}

void ColGenModel::optimize() {
    model->set(GRB_IntParam_Method, 0);
    model->optimize();
}

vector<double> ColGenModel::reducedCosts(int facility, vector<double> pi) {
    vector<double> reduced_costs;
    double cost;
    for (int c = 0; c < pi.size(); c++) {
        cost = distance(inst.customer_positions[c], inst.facility_positions[facility]);
        reduced_costs.push_back(cost - pi[c]);
    }
    return reduced_costs;
}

pair<double, Column> ColGenModel::pricingSubProblemMIP(int facility, double theta, vector<double> pi) {
    // Get the reduced costs
    vector<double> reduced_costs = reducedCosts(facility, pi);
    // Create pricing model
    GRBModel pricing_model(env);
    vector<GRBVar> z(inst.nb_customers);
    GRBLinExpr expr;
    for (int c = 0; c < inst.nb_customers; c++) {
        z[c] = pricing_model.addVar(0, 1, reduced_costs[c], GRB_BINARY);
        expr += z[c] * inst.customer_demands[c];
    }
    pricing_model.addConstr(expr, GRB_LESS_EQUAL, inst.facility_capacities[facility], "capacity constraint");

    // Solve the pricing model
    pricing_model.optimize();

    // Check if col has negative reduced cost
    double obj_val = pricing_model.get(GRB_DoubleAttr_ObjVal) - theta;

    // If positive (with small allowed rounding error), return blank column
    if (obj_val >= -1e-6) {
        return {0, Column()};
    }
    vector<int> col;
    for (int c = 0; c < inst.nb_customers; c++) {
        if (z[c].get(GRB_DoubleAttr_X) > 0.5) {
            col.push_back(c);
        }
    }
    // Otherwise return optimal solution
    return {obj_val, Column(facility, col)};
}

pair<double, Column> ColGenModel::pricingSubProblemDP(int facility, double theta, vector<double> pi) {
    // Get the reduced costs for each customer
    vector<double> rc = reducedCosts(facility, pi);

    int capacity = inst.facility_capacities[facility];

    // Store the best found reduced costs for each capacity state (from 0 to u_f)
    // We are minimizing so initialize all with +inf
    vector<double> RC(capacity + 1, numeric_limits<double>::infinity());
    RC[0] = 0;  // Initialize
    // We also have to store which customers are in the best sol for each capacity state
    vector<vector<bool>> c_in_best_sol(inst.nb_customers, vector<bool>(capacity + 1, false));

    // For each customer, see if adding it to a state is beneficial
    for (int c = 0; c < inst.nb_customers; c++) {
        int demand = inst.customer_demands[c];
        double c_rc = rc[c];
        // Go from end to start
        for (int state = capacity; state >= demand; state--) {
            if (RC[state - demand] == numeric_limits<double>::infinity()) {
                // State not accessible yet
                continue;
            }
            // Beneficial to add customer?
            if (RC[state - demand] + c_rc < RC[state]) {
                RC[state] = RC[state - demand] + c_rc;
                c_in_best_sol[c][state] = true;
            }
        }
    }

    // get best RC
    double best_rc = theta - 1e-6;
    int best_state = -1;
    for (int state = 0; state < capacity + 1; state++) {
        if (RC[state] < best_rc) {
            best_rc = RC[state];
            best_state = state;
        }
    }
    // cout << "best state" << best_state << endl;
    //  If positive, return blank column
    if (best_state == -1) {
        return {0, Column()};
    }

    // Backtrack: find customers in best sol
    int current_state = best_state;
    vector<int> best_customers = {};
    for (int c = inst.nb_customers - 1; c >= 0; c--) {
        if (c_in_best_sol[c][current_state]) {
            best_customers.push_back(c);
            current_state -= inst.customer_demands[c];
        }
    }

    // Otherwise return optimal solution
    Column col = Column(facility, best_customers);
    // cout << "adding col " << col << " rc = " << best_rc << endl;
    return {best_rc - theta, col};
}

vector<Column> ColGenModel::pricing() {
    vector<Column> cols;
    vector<double> col_values;
    Column best_col;
    double best_col_value = 0;
    double best_col_value_stab = 0;
    double obj_val = obj();
    // Calculate duals (depending on if stabilization or not)
    double theta;
    vector<double> pi;
    if (stabilization == Stabilization::INOUT) {
        theta = getStabilizedTheta();
        pi = getStabilizedPi();
    } else {
        theta = getTheta();
        pi = getPi();
    }
    for (int facility = 0; facility < inst.nb_potential_facilities; facility++) {
        // Solve the sub problem associated with facility (with given method)
        pair<double, Column> sub_pb;

        if (pricing_method == PricingMethod::MIP) {
            sub_pb = pricingSubProblemMIP(facility, theta, pi);
        } else {
            sub_pb = pricingSubProblemDP(facility, theta, pi);
        }
        if (sub_pb.second.facility == -1) {  // No column was found -> ignore
            continue;
        }
        // Update best column if necesssary
        if (stabilization == Stabilization::INOUT) {
            // Calculate reduced cost for normal duals
            Column col = sub_pb.second;
            double rc = -getTheta();
            vector<double> normal_pi = getPi();
            for (int c : col.customers) {
                rc += distance(inst.customer_positions[c], inst.facility_positions[facility]) - normal_pi[c];
            }
            if (rc < -1e-6) {
                // add to list of columns
                cols.push_back(sub_pb.second);
                col_values.push_back(sub_pb.first);
            }
            if (rc < best_col_value) {
                best_col = col;
                best_col_value = rc;
            }
        } else {
            if (sub_pb.first < best_col_value) {
                best_col = sub_pb.second;
                best_col_value = sub_pb.first;
            }
            // add to list of columns
            cols.push_back(sub_pb.second);
            col_values.push_back(sub_pb.first);
        }
    }
    if (best_col_value == 0) {  // No column found
        return {};
    }
    if (stabilization == Stabilization::INOUT) {
        double lagrangian_bound = obj() + inst.nb_max_open_facilities * best_col_value;
        if (lagrangian_bound > best_LB) {
            best_LB = lagrangian_bound;
            theta_center = theta;
            pi_center = pi;
        }
    }
    // Either return the best column or all of them (depending on what is asked)
    if (column_strategy == ColumnStrategy::MULTI) {
        return cols;
    }
    return {best_col};
}

Solution ColGenModel::convertSolution() {
    // TODO
    Solution sol;
    return sol;
}

int ColGenModel::solve(int time_limit) {
    int nb_cols = 0;
    auto start = chrono::high_resolution_clock::now();
    chrono::duration<double> time_elapsed = chrono::high_resolution_clock::now() - start;
    while (true) {
        time_elapsed = chrono::high_resolution_clock::now() - start;
        if (time_elapsed.count() >= time_limit) {
            break;
        }
        vector<Column> cols = pricing();
        if (cols.empty()) break;
        for (Column col : cols) {
            addColumn(col);
            nb_cols++;
        }
        optimize();
    }
    time_elapsed = chrono::high_resolution_clock::now() - start;
    runtime = time_elapsed.count();
    return nb_cols;
}

void ColGenModel::printResult() {
    int status = model->get(GRB_IntAttr_Status);
    double obj_val = model->get(GRB_DoubleAttr_ObjVal);
    if (status == GRB_OPTIMAL) {
        cout << "-----------------------" << endl;
        cout << "OPTIMAL SOLUTION FOUND!" << endl;
        cout << "-----------------------" << endl;
        cout << "Optimal solution value : " << obj_val << " (" << fixed << setprecision(4) << runtime << "s)" << endl;
    } else if (status == GRB_TIME_LIMIT) {
        cout << "--------------------------------------------" << endl;
        cout << "NO OPTIMAL SOLUTION FOUND WITHIN TIME LIMIT!" << endl;
        cout << "--------------------------------------------" << endl;
        cout << "Best solution value : " << obj_val << " (" << fixed << setprecision(4) << runtime << "s)" << endl;
        cout << "Dual Bound : " << model->get(GRB_DoubleAttr_ObjBound) << endl;
    } else {
        cout << "---------------------------" << endl;
        cerr << "NO FEASIBLE SOLUTION FOUND!" << endl;
        cout << "---------------------------" << endl;
    }
}

ColGenModel::~ColGenModel() {
    delete env;
    delete model;
}
