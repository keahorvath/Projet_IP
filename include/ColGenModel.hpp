#ifndef COLGENMODEL_HPP
#define COLGENMODEL_HPP
#include <gurobi_c++.h>

#include <utility>

#include "Column.hpp"
#include "Instance.hpp"

// Col Gen Parameters
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

    // Variables
    std::vector<GRBVar> lambda;
    std::vector<Column> model_cols;  // used for diving

    // Constraints
    GRBConstr theta_constr;
    std::vector<GRBConstr> pi_constrs;

    // For stabilization
    double theta_center;
    std::vector<double> pi_center;
    double stab_alpha = 0.5;
    double best_LB;  // Lagrangian bound

    /**
     * @brief Instanciate Relaxed Master Problem: create constraints and create initial cols to make a feasible solution
     * default pricing method and column strategy are set to the best (found after testing): DP an dMULTI and INOOUT stabilization
     */
    ColGenModel(const Instance& inst_, PricingMethod pricing_method = PricingMethod::DP, ColumnStrategy column_strategy = ColumnStrategy::MULTI,
                Stabilization stabilization = Stabilization::INOUT, bool verbose_ = false);

    /**
     * @brief Take a column and add it the RMP, also update the column storage vector
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

    /**
     * @brief Get the value of the separation theta (using formula seen in class)
     */
    double getSeparationTheta();

    /**
     * @brief Get the value of the separation pi (using formula seen in class)
     */
    std::vector<double> getSeparationPi();

    /**
     * @brief Get the value of the objection function in the current state of the model
     */
    double obj();

    /**
     * @brief Optimize the model
     */
    void optimize();

    /**
     * @brief Get the reduced costs of each customer associated with given facility
     * and the dual vector pi to consider
     */
    std::vector<double> reducedCosts(int facility, std::vector<double> pi);

    /**
     * @brief Solve the pricing sub problem associated with given facility and given dual values
     * using a mip solver
     * @return a pair containing the best reduced cost found and the best column found

     */
    std::pair<double, Column> pricingSubProblemMIP(int facility, double theta, std::vector<double> pi);

    /**
     * @brief Solve the pricing sub problem associated with given facility and given dual values
     * using a dynammic programming approach
     * @return a pair containing the best reduced cost found and the best column found
     */
    std::pair<double, Column> pricingSubProblemDP(int facility, double theta, std::vector<double> pi);

    /**
     * @brief Solve the current pricing problem
     * @return a vector with all of the columns to add to the master problem
     */
    std::vector<Column> pricing();

    // NOTE: I could have handled the in out separation technique in the pricing() method directly,
    // but i thought it would be easier to understand the code if in separate methods
    /**
     * @brief Solve the current pricing problem using the in out separation technique
     * @return a vector with all of the columns to add to the master problem
     */
    std::vector<Column> inOutPricing();

    /**
     * @brief solve the column generation
     * @return the number of columns in the model
     */
    int solve(int time_limit);

    /**
     * @brief print the result in the terminal
     */
    void printResult();

    ~ColGenModel();
};
#endif