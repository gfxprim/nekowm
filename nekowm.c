//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2025 Cyril Hrubis <metan@ucw.cz>

 */

#include <signal.h>
#include <gfxprim.h>
#include <sys/socket.h>

#include <backends/gp_proxy_proto.h>
#include <backends/gp_proxy_conn.h>
#include <backends/gp_proxy_shm.h>
#include <backends/gp_proxy_cli.h>

#include "neko_keybindings.h"
#include "neko_ctx.h"
#include "neko_view.h"
#include "neko_view_app_launcher.h"
#include "neko_view_running_apps.h"
#include "neko_view_app.h"
#include "neko_view_exit.h"

static gp_backend *backend;

/** @brief A list of application connected to the proxy backend. */
gp_dlist apps_list;

#define NEKO_MAIN_VIEWS 4
static neko_view main_views[NEKO_MAIN_VIEWS];
static size_t cur_view = 1;

static neko_view left_view;
static neko_view right_view;

static void do_exit(enum neko_view_exit_type exit_type)
{
	neko_view_slot *exit_view = neko_view_exit_init(exit_type);
	neko_view_slot_put(&main_views[cur_view], exit_view);
}

static void resize_views(gp_size w, gp_size h)
{
	unsigned int i;

	for (i = 0; i < NEKO_MAIN_VIEWS; i++)
		neko_view_resize(&main_views[i], w, h);

	neko_view_repaint(&main_views[cur_view]);
}

static void backend_event(gp_backend *b)
{
	gp_event *ev;

	while ((ev = gp_backend_ev_get(b))) {
		switch (ev->type) {
		case GP_EV_KEY:
			if (ev->code != GP_EV_KEY_DOWN)
				break;

			switch (ev->val) {
			case GP_KEY_POWER:
				do_exit(NEKO_VIEW_EXIT_POWEROFF);
			break;
			}

			if (!gp_ev_any_key_pressed(ev, NEKO_KEYS_MOD_WM))
				break;

			switch (ev->val) {
			case NEKO_KEYS_EXIT:
				do_exit(NEKO_VIEW_EXIT_QUIT);
			break;
			//TODO: Move to VIEWS
			case NEKO_KEYS_VIRT_SCREENS_LEFT:
				if (cur_view != 0) {
					neko_view_hide(&main_views[cur_view]);
					cur_view--;
					neko_view_show(&main_views[cur_view]);
				}
			break;
			//TODO: Move to VIEWS
			case NEKO_KEYS_VIRT_SCREENS_RIGHT:
				if (cur_view < NEKO_MAIN_VIEWS-1) {
					neko_view_hide(&main_views[cur_view]);
					cur_view++;
					neko_view_show(&main_views[cur_view]);
				}
			break;
			case NEKO_KEYS_ROTATE:
				//TODO: Add gp_backend_rotate_*() functions and
				//generate resize events in backend on rotate!
				gp_pixmap_rotate_cw(backend->pixmap);
				resize_views(gp_pixmap_w(backend->pixmap), gp_pixmap_h(backend->pixmap));
			break;
			default:
			break;
			}

		break;
		case GP_EV_SYS:
			switch (ev->code) {
			case GP_EV_SYS_QUIT:
				do_exit(NEKO_VIEW_EXIT_QUIT);
			break;
			case GP_EV_SYS_RESIZE:
				gp_backend_resize_ack(b);
				resize_views(ev->sys.w, ev->sys.h);
				return;
			}
		break;
		default:
		break;
		}

		neko_view_event(&main_views[cur_view], ev);
	}
}

/**
 * @brief Slot empty callback.
 *
 * If slot becomes empty put a list of running apps into it.
 */
static void slot_exit_running_apps(neko_view *self)
{
	neko_view_slot_put(self, neko_running_apps_init());
}

static int client_add(gp_backend *backend, int fd)
{
	gp_proxy_cli *cli = gp_proxy_cli_add(&apps_list, fd);

	if (!cli)
		goto err0;

	neko_view_slot *app = neko_view_app_init(cli);

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

struct neko_config {
	char backend_opts[256];
	char font_family[256];
	char rotate[4];
	bool color_swap;
};

static struct gp_json_struct neko_cfg_desc[] = {
	GP_JSON_SERDES_STR_CPY(struct neko_config, backend_opts, GP_JSON_SERDES_OPTIONAL, 256),
	GP_JSON_SERDES_BOOL(struct neko_config, color_swap, GP_JSON_SERDES_OPTIONAL),
	GP_JSON_SERDES_STR_CPY(struct neko_config, font_family, GP_JSON_SERDES_OPTIONAL, 256),
	//TODO: Add enum serdes?
	GP_JSON_SERDES_STR_CPY(struct neko_config, rotate, GP_JSON_SERDES_OPTIONAL, 4),
	{}
};

static void print_help(const char *name)
{
	printf("%s -b backend_options -f font_family -r\n", name);
	printf("\t-b backend options, pass 'help' for help\n");
	printf("\t-f font family, pass 'help' for help\n");
	printf("\t-r rotate display 90, 180, 270 degrees\n");
	printf("\t-s swap fg and bg colors (default is white on black)\n");
}

enum display_rotation {
	DISPLAY_ROTATE_0,
	DISPLAY_ROTATE_90,
	DISPLAY_ROTATE_180,
	DISPLAY_ROTATE_270,
	DISPLAY_ROTATE_INVALID = -1,
};

static void load_cfg(struct neko_config *cfg)
{
	gp_json_load_struct("/etc/nekowm.conf", neko_cfg_desc, cfg);
}

static enum display_rotation str_to_rot(char *rotate)
{
	if (!strcmp(rotate, "90"))
		return DISPLAY_ROTATE_90;
	else if (!strcmp(rotate, "180"))
		return DISPLAY_ROTATE_180;
	else if (!strcmp(rotate, "270"))
		return DISPLAY_ROTATE_270;
	else
		return DISPLAY_ROTATE_INVALID;
}

int main(int argc, char *argv[])
{
	int opt;
	struct neko_config cfg = {
		.font_family = "haxor-narrow-18",
	};

	enum display_rotation display_rotation = DISPLAY_ROTATE_0;

	load_cfg(&cfg);

	display_rotation = str_to_rot(cfg.rotate);

	signal(SIGPIPE, SIG_IGN);

	while ((opt = getopt(argc, argv, "b:f:hr:s")) != -1) {
	switch (opt) {
		case 'b':
			strncpy(cfg.backend_opts, optarg, sizeof(cfg.backend_opts)-1);
		break;
		case 'f':
			strncpy(cfg.font_family, optarg, sizeof(cfg.font_family)-1);
		break;
		case 'h':
			print_help(argv[0]);
			exit(0);
		break;
		case 'r':
			display_rotation = str_to_rot(optarg);

			if (display_rotation == DISPLAY_ROTATE_INVALID) {
				print_help(argv[0]);
				exit(1);
			}
		break;
		case 's':
			cfg.color_swap = true;
		break;
		default:
			print_help(argv[0]);
			exit(1);
		}
	}

	if (!strcmp(cfg.font_family, "help")) {
		gp_fonts_iter i;
		const gp_font_family *f;

		printf("Available compiled in fonts:\n\n");

		GP_FONT_FAMILY_FOREACH(&i, f)
			printf("\t\t - %s\n", f->family_name);

		printf("\n");
		exit(0);
	}

	backend = gp_backend_init(cfg.backend_opts, 0, 0, "NekoWM");
	if (!backend) {
		fprintf(stderr, "Failed to initialize backend\n");
		return 1;
	}

	switch (display_rotation) {
	case DISPLAY_ROTATE_270:
		gp_pixmap_rotate_cw(backend->pixmap);
	/* fallthrough */
	case DISPLAY_ROTATE_180:
		gp_pixmap_rotate_cw(backend->pixmap);
	/* fallthrough */
	case DISPLAY_ROTATE_90:
		gp_pixmap_rotate_cw(backend->pixmap);
	break;
	case DISPLAY_ROTATE_INVALID:
	case DISPLAY_ROTATE_0:
	break;
	}

	gp_size w = gp_pixmap_w(backend->pixmap);
	gp_size h = gp_pixmap_h(backend->pixmap);

	neko_ctx_init(backend, cfg.color_swap, cfg.font_family);

	unsigned int i;

	for (i = 0; i < NEKO_MAIN_VIEWS; i++) {
		neko_view_init(&main_views[i], 0, 0, w, h);
		main_views[i].slot_exit = slot_exit_running_apps;
	}

	main_views[0].slot = neko_app_launcher_init();

	neko_subviews_init(&left_view, &right_view, &main_views[3]);

	neko_view_slot_put(&left_view, neko_running_apps_init());
	left_view.slot_exit = slot_exit_running_apps;
	neko_view_slot_put(&right_view, neko_running_apps_init());
	right_view.slot_exit = slot_exit_running_apps;

	neko_view_slot_put(&main_views[1], neko_running_apps_init());
	main_views[1].slot_exit = slot_exit_running_apps;

	neko_view_slot_put(&main_views[2], neko_running_apps_init());
	main_views[2].slot_exit = slot_exit_running_apps;

	neko_view_show(&main_views[cur_view]);

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
