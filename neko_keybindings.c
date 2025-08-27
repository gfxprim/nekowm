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
