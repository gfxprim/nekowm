//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2023 Cyril Hrubis <metan@ucw.cz>

 */

/*
 * Application launcher
 */

#ifndef NEKO_APP_LAUNCHER_H__
#define NEKO_APP_LAUNCHER_H__

#include <input/gp_types.h>
#include "neko_view.h"

void neko_app_launcher_init(void);

void neko_app_launcher_exit(void);

int neko_app_launcher_event(gp_event *ev, neko_view *view);

void neko_app_launcher_show(neko_view *view);

void neko_app_launcher_hide(void);

#endif /* NEKO_APP_LAUNCHER_H__ */
