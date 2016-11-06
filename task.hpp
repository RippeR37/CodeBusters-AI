#pragma once

#include "types.hpp"


// Tasks:
//
// - bust visible ghost                 (Create task in: on_reappear_ghost)     (Destroy task in: on_disappeared_ghost)
// - stun (/track) visible enemy        (Create task in: on_reappear_enemy)     (Destroy task in: on_disappeared_enemy)
// - cover own carrier                  (Create task in: on_start_carrying)     (Destroy task in: on_stop_carrying)
// - explore
//   - out-of-scope ghost               (Create task in: on_disappeared_enemy)  (Destroy task in: on_reappear_ghost)
//   - projected ghost                  (Create task in: on_new_ghost)          (Destroy when in general area)
//   - general exploring                (Create task when empty task list)      (Destroy when in general area)
// - return ghost to base
// - use radar

class task_t
{
public:
    enum class type_t
    {
        BUST,
        STUN,
        COVER,
        EXPLORE,
        RETURN,
        RADAR,
    };

    type_t type;
    id_type id; // for: bust, stun, cover
    position_t position; // for: explore
    factor_t factor; // for: explore


public:
    static task_t make_bust(id_type ghost_id)
    {
        return task_t { type_t::BUST, ghost_id };
    }

    static task_t make_stun(id_type enemy_id)
    {
        return task_t { type_t::STUN, enemy_id };
    }

    static task_t make_cover(id_type buster_id)
    {
        return task_t { type_t::COVER, buster_id };
    }

    static task_t make_explore(position_t position, factor_t factor)
    {
        return task_t { type_t::EXPLORE, position, factor };
    }

    static task_t make_return()
    {
        return task_t { type_t::RETURN, 0 };
    }

    static task_t make_radar()
    {
        return task_t { type_t::RADAR, 0 };
    }


public:
    task_t()
        : type(type_t::EXPLORE), id(), position({ 8000, 4500 }), factor()
    {
    }

    task_t(type_t type, id_type id)
        : type(type), id(id), position(), factor()
    {
    }

    task_t(type_t type, position_t position, factor_t factor)
        : type(type), id(), position(position), factor(factor)
    {
    }
};


// Assignment is a tuple of given task, buster who will execute it and computed score factor for this task-buster assignment.
// It's used to compute which task is best for given buster allowing per-buster score computations and execution of same task
// by multiple busters.

class assignment_t
{
public:
    assignment_t()
        : task(), owner(), score()
    {
    }

    assignment_t(task_t task, id_type owner, factor_t score)
        : task(task), owner(owner), score(score)
    {
    }

    bool operator<(const assignment_t& other) const
    {
        return score < other.score;
    }


public:
    task_t task;
    id_type owner;
    factor_t score;
};
