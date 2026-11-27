/*
 *  (C) Copyright 2012, 2026 Stephen M. Cameron.
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
#ifndef DMOST_GAME_H
#define DMOST_GAME_H

#include <stdint.h>

#define BOARD_SIZE 10
#define NUM_PIECES 18
#define PIECE_INDEX_HP 17
#define UNKNOWN_POS (-1)
#define MAX_PATH_LEN 10
#define MAX_COLLISIONS_PER_BATCH 32

enum player_side {
	SIDE_RED = 1,
	SIDE_BLUE = 2,
};

/*
 * Piece types ordered by combat strength (except ASTEROID, which destroys
 * anything it touches and is destroyed by anything it touches).
 */
enum piece_type {
	PIECE_HOMEPLANET = 0,
	PIECE_ASTEROID   = 1,
	PIECE_ZFIGHTER   = 2,
	PIECE_YFIGHTER   = 3,
	PIECE_XFIGHTER   = 4,
	PIECE_BFIGHTER   = 5,
	PIECE_WFIGHTER   = 6,
};

enum combat_outcome {
	COMBAT_BOTH_DIE = 0,
	COMBAT_MOVER_WINS = 1,
	COMBAT_DEFENDER_WINS = 2,
};

enum game_phase {
	PHASE_SETUP = 0,
	PHASE_PLAYING = 1,
	PHASE_RESOLVING = 2,
	PHASE_GAME_OVER = 3,
};

enum game_result {
	RESULT_ONGOING = 0,
	RESULT_RED_WINS = 1,
	RESULT_BLUE_WINS = 2,
	RESULT_DRAW = 3,
};

enum dmost_msg_type {
	MSG_READY = 1,
	MSG_DEPART = 2,
	MSG_COLLISION_BATCH = 3,
	MSG_PASS = 4,
	MSG_DRAW_NOTIFY = 5,
};

enum dmost_event_type {
	EVENT_DEPART = 1,
	EVENT_COLLISION = 2,
	EVENT_PASS = 3,
	EVENT_GAME_OVER = 4,
};

struct square {
	int x;
	int y;
};

struct piece_info {
	const char *name;
	const char *description;
	enum piece_type type;
};

struct own_piece {
	int id;
	enum piece_type type;
	int x, y;               /* current actual position, or UNKNOWN_POS off board */
	int prev_x, prev_y;     /* previous position for trail rendering */
	int announced_x;        /* last announced departure square, or UNKNOWN_POS */
	int announced_y;
	int in_transit;         /* 1 if moved since last announced departure */
	int alive;
};

struct known_enemy_piece {
	int id;
	enum piece_type type;
	int known_x, known_y;   /* last announced/revealed square, or UNKNOWN_POS */
	int prev_x, prev_y;     /* prior known square for trail rendering */
	int alive;
	int resolved_turn;      /* last turn_number checked at (resolved_x, resolved_y) */
	int resolved_x;
	int resolved_y;
};

struct pending_move {
	int active;
	int piece_id;
	int from_x, from_y;
	int to_x, to_y;
};

struct dmost_collision {
	int mover_id;           /* piece id of the incoming mover */
	int defender_id;        /* piece id of the stationary piece hit */
	int x, y;               /* square of collision */
	int mover_died;
	int defender_died;
};

struct dmost_msg {
	enum dmost_msg_type type;
	enum player_side sender;
	int only_y_parity_mask;       /* -1 if N/A, else bitmask of alive Y parities */
	int only_y_enemy_can_hit_hp;  /* -1 if unknown, 0 if cannot hit, 1 if can */
	int y_draw_agreed;            /* 1 if sender confirmed mutual Y-parity draw */
	union {
		struct {
			int piece_id;
			int from_x, from_y;
		} depart;
		struct {
			int count;
			struct dmost_collision items[MAX_COLLISIONS_PER_BATCH];
		} collisions;
	} u;
};

struct dmost_event {
	enum dmost_event_type type;
	enum player_side actor;
	int piece_id;
	int other_piece_id;
	int x, y;
	int actor_died;
	int other_died;
	enum game_result result;
};

typedef void (*dmost_event_fn)(void *ctx, const struct dmost_event *ev);

struct player_board {
	enum player_side side;
	enum game_phase phase;
	enum game_result result;
	enum player_side current_turn;
	int turn_number;
	int consecutive_passes;

	int own_ready;
	int enemy_ready;

	struct own_piece own[NUM_PIECES];
	struct known_enemy_piece enemy[NUM_PIECES];

	struct pending_move pending;

	uint8_t own_visited[BOARD_SIZE][BOARD_SIZE];
	uint8_t enemy_clear[BOARD_SIZE][BOARD_SIZE];

	/* Non-cheating Y-fighter parity draw negotiation state */
	int enemy_y_parity_mask;      /* -1 until received */
	int enemy_can_hit_my_hp;      /* -1 until evaluated, 0 or 1 */
	int peer_cannot_hit_hp;       /* 1 once opponent reported enemy_can_hit_my_hp == 0 */

	dmost_event_fn event_cb;
	void *event_ctx;
};

extern const struct piece_info dmost_piece_info[NUM_PIECES];

enum player_side dmost_other_side(enum player_side side);
int dmost_side_owns_row(enum player_side side, int y);
int dmost_piece_is_mobile(enum piece_type type);

/*
 * Computes the square-by-square path from (x0, y0) to (x1, y1) for the given
 * piece type, including both endpoints. Returns the number of squares (>= 2),
 * or 0 if the move geometry is invalid or out of bounds.
 */
int dmost_compute_path(enum piece_type type, int x0, int y0, int x1, int y1,
		       struct square *out_path, int max_len);

enum combat_outcome dmost_resolve_combat(enum piece_type mover,
					 enum piece_type defender);

void dmost_board_init(struct player_board *b, enum player_side side,
		      enum player_side first_turn);
void dmost_board_set_event_cb(struct player_board *b, dmost_event_fn cb,
			      void *ctx);

int dmost_can_place_piece(const struct player_board *b, int piece_id,
			  int x, int y);
int dmost_place_piece(struct player_board *b, int piece_id, int x, int y);
int dmost_unplace_piece(struct player_board *b, int piece_id);
int dmost_setup_is_complete(const struct player_board *b);

/*
 * Locks in own setup and produces MSG_READY. Transitions to PHASE_PLAYING
 * immediately if enemy_ready is already set. Returns 1 on success, 0 if invalid.
 */
int dmost_mark_ready(struct player_board *b, struct dmost_msg *out_msg);

int dmost_is_legal_move(const struct player_board *b, int piece_id,
			int to_x, int to_y);
int dmost_has_any_legal_move(const struct player_board *b);

/*
 * Initiates a move from own[piece_id]'s current square to (to_x, to_y) and
 * produces MSG_DEPART announcing only the departure square. Returns 1 on success.
 */
int dmost_initiate_move(struct player_board *b, int piece_id,
			int to_x, int to_y, struct dmost_msg *out_msg);

/*
 * Passes the turn when the current player has no legal moves and produces
 * MSG_PASS. Returns 1 on success, 0 if a legal move exists or invalid state.
 */
int dmost_initiate_pass(struct player_board *b, struct dmost_msg *out_msg);

/*
 * Processes an incoming protocol message. If a reply is required, *has_reply
 * is set to 1 and *out_msg is populated; otherwise *has_reply is set to 0.
 * Returns 1 on success, 0 on protocol/state error.
 */
int dmost_handle_message(struct player_board *b,
			 const struct dmost_msg *in_msg,
			 struct dmost_msg *out_msg, int *has_reply);

void dmost_format_coord(int x, int y, char *buf, int bufsize);
void dmost_format_event(const struct dmost_event *ev, enum player_side viewer,
			char *buf, int bufsize);

#endif /* DMOST_GAME_H */
