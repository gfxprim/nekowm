//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

#include <core/gp_core.h>
#include <backends/gp_backend.h>
#include "neko_ctx.h"

struct neko_ctx ctx;
static gp_text_style style = GP_DEFAULT_TEXT_STYLE;
static gp_text_style style_bold = GP_DEFAULT_TEXT_STYLE;

void neko_ctx_init(gp_backend *backend, int reverse, const char *font_family)
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

	if (reverse)
		GP_SWAP(ctx.col_bg, ctx.col_fg);

	ctx.dark_theme = !reverse;

	if (reverse)
		ctx.col_sel = gp_rgb_to_pixmap_pixel(0xa0, 0xc0, 0xff, backend->pixmap);
	else
		ctx.col_sel = gp_rgb_to_pixmap_pixel(0, 0x20, 0x60, backend->pixmap);
}
