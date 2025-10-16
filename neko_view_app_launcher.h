//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2025 Cyril Hrubis <metan@ucw.cz>

 */

/**
 * @brief Application launcher.
 * @file neko_view_app_launcher.h
 */

#ifndef NEKO_VIEW_APP_LAUNCHER_H
#define NEKO_VIEW_APP_LAUNCHER_H

#include <input/gp_types.h>
#include "neko_view.h"

/**
 * @brief Creates a new slot content with app launcher.
 *
 * @return A neko view slot.
 */
neko_view_slot *neko_app_launcher_init(void);

#endif /* NEKO_VIEW_APP_LAUNCHER_H */
