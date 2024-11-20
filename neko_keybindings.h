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

/* @brief Quits the current app. */
#define NEKO_KEYS_QUIT GP_KEY_Q

/* @brief Exits the WM. */
#define NEKO_KEYS_EXIT GP_KEY_X

/* @brief Go to the list of running applications. */
#define NEKO_KEYS_LIST_APPS GP_KEY_R

/* @brief Show application launcher. */
#define NEKO_KEYS_APP_LAUNCHER GP_KEY_A

#endif /* NEKO_KEYBINDINGS_H */
