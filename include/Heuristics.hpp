#ifndef HEURISTICS_HPP
#define HEURISTICS_HPP

#include <utility>
#include <vector>

#include "Column.hpp"
#include "Instance.hpp"
#include "Solution.hpp"

/**
 * @brief this namespace contains all the methods that create:
 *
 * - valid solutions
 *
 * - valid sets of columns to initialize the column generation
 *
 * NOTE: Only one for now (I was planning on making better ones but didn't have time :'( )
 */
namespace Heuristics {

/**
 * @brief Very simple heuristic: put all customers in the biggest facilities
 * @return the columns creating a valid solution
 */
std::vector<Column> pBiggestFacilities(const Instance& inst);
}  // namespace Heuristics

#endif