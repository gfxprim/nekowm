//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2025 Cyril Hrubis <metan@ucw.cz>

 */

#include <unistd.h>

#include <gfxprim.h>

#include <backends/gp_proxy_shm.h>
#include <backends/gp_proxy_cli.h>

#include "neko_keybindings.h"
#include "neko_view.h"
#include "neko_ctx.h"
#include "neko_view_app.h"
#include "neko_view_exit.h"
#include "neko_logo.h"

static enum neko_view_exit_type exit_type;
static int timeout = 30;

static uint32_t exit_timeout_callback(gp_timer *self);

static gp_timer exit_timer = {
	.expires = 1000,
	.period = 1000,
	.id = "Exit timer",
	.callback = exit_timeout_callback,
};

static void do_exit(void)
{
	GP_DEBUG(1, "Applications finished, exitting...");
	gp_backend_exit(ctx.backend);
	exit(0);
}

static void do_poweroff(void)
{
	gp_backend_exit(ctx.backend);
	GP_DEBUG(1, "Applications finished, calling poweroff...");

	execlp("sudo", "sudo", "poweroff", NULL);

	/* Shoudln't be reached. */
	exit(0);
}

static void print_poweroff(neko_view *self, gp_size w, gp_size h, gp_size ta)
{
	gp_pixmap *pixmap = neko_view_pixmap(self);

	neko_logo_render(pixmap, &neko_logo_cat, h/5, ctx.dark_theme);

	gp_coord y = h/2 + h/4;

	gp_print(pixmap, ctx.font_bold, w/2, y, GP_ALIGN_CENTER|GP_VALIGN_CENTER,
		         ctx.col_fg, ctx.col_bg,
	                 "\u00ab Machine is powered off \u00bb");
	neko_view_flip(self);
	gp_backend_ev_poll(ctx.backend);
	//TODO: Fix e-ink to do a final refresh
	sleep(1);
}

static void exit_show(neko_view *self)
{
	gp_pixmap *pixmap = neko_view_pixmap(self);
	gp_size w = gp_pixmap_w(pixmap);
	gp_size h = gp_pixmap_h(pixmap);
	gp_size ta = gp_text_ascent(ctx.font);

	neko_logo_render(pixmap, &neko_logo_cat, h/5, ctx.dark_theme);

	gp_coord y = h/2 + h/4;

	gp_print(pixmap, ctx.font_bold, w/2, y - 2*ta, GP_ALIGN_CENTER|GP_VALIGN_CENTER,
		         ctx.col_fg, ctx.col_bg,
	                 "\u00ab %s \u00bb", exit_type == NEKO_VIEW_EXIT_POWEROFF ? "Powering off" : "Exitting");

	gp_print(pixmap, ctx.font, w/2, y, GP_ALIGN_CENTER|GP_VALIGN_CENTER,
		         ctx.col_fg, ctx.col_bg,
	                 "Press %s+%s to force %s.",
			 gp_ev_key_name(NEKO_KEYS_MOD_WM), gp_ev_key_name(NEKO_KEYS_FORCE),
	                 exit_type == NEKO_VIEW_EXIT_POWEROFF ? "power off" : "exit");


	gp_print(pixmap, ctx.font, w/2, y + 2*ta, GP_ALIGN_CENTER|GP_VALIGN_CENTER,
		         ctx.col_fg, ctx.col_bg,
	                 "Running apps %zu timeout %is",
	                 gp_vec_len(neko_apps), timeout);

	neko_view_flip(self);

	if (!neko_view_app_cnt() || timeout <= 0) {
		gp_backend_timer_rem(ctx.backend, &exit_timer);
		sleep(1);
		switch (exit_type) {
		case NEKO_VIEW_EXIT_POWEROFF:
			print_poweroff(self, w, h, ta);
			do_poweroff();
		break;
		default:
			do_exit();
		}
	}
}

static void exit_event(neko_view *self, gp_event *ev)
{
	switch (ev->type) {
	case GP_EV_KEY:
		if (!gp_ev_any_key_pressed(ev, NEKO_KEYS_MOD_WM))
			break;

		if (ev->code != GP_EV_KEY_DOWN)
			break;

		if (ev->key.key == NEKO_KEYS_FORCE)
			timeout = 0;
	break;
	}
}

static const neko_view_slot_ops exit_ops = {
	.show = exit_show,
	.resize = exit_show,
	.event = exit_event,
};

static neko_view_slot exit_slot = {
	.ops = &exit_ops,
};

void neko_view_exit_app_disconnected(void)
{
	if (!exit_type)
		return;

	exit_show(exit_slot.view);
}

void neko_view_exit_app_connected(gp_proxy_cli *cli)
{
	if (!exit_type)
		return;

	neko_view_app_exit(cli);

	exit_show(exit_slot.view);
}

static uint32_t exit_timeout_callback(gp_timer *self)
{
	timeout--;
	exit_show(exit_slot.view);
	return self->period;
}

neko_view_slot *neko_view_exit_init(enum neko_view_exit_type type)
{
	size_t i;

	GP_DEBUG(1, "Shutting down all applications");

	for (i = 0; i < gp_vec_len(neko_apps); i++)
		neko_view_app_exit(neko_view_app_cli(neko_apps[i]));

	exit_type = type;

	gp_backend_timer_add(ctx.backend, &exit_timer);

	return &exit_slot;
}

