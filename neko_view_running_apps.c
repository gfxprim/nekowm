//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>

#include <backends/gp_proxy_shm.h>
#include <backends/gp_proxy_cli.h>

#include "neko_menu.h"
#include "neko_keybindings.h"
#include "neko_ctx.h"
#include "neko_view.h"
#include "neko_view_app.h"

struct running_apps {
	size_t app_sel;
	size_t app_off;
};

#define RUNNING_APPS_PRIV(self) (struct running_apps*)((self)->priv)

static void draw_entry(size_t idx, gp_pixmap *pixmap, gp_pixel fg, gp_pixel bg,
                       gp_coord x, gp_coord y, gp_size w, gp_size h)
{
	gp_proxy_cli *cli = neko_view_app_cli(neko_apps[idx]);

	gp_print(pixmap, ctx.font, x, y, GP_ALIGN_RIGHT|GP_VALIGN_BOTTOM,
		 fg, bg, "%zu: '%s'", idx, cli->name);
}

static void redraw_running_apps(neko_view *view)
{
	struct running_apps *apps = RUNNING_APPS_PRIV(view->slot);
	gp_pixmap *pixmap = neko_view_pixmap(view);

	gp_fill(pixmap, ctx.col_bg);

	char heading[128];

	struct neko_menu menu = {
		.heading = heading,
		.items_cnt = gp_vec_len(neko_apps),
		.items_offset = apps->app_off,
		.item_sel = apps->app_sel,
		.focused = neko_view_is_focused(view),
		.entry_h = gp_text_ascent(ctx.font),
		.draw_entry = draw_entry,
	};

	snprintf(heading, sizeof(heading), "Connected apps (%zu)", gp_vec_len(neko_apps));

	neko_menu_repaint(&menu, pixmap);
	neko_view_flip(view);

	apps->app_off = menu.items_offset;
}

/**
 * @brief A list of all running apps view childs.
 *
 * We traverse the list when we need to repaint the list because app connected
 * or disconnected and repaint these children that are inserted into a view.
 */
static gp_dlist app_lists;

static const neko_view_slot_ops running_apps_ops;

neko_view_slot *neko_running_apps_init(void)
{
	neko_view_slot *ret = malloc(sizeof(neko_view_slot) + sizeof(struct running_apps));
	if (!ret)
		return NULL;

	memset(ret, 0, sizeof(*ret));

	struct running_apps *apps = RUNNING_APPS_PRIV(ret);

	apps->app_sel = 0;
	apps->app_off = 0;

	ret->ops = &running_apps_ops;
	gp_dlist_push_head(&app_lists, &ret->list);

	return ret;
}

void neko_running_apps_changed(void)
{
	gp_dlist_head *i;

	GP_LIST_FOREACH(&app_lists, i) {
		neko_view_slot *slot = GP_LIST_ENTRY(i, neko_view_slot, list);

		struct running_apps *apps = RUNNING_APPS_PRIV(slot);

		if (apps->app_sel >= gp_vec_len(neko_apps))
			apps->app_sel = 0;

		if (neko_view_is_shown(slot->view))
			redraw_running_apps(slot->view);
	}
}

static void running_apps_remove(neko_view *view)
{
	neko_view_slot *slot = view->slot;

	view->slot = NULL;

	gp_dlist_rem(&app_lists, &slot->list);
	free(slot);
}

static void show_client(neko_view *view, size_t i)
{
	if (i >= gp_vec_len(neko_apps))
		return;

	neko_view_slot_put(view, neko_apps[i]);
}

void running_apps_event(neko_view *view, gp_event *ev)
{
	struct running_apps *apps = RUNNING_APPS_PRIV(view->slot);

	switch (ev->type) {
	case GP_EV_KEY:
		if (ev->code != GP_EV_KEY_DOWN)
			return;

		switch (ev->val) {
		case GP_KEY_DOWN:
			if (apps->app_sel + 1 >= gp_vec_len(neko_apps))
				return;

			apps->app_sel++;
			redraw_running_apps(view);
		break;
		case GP_KEY_UP:
			if (apps->app_sel == 0)
				return;

			apps->app_sel--;
			redraw_running_apps(view);
		break;
		case GP_KEY_ENTER:
			show_client(view, apps->app_sel);
		break;
		}

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

static const neko_view_slot_ops running_apps_ops = {
	.show = redraw_running_apps,
	.repaint = redraw_running_apps,
	.event = running_apps_event,
	.remove = running_apps_remove,
};

