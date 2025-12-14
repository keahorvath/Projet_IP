#ifndef COLGENMODEL_HPP
#define COLGENMODEL_HPP
#include <gurobi_c++.h>

#include <utility>

#include "Column.hpp"
#include "Instance.hpp"

enum class PricingMethod { DP, MIP };
enum class ColumnStrategy { SINGLE, MULTI };
enum class Stabilization { NONE, INOUT };

/**
 * @struct struct that contains methods to solve the problem
 *         using a column generation approach
 */
struct ColGenModel {
    GRBEnv* env;
    GRBModel* model;
    bool verbose;
    Instance inst;

    // Pricing parameters
    PricingMethod pricing_method;
    ColumnStrategy column_strategy;
    Stabilization stabilization;

    // To keep in memory total elapsed time (multiple optimize())
    double runtime;

    std::vector<GRBVar> lambda;

    GRBConstr theta_constr;
    std::vector<GRBConstr> pi_constrs;

    // For stabilization
    double theta_center;
    std::vector<double> pi_center;
    double stab_alpha = 0.5;
    double best_LB;  // Lagrangian bound

    /**
     * @brief Instanciate Relaxed Master Problem: create constraints and create initial cols to make a feasible solution
     * default pricing method and column strategy are set to the best (found after testing): DP andMULTI
     */
    ColGenModel(const Instance& inst_, PricingMethod pricing_method = PricingMethod::DP, ColumnStrategy column_strategy = ColumnStrategy::MULTI,
                Stabilization stabilization = Stabilization::INOUT, bool verbose_ = false);

    /**
     * @brief Take a column and add it the RMP
     */
    void addColumn(Column col);

    /**
     * @brief Get the value of theta
     */
    double getTheta();

    /**
     * @brief Get the values of pi_c
     */
    std::vector<double> getPi();

    double getStabilizedTheta();

    std::vector<double> getStabilizedPi();

    double obj();

    void optimize();

    std::vector<double> reducedCosts(int facility, std::vector<double> pi);

    std::pair<double, Column> pricingSubProblemMIP(int facility, double theta, std::vector<double> pi);

    std::pair<double, Column> pricingSubProblemDP(int facility, double theta, std::vector<double> pi);

    std::vector<Column> pricing();

    Solution convertSolution();

    // return number of cols
    int solve(int time_limit);

    void printResult();

    ~ColGenModel();
};
#endif