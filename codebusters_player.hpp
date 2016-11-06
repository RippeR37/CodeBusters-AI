#pragma once

#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "command.hpp"
#include "constants.hpp"
#include "entity.hpp"
#include "game_data.hpp"
#include "input.hpp"
#include "task.hpp"
#include "tracking_data.hpp"
#include "types.hpp"
#include "utils.hpp"


// Tracked informations:
// =====================
// - game_data
//   - visible busters
//   - visible enemies
//   - visible ghosts
// - tracking_data
//   - projected ghosts
//     - symetry (only when ghost is seen for the first time and spotted in our half of map)
//     - seen but left alone and went out of scope
//   - projected enemies
//     - TODO: not important now
//   - STUN-related data
//     - when buster #n fired
//     - when enemy  #k fired
//     - when buster #n stun ends
//     - when enemy  #k stun ends
//     - when buster #n cooldown ends
//     - when enemy  #k cooldown ends
//   - BUST-related data
//     - who from busters was busting given ghost
//     - how many enemies bust given ghost
//   - LOCATION-related data
//     - how many enemies are in distance less then N
//     - how many ghosts are in distance less then N

class codebusters_player_t
{
public:
    codebusters_player_t()
        : game_data(input::read_game_data())
    {
    }

    void play()
    {
        const count_t ROUND_COUNT = 250;

        assign_initial_tasks();

        while (game_data.round < ROUND_COUNT)
        {
            process_round_data();

            on_new_round();

            assign_tasks();
            execute_assignments();

            move_to_next_round();
        }
    }


private: // General flow methods
    void process_round_data()
    {
        game_data_t previous_game_data = game_data;
        game_data.prepare_for_next_round();

        input::read_round_data(game_data);

        compute_tracking_data((game_data.round > 0) ? previous_game_data : game_data);
    }

    void compute_tracking_data(const game_data_t& previous_game_data)
    {
        compute_appeared_ghosts(previous_game_data); // finds if any present ghost just appeared, and if so then if it's first time then mark symmetry ghost
        compute_disappeared_ghosts(previous_game_data); // if ghost went out of scope, save information about him for later
        compute_stun_tracking_data(previous_game_data); // computes anything stun-related
        compute_appeared_enemies(previous_game_data); // finds any enemies that just came on screen
        compute_disappeared_enemies(previous_game_data); // discovers which enemies just went out of scope
        compute_busters_start_carrying_ghosts(previous_game_data); // find which busters started to carry ghost
        compute_busters_stop_carrying_ghosts(previous_game_data); // find which busters stopped carrying ghost
        compute_lost_ghosts(previous_game_data); // find if any buster lost his ghost while carring it
    }

    void assign_tasks()
    {
        assignments.clear();

        // Create initial assignments if not done yet (they will be marked as pending assignments)
        count_t i = 0;
        for (const auto& id_buster_pair : game_data.busters)
        {
            const buster_t& buster = id_buster_pair.second;

            if (initial_assignments_done.find(buster.id) == initial_assignments_done.end())
                do_initial_assignment(buster, initial_goal_positions.at(game_data.busters_count)[i]);

            ++i;
        }

        // Add pending assignments
        assign_pending_assignments();

        // Compute assignments
        std::vector<assignment_t> current_assignments;
        for (const task_t& task : tasks)
        {
            for (const auto& id_buster_pair : game_data.busters)
            {
                const buster_t& buster = id_buster_pair.second;
                current_assignments.push_back(assignment_t { task, buster.id, get_score_for_assignment(buster, task) });
            }
        }

        // Sort assignments for best-is-first order
        std::sort(std::begin(current_assignments), std::end(current_assignments));

        // Comput best-for-each-buster
        std::vector<position_t> explore_locations; // mark those when buster already will go there
        std::vector<id_type> stun_targets; // mark targets already stunned
        std::map<id_type, assignment_t> current_best_assignments;
        for (std::size_t i = 0; i < current_assignments.size(); ++i)
        {
            if (current_best_assignments.size() == game_data.busters_count)
                break;

            if (current_best_assignments.count(current_assignments[i].owner) == 0)
            {
                const task_t& current_task = current_assignments[i].task;
                bool allowed = true;

                // Forbid explorations where one buster is already going there
                if (current_task.type == task_t::type_t::EXPLORE)
                {
                    for (const auto& explore_location : explore_locations)
                    {
                        if (distance_between(current_task.position, explore_location) < game_data.MOVE_RANGE * 1.5) // TODO experimental factor
                        {
                            allowed = false;
                            break;
                        }
                    }

                    if (allowed)
                        explore_locations.push_back(current_task.position);
                }

                // Forbid stunning enemy multiple times at same round (won't stun in next either)
                else if (current_task.type == task_t::type_t::STUN)
                {
                    for (const auto& stun_target : stun_targets)
                    {
                        if (stun_target == current_task.id)
                        {
                            allowed = false;
                            break;
                        }
                    }

                    if (allowed)
                        stun_targets.push_back(current_task.id);
                }

                if (allowed)
                    current_best_assignments[current_assignments[i].owner] = current_assignments[i];
            }
        }

        // Assign task if buster doesn't have any yet (he might have from pending assignments)
        for (const auto& id_buster_pair : game_data.busters)
        {
            const auto& buster = id_buster_pair.second;

            auto buster_assignment = assignments.find(buster.id);
            if (buster_assignment == assignments.end())
                assignments.insert({ buster.id, current_best_assignments[buster.id] });
        }
    }

    void do_initial_assignment(const buster_t& buster, const position_t& initial_assignment_position)
    {
        double distance = distance_between(buster, initial_assignment_position);

        if (distance < (game_data.MOVE_RANGE / 2.0))
        {
            auto radar_task = task_t::make_radar();
            auto radar_assignment = assignment_t { radar_task, buster.id, 0.0 };
            pending_assignments.insert({ buster.id, radar_assignment });

            initial_assignments_done.insert(buster.id);
        }
        else
        {
            auto move_to_radar_task = task_t::make_explore(initial_assignment_position, 0.0);
            auto move_to_radar_assignment = assignment_t { move_to_radar_task, buster.id, 0.0 };

            pending_assignments.insert({ buster.id, move_to_radar_assignment });
        }
    }

    void assign_pending_assignments()
    {
        assignments = pending_assignments;
        pending_assignments.clear();
    }

    void execute_assignments()
    {
        // TODO: if one is ejectng, other can check if it can go closer to base
        for (const auto& id_buster_pair : game_data.busters)
            execute_task(id_buster_pair.second, assignments[id_buster_pair.first].task);
    }

    void move_to_next_round()
    {
        game_data.mark_next_round();
    }


private: // Command execution
    void execute_command(const command_t& command)
    {
        switch (command.type)
        {
        case command_t::type_t::MOVE:
            execute_specials_for_move_command(static_cast<const move_command_t&>(command));
            break;
        case command_t::type_t::BUST:
            execute_specials_for_bust_command(static_cast<const bust_command_t&>(command));
            break;
        case command_t::type_t::STUN:
            execute_specials_for_stun_command(static_cast<const stun_command_t&>(command));
            break;
        case command_t::type_t::RELEASE:
            execute_specials_for_release_command(static_cast<const release_command_t&>(command));
            break;
        case command_t::type_t::RADAR:
            execute_specials_for_radar_command(static_cast<const radar_command_t&>(command));
            break;
        case command_t::type_t::EJECT:
            execute_specials_for_eject_command(static_cast<const eject_command_t&>(command));
        }

        command.execute("Buster #" + std::to_string(command.owner_id));
    }

    void execute_specials_for_move_command(const move_command_t& move_command)
    {
    }

    void execute_specials_for_bust_command(const bust_command_t& bust_command)
    {
    }

    void execute_specials_for_stun_command(const stun_command_t& stun_command)
    {
        tracking_data.buster_stun_usage[stun_command.owner_id] = game_data.round;
        tracking_data.enemy_stunned_since[stun_command.enemy_id] = game_data.round;
    }

    void execute_specials_for_release_command(const release_command_t& release_command)
    {
        game_data.count_new_point();
    }

    void execute_specials_for_radar_command(const radar_command_t& radar_command)
    {
        tracking_data.radar_usage.insert(radar_command.owner_id);
    }

    void execute_specials_for_eject_command(const eject_command_t& eject_command)
    {
    }


private: // Task execution
    void execute_task(const buster_t& buster, const task_t& task)
    {
        switch (task.type)
        {
        case task_t::type_t::BUST:
            execute_bust_task(buster, task);
            break;

        case task_t::type_t::COVER:
            execute_cover_task(buster, task);
            break;

        case task_t::type_t::EXPLORE:
            execute_explore_task(buster, task);
            break;

        case task_t::type_t::STUN:
            execute_stun_task(buster, task);
            break;

        case task_t::type_t::RETURN:
            execute_return_task(buster, task);
            break;

        case task_t::type_t::RADAR:
            execute_radar_task(buster, task);
            break;
        }
    }

    void execute_bust_task(const buster_t& buster, const task_t& task)
    {
        const ghost_t& ghost = game_data.ghosts.at(task.id);
        double distance = distance_between(buster, ghost.position);

        if (distance < game_data.BUST_RANGE_MIN)
            execute_command(move_command_t {
            buster.id,
            game_data.get_position_in_range(game_data.base_position.own, ghost.position, game_data.BUST_RANGE_MIN + 10.0) });
        else if (game_data.BUST_RANGE_MAX < distance)
            execute_command(move_command_t {
            buster.id,
            game_data.get_position_in_range(buster.position, ghost.position, game_data.BUST_RANGE_MIN + 10.0) });
        else
            execute_command(bust_command_t { buster.id, ghost.id });
    }

    void execute_cover_task(const buster_t& buster, const task_t& task)
    {
        const buster_t& carrier = game_data.busters.at(task.id);
        execute_command(move_command_t { buster.id, game_data.get_position_in_range(buster.position, carrier.position, game_data.BUST_RANGE_MIN - 10.0) });
    }

    void execute_explore_task(const buster_t& buster, const task_t& task)
    {
        execute_command(move_command_t { buster.id, task.position });
    }

    void execute_stun_task(const buster_t& buster, const task_t& task)
    {
        const buster_t& enemy = game_data.enemies.at(task.id);
        double distance = distance_between(buster, enemy);

        if (distance <= game_data.STUN_RANGE)
            execute_command(stun_command_t { buster.id, task.id });
        else
            execute_command(move_command_t { buster.id, enemy.position });
    }

    void execute_return_task(const buster_t& buster, const task_t& task)
    {
        if (game_data.is_in_base_range(buster))
        {
            execute_command(release_command_t { buster.id });
        }
        else if (can_eject_to_friend(buster))
        {
            eject_command_t::params_t eject_params;
            eject_params = get_best_eject_params(buster);

            // Eject to friendly buster
            execute_command(eject_command_t { buster.id, eject_params.position });

            // Add assignment for friendly buster
            auto bust_task = task_t::make_bust(static_cast<id_type>(buster.value));
            auto bust_assignment = assignment_t { bust_task, eject_params.buster_id, 0.0 };
            pending_assignments.insert({ eject_params.buster_id, bust_assignment });
        }
        else
        {
            execute_command(move_command_t { buster.id, game_data.get_position_in_range(buster.position, game_data.base_position.own, game_data.BASE_RELEASE_RANGE - 10.0) });
        }
    }

    void execute_radar_task(const buster_t& buster, const task_t& task)
    {
        execute_command(radar_command_t { buster.id });
    }


private: // Task helper methods

    // Conditions for buster A to eject to friend:
    // - there is a buster B between A and A's base
    //   - distance(A,B) <= 4320 (1760 + 1760 + 800), TODO: check if B can't move earlier to this location
    // - B has less moves to base then A (at least two moves less)
    // - B is in noraml state or busting a ghost with at least 5 stamina
    // - TODO: (OPTIONAL) B is relatively safe (no enemies in (MOVE_RANGE + STUN_RANGE) range)
    bool can_eject_to_friend(const buster_t& buster)
    {
        for (const auto& id_buster_pair : game_data.busters)
        {
            const auto& other = id_buster_pair.second;

            // Can't pass it to yourself
            if (other.id == buster.id)
                continue;

            bool can_pass_to_other = true;
            can_pass_to_other &= (distance_between(buster, other) <= 4320.0);

            position_t buster_target_position = game_data.get_position_in_range(buster.position, game_data.base_position.own, game_data.BASE_RELEASE_RANGE);
            position_t other_target_position = game_data.get_position_in_range(other.position, game_data.base_position.own, game_data.BASE_RELEASE_RANGE);
            count_t moves_from_buster = moves_from_distance(distance_between(buster, buster_target_position));
            count_t moves_from_other = moves_from_distance(distance_between(other, other_target_position));
            can_pass_to_other &= (moves_from_other + 2 < moves_from_buster);


            bool state_requirement = false;
            count_t min_busting_stamina = 6;
            count_t max_STUN_TIMEOUT = 2;
            id_type busting_ghost_id = static_cast<id_type>(other.value);
            state_requirement |= (other.state == buster_t::state_t::NORMAL);
            state_requirement |= (other.state == buster_t::state_t::BUSTING_GHOST && game_data.ghosts.at(busting_ghost_id).stamina >= min_busting_stamina);
            state_requirement |= (other.state == buster_t::state_t::STUNNED && get_stunned_timeout(other) <= max_STUN_TIMEOUT);
            can_pass_to_other &= state_requirement;


            // TODO: optional check if `other` is safe

            if (can_pass_to_other)
                return true;
        }

        return false;
    }

    eject_command_t::params_t get_best_eject_params(const buster_t& buster)
    {
        count_t best_other_least_moves_to_base = 99999;
        eject_command_t::params_t best_other;

        for (const auto& id_buster_pair : game_data.busters)
        {
            const auto& other = id_buster_pair.second;

            // Can't pass it to yourself
            if (other.id == buster.id)
                continue;

            bool can_pass_to_other = true;
            can_pass_to_other &= (distance_between(buster, other) <= 4320.0);

            position_t buster_target_position = game_data.get_position_in_range(buster.position, game_data.base_position.own, game_data.BASE_RELEASE_RANGE);
            position_t other_target_position = game_data.get_position_in_range(other.position, game_data.base_position.own, game_data.BASE_RELEASE_RANGE);
            count_t moves_from_buster = moves_from_distance(distance_between(buster, buster_target_position));
            count_t moves_from_other = moves_from_distance(distance_between(other, other_target_position));
            can_pass_to_other &= (moves_from_other + 2 < moves_from_buster);


            bool state_requirement = false;
            count_t min_busting_stamina = 6;
            count_t max_STUN_TIMEOUT = 1;
            id_type busting_ghost_id = static_cast<id_type>(other.value);
            state_requirement |= (other.state == buster_t::state_t::NORMAL);
            state_requirement |= (other.state == buster_t::state_t::BUSTING_GHOST && game_data.ghosts.at(busting_ghost_id).stamina >= min_busting_stamina);
            state_requirement |= (other.state == buster_t::state_t::STUNNED && get_stunned_timeout(other) <= max_STUN_TIMEOUT);
            can_pass_to_other &= state_requirement;

            // TODO: optional check if `other` is safe

            if (can_pass_to_other)
            {
                if (moves_from_other < best_other_least_moves_to_base)
                {
                    best_other.buster_id = other.id;
                    best_other.position = game_data.get_position_in_range(buster.position, other.position, game_data.MOVE_RANGE + 10.0);
                    best_other_least_moves_to_base = moves_from_other;
                }
            }
        }

        return best_other;
    }

    void delete_tasks(task_t::type_t type, const position_t& near_position, double near_distance = 500.0)
    {
        tasks.erase(
            std::remove_if(
            tasks.begin(),
            tasks.end(),
            [type, near_position, near_distance](const task_t& task) {
            return (task.type == type && distance_between(task.position, near_position) <= near_distance);
        }),
            tasks.end());
    }

    void delete_tasks(task_t::type_t type, id_type id)
    {
        tasks.erase(
            std::remove_if(
            tasks.begin(),
            tasks.end(),
            [type, id](const task_t& task) {
            return (task.type == type && task.id == id);
        }),
            tasks.end());
    }

    count_t get_tasks_count_of_type(task_t::type_t type)
    {
        count_t result = 0;

        for (const auto& task : tasks)
        {
            if (task.type == type)
                ++result;
        }

        return result;
    }

    void insert_random_explore_tasks(count_t count)
    {
        for (count_t i = 0; i < count; ++i)
        {
            coord_t x = (std::rand() % game_data.map_size.x);
            coord_t y = (std::rand() % game_data.map_size.y);

            tasks.push_back(task_t::make_explore({ x, y }, explore_factor));
        }
    }

    void assign_initial_tasks()
    {
        insert_random_explore_tasks(50);
        tasks.push_back(task_t::make_return());
    }


private: // Scoring assignments
    factor_t get_score_for_assignment(const buster_t& buster, const task_t& task)
    {
        switch (task.type)
        {
        case task_t::type_t::BUST:
            return get_score_for_bust_assignment(buster, task);

        case task_t::type_t::COVER:
            return get_score_for_cover_assignment(buster, task);

        case task_t::type_t::EXPLORE:
            return get_score_for_explore_assignment(buster, task);

        case task_t::type_t::STUN:
            return get_score_for_stun_assignment(buster, task);

        case task_t::type_t::RETURN:
            return get_score_for_return_assignment(buster, task);
        }

        return 999999.0;
    }

    factor_t get_score_for_bust_assignment(const buster_t& buster, const task_t& task)
    {
        const ghost_t& ghost = game_data.ghosts.at(task.id);
        count_t moves_needed = game_data.get_bust_moves_from_distance(distance_between(buster, ghost.position));

        count_t ghost_stamina = std::min(ghost.stamina, static_cast<count_t>(30));
        factor_t score = (ghost_stamina / (std::ceil((game_data.points + 0.1) / 4.0))) + (moves_needed * 4.0); // TODO: Experimental factor!

        return score;
    }

    factor_t get_score_for_cover_assignment(const buster_t& buster, const task_t& task)
    {
        if (buster.state == buster_t::state_t::CARRY_GHOST)
            return 999999.0;

        const buster_t& carrier = game_data.busters.at(task.id);
        count_t moves_to_carrier = moves_from_distance(distance_between(buster, carrier));
        count_t moves_to_base = moves_from_distance(distance_between(carrier, game_data.base_position.own));

        factor_t score = moves_to_carrier * 15.0 + moves_to_base * 20.0;

        return score;
    }

    factor_t get_score_for_explore_assignment(const buster_t& buster, const task_t& task)
    {
        count_t moves_to_explore = moves_from_distance(distance_between(buster, task.position));

        factor_t ghosts_to_win = (game_data.ghosts_count / 2);
        ghosts_to_win -= game_data.points;

        factor_t end_of_game_factor = std::max(1.0, (5.0 - ghosts_to_win) / 2.0);

        return (moves_to_explore * task.factor) / end_of_game_factor;
    }

    factor_t get_score_for_stun_assignment(const buster_t& buster, const task_t& task)
    {
        factor_t score = 999999.0;

        try
        {
            const buster_t& enemy = game_data.enemies.at(task.id);

            if (get_enemy_stunned_timeout(enemy) > 3)
            {
                // Don't bother to stun enemy who is already stunned and has long stun timeout
            }
            else if (can_stun_enemy_now(buster, enemy) && enemy.state != buster_t::state_t::BUSTING_GHOST && enemy.state != buster_t::state_t::STUNNED)
            {
                score = 0.0;
            }
            else if (enemy.state == buster_t::state_t::CARRY_GHOST)
            {
                count_t enemy_carrier_moves = moves_from_distance(
                    distance_between(
                    enemy,
                    game_data.get_position_in_range(
                    enemy.position,
                    game_data.base_position.enemy,
                    game_data.BASE_RELEASE_RANGE)));
                count_t buster_moves_to_enemy_base =
                    moves_from_distance(distance_between(buster,
                    game_data.get_position_in_range(buster.position,
                    game_data.base_position.enemy,
                    game_data.BASE_RELEASE_RANGE)));

                if (get_stunned_timeout(buster) + 1 < enemy_carrier_moves &&
                    get_stun_cooldown(buster) + 1 < enemy_carrier_moves &&
                    buster_moves_to_enemy_base + 1 < enemy_carrier_moves &&
                    moves_from_distance(distance_between(buster, enemy)) < 2)
                {
                    score = 0.11;
                }
            }
            else if (enemy.state == buster_t::state_t::STUNNED)
            {
                if (get_enemy_stunned_timeout(enemy) < 3 && can_stun_now(buster))
                {
                    score = 0.13;
                }
            }
            else if (enemy.state == buster_t::state_t::BUSTING_GHOST)
            {
                id_type ghost_id = static_cast<id_type>(enemy.value);
                if (game_data.ghosts.at(ghost_id).stamina < 10 && can_stun_now(buster))
                {
                    score = 0.12;
                }
            }

        }
        catch (...) {}

        return score;
    }

    factor_t get_score_for_return_assignment(const buster_t& buster, const task_t& task)
    {
        if (buster.state == buster_t::state_t::CARRY_GHOST)
            return 0.0001;

        return 99999999.0;
    }


private: // Compute tracking data methods
    void compute_appeared_ghosts(const game_data_t& previous_game_data)
    {
        for (const auto& id_ghost_pair : game_data.ghosts)
        {
            const ghost_t& ghost = id_ghost_pair.second;

            if (previous_game_data.ghosts.find(ghost.id) == previous_game_data.ghosts.end() ||
                tracking_data.ghosts_spotted.find(ghost.id) == tracking_data.ghosts_spotted.end())
            {
                on_reappeared_ghost(id_ghost_pair.second);
            }

            if (tracking_data.ghosts_spotted.find(ghost.id) == tracking_data.ghosts_spotted.end())
            {
                tracking_data.ghosts_spotted.insert(ghost.id);
                on_new_ghost(id_ghost_pair.second);
            }
        }
    }

    void compute_disappeared_ghosts(const game_data_t& previous_game_data)
    {
        for (const auto& id_ghost_pair : previous_game_data.ghosts)
        {
            id_type ghost_id = id_ghost_pair.first;

            if (game_data.ghosts.find(ghost_id) == game_data.ghosts.end())
                on_disappeared_ghost(id_ghost_pair.second);
        }
    }

    void compute_stun_tracking_data(const game_data_t& previous_game_data)
    {
        // Finding out if any buster got stunned and if so, try to find out which stunned it
        for (const auto& id_buster_pair : game_data.busters)
        {
            const buster_t& buster = id_buster_pair.second;

            if (buster.state == buster_t::state_t::STUNNED &&
                previous_game_data.busters.at(id_buster_pair.first).state != buster_t::state_t::STUNNED)
            {
                on_buster_stunned(buster);

                std::vector<id_type> enemies_close_by = game_data.get_enemies_within_range(buster, game_data.STUN_RANGE);
                if (enemies_close_by.size() == 1)
                    on_enemy_use_stun(game_data.enemies.at(enemies_close_by.front()));
            }
        }
    }

    void compute_appeared_enemies(const game_data_t& previous_game_data)
    {
        for (const auto& id_enemy_pair : game_data.enemies)
        {
            const buster_t& enemy = id_enemy_pair.second;

            if (previous_game_data.enemies.find(enemy.id) == previous_game_data.enemies.end())
                on_reappeared_enemy(enemy);
        }
    }

    void compute_disappeared_enemies(const game_data_t& previous_game_data)
    {
        for (const auto& id_enemy_pair : previous_game_data.enemies)
        {
            const buster_t& enemy = id_enemy_pair.second;

            if (game_data.enemies.find(enemy.id) == game_data.enemies.end())
                on_disappeared_enemy(enemy);
        }
    }

    void compute_busters_start_carrying_ghosts(const game_data_t& previous_game_data)
    {
        for (const auto& id_buster_pair : game_data.busters)
        {
            const buster_t& buster = id_buster_pair.second;

            if (previous_game_data.busters.at(buster.id).state != buster_t::state_t::CARRY_GHOST
                && game_data.busters.at(buster.id).state == buster_t::state_t::CARRY_GHOST)
            {
                on_start_carrying_ghost(buster);
            }
        }
    }

    void compute_busters_stop_carrying_ghosts(const game_data_t& previous_game_data)
    {
        for (const auto& id_buster_pair : game_data.busters)
        {
            const buster_t& buster = id_buster_pair.second;

            if (previous_game_data.busters.at(buster.id).state == buster_t::state_t::CARRY_GHOST
                && game_data.busters.at(buster.id).state != buster_t::state_t::CARRY_GHOST)
            {
                on_stop_carrying_ghost(buster);
            }
        }
    }

    void compute_lost_ghosts(const game_data_t& previous_game_data)
    {
        for (const auto& id_buster_pair : game_data.busters)
        {
            const buster_t& buster = id_buster_pair.second;

            if (previous_game_data.busters.at(buster.id).state == buster_t::state_t::CARRY_GHOST
                && game_data.busters.at(buster.id).state == buster_t::state_t::STUNNED)
            {
                on_lose_ghost(buster);
            }
        }
    }


private: // Tracking callback methods
    void on_new_ghost(const ghost_t& ghost)
    {
        if ((ghost.position.x + ghost.position.y) <= (game_data.map_size.x + game_data.map_size.y) / 2)
        {
            tracking_data.ghosts_projected.push_back(game_data.get_inverted_position(ghost.position));

            // Create explore (projected) task
            tasks.push_back(task_t::make_explore(game_data.get_inverted_position(ghost.position), projected_ghost_factor));
        }
    }

    void on_reappeared_ghost(const ghost_t& ghost)
    {
        tracking_data.ghosts_out_of_scope.erase(ghost.id);

        // Create bust task
        tasks.push_back(task_t::make_bust(ghost.id));

        // Delete explore (out-of-scope) task
        delete_tasks(task_t::type_t::EXPLORE, ghost.position);
    }

    void on_disappeared_ghost(const ghost_t& ghost)
    {
        tracking_data.ghosts_out_of_scope.insert({ ghost.id, ghost });

        factor_t stamina_factor = 1.0;
        if (ghost.stamina < 5)
            stamina_factor = 0.7;
        else if (ghost.stamina < 16)
            stamina_factor = 0.9;

        // Create explore (out-of-scope) task
        tasks.push_back(task_t::make_explore(ghost.position, out_of_scope_ghost_factor * stamina_factor));

        // Delete bust task
        delete_tasks(task_t::type_t::BUST, ghost.id);
    }

    void on_reappeared_enemy(const buster_t& enemy)
    {
        // Create stun task
        tasks.push_back(task_t::make_stun(enemy.id));
    }

    void on_disappeared_enemy(const buster_t& enemy)
    {
        // Delete stun task
        delete_tasks(task_t::type_t::STUN, enemy.id);
    }

    void on_buster_stunned(const buster_t& buster)
    {
        tracking_data.buster_stunned_since[buster.id] = game_data.round;
    }

    void on_enemy_use_stun(const buster_t& enemy)
    {
        tracking_data.enemy_stun_usage[enemy.id] = game_data.round;
    }

    void on_start_carrying_ghost(const buster_t& buster)
    {
        // Create cover task
        tasks.push_back(task_t::make_cover(buster.id));
    }

    void on_stop_carrying_ghost(const buster_t& buster)
    {
        // Delete cover task
        delete_tasks(task_t::type_t::COVER, buster.id);
    }

    void on_new_round()
    {
        for (const auto& id_buster_pair : game_data.busters)
        {
            const buster_t& buster = id_buster_pair.second;

            delete_tasks(task_t::type_t::EXPLORE, buster.position, game_data.MOVE_RANGE / 2.0);
        }

        if (get_tasks_count_of_type(task_t::type_t::EXPLORE) < game_data.busters_count)
        {
            insert_random_explore_tasks(game_data.busters_count * 3);
        }
    }

    void on_lose_ghost(const buster_t& buster)
    {
        tracking_data.lose_ghost_count++;

        if (game_data.is_in_base_range(buster))
            game_data.points++;
    }


private: // Tracking utility methods
    bool can_stun_now(const buster_t& buster)
    {
        if (buster.state == buster_t::state_t::STUNNED)
            return false;

        return (get_stun_cooldown(buster) == 0);
    }

    bool can_stun_enemy_now(const buster_t& buster, const buster_t& enemy)
    {
        return (can_stun_now(buster) && distance_between(buster, enemy) <= game_data.STUN_RANGE);
    }

    count_t get_stun_cooldown(const buster_t& buster)
    {
        long long int result = 0;

        auto stun_usage = tracking_data.buster_stun_usage.find(buster.id);
        if (stun_usage != tracking_data.buster_stun_usage.end())
        {
            result += (*stun_usage).second;
            result += game_data.STUN_COOLDOWN;
            result -= game_data.round;
        }

        return static_cast<count_t>(std::max(0ll, result));
    }

    count_t get_enemy_stun_cooldown(const buster_t& enemy) // time until enemy can stun
    {
        long long int result = 0;

        auto stun_usage = tracking_data.enemy_stun_usage.find(enemy.id);
        if (stun_usage != tracking_data.enemy_stun_usage.end())
        {
            result += (*stun_usage).second;
            result += game_data.STUN_COOLDOWN;
            result -= game_data.round;
        }

        return static_cast<count_t>(std::max(0ll, result));
    }

    count_t get_stunned_timeout(const buster_t& buster) // time until stun wears off, or 0
    {
        if (buster.state != buster_t::state_t::STUNNED)
            return 0u;

        long long int result = 0;
        auto stunned_since = tracking_data.buster_stunned_since.find(buster.id);
        if (stunned_since != tracking_data.buster_stunned_since.end())
        {
            result += (*stunned_since).second;
            result += game_data.STUN_TIMEOUT;
            result -= game_data.round;
        }

        return static_cast<count_t>(std::max(0ll, result));
    }

    count_t get_enemy_stunned_timeout(const buster_t& enemy)
    {
        if (enemy.state != buster_t::state_t::STUNNED)
            return 0u;

        long long int result = 0;
        auto stunned_since = tracking_data.enemy_stunned_since.find(enemy.id);
        if (stunned_since != tracking_data.enemy_stunned_since.end())
        {
            result += (*stunned_since).second;
            result += game_data.STUN_TIMEOUT;
            result -= game_data.round;
        }

        return static_cast<count_t>(std::max(0ll, result));
    }

    count_t get_enemy_busting_ghost_count(const ghost_t& ghost)
    {
        long long int result = ghost.busters_catching;

        auto busters_busting_iter = tracking_data.busters_busting_ghost.find(ghost.id);
        if (busters_busting_iter != tracking_data.busters_busting_ghost.end())
            result -= busters_busting_iter->second.size();

        return static_cast<count_t>(std::max(0ll, result));
    }


private:
    game_data_t game_data; // all game data recieved as input
    tracking_data_t tracking_data; // all crurrently tracked data
    std::vector<task_t> tasks; // all currently available tasks
    std::map<id_type, assignment_t> assignments; // assignments in current round
    std::map<id_type, assignment_t> pending_assignments; // assignments for curent round from last round (continuations)
    std::set<id_type> initial_assignments_done; // who already done it's initial assignment (radar explore)


private:
    const factor_t out_of_scope_ghost_factor = 12.0;
    const factor_t projected_ghost_factor = 18.0;
    const factor_t explore_factor = 50.0; // explore should be expensive since we should get most data from initial radar move
};
