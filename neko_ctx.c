//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2023 Cyril Hrubis <metan@ucw.cz>

 */

#include <core/gp_core.h>
#include <backends/gp_backend.h>
#include "neko_ctx.h"

struct neko_ctx ctx;
static gp_text_style style = GP_DEFAULT_TEXT_STYLE;

void neko_ctx_init(gp_backend *backend, const char *font_family)
{
	const gp_font_family *family = gp_font_family_lookup(font_family);

	style.font = gp_font_family_face_lookup(family, GP_FONT_REGULAR);
	if (!style.font)
		style.font = gp_font_family_face_lookup(family, GP_FONT_MONO);

	if (style.font)
		ctx.font = &style;

	ctx.backend = backend;
	ctx.padd = gp_text_descent(ctx.font)+1;

	ctx.col_bg = gp_rgb_to_pixmap_pixel(0, 0, 0, backend->pixmap);
	ctx.col_fg = gp_rgb_to_pixmap_pixel(0xff, 0xff, 0xff, backend->pixmap);
	ctx.col_sel = gp_rgb_to_pixmap_pixel(0, 0x20, 0x60, backend->pixmap);
}
