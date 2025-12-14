#include <fstream>
#include <iostream>
#include <string>

#include "ColGenModel.hpp"
#include "Heuristics.hpp"
#include "Instance.hpp"

using namespace std;

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
    model.solve(true, false, false);
    return 0;
}