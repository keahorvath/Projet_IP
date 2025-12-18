#include <fstream>
#include <iostream>
#include <string>

#include "ColGenModel.hpp"
#include "DivingHeuristic.hpp"
#include "Heuristics.hpp"
#include "Instance.hpp"
using namespace std;

void usage(const string& prog_name) {
    cout << "Usage: " << prog_name << " file_path [time_limit] [pricing_method] [column_strategy] [stabilization] [-v]" << endl;
    cout << "  file_path       : path to the input instance file" << endl;
    cout << "  time_limit      : maximum execution time in seconds (optional), default is 300s" << endl;
    cout << "  pricing_method  : MIP or DP (optional), default is MULTI" << endl;
    cout << "  column_strategy : SINGLE or MULTI (optional), default is MULTI" << endl;
    cout << "  stabilization   : INOUT or NONE (optional), default is INOUT" << endl;
    cout << "  -v              : add to enable verbose output (optional)" << endl;
}

int main(int argc, char** argv) {
    int time_limit = 300;
    bool verbose = false;
    // Default values (the best)
    PricingMethod pricing_method = PricingMethod::DP;
    ColumnStrategy column_strategy = ColumnStrategy::MULTI;
    Stabilization stabilization = Stabilization::INOUT;

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
            } else if (arg == "SINGLE") {
                column_strategy = ColumnStrategy::SINGLE;
            } else if (arg == "MIP") {
                pricing_method = PricingMethod::MIP;
            } else if (arg == "NONE") {
                stabilization = Stabilization::NONE;
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
    if (!inst.isFeasible()) {
        cout << "Instance " << file_name << " is infeasible" << endl;
        return 0;
    }

    cout << "Solving model ..." << endl;
    ColGenModel model(inst, pricing_method, column_strategy, stabilization, verbose);
    // model.solve(time_limit);
    // model.printResult();
    DivingHeuristic diving(model);
    diving.solve(60);

    // for (GRBVar var : model.lambda) {
    //     cout << var.get(GRB_StringAttr_VarName) << " = " << var.get(GRB_DoubleAttr_X) << endl;
    // }
    cout << "Value = " << model.obj() << endl;
    Solution sol = diving.convertSolution();
    if (inst.checker(sol)) {
        cout << "Solution is valid" << endl;
    } else {
        cout << "Solution in NOT valid" << endl;
    }
    return 0;
}