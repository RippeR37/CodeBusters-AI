#pragma once

#include <cmath>

#include "entity.hpp"
#include "types.hpp"


double distance_between(const position_t& p1, const position_t& p2)
{
    double dx = static_cast<double>(p1.x) - static_cast<double>(p2.x);
    double dy = static_cast<double>(p1.y) - static_cast<double>(p2.y);

    return std::sqrt(dx*dx + dy*dy);
}

double distance_between(const position_t& position, const buster_t& buster)
{
    return distance_between(position, buster.position);
}

double distance_between(const buster_t& buster, const position_t& position)
{
    return distance_between(buster.position, position);
}

double distance_between(const buster_t& b1, const buster_t& b2)
{
    return distance_between(b1.position, b2.position);
}

count_t moves_from_distance(double distance)
{
    const double MOVE_DISTANCE = 800.0;
    return static_cast<count_t>(std::ceil(distance / MOVE_DISTANCE));
}
