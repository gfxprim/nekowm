//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

/**
 * @brief An utility to check user password.
 * @file login.h
 */

#ifndef NEKO_LOGIN_H
#define NEKO_LOGIN_H

/**
 * @brief Checks for a correct user password.
 *
 * @param username An user name.
 * @param password An user password.
 *
 * @return Non-zero if user name and password combination is valid, zero
 *         otherwise.
 */
int neko_check_login(const char *username, const char *passwd);

/**
 * @brief Switches to an user.
 *
 * @param username An user name.
 *
 * @return Zero on success, non-zero on a failure.
 */
int neko_switch_user(const char *username);

#endif /* NEKO_LOGIN_H */
