#include "CompactModel.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "Instance.hpp"
#include "Solution.hpp"
#include "gurobi_c++.h"

using namespace std;

CompactModel::CompactModel(const Instance& inst_, bool verbose_) : inst(inst_), verbose(verbose_) {
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
    facility_nb_cap = model->addConstr(expr, GRB_LESS_EQUAL, inst.nb_max_open_facilities, "no more open facilities than allowed");

    // Each customer is assigned to one facility
    cust_assignments.resize(inst.nb_customers);
    for (int c = 0; c < inst.nb_customers; c++) {
        expr = 0;
        for (int f = 0; f < inst.nb_potential_facilities; f++) {
            expr += x[f][c];
        }
        cust_assignments[c] = model->addConstr(expr, GRB_EQUAL, 1, "assign once customer " + to_string(c));
    }

    // Demand isn't more than capacity
    demand_caps.resize(inst.nb_potential_facilities);
    for (int f = 0; f < inst.nb_potential_facilities; f++) {
        expr = 0;
        for (int c = 0; c < inst.nb_customers; c++) {
            expr += x[f][c] * inst.customer_demands[c];
        }
        demand_caps[f] =
            model->addConstr(expr, GRB_LESS_EQUAL, inst.facility_capacities[f] * y[f], "facility " + to_string(f) + " capacity not exceeded");
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

    // Initialize relaxed model
    model->update();
    relaxed_model = new GRBModel(model->relax());
}

Solution CompactModel::convertSolution() {
    // If no valid solution, return empty sol
    if (model->get(GRB_IntAttr_SolCount) == 0) {
        return Solution();
    }
    // Otherwise, convert and return solution
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

void CompactModel::solve(int time_limit) {
    model->set(GRB_DoubleParam_TimeLimit, time_limit);
    model->optimize();
}

void CompactModel::solveRelaxation(int time_limit) {
    relaxed_model->set(GRB_DoubleParam_TimeLimit, time_limit);
    relaxed_model->optimize();
}

void CompactModel::printResult() {
    int status = model->get(GRB_IntAttr_Status);
    double runtime = model->get(GRB_DoubleAttr_Runtime);
    double obj_val = model->get(GRB_DoubleAttr_ObjVal);
    int status_relaxed = relaxed_model->get(GRB_IntAttr_Status);
    double runtime_relaxed = relaxed_model->get(GRB_DoubleAttr_Runtime);
    double obj_val_relaxed = relaxed_model->get(GRB_DoubleAttr_ObjVal);
    if (status == GRB_OPTIMAL) {
        cout << "-----------------------" << endl;
        cout << "OPTIMAL SOLUTION FOUND!" << endl;
        cout << "-----------------------" << endl;
        cout << "Optimal solution value : " << obj_val << " (" << runtime << "s)" << endl;
    } else if (status == GRB_TIME_LIMIT) {
        cout << "--------------------------------------------" << endl;
        cout << "NO OPTIMAL SOLUTION FOUND WITHIN TIME LIMIT!" << endl;
        cout << "--------------------------------------------" << endl;
        cout << "Best solution value : " << obj_val << " (" << runtime << "s)" << endl;
        cout << "Dual Bound : " << model->get(GRB_DoubleAttr_ObjBound) << endl;
    } else {
        cout << "---------------------------" << endl;
        cerr << "NO FEASIBLE SOLUTION FOUND!" << endl;
        cout << "---------------------------" << endl;
    }
    if (status_relaxed == GRB_OPTIMAL) {
        cout << "Optimal relaxation value : " << obj_val_relaxed << " (" << runtime_relaxed << "s)" << endl;
    } else if (status_relaxed == GRB_TIME_LIMIT) {
        cout << "Best relaxation value : " << obj_val_relaxed << " (" << runtime_relaxed << "s)" << endl;
    } else {
        cout << "No feasible relaxed solution found" << endl;
    }
}

CompactModel::~CompactModel() {
    delete env;
    delete model;
    delete relaxed_model;
}
