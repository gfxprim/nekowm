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

/**
 * @brief A global context.
 */
struct neko_ctx {
	/**
	 * @brief A foreground color.
	 */
	gp_pixel col_fg;
	/**
	 * @brief A background color.
	 */
	gp_pixel col_bg;
	/**
	 * @brief A selected color.
	 */
	gp_pixel col_sel;

	uint32_t dark_theme:1;

	gp_size padd;

	/**
	 * @brief Current font.
	 */
	gp_text_style *font;

	/**
	 * @brief Current bold font.
	 */
	gp_text_style *font_bold;

	/**
	 * @brief Pointer to the current backend.
	 */
	gp_backend *backend;
};

extern struct neko_ctx ctx;

void neko_ctx_init(gp_backend *backend, int reverse, const char *font_family);

#endif /* NEKO_CTX_H */
