#ifndef DIVINGHEURISTIC_HPP
#define DIVINGHEURISTIC_HPP

#include <vector>

#include "ColGenModel.hpp"
#include "Column.hpp"

/**
 * @struct Contains all the methods and attributes needed to calcule a valid integer solution for the problem
 * using a basic diving heuristic
 */
struct DivingHeuristic {
    ColGenModel& model;  // model address so we can modify it from here

    std::vector<int> forced_facility_for_client;  // keep in memory which assignments are forced

    /**
     * @brief Constructor for the DivingHeursitic structure
     */
    DivingHeuristic(ColGenModel& model);

    /**
     * @brief Return the best Facility-Customer pair:
     *
     * get the fictional x_fc values for the current state of the model
     * and return the fc pair with the highest NON INTEGER value
     */
    std::pair<int, int> getBestFCPair();

    /**
     * @brief Modify the model to prohibid all the columns that place the given customer with a facility that isn't the given one
     */
    void prohibidCols(int customer, int facility);

    /**
     * @brief Solve the pricing problem : similar to the one is ColGenModel but adapted for the diving heuristic
     *
     * NOTE: Should have used the in out separation version but more it's more complicated and I started having bugs and I was running out of time!
     * So i used the simple pricing version
     * @return the columns to add to the master
     */
    std::vector<Column> pricing();

    /**
     * @brief Solve the pricing sub problem for given facility: similar to the one is ColGenModel (dynamic programming) but adapted for the diving
     * heuristic
     */
    std::pair<double, Column> pricingSubProblem(int facility, double theta, const std::vector<double>& pi);

    /**
     * @brief Convert the current model state to a valid (checks if solution is valid first)
     */
    Solution convertSolution();

    /**
     * @brief Solve diving heuristic
     *
     * NOTE: the time limit is only used for the initial model solve
     * (was too lazy to handle what happens if no integer sol found in time limit since,
     * in practice, I saw that finding an integer sol was pretty fast for all our instances)
     */
    void solve(int time_limit);
};
#endif