//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

/**
 * @brief A view slot content with a list of runinng apps.
 * @file neko_view_running_apps.h
 */

#ifndef NEKO_RUNNING_APPS_H
#define NEKO_RUNINNG_APPS_H

/**
 * @brief Creates a new running apps view child.
 *
 * Displays a list of currently connected applications.
 *
 * @return A neko view slot.
 */
neko_view_slot *neko_running_apps_init(void);

/**
 * @brief Called when new application has connected or an application has disconnected.
 *
 * This causes all currently shown running app lists to update.
 */
void neko_running_apps_changed(void);

#endif /* NEKO_RUNNING_APPS_H */
