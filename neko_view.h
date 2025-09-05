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
 * @brief A split mode.
 *
 * If view is split it can be split either horizontally or vertically.
 */
enum neko_view_split_mode {
	NEKO_VIEW_SPLIT_NONE,
	NEKO_VIEW_SPLIT_HORIZ,
	NEKO_VIEW_SPLIT_VERT,
};

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
	/** @brief Set if the view has focus. */
	unsigned int is_focused:1;

	/** @brief A view may be split into two subviews. */
	struct neko_view *subviews[2];
	/** @brief Currently focused subview. */
	unsigned int focused_subview;
	/** @brief Type of the split. */
	enum neko_view_split_mode split_mode;

	/** @brief What is shown in the view. */
	neko_view_slot *slot;

	/** @brief A view name. */
	char name[32];
} neko_view;

/**
 * @brief Initialize a view.
 *
 * @param self A view to initialize.
 * @param x A x offset of the view on a screen.
 * @param y A y offset of the view on a screen.
 * @param w A view width in pixels.
 * @param h A view height in pixels.
 * @param name A view name.
 */
void neko_view_init(neko_view *self,
                    gp_size x, gp_size y, gp_size w, gp_size h,
		    const char *name);

void neko_subviews_init(neko_view *left, neko_view *right, neko_view *parent, enum neko_view_split_mode mode);

/**
 * @brief Returns view focused child.
 *
 * @param self A neko view.
 *
 * @return Focused child, or NULL if view does not have any children.
 */
neko_view *neko_view_focused_child(neko_view *self);

/**
 * @brief Returns true if view is focused.
 *
 * @param self A neko view.
 *
 * @return Returns true if view is focused.
 */
int neko_view_is_focused(neko_view *self);

/**
 * @brief Removes a slot from a view.
 *
 * @param self A neko view.
 */
void neko_view_slot_rem(neko_view *self);

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

/**
 * @brief Switches a view shown on a display.
 *
 * Before new view is shown current while, if any, must be hidden with
 * neko_view_hide().
 *
 * @param self A view to be shown.
 */
void neko_view_show(neko_view *self);

/**
 * @brief Hides a view from a display.
 *
 * This function can be called only on a view that is currently shown on a
 * screen.
 *
 * @param self A currently shown view.
 */
void neko_view_hide(neko_view *self);

/**
 * @brief Sends a focus in event to the view event handler.
 *
 * @param self A view.
 */
static inline void neko_view_focus_in(neko_view *self)
{
	gp_event ev = {
		.type = GP_EV_SYS,
		.code = GP_EV_SYS_FOCUS,
		.val = GP_EV_SYS_FOCUS_IN,
	};

	neko_view_event(self, &ev);
}

/**
 * @brief Sends a focus out event to the view event handler.
 *
 * @param self A view.
 */
static inline void neko_view_focus_out(neko_view *self)
{
	gp_event ev = {
		.type = GP_EV_SYS,
		.code = GP_EV_SYS_FOCUS,
		.val = GP_EV_SYS_FOCUS_OUT,
	};

	neko_view_event(self, &ev);
}

#endif /* NEKO_VIEW_H */
