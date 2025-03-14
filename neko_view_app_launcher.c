//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <utils/gp_vec.h>
#include <core/gp_core.h>
#include <input/gp_input.h>

//TODO: Move!
#include <widgets/gp_widget_gfx.h>

#include "neko_menu.h"
#include "neko_ctx.h"
#include "neko_view_app_launcher.h"

struct apps {
	char name[32];
	char cmdline[128];
	gp_pixmap *icon;
};

static struct apps *apps;
static unsigned int apps_refcnt;
static const struct neko_view_slot_ops app_launcher_ops;

struct app_launcher {
	unsigned int app_offset;
	unsigned int app_selected;
};

#define APP_LAUNCHER_PRIV(self) (struct app_launcher*)((self)->priv)

static void load_app_list(void)
{
	apps = gp_vec_new(3, sizeof(struct apps));

	strcpy(apps[0].name, "Termini");
	strcpy(apps[0].cmdline, "termini -r -b proxy");
	apps[0].icon = NULL;

	strcpy(apps[1].name, "Dictionary");
	strcpy(apps[1].cmdline, "gpdict -b proxy");
	apps[1].icon = NULL;

	strcpy(apps[2].name, "Music Player");
	strcpy(apps[2].cmdline, "gpplayer -b proxy");
	apps[2].icon = NULL;
}

neko_view_slot *neko_app_launcher_init(void)
{
	if (!apps)
		load_app_list();

	neko_view_slot *ret = malloc(sizeof(neko_view_slot) + sizeof(struct app_launcher));
	if (!ret) {
		//TODO refcounting!
		//gp_vec_free(apps);
		return NULL;
	}

	memset(ret, 0, sizeof(*ret));

	apps_refcnt++;

	ret->ops = &app_launcher_ops;

	struct app_launcher *app_launcher = APP_LAUNCHER_PRIV(ret);

	app_launcher->app_offset = 0;
	app_launcher->app_selected = 0;

	return ret;
}

void neko_app_launcher_exit(struct neko_view_slot *self)
{
	if (--apps_refcnt == 0)
		gp_vec_free(apps);

	free(self);
}

static void draw_entry(size_t idx, gp_pixmap *pixmap, gp_pixel fg, gp_pixel bg,
                       gp_coord x, gp_coord y, gp_size w, gp_size h)
{
	gp_text_fit(pixmap, ctx.font, x, y, w,
	            GP_ALIGN_LEFT|GP_VALIGN_BELOW,
		    fg, bg, apps[idx].name);
}

void neko_app_launcher_show(neko_view *view)
{
	struct app_launcher *app_launcher = APP_LAUNCHER_PRIV(view->slot);
	gp_pixmap *pixmap = neko_view_pixmap(view);

	struct neko_menu menu = {
		.heading = "Application launcher",
		.items_cnt = gp_vec_len(apps),
		.items_offset = app_launcher->app_offset,
		.item_sel = app_launcher->app_selected,
		.focused = neko_view_is_focused(view),
		.entry_h = gp_text_ascent(ctx.font),
		.draw_entry = draw_entry,
	};

	neko_menu_repaint(&menu, pixmap);
	neko_view_flip(view);

	app_launcher->app_offset = menu.items_offset;
}

static void run_selected_app(struct app_launcher *app_launcher)
{
	char *cmdline = apps[app_launcher->app_selected].cmdline;

	int pid = fork();
	if (pid < 0)
		return;

	if (pid == 0) {
		unsigned int i = 0;
		char *opts[128] = {};

		opts[0] = cmdline;

		while (*cmdline) {
			if (*cmdline == ' ') {
				*cmdline = 0;

				if (cmdline[1])
					opts[++i] = cmdline+1;
			}

			cmdline++;

			if (i >= 128)
				break;
		}

		execvp(opts[0], opts);
		exit(1);
	}
}

static void selected_up(neko_view *view, struct app_launcher *app_launcher)
{
	if (app_launcher->app_selected)
		app_launcher->app_selected--;
	else
		app_launcher->app_selected = gp_vec_len(apps)-1;

	//TODO:
	neko_app_launcher_show(view);
}

static void selected_down(neko_view *view, struct app_launcher *app_launcher)
{
	if (app_launcher->app_selected + 1 < gp_vec_len(apps))
		app_launcher->app_selected++;
	else
		app_launcher->app_selected = 0;

	//TODO:
	neko_app_launcher_show(view);
}

void neko_app_launcher_event(neko_view *view, gp_event *ev)
{
	struct app_launcher *app_launcher = APP_LAUNCHER_PRIV(view->slot);

	switch (ev->type) {
	case GP_EV_KEY:
		if (ev->code != GP_EV_KEY_DOWN)
			break;

		switch (ev->val) {
		case GP_KEY_ENTER:
			run_selected_app(app_launcher);
			//neko_view_slot_exit(view);
		break;
		case GP_KEY_DOWN:
			selected_down(view, app_launcher);
		break;
		case GP_KEY_UP:
			selected_up(view, app_launcher);
		break;
		case GP_KEY_ESC:
			neko_view_slot_exit(view);
		break;
		}
	break;
	}
}

static const struct neko_view_slot_ops app_launcher_ops = {
	.show = neko_app_launcher_show,
	.repaint = neko_app_launcher_show,
	.event = neko_app_launcher_event,
};
