//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>
*/

/**
 * @brief A running application.
 * @file neko_view_app.h
 */

#ifndef NEKO_VIEW_APP_H
#define NEKO_VIEW_APP_H

/**
 * @brief Creates a slot content for a running application.
 *
 * @param cli A proxy backend client handle.
 *
 * Displays an application.
 */
neko_view_slot *neko_view_app_init(gp_proxy_cli *cli);

/**
 * @brief A poll handler for the app slot.
 *
 * @param self A gfxprim poll fd. The priv pointer must point to the
 * application slot returned from neko_view_app_init().
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

/**
 * @brief Requests an client exit.
 *
 * @param cli A client to request exit.
 */
void neko_view_app_exit(gp_proxy_cli *cli);

static inline size_t neko_view_app_cnt(void)
{
	return gp_vec_len(neko_apps);
}

#endif /* NEKO_VIEW_APP_H */
