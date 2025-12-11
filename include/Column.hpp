#ifndef COLUMN_HPP
#define COLUMN_HPP

#include <fstream>
#include <functional>
#include <iostream>
#include <vector>

/**
 * @brief A column is represented by a facility and a set of customers that are assigned to it
 */
using Column = std::pair<int, std::vector<int>>;

/**
 *  For debugging purposes
 */
std::ostream& operator<<(std::ostream& out, const Column& col);

#endif