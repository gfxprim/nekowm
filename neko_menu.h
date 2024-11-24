//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

/**
 * @brief An on screen menu.
 * @file neko_menu.h
 */

#ifndef NEKO_MENU_H
#define NEKO_MENU_H

struct neko_menu {
	void (*draw_entry)(size_t index, gp_pixmap *pixmap, gp_pixel fg, gp_pixel bg,
			   gp_coord x, gp_coord y, gp_size w, gp_size h);
	size_t items_cnt;
	size_t items_offset;
	size_t item_sel;
	unsigned int focused:1;
	gp_size entry_h;
	char *heading;
};

void neko_menu_repaint(struct neko_menu *menu, gp_pixmap *pixmap);

#endif /* NEKO_MENU_H */
