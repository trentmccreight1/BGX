#include "utils.hpp"
#include "../plugin_sdk/cpplinq.h"

namespace utils
{
    std::string toUpper(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) {return static_cast<char>(std::toupper(c)); });
        return str;
    }

    std::string toLower(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) {return static_cast<char>(std::tolower(c)); });
        return str;
    }

    bool strContains(std::string strA, std::string strB, bool ignoreCase)
    {
        if (strA.empty() || strB.empty())
            return true;

        if (ignoreCase)
        {
            strA = toLower(strA);
            strB = toLower(strB);
        }

        if (strA.find(strB) != std::string::npos)
            return true;

        return false;
    }

    bool isYuumiAttached(const game_object_script& target)
    {
        // Check if the target is Yuumi and if it's attached to someone
        if (target->get_champion() == champion_id::Yuumi)
        {
            const auto& yuumiBuff = target->get_buff(buff_hash("YuumiWAttach"));
            if (yuumiBuff && yuumiBuff->get_caster()->get_handle() == target->get_handle()) return true;
        }
        return false;
    }

    bool isValidRecalling(const game_object_script& target, float range, const vector& from)
    {
        // Get valid recalling enemies in FoW
        auto fromPos = from;
        if (fromPos == vector::zero) fromPos = myhero->get_position();
        const auto& isValid = (target->is_valid() && target->is_ai_hero() && target->is_targetable() && target->is_targetable_to_team(myhero->get_team()) && !target->is_invulnerable() && isRecalling(target) && !target->is_dead() && fromPos.distance(target->get_position()) <= range);
        return isValid;
    }

    bool isRecalling(const game_object_script& target)
    {
        // Get if target is recalling
        const auto& isRecalling = target->is_teleporting() && (target->get_teleport_state() == "recall" || target->get_teleport_state() == "SuperRecall" || target->get_teleport_state() == "SummonerTeleport");
        return isRecalling;
    }

    bool debuffCantCast()
    {
        // Check if player has any debuff that prevents spell casting
        const auto& stunBuffList = { buff_type::Stun, buff_type::Silence, buff_type::Taunt, buff_type::Polymorph, buff_type::Fear, buff_type::Charm, buff_type::Suppression, buff_type::Knockup, buff_type::Knockback, buff_type::Asleep };
        for (auto&& buff : myhero->get_bufflist())
        {
            if (buff == nullptr || !buff->is_valid() || !buff->is_alive()) continue;
            for (const auto& buffType : stunBuffList)
            {
                if (buff->get_type() == buffType) return true;
            }
        }
        return false;
    }

    float getTimeToHit(prediction_input& input, prediction_output& predInfo, const bool takePing)
    {
        // Get time before spell hits
        if (predInfo.get_cast_position() == vector::zero) return FLT_MAX;
        const auto& timeToHit = (input._from.distance(predInfo.get_cast_position()) / input.speed) + input.delay + (takePing ? getPing() : 0);
        return timeToHit;
    }

    float getTotalHP(const game_object_script& target)
    {
        // Get total magic damage HP
        return target->get_health() + target->get_all_shield() + target->get_magical_shield();
    }

    float getPing()
    {
        // Get player's full ping (ping pong)
        return ping->get_ping() / 1000;
    }

    script_spell* get_script_spell(summoner_spell_type type)
    {
        script_spell* script_spell = nullptr;
        uint32_t spell_hash = 0U;
        float range = FLT_MAX;

        switch (type)
        {
        case summoner_spell_type::Ignite:
        {
            spell_hash = spell_hash("SummonerDot");
            range = 600;
            break;
        }

        case summoner_spell_type::Barrier:
        {
            spell_hash = spell_hash("SummonerBarrier");
            break;
        }

        case summoner_spell_type::Heal:
        {
            spell_hash = spell_hash("SummonerHeal");
            range = 850;
            break;
        }

        case summoner_spell_type::Exhaust:
        {
            spell_hash = spell_hash("SummonerExhaust");
            range = 650;
            break;
        }

        case summoner_spell_type::Cleanse:
        {
            spell_hash = spell_hash("SummonerBoost");
            break;
        }

        case summoner_spell_type::Flash:
        {
            spell_hash = spell_hash("SummonerFlash");
            range = 425;
            break;
        }

        case summoner_spell_type::Smite:
        {
            spell_hash = spell_hash("SummonerSmite");
            range = 560;
            break;
        }

        case summoner_spell_type::Teleport:
        {
            spell_hash = spell_hash("SummonerTeleport");
            break;
        }

        case summoner_spell_type::Ghost:
        {
            spell_hash = spell_hash("SummonerHaste");
            break;
        }

        // TODO: Add summoner spells
        case summoner_spell_type::Clarity:
        case summoner_spell_type::Snowball:
            break;

        default:
            break;
        }

        if (spell_hash)
        {
            if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash)
                script_spell = plugin_sdk->register_spell(spellslot::summoner1, range);
            else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash)
                script_spell = plugin_sdk->register_spell(spellslot::summoner2, range);
        }

        return script_spell;
    }

    vector redSpawn = vector(14626.f, 14738.f, 171.f);
    vector blueSpawn = vector(52.f, 86.f, 182.f);

    SpawnTeam getSpawnTeam(vector pos)
    {
        if (pos.x > 14000 && pos.y > 14000)
            return SpawnTeam::Red;
        else
            return SpawnTeam::Blue;
    }

    bool is_under_enemy_spawn(vector position)
    {
        for (auto&& f : entitylist->get_all_spawnpoints())
        {
            if (!f->is_enemy()) continue;
            SpawnTeam team = getSpawnTeam(f->get_position());
            if (team == SpawnTeam::Red && redSpawn.distance(position) <= 1450.f)
            {
                return true;
            }
            else if (team == SpawnTeam::Blue && blueSpawn.distance(position) <= 1450.f)
            {
                return true;
            }
        }
        return false;
    }

    bool CheckWalls(vector start, vector end)
    {
        float dist = start.distance(end);
        for (int i = 1; i < 6; i++)
        {
            if (start.extend(end, dist + 55 * i).is_wall())
                return true;
        }
        return false;
    }

    bool BigMonsterCheck(game_object_script mob)
    {
        auto name = mob->get_base_skin_name();
        return name == "SRU_Blue" || name == "SRU_Gromp" || name == "SRU_Red" || mob->is_epic_monster();
    }

    vector GetBestPositionAOE(script_spell* spell, std::vector<game_object_script>& objects)
    {
        std::vector<game_object_script> list = objects;
        vector result = {};

        for (auto& obj : list) {
            if (obj != nullptr && obj->is_valid())
                result = result + obj->get_position();
        }

        result = result / list.size();

        for (size_t i = 0; i < list.size(); i++)
        {
            if (!list[i]->is_valid_target(spell->range(), result))
            {
                list.erase(list.begin() + i);
                return GetBestPositionAOE(spell, list);
            }
        }

        return result;
    }

    bool has_mana(game_object_script object, float value)
    {
        if (value <= 0) return true;

        if (object && object->is_valid() && object->is_ai_hero())
        {
            return object->get_mana() >= value;
        }

        return false;
    }

    bool CanMove(game_object_script target)
    {
        if (!target->is_winding_up() && !target->can_move() || target->get_move_speed() < 50 || target->has_buff_type(buff_type::Stun) || target->has_buff_type(buff_type::Fear) || target->has_buff_type(buff_type::Snare) || target->has_buff_type(buff_type::Knockup) || target->is_recalling() ||
            target->has_buff_type(buff_type::Knockback) || target->has_buff_type(buff_type::Charm) || target->has_buff_type(buff_type::Taunt) || target->has_buff_type(buff_type::Suppression))
            return false;
        else
        {
            return true;
        }
    }

    int count_buff(game_object_script target, uint32_t hash)
    {
        if (target->is_valid())
        {
            auto buff = target->get_buff(hash);
            if (buff != nullptr && buff->is_valid() && buff->is_alive())
            {
                return buff->get_count();
            }
        }
        return 0;
    }



    bool IsKillableAndValidTargetCustom(script_spell* spell, damage_type type, game_object_script target, float customdamage, float distance, bool healthPredction, bool includeSkillShots)
    {
        auto calculatedDamage = customdamage;

        if (target == nullptr || target->get_name() == "gangplankbarrel")
            return false;

        if (target->has_buff_type(buff_type::Invulnerability))
        {
            return false;
        }

        if (target->has_buff(buff_hash("kindredrnodeathbuff")))
        {
            return false;
        }

        if (target->has_buff(buff_hash("Undying Rage")))
        {
            return false;
        }

        if (target->has_buff(buff_hash("JudicatorIntervention")))
        {
            return false;
        }

        if (target->has_buff(buff_hash("DiplomaticImmunity")) && !myhero->has_buff(buff_hash("poppyulttargetmark")))
        {
            return false;
        }

        if (target->has_buff(buff_hash("BansheesVeil")))
        {
            return false;
        }

        if (target->has_buff(buff_hash("SivirShield")))
        {
            return false;
        }

        if (target->has_buff(buff_hash("ShroudofDarkness")))
        {
            return false;
        }

        float healthTick = target->get_hp_regen_rate() / 2;
        float time_to_hit = spell->get_delay() + (myhero->get_distance(target) / spell->get_speed());
        int totalTicks = time_to_hit / 0.5;

        float realHealth = type == damage_type::physical ? target->get_real_health(true, false) : target->get_real_health(false, true);


        return healthPredction ? realHealth - health_prediction->get_incoming_damage(target, time_to_hit, includeSkillShots) + (healthTick * (totalTicks - 1)) < calculatedDamage : realHealth + (healthTick * (totalTicks - 1)) < calculatedDamage;
    }

    bool IsKillableAndValidTarget(script_spell* spell, damage_type type, game_object_script target, float distance, bool healthPredction, bool includeSkillShots)
    {
        auto calculatedDamage = damagelib->get_spell_damage(myhero, target, spell->get_slot());

        if (target == nullptr || target->get_name() == "gangplankbarrel")
            return false;

        if (target->has_buff_type(buff_type::Invulnerability))
        {
            return false;
        }

        if (target->has_buff(buff_hash("kindredrnodeathbuff")))
        {
            return false;
        }

        if (target->has_buff(buff_hash("Undying Rage")))
        {
            return false;
        }

        if (target->has_buff(buff_hash("JudicatorIntervention")))
        {
            return false;
        }

        if (target->has_buff(buff_hash("DiplomaticImmunity")) && !myhero->has_buff(buff_hash("poppyulttargetmark")))
        {
            return false;
        }

        if (target->has_buff(buff_hash("BansheesVeil")))
        {
            return false;
        }

        if (target->has_buff(buff_hash("SivirShield")))
        {
            return false;
        }

        if (target->has_buff(buff_hash("ShroudofDarkness")))
        {
            return false;
        }

        float healthTick = target->get_hp_regen_rate() / 2;
        float time_to_hit = spell->get_delay() + (myhero->get_distance(target) / spell->get_speed());
        int totalTicks = time_to_hit / 0.5;

        float realHealth = type == damage_type::physical ? target->get_real_health(true, false) : target->get_real_health(false, true);


        return healthPredction ? realHealth - health_prediction->get_incoming_damage(target, time_to_hit, includeSkillShots) + (healthTick * (totalTicks - 1)) < calculatedDamage : realHealth + (healthTick * (totalTicks - 1)) < calculatedDamage;
    }


    bool has_mana_percent(game_object_script object, float value)
    {
        if (value <= 0) return true;

        if (object && object->is_valid() && object->is_ai_hero())
        {
            return object->get_mana_percent() >= value;
        }

        return false;
    }

    bool has_mana_percent(game_object_script object, TreeEntry* entry_holder)
    {
        if (!entry_holder) return true;

        return has_mana_percent(object, entry_holder->get_int());
    }

    int count_nearby_objects(vector position, float range, std::vector<game_object_script> objects, bool is_target, ::geometry::polygon* region)
    {
        return get_nearby_objects(position, range, objects, is_target, region).size();
    }

    game_object_script get_nearby_object(vector position, float range, std::vector<game_object_script> objects, bool is_target, ::geometry::polygon* region)
    {
        auto nearby_objects = get_nearby_objects(position, range, objects, is_target, region);
        return nearby_objects.empty() ? nullptr : sort::sort_by_closest_distance(position, nearby_objects).front();
    }

    std::vector<game_object_script> get_nearby_objects(vector position, float range, std::vector<game_object_script> objects, bool is_target, ::geometry::polygon* region)
    {
        return cpplinq::from(objects) >>
            cpplinq::where([&](game_object_script object)
                {
                    if (!object || !object->is_valid() || object->is_dead() || object->get_distance(position) > range) return false;
                    if (is_target && object->is_enemy() && !object->is_valid_target()) return false;
                    if (region && !region->points.empty() && !region->is_inside(object->get_position())) return false;
                    return true;

                }) >> cpplinq::to_vector();
    }

    Raytrace_Output Raytrace_Input::process()
    {
        if (!this->is_valid())
            return Raytrace_Output::empty();

        std::vector<game_object_script> raytraced_objects;

        if (this->has_collision_flags())
        {
            std::vector<game_object_script> _objects;

            if (std::count(this->_collision_flags.begin(), this->_collision_flags.end(), collisionable_objects::allies) && this->_query_type != raytrace_query_type::hero_ally)
            {
                _objects = get_nearby_objects(myhero->get_position(), FLT_MAX, entitylist->get_ally_heroes(), false);
                this->_targets.reserve(_objects.size());
                this->_targets.insert(this->_targets.end(), _objects.begin(), _objects.end());

                _objects.clear();
                _objects.shrink_to_fit();
            }

            if (std::count(this->_collision_flags.begin(), this->_collision_flags.end(), collisionable_objects::heroes) && this->_query_type != raytrace_query_type::hero_enemy)
            {
                _objects = get_nearby_objects(myhero->get_position(), FLT_MAX, entitylist->get_enemy_heroes(), true);
                this->_targets.reserve(_objects.size());
                this->_targets.insert(this->_targets.end(), _objects.begin(), _objects.end());

                _objects.clear();
                _objects.shrink_to_fit();
            }

            if (std::count(this->_collision_flags.begin(), this->_collision_flags.end(), collisionable_objects::minions) && this->_query_type != raytrace_query_type::minion)
            {
                _objects = get_nearby_objects(myhero->get_position(), FLT_MAX, entitylist->get_enemy_minions(), true);
                this->_targets.reserve(_objects.size());
                this->_targets.insert(this->_targets.end(), _objects.begin(), _objects.end());
                _objects = get_nearby_objects(myhero->get_position(), FLT_MAX, entitylist->get_jugnle_mobs_minions(), true);
                if (this->_targets.capacity() < entitylist->get_jugnle_mobs_minions().size())
                    this->_targets.reserve(entitylist->get_jugnle_mobs_minions().size() - _objects.size());
                this->_targets.insert(this->_targets.end(), _objects.begin(), _objects.end());

                _objects.clear();
                _objects.shrink_to_fit();
            }

            // TODO: Add support to yasuo-wall

            // TODO: Add support for polygons (Terrain checker)

            this->_targets = utils::filter::case_is_duplicated(this->_targets);
            this->_targets.shrink_to_fit();
        }

        bool had_collisions = false;
        for (auto& _unit : this->_targets)
        {
            if (
                !_unit ||
                !_unit->is_valid() ||
                _unit->get_network_id() == myhero->get_network_id()
                )
                continue;

            const auto distance = _unit->get_position().distance(this->_from, this->_to, true);

            if (distance <= (this->_radius + (this->_use_boundings ? _unit->get_bounding_radius() : 0.f)))
            {
                if (this->_query_type == raytrace_query_type::minion && !_unit->is_ai_minion())
                {
                    had_collisions = true; break;
                }

                if (this->_query_type == raytrace_query_type::hero_ally && !(_unit->is_ai_hero() && _unit->is_ally()))
                {
                    had_collisions = true; break;
                }

                if (this->_query_type == raytrace_query_type::hero_enemy && !(_unit->is_ai_hero() && _unit->is_enemy()))
                {
                    had_collisions = true; break;
                }

                raytraced_objects.push_back(_unit);
            }
        }

        return Raytrace_Output(raytraced_objects, had_collisions);
    }

    bool is_minion_instance(game_object_script object, minion_type type)
    {
        if (!object || !object->is_ai_minion()) return false;

        return (static_cast<std::underlying_type<minion_type>::type>(type) == object->get_minion_type());
    }

    namespace buffs
    {
        buff_instance_script get_buff_by_fragmented_name(game_object_script object, std::vector<string> args)
        {
            if (!args.empty() && object && object->is_valid() && !object->get_bufflist().empty())
            {
                for (buff_instance_script _buff_inst : object->get_bufflist())
                {
                    if (!_buff_inst || !_buff_inst->is_valid()) continue;

                    size_t _name_count = 0;
                    for (string _arg_name : args)
                    {
                        if (_buff_inst->get_name().find(_arg_name) != string::npos) _name_count++;
                    }

                    if (_name_count == args.size()) return _buff_inst;
                }
            }

            return nullptr;
        }

        bool has_buff_by_fragmented_name(game_object_script object, std::vector<string> args)
        {
            return get_buff_by_fragmented_name(object, args) != nullptr;
        }

        bool has_group_control_debuff(game_object_script object)
        {
            if (!object || !object->is_valid() || object->is_dead()) return false;

            return object->has_buff_type(
                {
                    buff_type::Stun, buff_type::Polymorph, buff_type::Taunt,
                    buff_type::Snare, buff_type::Fear, buff_type::Charm, buff_type::Suppression,
                    buff_type::Knockup, buff_type::Grounded, buff_type::Asleep
                });
        }

        bool has_invulnerability_buff(game_object_script object)
        {
            if (!object || !object->is_valid() || object->is_dead()) return false;

            return object->has_buff({ buff_hash("ZhonyasRingShield"), buff_hash("willrevive"), buff_hash("ChronoRevive"), buff_hash("BardRStasis"), buff_hash("LissandraRSelf") });
        }

        bool has_buff_aa_block(game_object_script object)
        {
            return (
                object &&
                object->is_valid() &&
                !object->is_dead() &&
                (object->has_buff(buff_hash("ShenWBuff")) || object->has_buff(buff_hash("JaxCounterStrike")) || object->has_buff(buff_hash("FioraW")))
                );
        }

        bool has_unkillable_buff(game_object_script target)
        {
            return target->is_zombie() || target->has_buff({ buff_hash("UndyingRage"), buff_hash("ChronoShift"), buff_hash("KayleR"), buff_hash("KindredRNoDeathBuff") });;
        }

        bool buff_special(buff_instance_script buff)
        {
            switch (buff->get_hash_name())
            {
            case buff_hash("ZhonyasRingShield"):
                return true;
                break;

            case buff_hash("ChronoRevive"):
                return true;
                break;

            case buff_hash("BardRStasis"):
                return true;
                break;

            case buff_hash("LissandraRSelf"):
                return true;
                break;

            }
            return false;
        }
    }

    float damage::get_damage_on_unit(game_object_script target, std::vector<script_spell*> spells, bool ignore_non_ready, float extra_damage, damage_type damage_type)
    {
        if (!target || !target->is_valid_target() || spells.empty()) return 0.f;

        float _damage = 0.f;

        if (!spells.empty())
        {
            for (auto& _spell : spells)
            {
                const auto _spell_info = myhero->get_spell(_spell->get_slot());
                if (_spell && _spell_info && _spell_info->is_learned() && (!ignore_non_ready || _spell->is_ready()))
                {
                    _damage += _spell->get_damage(target);
                }
            }
        }

        if (extra_damage > 0)
            _damage += damagelib->calculate_damage_on_unit(myhero, target, damage_type, extra_damage);

        return _damage;
    }

    bool damage::can_kill_with_aa(game_object_script target, bool physical_shield, bool magical_shield)
    {
        if (!target || !target->is_valid_target() || !myhero->is_in_auto_attack_range(target) || !target->is_attack_allowed_on_target() || target->is_dead()) return false;

        return (myhero->get_auto_attack_damage(target) >= target->get_real_health(physical_shield, magical_shield));
    }

    namespace config
    {
        hit_chance get_hitchance_by_config(TreeEntry* hit)
        {
            switch (hit->get_int())
            {
            case 0:
                return hit_chance::low;
            case 1:
                return hit_chance::medium;
            case 2:
                return hit_chance::high;
            case 3:
                return hit_chance::very_high;
            case 4:
                return hit_chance::dashing;
            default:
                return hit_chance::immobile;
            }
        }

        bool get_status_from_config(std::map<uint32_t, TreeEntry*> map, uint32_t key)
        {
            const auto& found_value = map.find(key);

            if (found_value == map.end()) return false;

            return (found_value->second && found_value->second->get_bool());
        }
    }

    bool wards::is_valid_ward(const game_object_script object)
    {
        if (object == nullptr || !object->is_valid()) return false;
        for (auto ward_name : ward_names) {
            if (object->get_name().find(ward_name) != string::npos) {
                return true;
            }
        }

        return false;
    }

    namespace draw
    {
        void dmg_rl(game_object_script target, float value, unsigned long color, bool draw_dmg_text, bool physical_shield, bool magical_shield)
        {
            dmg_rl(target, { draw_inst(value, color) }, draw_dmg_text, physical_shield, magical_shield);
        }

        void dmg_rl(game_object_script target, std::vector<draw_inst> instructions, bool draw_dmg_text, bool physical_shield, bool magical_shield)
        {
            if (target == nullptr || !target->is_valid() || !target->is_hpbar_recently_rendered()) return;
            if (instructions.empty()) return;

            const auto bar_pos_fix = target->get_hpbar_pos();
            auto bar_pos = bar_pos_fix;

            if (!bar_pos.is_valid() || target->is_dead() || !target->is_visible()) return;

            const auto health = target->get_health();
            const auto bar_size = (105 * (health / target->get_max_health()));

            auto volatile_bar_pos = bar_pos = vector(bar_pos.x + bar_size, bar_pos.y -= 10);
            auto volatile_bar_size = 0.f;
            auto damage = 0.f;

            for (auto instruction : instructions)
            {
                if (instruction.value == 0.f) continue;
                if (volatile_bar_size > bar_size) continue;

                auto damage_size = (105 * (instruction.value / target->get_max_health()));

                if ((damage_size + volatile_bar_size) > bar_size) damage_size = bar_size - volatile_bar_size;

                auto size = vector(volatile_bar_pos.x + (damage_size * -1), volatile_bar_pos.y + 11);

                int color[4];

                misc::decompose_argb(instruction.color, color);
                draw_manager->add_filled_rect(volatile_bar_pos, size, D3DCOLOR_ARGB(instruction.transparency, color[0], color[1], color[2]));

                damage += instruction.value;
                volatile_bar_size += damage_size;
                volatile_bar_pos = vector(size.x, bar_pos.y);
            }

            if (draw_dmg_text) {
                if (damage == 0.f) return;
                auto damage_percent = damage / target->get_real_health(physical_shield, magical_shield) * 100;
                auto dmg_text = draw_manager->calc_text_size(12, "%.0f %%", damage_percent);
                if (damage_percent > 100.f) damage_percent = 100.f;
                auto draw_dmg_pos = vector(bar_pos_fix.x + 105 - dmg_text.x, (bar_pos_fix.y + 2.f) - dmg_text.y);
                draw_manager->add_text_on_screen(draw_dmg_pos, D3DCOLOR_ARGB(255, 255, 255, 255), 12, "%.0f %%", damage_percent);
            }
        }

        void dmg_lr(game_object_script target, float value, unsigned long color, bool draw_dmg_text, bool physical_shield, bool magical_shield)
        {
            dmg_lr(target, { draw_inst(value, color) }, draw_dmg_text, physical_shield, magical_shield);
        }

        void dmg_lr(game_object_script target, std::vector<draw_inst> instructions, bool draw_dmg_text, bool physical_shield, bool magical_shield)
        {
            if (target == nullptr || !target->is_valid() || !target->is_hpbar_recently_rendered()) return;
            if (instructions.empty()) return;

            const auto bar_pos_fix = target->get_hpbar_pos();
            auto bar_pos = bar_pos_fix;

            if (!bar_pos.is_valid() || target->is_dead() || !target->is_visible()) return;

            const auto health = target->get_health();
            const auto bar_size = (105 * (health / target->get_max_health()));

            auto volatile_bar_pos = bar_pos = vector(bar_pos.x, bar_pos.y -= 10);
            auto volatile_bar_size = 0.f;
            auto damage = 0.f;

            for (auto instruction : instructions)
            {
                if (instruction.value == 0.f) continue;
                if (volatile_bar_size > bar_size) continue;

                auto damage_size = (105 * (instruction.value / target->get_max_health()));

                if ((damage_size + volatile_bar_size) > bar_size) damage_size = bar_size - volatile_bar_size;

                auto size = vector(volatile_bar_pos.x + damage_size, volatile_bar_pos.y + 11);

                int color[4];

                misc::decompose_argb(instruction.color, color);
                draw_manager->add_filled_rect(volatile_bar_pos, size, D3DCOLOR_ARGB(instruction.transparency, color[0], color[1], color[2]));

                damage += instruction.value;
                volatile_bar_size += damage_size;
                volatile_bar_pos = vector(size.x, bar_pos.y);
            }

            if (draw_dmg_text)
            {
                if (damage == 0.f) return;
                auto damage_percent = damage / target->get_real_health(physical_shield, magical_shield) * 100;
                auto dmg_text = draw_manager->calc_text_size(12, "%.0f %%", damage_percent);
                if (damage_percent > 100.f) damage_percent = 100.f;
                auto draw_dmg_pos = vector(bar_pos_fix.x + 30 - dmg_text.x, (bar_pos_fix.y + 3.f) - dmg_text.y);
                draw_manager->add_text_on_screen(draw_dmg_pos, D3DCOLOR_ARGB(255, 255, 255, 255), 12, "%.0f %%", damage_percent);
            }
        }

        void mana_bar(game_object_script target, float mana_cost, unsigned long color)
        {
            if (target == nullptr || !target->is_valid() || !target->is_hpbar_recently_rendered()) return;

            auto bar_pos = target->get_hpbar_pos();
            if (!bar_pos.is_valid() || target->is_dead() || !target->is_visible()) return;

            const auto mana = target->get_mana();
            bar_pos = vector(bar_pos.x + (105 * (mana / target->get_max_mana())), bar_pos.y += 3.6f);

            auto mana_size = (105 * (mana_cost / target->get_max_mana()));

            if (mana_cost >= mana)
            {
                mana_size = (105 * (mana / target->get_max_mana()));
            }

            if (mana_size > 105)
            {
                mana_size = 105;
            }

            const auto size = vector(bar_pos.x + (mana_size * -1), bar_pos.y + 3.0f);
            draw_manager->add_filled_rect(bar_pos, size, color);
        }

        void semi_circle(vector pos, float radius, float width, float quality, float degree, unsigned long color)
        {
            float qty = (M_PI * 2) / quality;
            auto  a = vector(pos.x + radius * cos(6.28), pos.y - radius * sin(6.28), pos.z);
            for (float theta = qty; theta <= (M_PI / 180) * degree; theta += qty)
            {
                auto b = vector(pos.x + radius * cos(theta + 6.28), pos.y - radius * sin(theta + 6.28), pos.z);
                draw_manager->add_line(a, b, color, width);
                a = b;
            }
        }

        void draw_spell_range(float range, vector center, float offset, int size, int tickness, spellslot spellslot, bool drawIcon, int og)
        {
            auto texture = myhero->get_spell(spellslot)->get_icon_texture();
            if (!texture)
            {
                console->print_error("[TrentAIO] - Error Code: _G-DIR-TEX");
                return;
            }

            // Calculate color animation
            auto heroPos = center;
            const int numSegments = 60;
            const float segmentAngle = 360.0f / numSegments;
            float gameTime = gametime->get_time();
            int frameRate = 60;
            int frameCount = (int)(gameTime * frameRate);
            int animationPeriod = 60;
            int frameOffset = frameCount % animationPeriod;

            // Calculate color for circle
            std::vector<unsigned long> colors;
            for (int i = 0; i < numSegments; i++)
            {
                int animationOffset = ((i + frameOffset) % numSegments) + (numSegments / 2);
                int r = (int)(128.0f + 127.0f * sin(animationOffset * 0.1f));
                int g = (int)(128.0f + 127.0f * cos(animationOffset * 0.1f));
                int b = (int)(128.0f + 127.0f * sin((animationOffset + 30) * 0.1f));
                auto color = MAKE_COLOR(r, g, b, 140);
                colors.push_back(color);
            }

            // Draw circle
            for (int i = 0; i < numSegments; i++)
            {
                float startAngle = i * segmentAngle;
                float endAngle = (i + 1) * segmentAngle;
                auto color = colors[i];

                vector startVec = heroPos + vector(cos(startAngle * (M_PI / 180)) * range, sin(startAngle * (M_PI / 180)) * range, 0.f);
                vector endVec = heroPos + vector(cos(endAngle * (M_PI / 180)) * range, sin(endAngle * (M_PI / 180)) * range, 0.f);
                draw_manager->add_line(startVec, endVec, color, tickness);
            }

            if (drawIcon)
            {
                vector origin;
                switch (og)
                {
                case 1:
                    origin = vector(-150, -150, heroPos.z);
                    break;
                case 2:
                    origin = vector(-150, -450, heroPos.z);
                    break;
                case 3:
                    origin = vector(-150, -750, heroPos.z);
                    break;
                default:
                    origin = vector(-150, -150, heroPos.z);
                    break;
                }

                vector a = heroPos.extend(origin, (range + offset) * -1);
                renderer->world_to_screen(a, a);
                vector b = vector(a.x - 2, a.y - 2);
                vector c = vector(a.x + size + 2, a.y + size + 2);
                auto color = colors[numSegments / 2]; // Use color
                draw_manager->add_filled_rect(b, c, color);
                draw_manager->add_image(texture, a, vector(size, size));
            }
        }
    }

    namespace geometry
    {
        vector get_best_position(vector position, float range, float radius, std::vector<game_object_script> objects, size_t* buffer)
        {
            vector best_point = vector::zero;
            int16_t point_count = -1;

            if (!objects.empty() && position.is_valid())
            {
                for (auto& _object : objects)
                {
                    int16_t _nearby_objects_count = 0;

                    if (!_object || !_object->is_valid() || _object->is_dead() || _object->get_distance(position) > range)
                        continue;

                    for (auto& _other : objects)
                    {
                        if (!_other || !_other->is_valid())
                            continue;

                        if (_other->get_distance(_object) <= (_other->get_bounding_radius() + radius))
                        {
                            _nearby_objects_count++;
                        }
                    }

                    if (_nearby_objects_count > point_count)
                    {
                        best_point = _object->get_position();
                        point_count = _nearby_objects_count;
                    }
                }
            }

            if (buffer && point_count >= 0) *buffer = point_count;

            return best_point;
        }

        vector get_position_average(vector position, float range, std::vector<game_object_script> objects)
        {
            vector average_point = vector::zero;
            size_t average_count = 0;

            if (!objects.empty())
            {
                for (auto& _unit : objects)
                {
                    if (_unit && _unit->is_valid() && !_unit->is_dead() && _unit->get_distance(position) <= (range + _unit->get_bounding_radius()))
                    {
                        average_point = average_point + _unit->get_position();
                        average_count++;
                    }
                }
            }

            return (average_point != vector::zero ? (average_point / average_count) : average_point);
        }
    }
}