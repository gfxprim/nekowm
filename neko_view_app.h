//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef NEKO_VIEW_APP
#define NEKO_VIEW_APP

/**
 * @brief Creates a child for a running application.
 *
 * Displays an application.
 */
neko_view_slot *neko_view_app_init(gp_proxy_cli *cli);

/**
 * @brief A poll handler for the app child.
 */
enum gp_poll_event_ret neko_view_app_event(gp_fd *self);

/**
 * @brief A gp_vec of running applications.
 */
extern neko_view_slot **neko_apps;

/**
 * @brief Returns a proxy cli handle for a neko_view_app.
 *
 * This is supposed to be used on the pointers in the #neko_apps array.
 *
 * @param self A neko_view_slot with an app.
 *
 * @return A proxy cli handle.
 */
gp_proxy_cli *neko_view_app_cli(neko_view_slot *self);

#endif /* NEKO_VIEW_APP */
