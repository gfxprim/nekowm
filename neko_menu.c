//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

#include <core/gp_common.h>
#include <gfx/gp_gfx.h>

#include "neko_ctx.h"
#include "neko_menu.h"

static size_t min_offset(size_t item_sel, gp_size avail_h, gp_size entry_h)
{
	size_t shown_entries = avail_h/entry_h;

	if (item_sel < shown_entries)
		return 0;

	return item_sel - shown_entries;
}

void neko_menu_repaint(struct neko_menu *menu, gp_pixmap *pixmap)
{
	gp_coord cur_y = ctx.padd;
	gp_size w = pixmap->w;
	gp_size h = pixmap->h;
	gp_size p = ctx.padd;
	gp_size p2 = 2 * ctx.padd;
	gp_size ta = gp_text_ascent(ctx.font);
	gp_size eh = menu->entry_h + p2;
	gp_coord last_y = h - ta - ctx.padd - eh;

	gp_fill(pixmap, ctx.col_bg);

	if (menu->heading) {
		gp_text_style *font = menu->focused ? ctx.font_bold : ctx.font;

		if (menu->focused)
			gp_fill_rect_xywh(pixmap, 0, 0, w, 2*ctx.padd + ta, ctx.col_sel);

		gp_print(pixmap, font, w/2, cur_y, GP_ALIGN_CENTER|GP_VALIGN_BOTTOM,
		         ctx.col_fg, ctx.col_bg, "\u00ab %s \u00bb", menu->heading);

		cur_y += 2 * ctx.padd + ta;
		gp_hline_xyw(pixmap, 0, cur_y-ctx.padd, w, ctx.col_fg);
	}

	gp_rect_xywh(pixmap, 0, 0, w, h, ctx.col_fg);

	menu->items_offset = GP_MAX(menu->items_offset, min_offset(menu->item_sel, last_y - cur_y - ta - ctx.padd, eh));
	menu->items_offset = GP_MIN(menu->items_offset, menu->item_sel);

	size_t idx = menu->items_offset;

	if (idx)
		gp_symbol(pixmap, w/2, cur_y+ctx.padd, ta/2, ta/2, GP_TRIANGLE_UP, ctx.col_fg);

	cur_y += ta + ctx.padd;

	for (;;) {
		if (cur_y >= last_y)
			break;

		if (idx >= menu->items_cnt)
			break;

		gp_pixel fg = ctx.col_fg;
		gp_pixel bg = ctx.col_bg;

		if (idx == menu->item_sel) {
		//	gp_symbol(pixmap, p, cur_y + eh/2, p/2, p/2, GP_TRIANGLE_LEFT, ctx.col_fg);
		//	gp_symbol(pixmap, w-p, cur_y + eh/2, p/2, p/2, GP_TRIANGLE_RIGHT, ctx.col_fg);
			bg = ctx.col_sel;
			gp_fill_rect_xywh(pixmap, p, cur_y, w-2*p, eh, bg);
			gp_rect_xywh(pixmap, p, cur_y, w-2*p, eh, fg);
		}

		menu->draw_entry(idx, pixmap, fg, bg, p2, cur_y+p, w-2*p2, eh);

		cur_y += eh;
		idx++;
	}

	if (idx < menu->items_cnt)
		gp_symbol(pixmap, w/2, h - ta, ta/2, ta/2, GP_TRIANGLE_DOWN, ctx.col_fg);
}
