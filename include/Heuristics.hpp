#ifndef HEURISTICS_HPP
#define HEURISTICS_HPP

#include <utility>
#include <vector>

#include "Column.hpp"
#include "Instance.hpp"
#include "Solution.hpp"

/**
 * @namespace this namespace contains all the methods that create:
 * - valid solutions
 * - valid sets of columns to initialize the column generation
 */
namespace Heuristics {

std::vector<Column> generateBasicCols(const Instance& inst);

}

#endif