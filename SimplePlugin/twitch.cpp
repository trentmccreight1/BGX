#include "../plugin_sdk/plugin_sdk.hpp"
#include "../plugin_sdk/plugin_sdk.hpp"
#include "twitch.h"
#include "permashow.hpp"

namespace twitch
{

    script_spell* q = nullptr;
    script_spell* w = nullptr;
    script_spell* e = nullptr;
    script_spell* r = nullptr;
    script_spell* b = nullptr;

    TreeTab* main_tab = nullptr;

    namespace draw_settings
    {
        TreeEntry* draw_range_q = nullptr;
        TreeEntry* q_color = nullptr;
        TreeEntry* draw_range_w = nullptr;
        TreeEntry* w_color = nullptr;
        TreeEntry* draw_range_e = nullptr;
        TreeEntry* e_color = nullptr;
        TreeEntry* draw_range_r = nullptr;
        TreeEntry* r_color = nullptr;
        TreeEntry* draw_e_stacks_time = nullptr;
        TreeEntry* draw_damage_e = nullptr;
        TreeEntry* draw_q_timeleft = nullptr;
        TreeEntry* draw_r_timeleft = nullptr;
        TreeEntry* draw_e_stacks = nullptr;
        TreeEntry* e_stacks_color = nullptr;
        TreeEntry* e_stacks_time_color = nullptr;
        TreeEntry* q_timeleft_color = nullptr;
        TreeEntry* r_timeleft_color = nullptr;
    }

    namespace combo
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* w_mode = nullptr;
        TreeEntry* w_dont_use_on_q = nullptr;
        TreeEntry* w_dont_use_on_r = nullptr;
        TreeEntry* w_dont_use_if_killable_by_x_aa = nullptr;
        TreeEntry* use_e = nullptr;
        TreeEntry* e_if_target_leaving_range = nullptr;
        TreeEntry* e_leaving_range_minimum_stacks = nullptr;
        TreeEntry* e_use_before_death = nullptr;
        TreeEntry* e_before_death_use_on_x_stacks = nullptr;
        TreeEntry* e_before_death_myhero_under_hp = nullptr;
        TreeEntry* e_before_death_calculate_incoming_damage = nullptr;
        TreeEntry* e_before_death_damage_time = nullptr;
        TreeEntry* e_before_death_over_my_hp_in_percent = nullptr;
        TreeEntry* use_r = nullptr;
        TreeEntry* r_use_if_enemies_more_than = nullptr;
    }

    namespace harass
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* use_e = nullptr;
        TreeEntry* e_use_on_x_stacks = nullptr;
    }

    namespace laneclear
    {
        TreeEntry* spell_farm = nullptr;
        TreeEntry* use_q = nullptr;
        TreeEntry* use_q_on_turret = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* w_minimum_minions = nullptr;
        TreeEntry* use_e = nullptr;
        TreeEntry* e_use_if_killable_minions = nullptr;
    }

    namespace jungleclear
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* use_e = nullptr;
    }

    namespace fleemode
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
    }

    namespace antigapclose
    {
        TreeEntry* use_w = nullptr;
    }

    namespace misc
    {
        TreeEntry* stealth_recall = nullptr;
        TreeEntry* stealth_recall_key = nullptr;
    }

    namespace hitchance
    {
        TreeEntry* w_hitchance = nullptr;
    }

    void on_update();
    void on_draw();
    void on_after_attack_orbwalker(game_object_script target);
    void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args);

    void w_logic();
    void e_logic();
    void r_logic();


    hit_chance get_hitchance(TreeEntry* entry);
    inline void draw_dmg_rl(game_object_script target, float damage, unsigned long color);
    int get_twitch_e_stacks(game_object_script target);

    void load()
    {
        q = plugin_sdk->register_spell(spellslot::q, 500);
        w = plugin_sdk->register_spell(spellslot::w, 950);
        w->set_skillshot(0.25f, 100.f, 1400.0f, { }, skillshot_type::skillshot_circle);
        e = plugin_sdk->register_spell(spellslot::e, 1200);
        r = plugin_sdk->register_spell(spellslot::r, 1100);
        b = plugin_sdk->register_spell(spellslot::recall, 0);

        main_tab = menu->create_tab("TrentAIO", "Trent Twitch");
        main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
        {

            auto combo = main_tab->add_tab(myhero->get_model() + ".combo", "Combo Settings");
            {
                combo::use_q = combo->add_checkbox(myhero->get_model() + ".combo.q", "Use Q after AA", true);
                combo::use_w = combo->add_checkbox(myhero->get_model() + ".combo.w", "Use W", true);
                auto w_config = combo->add_tab(myhero->get_model() + "combo.w.config", "W Config");
                {
                    combo::w_mode = w_config->add_combobox(myhero->get_model() + ".combo.w.mode", "W Mode", { {"If enemy above AA range or After AA", nullptr}, {"In Combo", nullptr}, {"After AA", nullptr } }, 0);
                    combo::w_dont_use_if_killable_by_x_aa = w_config->add_slider(myhero->get_model() + ".combo.w.dont_use_if_killable_by_x_aa", "Dont use W if killable by x AA", 2, 0, 4);
                }
                combo::use_e = combo->add_checkbox(myhero->get_model() + ".combo.e", "Use E on Killable", true);
                auto e_config = combo->add_tab(myhero->get_model() + "combo.e.config", "E Config");
                {
                    combo::e_if_target_leaving_range = e_config->add_checkbox(myhero->get_model() + ".combo.e.target_leaving_range", "Use E if target leaving E range", true);
                    combo::e_leaving_range_minimum_stacks = e_config->add_slider(myhero->get_model() + ".combo.e.leaving_range_minimum_stacks", "^ Minimum stacks", 6, 1, 6);

                    auto before_death_config = e_config->add_tab(myhero->get_model() + "combo.e.before_death.config", "Use E before death");
                    {
                        combo::e_use_before_death = before_death_config->add_checkbox(myhero->get_model() + ".combo.e.use_before_death", "Use E before death", true);
                        combo::e_before_death_use_on_x_stacks = before_death_config->add_slider(myhero->get_model() + ".combo.e.before_death_use_on_x_stacks", "Use E on x stacks", 6, 1, 6);
                        combo::e_before_death_myhero_under_hp = before_death_config->add_slider(myhero->get_model() + ".combo.e.before_death_myhero_under_hp", "Myhero HP is under (in %)", 10, 0, 100);
                        combo::e_before_death_calculate_incoming_damage = before_death_config->add_checkbox(myhero->get_model() + ".combo.e.before_death_calculate_incoming_damage", "Calculate incoming damage", true);
                        combo::e_before_death_damage_time = before_death_config->add_slider(myhero->get_model() + ".combo.e.before_death_damage_time", "Incoming damage time (in ms)", 600, 0, 1000);
                        combo::e_before_death_over_my_hp_in_percent = before_death_config->add_slider(myhero->get_model() + ".combo.w.before_death_over_my_hp_in_percent", "Incoming damage is over my HP (in %)", 90, 0, 100);
                    }
                }
                combo::use_r = combo->add_checkbox(myhero->get_model() + ".combo.r", "Use R", true);
                auto r_config = combo->add_tab(myhero->get_model() + "combo.r.config", "R Config");
                {
                    combo::r_use_if_enemies_more_than = r_config->add_slider(myhero->get_model() + ".combo.r.use_if_enemies_more_than", "Use if enemies more than", 3, 0, 5);
                }
            }

            auto harass = main_tab->add_tab(myhero->get_model() + ".harass", "Harass Settings");
            {
                harass::use_q = harass->add_checkbox(myhero->get_model() + ".harass.q", "Use Q", false);
                harass::use_w = harass->add_checkbox(myhero->get_model() + ".harass.w", "Use W", true);
                harass::use_e = harass->add_checkbox(myhero->get_model() + ".harass.e", "Use E", true);
                auto e_config = harass->add_tab(myhero->get_model() + ".harass.e.config", "E Config");
                {
                    harass::e_use_on_x_stacks = e_config->add_slider(myhero->get_model() + ".harass.e.use_on_x_stacks", "Use on x stacks", 6, 1, 6);
                }
            }

            auto laneclear = main_tab->add_tab(myhero->get_model() + ".laneclear", "Lane Clear Settings");
            {
                laneclear::spell_farm = laneclear->add_hotkey(myhero->get_model() + ".laneclear.enabled", "Toggle Spell Farm", TreeHotkeyMode::Toggle, 0x04, true);
                laneclear::use_q = laneclear->add_checkbox(myhero->get_model() + ".laneclear.q", "Use Q", false);
                laneclear::use_q_on_turret = laneclear->add_checkbox(myhero->get_model() + ".laneclear.q.on_turret", "Use Q On Turret", false);
                laneclear::use_w = laneclear->add_checkbox(myhero->get_model() + ".laneclear.w", "Use W", true);
                auto w_config = laneclear->add_tab(myhero->get_model() + ".laneclear.w.config", "W Config");
                {
                    laneclear::w_minimum_minions = w_config->add_slider(myhero->get_model() + ".laneclear.w.minimum_minions", "Minimum minions", 3, 0, 5);
                }
                laneclear::use_e = laneclear->add_checkbox(myhero->get_model() + ".laneclear.e", "Use E", true);
                auto e_config = laneclear->add_tab(myhero->get_model() + ".laneclear.e.config", "E Config");
                {
                    laneclear::e_use_if_killable_minions = e_config->add_slider(myhero->get_model() + ".laneclear.e.use_if_killable_minions", "Use only when killable minions more than", 3, 0, 5);
                }
            }

            auto jungleclear = main_tab->add_tab(myhero->get_model() + ".jungleclear", "Jungle Clear Settings");
            {
                jungleclear::use_q = jungleclear->add_checkbox(myhero->get_model() + ".jungleclear.q", "Use Q", true);
                jungleclear::use_w = jungleclear->add_checkbox(myhero->get_model() + ".jungleclear.w", "Use W", true);
                jungleclear::use_e = jungleclear->add_checkbox(myhero->get_model() + ".jungleclear.e", "Use E", true);
            }

            auto fleemode = main_tab->add_tab(myhero->get_model() + ".flee", "Flee Mode");
            {
                fleemode::use_q = fleemode->add_checkbox(myhero->get_model() + ".flee.q", "I WAS HIDING HAHAHAHA", true);
                fleemode::use_w = fleemode->add_checkbox(myhero->get_model() + ".flee.w", "Use W to slow enemies", true);
            }

            auto antigapclose = main_tab->add_tab(myhero->get_model() + ".antigapclose", "Anti Gapclose");
            {
                antigapclose::use_w = antigapclose->add_checkbox(myhero->get_model() + ".antigapclose.w", "Use W", true);
            }

            auto misc = main_tab->add_tab(myhero->get_model() + ".misc", "Miscellaneous Settings");
            {
                misc::stealth_recall = misc->add_checkbox(myhero->get_model() + ".misc.stealth_recall", "Stealth Recall", true);
                misc::stealth_recall_key = misc->add_hotkey(myhero->get_model() + ".misc.stealth_recall.key", "Stealth Recall Key", TreeHotkeyMode::Hold, 'S', true);
            }

            auto hitchance = main_tab->add_tab(myhero->get_model() + ".hitchance", "Hitchance Settings");
            {
                hitchance::w_hitchance = hitchance->add_combobox(myhero->get_model() + ".hitchance.w", "Hitchance W", { {"Low",nullptr},{"Medium",nullptr },{"High", nullptr},{"Very High",nullptr} }, 1);
            }

            auto draw_settings = main_tab->add_tab(myhero->get_model() + ".draw", "Drawings Settings");
            {

                draw_settings::draw_range_q = draw_settings->add_checkbox(myhero->get_model() + ".draw.q", "Draw Q range", true);
                draw_settings::draw_range_w = draw_settings->add_checkbox(myhero->get_model() + ".draw.w", "Draw W range", true);
                draw_settings::draw_range_e = draw_settings->add_checkbox(myhero->get_model() + ".draw.e", "Draw E range", true);
                draw_settings::draw_range_r = draw_settings->add_checkbox(myhero->get_model() + ".draw.r", "Draw R range", true);

                draw_settings->add_separator(myhero->get_model() + ".draw.separator1", "");
                draw_settings::draw_e_stacks = draw_settings->add_checkbox(myhero->get_model() + ".draw.e.stacks", "Draw Passive Stacks", true);
                draw_settings::draw_e_stacks->set_texture(myhero->get_passive_icon_texture());
                draw_settings::draw_damage_e = draw_settings->add_checkbox(myhero->get_model() + "draw.e.damage", "Draw E Damage", true);

                draw_settings->add_separator(myhero->get_model() + ".draw.separator2", "");
                draw_settings::draw_e_stacks_time = draw_settings->add_checkbox(myhero->get_model() + ".draw.e.stacks.time", "Draw Passive Stacks Time Left", true);
                draw_settings::draw_e_stacks_time->set_texture(myhero->get_passive_icon_texture());

                draw_settings::draw_q_timeleft = draw_settings->add_checkbox(myhero->get_model() + "draw.q.time", "Draw Q Time Left", true);

                draw_settings::draw_r_timeleft = draw_settings->add_checkbox(myhero->get_model() + "draw.r.time", "Draw R Time Left", true);

                draw_settings->add_separator(myhero->get_model() + ".draw.separator3", "");
                float color1[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                draw_settings::e_stacks_color = draw_settings->add_colorpick(myhero->get_model() + ".draw.e.stacks.color", "Passive Stacks Color", color1);
                draw_settings::e_stacks_time_color = draw_settings->add_colorpick(myhero->get_model() + ".draw.e.stacks.time.color", "Passive Stacks Time Left Color", color1);
                draw_settings::q_timeleft_color = draw_settings->add_colorpick(myhero->get_model() + ".draw.q.time.color", "Q Time Left Color", color1);
                draw_settings::r_timeleft_color = draw_settings->add_colorpick(myhero->get_model() + ".draw.r.time.color", "R Time Left Color", color1);
            }
        }

 
        {
            Permashow::Instance.Init(main_tab);
            Permashow::Instance.AddElement("Spell Farm", laneclear::spell_farm);
            Permashow::Instance.AddElement("Stealth Recall", misc::stealth_recall_key);
        }


        antigapcloser::add_event_handler(on_gapcloser);

  
        event_handler<events::on_update>::add_callback(on_update);
        event_handler<events::on_draw>::add_callback(on_draw);
        event_handler<events::on_after_attack_orbwalker>::add_callback(on_after_attack_orbwalker);
    }

    void unload()
    {

        plugin_sdk->remove_spell(q);
        plugin_sdk->remove_spell(w);
        plugin_sdk->remove_spell(e);
        plugin_sdk->remove_spell(r);

     
        menu->delete_tab(main_tab);


        Permashow::Instance.Destroy();

 
        antigapcloser::remove_event_handler(on_gapcloser);

        event_handler<events::on_update>::remove_handler(on_update);
        event_handler<events::on_draw>::remove_handler(on_draw);
        event_handler<events::on_after_attack_orbwalker>::remove_handler(on_after_attack_orbwalker);
    }

    void on_update()
    {
        if (myhero->is_dead())
        {
            return;
        }
        if (orbwalker->can_move(0.05f))
        {
            if (q->is_ready() && b->is_ready() && misc::stealth_recall->get_bool() && misc::stealth_recall_key->get_bool())
            {
                if (q->cast())
                {
                    scheduler->delay_action(0.25f + ping->get_ping() / 1000, [] { b->cast(); });
                    return;
                }
            }

            if (e->is_ready())
            {
                e_logic();
            }

            if (orbwalker->combo_mode())
            {
                if (w->is_ready() && combo::use_w->get_bool())
                {
                    w_logic();
                }

                if (r->is_ready() && combo::use_r->get_bool())
                {
                    r_logic();
                }
            }

            if (orbwalker->harass())
            {
                if (!myhero->is_under_enemy_turret())
                {
                    if (w->is_ready() && harass::use_w->get_bool())
                    {
                        w_logic();
                    }
                }
            }

            if (orbwalker->flee_mode())
            {
                if (q->is_ready() && fleemode::use_q->get_bool())
                {
                    if (q->cast())
                    {
                        return;
                    }
                }
                if (w->is_ready() && fleemode::use_w->get_bool())
                {
                    w_logic();
                }
            }

            if (orbwalker->lane_clear_mode() && laneclear::spell_farm->get_bool())
            {
                auto lane_minions = entitylist->get_enemy_minions();

                auto monsters = entitylist->get_jugnle_mobs_minions();

                lane_minions.erase(std::remove_if(lane_minions.begin(), lane_minions.end(), [](game_object_script x)
                    {
                        return !x->is_valid_target(e->range());
                    }), lane_minions.end());

                monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](game_object_script x)
                    {
                        return !x->is_valid_target(e->range());
                    }), monsters.end());

                std::sort(lane_minions.begin(), lane_minions.end(), [](game_object_script a, game_object_script b)
                    {
                        return a->get_position().distance(myhero->get_position()) < b->get_position().distance(myhero->get_position());
                    });

                std::sort(monsters.begin(), monsters.end(), [](game_object_script a, game_object_script b)
                    {
                        return a->get_max_health() > b->get_max_health();
                    });

                if (!lane_minions.empty())
                {
                    if (e->is_ready() && laneclear::use_e->get_bool())
                    {
                        int killable_minions = 0;

                        for (auto& minion : lane_minions)
                        {
                            if (e->get_damage(minion) > minion->get_health())
                            {
                                killable_minions++;
                            }
                        }

                        if (killable_minions >= laneclear::e_use_if_killable_minions->get_int())
                        {
                            if (e->cast())
                            {
                                return;
                            }
                        }
                    }
                    if (w->is_ready() && laneclear::use_w->get_bool())
                    {
                        if (w->cast_on_best_farm_position(laneclear::w_minimum_minions->get_int()))
                        {
                            return;
                        }
                    }
                }

                if (!monsters.empty())
                {
                    if (e->is_ready() && jungleclear::use_e->get_bool())
                    {
                        if (e->get_damage(monsters.front()) > monsters.front()->get_health())
                        {
                            if (e->cast())
                            {
                                return;
                            }
                        }
                    }
                    if (w->is_ready() && jungleclear::use_w->get_bool())
                    {
                        if (w->cast_on_best_farm_position(1, true))
                        {
                            return;
                        }
                    }
                }
            }
        }
    }

#pragma region w_logic
    void w_logic()
    {
        auto target = target_selector->get_target(w->range(), damage_type::physical);

        if (target != nullptr)
        {
            auto w_mode = combo::w_mode->get_int();
            if ((w_mode == 0 && myhero->get_distance(target) > myhero->get_attack_range()) || w_mode == 1)
            {
                if  (!myhero->has_buff(buff_hash("TwitchFullAutomatic")))
                {
                    int value = combo::w_dont_use_if_killable_by_x_aa->get_int();
                    if (value == 0 || myhero->get_auto_attack_damage(target) * value < target->get_real_health())
                    {
                        w->cast(target, get_hitchance(hitchance::w_hitchance));
                    }
                }
            }
        }
    }
#pragma endregion

#pragma region e_logic
    void e_logic()
    {
        auto enemies = entitylist->get_enemy_heroes();

        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](game_object_script x)
            {
                return !x->is_valid();
            }), enemies.end());

        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](game_object_script x)
            {
                return x->is_dead();
            }), enemies.end());

        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](game_object_script x)
            {
                return !x->is_valid_target(e->range());
            }), enemies.end());

        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](game_object_script x)
            {
                return x->has_buff({ buff_hash("UndyingRage"), buff_hash("ChronoShift"), buff_hash("KayleR"), buff_hash("KindredRNoDeathBuff") });
            }), enemies.end());

        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](game_object_script x)
            {
                return x->is_zombie();
            }), enemies.end());

        if ((orbwalker->harass() || orbwalker->lane_clear_mode()) && harass::use_e->get_bool())
        {
            for (auto& enemy : enemies)
            {
                if (get_twitch_e_stacks(enemy) >= harass::e_use_on_x_stacks->get_int())
                {
                    if (e->cast())
                    {
                        return;
                    }
                }
            }
        }

        if (combo::use_e->get_bool())
        {
            for (auto& enemy : enemies)
            {
                if (e->get_damage(enemy) > enemy->get_real_health())
                {
                    e->cast();
                }
                else if (combo::e_use_before_death->get_bool()
                    && (myhero->get_health_percent() <= combo::e_before_death_myhero_under_hp->get_int()
                        || (combo::e_before_death_calculate_incoming_damage->get_bool() && (health_prediction->get_incoming_damage(myhero, combo::e_before_death_damage_time->get_int() / 1000.f, true) * 100.f) /
                            myhero->get_max_health() > myhero->get_health_percent() * (combo::e_before_death_over_my_hp_in_percent->get_int() / 100.f))) && get_twitch_e_stacks(enemy) >= combo::e_before_death_use_on_x_stacks->get_int())
                {
                    e->cast();
                }
                else if (combo::e_if_target_leaving_range->get_bool() && get_twitch_e_stacks(enemy) >= combo::e_leaving_range_minimum_stacks->get_int() && myhero->count_enemies_in_range(e->range() - 50) == 0)
                {
                    e->cast();
                }
            }
        }
    }
#pragma endregion

#pragma region r_logic
    void r_logic()
    {
        auto enemies = myhero->count_enemies_in_range(r->range());

        if (enemies >= combo::r_use_if_enemies_more_than->get_int())
        {
            r->cast();
        }
    }
#pragma endregion

#pragma region get_hitchance
    hit_chance get_hitchance(TreeEntry* entry)
    {
        switch (entry->get_int())
        {
        case 0:
            return hit_chance::low;
        case 1:
            return hit_chance::medium;
        case 2:
            return hit_chance::high;
        case 3:
            return hit_chance::very_high;
        }
        return hit_chance::medium;
    }
#pragma endregion

    inline void draw_dmg_rl(game_object_script target, float damage, unsigned long color)
    {
        if (target != nullptr && target->is_valid() && target->is_hpbar_recently_rendered())
        {
            auto bar_pos = target->get_hpbar_pos();

            if (bar_pos.is_valid() && !target->is_dead() && target->is_visible())
            {
                const auto health = target->get_health();

                bar_pos = vector(bar_pos.x + (105 * (health / target->get_max_health())), bar_pos.y -= 10);

                auto damage_size = (105 * (damage / target->get_max_health()));

                if (damage >= health)
                {
                    damage_size = (105 * (health / target->get_max_health()));
                }

                if (damage_size > 105)
                {
                    damage_size = 105;
                }

                const auto size = vector(bar_pos.x + (damage_size * -1), bar_pos.y + 11);

                draw_manager->add_filled_rect(bar_pos, size, color);
            }
        }
    }

    unsigned long RainbowGradient1(int alpha, float speed, float angle, float timeOffset)
    {
        auto r = (int)floor(sin((angle + timeOffset) / 180 * M_PI * speed) * 127 + 128);
        auto g = (int)floor(sin((angle + 120 + timeOffset) / 180 * M_PI * speed) * 127 + 128);
        auto b = (int)floor(sin((angle + 240 + timeOffset) / 180 * M_PI * speed) * 127 + 128);
        return MAKE_COLOR(r, g, b, alpha);
    }

    void drawCircle(vector pos, int radius, int quality, int thickness = 2)
    {
        auto tickCount = gametime->get_prec_time();
        float timeOffset = tickCount * 1000;

        const auto points = geometry::geometry::circle_points(pos, radius, quality);
        float angleStep = 360.0f / quality;

        for (int i = 0; i < points.size(); i++)
        {
            const int next_index = (i + 1) % points.size();
            const auto start = points[i];
            const auto end = points[next_index];

            vector screenPosStart;
            renderer->world_to_screen(start, screenPosStart);
            vector screenPosEnd;
            renderer->world_to_screen(end, screenPosEnd);
            if (!renderer->is_on_screen(screenPosStart, 50) && !renderer->is_on_screen(screenPosEnd, 50))
                continue;


            int alpha = 255;
            float speed = 1;
            float angle = (i * angleStep) + timeOffset / 5;
            unsigned long color = RainbowGradient1(alpha, speed, angle, 0);

            draw_manager->add_line(points[i].set_z(pos.z), points[next_index].set_z(pos.z), color, thickness);
        }
    }

    void on_draw()
    {
        if (myhero->is_dead())
        {
            return;
        }

        if (q->is_ready() && draw_settings::draw_range_q->get_bool())
            drawCircle(myhero->get_position(), q->range(), 1000);

        if (w->is_ready() && draw_settings::draw_range_w->get_bool())
            drawCircle(myhero->get_position(), w->range(), 1000);
        if (e->is_ready() && draw_settings::draw_range_e->get_bool())
            drawCircle(myhero->get_position(), e->range(), 1000);
        if (r->is_ready() && draw_settings::draw_range_r->get_bool())
            drawCircle(myhero->get_position(), r->range(), 1000);
        if (e->is_ready() && draw_settings::draw_damage_e->get_bool())
        {
            for (auto& enemy : entitylist->get_enemy_heroes())
            {
                if (enemy->is_valid() && !enemy->is_dead() && enemy->is_hpbar_recently_rendered())
                {
                    draw_dmg_rl(enemy, e->get_damage(enemy), 0x8000ff00);
                }
            }
        }

        if (draw_settings::draw_q_timeleft->get_bool())
        {
            auto q_buff = myhero->get_buff(buff_hash("TwitchHideInShadows"));

            if (q_buff != nullptr && q_buff->is_valid() && q_buff->is_alive())
            {
                auto pos = myhero->get_position() + vector(100, 100);
                renderer->world_to_screen(pos, pos);
                draw_manager->add_text_on_screen(pos, draw_settings::q_timeleft_color->get_color(), 18, "Q Time: [%.1fs]", q_buff->get_remaining_time());
            }
        }

        if (draw_settings::draw_r_timeleft->get_bool())
        {
            auto r_buff = myhero->get_buff(buff_hash("TwitchFullAutomatic"));

            if (r_buff != nullptr && r_buff->is_valid() && r_buff->is_alive())
            {
                auto pos = myhero->get_position() + vector(100, 70);
                renderer->world_to_screen(pos, pos);
                draw_manager->add_text_on_screen(pos, draw_settings::r_timeleft_color->get_color(), 18, "R Time: [%.1fs]", r_buff->get_remaining_time());
            }
        }

        if (draw_settings::draw_e_stacks->get_bool())
        {
            for (auto& enemy : entitylist->get_enemy_heroes())
            {
                if (enemy->is_valid() && !enemy->is_dead() && enemy->is_hpbar_recently_rendered())
                {
                    int stacks = get_twitch_e_stacks(enemy);

                    if (stacks != 0)
                    {
                        auto pos = enemy->get_position() + vector(-60, 20);
                        renderer->world_to_screen(pos, pos);

                        if (draw_settings::draw_e_stacks_time->get_bool())
                        {
                            draw_manager->add_text_on_screen(pos, draw_settings::e_stacks_color->get_color(), 18, "Stacks: %d [%.1fs]", stacks, enemy->get_buff_time_left(buff_hash("TwitchDeadlyVenom")));
                        }
                        else
                        {
                            draw_manager->add_text_on_screen(pos, draw_settings::e_stacks_color->get_color(), 18, "Stacks: %d", stacks);
                        }
                    }
                }
            }
        }
    }

    void on_after_attack_orbwalker(game_object_script target)
    {
        if (q->is_ready() && target->is_ai_hero() && ((orbwalker->combo_mode() && combo::use_q->get_bool()) || (orbwalker->harass() && harass::use_q->get_bool())))
        {
            if (q->cast())
            {
                return;
            }
        }

        if (w->is_ready() && combo::w_mode->get_int() != 1)
        {
            if (target->is_ai_hero() && ((orbwalker->combo_mode() && combo::use_w->get_bool()) || (orbwalker->harass() && harass::use_w->get_bool())))
            {
                if ((!combo::w_dont_use_on_q->get_bool() || !myhero->has_buff(buff_hash("TwitchHideInShadows"))) && (!combo::w_dont_use_on_r->get_bool() || !myhero->has_buff(buff_hash("TwitchFullAutomatic"))))
                {
                    int value = combo::w_dont_use_if_killable_by_x_aa->get_int();
                    if (value == 0 || myhero->get_auto_attack_damage(target) * value < target->get_real_health())
                    {
                        if (w->cast(target, get_hitchance(hitchance::w_hitchance)))
                        {
                            return;
                        }
                    }
                }
            }
        }

        if (q->is_ready() && target->is_minion() && (orbwalker->lane_clear_mode() && laneclear::spell_farm->get_bool() && laneclear::use_q->get_bool())) {
            if (q->cast())
            {
                return;
            }
        }

        if (q->is_ready() && target->is_monster() && (orbwalker->lane_clear_mode() && laneclear::spell_farm->get_bool() && jungleclear::use_q->get_bool())) {
            if (q->cast())
            {
                return;
            }
        }

        if (q->is_ready() && orbwalker->lane_clear_mode() && myhero->is_under_enemy_turret() && laneclear::use_q_on_turret->get_bool() && target->is_ai_turret())
        {
            if (q->cast())
            {
                return;
            }
        }
    }

    void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args)
    {
        if (antigapclose::use_w->get_bool() && w->is_ready())
        {
            if (sender->is_valid_target(w->range() + sender->get_bounding_radius()))
            {
                w->cast(sender, get_hitchance(hitchance::w_hitchance));
            }
        }
    }

    int get_twitch_e_stacks(game_object_script target)
    {
        if (target->is_valid())
        {
            auto buff = target->get_buff(buff_hash("TwitchDeadlyVenom"));
            if (buff != nullptr && buff->is_valid() && buff->is_alive())
            {
                return buff->get_count();
            }
        }
        return 0;
    }
};


