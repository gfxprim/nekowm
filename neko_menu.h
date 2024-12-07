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

/**
 * @brief A neko menu description.
 */
struct neko_menu {
	/**
	 * @brief Callback to draw a menu entry.
	 *
	 * @param index The index of the entry to render.
	 * @param pixmap A pixmap to render the entry into.
	 * @param fg A foreground color.
	 * @param bg A background color.
	 * @param x A x offset to start the rendering at.
	 * @param y A y offset to start the rendering at.
	 * @param w The width of the menu item.
	 * @param h The height of the menu intem.
	 */
	void (*draw_entry)(size_t index, gp_pixmap *pixmap, gp_pixel fg, gp_pixel bg,
			   gp_coord x, gp_coord y, gp_size w, gp_size h);
	/**
	 * @brief Number of items in the menu.
	 */
	size_t items_cnt;
	/**
	 * @brief Number of items to skip.
	 *
	 * This is modified by the menu code in order to fit the selected item
	 * into the view.
	 */
	size_t items_offset;
	/**
	 * @brief Currently selected item.
	 */
	size_t item_sel;
	/**
	 * @brief I set the menu is focused.
	 *
	 * This changes the visual style.
	 */
	unsigned int focused:1;
	/**
	 * @brief The height for the menu entry redered by the draw entry callback.
	 */
	gp_size entry_h;
	/**
	 * @brief Menu heading.
	 */
	char *heading;
};

/**
 * @brief Repaints the menu.
 *
 * The menu takes the whole pixmap.
 *
 * @param menu A menu description.
 * @param pixmap A pixmap to draw the menu into.
 */
void neko_menu_repaint(struct neko_menu *menu, gp_pixmap *pixmap);

#endif /* NEKO_MENU_H */
