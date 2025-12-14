#include <fstream>
#include <iostream>
#include <string>

#include "ColGenModel.hpp"
#include "Heuristics.hpp"
#include "Instance.hpp"

using namespace std;

void usage(const string& prog_name) {
    cout << "Usage: " << prog_name << " file_path [time_limit] [pricing_method] [column_strategy] [-v]" << endl;
    cout << "  file_path       : path to the input instance file" << endl;
    cout << "  time_limit      : maximum execution time in seconds (optional), default is 300s" << endl;
    cout << "  pricing_method  : MIP or DP (optional), default is MULTI" << endl;
    cout << "  column_strategy : SINGLE or MULTI (optional), default is MULTI" << endl;
    cout << "  -v             : add to enable verbose output (optional)" << endl;
}

int main(int argc, char** argv) {
    int time_limit = 300;
    bool verbose = false;
    PricingMethod pricing_method = PricingMethod::DP;
    ColumnStrategy column_strategy = ColumnStrategy::MULTI;

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
    cout << "Solving model ..." << endl;
    vector<Column> cols = Heuristics::closestCustomersCols(inst);
    for (Column c : cols) {
        cout << c;
    }
    ColGenModel model(inst, pricing_method, column_strategy, verbose);
    int nb_cols = model.solve(time_limit);
    model.printResult();

    return 0;
}