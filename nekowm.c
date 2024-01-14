//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2023 Cyril Hrubis <metan@ucw.cz>

 */

#include <signal.h>
#include <gfxprim.h>
#include <sys/socket.h>

#include <backends/gp_proxy_proto.h>
#include <backends/gp_proxy_conn.h>
#include <backends/gp_proxy_shm.h>
#include <backends/gp_proxy_cli.h>

#include "keybindings.h"

#include "neko_ctx.h"
#include "neko_view.h"
#include "neko_app_launcher.h"

static gp_backend *backend;
static const char *backend_opts = NULL;
static struct gp_proxy_shm *shm;
static gp_dlist clients;
static gp_proxy_cli *cli_shown;

static neko_view main_view;

enum view_shows {
	VIEW_RUNNING_APPS,
	VIEW_APP_LAUNCHER,
	VIEW_APP,
};

static enum view_shows main_view_shows = VIEW_RUNNING_APPS;

static void redraw_running_apps(void)
{
	gp_dlist_head *i;
	gp_fill(backend->pixmap, ctx.col_bg);

	gp_coord y = 20;
	gp_coord x = 20;
	gp_size spacing = 20;
	int n = 0;

	gp_print(backend->pixmap, ctx.font, x, y, GP_ALIGN_RIGHT|GP_VALIGN_BOTTOM,
	         ctx.col_fg, ctx.col_bg, "Connected clients");

	y += spacing;

	GP_LIST_FOREACH(&clients, i) {
		gp_proxy_cli *cli = GP_LIST_ENTRY(i, gp_proxy_cli, head);
		gp_print(backend->pixmap, ctx.font, x, y, GP_ALIGN_RIGHT|GP_VALIGN_BOTTOM,
			 ctx.col_fg, ctx.col_bg, "%i: '%s'", n++, cli->name);
		y += spacing;
	}

	gp_backend_flip(backend);
}

static void shm_update(gp_proxy_cli *self, gp_coord x, gp_coord y, gp_size w, gp_size h)
{
	gp_size screen_h = backend->pixmap->h;

	if (self != cli_shown)
		return;

	if (h > screen_h) {
		GP_WARN("Invalid height");
		h = screen_h;
	}

	printf("%i %i %u %u\n", x, y, w, h);

	//TODO: Check SIZE!!!
	gp_blit_xywh_clipped(&shm->pixmap, x, y, w, h, backend->pixmap, x, y);

	gp_backend_update_rect_xywh(backend, x, y, w, h);
}

static void do_exit(void)
{
	gp_backend_exit(backend);
	exit(0);
}

/*
 * App resize handler, we have to wait for the client to unmap the memory
 * before we resize it, hence we have to wait for the application to ack the resize.
 */
static void on_unmap(gp_proxy_cli *self)
{
	if (self == cli_shown) {
		if (gp_proxy_shm_resize(shm, backend->pixmap->w, backend->pixmap->h) < 0)
			do_exit();

		gp_proxy_cli_send(cli_shown, GP_PROXY_MAP, &shm->path);
		gp_proxy_cli_send(cli_shown, GP_PROXY_PIXMAP, &shm->pixmap);
		gp_proxy_cli_send(cli_shown, GP_PROXY_SHOW, NULL);
	}
}

struct gp_proxy_cli_ops cli_ops = {
	.update = shm_update,
	.on_unmap = on_unmap,
};

static void hide_client(void)
{
	if (!cli_shown)
		return;

	gp_proxy_cli_send(cli_shown, GP_PROXY_HIDE, NULL);
	gp_proxy_cli_send(cli_shown, GP_PROXY_UNMAP, NULL);

	cli_shown = NULL;
}

static void show_client(int pos)
{
	int n = 0;
	gp_dlist_head *i;


	GP_LIST_FOREACH(&clients, i) {
		if (n >= pos)
			break;
		n++;
	}

	if (!i)
		return;

	gp_proxy_cli *cli = GP_LIST_ENTRY(i, gp_proxy_cli, head);

	hide_client();

	gp_proxy_cli_send(cli, GP_PROXY_MAP, &shm->path);
	gp_proxy_cli_send(cli, GP_PROXY_PIXMAP, &shm->pixmap);
	gp_proxy_cli_send(cli, GP_PROXY_SHOW, NULL);

	cli_shown = cli;
}

static void resize_shown_client(void)
{
	if (!cli_shown) {
		if (gp_proxy_shm_resize(shm, backend->pixmap->w, backend->pixmap->h) < 0)
			do_exit();
		return;
	}

	gp_proxy_cli_send(cli_shown, GP_PROXY_UNMAP, NULL);
}

static void backend_event(gp_backend *b)
{
	gp_event *ev;

	while ((ev = gp_backend_poll_event(b))) {
		switch (ev->type) {
		case GP_EV_KEY:
			if (!gp_ev_any_key_pressed(ev, NEKO_KEYS_MOD_WM))
				goto to_cli;

			if (ev->code != GP_EV_KEY_DOWN)
				goto to_cli;

			switch (ev->val) {
			case NEKO_KEYS_EXIT:
				do_exit();
			break;
			case NEKO_KEYS_QUIT:
				if (cli_shown)
					gp_proxy_cli_send(cli_shown, GP_PROXY_EXIT, NULL);
			break;
			case NEKO_KEYS_LIST_APPS:
				hide_client();
				redraw_running_apps();
			break;
			case NEKO_KEYS_APP_LAUNCHER:
				neko_app_launcher_show(&main_view);
				main_view_shows = VIEW_APP_LAUNCHER;
			break;
			case GP_KEY_1 ... GP_KEY_9:
				show_client(ev->val - GP_KEY_1 + 1);
			break;
			case GP_KEY_0:
				show_client(0);
			break;
			}

		break;
		case GP_EV_SYS:
			switch (ev->code) {
			case GP_EV_SYS_QUIT:
				do_exit();
			break;
			case GP_EV_SYS_RESIZE:
				gp_backend_resize_ack(b);

				neko_view_resize(&main_view, ev->sys.w, ev->sys.h);

				redraw_running_apps();
				gp_backend_flip(b);
				resize_shown_client();
				return;
			break;
			}
		break;
		}
to_cli:
		switch (main_view_shows) {
		case VIEW_APP_LAUNCHER:
			if (neko_app_launcher_event(ev, &main_view))
				main_view_shows = VIEW_RUNNING_APPS;
		break;
		default:
			if (cli_shown)
				gp_proxy_cli_event(cli_shown, ev);
			else
				redraw_running_apps();
		break;
		}
	}
}

static int client_event(gp_fd *self)
{
	if (gp_proxy_cli_read(self->priv, &cli_ops)) {
		gp_backend_poll_rem(backend, self);

		close(self->fd);

		if (self->priv == cli_shown) {
			cli_shown = NULL;
			redraw_running_apps();
		}

		gp_proxy_cli_rem(&clients, self->priv);
	}

	return 0;
}

static int client_add(gp_backend *backend, int fd)
{
	gp_proxy_cli *cli = gp_proxy_cli_add(&clients, fd);

	if (!cli)
		goto err0;

	cli->fd.event = client_event;
	cli->fd.priv = cli;

	gp_backend_poll_add(backend, &cli->fd);

	return 0;
err0:
	close(fd);
	return 1;
}

static int server_event(gp_fd *self)
{
	int fd;

	while ((fd = accept(self->fd, NULL, NULL)) > 0) {
		/*
		 * Pixel type has to be send first so that backend can return
		 * from init() function.
		 */
		gp_proxy_send(fd, GP_PROXY_PIXEL_TYPE, &backend->pixmap->pixel_type);

		client_add(self->priv, fd);
	}

	return 0;
}

static void print_help(const char *name)
{
	printf("%s -b backend_options -f font_family\n", name);
}

int main(int argc, char *argv[])
{
	int opt;
	const char *font_family = "haxor-narrow-18";

	signal(SIGPIPE, SIG_IGN);

	while ((opt = getopt(argc, argv, "b:f:h")) != -1) {
	switch (opt) {
		case 'b':
			backend_opts = optarg;
		break;
		case 'f':
			font_family = optarg;
		break;
		case 'h':
			print_help(argv[0]);
		break;
		default:
			fprintf(stderr, "Invalid parameter '%c'", opt);
		}
	}

	backend = gp_backend_init(backend_opts, 0, 0, "NekoWM");
	if (!backend) {
		fprintf(stderr, "Failed to initialize backend\n");
		return 1;
	}

	gp_size w = backend->pixmap->w;
	gp_size h = backend->pixmap->h;

	neko_ctx_init(backend, font_family);
	neko_view_init(&main_view, backend, 0, 0, w, h);
	neko_app_launcher_init();

	shm = gp_proxy_shm_init("/dev/shm/.proxy_backend", w, h, backend->pixmap->pixel_type);
	if (!shm) {
		gp_backend_exit(backend);
		return 1;
	}

	redraw_running_apps();

	int fd = gp_proxy_server_init(NULL);
	gp_fd server_fd = {
		.fd = fd,
		.event = server_event,
		.events = GP_POLLIN,
		.priv = backend,
	};

	gp_backend_poll_add(backend, &server_fd);

	for (;;) {
		gp_backend_wait(backend);
		backend_event(backend);
	}

	return 0;
}
