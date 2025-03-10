//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfx/gp_gfx.h>
#include <core/gp_debug.h>
#include <backends/gp_backend.h>

#include "neko_keybindings.h"
#include "neko_ctx.h"
#include "neko_view.h"

void neko_view_update_rect(neko_view *self, gp_coord x, gp_coord y, gp_size w, gp_size h)
{
	if (self->x + x + w > self->w) {
		GP_WARN("w = %u > self->w %u", w, self->w);
		w = self->w - self->x - x;
	}

	if (self->y + y + h > self->h) {
		GP_WARN("h = %u > self->h %u", h, self->h);
		h = self->h - self->y - y;
	}

	gp_backend_update_rect_xywh(ctx.backend, self->x + x, self->y + y, w, h);
}

void neko_view_flip(neko_view *self)
{
	gp_backend_update_rect_xywh(ctx.backend, self->x, self->y, self->w, self->h);
}

static void empty_view(neko_view *self)
{
	gp_pixmap *pixmap = neko_view_pixmap(self);

	gp_fill(pixmap, ctx.col_bg);
	gp_rect_xywh(pixmap, 0, 0, self->w, self->h, ctx.col_fg);
	gp_text(pixmap, ctx.font, self->w/2, self->h/2,
	        GP_ALIGN_CENTER | GP_VALIGN_CENTER,
	        ctx.col_fg, ctx.col_bg, "EMPTY");
	neko_view_flip(self);
}

static void split_horiz(neko_view *self)
{
	if (self->subviews[0]) {
		self->subviews[0]->x = self->x;
		self->subviews[0]->y = self->y;
		neko_view_resize(self->subviews[0], self->w, self->h/2);
	}

	if (self->subviews[1]) {
		self->subviews[0]->x = self->x;
		self->subviews[0]->y = self->y + self->h/2;
		neko_view_resize(self->subviews[1], self->w, self->h/2);
	}
}

static void split_vert(neko_view *self)
{
	if (self->subviews[0]) {
		self->subviews[0]->x = self->x;
		self->subviews[0]->y = self->y;
		neko_view_resize(self->subviews[0], self->w/2, self->h);
	}

	if (self->subviews[1]) {
		self->subviews[1]->x = self->x + self->w/2;
		self->subviews[0]->y = self->y;
		neko_view_resize(self->subviews[1], self->w/2, self->h);
	}
}

void neko_view_resize(neko_view *self, gp_size w, gp_size h)
{
	self->w = w;
	self->h = h;

	if (self->slot && self->slot->ops->resize)
		self->slot->ops->resize(self);

	switch (self->split_mode) {
	case NEKO_VIEW_SPLIT_HORIZ:
		split_horiz(self);
	break;
	case NEKO_VIEW_SPLIT_VERT:
		split_vert(self);
	break;
	}
}

void neko_view_repaint(neko_view *self)
{
	if (self->slot) {
		if (self->slot->ops->repaint)
			self->slot->ops->repaint(self);
		return;
	}

	if (self->subviews[0])
		neko_view_repaint(self->subviews[0]);
	else
		empty_view(self);

	if (self->subviews[1])
		neko_view_repaint(self->subviews[1]);
	else
		empty_view(self);
}

void neko_view_init(neko_view *self,
                    gp_size x, gp_size y, gp_size w, gp_size h)
{
	self->x = x;
	self->y = y;
	self->w = w;
	self->h = h;

	self->parent = NULL;

	empty_view(self);
}

void neko_subviews_init(neko_view *left, neko_view *right, neko_view *parent)
{
	left->parent = parent;
	parent->subviews[0] = left;

	right->parent = parent;
	parent->subviews[1] = right;

	parent->split_mode = NEKO_VIEW_SPLIT_VERT;
	split_vert(parent);

	empty_view(left);
	empty_view(right);
}

gp_pixmap *neko_view_pixmap(neko_view *self)
{
	gp_sub_pixmap(ctx.backend->pixmap, &self->buf, self->x, self->y, self->w, self->h);

	return &self->buf;
}

static void view_slot_rem(neko_view *self)
{
	if (!self)
		return;

	if (self->slot && self->slot->ops->remove) {
		GP_DEBUG(1, "Removing view %p slot %p", self, self->slot);
		self->slot->ops->remove(self);
	}

	if (self->slot)
		self->slot->view = NULL;

	self->slot = NULL;
}

void neko_view_slot_put(neko_view *self, neko_view_slot *slot)
{
	view_slot_rem(self);

	self->slot = slot;

	if (slot) {
		neko_view *view = slot->view;
		view_slot_rem(view);
		neko_view_slot_exit(view);

		slot->view = self;

		if (neko_view_is_shown(self) && slot->ops->show) {
			GP_DEBUG(1, "Showing view %p slot %p", self, self->slot);
			slot->ops->show(self);
		}
	}
}

neko_view *neko_view_focused_child(neko_view *view)
{
	return view->subviews[view->focused_subview];
}

int neko_view_is_focused(neko_view *view)
{
	/* root view is always focused */
	if (!view->parent)
		return 1;

	return neko_view_focused_child(view->parent) == view;
}

static int try_switch_focus(neko_view *view)
{
	if (view->subviews[0] && view->subviews[1]) {
		view->focused_subview = !view->focused_subview;
		neko_view_repaint(view);
		return 1;
	}

	return 0;
}

static int cursor_in_view(neko_view *view, gp_event *ev)
{
	if (!view)
		return 0;

	if (ev->st->cursor_x >= view->x &&
	    ev->st->cursor_x < view->x + view->w &&
	    ev->st->cursor_y >= view->y &&
	    ev->st->cursor_y < view->y + view->h)
		return 1;

	return 0;
}

void neko_view_show(neko_view *self)
{
	if (!self)
		return;

	GP_DEBUG(1, "Showing view %p parent %p", self, self->parent);

	self->is_shown = 1;

	if (self->slot) {
		if (self->slot->ops->show)
			self->slot->ops->show(self);
		return;
	}

	neko_view_show(self->subviews[0]);
	neko_view_show(self->subviews[1]);

	if (!self->subviews[0])
		empty_view(self);
}

void neko_view_hide(neko_view *self)
{
	if (!self)
		return;

	GP_DEBUG(1, "Hiding view %p parent %p", self, self->parent);

	self->is_shown = 0;

	if (self->slot) {
		if (self->slot->ops->hide)
			self->slot->ops->hide(self);
		return;
	}

	neko_view_hide(self->subviews[0]);
	neko_view_hide(self->subviews[1]);
}

void neko_view_event(neko_view *self, gp_event *ev)
{
	switch (ev->type) {
	case GP_EV_KEY:
		if (!gp_ev_any_key_pressed(ev, NEKO_KEYS_MOD_WM))
			break;

		if (ev->code != GP_EV_KEY_DOWN)
			break;

		switch (ev->val) {
		case NEKO_KEYS_SWITCH_FOCUS:
			if (try_switch_focus(self))
				return;
		break;
		}
	break;
	case GP_EV_REL:
		if (ev->code == GP_EV_REL_POS) {
			if (cursor_in_view(self->subviews[0], ev)) {
				self->focused_subview = 0;
				neko_view_repaint(self);
			}

			if (cursor_in_view(self->subviews[1], ev)) {
				self->focused_subview = 1;
				neko_view_repaint(self);
			}
		}
	break;
	}

	if (self->slot) {
		self->slot->ops->event(self, ev);
		return;
	}

	if (self->subviews[self->focused_subview])
		neko_view_event(self->subviews[self->focused_subview], ev);
}
