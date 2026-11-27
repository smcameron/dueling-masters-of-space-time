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
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const struct piece_info dmost_piece_info[NUM_PIECES] = {
	{ "W",  "W fighter. Moves 2 then 1 (L-path). Strongest mobile piece.", PIECE_WFIGHTER },
	{ "B",  "B fighter. Moves like chess queen. Only W is stronger.",     PIECE_BFIGHTER },
	{ "X1", "X1 fighter. Moves like chess rook. W and B are stronger.",   PIECE_XFIGHTER },
	{ "X2", "X2 fighter. Moves like chess rook. W and B are stronger.",   PIECE_XFIGHTER },
	{ "Y1", "Y1 fighter. Moves like chess bishop. W, B, X are stronger.", PIECE_YFIGHTER },
	{ "Y2", "Y2 fighter. Moves like chess bishop. W, B, X are stronger.", PIECE_YFIGHTER },
	{ "Z1", "Z1 fighter. Moves like chess queen. Weakest mobile piece.",  PIECE_ZFIGHTER },
	{ "Z2", "Z2 fighter. Moves like chess queen. Weakest mobile piece.",  PIECE_ZFIGHTER },
	{ "Z3", "Z3 fighter. Moves like chess queen. Weakest mobile piece.",  PIECE_ZFIGHTER },
	{ "Z4", "Z4 fighter. Moves like chess queen. Weakest mobile piece.",  PIECE_ZFIGHTER },
	{ "A",  "Asteroid (mine). Destroys and is destroyed by everything.",  PIECE_ASTEROID },
	{ "A",  "Asteroid (mine). Destroys and is destroyed by everything.",  PIECE_ASTEROID },
	{ "A",  "Asteroid (mine). Destroys and is destroyed by everything.",  PIECE_ASTEROID },
	{ "A",  "Asteroid (mine). Destroys and is destroyed by everything.",  PIECE_ASTEROID },
	{ "A",  "Asteroid (mine). Destroys and is destroyed by everything.",  PIECE_ASTEROID },
	{ "A",  "Asteroid (mine). Destroys and is destroyed by everything.",  PIECE_ASTEROID },
	{ "A",  "Asteroid (mine). Destroys and is destroyed by everything.",  PIECE_ASTEROID },
	{ "HP", "Home Planet. Immobile. Losing it loses the game.",           PIECE_HOMEPLANET },
};

static int in_bounds(int x, int y)
{
	return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

static int sign_int(int v)
{
	if (v > 0)
		return 1;
	if (v < 0)
		return -1;
	return 0;
}

static void emit_event(const struct player_board *b, const struct dmost_event *ev)
{
	if (b->event_cb != NULL)
		b->event_cb(b->event_ctx, ev);
}

enum player_side dmost_other_side(enum player_side side)
{
	return (side == SIDE_RED) ? SIDE_BLUE : SIDE_RED;
}

int dmost_side_owns_row(enum player_side side, int y)
{
	if (side == SIDE_RED)
		return y >= 0 && y < 5;
	if (side == SIDE_BLUE)
		return y >= 5 && y < BOARD_SIZE;
	return 0;
}

int dmost_piece_is_mobile(enum piece_type type)
{
	return type != PIECE_ASTEROID && type != PIECE_HOMEPLANET;
}

int dmost_compute_path(enum piece_type type, int x0, int y0, int x1, int y1,
		       struct square *out_path, int max_len)
{
	int dx, dy, adx, ady, sx, sy, steps, i;

	if (!in_bounds(x0, y0) || !in_bounds(x1, y1))
		return 0;
	if (x0 == x1 && y0 == y1)
		return 0;

	dx = x1 - x0;
	dy = y1 - y0;
	adx = abs(dx);
	ady = abs(dy);

	if (type == PIECE_WFIGHTER) {
		if (!((adx == 2 && ady == 1) || (adx == 1 && ady == 2)))
			return 0;
		if (max_len < 4)
			return 0;
		out_path[0].x = x0;
		out_path[0].y = y0;
		if (ady == 2) {
			sy = sign_int(dy);
			out_path[1].x = x0;
			out_path[1].y = y0 + sy;
			out_path[2].x = x0;
			out_path[2].y = y0 + 2 * sy;
		} else {
			sx = sign_int(dx);
			out_path[1].x = x0 + sx;
			out_path[1].y = y0;
			out_path[2].x = x0 + 2 * sx;
			out_path[2].y = y0;
		}
		out_path[3].x = x1;
		out_path[3].y = y1;
		return 4;
	}

	switch (type) {
	case PIECE_BFIGHTER:
	case PIECE_ZFIGHTER:
		if (!(dx == 0 || dy == 0 || adx == ady))
			return 0;
		break;
	case PIECE_XFIGHTER:
		if (!(dx == 0 || dy == 0))
			return 0;
		break;
	case PIECE_YFIGHTER:
		if (adx != ady)
			return 0;
		break;
	case PIECE_ASTEROID:
	case PIECE_HOMEPLANET:
	default:
		return 0;
	}

	steps = (adx > ady) ? adx : ady;
	if (max_len < steps + 1)
		return 0;

	sx = sign_int(dx);
	sy = sign_int(dy);
	for (i = 0; i <= steps; i++) {
		out_path[i].x = x0 + i * sx;
		out_path[i].y = y0 + i * sy;
	}
	return steps + 1;
}

enum combat_outcome dmost_resolve_combat(enum piece_type mover,
					 enum piece_type defender)
{
	if (mover == PIECE_ASTEROID || defender == PIECE_ASTEROID ||
	    mover == defender)
		return COMBAT_BOTH_DIE;
	if (mover > defender)
		return COMBAT_MOVER_WINS;
	return COMBAT_DEFENDER_WINS;
}

void dmost_board_init(struct player_board *b, enum player_side side,
		      enum player_side first_turn)
{
	int i;

	memset(b, 0, sizeof(*b));
	b->side = side;
	b->phase = PHASE_SETUP;
	b->result = RESULT_ONGOING;
	b->current_turn = first_turn;
	b->turn_number = 1;
	b->enemy_y_parity_mask = -1;
	b->enemy_can_hit_my_hp = -1;

	for (i = 0; i < NUM_PIECES; i++) {
		b->own[i].id = i;
		b->own[i].type = dmost_piece_info[i].type;
		b->own[i].x = UNKNOWN_POS;
		b->own[i].y = UNKNOWN_POS;
		b->own[i].prev_x = UNKNOWN_POS;
		b->own[i].prev_y = UNKNOWN_POS;
		b->own[i].announced_x = UNKNOWN_POS;
		b->own[i].announced_y = UNKNOWN_POS;
		b->own[i].in_transit = 0;
		b->own[i].alive = 1;

		b->enemy[i].id = i;
		b->enemy[i].type = dmost_piece_info[i].type;
		b->enemy[i].known_x = UNKNOWN_POS;
		b->enemy[i].known_y = UNKNOWN_POS;
		b->enemy[i].prev_x = UNKNOWN_POS;
		b->enemy[i].prev_y = UNKNOWN_POS;
		b->enemy[i].alive = 1;
		b->enemy[i].resolved_turn = 0;
		b->enemy[i].resolved_x = UNKNOWN_POS;
		b->enemy[i].resolved_y = UNKNOWN_POS;
	}
}

void dmost_board_set_event_cb(struct player_board *b, dmost_event_fn cb,
			      void *ctx)
{
	b->event_cb = cb;
	b->event_ctx = ctx;
}

int dmost_can_place_piece(const struct player_board *b, int piece_id,
			  int x, int y)
{
	int i;
	int placing_mobile;

	if (b->phase != PHASE_SETUP || b->own_ready)
		return 0;
	if (piece_id < 0 || piece_id >= NUM_PIECES)
		return 0;
	if (!in_bounds(x, y) || !dmost_side_owns_row(b->side, y))
		return 0;

	placing_mobile = dmost_piece_is_mobile(b->own[piece_id].type);
	for (i = 0; i < NUM_PIECES; i++) {
		if (i == piece_id)
			continue;
		if (b->own[i].x == x && b->own[i].y == y) {
			/* Asteroids and Home Planet must occupy squares alone. */
			if (!placing_mobile || !dmost_piece_is_mobile(b->own[i].type))
				return 0;
		}
	}
	return 1;
}

int dmost_place_piece(struct player_board *b, int piece_id, int x, int y)
{
	if (!dmost_can_place_piece(b, piece_id, x, y))
		return 0;
	b->own[piece_id].x = x;
	b->own[piece_id].y = y;
	b->own[piece_id].prev_x = UNKNOWN_POS;
	b->own[piece_id].prev_y = UNKNOWN_POS;
	b->own[piece_id].announced_x = UNKNOWN_POS;
	b->own[piece_id].announced_y = UNKNOWN_POS;
	b->own[piece_id].in_transit = 0;
	b->own[piece_id].alive = 1;
	return 1;
}

int dmost_unplace_piece(struct player_board *b, int piece_id)
{
	if (b->phase != PHASE_SETUP || b->own_ready)
		return 0;
	if (piece_id < 0 || piece_id >= NUM_PIECES)
		return 0;
	b->own[piece_id].x = UNKNOWN_POS;
	b->own[piece_id].y = UNKNOWN_POS;
	return 1;
}

int dmost_setup_is_complete(const struct player_board *b)
{
	int i;

	for (i = 0; i < NUM_PIECES; i++) {
		if (!in_bounds(b->own[i].x, b->own[i].y))
			return 0;
		if (!dmost_side_owns_row(b->side, b->own[i].y))
			return 0;
	}
	return 1;
}

static void init_msg_header(struct dmost_msg *msg, enum dmost_msg_type type,
			    enum player_side sender)
{
	memset(msg, 0, sizeof(*msg));
	msg->type = type;
	msg->sender = sender;
	msg->only_y_parity_mask = -1;
	msg->only_y_enemy_can_hit_hp = -1;
}

static int own_has_any_mobile(const struct player_board *b)
{
	int i;

	for (i = 0; i < NUM_PIECES; i++) {
		if (b->own[i].alive && dmost_piece_is_mobile(b->own[i].type))
			return 1;
	}
	return 0;
}

static int enemy_has_any_mobile(const struct player_board *b)
{
	int i;

	for (i = 0; i < NUM_PIECES; i++) {
		if (b->enemy[i].alive && dmost_piece_is_mobile(b->enemy[i].type))
			return 1;
	}
	return 0;
}

static int own_has_only_y_mobile(const struct player_board *b)
{
	int i;

	for (i = 0; i < NUM_PIECES; i++) {
		if (!b->own[i].alive)
			continue;
		if (dmost_piece_is_mobile(b->own[i].type) &&
		    b->own[i].type != PIECE_YFIGHTER)
			return 0;
	}
	return 1;
}

static int enemy_has_only_y_mobile(const struct player_board *b)
{
	int i;

	for (i = 0; i < NUM_PIECES; i++) {
		if (!b->enemy[i].alive)
			continue;
		if (dmost_piece_is_mobile(b->enemy[i].type) &&
		    b->enemy[i].type != PIECE_YFIGHTER)
			return 0;
	}
	return 1;
}

static int compute_own_y_parity_mask(const struct player_board *b)
{
	int i, mask = 0;

	for (i = 0; i < NUM_PIECES; i++) {
		if (b->own[i].alive && b->own[i].type == PIECE_YFIGHTER &&
		    in_bounds(b->own[i].x, b->own[i].y))
			mask |= 1 << ((b->own[i].x + b->own[i].y) & 1);
	}
	return mask;
}

static void populate_y_draw_fields(const struct player_board *b,
				   struct dmost_msg *msg)
{
	if (!b->own[PIECE_INDEX_HP].alive || !b->enemy[PIECE_INDEX_HP].alive)
		return;
	if (!own_has_only_y_mobile(b) || !enemy_has_only_y_mobile(b))
		return;
	msg->only_y_parity_mask = compute_own_y_parity_mask(b);
	msg->only_y_enemy_can_hit_hp = b->enemy_can_hit_my_hp;
}

static void ingest_y_draw_fields(struct player_board *b,
				   const struct dmost_msg *in_msg)
{
	int hp_parity;

	if (in_msg->only_y_parity_mask >= 0 && b->own[PIECE_INDEX_HP].alive) {
		b->enemy_y_parity_mask = in_msg->only_y_parity_mask;
		hp_parity = (b->own[PIECE_INDEX_HP].x +
			     b->own[PIECE_INDEX_HP].y) & 1;
		b->enemy_can_hit_my_hp =
			((b->enemy_y_parity_mask & (1 << hp_parity)) != 0) ? 1 : 0;
	}
	if (in_msg->only_y_enemy_can_hit_hp == 0)
		b->peer_cannot_hit_hp = 1;
}

static int is_y_draw_ready(const struct player_board *b)
{
	return b->own[PIECE_INDEX_HP].alive &&
	       b->enemy[PIECE_INDEX_HP].alive &&
	       own_has_only_y_mobile(b) &&
	       enemy_has_only_y_mobile(b) &&
	       b->enemy_can_hit_my_hp == 0 &&
	       b->peer_cannot_hit_hp == 1;
}

int dmost_mark_ready(struct player_board *b, struct dmost_msg *out_msg)
{
	if (b->phase != PHASE_SETUP || b->own_ready)
		return 0;
	if (!dmost_setup_is_complete(b))
		return 0;

	b->own_ready = 1;
	if (b->enemy_ready)
		b->phase = PHASE_PLAYING;

	init_msg_header(out_msg, MSG_READY, b->side);
	return 1;
}

static int own_immobile_at(const struct player_board *b, int x, int y)
{
	int i;

	for (i = 0; i < NUM_PIECES; i++) {
		if (b->own[i].alive &&
		    !dmost_piece_is_mobile(b->own[i].type) &&
		    b->own[i].x == x && b->own[i].y == y)
			return 1;
	}
	return 0;
}

int dmost_is_legal_move(const struct player_board *b, int piece_id,
			int to_x, int to_y)
{
	struct square path[MAX_PATH_LEN];
	int len, i;

	if (b->phase != PHASE_PLAYING || b->current_turn != b->side)
		return 0;
	if (piece_id < 0 || piece_id >= NUM_PIECES)
		return 0;
	if (!b->own[piece_id].alive ||
	    !dmost_piece_is_mobile(b->own[piece_id].type))
		return 0;
	if (!in_bounds(b->own[piece_id].x, b->own[piece_id].y))
		return 0;

	len = dmost_compute_path(b->own[piece_id].type,
				 b->own[piece_id].x, b->own[piece_id].y,
				 to_x, to_y, path, MAX_PATH_LEN);
	if (len < 2)
		return 0;

	for (i = 1; i < len; i++) {
		if (own_immobile_at(b, path[i].x, path[i].y))
			return 0;
	}
	return 1;
}

int dmost_has_any_legal_move(const struct player_board *b)
{
	int id, x, y;

	if (b->phase != PHASE_PLAYING || b->current_turn != b->side)
		return 0;

	for (id = 0; id < NUM_PIECES; id++) {
		if (!b->own[id].alive || !dmost_piece_is_mobile(b->own[id].type))
			continue;
		for (x = 0; x < BOARD_SIZE; x++) {
			for (y = 0; y < BOARD_SIZE; y++) {
				if (dmost_is_legal_move(b, id, x, y))
					return 1;
			}
		}
	}
	return 0;
}

int dmost_initiate_move(struct player_board *b, int piece_id,
			int to_x, int to_y, struct dmost_msg *out_msg)
{
	struct dmost_event ev;

	if (!dmost_is_legal_move(b, piece_id, to_x, to_y))
		return 0;

	b->consecutive_passes = 0;
	b->phase = PHASE_RESOLVING;
	b->pending.active = 1;
	b->pending.piece_id = piece_id;
	b->pending.from_x = b->own[piece_id].x;
	b->pending.from_y = b->own[piece_id].y;
	b->pending.to_x = to_x;
	b->pending.to_y = to_y;

	init_msg_header(out_msg, MSG_DEPART, b->side);
	out_msg->u.depart.piece_id = piece_id;
	out_msg->u.depart.from_x = b->pending.from_x;
	out_msg->u.depart.from_y = b->pending.from_y;
	populate_y_draw_fields(b, out_msg);

	memset(&ev, 0, sizeof(ev));
	ev.type = EVENT_DEPART;
	ev.actor = b->side;
	ev.piece_id = piece_id;
	ev.x = b->pending.from_x;
	ev.y = b->pending.from_y;
	emit_event(b, &ev);

	return 1;
}

static void mark_path_squares(uint8_t grid[BOARD_SIZE][BOARD_SIZE],
			      const struct square *path, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		if (in_bounds(path[i].x, path[i].y))
			grid[path[i].x][path[i].y] = 1;
	}
}

static void evaluate_board_result(struct player_board *b)
{
	int own_hp = b->own[PIECE_INDEX_HP].alive;
	int enemy_hp = b->enemy[PIECE_INDEX_HP].alive;

	if (!own_hp && !enemy_hp)
		b->result = RESULT_DRAW;
	else if (!enemy_hp)
		b->result = (b->side == SIDE_RED) ? RESULT_RED_WINS : RESULT_BLUE_WINS;
	else if (!own_hp)
		b->result = (b->side == SIDE_RED) ? RESULT_BLUE_WINS : RESULT_RED_WINS;
	else if (!own_has_any_mobile(b) && !enemy_has_any_mobile(b))
		b->result = RESULT_DRAW;
}

static void trigger_game_over(struct player_board *b, enum game_result res)
{
	struct dmost_event ev;

	b->result = res;
	b->phase = PHASE_GAME_OVER;
	memset(&ev, 0, sizeof(ev));
	ev.type = EVENT_GAME_OVER;
	ev.result = res;
	emit_event(b, &ev);
}

int dmost_initiate_pass(struct player_board *b, struct dmost_msg *out_msg)
{
	struct dmost_event ev;

	if (b->phase != PHASE_PLAYING || b->current_turn != b->side)
		return 0;
	if (dmost_has_any_legal_move(b))
		return 0;

	b->consecutive_passes++;

	init_msg_header(out_msg, MSG_PASS, b->side);
	populate_y_draw_fields(b, out_msg);

	memset(&ev, 0, sizeof(ev));
	ev.type = EVENT_PASS;
	ev.actor = b->side;
	emit_event(b, &ev);

	evaluate_board_result(b);
	if (b->result != RESULT_ONGOING) {
		trigger_game_over(b, b->result);
		return 1;
	}
	if (b->consecutive_passes >= 2) {
		trigger_game_over(b, RESULT_DRAW);
		return 1;
	}

	b->current_turn = dmost_other_side(b->side);
	b->turn_number++;
	return 1;
}

static void mark_completed_own_transit(struct player_board *b,
				       const struct own_piece *p,
				       int end_x, int end_y, int include_end)
{
	struct square path[MAX_PATH_LEN];
	int len, count;

	if (p->announced_x == UNKNOWN_POS)
		return;
	if (p->announced_x == end_x && p->announced_y == end_y) {
		if (include_end && in_bounds(end_x, end_y)) {
			b->own_visited[end_x][end_y] = 1;
			b->enemy_clear[end_x][end_y] = 1;
		}
		return;
	}

	len = dmost_compute_path(p->type, p->announced_x, p->announced_y,
				 end_x, end_y, path, MAX_PATH_LEN);
	if (len <= 0)
		return;

	count = include_end ? len : (len - 1);
	mark_path_squares(b->own_visited, path, count);
	mark_path_squares(b->enemy_clear, path, count);
}

static void finish_turn(struct player_board *b)
{
	if (b->pending.active) {
		struct own_piece *p = &b->own[b->pending.piece_id];

		if (p->alive) {
			if (p->in_transit && p->announced_x != UNKNOWN_POS)
				mark_completed_own_transit(b, p, p->x, p->y, 1);
			p->prev_x = b->pending.from_x;
			p->prev_y = b->pending.from_y;
			p->announced_x = b->pending.from_x;
			p->announced_y = b->pending.from_y;
			p->x = b->pending.to_x;
			p->y = b->pending.to_y;
			p->in_transit = 1;
			b->own_visited[b->pending.from_x][b->pending.from_y] = 1;
			b->enemy_clear[b->pending.from_x][b->pending.from_y] = 1;
		}
		b->pending.active = 0;
	}

	evaluate_board_result(b);
	if (b->result != RESULT_ONGOING) {
		trigger_game_over(b, b->result);
		return;
	}

	b->current_turn = dmost_other_side(b->current_turn);
	b->turn_number++;
	b->phase = PHASE_PLAYING;
}

static int append_collision(struct dmost_msg *batch_msg,
			    int mover_id, int defender_id,
			    int x, int y, enum combat_outcome outcome)
{
	int idx;
	struct dmost_collision *c;

	idx = batch_msg->u.collisions.count;
	if (idx >= MAX_COLLISIONS_PER_BATCH)
		return 0;

	c = &batch_msg->u.collisions.items[idx];
	c->mover_id = mover_id;
	c->defender_id = defender_id;
	c->x = x;
	c->y = y;
	c->mover_died = (outcome == COMBAT_BOTH_DIE ||
			 outcome == COMBAT_DEFENDER_WINS) ? 1 : 0;
	c->defender_died = (outcome == COMBAT_BOTH_DIE ||
			    outcome == COMBAT_MOVER_WINS) ? 1 : 0;
	batch_msg->u.collisions.count++;
	return 1;
}

/*
 * Evaluates an enemy piece moving from its last known position to
 * (target_x, target_y) against all alive own pieces along the path.
 * Stops at the first square where the enemy piece is destroyed.
 */
static int check_enemy_path(struct player_board *b, int enemy_id,
			    int target_x, int target_y,
			    int skip_own_id, int skip_x, int skip_y,
			    struct dmost_msg *out_batch)
{
	struct known_enemy_piece *e;
	struct square path[MAX_PATH_LEN];
	int old_kx, old_ky, path_len, k, j;

	if (enemy_id < 0 || enemy_id >= NUM_PIECES || !in_bounds(target_x, target_y))
		return 0;

	e = &b->enemy[enemy_id];
	if (e->resolved_turn == b->turn_number &&
	    e->resolved_x == target_x && e->resolved_y == target_y)
		return 1;

	e->resolved_turn = b->turn_number;
	e->resolved_x = target_x;
	e->resolved_y = target_y;

	old_kx = e->known_x;
	old_ky = e->known_y;

	if (old_kx == UNKNOWN_POS || (old_kx == target_x && old_ky == target_y)) {
		path[0].x = target_x;
		path[0].y = target_y;
		path_len = 1;
	} else {
		path_len = dmost_compute_path(e->type, old_kx, old_ky,
					      target_x, target_y,
					      path, MAX_PATH_LEN);
		if (path_len <= 0)
			return 0;
	}

	for (k = 0; k < path_len; k++) {
		struct square sq = path[k];
		int mover_died_here = 0;

		for (j = 0; j < NUM_PIECES; j++) {
			enum combat_outcome outcome;
			struct dmost_event ev;
			int m_died, d_died;

			if (!b->own[j].alive)
				continue;
			if (b->own[j].x != sq.x || b->own[j].y != sq.y)
				continue;
			if (j == skip_own_id && sq.x == skip_x && sq.y == skip_y)
				continue;

			outcome = dmost_resolve_combat(e->type, b->own[j].type);
			if (!append_collision(out_batch, enemy_id, j, sq.x, sq.y,
					      outcome))
				return 0;

			m_died = (outcome == COMBAT_BOTH_DIE ||
				  outcome == COMBAT_DEFENDER_WINS);
			d_died = (outcome == COMBAT_BOTH_DIE ||
				  outcome == COMBAT_MOVER_WINS);

			memset(&ev, 0, sizeof(ev));
			ev.type = EVENT_COLLISION;
			ev.actor = dmost_other_side(b->side);
			ev.piece_id = enemy_id;
			ev.other_piece_id = j;
			ev.x = sq.x;
			ev.y = sq.y;
			ev.actor_died = m_died;
			ev.other_died = d_died;
			emit_event(b, &ev);

			if (d_died) {
				b->own[j].alive = 0;
			} else {
				b->own[j].announced_x = sq.x;
				b->own[j].announced_y = sq.y;
				b->own[j].in_transit = 0;
			}

			if (m_died) {
				mover_died_here = 1;
				break;
			}
		}

		if (mover_died_here) {
			e->alive = 0;
			if (old_kx != UNKNOWN_POS &&
			    (old_kx != sq.x || old_ky != sq.y)) {
				e->prev_x = old_kx;
				e->prev_y = old_ky;
			}
			e->known_x = sq.x;
			e->known_y = sq.y;
			mark_path_squares(b->enemy_clear, path, k);
			return 1;
		}
	}

	if (old_kx != UNKNOWN_POS &&
	    (old_kx != target_x || old_ky != target_y)) {
		e->prev_x = old_kx;
		e->prev_y = old_ky;
	}
	e->known_x = target_x;
	e->known_y = target_y;
	mark_path_squares(b->enemy_clear, path, path_len);
	return 1;
}

static void apply_incoming_mover_collision(struct player_board *b,
					   const struct dmost_collision *c)
{
	struct own_piece *m;
	struct dmost_event ev;

	if (c->mover_id < 0 || c->mover_id >= NUM_PIECES)
		return;
	m = &b->own[c->mover_id];

	if (m->in_transit && m->announced_x != UNKNOWN_POS)
		mark_completed_own_transit(b, m, c->x, c->y, !c->mover_died);

	if (c->mover_died) {
		if (m->in_transit && m->announced_x != UNKNOWN_POS &&
		    (m->announced_x != c->x || m->announced_y != c->y)) {
			m->prev_x = m->announced_x;
			m->prev_y = m->announced_y;
		}
		m->alive = 0;
		m->x = c->x;
		m->y = c->y;
		m->in_transit = 0;
	} else {
		/* Survived collision on path: record enemy square as cleared */
		if (in_bounds(c->x, c->y)) {
			b->own_visited[c->x][c->y] = 1;
			b->enemy_clear[c->x][c->y] = 1;
		}
	}

	memset(&ev, 0, sizeof(ev));
	ev.type = EVENT_COLLISION;
	ev.actor = b->side;
	ev.piece_id = c->mover_id;
	ev.other_piece_id = c->defender_id;
	ev.x = c->x;
	ev.y = c->y;
	ev.actor_died = c->mover_died;
	ev.other_died = c->defender_died;
	emit_event(b, &ev);
}

static int handle_depart_msg(struct player_board *b,
			     const struct dmost_msg *in_msg,
			     struct dmost_msg *out_msg, int *has_reply)
{
	struct dmost_event ev;
	int piece_id, fx, fy;

	if (b->phase != PHASE_PLAYING || b->current_turn != in_msg->sender ||
	    in_msg->sender == b->side)
		return 0;

	piece_id = in_msg->u.depart.piece_id;
	fx = in_msg->u.depart.from_x;
	fy = in_msg->u.depart.from_y;
	if (piece_id < 0 || piece_id >= NUM_PIECES ||
	    !dmost_piece_is_mobile(b->enemy[piece_id].type) ||
	    !in_bounds(fx, fy))
		return 0;

	b->consecutive_passes = 0;
	ingest_y_draw_fields(b, in_msg);

	memset(&ev, 0, sizeof(ev));
	ev.type = EVENT_DEPART;
	ev.actor = in_msg->sender;
	ev.piece_id = piece_id;
	ev.x = fx;
	ev.y = fy;
	emit_event(b, &ev);

	init_msg_header(out_msg, MSG_COLLISION_BATCH, b->side);
	if (!check_enemy_path(b, piece_id, fx, fy, -1, -1, -1, out_msg))
		return 0;

	populate_y_draw_fields(b, out_msg);
	*has_reply = 1;

	if (out_msg->u.collisions.count == 0) {
		finish_turn(b);
		if (b->result == RESULT_ONGOING && is_y_draw_ready(b)) {
			out_msg->y_draw_agreed = 1;
			trigger_game_over(b, RESULT_DRAW);
		}
	} else {
		b->phase = PHASE_RESOLVING;
	}
	return 1;
}

static int handle_collision_batch_msg(struct player_board *b,
				      const struct dmost_msg *in_msg,
				      struct dmost_msg *out_msg, int *has_reply)
{
	int i, count;

	if (b->phase != PHASE_RESOLVING || in_msg->sender == b->side)
		return 0;

	ingest_y_draw_fields(b, in_msg);
	count = in_msg->u.collisions.count;
	if (count < 0 || count > MAX_COLLISIONS_PER_BATCH)
		return 0;

	if (count == 0) {
		finish_turn(b);
		if (b->result == RESULT_ONGOING &&
		    (in_msg->y_draw_agreed || is_y_draw_ready(b))) {
			trigger_game_over(b, RESULT_DRAW);
			if (!in_msg->y_draw_agreed) {
				init_msg_header(out_msg, MSG_DRAW_NOTIFY, b->side);
				populate_y_draw_fields(b, out_msg);
				out_msg->y_draw_agreed = 1;
				*has_reply = 1;
				return 1;
			}
		}
		*has_reply = 0;
		return 1;
	}

	for (i = 0; i < count; i++)
		apply_incoming_mover_collision(b, &in_msg->u.collisions.items[i]);

	init_msg_header(out_msg, MSG_COLLISION_BATCH, b->side);

	for (i = 0; i < count; i++) {
		const struct dmost_collision *c = &in_msg->u.collisions.items[i];

		if (c->defender_id < 0 || c->defender_id >= NUM_PIECES)
			return 0;
		/*
		 * Check the revealed enemy defender's incoming trajectory BEFORE
		 * marking it dead from this collision, so any interception along
		 * its transit path is still detected.
		 */
		if (!check_enemy_path(b, c->defender_id, c->x, c->y,
				      c->mover_id, c->x, c->y, out_msg))
			return 0;
		if (c->defender_died)
			b->enemy[c->defender_id].alive = 0;
	}

	populate_y_draw_fields(b, out_msg);
	*has_reply = 1;

	if (out_msg->u.collisions.count == 0) {
		finish_turn(b);
		if (b->result == RESULT_ONGOING && is_y_draw_ready(b)) {
			out_msg->y_draw_agreed = 1;
			trigger_game_over(b, RESULT_DRAW);
		}
	} else {
		b->phase = PHASE_RESOLVING;
	}
	return 1;
}

static int handle_pass_msg(struct player_board *b,
			   const struct dmost_msg *in_msg,
			   struct dmost_msg *out_msg, int *has_reply)
{
	struct dmost_event ev;

	if (b->phase != PHASE_PLAYING || b->current_turn != in_msg->sender ||
	    in_msg->sender == b->side)
		return 0;

	b->consecutive_passes++;
	ingest_y_draw_fields(b, in_msg);

	memset(&ev, 0, sizeof(ev));
	ev.type = EVENT_PASS;
	ev.actor = in_msg->sender;
	emit_event(b, &ev);

	evaluate_board_result(b);
	if (b->result != RESULT_ONGOING) {
		trigger_game_over(b, b->result);
		*has_reply = 0;
		return 1;
	}
	if (b->consecutive_passes >= 2) {
		trigger_game_over(b, RESULT_DRAW);
		*has_reply = 0;
		return 1;
	}
	b->current_turn = b->side;
	b->turn_number++;
	if (is_y_draw_ready(b)) {
		trigger_game_over(b, RESULT_DRAW);
		init_msg_header(out_msg, MSG_DRAW_NOTIFY, b->side);
		populate_y_draw_fields(b, out_msg);
		out_msg->y_draw_agreed = 1;
		*has_reply = 1;
		return 1;
	}

	*has_reply = 0;
	return 1;
}

int dmost_handle_message(struct player_board *b,
			 const struct dmost_msg *in_msg,
			 struct dmost_msg *out_msg, int *has_reply)
{
	*has_reply = 0;

	switch (in_msg->type) {
	case MSG_READY:
		if (in_msg->sender == b->side || b->enemy_ready)
			return 0;
		b->enemy_ready = 1;
		if (b->own_ready && b->phase == PHASE_SETUP)
			b->phase = PHASE_PLAYING;
		return 1;

	case MSG_DEPART:
		return handle_depart_msg(b, in_msg, out_msg, has_reply);

	case MSG_COLLISION_BATCH:
		return handle_collision_batch_msg(b, in_msg, out_msg, has_reply);

	case MSG_PASS:
		return handle_pass_msg(b, in_msg, out_msg, has_reply);

	case MSG_DRAW_NOTIFY:
		if (in_msg->sender == b->side)
			return 0;
		ingest_y_draw_fields(b, in_msg);
		if (b->phase != PHASE_GAME_OVER && in_msg->y_draw_agreed)
			trigger_game_over(b, RESULT_DRAW);
		return 1;

	default:
		return 0;
	}
}

void dmost_format_coord(int x, int y, char *buf, int bufsize)
{
	if (!in_bounds(x, y)) {
		snprintf(buf, (size_t)bufsize, "??");
		return;
	}
	snprintf(buf, (size_t)bufsize, "%c%d", 'A' + y, x + 1);
}

void dmost_format_event(const struct dmost_event *ev, enum player_side viewer,
			char *buf, int bufsize)
{
	char coord[8];
	const char *actor_name = (ev->actor == SIDE_RED) ? "Red" : "Blue";
	const char *p1_name = (ev->piece_id >= 0 && ev->piece_id < NUM_PIECES) ?
			      dmost_piece_info[ev->piece_id].name : "?";
	const char *p2_name = (ev->other_piece_id >= 0 &&
			       ev->other_piece_id < NUM_PIECES) ?
			      dmost_piece_info[ev->other_piece_id].name : "?";

	(void)viewer;

	switch (ev->type) {
	case EVENT_DEPART:
		dmost_format_coord(ev->x, ev->y, coord, sizeof(coord));
		snprintf(buf, (size_t)bufsize, "%s %s departs %s",
			 actor_name, p1_name, coord);
		break;
	case EVENT_COLLISION:
		dmost_format_coord(ev->x, ev->y, coord, sizeof(coord));
		snprintf(buf, (size_t)bufsize,
			 "Collision at %s: %s %s (%s) vs %s %s (%s)",
			 coord,
			 actor_name, p1_name,
			 ev->actor_died ? "destroyed" : "survived",
			 (ev->actor == SIDE_RED) ? "Blue" : "Red", p2_name,
			 ev->other_died ? "destroyed" : "survived");
		break;
	case EVENT_PASS:
		snprintf(buf, (size_t)bufsize, "%s passes (no legal moves)",
			 actor_name);
		break;
	case EVENT_GAME_OVER:
		switch (ev->result) {
		case RESULT_RED_WINS:
			snprintf(buf, (size_t)bufsize, "Game Over: Red wins!");
			break;
		case RESULT_BLUE_WINS:
			snprintf(buf, (size_t)bufsize, "Game Over: Blue wins!");
			break;
		case RESULT_DRAW:
			snprintf(buf, (size_t)bufsize, "Game Over: Draw!");
			break;
		default:
			snprintf(buf, (size_t)bufsize, "Game Over");
			break;
		}
		break;
	default:
		snprintf(buf, (size_t)bufsize, "Unknown event");
		break;
	}
}
