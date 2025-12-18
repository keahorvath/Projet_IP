#include <fstream>
#include <iostream>
#include <string>

#include "ColGenModel.hpp"
#include "DivingHeuristic.hpp"
#include "Heuristics.hpp"
#include "Instance.hpp"
using namespace std;

void usage(const string& prog_name) {
    cout << "Usage: " << prog_name << " file_path [time_limit]" << endl;
    cout << "  file_path       : path to the input instance file" << endl;
    cout << "  time_limit      : maximum execution time in seconds (optional), default is 300s" << endl;
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
    if (argc == 3) {
        bool has_time_limit = false;
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
    } else if (argc > 3) {
        usage(argv[0]);
        return 1;
    }

    Instance inst;
    inst_file >> inst;
    if (!inst.isFeasible()) {
        cout << "Instance " << file_name << " is infeasible" << endl;
        return 0;
    }

    cout << "Solving model using diving heuristic..." << endl;
    ColGenModel model(inst, PricingMethod::DP, ColumnStrategy::MULTI, Stabilization::INOUT);
    DivingHeuristic diving(model);
    diving.solve(time_limit);
    diving.printResult();
    return 0;
}