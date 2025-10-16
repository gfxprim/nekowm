//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2025 Cyril Hrubis <metan@ucw.cz>

 */

#include <errno.h>
#include <string.h>

#include <core/gp_common.h>
#include <core/gp_debug.h>
#include <utils/gp_user_path.h>
#include <utils/gp_json.h>
#include <input/gp_keys.h>

#include "neko_app_launcher.h"
#include "neko_keybindings.h"

struct neko_keybinding neko_keybindings[] = {
	[NEKO_KEYS_MOD_WM_IDX] = NEKO_KEYS_MOD_WM_DEF,
	[NEKO_KEYS_QUIT_IDX] = NEKO_KEYS_QUIT_DEF,
	[NEKO_KEYS_EXIT_IDX] = NEKO_KEYS_EXIT_DEF,
	[NEKO_KEYS_FORCE_IDX] = NEKO_KEYS_FORCE_DEF,
	[NEKO_KEYS_LIST_APPS_IDX] = NEKO_KEYS_LIST_APPS_DEF,
	[NEKO_KEYS_APP_LAUNCHER_IDX] = NEKO_KEYS_APP_LAUNCHER_DEF,
	[NEKO_KEYS_SWITCH_FOCUS_IDX] = NEKO_KEYS_SWITCH_FOCUS_DEF,
	[NEKO_KEYS_VIRT_SCREENS_LEFT_IDX] = NEKO_KEYS_VIRT_SCREENS_LEFT_DEF,
	[NEKO_KEYS_VIRT_SCREENS_RIGHT_IDX] = NEKO_KEYS_VIRT_SCREENS_RIGHT_DEF,
	[NEKO_KEYS_ROTATE_IDX] = NEKO_KEYS_ROTATE_DEF,
	[NEKO_KEYS_HELP_IDX] = NEKO_KEYS_HELP_DEF,
	[NEKO_KEYS_POWEROFF_IDX] = NEKO_KEYS_POWEROFF_DEF,
};

static void set_keybinding(gp_json_reader *json, const char *key_name, const char *key_val)
{
	size_t i;
	size_t keybindings_cnt = GP_ARRAY_SIZE(neko_keybindings);

	for (i = 0; i < keybindings_cnt; i++) {
		if (!strcmp(key_name, neko_keybindings[i].name))
			break;
	}

	if (i >= keybindings_cnt) {
		gp_json_warn(json, "Invalid keybinding name");
		return;
	}

	int key = gp_ev_key_val(key_val);
	if (key < 0) {
		gp_json_warn(json, "Invalid key name");
		return;
	}

	GP_DEBUG(2, "Setting keybinding '%s' -> '%s'", key_name, key_val);

	neko_keybindings[i].key = key;
}

enum run_type {
	RUN_APP,
	RUN_CMD,
};

#define RUN_MAX 16

struct run {
	union {
		char app_name[32];
		char *cmdline;
	};
	enum run_type type;
	uint32_t key;
};

static struct run apps[RUN_MAX] = {
	[0] = {.app_name = "Termini", .type = RUN_APP, .key = GP_KEY_ENTER}
};
static unsigned int apps_cnt = 1;

int neko_process_keybindings(uint32_t key)
{
	unsigned int i;

	for (i = 0; i < apps_cnt; i++) {
		if (apps[i].key == key) {
			switch (apps[i].type) {
			case RUN_APP:
				neko_app_run(apps[i].app_name);
				return 1;
			case RUN_CMD:
				neko_cmd_run(apps[i].cmdline);
				return 1;
			default:
			break;
			}
		}
	}

	return 0;
}

static void parse_run_app(gp_json_reader *json, gp_json_val *val, enum run_type type)
{
	struct run *app;
	int key_set = 0;
	int run_set = 0;

	if (apps_cnt >= RUN_MAX) {
		gp_json_warn(json, "Too many apps and cmdlines defined!");
		gp_json_obj_skip(json);
		return;
	}

	app = apps + apps_cnt;

	memset(app, 0, sizeof(*app));

	app->type = type;

	GP_JSON_OBJ_FOREACH(json, val) {
		if (val->type != GP_JSON_STR) {
			gp_json_err(json, "Invalid value type, expected string.");
			return;
		}

		if (app->type == RUN_APP && !strcmp(val->id, "App_Name")) {
			if (strlen(val->val_str) + 1 >= sizeof(app->app_name)) {
				gp_json_err(json, "App name too long");
				return;
			}

			strcpy(app->app_name, val->val_str);
			run_set = 1;
		} else if (!strcmp(val->id, "Key")) {
			int key = gp_ev_key_val(val->val_str);
			if (key < 0) {
				gp_json_err(json, "Invalid key name");
				return;
			}
			app->key = key;
			key_set = 1;
		} else if (app->type == RUN_CMD && !strcmp(val->id, "Cmdline")) {
			app->cmdline = strdup(val->val_str);
			if (!app->cmdline) {
				gp_json_warn(json, "strdup() failed");
				return;
			}
			run_set = 1;
		} else {
			gp_json_err(json, "Wrong key, expected either 'App_Name' or 'Key'");
			return;
		}
	}

	if (!key_set || !run_set) {
		gp_json_warn(json, "Incomplete App or Cmdline!");
		if (app->type == RUN_CMD)
			free(app->cmdline);
		return;
	}

	switch (app->type) {
	case RUN_APP:
		GP_DEBUG(1, "App '%s' keybinding is Mod_WM+%s",
		         app->app_name, gp_ev_key_name(app->key));
	break;
	case RUN_CMD:
		GP_DEBUG(1, "Cmdline '%s' keybinding is Mod_WM+%s",
		         app->cmdline, gp_ev_key_name(app->key));
	break;
	}

	apps_cnt++;
}

void neko_load_keybindings(void)
{
	char *path = gp_user_path(".config/nekowm/", "keybindings.json");
	gp_json_reader *json;
	char buf[128];
	struct gp_json_val val = {
		.buf = buf,
		.buf_size = sizeof(buf),
	};

	if (!path) {
		GP_WARN("Failed to construct path to a config file!");
		return;
	}

	json = gp_json_reader_load(path);
	if (!json) {
		GP_DEBUG(1, "Failed to open '%s': %s", path, strerror(errno));
		return;
	}

	GP_DEBUG(1, "Loading keybindings from '%s'", path);

	GP_JSON_OBJ_FOREACH(json, &val) {
		if (val.type == GP_JSON_OBJ && !strcmp(val.id, "Run_App")) {
			parse_run_app(json, &val, RUN_APP);
			continue;
		}

		if (val.type == GP_JSON_OBJ && !strcmp(val.id, "Run_Cmd")) {
			parse_run_app(json, &val, RUN_CMD);
			continue;
		}

		if (val.type != GP_JSON_STR) {
			gp_json_err(json, "Invalid value type, expected string.");
			goto err;
		}

		set_keybinding(json, val.id, val.val_str);
	}

err:
	gp_json_reader_finish(json);
	gp_json_reader_free(json);
	free(path);
}
