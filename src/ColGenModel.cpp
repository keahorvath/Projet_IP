#include <fstream>
#include <iostream>
#include <string>

#include "Heuristics.hpp"
#include "Instance.hpp"
#include "Solution.hpp"
#include "gurobi_c++.h"
using namespace std;

/**
 * @struct struct that contains method to solve problem with a compact formulation
 */
struct ColGenModel {
    GRBEnv* env;
    GRBModel* model;
    int time_limit;
    bool verbose;
    Instance inst;

    vector<vector<GRBVar>> x;
    vector<GRBVar> y;

    GRBConstr facility_nb_cap;
    vector<GRBConstr> cust_assignments;
    vector<GRBConstr> demand_caps;

    GRBLinExpr objective;

    /**
     * @brief Instanciate Model: create variables, constraints and objective
     */
    ColGenModel(const Instance& inst_, int time_limit_ = 300, bool verbose_ = false) : inst(inst_), time_limit(time_limit_), verbose(verbose_) {
        env = new GRBEnv(true);
        if (!verbose) {
            env->set(GRB_IntParam_LogToConsole, 0);
        }

        env->start();

        model = new GRBModel(*env);

        // VARIABLES
        x.resize(inst.nb_potential_facilities);
        y.resize(inst.nb_potential_facilities);

        for (int f = 0; f < inst.nb_potential_facilities; f++) {
            x[f].resize(inst.nb_potential_facilities);
            // y_f equals 1 if facility f is open
            // 0 otherwise
            y[f] = model->addVar(0.0, 1.0, 0.0, GRB_BINARY, "y_" + to_string(f));
            for (int c = 0; c < inst.nb_customers; c++) {
                // x_f_c equals 1 if customer c is supplied by facility f
                // 0 otherwise
                x[f][c] = model->addVar(0.0, 1.0, 0.0, GRB_BINARY, "x_" + to_string(f) + "_" + to_string(c));
            }
        }

        // CONSTRAINTS
        // Max number of open facilities
        GRBLinExpr expr = 0;
        for (int f = 0; f < inst.nb_potential_facilities; f++) {
            expr += y[f];
        }
        facility_nb_cap = model->addConstr(expr <= inst.nb_max_open_facilities, "no more open facilities than allowed");

        // Each customer is assigned to one facility
        cust_assignments.resize(inst.nb_customers);
        for (int c = 0; c < inst.nb_customers; c++) {
            expr = 0;
            for (int f = 0; f < inst.nb_potential_facilities; f++) {
                expr += x[f][c];
            }
            cust_assignments[c] = model->addConstr(expr == 1, "assign once customer " + to_string(c));
        }

        // Demand isn't more than capacity
        demand_caps.resize(inst.nb_potential_facilities);
        for (int f = 0; f < inst.nb_potential_facilities; f++) {
            expr = 0;
            for (int c = 0; c < inst.nb_customers; c++) {
                expr += x[f][c] * inst.customer_demands[c];
            }
            demand_caps[f] = model->addConstr(expr <= inst.facility_capacities[f] * y[f], "facility " + to_string(f) + " capacity not exceeded");
        }

        // OBJECTIVE
        expr = 0;
        for (int f = 0; f < inst.nb_potential_facilities; f++) {
            for (int c = 0; c < inst.nb_customers; c++) {
                double dist = distance(inst.customer_positions[c], inst.facility_positions[f]);
                expr += x[f][c] * dist;
            }
        }
        objective = expr;
        model->setObjective(objective);
    }

    Solution convertSolution() {
        Solution sol;
        for (int c = 0; c < inst.nb_customers; c++) {
            for (int f = 0; f < inst.nb_potential_facilities; f++) {
                if (x[f][c].get(GRB_DoubleAttr_X) == 1) {
                    Point2D assignment = {inst.facility_positions[f]};
                    sol.push_back(assignment);
                }
            }
        }
        return sol;
    }

    Solution solve() {
        model->set(GRB_DoubleParam_TimeLimit, time_limit);
        model->optimize();

        // Check solution status and print results
        int status = model->get(GRB_IntAttr_Status);
        if (status == GRB_OPTIMAL) {
            cout << "-----------------------" << endl;
            cout << "OPTIMAL SOLUTION FOUND!" << endl;
            cout << "-----------------------" << endl;
            cout << "Runtime : " << model->get(GRB_DoubleAttr_Runtime) << "s" << endl;
            cout << "Optimal solution value : " << model->get(GRB_DoubleAttr_ObjVal) << endl;
            return convertSolution();
        } else if (status == GRB_TIME_LIMIT) {
            cout << "--------------------------------------------" << endl;
            cout << "NO OPTIMAL SOLUTION FOUND WITHIN TIME LIMIT!" << endl;
            cout << "--------------------------------------------" << endl;
            cout << "Runtime : " << model->get(GRB_DoubleAttr_Runtime) << "s" << endl;
            cout << "Best solution value : " << model->get(GRB_DoubleAttr_ObjVal) << endl;
            return convertSolution();
        } else {
            cout << "---------------------------" << endl;
            cerr << "NO FEASIBLE SOLUTION FOUND!" << endl;
            cout << "---------------------------" << endl;
            return Solution();
        }
        return convertSolution();
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
    // Test heuristic to find first valid cols
    vector<Column> cols = Heuristics::generateBasicCols(inst);
    for (Column col : cols) {
        cout << col;
    }
    return 0;
}