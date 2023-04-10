#include "ezreal.h"
#include "../plugin_sdk/cpplinq.h"
#include <regex>
#include "utils.hpp"
#include "permashow.hpp"

namespace ezreal
{

	TreeTab* main_tab = nullptr;

	namespace spells
	{
		constexpr float EZREAL_Q_RANGE = 1150.f; // 1200.f
		constexpr float EZREAL_W_RANGE = 1150.f;
		constexpr float EZREAL_E_RANGE = 450.f;
		constexpr float EZREAL_R_RANGE = FLT_MAX;

		script_spell* q = nullptr;
		script_spell* w = nullptr;
		script_spell* e = nullptr;
		script_spell* r = nullptr;


	}

	namespace settings
	{
		TreeEntry* farm_key = nullptr;

		namespace combo
		{
			TreeEntry* use_q = nullptr;
			TreeEntry* q_slider = nullptr;
			TreeEntry* q_check_block = nullptr;
			TreeEntry* auto_q = nullptr;
			TreeEntry* auto_q_turret = nullptr;
			TreeEntry* use_w = nullptr;
			TreeEntry* w_slider = nullptr;
			TreeEntry* use_r = nullptr;
			TreeEntry* semi_r = nullptr;
			TreeEntry* semir_mode = nullptr;
			TreeEntry* use_r_hard = nullptr;
			TreeEntry* min_r = nullptr;
			TreeEntry* max_r = nullptr;
		}

		namespace harass
		{
			TreeEntry* use_q = nullptr;
			TreeEntry* use_w = nullptr;
		}

		namespace laneclear
		{
			TreeEntry* use_q = nullptr;
			TreeEntry* minion_out_aa = nullptr;
		}

		namespace jungleclear
		{
			TreeEntry* use_q = nullptr;
			TreeEntry* use_w_epicmonster = nullptr;
		}

		namespace lasthit
		{
			TreeEntry* use_q = nullptr;
		}

		namespace clearobj
		{
			TreeEntry* w_in_objects = nullptr;
			TreeEntry* w_mode = nullptr;
		}

		namespace killsteal
		{
			TreeEntry* killsteal_enable = nullptr;
			TreeEntry* use_q = nullptr;
			TreeEntry* use_e_q = nullptr;
			TreeEntry* use_r = nullptr;
			TreeEntry* r_zonyas = nullptr;
		}

		namespace automatic
		{
			TreeEntry* automatic_enable = nullptr;
			TreeEntry* e_gapclose = nullptr;
			TreeEntry* use_r_hard = nullptr;
		}

		namespace draw
		{
			TreeEntry* draw_range_q = nullptr;
			TreeEntry* draw_range_q_color = nullptr;
			TreeEntry* draw_range_w = nullptr;
			TreeEntry* draw_range_w_color = nullptr;
			TreeEntry* thickness_line = nullptr;
		}
	}

	namespace variables
	{
		std::map<uint32_t, float> stasis_time;
	}

	namespace damage
	{
		float get_damage_on_unit(game_object_script target, std::vector<script_spell*> spells, bool ignore_non_ready, float extra_damage = 0.f, damage_type damage_type = damage_type::physical);

		bool can_kill_with_aa(game_object_script target, bool physical_shield = true, bool magical_shield = false);
	}

	float last_q_cast = 0;
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

	void focusW()
	{
		auto enemy_hxd = entitylist->get_enemy_heroes();

		enemy_hxd.erase(std::remove_if(enemy_hxd.begin(), enemy_hxd.end(), [](game_object_script x)
			{
				//console->print("reee");
				return !x->is_valid_target(settings::combo::q_slider->get_int()) || !x->has_buff(buff_hash("ezrealwattach"));
			}), enemy_hxd.end());
		if (enemy_hxd.empty())
		{
			//console->print("Array EMPTY");
		}
		else
		{
			//console->print("I FOUND EZREALS RING");
			for (const auto& enemy : enemy_hxd)
			{
				//console->print("ENEMY STOLE EZREALS RINGS");
				orbwalker->set_orbwalking_target(enemy);
			}


		}

	}

	void wantidash()
	{
		if (settings::automatic::e_gapclose && spells::q->is_ready())
			for (auto& enemy : entitylist->get_enemy_heroes())
			{
				if (enemy != nullptr && enemy->is_valid_target(300) && enemy->is_dashing())
				{


					vector from = myhero->get_position();
					vector to = enemy->get_position();

					float opposite_range = 450;
					vector opposite_direction = (from - to).normalized();
					vector new_position = from.extend(from + opposite_direction, opposite_range);

					if (new_position.is_under_enemy_turret() || new_position.is_wall())
					{
						//myhero->print_chat(0x01, "Rejected Either Wall or Enemy turret. "); // get end pos check if endpos is my pos if so do the thing 
						return;
					}
					//if (iswall(new_position))
					spells::e->cast(new_position);


					//myhero->print_chat(0x01, "Antigap W "); // get end pos check if endpos is my pos if so do the thing 
				}
			}


	}

	const std::regex auto_attack_regex("^Ezreal.+(Attack)", std::regex_constants::icase);

	uint16_t current_aa_target_id = 0U;

	void on_draw()
	{
		if (myhero->is_dead()) return;

		if (settings::draw::draw_range_q->get_bool())
		{
			drawCircle(myhero->get_position(), settings::combo::q_slider->get_int(), 1000);
		}

		if (settings::draw::draw_range_w->get_bool())
		{
			drawCircle(myhero->get_position(), settings::combo::w_slider->get_int(), 1000);
		}
	}

	void on_before_attack_orbwalker(game_object_script target, bool* process)
	{
		if (target->is_lane_minion()) {
			float time_to_land_attack = myhero->get_attack_cast_delay() + (ping->get_ping() / 2000) + myhero->get_attack_delay();
			float time_to_hit_aa = myhero->get_distance(target) / orbwalker->get_my_projectile_speed();

			float total_time_to_hit = time_to_land_attack + time_to_hit_aa;

			float damage = health_prediction->get_incoming_damage(target, total_time_to_hit, false) + myhero->get_auto_attack_damage(target);

			if (damage >= target->get_health()) current_aa_target_id = target->get_id();
		}

		if (orbwalker->lane_clear_mode() && settings::clearobj::w_in_objects->get_bool() && (target->is_ai_turret() || target->is_inhibitor() || target->is_nexus()))
		{
			if (settings::clearobj::w_mode->get_int() != 0) return;
			if ((target->is_ai_turret() && !myhero->is_under_enemy_turret()) || target->is_dead()) return;
			float t = (fmaxf(0.f, target->get_distance(myhero) - myhero->get_bounding_radius()) / 1700.f) + 0.25f;
			if (health_prediction->get_health_prediction(target, t, 0.25f) <= 10.f) return;

			if (myhero->get_distance(target) <= 900.f)
			{
				if (spells::w->cast(target)) return;
			}
		}
	}

	void on_after_attack_orbwalker(game_object_script target)
	{
		if (orbwalker->lane_clear_mode() && settings::clearobj::w_in_objects->get_bool() && (target->is_ai_turret() || target->is_inhibitor() || target->is_nexus()))
		{
			if (settings::clearobj::w_mode->get_int() != 1) return; // 1 = after AA
			if ((target->is_ai_turret() && !myhero->is_under_enemy_turret()) || target->is_dead()) return;
			float t = (fmaxf(0.f, target->get_distance(myhero) - myhero->get_bounding_radius()) / 1700.f) + 0.25f;
			if (health_prediction->get_health_prediction(target, t, 0.25f) <= 10.f) return;

			if (myhero->get_distance(target) <= 900.f)
			{
				if (spells::w->cast(target)) return;
			}
		}
	}

	float getPing()
	{
		return ping->get_ping() / 1000;
	}

	int last_q_minion_id = -1;


	void on_cast_spell(spellslot spellSlot, game_object_script target, vector& pos, vector& pos2, bool isCharge, bool* process)
	{
		last_q_cast = gametime->get_time() + 0.133 + getPing();

		if (spellSlot == spells::q->get_slot())
		{
			last_q_minion_id = target ? target->get_id() : -1;
			last_q_cast = gametime->get_time() + 0.133 + getPing();
		}
	}




	void on_process_spell_cast(game_object_script sender, spell_instance_script spell)
	{
		if (sender->get_handle() == myhero->get_handle())
		{
			last_q_cast = 0;
		}
	}

	void on_unkillable_minion(game_object_script minion)
	{
		if (minion->is_dead()) return;

		if (orbwalker->lane_clear_mode() && spells::q->is_ready() && settings::laneclear::use_q->get_bool() && !myhero->is_winding_up())
		{
			auto t = (fmaxf(0.f, minion->get_distance(myhero) - minion->get_bounding_radius()) / 2000.f) + 0.25f;
			auto minion_hp = health_prediction->get_health_prediction(minion, t - 0.1f);

			if (spells::q->get_damage(minion) > minion_hp && myhero->get_distance(minion) <= spells::q->range())
			{
				if (spells::q->get_prediction(minion).collision_objects.size() > 0) return;
				if (spells::q->cast(minion, hit_chance::high)) return;
			}
		}
	}



#pragma region _SPELL_LOGIC
	bool target_killed_by_previous_q(game_object_script& target)
	{
		if (last_q_cast == 0)
		{
			return false;
		}

		float time_since_last_q = gametime->get_time() - last_q_cast;
		float target_health_at_last_q = health_prediction->get_health_prediction(target, time_since_last_q);

		return target_health_at_last_q <= 0;
	}

	void q_logic()
	{
		if (!spells::q->is_ready()) return;

		auto target = target_selector->get_target(spells::q->range(), damage_type::physical);
		if (target == nullptr || !target->is_valid()) return;

		if (target_killed_by_previous_q(target))
		{
			return;
		}

		if (myhero->get_distance(target) <= static_cast<float>(settings::combo::q_slider->get_int()))
		{
			prediction_output output = spells::q->get_prediction(target);

			if (output._cast_position.is_valid() && output.collision_objects.size() <= 0)
			{
				if (output.hitchance >= hit_chance::high)
				{
					if (settings::combo::q_check_block->get_bool() && !target->is_winding_up())
					{
						if (spells::q->cast(output.get_cast_position()))
						{
							return;
						}
					}
				}
			}
		}
	}

	void w_logic()
	{
		if (!spells::w->is_ready()) return;
		auto target = target_selector->get_target(spells::w->range(), damage_type::physical);
		if (target == nullptr || !target->is_valid()) return;

		if (myhero->get_distance(target) <= static_cast<float>(settings::combo::w_slider->get_int()))
		{
			prediction_output output = spells::w->get_prediction(target);
			if (!output._cast_position.is_valid()) return;
			if (output.hitchance < hit_chance::very_high) return;

			if (spells::w->cast(output.get_cast_position())) return;
		}
	}

	float wDamage(game_object_script target)
	{
		float wAd = 80.f * myhero->get_spell(spellslot::w)->level() - (25.f * myhero->get_spell(spellslot::e)->level()) + 0.6f * (myhero->get_flat_physical_damage_mod() + myhero->get_base_attack_damage());
		float wAp = (5.f * myhero->get_spell(spellslot::w)->level() + 65.f) / 100.f * (myhero->get_base_ability_power() + myhero->get_flat_magic_damage_mod() * (1.f + myhero->get_percent_magic_damage_mod()));
		float rd = damagelib->calculate_damage_on_unit(myhero, target, damage_type::magical, wAd + wAp);
		return rd;
	}

	float rDamage(game_object_script target)
	{
		float Rdm = 200.f + (150.f * myhero->get_spell(spellslot::r)->level()) + (myhero->get_flat_physical_damage_mod() + myhero->get_base_attack_damage()) + (myhero->get_total_ability_power() * 0.9f);
		float rd = damagelib->calculate_damage_on_unit(myhero, target, damage_type::magical, Rdm);
		return rd;
	}


	void r_logic()
	{
		if (!spells::r->is_ready()) return;
		auto target = target_selector->get_target(static_cast<float>(settings::combo::max_r->get_int()), damage_type::magical);

		if (target == nullptr || !target->is_valid()) return;

		auto t = (fmaxf(0.f, target->get_distance(myhero) - target->get_bounding_radius()) / 2000.f) + 1.0f;
		if (rDamage(target) > health_prediction->get_health_prediction(target, t) && myhero->get_distance(target) <= settings::combo::max_r->get_int() && myhero->get_distance(target) > settings::combo::min_r->get_int())
		{
			if (settings::combo::use_r_hard->get_bool() && !target->is_immovable()) return;

			prediction_output output = spells::r->get_prediction_no_collision(target, true);
			{
				if (!output._cast_position.is_valid()) return;
				if (output.hitchance < hit_chance::very_high) return;

				if (spells::r->cast(output.get_cast_position())) return;
			}
		}
	}

	void semi_r()
	{
		if (!spells::r->is_ready()) return;
		auto target = target_selector->get_target(static_cast<float>(settings::combo::max_r->get_int()), damage_type::magical);
		if (target == nullptr || !target->is_valid()) return;

		if (myhero->get_distance(target) <= settings::combo::max_r->get_int())
		{
			vector gayshit = hud->get_hud_input_logic()->get_game_cursor_position();
			switch (settings::combo::semir_mode->get_int())
			{
			case 0:
				gayshit = hud->get_hud_input_logic()->get_game_cursor_position();
				break;
			case 1:
			{
				prediction_output output = spells::r->get_prediction_no_collision(target, true);
				if (!output._cast_position.is_valid()) return;
				if (output.hitchance < hit_chance::very_high) return;
				gayshit = output.get_cast_position();
				break;
			}
			default:
				break;
			}

			if (spells::r->cast(gayshit)) return;
		}
	}
#pragma endregion


#pragma region _SCRIPT_LOGIC
	void combo()
	{
		if (settings::combo::use_w->get_bool())
		{
			w_logic();
		}

		if (settings::combo::use_q->get_bool())
		{
			q_logic();
		}

		if (settings::combo::use_r->get_bool())
		{
			r_logic();
		}
	}

	void harass()
	{
		if (settings::harass::use_q->get_bool())
		{
			q_logic();
		}

		if (settings::harass::use_w->get_bool())
		{
			w_logic();
		}
	}

	bool minion_killed_by_previous_q(game_object_script minion)
	{
		if (last_q_minion_id == -1 || minion->get_id() != last_q_minion_id)
		{
			return false;
		}

		auto t = (fmaxf(0.f, minion->get_distance(myhero) - myhero->get_bounding_radius()) / 2000.f) + 0.25f;
		auto minion_hp = health_prediction->get_lane_clear_health_prediction(minion, t - 0.1f);
		return minion_hp <= 0;
	}

	void farm()
	{
		std::vector<game_object_script> minions = utils::get_nearby_objects(myhero->get_position(), 1100.f, entitylist->get_enemy_minions());

		for (auto& minion : minions)
		{
			bool otherMinionWillDie = false;

			if (minion->is_dead()) continue;

			if ((orbwalker->can_attack() && myhero->is_in_auto_attack_range(minion)) || minion->get_id() == current_aa_target_id) continue;

			if (orbwalker->last_hit_mode())
			{
				if (spells::q->is_ready() && settings::lasthit::use_q->get_bool() && !myhero->is_winding_up())
				{
					auto t = (fmaxf(0.f, minion->get_distance(myhero) - myhero->get_bounding_radius()) / 2000.f) + 0.25f;
					auto minion_hp = health_prediction->get_lane_clear_health_prediction(minion, t - 0.1f);
					if (minion_hp <= 0) continue;

					if (minion_hp - spells::q->get_damage(minion) <= 0)
					{
						if (spells::q->cast(minion, hit_chance::high)) break;
					}
				}
			}
			else if (orbwalker->lane_clear_mode()) {

				auto t = (fmaxf(0.f, minion->get_distance(myhero) - minion->get_bounding_radius()) / 2000.f) + 0.25f;

				auto minion_hp = health_prediction->get_lane_clear_health_prediction(minion, t - 0.1f);
				if (minion_hp <= 0) continue;

				float delay_since_last_auto_attack = gametime->get_time() - orbwalker->get_last_aa_time();
				float delay_to_next_auto_attack = myhero->get_attack_delay() - delay_since_last_auto_attack;
				delay_to_next_auto_attack = fmax(0.f, delay_to_next_auto_attack);
				float time_to_land_attack = myhero->get_attack_cast_delay() + (ping->get_ping() / 2000) + delay_to_next_auto_attack;

				if (spells::q->is_ready() && settings::laneclear::use_q->get_bool())
				{
					if (minion_killed_by_previous_q(minion)) continue;

					std::vector<game_object_script> minions_aa = utils::get_nearby_objects(myhero->get_position(), myhero->get_attack_range(), entitylist->get_enemy_minions());

					for (auto minion_aa : minions_aa) {

						if (minion_aa->get_network_id() == minion->get_network_id()) continue;

						float time_to_hit_aa = myhero->get_distance(minion_aa) / orbwalker->get_my_projectile_speed();

						float total_time_to_hit = t + time_to_land_attack + time_to_hit_aa;

						float damage = health_prediction->get_incoming_damage(minion_aa, total_time_to_hit, false);

						if (damage >= minion_aa->get_health()) {
							otherMinionWillDie = true;
							break;
						}
					}

					if (otherMinionWillDie) continue;

					if (settings::laneclear::minion_out_aa->get_bool() && !myhero->is_in_auto_attack_range(minion))
					{
						if (spells::q->get_prediction(minion).collision_objects.size() > 0) continue;
						if (minion_hp - spells::q->get_damage(minion) <= 0)
						{
							if (spells::q->cast(minion, hit_chance::high)) break;
						}
					}

					if (minion_hp - spells::q->get_damage(minion) <= 0)
					{
						if (spells::q->cast(minion, hit_chance::high)) break;
					}

					if (orbwalker->get_orb_state() == 34)
					{
						float time_to_hit_aa = myhero->get_distance(minion) / orbwalker->get_my_projectile_speed();

						float total_time_to_hit = t + time_to_land_attack + time_to_hit_aa;

						float damage = health_prediction->get_incoming_damage(minion, total_time_to_hit, false) + spells::q->get_damage(minion);

						if (damage >= minion->get_health()) continue;

						if (minion_hp - myhero->get_auto_attack_damage(minion) <= 0) continue;

						if (spells::q->cast(minion, hit_chance::high)) break;
					}
				}
			}
		}


		std::vector<game_object_script> monsters = utils::get_nearby_objects(myhero->get_position(), 600.f, entitylist->get_jugnle_mobs_minions());
		monsters = utils::sort::sort_by_closest_distance(myhero->get_position(), monsters);
		for (auto monster : monsters)
		{
			auto name = monster->get_base_skin_name();

			if (orbwalker->lane_clear_mode())
			{
				if (spells::q->is_ready() && settings::jungleclear::use_q->get_bool())
				{
					if (monster->get_distance(myhero) < spells::q->range())
					{
						if (spells::q->cast(monster)) return;
					}
				}

				if (name == "SRU_Krug11.1.1" || name == "SRU_Krug5.1.1") continue;
				if (spells::w->is_ready() && settings::jungleclear::use_w_epicmonster->get_bool() && monster->is_epic_monster())
				{
					if (monster->get_distance(myhero) < spells::w->range())
					{
						spells::w->cast(monster);
					}
				}
			}
		}
	}

	void killsteal()
	{
		auto enemies = utils::get_nearby_objects(myhero->get_position(), 1500.f, entitylist->get_enemy_heroes());

		for (const auto& enemy : enemies)
		{
			if (enemy == nullptr || enemy->is_dead() || enemy->is_zombie() || enemy->is_invulnerable()) continue;

			if (settings::killsteal::use_q->get_bool())
			{
				if (spells::q->get_damage(enemy) >= enemy->get_health())
				{
					if (myhero->get_distance(enemy) <= spells::q->range())
					{
						if (spells::q->cast(enemy, hit_chance::high)) return;
					}
				}
			}

			if (settings::killsteal::use_e_q->get_bool() && spells::q->is_ready())
			{
				if (spells::e->is_ready())
				{
					if (spells::q->get_damage(enemy) >= enemy->get_health())
					{
						if (myhero->get_distance(enemy) <= (900.f + spells::e->range()) && myhero->get_distance(enemy) > 1000.f)
						{
							spells::e->cast(enemy);
							scheduler->delay_action(0.1f, [enemy]() {
								if (spells::q->cast(enemy, hit_chance::high)) return;
								});
						}
					}
				}
			}

			if (settings::killsteal::use_r->get_bool())
			{
				if (rDamage(enemy) > enemy->get_health() && myhero->get_distance(enemy) <= 1300.f && myhero->get_distance(enemy) > 900.f)
				{
					prediction_output output = spells::r->get_prediction_no_collision(enemy, true);
					if (!output._cast_position.is_valid()) return;
					if (output.hitchance < hit_chance::very_high) return;

					if (spells::r->cast(output.get_cast_position())) return;
				}
			}
		}
	}

	void autoQ()
	{
		if (!spells::q->is_ready()) return;
		std::vector<game_object_script> enemies = entitylist->get_enemy_heroes();
		enemies = utils::sort::sort_by_lowest_health(enemies);

		for (game_object_script target : enemies)
		{
			if (target->is_dead() || target->get_distance(myhero) > spells::q->range()) continue;

			if (settings::combo::auto_q_turret->get_bool() && myhero->is_under_enemy_turret()) continue;

			if (variables::stasis_time[target->get_network_id()] == 0 && target->is_valid() && target->is_valid_target())
			{
				prediction_output output = spells::q->get_prediction(target);
				if (!output._cast_position.is_valid()) continue;
				if (output.hitchance < hit_chance::high) continue;

				if (spells::q->cast(output.get_cast_position())) return;
			}
			else
			{
				bool hit_other = false;
				vector targetPos = target->get_position();
				float delayToEnd = variables::stasis_time[target->get_network_id()] - gametime->get_time();
				float timeToHit = fmaxf(0.f, target->get_distance(myhero)) / 2000.f + 0.25f;
				float reaminingTime = delayToEnd - timeToHit;

				if (reaminingTime >= 0.f && reaminingTime <= (ping->get_ping() / 1000)) {
					std::vector<game_object_script> colisions = spells::q->get_collision(myhero->get_position(), { targetPos });
					if (!colisions.empty()) {
						for (auto& colision : colisions) {
							if (colision != target) {
								hit_other = true;
								break;
							}
						}
					}

					if (!hit_other) {
						if (spells::q->cast(targetPos)) return;
					}
				}
			}
		}
	}
#pragma endregion




	void on_update()
	{
		focusW();
		wantidash();

		if (myhero->is_dead()) return;

		if (settings::killsteal::killsteal_enable->get_bool())
		{
			killsteal();
		}

		if (settings::combo::auto_q->get_bool() && !myhero->is_recalling())
		{
			autoQ();
		}

		if (settings::combo::semi_r->get_bool())
		{
			vector cursos_pos = hud->get_hud_input_logic()->get_game_cursor_position();
			orbwalker->move_to(cursos_pos);
			semi_r();
		}

		if (orbwalker->combo_mode())
		{
			combo();
		}

		if (orbwalker->harass())
		{
			harass();
		}

		if (settings::farm_key->get_bool())
		{
			farm();
		}
	}

	void load()
	{
		spells::q = plugin_sdk->register_spell(spellslot::q, spells::EZREAL_Q_RANGE);
		spells::w = plugin_sdk->register_spell(spellslot::w, spells::EZREAL_W_RANGE);
		spells::e = plugin_sdk->register_spell(spellslot::e, spells::EZREAL_E_RANGE);
		spells::r = plugin_sdk->register_spell(spellslot::r, spells::EZREAL_R_RANGE);

		spells::q->set_skillshot(0.25f, 60.f, 2000.f, { collisionable_objects::minions, collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);
		spells::w->set_skillshot(0.25f, 80.f, 1600.f, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);
		spells::r->set_skillshot(1.0f, 160.f, 2000.f, {}, skillshot_type::skillshot_line);

#pragma region _MENU_SETUP

		main_tab = menu->create_tab("TrentAIO", "Trent Ezreal");
		main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
		{
			auto combo = main_tab->add_tab("combo.tab", "Combo Settings");
			{
				auto q_config = combo->add_tab("q.settings", "Q settings");
				q_config->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				{
					settings::combo::use_q = q_config->add_checkbox("use.q", "Use Q", true);
					settings::combo::q_slider = q_config->add_slider("q.slider", " ^~ Q Range", 1150, 100, 1150);
					settings::combo::q_check_block = q_config->add_checkbox("check.block", " ^~ Check if enemy AA block", true);
					settings::combo::auto_q = q_config->add_hotkey("autoq.key", " ^~ Auto Q", TreeHotkeyMode::Toggle, 0x53, true);
					settings::combo::auto_q_turret = q_config->add_checkbox("autoq.turret", " ^~ Disable Auto Q under turret", false);
				}

				auto w_config = combo->add_tab("w.settings", "W settings");
				w_config->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				{
					settings::combo::use_w = w_config->add_checkbox("use.w", "Use W", true);
					settings::combo::w_slider = w_config->add_slider("w.slider", " ^~ W Range", 1100, 100, 1150);
				}

				auto r_config = combo->add_tab("r.settings", "R Settings");
				r_config->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
				{
					settings::combo::use_r = r_config->add_checkbox("use.r", "Use R", false);
					settings::combo::semi_r = r_config->add_hotkey("semir.key", "Semi-R", TreeHotkeyMode::Hold, 0x54, false);
					settings::combo::semir_mode = r_config->add_combobox("semir.mode", " ^~ Mode", { {"Mouse Pos", nullptr}, {"Prediction", nullptr} }, 1);
					settings::combo::use_r_hard = r_config->add_checkbox("r.only.cc", " ^~ Only CC/Dash", true);
					settings::combo::min_r = r_config->add_slider("min.range.slider", " ^~ Min R Range", 200, 100, 2000);
					settings::combo::max_r = r_config->add_slider("max.range.slider", " ^~ Max R Range", 1000, 1000, 3000);
				}
			}

			auto harass = main_tab->add_tab("harass.tab", "Harass Settings");
			{
				auto q_harass = harass->add_tab("q.settings", "Q Settings");
				q_harass->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				{
					settings::harass::use_q = q_harass->add_checkbox("use.q", "Use Q in Harass", true);
				}

				auto w_harass = harass->add_tab("w.settings", "W Settings");
				w_harass->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				{
					settings::harass::use_w = w_harass->add_checkbox("use.w", "Use W in Harass", true);
				}
			}

			auto farming = main_tab->add_tab("farming.tab", "Farming settings");
			{
				settings::farm_key = farming->add_hotkey("farm.key", "Farm key", TreeHotkeyMode::Toggle, 0x04, true);

				auto laneclear = farming->add_tab("laneclear.tab", "Laneclear Settings");
				{
					auto q_laneclear = laneclear->add_tab("q.settings", "Q Settings");
					q_laneclear->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
					{
						settings::laneclear::use_q = q_laneclear->add_checkbox("q.use", "Use Q", true);
						settings::laneclear::minion_out_aa = q_laneclear->add_checkbox("out.aa", " ^~ If minion killable out AA", false);
					}
				}

				auto lasthit = farming->add_tab("lasthit.tab", "Lasthit Settings");
				{
					auto q_lasthit = lasthit->add_tab("q.settings", "Q Settings");
					q_lasthit->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
					{
						settings::lasthit::use_q = q_lasthit->add_checkbox("q.use", "Use Q", true);
					}
				}

				auto jungleclear = farming->add_tab("jungleclear.tab", "Jungleclear Settings");
				{
					auto q_jungleclear = jungleclear->add_tab("q.settings", "Q Settings");
					q_jungleclear->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
					{
						settings::jungleclear::use_q = q_jungleclear->add_checkbox("q.use", "Use Q", true);
					}

					auto w_jungleclear = jungleclear->add_tab("w.settings", "W Settings");
					w_jungleclear->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
					{
						settings::jungleclear::use_w_epicmonster = w_jungleclear->add_checkbox("use.w.epic", "Use W in Epic monsters", true);
					}
				}

				auto misc = farming->add_tab("misc.tab", "Misc Settings");
				{
					auto w_misc = misc->add_tab("w.settings", "W Settings");
					w_misc->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
					{
						settings::clearobj::w_in_objects = w_misc->add_checkbox("use.w.obj", "Use W in objects", true);
						settings::clearobj::w_mode = w_misc->add_combobox("w.mode", " ^~ Mode", { {"Before Attack (Orbwalker)", nullptr}, {"After Attack (Orbwalker)", nullptr} }, 1);
					}
				}
			}

			auto killsteal = main_tab->add_tab("killsteal.tab", "Killsteal Settings");
			{
				killsteal->add_separator("ks.ac", "Killsteal Settings");
				settings::killsteal::killsteal_enable = killsteal->add_checkbox("use.ks", "Enabled killsteal", true);

				auto q_killsteal = killsteal->add_tab("q.settings", "Q Settings");
				q_killsteal->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				{
					settings::killsteal::use_q = q_killsteal->add_checkbox("use.q", "Use Q", true);
				}

				auto e_killsteal = killsteal->add_tab("e.settings", "E Settings");
				e_killsteal->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
				{
					settings::killsteal::use_e_q = e_killsteal->add_checkbox("use.e.q", "Use E & Q", false);
				}

				auto r_killsteal = killsteal->add_tab("r.settings", "R Settings");
				r_killsteal->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
				{
					settings::killsteal::use_r = r_killsteal->add_checkbox("use.r", "Use R", false);
				}
			}

			auto draw_settings = main_tab->add_tab("drawings", "Drawings Settings");
			{
				auto q_draw = draw_settings->add_tab("drawings.settingsQ", "Q Settings");
				q_draw->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				{
					settings::draw::draw_range_q = q_draw->add_checkbox("drawingQ", "Draw Q range", true);
				}

				auto w_draw = draw_settings->add_tab("drawings.settingsW", "W Settings");
				w_draw->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				{
					settings::draw::draw_range_w = w_draw->add_checkbox("drawingW", "Draw W range", true);
				}
			}

			auto automatic_settings = main_tab->add_tab("automatic", "Automatic Settings");
			{
				auto e_automatic = automatic_settings->add_tab("e.settings", "E Settings");
				e_automatic->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
				{
					settings::automatic::e_gapclose = e_automatic->add_checkbox("use.e", "Use E on Gapclosers", true);
				}
			}
		}



		{
			Permashow::Instance.Init("Trent AIO");
			Permashow::Instance.AddElement("Auto Q", settings::combo::auto_q);
			Permashow::Instance.AddElement("R Manual", settings::combo::semi_r);
			Permashow::Instance.AddElement("Farm key", settings::farm_key);
		}

#pragma endregion

		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_process_spell_cast>::add_callback(on_process_spell_cast);
		event_handler<events::on_before_attack_orbwalker>::add_callback(on_before_attack_orbwalker);
		event_handler<events::on_after_attack_orbwalker>::add_callback(on_after_attack_orbwalker);
		event_handler<events::on_unkillable_minion>::add_callback(on_unkillable_minion);
		event_handler<events::on_cast_spell>::add_callback(on_cast_spell);

	}

	void unload()
	{
		plugin_sdk->remove_spell(spells::q);
		plugin_sdk->remove_spell(spells::e);
		plugin_sdk->remove_spell(spells::w);
		plugin_sdk->remove_spell(spells::r);

		menu->delete_tab(main_tab);

		Permashow::Instance.Destroy();


		event_handler<events::on_update>::remove_handler(on_update);
		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_process_spell_cast>::remove_handler(on_process_spell_cast);
		event_handler<events::on_before_attack_orbwalker>::remove_handler(on_before_attack_orbwalker);
		event_handler<events::on_after_attack_orbwalker>::remove_handler(on_after_attack_orbwalker);
		event_handler<events::on_unkillable_minion>::remove_handler(on_unkillable_minion);
		event_handler< events::on_cast_spell >::remove_handler(on_cast_spell);

	}
}