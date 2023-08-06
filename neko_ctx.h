//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2023 Cyril Hrubis <metan@ucw.cz>

 */

/*
 * Configuration.
 */

#ifndef NEKO_CTX__
#define NEKO_CTX__

#include <text/gp_text.h>
#include <core/gp_types.h>
#include <backends/gp_types.h>

struct neko_ctx {
	gp_pixel col_fg;
	gp_pixel col_bg;

	gp_size padd;

	gp_text_style *font;

	gp_backend *backend;
};

extern struct neko_ctx ctx;

void neko_ctx_init(gp_backend *backend, const char *font_family);

#endif /* NEKO_CTX__ */
