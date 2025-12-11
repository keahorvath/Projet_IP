#ifndef POINT2D_HPP
#define POINT2D_HPP
#include <cmath>
#include <functional>

/**
 * @struct Point2D
 * @brief Represents a position (x,y)
 */
struct Point2D {
    double x, y;
    bool operator==(const Point2D& other) const {
        return x == other.x && y == other.y;
    }
};

/**
 * @struct Point2DHash
 * @brief Used to create a set of 2D points
 */
struct Point2DHash {
    size_t operator()(const Point2D& p) const {
        auto h1 = std::hash<double>{}(p.x);
        auto h2 = std::hash<double>{}(p.y);
        return h1 ^ (h2 << 1);
    }
};

/**
 * @brief Calculate the distance between two points
 */
inline double distance(const Point2D& pos1, const Point2D& pos2) {
    return std::sqrt(std::pow(pos1.x - pos2.x, 2) + std::pow(pos1.y - pos2.y, 2));
}

#endif