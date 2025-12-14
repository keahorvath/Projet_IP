#ifndef COMPACTMODEL_HPP
#define COMPACTMODEL_HPP

#include <vector>

#include "Instance.hpp"
#include "gurobi_c++.h"

/**
 * @struct struct that contains method to solve problem with a compact formulation
 */
struct CompactModel {
    GRBEnv* env;
    GRBModel* model;
    GRBModel* relaxed_model;

    bool verbose;
    Instance inst;

    std::vector<std::vector<GRBVar>> x;
    std::vector<GRBVar> y;

    GRBConstr facility_nb_cap;
    std::vector<GRBConstr> cust_assignments;
    std::vector<GRBConstr> demand_caps;

    GRBLinExpr objective;

    /**
     * @brief Instanciate Model: create variables, constraints and objective
     */
    CompactModel(const Instance& inst_, bool verbose_ = false);

    Solution convertSolution();

    void solve(int time_limit);

    void solveRelaxation(int time_limit);

    void printResult();

    ~CompactModel();
};

#endif