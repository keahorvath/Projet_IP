#include "Instance.hpp"

#include <fstream>
#include <unordered_set>

#include "Solution.hpp"
using namespace std;

bool Instance::isFeasible() {
    // Get total customer demand
    double total_demand = 0;
    for (double d : customer_demands) {
        total_demand += d;
    }
    // Get all the facility capacities and sort them
    vector<pair<double, int>> sorted_capacities;
    for (int f = 0; f < nb_potential_facilities; ++f) {
        sorted_capacities.push_back({facility_capacities[f], f});
    }
    sort(sorted_capacities.rbegin(), sorted_capacities.rend());

    // Get max possible capacity of facilities (taking the capacity of the p biggest facilities)
    double max_possible_capacity = 0;

    for (int i = 0; i < nb_max_open_facilities; ++i) {
        max_possible_capacity += sorted_capacities[i].first;
    }

    if (total_demand > max_possible_capacity) {
        return false;
    }
    return true;
}

bool Instance::checker(const Solution& sol) {
    if (sol.size() != nb_customers) {
        cerr << "The solution should have the same number of customers as the instance!\n";
        return false;
    }
    unordered_set<Point2D, Point2DHash> used_facilities(sol.begin(), sol.end());
    int nb_used_facilities = used_facilities.size();
    if (nb_used_facilities > nb_max_open_facilities) {
        cerr << nb_used_facilities << " facilities are used but only " << nb_max_open_facilities << " are allowed!\n";
        return false;
    }
    vector<double> demand(nb_potential_facilities, 0.0);
    for (int c = 0; c < nb_customers; c++) {
        int facility_index = get_facility_index(sol[c]);
        if (facility_index == -1) {
            cerr << "Customer " << c + 1 << " is assigned to a facility that doesn't exist! (" << sol[c].x << "," << sol[c].y << ")\n";
            return false;
        }
        demand[facility_index] += customer_demands[c];
    }
    for (int f = 0; f < nb_potential_facilities; f++) {
        if (demand[f] > facility_capacities[f]) {
            cerr << "Facility " << f + 1 << " has a capacity of " << facility_capacities[f] << " but has a total demand of " << demand[f] << "!\n";
            return false;
        }
    }
    return true;
}
double Instance::objective_value(const Solution& sol) {
    // First check if solution is valid
    if (!checker(sol)) {
        return numeric_limits<double>::infinity();
    }
    // For each customer, get the distance with his assigned facility
    double total_cost = 0.0;
    for (int c = 0; c < nb_customers; c++) {
        total_cost += distance(sol[c], customer_positions[c]);
    }
    return total_cost;
}

int Instance::get_facility_index(const Point2D& pos) {
    // TODO: see if creating a map to make this faster is useful
    for (int f = 0; f < nb_potential_facilities; f++) {
        if (facility_positions[f] == pos) {
            return f;
        }
    }
    return -1;
}

void Instance::visualize(const Solution& sol, string instance_name) {
    // Make sure solution is valid
    if (!checker(sol)) {
        cerr << "Given solution isn't valid : can't create a visualizer!\n";
        return;
    }
    // Create file
    ofstream svg("../" + instance_name + ".svg");
    if (!svg) {
        cerr << "Couldn't create SVG file!\n";
        return;
    }
    // Create the initial image
    int size = 800;
    svg << R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1" width=")" << size << R"(" height=")" << size << R"(">)" << "\n";
    // Create background
    svg << R"("<rect x="0.0" y="0.0" width="800.0" height="800.0" fill ="white" />)";
    // Create a blue dot for each used facility
    unordered_set<Point2D, Point2DHash> used_facilities(sol.begin(), sol.end());
    for (const Point2D& facility : used_facilities) {
        // The positions are always between 0 and 1 -> we can multiply the value by the size of the image
        svg << R"(<circle cx=")" << facility.x * size << R"(" cy=")" << facility.y * size
            << R"(" r="6.0" fill ="blue" stroke="black" stroke-width="1" />)" << "\n";
    }
    // Create a red dot for each customer and a line between each customer and its supplier
    for (int c = 0; c < nb_customers; c++) {
        svg << R"(<circle cx=")" << customer_positions[c].x * size << R"(" cy=")" << customer_positions[c].y * size
            << R"(" r="3.0" fill="red" stroke="black" stroke-width="1" />)" << "\n";
        svg << R"( <line x1=")" << customer_positions[c].x * size << R"(" y1=")" << customer_positions[c].y * size << R"(" x2=")" << sol[c].x * size
            << R"(" y2=")" << sol[c].y * size << R"(" opacity="0.3" stroke="green" stroke-width="2" />)" << "\n";
    }

    // End document
    svg << "</svg>\n";
    svg.close();
}

ostream& operator<<(ostream& out, const Instance& inst) {
    out << inst.nb_customers << " " << inst.nb_potential_facilities << " " << inst.nb_max_open_facilities << " " << inst.max_cap_new_depots << "\n";
    for (int c = 0; c < inst.nb_customers; c++) {
        Point2D pos = inst.customer_positions[c];
        out << pos.x << " " << pos.y << " " << inst.customer_demands[c] << "\n";
    }
    for (int f = 0; f < inst.nb_potential_facilities; f++) {
        Point2D pos = inst.facility_positions[f];
        out << pos.x << " " << pos.y << " " << inst.facility_capacities[f] << "\n";
    }
    return out;
}

istream& operator>>(istream& in, Instance& inst) {
    in >> inst.nb_customers >> inst.nb_potential_facilities >> inst.nb_max_open_facilities >> inst.max_cap_new_depots;
    inst.customer_demands.clear();
    inst.customer_positions.clear();
    inst.customer_positions.resize(inst.nb_customers);
    inst.customer_demands.resize(inst.nb_customers);
    for (int c = 0; c < inst.nb_customers; c++) {
        Point2D pos;
        in >> pos.x >> pos.y >> inst.customer_demands[c];
        inst.customer_positions[c] = pos;
    }
    inst.facility_positions.clear();
    inst.facility_capacities.clear();
    inst.facility_positions.resize(inst.nb_potential_facilities);
    inst.facility_capacities.resize(inst.nb_potential_facilities);
    for (int f = 0; f < inst.nb_customers; f++) {
        Point2D pos;
        in >> pos.x >> pos.y >> inst.facility_capacities[f];
        inst.facility_positions[f] = pos;
    }
    return in;
}
