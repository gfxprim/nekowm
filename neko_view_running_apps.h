//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef NEKO_RUNNING_APPS
#define NEKO_RUNINNG_APPS

/**
 * @brief Creates a new running apps view child.
 *
 * Displays a list of currently connected applications.
 */
neko_view_slot *neko_running_apps_init(void);

/**
 * @brief Called when new application has connected or an application has disconnected.
 *
 * This causes all currently shown running app lists to update.
 */
void neko_running_apps_changed(void);

#endif /* NEKO_RUNNING_APPS */
