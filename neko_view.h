//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

/**
 * @brief View is an abstraction for a piece of a screen.
 * @file neko_view.h
 *
 * View can be divided into subviews recursively.
 *
 * Each view has a slot, if slot is not empty the screen is taken over by the
 * slot content.
 */
#ifndef NEKO_VIEW_H
#define NEKO_VIEW_H

#include <core/gp_core.h>
#include <backends/gp_types.h>

struct neko_view;
struct neko_view_slot;

/**
 * @brief A view slot ops.
 *
 * Callback to render a slot content.
 */
typedef struct neko_view_slot_ops {
	/** @brief Request child exit. */
	void (*exit)(struct neko_view_slot *self);

	/** @brief A child input event. */
	void (*event)(struct neko_view *self, gp_event *ev);

	/**
	 * @brief Show a view slot.
	 *
	 * This is called in order to start a slot rendering because the view
	 * the slot is inserted into has became visible on the screen.
	 *
	 * It's also called when a slot is inserted into a view that is already
	 * visible on the screen.
	 */
	void (*show)(struct neko_view *self);

	/**
	 * @brief Hide a view slot.
	 *
	 * This is called in order to stop a slot rendering because the view
	 * the slot is inserted into is not visible on the screen anymore.
	 */
	void (*hide)(struct neko_view *self);

	/**
	 * @brief Callback for a slot removal.
	 *
	 * This is called before a slot content is removed from a view.
	 *
	 * If you free() the slot in this function you also need to set the
	 * view->slot pointer to NULL.
	 */
	void (*remove)(struct neko_view *self);

	/** @brief Request full repaint. */
	void (*repaint)(struct neko_view *self);

	/** @brief Resize the child because the view was resized. */
	void (*resize)(struct neko_view *self);
} neko_view_slot_ops;

/**
 * @brief A view slot content.
 *
 * Something to be shown in the view on the screen.
 */
typedef struct neko_view_slot {
	/** @brief A neko view child implementation. */
	const neko_view_slot_ops *ops;

	/** @brief Set to a parent if slot is inserted into a view. */
	struct neko_view *view;

	/** @brief A list head, used for grouping children into lists. */
	gp_dlist_head list;

	char priv[];
} neko_view_slot;

/**
 * @brief View a part of a screen.
 */
typedef struct neko_view {
	/** @brief A x offset on the backend pixmap. */
	gp_size x;
	/** @brief A y offset on the backend pixmap. */
	gp_size y;
	/** @brief A width on the backend pixmap. */
	gp_size w;
	/** @brief A height on the backend pixmap. */
	gp_size h;

	//TODO: Remove?
	gp_pixmap buf;

	/** @brief Called by child when it did exit. */
	//TODO Move to ops?
	void (*slot_exit)(struct neko_view *self);

	/** @brief A view parent, NULL for top level view. */
	struct neko_view *parent;

	/** @brief Set if view is shown on a screen. */
	unsigned int is_shown:1;

	/** @brief A view may be split into two subviews. */
	struct neko_view *subviews[2];
	/** @brief Currently focused subview. */
	unsigned int focused_subview;

	/** @brief What is shown in the view. */
	neko_view_slot *slot;
} neko_view;

void neko_view_init(neko_view *self,
                    gp_size x, gp_size y, gp_size w, gp_size h);

void neko_subviews_init(neko_view *left, neko_view *right, neko_view *parent);


static inline int neko_view_is_shown(neko_view *self)
{
	if (!self)
		return 0;

	return self->is_shown;
}

/**
 * @brief Child did exit.
 *
 * This is called by a child when it did exit. The view should replace the
 * child with a different one (if possible) in the handler.
 *
 * @param self A neko view.
 */
static inline void neko_view_slot_exit(neko_view *self)
{
	if (!self)
		return;

	self->slot = NULL;

	if (self->slot_exit) {
		GP_DEBUG(1, "Calling slot empty callback view %p", self);
		self->slot_exit(self);
	}
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
 * @brief Fills in a slot in a neko view.
 *
 * Sets a new view slot content, possibly replaces currently shown content. If
 * slot is NULL the view becomes empty.
 *
 * @param self A neko view.
 * @param slot A new neko view slot content to be shonw in the view.
 */
void neko_view_slot_put(neko_view *self, neko_view_slot *slot);

/**
 * @brief Sends an input event to a view child.
 *
 * @param self A neko view.
 * @param ev An input event.
 */
void neko_view_event(neko_view *self, gp_event *ev);

/**
 * @brief Returns a pixmap for the view.
 *
 * @param self A neko view.
 */
gp_pixmap *neko_view_pixmap(neko_view *self);

/**
 * @brief Resizes the view recursively along with all slots.
 *
 * @param self A neko view.
 * @param w New width.
 * @param h New height.
 */
void neko_view_resize(neko_view *self, gp_size w, gp_size h);

/**
 * @brief Request a view repaint.
 *
 * Needs to be called after resize on view(s) shown on the screen(s).
 *
 * @param self A view to be repainted.
 */
void neko_view_repaint(neko_view *self);

void neko_view_show(neko_view *self);

void neko_view_hide(neko_view *self);

#endif /* NEKO_VIEW_H */
