#ifndef COLUMN_HPP
#define COLUMN_HPP

#include <fstream>
#include <functional>
#include <iostream>
#include <vector>

#include "Instance.hpp"

/**
 * @brief A column is represented by a facility and a set of customers that are assigned to it
 */
struct Column {
    int facility;
    std::vector<int> customers;

    /**
     * @brief default constructor -> creates an empty (invalid) column
     */
    Column();

    /**
     * @brief Constructor: creates a column with given parameters
     */
    Column(int facility, std::vector<int> customers);

    /**
     * @brief Get the cost associated with the column
     * (sum of the distances of each customer to the facility)
     */
    double cost(const Instance& inst);

    /**
     * @brief Override the << operator
     */
    friend std::ostream& operator<<(std::ostream& out, const Column& inst);
};

#endif