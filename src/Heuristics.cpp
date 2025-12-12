#include "Heuristics.hpp"

#include <numeric>
using namespace std;

namespace Heuristics {

vector<Column> generateBasicCols(const Instance& inst) {
    vector<Column> cols;
    // Get closest facility for each customer
    vector<vector<int>> assignments;  // assignments[f] : all the customers that have f as closest facility
    assignments.resize(inst.nb_potential_facilities);
    for (int c = 0; c < inst.nb_customers; c++) {
        Point2D c_pos = inst.customer_positions[c];
        int closest = 0;
        double closest_dist = distance(c_pos, inst.facility_positions[0]);
        for (int f = 1; f < inst.nb_potential_facilities; f++) {
            double dist = distance(c_pos, inst.facility_positions[f]);
            if (dist < closest_dist) {
                closest = f;
                closest_dist = dist;
            }
        }
        assignments[closest].push_back(c);
    }
    vector<int> current_set;
    int current_demand = 0;
    vector<int> unassigned_customers;  // add to this vector if the customer's closest facility doesn't have a big enough capacity (d_c > u_f)
                                       // (don't know if it can happen in practice but check just to be sure)
    // Create columns
    for (int f = 0; f < inst.nb_potential_facilities; f++) {
        int cap = inst.facility_capacities[f];
        if (assignments[f].size() > 0) {
            for (int c : assignments[f]) {
                int d = inst.customer_demands[c];
                if (d > cap) {  // deal with these customers later
                    unassigned_customers.push_back(c);
                    continue;
                }
                if (current_demand + d > cap) {  // add the current column and open a new one with the same facility
                    cols.push_back({f, current_set});
                    current_demand = 0;
                    current_set.clear();
                }
                current_set.push_back(c);  // add customer to the current set
                current_demand += d;
            }
            if (current_demand != 0) {  // end of facility f -> add col if set not empty and clear set
                cols.push_back({f, current_set});
                current_demand = 0;
                current_set.clear();
            }
        }
    }
    // Take care of customers who had a bigger demand than their closest facility could handle
    bool found = false;
    for (int c : unassigned_customers) {
        int d = inst.customer_demands[c];
        // assign customer to the first facility that can accomodate it
        for (int f = 0; f < inst.nb_potential_facilities; f++) {
            if (d <= inst.facility_capacities[f]) {
                cols.push_back({f, {c}});
                found = true;
                break;
            }
        }
        if (!found) {
            cerr << "Error : No facility has big enough capacity to handle customer " << c << endl;
            cerr << "No feasible solution exists!";
            exit(EXIT_FAILURE);
        }
        found = false;  // reset
    }
    return cols;
}

vector<Column> oneColPerCustomer(const Instance& inst) {
    vector<Column> cols;
    for (int c = 0; c < inst.nb_customers; c++) {
        double best_dist = distance(inst.customer_positions[c], inst.facility_positions[0]);
        int best_facility = 0;
        for (int f = 0; f < inst.nb_potential_facilities; f++) {
            double dist = distance(inst.customer_positions[c], inst.facility_positions[f]);
            if (dist < best_dist) {
                dist = best_dist;
                best_facility = f;
            }
        }
        cols.push_back(Column(best_facility, {c}));
    }
    return cols;
}

vector<Column> closestCustomersCols(const Instance& inst) {
    // Take p biggest facilities
    vector<int> facilities(inst.nb_potential_facilities);
    iota(facilities.begin(), facilities.end(), 0);

    // Sort the p facilities
    partial_sort(facilities.begin(), facilities.begin() + inst.nb_max_open_facilities, facilities.end(),
                 [&](int i, int j) { return inst.facility_capacities[i] > inst.facility_capacities[j]; });

    vector<int> p_facilities(facilities.begin(), facilities.begin() + inst.nb_max_open_facilities);

    vector<Column> cols;
    vector<int> customers;
    int current_facility = p_facilities[0];
    int current_index = 0;
    int current_demand = 0;
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
}  // namespace Heuristics