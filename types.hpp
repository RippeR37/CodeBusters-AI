#pragma once

#include <algorithm>
#include <cstddef>
#include <ostream>


using id_type = std::size_t;
using count_t = std::size_t;
using coord_t = std::size_t;
using value_t = long long int;
using round_num_t = std::size_t;
using factor_t = double;


class position_t
{
public:
    position_t()
        : position_t(0, 0)
    {
    }

    position_t(coord_t x_, coord_t y_)
        : x(x_), y(y_)
    {
    }

    bool operator==(const position_t& other) const
    {
        return ((x == other.x) && (y == other.y));
    }

    friend std::ostream& operator<< (std::ostream& stream, const position_t& position)
    {
        stream << position.x << " " << position.y;
        return stream;
    }


public:
    coord_t x, y;
};


class base_position_t
{
public:
    base_position_t(id_type team_id)
        : own({ 0, 0 }), enemy({ 16000, 9000 })
    {
        if (team_id == 1)
            std::swap(own, enemy);
    }


public:
    position_t own;
    position_t enemy;
};
