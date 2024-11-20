//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>

#include <backends/gp_proxy_shm.h>
#include <backends/gp_proxy_cli.h>

#include "nekowm.h"
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
neko_view_child **neko_apps;

#define APP_PRIV(self) (struct app*)((self)->priv)

static const neko_view_child_ops app_ops;

neko_view_child *neko_view_app_init(gp_proxy_cli *cli)
{
	neko_view_child *ret = malloc(sizeof(neko_view_child) + sizeof(struct app));
	if (!ret)
		goto err0;

	ret->ops = &app_ops;

	struct app *app = APP_PRIV(ret);

	app->cli = cli;
	app->shm = NULL;

	if (!neko_apps) {
		neko_apps = gp_vec_new(0, sizeof(neko_view_child *));
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

gp_proxy_cli *neko_view_app_cli(neko_view_child *self)
{
	struct app *app = APP_PRIV(self);

	return app->cli;
}

static void app_resize(neko_view *self)
{
	struct app *app = APP_PRIV(self->child);

	/**
	 * We cannot resize the SHM until app stops using it, so we only
	 * request unmap in the resize call and remap the application in
	 * the message handler.
	 */
	gp_proxy_cli_send(app->cli, GP_PROXY_UNMAP, NULL);
}

static void app_hide(neko_view *self)
{
	struct app *app = APP_PRIV(self->child);

	gp_proxy_shm_exit(app->shm);
	app->shm = NULL;

	gp_proxy_cli_hide(app->cli);
}

static void app_show(neko_view *self)
{
	gp_backend *backend = self->backend;

	struct gp_proxy_coord cur_pos = {
		.x = backend->event_queue->state.cursor_x - self->x,
		.y = backend->event_queue->state.cursor_y - self->y,
	};

	struct app *app = APP_PRIV(self->child);

	//TODO: Randomize shm path!
	app->shm = gp_proxy_shm_init("/dev/shm/.proxy_backend", self->w, self->h, backend->pixmap->pixel_type);
	if (!app->shm) {
		GP_WARN("Failed to initialize SHM");
		//TODO proper error handling
		return;
	}

	gp_proxy_cli_show(app->cli, app->shm, &cur_pos);
}

static void app_event(neko_view *self, gp_event *ev)
{
	struct app *app = APP_PRIV(self->child);

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

static const neko_view_child_ops app_ops = {
	.show = app_show,
	.hide = app_hide,
	.resize = app_resize,
	.event = app_event,
};

static void err_rem_cli(neko_view_child *view_child, gp_fd *self)
{
	struct app *app = APP_PRIV(view_child);
	size_t i;

	for (i = 0; i < gp_vec_len(neko_apps); i++) {
		if (view_child == neko_apps[i])
			neko_apps = gp_vec_del(neko_apps, i, 1);
	}

	close(self->fd);

	//TODO: Move the deallocation to child exit!
	nekowm_poll_rem(self);

	//TODO! No cli list? keeps apps in vector?
	gp_proxy_cli_rem(&apps_list, app->cli);

	neko_view_child_exit(view_child->parent);

	free(view_child);

	neko_running_apps_changed();
}

/*
 * App resize handler, we have to wait for the client to unmap the memory
 * before we resize it, hence we have to wait for the application to ack the resize.
 */
static void on_unmap(neko_view_child *view_child, gp_proxy_cli *cli)
{
	if (!view_child->parent)
		return;

	struct app *app = APP_PRIV(view_child);

	if (gp_proxy_shm_resize(app->shm, view_child->parent->w, view_child->parent->h) < 0) {
		GP_WARN("Failed to resize shm!");
		//TODO: proper error
		return;
	}

	gp_proxy_cli_send(cli, GP_PROXY_MAP, &app->shm->path);
	gp_proxy_cli_send(cli, GP_PROXY_PIXMAP, &app->shm->pixmap);
	gp_proxy_cli_send(cli, GP_PROXY_SHOW, NULL);
}

static void shm_update(neko_view_child *view_child, gp_coord x, gp_coord y, gp_size w, gp_size h)
{
	if (!view_child->parent)
		return;

	neko_view *view = view_child->parent;
	gp_size screen_h = view->h;
	struct app *app = APP_PRIV(view->child);

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
	neko_view_child *view_child = self->priv;
	struct app *app = APP_PRIV(view_child);

	if (gp_proxy_cli_read(app->cli)) {
		err_rem_cli(view_child, self);
		return 0;
	}

	for (;;) {
		if (gp_proxy_cli_msg(app->cli, &msg)) {
			err_rem_cli(view_child, self);
			return 0;
		}

		if (!msg)
			return 0;

		switch (msg->type) {
		case GP_PROXY_UNMAP:
			on_unmap(view_child, app->cli);
		break;
		case GP_PROXY_UPDATE:
			shm_update(view_child,
				   msg->rect.rect.x, msg->rect.rect.y,
				   msg->rect.rect.w, msg->rect.rect.h);
		break;
		case GP_PROXY_NAME:
			neko_running_apps_changed();
		break;
		}

	}
}

