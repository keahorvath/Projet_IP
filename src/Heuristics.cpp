#include "Heuristics.hpp"

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
}  // namespace Heuristics