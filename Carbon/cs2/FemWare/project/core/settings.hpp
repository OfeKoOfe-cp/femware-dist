#pragma once

#include <utilities/math/math.hpp>
#include <external/config.hpp>

#include "settings/settings.combat.hpp"
#include "settings/settings.changer.hpp"
#include "settings/settings.esp.hpp"
#include "settings/settings.misc.hpp"
#include "settings/settings.movement.hpp"
#include "settings/settings.theme.hpp"
#include "settings/settings.world.hpp"

namespace settings {

	struct cheat
	{
		xui::setting unsafe_mode{ false, {}, "unsafe mode", "cheat" };

		theme m_theme{};
	};

	inline combat g_combat{};
	inline esp g_esp{};
	inline changer g_changer{};
	inline misc g_misc{};
	inline movement g_movement{};
	inline world g_world{};
	inline cheat g_cheat{};

	inline void finalize_binds( )
	{
		auto& aa = g_combat.m_antiaim;
		aa.manual_left.bind.excludes = &aa.manual_right;
		aa.manual_right.bind.excludes = &aa.manual_left;

		if ( aa.manual_left.value && aa.manual_right.value )
		{
			aa.manual_right.value = false;
			aa.manual_right.bind.active = false;
		}
	}

} // namespace settings
