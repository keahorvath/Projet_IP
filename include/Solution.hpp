#ifndef SOLUTION_HPP
#define SOLUTION_HPP

#include <fstream>
#include <iostream>
#include <string>

#include "Point2D.hpp"

/**
 * @brief A solution is represented as a vector of facility positions
 * (where each line represents the coordinates of the opened facility associated with the customer)
 */
using Solution = std::vector<Point2D>;

/**
 * @brief Export the solution in a .sol file
 */
void exportSolution(Solution sol, std::string sol_name);

/**
 * @brief Override the << operator
 */
std::ostream& operator<<(std::ostream& out, const Solution& sol);

/**
 * @brief Override the >> operator
 */
std::istream& operator>>(std::istream& in, Solution& sol);

#endif