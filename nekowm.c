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

#include "nekowm.h"
#include "neko_keybindings.h"
#include "neko_ctx.h"
#include "neko_view.h"
#include "neko_view_app_launcher.h"
#include "neko_view_running_apps.h"
#include "neko_view_app.h"

static gp_backend *backend;
static const char *backend_opts = NULL;

/** @brief A list of application connected to the proxy backend. */
gp_dlist apps_list;

static void view_child_exit(neko_view *self);

static neko_view main_view = {
	.child_exit = view_child_exit,
};

static neko_view_child *app_launcher;
static neko_view_child *running_apps;

static void do_exit(void)
{
	gp_backend_exit(backend);
	exit(0);
}

void nekowm_poll_rem(gp_fd *self)
{
	gp_backend_poll_rem(backend, self);
}

static void backend_event(gp_backend *b)
{
	gp_event *ev;

	while ((ev = gp_backend_poll_event(b))) {
		switch (ev->type) {
		case GP_EV_KEY:
			if (!gp_ev_any_key_pressed(ev, NEKO_KEYS_MOD_WM))
				break;

			if (ev->code != GP_EV_KEY_DOWN)
				break;

			switch (ev->val) {
			case NEKO_KEYS_EXIT:
				do_exit();
			break;
			case NEKO_KEYS_LIST_APPS:
				neko_view_show_child(&main_view, running_apps);
				return;
			break;
			case NEKO_KEYS_APP_LAUNCHER:
				neko_view_show_child(&main_view, app_launcher);
				return;
			break;
			default:
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
				return;
			}
		break;
		default:
		break;
		}

		neko_view_event(&main_view, ev);
	}
}

static void view_child_exit(neko_view *self)
{
	neko_view_show_child(self, running_apps);
}

static int client_add(gp_backend *backend, int fd)
{
	gp_proxy_cli *cli = gp_proxy_cli_add(&apps_list, fd);

	if (!cli)
		goto err0;

	neko_view_child *app = neko_view_app_init(cli);

	cli->fd.event = neko_view_app_event;
	cli->fd.priv = app;

	gp_backend_poll_add(backend, &cli->fd);

	return 0;
err0:
	close(fd);
	return 1;
}

static enum gp_poll_event_ret server_event(gp_fd *self)
{
	int fd;

	while ((fd = accept(self->fd, NULL, NULL)) > 0) {
		struct gp_proxy_cli_init_ init = {
			.pixel_type = backend->pixmap->pixel_type,
			.dpi = backend->dpi,
		};
		gp_proxy_send(fd, GP_PROXY_CLI_INIT, &init);

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

	app_launcher = neko_app_launcher_init();
	running_apps = neko_running_apps_init();

	neko_view_show_child(&main_view, running_apps);

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
