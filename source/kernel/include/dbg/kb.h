/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <common.h>

class Keyboard {
	Keyboard() = delete;

	struct KeymapEntry {
		char def;
		char shift;
		char alt;
	};

public:
	enum {
		EVENT_PRESS 	= 1,
		EVENT_RELEASE 	= 2,
	};

	enum {
		MOD_SHIFT		= 1,
		MOD_CTRL		= 2,
		MOD_ALT			= 4,
		MOD_BREAK		= 8,
	};

	struct Event {
		uint8_t keycode;
		char character;
		uint8_t flags;
	};

	/**
	 * @param flags will be changed to set/unset MOD_BREAK
	 * @return the keycode, if there is any, or VK_NOKEY
	 */
	static uint8_t getKeyCode(uint *flags);

	/**
	 * Fills the given keyevent. If <wait> is true, it waits until a scancode is present. Otherwise
	 * it just checks whether one is present. If no, it gives up.
	 *
	 * @param ev the event to fill (may be NULL if you just want to wait for a keypress/-release)
	 * @param events a mask of events to react on (KEV_*)
	 * @param wait whether to wait until a scancode is present
	 * @return true if a key could be read. false if not (always true, when <wait> is true)
	 */
	static bool get(Event *ev,uint8_t events,bool wait);

private:
	static bool translate(Event *ev,uint8_t scanCode);
	static uint8_t toggleFlag(bool isbreak,uint8_t val,uint8_t flag);

	static uint flags;
	static KeymapEntry keymap[];
};
