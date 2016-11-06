#pragma once

#include "types.hpp"


class entity_t
{
public:
    entity_t(id_type id, position_t position)
        : id(id), position(position)
    {
    }


public:
    id_type id;
    position_t position;
};


class buster_t : public entity_t
{
public:
    enum class state_t
    {
        NORMAL = 0,
        CARRY_GHOST = 1,
        STUNNED = 2,
        BUSTING_GHOST = 3,
    };


public:
    buster_t()
        : entity_t(0, {}), state(state_t::NORMAL), value(0)
    {
    }


public:
    state_t state;
    value_t value;
};


class ghost_t : public entity_t
{
public:
    ghost_t()
        : entity_t(0, {}), stamina(0), busters_catching(0)
    {
    }

    bool is_busted() const
    {
        return (busters_catching > 0);
    }


public:
    count_t stamina;
    count_t busters_catching;
};
