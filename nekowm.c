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
#include "neko_logo.h"

static gp_backend *backend;

/** @brief A list of application connected to the proxy backend. */
gp_dlist apps_list;

#define NEKO_MAIN_VIEWS 5
static neko_view main_views[NEKO_MAIN_VIEWS];
static size_t cur_view = 1;

static neko_view left_view;
static neko_view right_view;

static neko_view top_view;
static neko_view bottom_view;

static neko_view bottom_left_view;
static neko_view bottom_right_view;

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

static void show_view(unsigned int i, int wm_is_focused)
{
	if (i >= NEKO_MAIN_VIEWS)
		return;

	if (cur_view == i)
		return;

	/* send focus out event to the focused app */
	if (wm_is_focused)
		neko_view_focus_out(&main_views[cur_view]);
	neko_view_hide(&main_views[cur_view]);

	cur_view = i;

	/* And focus in event to the focused app */
	if (wm_is_focused)
		neko_view_focus_in(&main_views[cur_view]);
	neko_view_show(&main_views[cur_view]);
}

static void backend_event(gp_backend *b)
{
	static int wm_is_focused = 0;
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

			if (ev->key.key == NEKO_KEYS_EXIT) {
				do_exit(NEKO_VIEW_EXIT_QUIT);
			} else if (ev->key.key == NEKO_KEYS_POWEROFF) {
				do_exit(NEKO_VIEW_EXIT_POWEROFF);
			} else if (ev->key.key ==  NEKO_KEYS_VIRT_SCREENS_LEFT) {
				//TODO: Move to VIEWS
				if (cur_view != 0) {
					neko_view_hide(&main_views[cur_view]);
					cur_view--;
					neko_view_show(&main_views[cur_view]);
				}
			} else if (ev->key.key == NEKO_KEYS_VIRT_SCREENS_RIGHT) {
				//TODO: Move to VIEWS
				if (cur_view < NEKO_MAIN_VIEWS-1) {
					neko_view_hide(&main_views[cur_view]);
					cur_view++;
					neko_view_show(&main_views[cur_view]);
				}
			} else if (ev->key.key == NEKO_KEYS_ROTATE) {
				//TODO: Add gp_backend_rotate_*() functions and
				//generate resize events in backend on rotate!
				gp_pixmap_rotate_cw(backend->pixmap);
				resize_views(gp_pixmap_w(backend->pixmap), gp_pixmap_h(backend->pixmap));
			}

			switch (ev->val) {
			case GP_KEY_F1 ... GP_KEY_F10:
				show_view(ev->val - GP_KEY_F1, wm_is_focused);
			break;
			case GP_KEY_F11:
			case GP_KEY_F12:
				show_view(ev->val - GP_KEY_F11, wm_is_focused);
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
			case GP_EV_SYS_FOCUS:
				wm_is_focused = ev->val;
				neko_view_event(&main_views[cur_view], ev);
			break;
			case GP_EV_SYS_VISIBILITY:
				return;
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
		struct gp_proxy_cli_init init = {
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
	char theme[64];
};

static struct gp_json_struct neko_cfg_desc[] = {
	GP_JSON_SERDES_STR_CPY(struct neko_config, backend_opts, GP_JSON_SERDES_OPTIONAL, 256),
	GP_JSON_SERDES_STR_CPY(struct neko_config, font_family, GP_JSON_SERDES_OPTIONAL, 256),
	GP_JSON_SERDES_STR_CPY(struct neko_config, rotate, GP_JSON_SERDES_OPTIONAL, 4),
	GP_JSON_SERDES_STR_CPY(struct neko_config, theme, GP_JSON_SERDES_OPTIONAL, 64),
	{}
};

static void print_help(const char *name)
{
	printf("%s -b backend_options -f font_family -r\n", name);
	printf("\t-b backend options, pass 'help' for help\n");
	printf("\t-f font family, pass 'help' for help\n");
	printf("\t-r rotate display 90, 180, 270 degrees\n");
	printf("\t-t theme either 'dark' or 'light'\n");
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

static enum neko_theme str_to_theme(const char *theme)
{
	if (!strcmp(theme, "light"))
		return NEKO_THEME_LIGHT;
	else if (!strcmp(theme, "dark"))
		return NEKO_THEME_DARK;
	else
		return NEKO_THEME_INVALID;
}

int main(int argc, char *argv[])
{
	int opt;
	struct neko_config cfg = {
		.font_family = "haxor-narrow-18",
		.theme = "dark",
	};
	enum neko_theme theme;
	enum display_rotation display_rotation = DISPLAY_ROTATE_0;

	load_cfg(&cfg);

	display_rotation = str_to_rot(cfg.rotate);
	theme = str_to_theme(cfg.theme);

	if (theme == NEKO_THEME_INVALID) {
		fprintf(stderr, "Invalid theme from config!\n");
		theme = NEKO_THEME_DARK;
	}

	signal(SIGPIPE, SIG_IGN);

	while ((opt = getopt(argc, argv, "b:f:hr:t:")) != -1) {
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
				fprintf(stderr, "Invalid rotation '%s'\n", optarg);
				print_help(argv[0]);
				exit(1);
			}
		break;
		case 't':
			theme = str_to_theme(optarg);
			if (theme == NEKO_THEME_INVALID) {
				fprintf(stderr, "Invalid theme '%s'\n", optarg);
				print_help(argv[0]);
				exit(1);
			}
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

	neko_ctx_init(backend, theme, cfg.font_family);

	neko_logo_render(backend->pixmap, &neko_logo_text, 0);
	gp_backend_flip(backend);
	sleep(1);

	gp_size w = gp_pixmap_w(backend->pixmap);
	gp_size h = gp_pixmap_h(backend->pixmap);

	neko_load_keybindings();

	unsigned int i;

	for (i = 0; i < NEKO_MAIN_VIEWS; i++) {
		char name[16];

		snprintf(name, sizeof(name), "View%02i", i);
		neko_view_init(&main_views[i], 0, 0, w, h, name);
		main_views[i].slot_exit = slot_exit_running_apps;
	}

	main_views[0].slot = neko_app_launcher_init();

	neko_subviews_init(&left_view, &right_view, &main_views[3], NEKO_VIEW_SPLIT_VERT);

	neko_view_slot_put(&left_view, neko_running_apps_init());
	left_view.slot_exit = slot_exit_running_apps;
	neko_view_slot_put(&right_view, neko_running_apps_init());
	right_view.slot_exit = slot_exit_running_apps;

	neko_subviews_init(&top_view, &bottom_view, &main_views[4], NEKO_VIEW_SPLIT_HORIZ);

	neko_view_slot_put(&top_view, neko_running_apps_init());
	top_view.slot_exit = slot_exit_running_apps;

	neko_subviews_init(&bottom_left_view, &bottom_right_view, &bottom_view, NEKO_VIEW_SPLIT_VERT);

	neko_view_slot_put(&bottom_left_view, neko_running_apps_init());
	bottom_left_view.slot_exit = slot_exit_running_apps;

	neko_view_slot_put(&bottom_right_view, neko_running_apps_init());
	bottom_right_view.slot_exit = slot_exit_running_apps;

	neko_view_slot_put(&main_views[1], neko_running_apps_init());
	neko_view_slot_put(&main_views[2], neko_running_apps_init());

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
