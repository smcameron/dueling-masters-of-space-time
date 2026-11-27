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
#ifndef DMOST_AI_H
#define DMOST_AI_H

#include <stdint.h>

#include "game.h"
#include "opponent.h"

/*
 * Populates b->own[] in PHASE_SETUP with a randomized, sensible defensive and
 * scouting layout in b->side's territory. Returns 1 if setup is complete.
 */
int dmost_ai_setup_board(struct player_board *b, uint32_t *rng_state);

/*
 * Creates a non-cheating AI opponent backed by its own private player_board.
 * The AI sets up its board and queues MSG_READY immediately.
 */
struct dmost_opponent *dmost_ai_opponent_create(enum player_side ai_side,
						 enum player_side first_turn,
						 uint32_t seed);

/*
 * Read-only accessor for the AI's private board (intended for tests/debug).
 */
const struct player_board *dmost_ai_get_board(const struct dmost_opponent *opp);

#endif /* DMOST_AI_H */
