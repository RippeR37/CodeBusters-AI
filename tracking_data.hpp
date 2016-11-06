#pragma once

#include <map>
#include <set>
#include <vector>

#include "entity.hpp"
#include "types.hpp"


struct tracking_data_t
{
    std::set<id_type> ghosts_spotted; // set of spotted ghosts
    std::map<id_type, ghost_t> ghosts_out_of_scope; // ghosts which disappeared by leaving visible scope
    std::vector<position_t> ghosts_projected; // projected ghosts via map's symmetry

    std::map<id_type, round_num_t> buster_stun_usage; // `buster_stun_usage[id] == k` means buster #id used STUN last in round #k
    std::map<id_type, round_num_t> enemy_stun_usage;  // -||-

    std::map<id_type, round_num_t> buster_stunned_since; // `buster_stunned_since[id] == k` means buster #id was last stunned in round #k
    std::map<id_type, round_num_t> enemy_stunned_since;  // -||-

    std::map<id_type, std::vector<id_type>> busters_busting_ghost; // `busters_busting_ghost[id]` is a list of buster ids who were busting ghost #id

    count_t lose_ghost_count; // how many times enemy intercepted our ghost

    std::set<id_type> radar_usage; // track which busters used radar already
};
