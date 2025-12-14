#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "CompactModel.hpp"
using namespace std;
namespace fs = std::filesystem;

vector<string> getSortedFiles(string data_folder) {
    // Done with the help of an LLM
    vector<string> file_paths;

    for (const auto& entry : fs::directory_iterator(data_folder)) {
        if (entry.path().filename().string()[0] != '.') {
            file_paths.push_back(entry.path().string());
        }
    }
    // Personnalize sort to make sure that uniform_10.inst isn't before uniform_2.inst (for example)
    sort(file_paths.begin(), file_paths.end(), [](const string& a, const string& b) {
        string nameA = fs::path(a).stem().string();
        string nameB = fs::path(b).stem().string();

        auto getNumber = [](const string& s) {
            string numStr = "";
            for (char c : s) {
                if (isdigit(c)) numStr += c;
            }
            try {
                return numStr.empty() ? 0 : stoi(numStr);
            } catch (...) {
                return 0;
            }
        };

        int numA = getNumber(nameA);
        int numB = getNumber(nameB);

        if (numA != numB) {
            return numA < numB;
        }

        return nameA < nameB;
    });

    return file_paths;
}

void compactModelResults(vector<string> file_paths, string csv_file, int time_limit) {
    // Create csv file
    ofstream file(csv_file);
    if (!file.is_open()) {
        cerr << "Error : Couldn't create file " << csv_file << endl;
        return;
    }

    file << "Instance;Opt Found?;Best Sol;Dual Bound;Gap;Duration(s); Relax Sol\n";
    cout << "=== STARTING COMPACT MODEL BENCHMARK ===" << endl;

    for (const string& file_path : file_paths) {
        // Remove
        string file_name_clean = fs::path(file_path).stem().string();

        cout << "Solving instance : " << file_name_clean << " ... " << flush;

        try {
            // Loading instance
            ifstream inst_file(file_path);
            Instance inst;
            inst_file >> inst;

            auto start = chrono::high_resolution_clock::now();

            // Solve instance
            CompactModel solver(inst);
            solver.solve(time_limit);
            solver.solveRelaxation(time_limit);

            auto end = chrono::high_resolution_clock::now();
            chrono::duration<double> runtime = end - start;

            // Get results from Gurobi
            int status = solver.model->get(GRB_IntAttr_Status);
            int sol_count = solver.model->get(GRB_IntAttr_SolCount);

            // If no solution, skip
            if (sol_count == 0) {
                cout << "No solution found -> skipping" << endl;
                file << file_name_clean << ";NO_SOL;;;;\n";
                continue;
            }

            // Check validity
            Solution sol = solver.convertSolution();
            bool check = inst.checker(sol);
            if (!check) {
                cout << "Solution is NOT valid -> skipping" << endl;
                file << file_name_clean << ";INVALID;;;;\n";
                continue;
            }

            double best_sol = solver.model->get(GRB_DoubleAttr_ObjVal);
            double dual_bound = solver.model->get(GRB_DoubleAttr_ObjBound);
            double gap = solver.model->get(GRB_DoubleAttr_MIPGap);
            bool found_opt = (status == GRB_OPTIMAL);
            double relax_sol = solver.relaxed_model->get(GRB_DoubleAttr_ObjVal);
            // Write in csv file
            file << file_name_clean << ";" << (found_opt ? "YES" : "NO") << ";" << fixed << setprecision(4) << best_sol << ";" << fixed
                 << setprecision(4) << dual_bound << ";" << fixed << setprecision(2) << (gap * 100) << "%" << ";" << fixed << setprecision(2)
                 << runtime.count() << ";" << relax_sol << endl;

            cout << "DONE! (" << runtime.count() << "s)" << endl;

        } catch (GRBException& e) {
            cerr << "GUROBI error : " << e.getMessage() << endl;
            file << file_name_clean << ";ERROR;;;;\n";
        } catch (...) {
            cerr << "Unknown error on " << file_name_clean << endl;
            file << file_name_clean << ";CRASH;;;;\n";
        }
    }
    file.close();
    cout << "=== END OF BENCHMARK. Results are in " << csv_file << " ===" << endl;
}

void compactModelResults(int time_limit);

void compareOneColPerFacilityOrOneCol();

void compareLPRelaxedCompactAndBasicColGen();

void compareMIPpricingAndDPpricing();

void compareWithAndWithoutStabilization();

int main(int argc, char** argv) {
    vector<string> file_paths = getSortedFiles("../instances");
    compactModelResults(file_paths, "compact_model_res.csv", 60);
    return 1;
}
