#ifndef COLGENMODEL_HPP
#define COLGENMODEL_HPP
#include <gurobi_c++.h>

#include <utility>

#include "Column.hpp"
#include "Instance.hpp"

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

    std::vector<GRBVar> lambda;

    GRBConstr col_cap;
    std::vector<GRBConstr> cust_assignments;

    /**
     * @brief Instanciate Relaxed Master Problem: create constraints and create initial cols to make a feasible solution
     */
    ColGenModel(const Instance& inst_, int time_limit_ = 300, bool verbose_ = false);

    /**
     * @brief Take a column and add it the RMP
     */
    void addColumn(Column col);

    /**
     * @brief Get the value of theta
     */
    double colCapDual();

    /**
     * @brief Get the values of pi_c
     */
    std::vector<double> custAssignmentsDuals();

    double obj();

    void optimize();

    std::vector<double> reducedCosts(int facility);

    std::pair<double, Column> pricingSubProblemMIP(int facility);

    std::pair<double, Column> pricingSubProblemDP(int facility);

    std::vector<Column> pricing(bool one_col_per_sub_pb, bool mip);

    Solution convertSolution();

    void solve(bool print_res, bool one_col_per_sub_pb, bool mip);
    ~ColGenModel();
};
#endif