//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2025 Cyril Hrubis <metan@ucw.cz>

 */

/**
 * @brief Application launcher.
 * @file neko_app_launcher.h
 */

#ifndef NEKO_APP_LAUNCHER_H
#define NEKO_APP_LAUNCHER_H

/**
 * @brief Runs an application by name.
 *
 * Looks up the application in the application list and starts it.
 *
 * @app_name An application name to be started.
 */
void neko_app_run(const char *app_name);

/**
 * @brief Runs a command.
 *
 * Forks the process and executes the cmdline.
 *
 * E.g. neko_run("termini -r -b proxy") starts termini with reverse colors and
 * proxy backend.
 *
 * @cmdline A command to execute.
 */
void neko_cmd_run(char *cmdline);

#endif /* NEKO_APP_LAUNCHER_H */
