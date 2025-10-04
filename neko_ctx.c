//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2025 Cyril Hrubis <metan@ucw.cz>

 */

#include <core/gp_core.h>
#include <backends/gp_backend.h>
#include "neko_ctx.h"

struct neko_ctx ctx;
static gp_text_style style = GP_DEFAULT_TEXT_STYLE;
static gp_text_style style_bold = GP_DEFAULT_TEXT_STYLE;

void neko_ctx_init(gp_backend *backend, enum neko_theme theme, const char *font_family)
{
	const gp_font_family *family = gp_font_family_lookup(font_family);

	style.font = gp_font_family_face_lookup(family, GP_FONT_REGULAR);
	if (!style.font)
		style.font = gp_font_family_face_lookup(family, GP_FONT_MONO);

	if (style.font)
		ctx.font = &style;

	style_bold.font = gp_font_family_face_lookup(family, GP_FONT_REGULAR | GP_FONT_BOLD);
	if (!style_bold.font)
		style_bold.font = gp_font_family_face_lookup(family, GP_FONT_MONO | GP_FONT_BOLD);

	if (!style_bold.font) {
		style_bold.font = gp_font_family_face_lookup(family, GP_FONT_MONO);
		gp_text_style_embold(&style_bold, style.font, 1);
	}

	ctx.font_bold = &style_bold;

	ctx.backend = backend;
	ctx.padd = gp_text_descent(ctx.font)+1;

	ctx.col_bg = gp_rgb_to_pixmap_pixel(0, 0, 0, backend->pixmap);
	ctx.col_fg = gp_rgb_to_pixmap_pixel(0xff, 0xff, 0xff, backend->pixmap);

	ctx.theme = theme;

	if (ctx.theme == NEKO_THEME_LIGHT)
		GP_SWAP(ctx.col_bg, ctx.col_fg);

	switch (gp_pixel_size(backend->pixmap->pixel_type)) {
	case 1:
		ctx.col_sel = ctx.col_bg;
		ctx.col_fout_bg = ctx.col_bg;
		ctx.col_fin_bg = ctx.col_bg;
		ctx.col_fin_fr = ctx.col_fg;
		ctx.col_fout_fr = gp_pixel_pattern(1, 0, GP_PIXEL_PATTERN_50);
	break;
	case 2:
		if (ctx.theme == NEKO_THEME_DARK) {
			ctx.col_sel = gp_rgb_to_pixmap_pixel(0x40, 0x40, 0x40, backend->pixmap);
			ctx.col_fout_fr = gp_rgb_to_pixmap_pixel(0x80, 0x80, 0x80, backend->pixmap);
		} else {
			ctx.col_sel = gp_rgb_to_pixmap_pixel(0x80, 0x80, 0x80, backend->pixmap);
			ctx.col_fout_fr = gp_rgb_to_pixmap_pixel(0x40, 0x40, 0x40, backend->pixmap);
		}

		ctx.col_fout_bg = ctx.col_bg;
		ctx.col_fin_bg = ctx.col_sel;
		ctx.col_fin_fr = ctx.col_fg;
	break;
	default:
		if (ctx.theme == NEKO_THEME_LIGHT) {
			ctx.col_sel = gp_rgb_to_pixmap_pixel(0x6f, 0xa5, 0xd4, backend->pixmap);
			ctx.col_fout_bg = gp_rgb_to_pixmap_pixel(0xcc, 0xcc, 0xcc, backend->pixmap);
			ctx.col_fin_fr = gp_rgb_to_pixmap_pixel(0x4f, 0x85, 0xb4, backend->pixmap);
		} else {
			ctx.col_sel = gp_rgb_to_pixmap_pixel(0x22, 0x2b, 0x3f, backend->pixmap);
			ctx.col_fout_bg = gp_rgb_to_pixmap_pixel(0x1f, 0x1f, 0x1f, backend->pixmap);
			ctx.col_fin_fr = gp_rgb_to_pixmap_pixel(0x32, 0x32, 0x4a, backend->pixmap);
		}

		ctx.col_fin_bg = ctx.col_sel;
		ctx.col_fout_fr = ctx.col_fout_bg;
	break;
	}


}
