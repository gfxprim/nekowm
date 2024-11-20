//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>

#include <backends/gp_proxy_shm.h>
#include <backends/gp_proxy_cli.h>

#include "neko_keybindings.h"
#include "neko_ctx.h"
#include "neko_view.h"
#include "neko_view_app.h"

static void redraw_running_apps(neko_view *view)
{
	gp_pixmap *pixmap = neko_view_pixmap(view);

	gp_fill(pixmap, ctx.col_bg);

	gp_coord y = 20;
	gp_coord x = 20;
	gp_size spacing = 20;
	int n = 0;

	gp_print(pixmap, ctx.font, x, y, GP_ALIGN_RIGHT|GP_VALIGN_BOTTOM,
	         ctx.col_fg, ctx.col_bg, "Connected clients (%zu)", gp_vec_len(neko_apps));

	y += spacing;

	GP_VEC_FOREACH(neko_apps, neko_view_child *, child) {
		gp_proxy_cli *cli = neko_view_app_cli(*child);
		gp_print(pixmap, ctx.font, x, y, GP_ALIGN_RIGHT|GP_VALIGN_BOTTOM,
			 ctx.col_fg, ctx.col_bg, "%i: '%s'", n++, cli->name);
		y += spacing;
	}

	neko_view_flip(view);
}

/**
 * @brief A list of all running apps view childs.
 *
 * We traverse the list when we need to repaint the list because app connected
 * or disconnected and repaint these children that are inserted into a view.
 */
static gp_dlist app_lists;

struct running_apps {
};

#define RUNNING_APPS_PRIV(self) (struct running_apps*)((self)->priv)

static const neko_view_child_ops running_apps_ops;

neko_view_child *neko_running_apps_init(void)
{
	neko_view_child *ret = malloc(sizeof(neko_view_child) + sizeof(struct running_apps));
	if (!ret)
		return NULL;

	ret->ops = &running_apps_ops;
	gp_dlist_push_head(&app_lists, &ret->list);

	return ret;
}

void neko_running_apps_changed(void)
{
	gp_dlist_head *i;

	GP_LIST_FOREACH(&app_lists, i) {
		neko_view_child *child = GP_LIST_ENTRY(i, neko_view_child, list);

		if (child->parent)
			redraw_running_apps(child->parent);
	}
}

static void show_client(neko_view *view, size_t i)
{
	if (i >= gp_vec_len(neko_apps))
		return;

	neko_view_show_child(view, neko_apps[i]);
}

void running_apps_event(neko_view *view, gp_event *ev)
{
	switch (ev->type) {
	case GP_EV_KEY:
		if (ev->code != GP_EV_KEY_DOWN)
			return;

		if (!gp_ev_any_key_pressed(ev, NEKO_KEYS_MOD_WM))
			return;

		switch (ev->val) {
			case GP_KEY_1 ... GP_KEY_9:
				show_client(view, ev->val - GP_KEY_1 + 1);
			break;
			case GP_KEY_0:
				show_client(view, 0);
			break;
		}

	break;
	}
}

static const neko_view_child_ops running_apps_ops = {
	.show = redraw_running_apps,
	.resize = redraw_running_apps,
	.event = running_apps_event,
};

