#include "Solution.hpp"

#include <fstream>
#include <iostream>

#include "Point2D.hpp"
using namespace std;

/**
 * @brief A solution is represented as a vector of facility positions
 * (where each line represents the coordinates of the opened facility associated with the customer)
 */
using Solution = vector<Point2D>;

/**
 * @brief Export the solution in a .sol file
 */
void exportSolution(Solution sol, string sol_name) {
    ofstream fout("../" + sol_name + ".sol");
    if (!fout) {
        cerr << "Error: couldn't open file for writing" << endl;
    } else {
        fout << sol;
        fout.close();
    }
}

ostream& operator<<(ostream& out, const Solution& sol) {
    for (int c = 0; c < sol.size(); c++) {
        out << sol[c].x << " " << sol[c].y << "\n";
    }
    return out;
}

istream& operator>>(istream& in, Solution& sol) {
    sol.clear();
    Point2D p;
    while (in >> p.x >> p.y) {
        sol.push_back(p);
    }
    return in;
}
