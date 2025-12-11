#include "Column.hpp"

#include <ostream>
using namespace std;

ostream& operator<<(ostream& out, const Column& col) {
    out << col.first << " -> " << "( ";
    for (int c : col.second) {
        out << c << " ";
    }
    out << ")" << std::endl;
    return out;
}
