#include "Column.hpp"

#include <ostream>
using namespace std;

Column::Column(int facility, vector<int> customers) : facility(facility), customers(customers) {}

double Column::cost(const Instance& inst) {
    double cost;
    for (int c : customers) {
        cost += distance(inst.customer_positions[c], inst.facility_positions[facility]);
    }
    return cost;
}

ostream& operator<<(ostream& out, const Column& col) {
    out << col.facility << " -> " << "( ";
    for (int c : col.customers) {
        out << c << " ";
    }
    out << ")" << std::endl;
    return out;
}
