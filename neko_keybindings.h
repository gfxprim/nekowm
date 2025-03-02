//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

/**
 * @brief Default key bindings.
 * @file neko_keybindings.h
 */

#ifndef NEKO_KEYBINDINGS_H
#define NEKO_KEYBINDINGS_H

/** @brief Modifier key needed to be pressed to send keypresses to the WM. */
#define NEKO_KEYS_MOD_WM GP_KEY_LEFT_META

/** @brief Quits the current app. */
#define NEKO_KEYS_QUIT GP_KEY_Q

/** @brief Exits the WM. */
#define NEKO_KEYS_EXIT GP_KEY_X

/** @brief Key to force neko exit or poweroff */
#define NEKO_KEYS_FORCE GP_KEY_F

/** @brief Go to the list of running applications. */
#define NEKO_KEYS_LIST_APPS GP_KEY_R

/** @brief Show application launcher. */
#define NEKO_KEYS_APP_LAUNCHER GP_KEY_A

/** @brief Switch focus between views in split screen. */
#define NEKO_KEYS_SWITCH_FOCUS GP_KEY_TAB

/** @brief Moves virtual screens left. */
#define NEKO_KEYS_VIRT_SCREENS_LEFT GP_KEY_LEFT_BRACE

/** @brief Moves virtual screens right. */
#define NEKO_KEYS_VIRT_SCREENS_RIGHT GP_KEY_RIGHT_BRACE

/** @brief Displays a help. */
#define NEKO_KEYS_HELP GP_KEY_H

#endif /* NEKO_KEYBINDINGS_H */
