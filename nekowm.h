//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

/*
 * Access to nekowm fd loop etc.
 */

#ifndef NEKOWM_H
#define NEKOWM_H

#include <utils/gp_poll.h>

void nekowm_poll_rem(gp_fd *self);

#endif /* NEKOWM_H */
