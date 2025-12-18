#ifndef INSTANCE_HPP
#define INSTANCE_HPP
#include <fstream>
#include <string>
#include <unordered_set>

#include "Point2D.hpp"
#include "Solution.hpp"

/**
 * @struct Instance
 * @brief Struct that contains all the information and methods related to an instance of the problem
 */
struct Instance {
    int nb_customers;
    int nb_potential_facilities;
    int nb_max_open_facilities;
    int max_cap_new_depots;
    std::vector<Point2D> customer_positions;
    std::vector<int> customer_demands;
    std::vector<Point2D> facility_positions;
    std::vector<int> facility_capacities;

    /**
     * @brief Checks if the instance is feasible
     */
    bool isFeasible();

    /**
     * @brief Checks if the given solution is valid
     */
    bool checker(const Solution& sol);

    /**
     * @brief Returs the value of the given solution
     * (returns +inf if solution is not valid)
     */
    double objective_value(const Solution& sol);

    /**
     * @brief Given the position of a facility, return its corresponding index in the facility_positions vector
     * return -1 if the position isn't a facility position
     */
    int get_facility_index(const Point2D& pos);

    /**
     * @brief Create an SVG file to visualize an instance/solution
     */
    void visualize(const Solution& sol, std::string instance_name);

    /**
     * @brief Override the << operator
     */
    friend std::ostream& operator<<(std::ostream& out, const Instance& inst);

    /**
     * @brief Override the >> operator
     */
    friend std::istream& operator>>(std::istream& in, Instance& inst);
};

#endif