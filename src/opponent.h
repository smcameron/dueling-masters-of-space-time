/*
 *  (C) Copyright 2026 Stephen M. Cameron.
 *
 *  This file is part of Dueling Masters of Space Time.
 *
 *  Dueling Masters of Space Time is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dueling Masters of Space Time is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dueling Masters of Space Time; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef DMOST_OPPONENT_H
#define DMOST_OPPONENT_H

#include <stddef.h>

#include "game.h"

struct dmost_opponent;

/*
 * Transport-agnostic opponent interface. Both the local computer AI and a
 * future network peer implement this vtable, exchanging only dmost_msg values.
 */
struct dmost_opponent_ops {
	int (*send_msg)(struct dmost_opponent *opp, const struct dmost_msg *msg);
	/* Returns 1 if out_msg populated, 0 if none ready, -1 on error. */
	int (*poll_msg)(struct dmost_opponent *opp, struct dmost_msg *out_msg);
	/* Returns pollable file descriptor (e.g., socket) or -1 if local. */
	int (*get_fd)(struct dmost_opponent *opp);
	void (*destroy)(struct dmost_opponent *opp);
};

struct dmost_opponent {
	const struct dmost_opponent_ops *ops;
	void *priv;
};

static inline int dmost_opponent_send(struct dmost_opponent *opp,
				      const struct dmost_msg *msg)
{
	return opp->ops->send_msg(opp, msg);
}

static inline int dmost_opponent_poll(struct dmost_opponent *opp,
				      struct dmost_msg *out_msg)
{
	return opp->ops->poll_msg(opp, out_msg);
}

static inline int dmost_opponent_get_fd(struct dmost_opponent *opp)
{
	return opp->ops->get_fd(opp);
}

static inline void dmost_opponent_destroy(struct dmost_opponent *opp)
{
	if (opp != NULL)
		opp->ops->destroy(opp);
}

#endif /* DMOST_OPPONENT_H */
