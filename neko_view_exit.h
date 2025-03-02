//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2025 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef NEKO_VIEW_APPS_EXIT_H
#define NEKO_VIEW_APPS_EXIT_H

/**
 * @brief Callback called when application has disconnected.
 */
void neko_view_exit_app_disconnected(void);

/**
 * @brief Callback called when application has been connected.
 *
 * @param cli A client that has connected.
 */
void neko_view_exit_app_connected(gp_proxy_cli *cli);

enum neko_view_exit_type {
	NEKO_VIEW_EXIT_QUIT = 0x0f,
	NEKO_VIEW_EXIT_POWEROFF = 0xf0,
};

/**
 * @brief Starts a shutdown sequence and returns shutdown view.
 *
 * This call initializes a proper shutdown sequence that is:
 *
 * - send all running applications exit requests
 * - waits, with a timeout, for applications to exit
 * - calls the do_exit callback when the sequence is finished
 *
 * @param do_exit An exit callback.
 * @return A view slot to be shown while shutdown is in progress.
 */
neko_view_slot *neko_view_exit_init(enum neko_view_exit_type exit_type);

#endif /* NEKO_VIEW_APPS_EXIT_H */
