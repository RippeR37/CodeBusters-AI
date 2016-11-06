#pragma once

#include <map>
#include <vector>

#include "entity.hpp"
#include "types.hpp"
#include "utils.hpp"


class game_data_t
{
public:
    game_data_t(id_type team_id, count_t busters_count, count_t ghosts_count)
        : team_id(team_id),
        busters_count(busters_count),
        ghosts_count(ghosts_count),
        base_position(team_id),
        map_size({ 16001, 9001 }),
        points(0),
        round(0)
    {
    }

    void prepare_for_next_round()
    {
        busters.clear();
        enemies.clear();
        ghosts.clear();
    }

    void insert_buster(id_type id, position_t position, buster_t::state_t state, value_t value)
    {
        busters[id].id = id;
        busters[id].position = position;
        busters[id].state = state;
        busters[id].value = value;
    }

    void insert_enemy(id_type id, position_t position, buster_t::state_t state, value_t value)
    {
        enemies[id].id = id;
        enemies[id].position = position;
        enemies[id].state = state;
        enemies[id].value = value;

        if (INSERT_CARRIED_GHOST && state == buster_t::state_t::CARRY_GHOST)
            insert_ghost(static_cast<id_type>(value), position, 0, 1);
    }

    void insert_ghost(id_type id, position_t position, count_t stamina, count_t busters_catching)
    {
        ghosts[id].id = id;
        ghosts[id].position = position;
        ghosts[id].stamina = stamina;
        ghosts[id].busters_catching = busters_catching;
    }

    void count_new_point()
    {
        ++points;
    }

    void mark_next_round()
    {
        ++round;
    }


public:
    bool need_one_ghost_more() const
    {
        return ((ghosts_count - 1) / 2 <= points);
    }

    bool is_in_base_range(const position_t& position) const
    {
        return (distance_between(position, base_position.own) <= BASE_RELEASE_RANGE);
    }

    bool is_in_base_range(const buster_t& buster) const
    {
        return is_in_base_range(buster.position);
    }

    double distance_to_base_range(const position_t& position) const
    {
        return std::max(0.0, distance_between(position, base_position.own) - BASE_RELEASE_RANGE);
    }

    double distance_to_base_range(const buster_t& buster) const
    {
        return distance_to_base_range(buster.position);
    }

    position_t get_inverted_position(const position_t& position)
    {
        return { map_size.x - position.x - 1, map_size.y - position.y - 1 };
    }

    position_t get_clamped_position(double x, double y)
    {
        double map_size_x = static_cast<double>(map_size.x);
        double map_size_y = static_cast<double>(map_size.y);

        double clamped_x = std::max(0.0, std::min(map_size_x, x));
        double clamped_y = std::max(0.0, std::min(map_size_y, y));

        return { static_cast<std::size_t>(clamped_x), static_cast<std::size_t>(clamped_y) };
    }

    position_t get_position_in_range(position_t from, position_t to, double range_distance)
    {
        if (from.x == to.x && from.y == to.y)
            from = base_position.enemy;

        double distance = distance_between(from, to);
        double distance_to_go = distance - range_distance;

        double dx = static_cast<double>(to.x) - static_cast<double>(from.x);
        double dy = static_cast<double>(to.y) - static_cast<double>(from.y);
        double factor = distance_to_go / distance;

        position_t result = get_clamped_position(from.x + dx * factor, from.y + dy * factor);

        return result;
    }

    std::vector<id_type> get_enemies_within_range(const buster_t& buster, double range)
    {
        std::vector<id_type> results;

        for (const auto& id_enemy_pair : enemies)
        {
            if (distance_between(buster, id_enemy_pair.second) <= range)
                results.push_back(id_enemy_pair.first);
        }

        return results;
    }

    count_t get_bust_moves_from_distance(double distance)
    {
        if (distance < BUST_RANGE_MIN)
            return moves_from_distance(distance);
        else if (BUST_RANGE_MAX < distance)
            return moves_from_distance(distance - ((BUST_RANGE_MIN + BUST_RANGE_MAX) / 2.0));
        else
            return 0;
    }


public:
    const id_type team_id;
    const count_t busters_count; // per player
    const count_t ghosts_count; // total
    const base_position_t base_position;
    const position_t map_size;

    count_t points;
    round_num_t round;
    std::map<id_type, buster_t> busters;
    std::map<id_type, buster_t> enemies;
    std::map<id_type, ghost_t>  ghosts;


public:
    const double MOVE_RANGE = 800.0;
    const double BUST_RANGE_MIN = 900.0;
    const double BUST_RANGE_MAX = 1760.0;
    const double STUN_RANGE = 1760.0;
    const double BASE_RELEASE_RANGE = 1600.0;

    const count_t STUN_TIMEOUT = 11;
    const count_t STUN_COOLDOWN = 21;

    const bool INSERT_CARRIED_GHOST = true;
};
