#include "Heuristics.hpp"

#include <numeric>
using namespace std;

vector<Column> Heuristics::pBiggestFacilities(const Instance& inst) {
    // Take p biggest facilities
    vector<int> facilities(inst.nb_potential_facilities);
    iota(facilities.begin(), facilities.end(), 0);

    // Sort the p facilities from biggest to smallest
    partial_sort(facilities.begin(), facilities.begin() + inst.nb_max_open_facilities, facilities.end(),
                 [&](int i, int j) { return inst.facility_capacities[i] > inst.facility_capacities[j]; });

    vector<int> p_facilities(facilities.begin(), facilities.begin() + inst.nb_max_open_facilities);

    vector<Column> cols;
    vector<int> customers;
    int current_facility = p_facilities[0];
    int current_index = 0;
    int current_demand = 0;
    // Simply put the customers in order (very stupid heuristic indeed)
    for (int c = 0; c < inst.nb_customers; c++) {
        if (current_demand >= inst.facility_capacities[current_facility]) {
            cols.push_back(Column(current_facility, customers));
            current_demand = 0;
            customers.clear();
            current_index++;
            current_facility = p_facilities[current_index];
        }
        customers.push_back(c);
        current_demand += inst.customer_demands[c];
    }
    if (customers.size() > 0) {
        cols.push_back(Column(current_facility, customers));
    }
    return cols;
}
