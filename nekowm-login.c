//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include "login.h"

static gp_widget *user_name, *user_passwd;

static int login_on_event(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	if (ev->self->type == GP_WIDGET_TBOX && ev->sub_type != GP_WIDGET_TBOX_TRIGGER)
		return 0;

	const char *name = gp_widget_tbox_text(user_name);
	const char *passwd = gp_widget_tbox_text(user_passwd);

	if (neko_check_login(name, passwd)) {
		gp_widgets_quit();
		neko_switch_user(name);
		execlp("nekowm", "nekowm", NULL);
		exit(1);
	}

	gp_widget_tbox_clear(user_passwd);

	return 0;
}

int main(int argc, char *argv[])
{
	gp_widget *layout, *btn;

	layout = gp_widget_grid_new(1, 3, GP_WIDGET_GRID_UNIFORM);
	layout->align = GP_FILL;

	gp_widget_grid_border_set(layout, GP_WIDGET_BORDER_ALL, 1, 1);

	gp_widget_grid_row_fill_set(layout, 0, 0);
	gp_widget_grid_row_fill_set(layout, 1, 0);
	gp_widget_grid_row_fill_set(layout, 2, 0);

	user_name = gp_widget_tbox_new(NULL, 0, 20, 0, NULL, GP_WIDGET_TBOX_NONE);
	user_name->align = GP_HFILL;
	gp_widget_tbox_help_set(user_name, "user name");
	gp_widget_grid_put(layout, 0, 0, user_name);
	gp_widget_on_event_set(user_name, login_on_event, NULL);

	user_passwd = gp_widget_tbox_new(NULL, 0, 20, 0, NULL, GP_WIDGET_TBOX_HIDDEN);
	user_passwd->align = GP_HFILL;
	gp_widget_tbox_help_set(user_passwd, "password");
	gp_widget_grid_put(layout, 0, 1, user_passwd);
	gp_widget_on_event_set(user_passwd, login_on_event, NULL);

	btn = gp_widget_button_new("Login", GP_BUTTON_LABEL);
	gp_widget_on_event_set(btn, login_on_event, NULL);
	btn->align = GP_HFILL;

	gp_widget_grid_put(layout, 0, 2, btn);

	gp_widgets_main_loop(layout, NULL, argc, argv);

	return 0;
}
