//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

/**
 * @brief Global context, font, colors, backend pointer.
 * @file neko_ctx.h
 */

#ifndef NEKO_CTX_H
#define NEKO_CTX_H

#include <text/gp_text.h>
#include <core/gp_types.h>
#include <backends/gp_types.h>

struct neko_ctx {
	gp_pixel col_fg;
	gp_pixel col_bg;
	gp_pixel col_sel;

	gp_size padd;

	gp_text_style *font;

	gp_backend *backend;
};

extern struct neko_ctx ctx;

void neko_ctx_init(gp_backend *backend, int reverse, const char *font_family);

#endif /* NEKO_CTX_H */
