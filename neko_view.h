//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2023 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef NEKO_VIEW
#define NEKO_VIEW

#include <core/gp_core.h>
#include <backends/gp_types.h>

typedef struct neko_view {
	gp_size x;
	gp_size y;
	gp_size w;
	gp_size h;
	gp_pixmap pixmap;
	gp_backend *backend;
} neko_view;

void neko_view_init(neko_view *self, gp_backend *backend,
                    gp_size x, gp_size y, gp_size w, gp_size h);

void neko_view_update_rect(neko_view *self, gp_size w, gp_size h);

void neko_view_flip(neko_view *self);

void neko_view_resize(neko_view *self, gp_size w, gp_size h);

static inline gp_pixmap *neko_view_pixmap(neko_view *self)
{
	return &(self->pixmap);
}

#endif /* NEKO_VIEW */
