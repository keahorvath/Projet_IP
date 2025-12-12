#include <fstream>
#include <iostream>
#include <string>

#include "Heuristics.hpp"
#include "Instance.hpp"
#include "Solution.hpp"
#include "gurobi_c++.h"
using namespace std;

/**
 * @struct struct that contains methods to solve the problem
 *         using a column generation approach
 */
struct ColGenModel {
    GRBEnv* env;
    GRBModel* model;
    int time_limit;
    bool verbose;
    Instance inst;

    vector<GRBVar> lambda;

    GRBConstr col_cap;
    vector<GRBConstr> cust_assignments;

    /**
     * @brief Instanciate Relaxed Master Problem: create constraints and create initial cols to make a feasible solution
     */
    ColGenModel(const Instance& inst_, int time_limit_ = 300, bool verbose_ = false) : inst(inst_), time_limit(time_limit_), verbose(verbose_) {
        env = new GRBEnv(true);
        if (!verbose) {
            env->set(GRB_IntParam_LogToConsole, 0);
        }
        env->start();
        model = new GRBModel(*env);

        // CONSTRAINTS
        // Each customer is assigned to one facility
        cust_assignments.resize(inst.nb_customers);
        for (int c = 0; c < inst.nb_customers; c++) {
            cust_assignments[c] = model->addConstr(0, GRB_EQUAL, 1, "assign once customer " + to_string(c));
        }
        // Don't use more than p columns
        col_cap = model->addConstr(0, GRB_LESS_EQUAL, inst.nb_max_open_facilities, "don't have more than p columns");

        // Create an initial valid solution
        vector<Column> cols = Heuristics::closestCustomersCols(inst);
        for (Column col : cols) {
            addColumn(col);
        }
        optimize();
        cout << "test 1 " << endl;
    }

    /**
     * @brief Take a column and add it the RMP
     */
    void addColumn(Column col) {
        GRBColumn grb_col;
        for (int c : col.customers) {
            cout << "adding customer " << c << endl;
            grb_col.addTerm(1, cust_assignments[c]);
        }
        grb_col.addTerm(1, col_cap);

        lambda.push_back(model->addVar(0, INFINITY, col.cost(inst), GRB_CONTINUOUS, grb_col));
    }

    /**
     * @brief Get the value of theta
     */
    double colCapDual() {
        return col_cap.get(GRB_DoubleAttr_Pi);
    }

    /**
     * @brief Get the values of pi_c
     */
    vector<double> custAssignmentsDuals() {
        vector<double> duals;
        for (auto& constr : cust_assignments) {
            duals.push_back(constr.get(GRB_DoubleAttr_Pi));
        }
        return duals;
    }

    double obj() {
        return model->get(GRB_DoubleAttr_ObjVal);
    }

    void optimize() {
        model->set(GRB_IntParam_Method, 0);
        model->optimize();
    }

    Column pricing() {
        // Find the best column for each facility and keep the best one
        Column best_col = Column(-1, vector<int>{});
        int best_facility = 0;
        double best_reduced_cost = 0;
        for (int facility = 0; facility < inst.nb_potential_facilities; facility++) {
            // Get the reduced costs
            vector<double> duals = custAssignmentsDuals();
            vector<double> reduced_costs;
            double cost;
            for (int c = 0; c < duals.size(); c++) {
                cost = distance(inst.customer_positions[c], inst.facility_positions[facility]);
                reduced_costs.push_back(cost + duals[c]);
            }
            // Create pricing model
            GRBModel pricing_model(env);
            vector<GRBVar> z(inst.nb_customers);
            GRBLinExpr expr;
            cout << "test 2 " << endl;

            for (int c = 0; c < inst.nb_customers; c++) {
                z[c] = pricing_model.addVar(0, 1, reduced_costs[c], GRB_BINARY);
                expr += z[c] * inst.customer_demands[c];
            }

            pricing_model.addConstr(expr, GRB_LESS_EQUAL, inst.facility_capacities[facility], "capacity constraint");

            // Solve the pricing model
            pricing_model.optimize();

            double obj_val = pricing_model.get(GRB_DoubleAttr_ObjVal);

            // If this column is better than previous best, save
            if (obj_val <= best_reduced_cost) {
                vector<int> col;
                for (int c = 0; c < inst.nb_customers; c++) {
                    col.push_back(z[c].get(GRB_DoubleAttr_X));
                }
                best_facility = facility;
                best_reduced_cost = obj_val;
                best_col = Column(facility, col);
            }
        }
        return best_col;
    }

    Solution convertSolution() {
        // TODO
        Solution sol;
        return sol;
    }

    void solve() {
        while (true) {
            cout << "before pricing " << endl;

            Column col = pricing();
            if (col.facility == -1) break;
            cout << "before col add" << endl;
            addColumn(col);
            cout << "before optimize" << endl;
            optimize();
        }
        for (int g = 0; g < lambda.size(); g++) {
            cout << "col ", g, " has value", lambda[g].get(GRB_DoubleAttr_X);
        }
    }

    ~ColGenModel() {
        delete env;
        delete model;
    }
};

void usage(const string& prog_name) {
    cout << "Usage: " << prog_name << " file_path [time_limit] [-v] [-e]" << endl;
    cout << "  file_path  : path to the input instance file" << endl;
    cout << "  time_limit : maximum execution time in seconds (optional), default is 300s" << endl;
    cout << "  -v         : add to enable verbose output (optional)" << endl;
    cout << "  -e         : add to export solution file and solution visualizer (optional)" << endl;
}

int main(int argc, char** argv) {
    int time_limit = 300;
    bool verbose = false;
    bool export_res = false;
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }
    // First argument : File path
    string file_name = argv[1];
    ifstream inst_file(file_name);
    if (!inst_file) {
        cerr << "Error: Couldn't open file!" << endl;
        cerr << "Please enter valid file path" << endl;
        return 1;
    }
    // Optionnal arguments
    if (argc >= 3) {
        bool has_time_limit = false;
        for (int i = 2; i < argc; i++) {
            string arg = argv[i];
            if (arg == "-v") {
                verbose = true;
            } else if (arg == "-e") {
                export_res = true;
            } else if (!has_time_limit) {
                try {
                    time_limit = stod(argv[2]);
                    has_time_limit = true;
                    if (time_limit <= 0) {
                        cerr << "Error: time_limit must be positive" << endl;
                        usage(argv[0]);
                        return 1;
                    }
                } catch (...) {
                    cerr << "Error: Unknown argument" << endl;
                    usage(argv[0]);
                    return 1;
                }
            } else {
                cerr << "Error: Unknown argument" << endl;
                usage(argv[0]);
                return 1;
            }
        }
    }

    Instance inst;
    inst_file >> inst;
    vector<Column> cols = Heuristics::closestCustomersCols(inst);
    for (Column c : cols) {
        cout << c;
    }
    ColGenModel model(inst, time_limit, verbose);
    model.solve();
    return 0;
}