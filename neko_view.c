//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2023 Cyril Hrubis <metan@ucw.cz>

 */

#include <core/gp_debug.h>
#include <backends/gp_backend.h>
#include "neko_view.h"

void neko_view_update_rect(neko_view *self, gp_coord x, gp_coord y, gp_size w, gp_size h)
{
	if (x + w > self->w)
		GP_WARN("w = %u > self->w %u", w, self->w);

	if (y + h > self->h)
		GP_WARN("h = %u > self->h %u", h, self->h);

	gp_backend_update_rect_xywh(self->backend, self->x + x, self->y + y, w, h);
}

void neko_view_flip(neko_view *self)
{
	gp_backend_update_rect_xywh(self->backend, self->x, self->y, self->w, self->h);

	gp_sub_pixmap(self->backend->pixmap, &self->pixmap, self->x, self->y, self->w, self->h);
}

void neko_view_resize(neko_view *self, gp_size w, gp_size h)
{
	self->w = w;
	self->h = h;

	gp_sub_pixmap(self->backend->pixmap, &self->pixmap, self->x, self->y, self->w, self->h);

	if (self->child && self->child->ops->resize)
		self->child->ops->resize(self);
}

void neko_view_init(neko_view *self, gp_backend *backend,
                    gp_size x, gp_size y, gp_size w, gp_size h)
{
	gp_sub_pixmap(backend->pixmap, &self->pixmap, x, y, w, h);

	self->backend = backend;
	self->x = x;
	self->y = y;
	self->w = w;
	self->h = h;
}

void neko_view_show_child(neko_view *self, neko_view_child *child)
{
	if (self->child && self->child->ops->hide) {
		GP_DEBUG(1, "Hiding view %p child %p", self, self->child);
		self->child->ops->hide(self);
	}

	if (self->child)
		self->child->parent = NULL;

	self->child = child;

	if (child)
		child->parent = self;

	if (self->child && self->child->ops->show) {
		GP_DEBUG(1, "Showing view %p child %p", self, self->child);
		self->child->ops->show(self);
	}
}
