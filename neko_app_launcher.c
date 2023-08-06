//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2023 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <utils/gp_vec.h>
#include <core/gp_core.h>
#include <input/gp_input.h>

//TODO: Move!
#include <widgets/gp_widget_gfx.h>

#include "neko_ctx.h"
#include "neko_app_launcher.h"

struct apps {
	char name[32];
	char cmdline[128];
	gp_pixmap *icon;
};

static struct apps *apps;

static unsigned int app_offset = 0;
static unsigned int app_selected = 0;

void neko_app_launcher_init(void)
{
	apps = gp_vec_new(2, sizeof(struct apps));

	strcpy(apps[0].name, "Termini");
	strcpy(apps[0].cmdline, "termini -b proxy");
	apps[0].icon = NULL;

	strcpy(apps[1].name, "Dictionary");
	strcpy(apps[1].cmdline, "gpdict -b proxy");
	apps[1].icon = NULL;
}

void neko_app_launcher_exit(void)
{
	gp_vec_free(apps);
}

void neko_app_launcher_show(neko_view *view)
{
	gp_pixmap *pix = neko_view_pixmap(view);
	gp_size ascent = gp_text_ascent(ctx.font);

	gp_fill(pix, ctx.col_bg);

	unsigned int i = app_offset;
	gp_coord y = ctx.padd;
	gp_coord x = ctx.padd;
	gp_size w = view->w - 2 * ctx.padd;

	for (;;) {
		if (i >= gp_vec_len(apps))
			break;


		if (y + ascent + ctx.padd > view->h)
			break;

		gp_pixel fg = ctx.col_fg;
		gp_pixel bg = ctx.col_bg;

		if (i == app_selected) {
			GP_SWAP(bg, fg);
			gp_fill_rect_xywh(pix, x, y, w, ascent+ctx.padd, bg);
		}

		y += ascent;

		gp_text_fit(pix, ctx.font, x, y, w,
				GP_ALIGN_LEFT|GP_VALIGN_BASELINE, fg,
				bg, apps[i].name);

		y += ctx.padd;
		i++;
	}

	neko_view_flip(view);
}

static void run_selected_app(void)
{
	char *cmdline = apps[app_selected].cmdline;

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

static void selected_up(neko_view *view)
{
	if (app_selected)
		app_selected--;
	else
		app_selected = gp_vec_len(apps)-1;

	//TODO:
	neko_app_launcher_show(view);
}

static void selected_down(neko_view *view)
{
	if (app_selected + 1 < gp_vec_len(apps))
		app_selected++;
	else
		app_selected = 0;

	//TODO:
	neko_app_launcher_show(view);
}

int neko_app_launcher_event(gp_event *ev, neko_view *view)
{
	switch (ev->type) {
	case GP_EV_KEY:
		if (ev->code != GP_EV_KEY_DOWN)
			break;

		switch (ev->val) {
		case GP_KEY_ENTER:
			run_selected_app();
			return 1;
		break;
		case GP_KEY_DOWN:
			selected_down(view);
		break;
		case GP_KEY_UP:
			selected_up(view);
		break;
		case GP_KEY_ESC:
			return 1;
		break;
		}
	break;
	}

	return 0;
}

void neko_app_launcher_hide(void)
{

}
