#pragma once

#include <iostream>

#include "game_data.hpp"


class input
{
public:
    static game_data_t read_game_data()
    {
        count_t busters_count;
        count_t ghosts_count;
        id_type team_id;

        std::cin >> busters_count; std::cin.ignore();
        std::cin >> ghosts_count;  std::cin.ignore();
        std::cin >> team_id;       std::cin.ignore();

        return game_data_t { team_id, busters_count, ghosts_count };
    }

    static void read_round_data(game_data_t& game_data)
    {
        game_data.prepare_for_next_round();

        count_t entities_count;
        std::cin >> entities_count;
        std::cin.ignore();

        for (std::size_t i = 0; i < entities_count; ++i)
            read_entity_data(game_data);
    }


private:
    static void read_entity_data(game_data_t& game_data)
    {
        const int GHOST_TYPE = -1;

        id_type id;
        position_t position;
        int type;

        std::cin >> id
            >> position.x
            >> position.y
            >> type;

        if (type == GHOST_TYPE)
        {
            count_t stamina;
            count_t busters_catching;

            std::cin >> stamina >> busters_catching;
            std::cin.ignore();

            game_data.insert_ghost(id, position, stamina, busters_catching);
        }
        else
        {
            id_type team_id = static_cast<id_type>(type);
            std::size_t state_value;
            value_t value;

            std::cin >> state_value >> value;
            std::cin.ignore();

            buster_t::state_t state = static_cast<buster_t::state_t>(state_value);

            if (team_id == game_data.team_id)
                game_data.insert_buster(id, position, state, value);
            else
                game_data.insert_enemy(id, position, state, value);
        }
    }
};
