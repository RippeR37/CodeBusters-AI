#include <map>
#include <vector>

#include "types.hpp"


static const std::map<count_t, std::vector<position_t>> initial_goal_positions
{
    {
        2,
        { { 4500, 5500 }, { 8000, 3500 } },
    },
    {
        3,
        { { 9000, 3200 }, { 6000, 4500 }, { 3000, 5800 } },
    },
    {
        4,
        { { 2000, 7000 }, { 4300, 5250 }, { 6500, 3750 }, { 9200, 2000 } },
    },
    {
        5,
        { { 1800, 7000 }, { 3500, 5750 }, { 5200, 4500 }, { 6900, 2250 }, { 8000, 2000 } }
    }
};
