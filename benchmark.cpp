#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "ColGenModel.hpp"
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

vector<string> getValidInstances(vector<string> file_paths) {
    vector<string> valid_instances;
    for (const string& file_path : file_paths) {
        string file_name_clean = fs::path(file_path).stem().string();

        ifstream inst_file(file_path);
        Instance inst;
        inst_file >> inst;
        if (!inst.isFeasible()) {
            cout << "Instance " << file_name_clean << " is infeasible : SKIPPING" << endl;
            continue;
        }
        valid_instances.push_back(file_path);
    }
    return valid_instances;
}

void compactModelResults(vector<string> file_paths, string csv_file, int time_limit) {
    // Create csv file
    ofstream file(csv_file);
    if (!file.is_open()) {
        cerr << "Error : Couldn't create file " << csv_file << endl;
        return;
    }

    file << "Instance;Opt Found?;Best Sol;Dual Bound;Gap;Duration(s); Relax Sol; Relax Gap; Duration(s)" << endl;
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

            // Solve instance
            CompactModel solver(inst);
            solver.solve(time_limit);
            solver.solveRelaxation(time_limit);

            // Get results from Gurobi
            int status = solver.model->get(GRB_IntAttr_Status);
            int sol_count = solver.model->get(GRB_IntAttr_SolCount);

            // If no solution, skip
            if (sol_count == 0) {
                cout << "No solution found -> skipping" << endl;
                file << file_name_clean << ";NO_SOL;;;;;;;" << endl;
                continue;
            }

            // Check validity
            Solution sol = solver.convertSolution();
            bool check = inst.checker(sol);
            if (!check) {
                cout << "Solution is NOT valid -> skipping" << endl;
                file << file_name_clean << ";INVALID;;;;;;;" << endl;
                continue;
            }

            double best_sol = solver.model->get(GRB_DoubleAttr_ObjVal);
            double dual_bound = solver.model->get(GRB_DoubleAttr_ObjBound);
            double gap = solver.model->get(GRB_DoubleAttr_MIPGap);
            bool found_opt = (status == GRB_OPTIMAL);
            double relax_sol = solver.relaxed_model->get(GRB_DoubleAttr_ObjVal);
            double relax_gap = (best_sol - relax_sol) / best_sol;
            double runtime = solver.model->get(GRB_DoubleAttr_Runtime);
            double relax_runtime = solver.relaxed_model->get(GRB_DoubleAttr_Runtime);

            // Write in csv file
            file << file_name_clean << ";" << (found_opt ? "YES" : "NO") << ";" << fixed << setprecision(4) << best_sol << ";" << fixed
                 << setprecision(4) << dual_bound << ";" << fixed << setprecision(2) << (gap * 100) << "%" << ";" << fixed << setprecision(4)
                 << runtime << ";" << fixed << setprecision(4) << relax_sol << ";" << fixed << setprecision(2) << (relax_gap * 100) << "%"
                 << ";" << fixed << setprecision(4) << relax_runtime << endl;

            cout << "DONE! (" << runtime + relax_runtime << "s)" << endl;

        } catch (GRBException& e) {
            cerr << "GUROBI error : " << e.getMessage() << endl;
            file << file_name_clean << ";ERROR;;;;;" << endl;
        }
    }
    file.close();
    cout << "=== END OF BENCHMARK. Results are in " << csv_file << " ===" << endl;
}

void singleVsMulti(vector<string> file_paths, string csv_file, int time_limit) {
    // Create csv file
    ofstream file(csv_file);
    if (!file.is_open()) {
        cerr << "Error : Couldn't create file " << csv_file << endl;
        return;
    }

    file << "Instance;SINGLE Value;Nb cols; Durations(s);MULTI Value;Nb cols;Duration(s)" << endl;
    cout << "=== STARTING COLUMN STRATEGY BENCHMARK ===" << endl;

    for (const string& file_path : file_paths) {
        // Remove
        string file_name_clean = fs::path(file_path).stem().string();

        cout << "Solving instance : " << file_name_clean << " ... " << flush;

        try {
            // Loading instance
            ifstream inst_file(file_path);
            Instance inst;
            inst_file >> inst;

            // Solve SINGLE instance
            ColGenModel single_solver(inst, PricingMethod::MIP, ColumnStrategy::SINGLE);
            int single_nb_cols = single_solver.solve(time_limit);
            double single_runtime = single_solver.runtime;
            bool single_TLR = single_runtime > time_limit;  // TLR:time limit reached

            // Solve MULTI instance
            ColGenModel multi_solver(inst, PricingMethod::MIP, ColumnStrategy::MULTI);
            int multi_nb_cols = multi_solver.solve(time_limit);
            double multi_runtime = multi_solver.runtime;
            bool multi_TLR = multi_runtime > time_limit;  // TLR:time limit reached

            // We should make sure that we got valid solutions for each model and handle errors but.... flemme

            double multi_best_sol = multi_solver.model->get(GRB_DoubleAttr_ObjVal);
            double single_best_sol = single_solver.model->get(GRB_DoubleAttr_ObjVal);

            // Write in csv file
            file << file_name_clean << ";" << fixed << setprecision(4) << single_best_sol << (single_TLR ? "(TLR)" : "") << ";" << single_nb_cols
                 << ";" << fixed << setprecision(2) << single_runtime << ";" << fixed << setprecision(4) << multi_best_sol
                 << (multi_TLR ? "(TLR)" : "") << ";" << multi_nb_cols << ";" << fixed << setprecision(2) << multi_runtime << endl;
            cout << "DONE! (" << single_runtime + multi_runtime << "s)" << endl;
        } catch (...) {
            cerr << "Unknown error on " << file_name_clean << endl;
            file << file_name_clean << ";CRASH;;;;" << endl;
        }
    }
    file.close();
    cout << "=== END OF BENCHMARK. Results are in " << csv_file << " ===" << endl;
}

void comparePricingMethods(vector<string> file_paths, string csv_file, int time_limit) {
    // Create csv file
    ofstream file(csv_file);
    if (!file.is_open()) {
        cerr << "Error : Couldn't create file " << csv_file << endl;
        return;
    }

    file << "Instance;MIP Value;Nb cols; Durations(s);DP Value;Nb cols;Duration(s)" << endl;
    cout << "=== STARTING PRICING METHOD BENCHMARK ===" << endl;

    for (const string& file_path : file_paths) {
        // Remove
        string file_name_clean = fs::path(file_path).stem().string();

        cout << "Solving instance : " << file_name_clean << " ... " << flush;

        try {
            // Loading instance
            ifstream inst_file(file_path);
            Instance inst;
            inst_file >> inst;

            // Solve MIP instance
            ColGenModel mip_solver(inst, PricingMethod::MIP, ColumnStrategy::MULTI);
            int mip_nb_cols = mip_solver.solve(time_limit);
            double mip_runtime = mip_solver.runtime;
            bool mip_TLR = mip_runtime > time_limit;  // TLR:time limit reached

            // Solve DP instance
            ColGenModel dp_solver(inst, PricingMethod::DP, ColumnStrategy::MULTI);
            int dp_nb_cols = dp_solver.solve(time_limit);
            double dp_runtime = dp_solver.runtime;
            bool dp_TLR = dp_runtime > time_limit;  // TLR:time limit reached

            // We should make sure that we got valid solutions for each model and handle errors but.... flemme

            double mip_best_sol = mip_solver.model->get(GRB_DoubleAttr_ObjVal);
            double dp_best_sol = dp_solver.model->get(GRB_DoubleAttr_ObjVal);

            // Write in csv file
            file << file_name_clean << ";" << fixed << setprecision(4) << mip_best_sol << (mip_TLR ? "(TLR)" : "") << ";" << mip_nb_cols << ";"
                 << fixed << setprecision(2) << mip_runtime << ";" << fixed << setprecision(4) << dp_best_sol << (dp_TLR ? "(TLR)" : "") << ";"
                 << dp_nb_cols << ";" << fixed << setprecision(2) << dp_runtime << endl;

            cout << "DONE! (" << mip_runtime + dp_runtime << "s)" << endl;
        } catch (...) {
            cerr << "Unknown error on " << file_name_clean << endl;
            file << file_name_clean << ";CRASH;;;;;" << endl;
        }
    }
    file.close();
    cout << "=== END OF BENCHMARK. Results are in " << csv_file << " ===" << endl;
}

void compareWithAndWithoutStabilization(vector<string> file_paths, string csv_file, int time_limit) {
    // Create csv file
    ofstream file(csv_file);
    if (!file.is_open()) {
        cerr << "Error : Couldn't create file " << csv_file << endl;
        return;
    }

    file << "Instance;NOSTAB Value;Nb cols; Durations(s);INOUT Value;Nb cols;Duration(s)" << endl;
    cout << "=== STARTING STABILIZATION BENCHMARK ===" << endl;

    for (const string& file_path : file_paths) {
        // Remove
        string file_name_clean = fs::path(file_path).stem().string();

        cout << "Solving instance : " << file_name_clean << " ... " << flush;

        try {
            // Loading instance
            ifstream inst_file(file_path);
            Instance inst;
            inst_file >> inst;

            // Solve no stabilization instance
            ColGenModel none_solver(inst, PricingMethod::DP, ColumnStrategy::MULTI, Stabilization::NONE);
            int none_nb_cols = none_solver.solve(time_limit);
            double none_runtime = none_solver.runtime;
            bool none_TLR = none_runtime > time_limit;  // TLR:time limit reached

            // Solve DP instance
            ColGenModel inout_solver(inst, PricingMethod::DP, ColumnStrategy::MULTI, Stabilization::INOUT);
            int inout_nb_cols = inout_solver.solve(time_limit);
            double inout_runtime = inout_solver.runtime;
            bool inout_TLR = inout_runtime > time_limit;  // TLR:time limit reached

            // We should make sure that we got valid solutions for each model and handle errors but.... flemme

            double none_best_sol = none_solver.model->get(GRB_DoubleAttr_ObjVal);
            double inout_best_sol = inout_solver.model->get(GRB_DoubleAttr_ObjVal);

            // Write in csv file
            file << file_name_clean << ";" << fixed << setprecision(4) << none_best_sol << (none_TLR ? "(TLR)" : "") << ";" << none_nb_cols << ";"
                 << fixed << setprecision(2) << none_runtime << ";" << fixed << setprecision(4) << inout_best_sol << (inout_TLR ? "(TLR)" : "") << ";"
                 << inout_nb_cols << ";" << fixed << setprecision(2) << inout_runtime << endl;

            cout << "DONE! (" << none_runtime + inout_runtime << "s)" << endl;
        } catch (...) {
            cerr << "Unknown error on " << file_name_clean << endl;
            file << file_name_clean << ";CRASH;;;;;" << endl;
        }
    }
    file.close();
    cout << "=== END OF BENCHMARK. Results are in " << csv_file << " ===" << endl;
}

int main(int argc, char** argv) {
    vector<string> file_paths = getSortedFiles("../instances");
    vector<string> valid_paths = getValidInstances(file_paths);
    compactModelResults(valid_paths, "compact_model.csv", 600);
    //  singleVsMulti(file_paths, "single_vs_multi.csv", 60);
    //  comparePricingMethods(file_paths, "pricing_method.csv", 60);
    // compareWithAndWithoutStabilization(valid_paths, "with_without_stabilization.csv", 60);
    return 1;
}
