//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2025 Cyril Hrubis <metan@ucw.cz>

 */

/**
 * @brief Default key bindings.
 * @file neko_keybindings.h
 */

#ifndef NEKO_KEYBINDINGS_H
#define NEKO_KEYBINDINGS_H

#include <stdint.h>
#include <input/gp_event_keys.h>

/** @brief Keybinding */
struct neko_keybinding {
	/**
	 * @brief A key name.
	 *
	 * This is used when keys are loaded from config.
	 */
	const char *name;
	/**
	 * @brief A key value.
	 */
	uint32_t key;
};

/** @brief Array to store the keybindings to. */
extern struct neko_keybinding neko_keybindings[];

/** @brief Indexes into neko_keybindings array. */
enum neko_keybindings_idx {
	NEKO_KEYS_MOD_WM_IDX,
	NEKO_KEYS_QUIT_IDX,
	NEKO_KEYS_EXIT_IDX,
	NEKO_KEYS_FORCE_IDX,
	NEKO_KEYS_LIST_APPS_IDX,
	NEKO_KEYS_APP_LAUNCHER_IDX,
	NEKO_KEYS_SWITCH_FOCUS_IDX,
	NEKO_KEYS_VIRT_SCREENS_LEFT_IDX,
	NEKO_KEYS_VIRT_SCREENS_RIGHT_IDX,
	NEKO_KEYS_ROTATE_IDX,
	NEKO_KEYS_HELP_IDX,
};

/* Default keybindings. */
#define NEKO_KEYS_MOD_WM_DEF {"WM_Mod", GP_KEY_LEFT_META}
#define	NEKO_KEYS_QUIT_DEF {"App_Quit", GP_KEY_Q}
#define NEKO_KEYS_EXIT_DEF {"WM_Exit", GP_KEY_X}
#define NEKO_KEYS_FORCE_DEF {"WM_Force", GP_KEY_F}
#define NEKO_KEYS_LIST_APPS_DEF {"List_Apps", GP_KEY_L}
#define NEKO_KEYS_APP_LAUNCHER_DEF {"App_Launcher", GP_KEY_A}
#define NEKO_KEYS_SWITCH_FOCUS_DEF {"Switch_Focus", GP_KEY_TAB}
#define NEKO_KEYS_VIRT_SCREENS_LEFT_DEF {"View_Left", GP_KEY_LEFT}
#define NEKO_KEYS_VIRT_SCREENS_RIGHT_DEF {"View_Right", GP_KEY_RIGHT}
#define NEKO_KEYS_ROTATE_DEF {"Rotate_Screen", GP_KEY_R}
#define NEKO_KEYS_HELP_DEF {"Help", GP_KEY_H}

/** @brief Modifier key needed to be pressed to send keypresses to the WM. */
#define NEKO_KEYS_MOD_WM neko_keybindings[NEKO_KEYS_MOD_WM_IDX].key

/** @brief Quits the current app. */
#define NEKO_KEYS_QUIT neko_keybindings[NEKO_KEYS_QUIT_IDX].key

/** @brief Exits the WM. */
#define NEKO_KEYS_EXIT neko_keybindings[NEKO_KEYS_EXIT_IDX].key

/** @brief Key to force neko exit or poweroff */
#define NEKO_KEYS_FORCE neko_keybindings[NEKO_KEYS_FORCE_IDX].key

/** @brief Go to the list of running applications. */
#define NEKO_KEYS_LIST_APPS neko_keybindings[NEKO_KEYS_LIST_APPS_IDX].key

/** @brief Show application launcher. */
#define NEKO_KEYS_APP_LAUNCHER neko_keybindings[NEKO_KEYS_APP_LAUNCHER_IDX].key

/** @brief Switch focus between views in split screen. */
#define NEKO_KEYS_SWITCH_FOCUS neko_keybindings[NEKO_KEYS_SWITCH_FOCUS_IDX].key

/** @brief Moves virtual screens left. */
#define NEKO_KEYS_VIRT_SCREENS_LEFT neko_keybindings[NEKO_KEYS_VIRT_SCREENS_LEFT_IDX].key

/** @brief Moves virtual screens right. */
#define NEKO_KEYS_VIRT_SCREENS_RIGHT neko_keybindings[NEKO_KEYS_VIRT_SCREENS_RIGHT_IDX].key

/** @brief Rotate screen left */
#define NEKO_KEYS_ROTATE neko_keybindings[NEKO_KEYS_ROTATE_IDX].key

/** @brief Displays a help. */
#define NEKO_KEYS_HELP neko_keybindings[NEKO_KEYS_HELP_IDX].key

/**
 * @brief Loads keybindings from a file.
 */
void neko_load_keybindings(void);

#endif /* NEKO_KEYBINDINGS_H */
