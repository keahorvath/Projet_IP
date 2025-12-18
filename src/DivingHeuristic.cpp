#include "DivingHeuristic.hpp"

using namespace std;

DivingHeuristic::DivingHeuristic(ColGenModel& model) : model(model) {}

pair<double, Column> DivingHeuristic::pricingSubProblem(int facility, double theta, const vector<double>& pi) {
    // Get the reduced costs for each customer
    vector<double> rc = model.reducedCosts(facility, pi);

    int capacity = model.inst.facility_capacities[facility];
    int nb_customers = model.inst.nb_customers;

    // Store the best found reduced costs for each capacity state (from 0 to u_f)
    // We are minimizing so initialize all with +inf
    vector<double> RC(capacity + 1, numeric_limits<double>::infinity());
    RC[0] = 0;
    // We also have to store which customers are in the best sol for each capacity state
    vector<vector<bool>> c_in_best_sol(nb_customers, vector<bool>(capacity + 1, false));
    // For each customer, see if adding it to a state is beneficial
    for (int c = 0; c < nb_customers; c++) {
        // If c is forced to be with a facility that isn't the one here, never add it to a col
        if (forced_facility_for_client[c] != -1 && forced_facility_for_client[c] != facility) {
            continue;
        }
        int demand = model.inst.customer_demands[c];
        double c_rc = rc[c];

        // If c is forced to be with given facility, it HAS to be in every col
        if (forced_facility_for_client[c] == facility) {
            for (int state = capacity; state >= 0; state--) {
                // If customer can be placed, place it
                if (state >= demand && RC[state - demand] != numeric_limits<double>::infinity()) {
                    RC[state] = RC[state - demand] + c_rc;
                    c_in_best_sol[c][state] = true;
                } else {  // If customer can't be placed, make state inaccessible
                    RC[state] = numeric_limits<double>::infinity();
                }
            }
        } else {  // customer isn't forced or prohibited (same as normal DP)
            for (int state = capacity; state >= demand; state--) {
                if (RC[state - demand] != numeric_limits<double>::infinity()) {
                    if (RC[state - demand] + c_rc < RC[state]) {
                        RC[state] = RC[state - demand] + c_rc;
                        c_in_best_sol[c][state] = true;
                    }
                }
            }
        }
    }

    // get best RC
    double best_rc = theta - 1e-6;
    int best_state = -1;
    for (int state = 0; state <= capacity; state++) {
        if (RC[state] < best_rc) {
            best_rc = RC[state];
            best_state = state;
        }
    }
    //  If positive, return blank column
    if (best_state == -1) return {0, Column()};

    // Backtrack: find customers in best sol
    vector<int> best_customers;
    int current_state = best_state;
    for (int c = nb_customers - 1; c >= 0; c--) {
        if (c_in_best_sol[c][current_state]) {
            best_customers.push_back(c);
            current_state -= model.inst.customer_demands[c];
        }
    }
    // Otherwise return optimal solution
    return {best_rc - theta, Column(facility, best_customers)};
}

vector<Column> DivingHeuristic::pricing() {
    vector<Column> new_cols;

    // get duals
    double theta = model.getTheta();
    vector<double> pi = model.getPi();

    for (int f = 0; f < model.inst.nb_potential_facilities; f++) {
        // only difference with normal pricing: use pricing sub problem adapted to diving
        pair<double, Column> result = pricingSubProblem(f, theta, pi);

        // If valid solution found
        if (result.second.facility != -1) {
            new_cols.push_back(result.second);
        }
    }
    return new_cols;  // using method MULTI because faster
}

pair<int, int> DivingHeuristic::getBestFCPair() {
    // Reconstructing x[f][c] matrix to see "how much" each customer is with each facility
    int nb_f = model.inst.nb_potential_facilities;
    int nb_c = model.inst.nb_customers;
    vector<vector<double>> x(nb_f, vector<double>(nb_c, 0.0));
    for (int i = 0; i < model.lambda.size(); i++) {
        double val = model.lambda[i].get(GRB_DoubleAttr_X);
        if (val > 1e-6) {  // Allow for rounding errors
            Column col = model.model_cols[i];
            for (int c : col.customers) {
                x[col.facility][c] += val;
            }
        }
    }

    pair<int, int> best_pair = {-1, -1};
    double best_value = -1.0;

    // Get the pair with biggest value between 0 and 1 (but not 0 or 1!)
    for (int f = 0; f < nb_f; f++) {
        for (int c = 0; c < nb_c; c++) {
            double val = x[f][c];
            if (val > 1e-4 && val < 1.0 - 1e-4) {  // allow for rounding erros
                if (val > best_value) {
                    best_value = val;
                    best_pair = {f, c};
                }
            }
        }
    }
    return best_pair;
}

void DivingHeuristic::prohibidCols(int customer, int facility) {
    int count_removed = 0;
    // Go through each column and check if it allowed when forcing given customer and facility together
    for (int i = 0; i < model.lambda.size(); i++) {
        // If already disabled, skip
        if (model.lambda[i].get(GRB_DoubleAttr_UB) < 0.5) {
            continue;
        }
        Column col = model.model_cols[i];
        bool invalid = false;
        // First scenario:
        // Column is associated with given facility but the customer is not in it
        if (col.facility == facility) {
            bool contains_customer = false;
            for (int c : col.customers) {
                if (c == customer) {
                    contains_customer = true;
                    break;
                }
            }
            if (!contains_customer) {
                model.lambda[i].set(GRB_DoubleAttr_UB, 0.0);  // disable column
                count_removed++;
            }
        }
        // Second scenario:
        // Column is not associated with given facility but given customer is in it
        else {
            for (int c : col.customers) {
                if (c == customer) {
                    model.lambda[i].set(GRB_DoubleAttr_UB, 0.0);  // disable column
                    count_removed++;
                }
            }
        }
    }
    model.model->update();  // make sure changes are applied
    // for debugging
    // cout << "Disabled " << count_removed << " incompatible columns" << endl;
}

Solution DivingHeuristic::convertSolution() {
    vector<int> facility_for_each_customer(model.inst.nb_customers, -1);
    for (int i = 0; i < model.lambda.size(); i++) {
        double col_val = model.lambda[i].get(GRB_DoubleAttr_X);
        if (col_val > 0.5) {  // if column is used in current solution
            Column col = model.model_cols[i];
            for (int c : col.customers) {
                if (facility_for_each_customer[c] != -1) {
                    cout << "ERROR: customer " << c << " is assigned to multiple facilities!" << endl;
                }
                facility_for_each_customer[c] = col.facility;
            }
        }
    }
    Solution sol;
    for (int c = 0; c < model.inst.nb_customers; c++) {
        int f = facility_for_each_customer[c];
        Point2D assignment = {model.inst.facility_positions[f]};
        sol.push_back(assignment);
    }
    return sol;
}

void DivingHeuristic::solve(int time_limit) {
    forced_facility_for_client.assign(model.inst.nb_customers, -1);

    // Solve model
    model.solve(time_limit);

    while (true) {
        // Find the best customer-facility pair that isn't forced yet
        pair<int, int> pair = getBestFCPair();
        if (pair.first == -1) {  // all customers are assigned to a facility
            break;
        }

        int f = pair.first;
        int c = pair.second;

        // for debugging:
        // cout << "Assigning C" << c << " to F" << f << endl;

        // Force client c to be with facility f
        if (forced_facility_for_client[c] != -1) {  // make sure assignments is valid
            cout << "ERROR : customer " << c << " is assigned to multiple clients" << endl;
        }
        forced_facility_for_client[c] = f;

        // Remove incompatible columns
        prohibidCols(c, f);

        // Update model
        model.optimize();

        // Find best valid columns to add
        while (true) {
            vector<Column> cols = pricing();
            if (cols.empty()) {
                break;
            }
            for (Column& col : cols) {
                model.addColumn(col);
            }
            model.optimize();
        }
    }
}
