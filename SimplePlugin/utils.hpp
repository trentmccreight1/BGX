#pragma once


#include "../plugin_sdk/plugin_sdk.hpp"
#include <functional>
#include <unordered_set>
#include <unordered_map>

using std::string;
using std::to_string;

namespace utils
{
    std::string toUpper(std::string str);
    std::string toLower(std::string str);
    bool strContains(std::string strA, std::string strB, bool ignoreCase = false);

    vector GetBestPositionAOE(script_spell* spell, std::vector<game_object_script>& objects);
    bool CheckWalls(vector start, vector end);
    bool BigMonsterCheck(game_object_script mob);
    bool isYuumiAttached(const game_object_script& target);
    bool isRecalling(const game_object_script& target);
    bool debuffCantCast();
    float getTotalHP(const game_object_script& target);
    float getPing();
    float getTimeToHit(prediction_input& input, prediction_output& predInfo, const bool takePing);
    bool isValidRecalling(const game_object_script& target, float range, const vector& from);
    bool IsKillableAndValidTargetCustom(script_spell* spell, damage_type type, game_object_script target, float customdamage, float distance, bool healthPredction, bool includeSkillShots);
    bool IsKillableAndValidTarget(script_spell* spell, damage_type type, game_object_script target, float distance, bool healthPredction, bool includeSkillShots);

    bool is_under_enemy_spawn(vector position);

    enum class SpawnTeam
    {
        Blue,
        Red
    };

    SpawnTeam getSpawnTeam(vector pos);

    enum class summoner_spell_type
    {
        Ignite,
        Barrier,
        Heal,
        Exhaust,
        Cleanse,
        Flash,
        Smite,
        Teleport,
        Clarity,  // Only Aram: Non registered.
        Ghost,
        Snowball, // Only Aram: Non registered.
        Unknown
    };


    script_spell* get_script_spell(summoner_spell_type type);

    bool has_mana(game_object_script object, float value);

    bool has_mana_percent(game_object_script object, float value);

    bool has_mana_percent(game_object_script object, TreeEntry* entry_holder);

    int count_nearby_objects(vector position, float range, std::vector<game_object_script> objects, bool is_target = true, ::geometry::polygon* region = nullptr);

    game_object_script get_nearby_object(vector position, float range, std::vector<game_object_script> objects, bool is_target = true, ::geometry::polygon* region = nullptr);

    std::vector<game_object_script> get_nearby_objects(vector position, float range, std::vector<game_object_script> objects, bool is_target = true, ::geometry::polygon* region = nullptr);

    class Raytrace_Input;
    class Raytrace_Output;

    enum raytrace_query_type : uint32_t
    {
        minion = 0,
        hero_enemy = 1,
        hero_ally = 2,
        terrain = 3
    };

    class Raytrace_Input
    {
    public:
        raytrace_query_type _query_type;
        vector _from;
        vector _to;
        float _radius;
        bool _use_boundings;
        std::vector<collisionable_objects> _collision_flags;
        std::vector<game_object_script> _targets;

        Raytrace_Input(raytrace_query_type query_type, vector from, vector to, float radius, std::vector<game_object_script> targets, bool use_boundings = true, std::vector<collisionable_objects> collision_flags = {})
        {
            this->_query_type = query_type;
            this->_from = from;
            this->_to = to;
            this->_radius = radius;
            this->_targets = targets;
            this->_use_boundings = use_boundings;
            this->_collision_flags = collision_flags;
        }

        Raytrace_Input()
        {}

        void set_point_from(vector value) { this->_from = value; }
        void set_point_to(vector value) { this->_to = value; }
        void set_radius(float value) { this->_radius = value; }
        void set_targets(std::vector<game_object_script> value) { this->_targets = value; }
        void set_collision_flags(std::vector<collisionable_objects> value) { this->_collision_flags = value; }

        vector get_start_point() { return this->_from; }
        vector get_end_point() { return this->_to; }
        float get_radius() { return this->_radius; }
        std::vector<game_object_script> get_targets() { return this->_targets; }
        std::vector<collisionable_objects> get_collision_flags() { return this->_collision_flags; }

        float get_distance_between_points() { return _from.distance(_to); }
        float get_start_distance_from_myhero() { return myhero->get_distance(_from); }
        float get_end_distance_from_myhero() { return myhero->get_distance(_to); }

        bool has_collision_flags() { return !this->_collision_flags.empty(); }

        bool is_valid()
        {
            return this->_from.is_valid() && this->_to.is_valid() && (this->_query_type == raytrace_query_type::terrain || !this->_targets.empty()) &&
                (!this->_use_boundings || this->_radius > 0);
        }

        Raytrace_Output process();
    };

    class Raytrace_Output
    {
    private:
        std::vector<game_object_script> _objects;
        bool _had_collision;

    public:
        Raytrace_Output(std::vector<game_object_script> objects, bool had_collision)
        {
            this->_objects = objects;
            this->_had_collision = had_collision;
        }

        Raytrace_Output()
        {}

        std::vector<game_object_script> get_objects() { return this->_objects; }
        game_object_script get_first() { return (!this->_objects.empty() ? this->_objects.front() : nullptr); }

        int count_match() { return this->_objects.size(); }
        bool has_match() { return !this->_objects.empty(); }
        bool had_collisions() { return this->_had_collision; };

        static Raytrace_Output empty() { return Raytrace_Output(); }
    };

    enum class minion_type : uint32_t
    {
        jungle_monsters = 2,
        lane_minion_warrior = 4,
        lane_minion_mage = 5,
        lane_minion_cannon = 6,
        lane_super_minion = 7
    };

    bool is_minion_instance(game_object_script object, minion_type type);

    namespace buffs
    {
        buff_instance_script get_buff_by_fragmented_name(game_object_script object, std::vector<string> args);

        bool has_buff_by_fragmented_name(game_object_script object, std::vector<string> args);

        bool has_group_control_debuff(game_object_script object);

        bool has_invulnerability_buff(game_object_script object);

        bool has_buff_aa_block(game_object_script object);

        bool has_unkillable_buff(game_object_script target);

        bool buff_special(buff_instance_script buff);
    }

    namespace damage
    {
        float get_damage_on_unit(game_object_script target, std::vector<script_spell*> spells, bool ignore_non_ready, float extra_damage = 0.f, damage_type damage_type = damage_type::physical);

        bool can_kill_with_aa(game_object_script target, bool physical_shield = true, bool magical_shield = false);
    }

    namespace config
    {
        hit_chance get_hitchance_by_config(TreeEntry* hit);

        bool get_status_from_config(std::map<uint32_t, TreeEntry*> map, uint32_t key);
    }

    namespace wards
    {
        const std::vector<ItemId> ward_items =
        {
            ItemId::Control_Ward,
            ItemId::Stealth_Ward,
            ItemId::Farsight_Alteration,
            ItemId::Harrowing_Crescent,
            ItemId::Frostfang,
            ItemId::Targons_Buckler,
            ItemId::Runesteel_Spaulders,
            ItemId::Black_Mist_Scythe,
            ItemId::Bulwark_of_the_Mountain,
            ItemId::Shard_of_True_Ice,
            ItemId::Pauldrons_of_Whiterock
        };

        const std::vector<ItemId> targetable_ward =
        {
            ItemId::Control_Ward,
            ItemId::Stealth_Ward,
            ItemId::Harrowing_Crescent,
            ItemId::Frostfang,
            ItemId::Targons_Buckler,
            ItemId::Runesteel_Spaulders,
            ItemId::Black_Mist_Scythe,
            ItemId::Bulwark_of_the_Mountain,
            ItemId::Shard_of_True_Ice,
            ItemId::Pauldrons_of_Whiterock
        };

        const std::vector<ItemId> support_items =
        {
            ItemId::Harrowing_Crescent,
            ItemId::Frostfang,
            ItemId::Targons_Buckler,
            ItemId::Runesteel_Spaulders,
            ItemId::Black_Mist_Scythe,
            ItemId::Bulwark_of_the_Mountain,
            ItemId::Shard_of_True_Ice,
            ItemId::Pauldrons_of_Whiterock,
        };

        const std::vector<string> ward_names =
        {
            "BlueTrinket",
            "JammerDevice",
            "PerksZombieWard",
            "SightWard",
            "VisionWard",
            "YellowTrinket",
            "YellowTrinketUpgrade",
        };

        bool is_valid_ward(const game_object_script object);
    }

    namespace draw
    {
        struct draw_inst
        {
            float value;
            unsigned long color;
            uint8_t transparency;

            draw_inst(float value, unsigned long color) : value(value), color(color), transparency(255)
            {}

            draw_inst(float value, unsigned long color, uint8_t transparency) : value(value), color(color), transparency(transparency)
            {}

            draw_inst() : value(0.f), color(0UL), transparency(255)
            {}
        };

        void dmg_rl(game_object_script target, float value, unsigned long color, bool draw_dmg_text, bool physical_shield, bool magical_shield);

        void dmg_rl(game_object_script target, std::vector<draw_inst> instructions, bool draw_dmg_text, bool physical_shield, bool magical_shield);

        void dmg_lr(game_object_script target, float value, unsigned long color, bool draw_dmg_text, bool physical_shield, bool magical_shield);

        void dmg_lr(game_object_script target, std::vector<draw_inst> instructions, bool draw_dmg_text, bool physical_shield, bool magical_shield);

        void mana_bar(game_object_script target, float mana_cost, unsigned long color);

        void semi_circle(vector pos, float radius, float width, float quality, float degree, unsigned long color);

        void draw_spell_range(float range, vector center, float offset, int size, int tickness, spellslot spellslot, bool drawIcon, int og);
    }

    namespace geometry
    {
        vector get_best_position(vector position, float range, float radius, std::vector<game_object_script> objects, size_t* buffer = nullptr);

        vector get_position_average(vector position, float range, std::vector<game_object_script> objects);
    }

    namespace misc
    {
        inline void decompose_argb(const uint32_t colour, int callback[4])
        {
            callback[3] = (colour >> 24) & 0xff; // alpha
            callback[2] = (colour >> 16) & 0xff; // blue
            callback[1] = (colour >> 8) & 0xff; // green
            callback[0] = colour & 0xff; // red
        }
    }

    namespace sort
    {
        inline std::vector<game_object_script> sort_by_highest_damage_applicable(std::vector<script_spell*> spells, damage_type damage_type, std::vector<game_object_script> objects, float extra_raw_damage = 0.f)
        {
            if (objects.empty() || objects.size() == 1) return objects;

            std::vector<game_object_script> copy_of(objects);
            std::sort(copy_of.begin(), copy_of.end(), [spells, damage_type, extra_raw_damage](game_object_script a, game_object_script b)
                {
                    const auto damage_a = damage::get_damage_on_unit(a, spells, true, extra_raw_damage, damage_type);
                    const auto damage_b = damage::get_damage_on_unit(b, spells, true, extra_raw_damage, damage_type);

                    return (damage_a > damage_b);
                }
            );

            return copy_of;
        }

        inline std::vector<game_object_script> sort_by_highest_max_health(std::vector<game_object_script> objects)
        {
            if (objects.empty() || objects.size() == 1) return objects;

            std::vector<game_object_script> copy_of(objects);
            std::sort(copy_of.begin(), copy_of.end(), [](game_object_script a, game_object_script b)
                {
                    return (a->get_max_health() > b->get_max_health());
                }
            );

            return copy_of;
        }

        inline std::vector<game_object_script> sort_by_lowest_max_health(std::vector<game_object_script> objects)
        {
            if (objects.empty() || objects.size() == 1) return objects;

            std::vector<game_object_script> copy_of(objects);
            std::sort(copy_of.begin(), copy_of.end(), [](game_object_script a, game_object_script b)
                {
                    return (a->get_max_health() < b->get_max_health());
                }
            );

            return copy_of;
        }

        inline std::vector<game_object_script> sort_by_highest_health(std::vector<game_object_script> objects)
        {
            if (objects.empty() || objects.size() == 1) return objects;

            std::vector<game_object_script> copy_of(objects);
            std::sort(copy_of.begin(), copy_of.end(), [](game_object_script a, game_object_script b)
                {
                    return (a->get_health() > b->get_health());
                }
            );

            return copy_of;
        }

        inline std::vector<game_object_script> sort_by_lowest_health(std::vector<game_object_script> objects)
        {
            if (objects.empty() || objects.size() == 1) return objects;

            std::vector<game_object_script> copy_of(objects);
            std::sort(copy_of.begin(), copy_of.end(), [](game_object_script a, game_object_script b)
                {
                    return (a->get_health() < b->get_health());
                }
            );

            return copy_of;
        }

        inline std::vector<game_object_script> sort_by_highest_health_percent(std::vector<game_object_script> objects)
        {
            if (objects.empty() || objects.size() == 1) return objects;

            std::vector<game_object_script> copy_of(objects);
            std::sort(copy_of.begin(), copy_of.end(), [](game_object_script a, game_object_script b)
                {
                    return (a->get_health_percent() > b->get_health_percent());
                }
            );

            return copy_of;
        }

        inline std::vector<game_object_script> sort_by_lowest_health_percent(std::vector<game_object_script> objects)
        {
            if (objects.empty() || objects.size() == 1) return objects;

            std::vector<game_object_script> copy_of(objects);
            std::sort(copy_of.begin(), copy_of.end(), [](game_object_script a, game_object_script b)
                {
                    return (a->get_health_percent() < b->get_health_percent());
                }
            );

            return copy_of;
        }

        inline std::vector<game_object_script> sort_by_highest_distance(vector position, std::vector<game_object_script> objects)
        {
            if (objects.empty() || objects.size() == 1) return objects;

            std::vector<game_object_script> copy_of(objects);
            std::sort(copy_of.begin(), copy_of.end(), [position](game_object_script a, game_object_script b)
                {
                    return (a->get_distance(position) < b->get_distance(position));
                }
            );

            return copy_of;
        }

        inline std::vector<game_object_script> sort_by_closest_distance(vector position, std::vector<game_object_script> objects)
        {
            if (objects.empty() || objects.size() == 1) return objects;

            std::vector<game_object_script> copy_of(objects);
            std::sort(copy_of.begin(), copy_of.end(), [position](game_object_script a, game_object_script b)
                {
                    return (a->get_distance(position) < b->get_distance(position));
                }
            );

            return copy_of;
        }

        inline std::vector<vector> sort_by_closest_distance(vector position, std::vector<vector> vectors)
        {
            if (vectors.empty() || vectors.size() == 1) return vectors;

            std::vector<vector> copy_of(vectors);
            std::sort(copy_of.begin(), copy_of.end(), [position](vector a, vector b)
                {
                    return (a.distance(position) < b.distance(position));
                }
            );

            return copy_of;
        }

        //Get best priority target from a TreeEntry* PriorityList, need to check if return its empty_list.
        inline std::vector<std::pair<game_object_script, std::int32_t>> sort_by_priority(TreeEntry* priority_entry, std::vector<game_object_script> objects)
        {
            std::vector<std::pair<game_object_script, std::int32_t>> valid_objects;
            if (priority_entry == nullptr || priority_entry->element_type() != TreeEntryType::ProrityList || objects.empty()) return valid_objects;
            for (auto&& object : objects)
            {
                auto object_prority = priority_entry->get_prority(object->get_network_id());

                if (object_prority.first == -1 || !object_prority.second)
                    continue;

                if (object->is_dead() || object->is_invulnerable())
                    continue;

                valid_objects.push_back({ object, object_prority.first });
            }

            std::sort(valid_objects.begin(), valid_objects.end(), [](const std::pair<game_object_script, std::int32_t>& a, const std::pair<game_object_script, std::int32_t>& b) {
                return (a.second < b.second);
                });

            return valid_objects;
        }
    }

    namespace filter
    {
        inline std::vector<game_object_script> case_distance_is_high_equals_than(vector position, float range, std::vector<game_object_script> objects)
        {
            std::vector<game_object_script> _filtered_objects;

            if (!objects.empty())
            {
                std::copy_if(objects.begin(), objects.end(), _filtered_objects.begin(),

                    [position, range](game_object_script object)
                    {
                        return (object->get_position().distance(position) >= range);
                    }
                );
            }

            return _filtered_objects;
        }

        inline std::vector<game_object_script> case_distance_is_lower_equals_than(vector position, float range, std::vector<game_object_script> objects)
        {
            std::vector<game_object_script> _filtered_objects;

            if (!objects.empty())
            {
                std::copy_if(objects.begin(), objects.end(), _filtered_objects.begin(),

                    [position, range](game_object_script object)
                    {
                        return (object->get_position().distance(position) <= range);
                    }
                );
            }

            return _filtered_objects;
        }

        inline std::vector<game_object_script> case_can_kill(std::vector<script_spell*> spells, std::vector<game_object_script> objects, float raw_damage = 0.f, damage_type raw_dmg_type = damage_type::physical, std::vector<std::function<float(game_object_script)>> custom_damage = {})
        {
            std::vector<game_object_script> _filtered_objects;

            if (!objects.empty() && (!spells.empty() || raw_damage > 0 || !custom_damage.empty()))
            {
                for (auto& _unit : objects)
                {
                    if (!_unit || !_unit->is_valid() || _unit->is_dead() || !_unit->is_valid_target()) continue;
                    if (_unit->is_ai_hero() && (_unit->is_invulnerable() || _unit->is_zombie())) continue;
                    if (!myhero->can_cast()) continue;

                    const auto distance = myhero->get_distance(_unit);
                    auto damage = 0.f;

                    // Spells Damage Sec.
                    for (auto _spell : spells)
                    {
                        if (!_spell || !_spell->is_ready() || !_spell->can_cast(_unit)) continue;

                        auto delay = _spell->delay;
                        auto speed = _spell->speed;

                        if (delay > 0.f || speed > 0.f)
                        {
                            auto health = _unit->get_health();
                            auto health_pred = health_prediction->get_incoming_damage(_unit, (delay + (speed > 0.f ? (distance / speed) : 0.f)), true);
                            auto damage = _spell->get_damage(_unit);

                            if (health_pred <= 0 || (health - (health_pred + damage)) > 0) continue;
                        }

                        damage += _spell->get_damage(_unit);
                    }

                    // Custom Damage Fun. Sec.
                    for (auto _custom_damage : custom_damage)
                        if (_custom_damage) damage += _custom_damage(_unit);

                    // Extra Raw Damage Sec.
                    if (raw_damage > 0)
                        damage += damagelib->calculate_damage_on_unit(myhero, _unit, raw_dmg_type, raw_damage);

                    if (_unit->get_real_health(true, true) < damage)
                    {
                        _filtered_objects.push_back(_unit);
                    }
                }

            }

            return _filtered_objects;
        }

        inline std::vector<game_object_script> case_is_duplicated(std::vector<game_object_script> objects)
        {
            std::vector<game_object_script> _filtered_objects;
            std::vector<uint32_t> _fingerprint;

            for (auto& _unit : objects)
            {
                if (!_unit || !_unit->is_valid() || std::find(_fingerprint.begin(), _fingerprint.end(), _unit->get_network_id()) != _fingerprint.end()) continue;

                _filtered_objects.push_back(_unit);
                _fingerprint.push_back(_unit->get_network_id());
            }

            return _filtered_objects;
        }
    }
};
