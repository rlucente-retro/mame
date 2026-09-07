// license:BSD-3-Clause
// copyright-holders:R. Belmont
//============================================================
//
//  input_mac.cpp - Mac input
//
//  Mac OSD by R. Belmont
//
//============================================================

#include "input_module.h"
#include "modules/osdmodule.h"

#if defined(OSD_MAC)

// System headers
#include <cctype>
#include <cstddef>
#include <mutex>
#include <memory>
#include <algorithm>

// MAME headers
#include "emu.h"
#include "input.h"
#include "osdepend.h"
#include "ui/uimain.h"
#include "uiinput.h"
#include "window.h"
#include "strconv.h"

#include "../../mac/osdmac.h"
#include "input_common.h"

void mac_osd_interface::customize_input_type_list(std::vector<input_type_entry> &typelist)
{
}

void mac_osd_interface::release_keys()
{
}

bool mac_osd_interface::should_hide_mouse()
{
	if (machine().paused())
		return false;

	if (machine().ui().is_menu_active())
		return false;

	bool const mouse_enabled = options().mouse() || machine().input().class_enabled(DEVICE_CLASS_MOUSE);
	bool const lightgun_enabled = options().lightgun() || machine().input().class_enabled(DEVICE_CLASS_LIGHTGUN);
	if (!mouse_enabled && !lightgun_enabled)
		return false;

	return true;
}

void mac_osd_interface::process_events_buf()
{
}

#endif
