//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

/**
 * @brief View is an abstraction for a piece of a screen.
 */
#ifndef NEKO_VIEW
#define NEKO_VIEW

#include <core/gp_core.h>
#include <backends/gp_types.h>

struct neko_view;
struct neko_view_child;

typedef struct neko_view_child_ops {
	/** @brief Request child exit. */
	void (*exit)(struct neko_view_child *self);

	/** @brief A child input event. */
	void (*event)(struct neko_view *self, gp_event *ev);

	/** @brief Show child, called after child has been added to the view. */
	void (*show)(struct neko_view *self);

	/** @brief Hide child, called before child is removed from the view. */
	void (*hide)(struct neko_view *self);

	/** @brief Request (partial) child repaint. */
	void (*update_rect)(struct neko_view *self,
	                    gp_size x, gp_size y,
	                    gp_size w, gp_size h);

	/** @brief Request full child repaint. */
	void (*flip)(struct neko_view *self);

	/** @brief Resize the child because the view was resized. */
	void (*resize)(struct neko_view *self);
} neko_view_child_ops;

typedef struct neko_view_child {
	/** @brief A neko view implementation. */
	const neko_view_child_ops *ops;

	/** @brief Set to a parent view if child is shown on a screen. */
	//TODO: Rename to view
	struct neko_view *parent;

	/** @brief A list head, used for grouping children into lists. */
	gp_dlist_head list;

	char priv[];
} neko_view_child;

/** @brief View a part of a screen. */
typedef struct neko_view {
	gp_size x;
	gp_size y;
	gp_size w;
	gp_size h;
	gp_pixmap pixmap;
	gp_backend *backend;

	/** @brief Called by child when it did exit. */
	void (*child_exit)(struct neko_view *self);

	/** @brief What is shown in the view. */
	neko_view_child *child;
} neko_view;

void neko_view_init(neko_view *self, gp_backend *backend,
                    gp_size x, gp_size y, gp_size w, gp_size h);

/**
 * @brief Child did exit.
 *
 * This is called by a child when it did exit. The view should replace the
 * child with a different one (if possible) in the handler.
 *
 * @param self A neko view.
 */
static inline void neko_view_child_exit(neko_view *self)
{
	if (!self)
		return;

	if (!self->child_exit) {
		self->child = NULL;
		return;
	}

	self->child = NULL;

	self->child_exit(self);
}

/**
 * @brief Update rectangle in the view on the screen.
 *
 * This is called by the child when content needs to be updated from the view
 * pixmap and painted on the screen.
 *
 * @param self A neko view.
 */
void neko_view_update_rect(neko_view *self, gp_coord x, gp_coord y, gp_size w, gp_size h);

/**
 * @brief Update whole view on the screen.
 *
 * This is called by the child when content needs to be updated from the view
 * pixmap and painted on the screen.
 *
 * @param self A neko view.
 */
void neko_view_flip(neko_view *self);

/**
 * @brief Resizes the view and the view child (if any).
 *
 * @param self A neko view.
 * @param w New width.
 * @param h New height.
 */
void neko_view_resize(neko_view *self, gp_size w, gp_size h);

/**
 * @brief Sets new child to a neko view.
 *
 * Sets a new view child, possibly replaces currently shown child. If child is
 * NULL the view becomes empty.
 *
 * @param self A neko view.
 * @param child A new neko view child to be shonw in the view.
 */
void neko_view_show_child(neko_view *self, neko_view_child *child);

/**
 * @brief Sends an input event to a view child.
 *
 * @param self A neko view.
 * @param ev An input event.
 */
static inline void neko_view_event(neko_view *self, gp_event *ev)
{
	if (!self->child)
		return;

	if (!self->child->ops->event)
		return;

	self->child->ops->event(self, ev);
}

static inline gp_pixmap *neko_view_pixmap(neko_view *self)
{
	return &(self->pixmap);
}

#endif /* NEKO_VIEW */
