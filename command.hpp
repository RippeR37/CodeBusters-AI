#pragma once

#include "types.hpp"


class command_t
{
public:
    enum class type_t
    {
        BUST,
        EJECT,
        MOVE,
        RADAR,
        RELEASE,
        STUN,
    };


public:
    command_t(type_t type, id_type owner_id)
        : type(type), owner_id(owner_id)
    {
    }

    virtual ~command_t() = default;
    virtual void execute(const std::string& message) const = 0;


public:
    type_t type;
    id_type owner_id;
};


class move_command_t : public command_t
{
public:
    move_command_t(id_type owner_id, position_t position)
        : command_t(type_t::MOVE, owner_id), position(position)
    {
    }

    void execute(const std::string& message) const override
    {
        std::cout << "MOVE " << position << " " << message << std::endl;
    }


public:
    position_t position;
};


class bust_command_t : public command_t
{
public:
    bust_command_t(id_type owner_id, id_type ghost_id)
        : command_t(type_t::BUST, owner_id), ghost_id(ghost_id)
    {
    }

    void execute(const std::string& message) const override
    {
        std::cout << "BUST " << ghost_id << " " << message << std::endl;
    }


public:
    id_type ghost_id;
};


class stun_command_t : public command_t
{
public:
    stun_command_t(id_type owner_id, id_type enemy_id)
        : command_t(type_t::STUN, owner_id), enemy_id(enemy_id)
    {
    }

    void execute(const std::string& message) const override
    {
        std::cout << "STUN " << enemy_id << " " << message << std::endl;
    }


public:
    id_type enemy_id;
};


class release_command_t : public command_t
{
public:
    release_command_t(id_type owner_id)
        : command_t(type_t::RELEASE, owner_id)
    {
    }

    void execute(const std::string& message) const override
    {
        std::cout << "RELEASE" << " " << message << std::endl;
    }
};


class radar_command_t : public command_t
{
public:
    radar_command_t(id_type owner_id)
        : command_t(type_t::RADAR, owner_id)
    {
    }

    void execute(const std::string& message) const override
    {
        std::cout << "RADAR" << " " << message << std::endl;
    }
};


class eject_command_t : public command_t
{
public:
    struct params_t
    {
        position_t position;
        id_type buster_id;
    };


public:
    eject_command_t(id_type owner_id, position_t position)
        : command_t(type_t::EJECT, owner_id), position(position)
    {
    }

    void execute(const std::string& message) const override
    {
        std::cout << "EJECT " << position << " " << message << std::endl;
    }


public:
    position_t position;
};
