//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>

#include <backends/gp_proxy_shm.h>
#include <backends/gp_proxy_cli.h>

#include "neko_keybindings.h"
#include "neko_view.h"
#include "neko_ctx.h"
#include "neko_view_running_apps.h"
#include "neko_view_app.h"

extern gp_dlist apps_list;

struct app {
	gp_proxy_cli *cli;
	gp_proxy_shm *shm;
};

/* A gp_vec of all connected apps. */
neko_view_slot **neko_apps;

#define APP_PRIV(self) (struct app*)((self)->priv)

static const neko_view_slot_ops app_ops;

neko_view_slot *neko_view_app_init(gp_proxy_cli *cli)
{
	neko_view_slot *ret = malloc(sizeof(neko_view_slot) + sizeof(struct app));
	if (!ret)
		goto err0;

	memset(ret, 0, sizeof(*ret));

	ret->ops = &app_ops;

	struct app *app = APP_PRIV(ret);

	app->cli = cli;
	app->shm = NULL;

	if (!neko_apps) {
		neko_apps = gp_vec_new(0, sizeof(neko_view_slot *));
		if (!neko_apps)
			goto err1;
	}

	if (!GP_VEC_APPEND(neko_apps, ret))
		goto err1;

	return ret;
err1:
	free(ret);
err0:
	return NULL;
}

gp_proxy_cli *neko_view_app_cli(neko_view_slot *self)
{
	struct app *app = APP_PRIV(self);

	return app->cli;
}

static void app_resize(neko_view *self)
{
	struct app *app = APP_PRIV(self->slot);

	/**
	 * We cannot resize the SHM until app stops using it, so we only
	 * request unmap in the resize call and remap the application in
	 * the message handler.
	 */
	gp_proxy_cli_send(app->cli, GP_PROXY_UNMAP, NULL);
}

static void app_hide(neko_view *self)
{
	struct app *app = APP_PRIV(self->slot);

	if (app->shm) {
		gp_proxy_shm_exit(app->shm);
		app->shm = NULL;
	}

	gp_proxy_cli_hide(app->cli);
}

static void app_show(neko_view *self)
{
	gp_backend *backend = ctx.backend;
	static int proxy_cnt;

	struct gp_proxy_coord cur_pos = {
		.x = backend->event_queue->state.cursor_x - self->x,
		.y = backend->event_queue->state.cursor_y - self->y,
	};

	struct app *app = APP_PRIV(self->slot);
	char proxy_path[64];

	//TODO: Move unique path creation to the library.
	snprintf(proxy_path, sizeof(proxy_path), "/dev/shm/.proxy_backend-%i", proxy_cnt++);

	app->shm = gp_proxy_shm_init(proxy_path, self->w, self->h, backend->pixmap->pixel_type);
	if (!app->shm) {
		GP_WARN("Failed to initialize SHM");
		//TODO proper error handling
		return;
	}

	gp_proxy_cli_show(app->cli, app->shm, &cur_pos);
}

static void app_event(neko_view *self, gp_event *ev)
{
	struct app *app = APP_PRIV(self->slot);

	switch (ev->type) {
	case GP_EV_KEY:
		if (!gp_ev_any_key_pressed(ev, NEKO_KEYS_MOD_WM))
			break;

		if (ev->code != GP_EV_KEY_DOWN)
			break;

		switch (ev->val) {
		case NEKO_KEYS_QUIT:
			GP_DEBUG(4, "Closing cli (%p) '%s'", app->cli, app->cli->name);
			gp_proxy_cli_send(app->cli, GP_PROXY_EXIT, NULL);
			return;
		break;
		}
	break;
	}

	gp_proxy_cli_event(app->cli, ev);
}

static const neko_view_slot_ops app_ops = {
	.show = app_show,
	.hide = app_hide,
	.remove = app_hide,
	.resize = app_resize,
	.event = app_event,
};

static void err_rem_cli(neko_view_slot *slot, gp_fd *self)
{
	struct app *app = APP_PRIV(slot);
	size_t i;

	for (i = 0; i < gp_vec_len(neko_apps); i++) {
		if (slot == neko_apps[i])
			neko_apps = gp_vec_del(neko_apps, i, 1);
	}

	gp_backend_poll_rem(ctx.backend, self);

	close(self->fd);

	//TODO! No cli list? keeps apps in vector?
	gp_proxy_cli_rem(&apps_list, app->cli);

	neko_view_slot_exit(slot->view);

	free(slot);

	neko_running_apps_changed();
}

/*
 * App resize handler, we have to wait for the client to unmap the memory
 * before we resize it, hence we have to wait for the application to ack the resize.
 */
static void on_unmap(neko_view_slot *slot, gp_proxy_cli *cli)
{
	if (!neko_view_is_shown(slot->view))
		return;

	struct app *app = APP_PRIV(slot);

	if (gp_proxy_shm_resize(app->shm, slot->view->w, slot->view->h) < 0) {
		GP_WARN("Failed to resize shm!");
		//TODO: proper error
		return;
	}

	gp_proxy_cli_send(cli, GP_PROXY_MAP, &app->shm->path);
	gp_proxy_cli_send(cli, GP_PROXY_PIXMAP, &app->shm->pixmap);
	gp_proxy_cli_send(cli, GP_PROXY_SHOW, NULL);
}

static void shm_update(neko_view_slot *slot, gp_coord x, gp_coord y, gp_size w, gp_size h)
{
	if (!neko_view_is_shown(slot->view))
		return;

	neko_view *view = slot->view;
	gp_size screen_h = view->h;
	struct app *app = APP_PRIV(slot);

	if (h > screen_h) {
		GP_WARN("Invalid height");
		h = screen_h;
	}

	//printf("%i %i %u %u\n", x, y, w, h);

	//TODO: Check SIZE!!!
	gp_blit_xywh_clipped(&app->shm->pixmap, x, y, w, h, neko_view_pixmap(view), x, y);

	neko_view_update_rect(view, x, y, w, h);
}

enum gp_poll_event_ret neko_view_app_event(gp_fd *self)
{
	gp_proxy_msg *msg;
	neko_view_slot *slot = self->priv;
	struct app *app = APP_PRIV(slot);

	if (gp_proxy_cli_read(app->cli)) {
		err_rem_cli(slot, self);
		return 0;
	}

	for (;;) {
		if (gp_proxy_cli_msg(app->cli, &msg)) {
			err_rem_cli(slot, self);
			return 0;
		}

		if (!msg)
			return 0;

		switch (msg->type) {
		case GP_PROXY_UNMAP:
			on_unmap(slot, app->cli);
		break;
		case GP_PROXY_UPDATE:
			shm_update(slot,
				   msg->rect.rect.x, msg->rect.rect.y,
				   msg->rect.rect.w, msg->rect.rect.h);
		break;
		case GP_PROXY_NAME:
			neko_running_apps_changed();
		break;
		}

	}
}

