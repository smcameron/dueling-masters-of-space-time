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
#include "ai.h"

#include <stdlib.h>
#include <string.h>

#define AI_OUTBOX_CAP 32

enum piece_ids {
	AI_ID_W = 0,
	AI_ID_B = 1,
	AI_ID_X1 = 2,
	AI_ID_X2 = 3,
	AI_ID_Y1 = 4,
	AI_ID_Y2 = 5,
	AI_ID_Z1 = 6,
	AI_ID_Z2 = 7,
	AI_ID_Z3 = 8,
	AI_ID_Z4 = 9,
	AI_ID_ASTEROID_START = 10,
	AI_ID_ASTEROID_END = 16,
	AI_ID_HP = PIECE_INDEX_HP,
};

struct ai_ctx {
	struct player_board board;
	uint32_t rng_state;

	struct dmost_msg outbox[AI_OUTBOX_CAP];
	int out_head;
	int out_count;

	/* Belief / tracking updated strictly from messages and own events */
	int enemy_depart_turn[NUM_PIECES];
	int own_last_from_x[NUM_PIECES];
	int own_last_from_y[NUM_PIECES];
	int own_ambush_until_turn[NUM_PIECES];
};

static uint32_t rng_next(uint32_t *state)
{
	uint32_t x = *state;

	if (x == 0)
		x = 0x9e3779b9u;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;
	return x;
}

static int rng_range(uint32_t *state, int n)
{
	if (n <= 1)
		return 0;
	return (int)(rng_next(state) % (uint32_t)n);
}

static void shuffle_ints(int *arr, int n, uint32_t *rng_state)
{
	int i;

	for (i = n - 1; i > 0; i--) {
		int j = rng_range(rng_state, i + 1);
		int tmp = arr[i];

		arr[i] = arr[j];
		arr[j] = tmp;
	}
}

static void shuffle_squares(struct square *arr, int n, uint32_t *rng_state)
{
	int i;

	for (i = n - 1; i > 0; i--) {
		int j = rng_range(rng_state, i + 1);
		struct square tmp = arr[i];

		arr[i] = arr[j];
		arr[j] = tmp;
	}
}

static int outbox_push(struct ai_ctx *ctx, const struct dmost_msg *msg)
{
	int tail;

	if (ctx->out_count >= AI_OUTBOX_CAP)
		return 0;
	tail = (ctx->out_head + ctx->out_count) % AI_OUTBOX_CAP;
	ctx->outbox[tail] = *msg;
	ctx->out_count++;
	return 1;
}

static int outbox_pop(struct ai_ctx *ctx, struct dmost_msg *out_msg)
{
	if (ctx->out_count <= 0)
		return 0;
	*out_msg = ctx->outbox[ctx->out_head];
	ctx->out_head = (ctx->out_head + 1) % AI_OUTBOX_CAP;
	ctx->out_count--;
	return 1;
}

/*
 * Counts how many valid moves piece_id would have from (x, y) given the
 * currently placed own asteroids and HP on the board during setup.
 */
static int count_setup_mobility(const struct player_board *b, int piece_id,
				int x, int y)
{
	enum piece_type type = b->own[piece_id].type;
	struct square path[MAX_PATH_LEN];
	int tx, ty, count = 0;

	for (tx = 0; tx < BOARD_SIZE; tx++) {
		for (ty = 0; ty < BOARD_SIZE; ty++) {
			int len, k, blocked = 0;

			len = dmost_compute_path(type, x, y, tx, ty, path,
						 MAX_PATH_LEN);
			if (len < 2)
				continue;
			for (k = 1; k < len && !blocked; k++) {
				int j;

				for (j = AI_ID_ASTEROID_START; j < NUM_PIECES; j++) {
					if (b->own[j].x == path[k].x &&
					    b->own[j].y == path[k].y) {
						blocked = 1;
						break;
					}
				}
			}
			if (!blocked)
				count++;
		}
	}
	return count;
}

static int row_asteroid_count(const struct player_board *b, int y)
{
	int i, count = 0;

	for (i = AI_ID_ASTEROID_START; i <= AI_ID_ASTEROID_END; i++) {
		if (b->own[i].y == y)
			count++;
	}
	return count;
}

static int place_best_from_candidates(struct player_board *b, int piece_id,
				      const struct square *cands, int num_cands,
				      int required_parity, int min_mobility,
				      uint32_t *rng_state)
{
	int best_x = -1, best_y = -1;
	int best_score = -1;
	int i;

	for (i = 0; i < num_cands; i++) {
		int x = cands[i].x;
		int y = cands[i].y;
		int mob, score;

		if (required_parity >= 0 && ((x + y) & 1) != required_parity)
			continue;
		if (!dmost_can_place_piece(b, piece_id, x, y))
			continue;

		mob = count_setup_mobility(b, piece_id, x, y);
		if (mob < min_mobility)
			continue;

		score = mob * 4 + rng_range(rng_state, 5);
		if (score > best_score) {
			best_score = score;
			best_x = x;
			best_y = y;
		}
	}

	if (best_x >= 0)
		return dmost_place_piece(b, piece_id, best_x, best_y);
	return 0;
}

static int place_mobile_with_fallback(struct player_board *b, int piece_id,
				      const struct square *cands, int num_cands,
				      int required_parity, uint32_t *rng_state)
{
	struct square all_own[50];
	int x, y, n = 0;

	if (place_best_from_candidates(b, piece_id, cands, num_cands,
				       required_parity, 1, rng_state))
		return 1;

	/* Fallback: all squares in own territory, matching parity if needed */
	for (y = 0; y < BOARD_SIZE; y++) {
		if (!dmost_side_owns_row(b->side, y))
			continue;
		for (x = 0; x < BOARD_SIZE; x++) {
			all_own[n].x = x;
			all_own[n].y = y;
			n++;
		}
	}
	shuffle_squares(all_own, n, rng_state);

	if (place_best_from_candidates(b, piece_id, all_own, n,
				       required_parity, 1, rng_state))
		return 1;
	if (place_best_from_candidates(b, piece_id, all_own, n, -1, 1, rng_state))
		return 1;
	return place_best_from_candidates(b, piece_id, all_own, n, -1, 0, rng_state);
}

int dmost_ai_setup_board(struct player_board *b, uint32_t *rng_state)
{
	int back_y, dir;
	int hp_x, hp_y, hp_row_off;
	int i, ast_idx = AI_ID_ASTEROID_START;
	int shield_target;
	struct square neighbors[8];
	int n_neighbors = 0;
	struct square mid_squares[40];
	int n_mid = 0;
	int zones[4] = { 0, 1, 2, 3 };
	int x, y;
	static const int nb_offsets[8][2] = {
		{ -1, 1 }, { 0, 1 }, { 1, 1 },
		{ -1, 0 }, { 1, 0 },
		{ -1, -1 }, { 0, -1 }, { 1, -1 }
	};

	if (b->phase != PHASE_SETUP || b->own_ready)
		return 0;

	for (i = 0; i < NUM_PIECES; i++)
		dmost_unplace_piece(b, i);

	back_y = (b->side == SIDE_RED) ? 0 : 9;
	dir = (b->side == SIDE_RED) ? 1 : -1;

	/* 1. Place Home Planet in back 2 rows */
	hp_row_off = (rng_range(rng_state, 10) < 7) ? 0 : 1;
	hp_y = back_y + hp_row_off * dir;
	hp_x = rng_range(rng_state, BOARD_SIZE);
	if (!dmost_place_piece(b, AI_ID_HP, hp_x, hp_y))
		return 0;

	/* 2. Place 4-5 Asteroids shielding HP's immediate neighborhood */
	for (i = 0; i < 8; i++) {
		int nx = hp_x + nb_offsets[i][0];
		int ny = hp_y + nb_offsets[i][1] * dir;

		if (nx >= 0 && nx < BOARD_SIZE &&
		    dmost_side_owns_row(b->side, ny)) {
			neighbors[n_neighbors].x = nx;
			neighbors[n_neighbors].y = ny;
			n_neighbors++;
		}
	}
	/* Shuffle front neighbors (first 3 if valid) and side/back separately */
	shuffle_squares(neighbors, n_neighbors, rng_state);

	shield_target = 4 + rng_range(rng_state, 2); /* 4 or 5 */
	for (i = 0; i < n_neighbors && ast_idx <= AI_ID_ASTEROID_END &&
	     (ast_idx - AI_ID_ASTEROID_START) < shield_target; i++) {
		if (row_asteroid_count(b, neighbors[i].y) >= 3)
			continue;
		if (dmost_place_piece(b, ast_idx, neighbors[i].x, neighbors[i].y))
			ast_idx++;
	}

	/* 3. Place remaining Asteroids in mid/front territory as spread-out mines */
	for (i = 1; i <= 4; i++) {
		y = back_y + i * dir;
		for (x = 0; x < BOARD_SIZE; x++) {
			mid_squares[n_mid].x = x;
			mid_squares[n_mid].y = y;
			n_mid++;
		}
	}
	shuffle_squares(mid_squares, n_mid, rng_state);
	for (i = 0; i < n_mid && ast_idx <= AI_ID_ASTEROID_END; i++) {
		int mx = mid_squares[i].x;
		int my = mid_squares[i].y;

		if (my == back_y + 4 * dir && rng_range(rng_state, 3) != 0)
			continue; /* Prefer rows 1..3 slightly over front row */
		if (row_asteroid_count(b, my) >= 3)
			continue;
		if (dmost_place_piece(b, ast_idx, mx, my))
			ast_idx++;
	}
	/* Ensure all remaining asteroids are placed even if row cap skipped some */
	for (i = 0; i < n_mid && ast_idx <= AI_ID_ASTEROID_END; i++) {
		if (dmost_place_piece(b, ast_idx, mid_squares[i].x, mid_squares[i].y))
			ast_idx++;
	}

	/* 4. Place Z1..Z4 scouts in front rows (row_off 3..4) across 4 zones */
	shuffle_ints(zones, 4, rng_state);
	for (i = 0; i < 4; i++) {
		struct square cands[10];
		int nc = 0, zx_min, zx_max, r_off;

		zx_min = (zones[i] * BOARD_SIZE) / 4;
		zx_max = ((zones[i] + 1) * BOARD_SIZE) / 4;
		for (r_off = 3; r_off <= 4; r_off++) {
			y = back_y + r_off * dir;
			for (x = zx_min; x < zx_max; x++) {
				cands[nc].x = x;
				cands[nc].y = y;
				nc++;
			}
		}
		shuffle_squares(cands, nc, rng_state);
		if (!place_mobile_with_fallback(b, AI_ID_Z1 + i, cands, nc, -1,
						rng_state))
			return 0;
	}

	/* 5. Place Y1 (even parity) and Y2 (odd parity) on rows 2..4 */
	for (i = 0; i < 2; i++) {
		struct square cands[30];
		int nc = 0, r_off;

		for (r_off = 2; r_off <= 4; r_off++) {
			y = back_y + r_off * dir;
			for (x = 0; x < BOARD_SIZE; x++) {
				cands[nc].x = x;
				cands[nc].y = y;
				nc++;
			}
		}
		shuffle_squares(cands, nc, rng_state);
		if (!place_mobile_with_fallback(b, AI_ID_Y1 + i, cands, nc, i,
						rng_state))
			return 0;
	}

	/* 6. Place X1, X2 rooks on rows 2..4, favoring open columns */
	for (i = 0; i < 2; i++) {
		struct square cands[30];
		int nc = 0, r_off;

		for (r_off = 2; r_off <= 4; r_off++) {
			y = back_y + r_off * dir;
			for (x = 0; x < BOARD_SIZE; x++) {
				cands[nc].x = x;
				cands[nc].y = y;
				nc++;
			}
		}
		shuffle_squares(cands, nc, rng_state);
		if (!place_mobile_with_fallback(b, AI_ID_X1 + i, cands, nc, -1,
						rng_state))
			return 0;
	}

	/* 7. Place B and W in central defensive/counter-attack rows (row_off 1..3) */
	for (i = 0; i < 2; i++) {
		struct square cands[30];
		int nc = 0, r_off;
		int id = (i == 0) ? AI_ID_B : AI_ID_W;

		for (r_off = 1; r_off <= 3; r_off++) {
			y = back_y + r_off * dir;
			for (x = 1; x <= 8; x++) {
				cands[nc].x = x;
				cands[nc].y = y;
				nc++;
			}
		}
		shuffle_squares(cands, nc, rng_state);
		if (!place_mobile_with_fallback(b, id, cands, nc, -1, rng_state))
			return 0;
	}

	return dmost_setup_is_complete(b);
}

static void ai_on_event(void *ctx_ptr, const struct dmost_event *ev)
{
	struct ai_ctx *ctx = ctx_ptr;
	enum player_side enemy_side = dmost_other_side(ctx->board.side);

	if (ev->type == EVENT_DEPART && ev->actor == enemy_side &&
	    ev->piece_id >= 0 && ev->piece_id < NUM_PIECES) {
		ctx->enemy_depart_turn[ev->piece_id] = ctx->board.turn_number;
	} else if (ev->type == EVENT_COLLISION) {
		int enemy_id = (ev->actor == enemy_side) ?
			       ev->piece_id : ev->other_piece_id;

		if (enemy_id >= 0 && enemy_id < NUM_PIECES) {
			ctx->enemy_depart_turn[enemy_id] = 0;
			if (dmost_piece_info[enemy_id].type == PIECE_ASTEROID &&
			    ev->x >= 0 && ev->x < BOARD_SIZE &&
			    ev->y >= 0 && ev->y < BOARD_SIZE) {
				/* Destroyed asteroid square cannot hold enemy HP */
				ctx->board.enemy_clear[ev->x][ev->y] = 1;
			}
		}
	}
}

static double piece_value(enum piece_type t)
{
	switch (t) {
	case PIECE_ZFIGHTER: return 1.0;
	case PIECE_YFIGHTER: return 2.2;
	case PIECE_XFIGHTER: return 3.0;
	case PIECE_BFIGHTER: return 5.5;
	case PIECE_WFIGHTER: return 7.0;
	default: return 0.0;
	}
}

struct ai_tactical_maps {
	uint8_t unexplored_enemy[BOARD_SIZE][BOARD_SIZE];
	uint8_t pending_cover[BOARD_SIZE][BOARD_SIZE];
	double intercept_bonus[NUM_PIECES][BOARD_SIZE][BOARD_SIZE];
	double threat_risk[NUM_PIECES][BOARD_SIZE][BOARD_SIZE];
	int unexplored_count;
	int own_z_alive;
	int own_mobile_alive;
	int enemy_asteroids_alive;
	double asteroid_risk;
};

static double enemy_row_weight(enum player_side ai_side, int y)
{
	int dist_from_enemy_back;

	if (dmost_side_owns_row(ai_side, y))
		return 0.0;
	dist_from_enemy_back = (ai_side == SIDE_RED) ? (9 - y) : y;
	if (dist_from_enemy_back <= 1)
		return 1.35;
	return 1.0;
}

static void build_tactical_maps(const struct ai_ctx *ctx,
				struct ai_tactical_maps *m)
{
	const struct player_board *b = &ctx->board;
	enum player_side enemy_side = dmost_other_side(b->side);
	int hp_x = b->own[AI_ID_HP].x;
	int hp_y = b->own[AI_ID_HP].y;
	int i, x, y;

	memset(m, 0, sizeof(*m));

	for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++) {
			if (dmost_side_owns_row(enemy_side, y) &&
			    !b->enemy_clear[x][y]) {
				m->unexplored_enemy[x][y] = 1;
				m->unexplored_count++;
			}
		}
	}

	for (i = 0; i < NUM_PIECES; i++) {
		const struct own_piece *p = &b->own[i];

		if (!p->alive)
			continue;
		if (dmost_piece_is_mobile(p->type))
			m->own_mobile_alive++;
		if (p->type == PIECE_ZFIGHTER)
			m->own_z_alive++;

		if (p->in_transit && p->announced_x != UNKNOWN_POS) {
			struct square path[MAX_PATH_LEN];
			int len, k;

			len = dmost_compute_path(p->type, p->announced_x,
						 p->announced_y, p->x, p->y,
						 path, MAX_PATH_LEN);
			for (k = 1; k < len; k++)
				m->pending_cover[path[k].x][path[k].y] = 1;
		}
	}

	for (i = AI_ID_ASTEROID_START; i <= AI_ID_ASTEROID_END; i++) {
		if (b->enemy[i].alive)
			m->enemy_asteroids_alive++;
	}

	m->asteroid_risk = (m->unexplored_count > 0) ?
		((double)m->enemy_asteroids_alive / (double)m->unexplored_count) :
		0.0;
	if (m->asteroid_risk > 0.5)
		m->asteroid_risk = 0.5;

	/* Project potential positions of recently announced alive enemy ships */
	for (i = 0; i < NUM_PIECES; i++) {
		const struct known_enemy_piece *e = &b->enemy[i];
		struct square path[MAX_PATH_LEN];
		int age, kx, ky;

		if (!e->alive || !dmost_piece_is_mobile(e->type))
			continue;
		if (e->known_x == UNKNOWN_POS || ctx->enemy_depart_turn[i] <= 0)
			continue;
		age = b->turn_number - ctx->enemy_depart_turn[i];
		if (age < 0 || age > 6)
			continue;

		kx = e->known_x;
		ky = e->known_y;

		for (x = 0; x < BOARD_SIZE; x++) {
			for (y = 0; y < BOARD_SIZE; y++) {
				int reachable = 0;
				double decay = 1.0 / (double)(age + 1);
				int dist_hp = abs(x - hp_x) + abs(y - hp_y);
				int own_id;

				if (x == kx && y == ky) {
					if (age <= 2)
						reachable = 1;
				} else if (dmost_compute_path(e->type, kx, ky,
							      x, y, path,
							      MAX_PATH_LEN) >= 2) {
					reachable = 1;
				}
				if (!reachable)
					continue;

				for (own_id = 0; own_id < NUM_PIECES; own_id++) {
					enum piece_type ot = b->own[own_id].type;
					double val;

					if (!b->own[own_id].alive ||
					    !dmost_piece_is_mobile(ot))
						continue;
					val = piece_value(ot);
					if (ot > e->type) {
						double bonus = (12.0 + 4.0 * piece_value(e->type)) * decay;

						if (dmost_side_owns_row(b->side, y))
							bonus *= 1.5;
						if (dist_hp <= 3)
							bonus += 10.0 * decay;
						if (bonus > m->intercept_bonus[own_id][x][y])
							m->intercept_bonus[own_id][x][y] = bonus;
					} else if (ot < e->type) {
						double risk = (10.0 + 3.5 * val) * decay;

						if (risk > m->threat_risk[own_id][x][y])
							m->threat_risk[own_id][x][y] = risk;
					} else {
						double risk = (4.0 + 2.0 * val) * decay;

						if (risk > m->threat_risk[own_id][x][y])
							m->threat_risk[own_id][x][y] = risk;
					}
				}
			}
		}
	}
}

static double score_candidate_move(struct ai_ctx *ctx,
				   const struct ai_tactical_maps *m,
				   int id, int to_x, int to_y,
				   int *is_ambush_move)
{
	const struct player_board *b = &ctx->board;
	const struct own_piece *p = &b->own[id];
	enum player_side enemy_side = dmost_other_side(b->side);
	struct square path[MAX_PATH_LEN];
	int len, k;
	double score = 0.0;
	double prev_unexp = 0.0;
	double new_unexp = 0.0;
	double unexp_risk_cells = 0.0;
	double pval = piece_value(p->type);
	double role_explore_weight;
	double risk_weight;
	int min_dist = 999;
	int aligned_unexp = 0;
	int x, y;

	*is_ambush_move = 0;

	/* 1. Confirming an existing in-transit sweep */
	if (p->in_transit && p->announced_x != UNKNOWN_POS) {
		struct square prev_path[MAX_PATH_LEN];
		int prev_len = dmost_compute_path(p->type, p->announced_x,
						  p->announced_y, p->x, p->y,
						  prev_path, MAX_PATH_LEN);
		for (k = 1; k < prev_len; k++) {
			int px = prev_path[k].x;
			int py = prev_path[k].y;

			if (m->unexplored_enemy[px][py])
				prev_unexp += enemy_row_weight(b->side, py);
		}
		if (prev_unexp > 0.0) {
			double urgency = (m->unexplored_count <= 15) ? 1.5 : 1.0;

			score += prev_unexp * 35.0 * urgency;
		} else if (b->turn_number <= ctx->own_ambush_until_turn[id]) {
			/* Let ambushers hold position briefly unless needed */
			score -= 8.0;
		}
	}

	/* 2. New squares swept by this move */
	len = dmost_compute_path(p->type, p->x, p->y, to_x, to_y,
				 path, MAX_PATH_LEN);
	for (k = 1; k < len; k++) {
		int sx = path[k].x;
		int sy = path[k].y;

		if (m->unexplored_enemy[sx][sy]) {
			double rw = enemy_row_weight(b->side, sy);
			double cover_mult = m->pending_cover[sx][sy] ? 0.15 : 1.0;

			new_unexp += rw * cover_mult;
			unexp_risk_cells += 1.0;
		} else if (dmost_side_owns_row(b->side, sy) &&
			   !b->own_visited[sx][sy] && p->announced_x != UNKNOWN_POS) {
			/* Slight preference not to reveal safe lanes in own territory */
			score -= 0.4;
		}
	}

	/* Role-based exploration vs caution */
	if (p->type == PIECE_ZFIGHTER) {
		role_explore_weight = 14.0;
		risk_weight = 1.0;
	} else if (p->type == PIECE_YFIGHTER || p->type == PIECE_XFIGHTER) {
		role_explore_weight = (m->own_z_alive > 0) ? 7.5 : 13.0;
		risk_weight = (m->own_z_alive > 0) ? 1.2 : 0.45;
	} else {
		/* W and B act as interceptors early, close out when scouts gone */
		role_explore_weight = (m->own_z_alive > 0) ? 4.0 : 12.0;
		risk_weight = (m->own_z_alive > 0) ? 1.4 : 0.25;
		if (m->own_mobile_alive <= 3 || m->unexplored_count <= 14)
			risk_weight *= 0.2;
	}

	score += new_unexp * role_explore_weight;
	score -= unexp_risk_cells * m->asteroid_risk * pval * 6.0 * risk_weight;

	/* 3. Interception & threat dodging */
	if (m->intercept_bonus[id][to_x][to_y] > 0.0) {
		score += m->intercept_bonus[id][to_x][to_y];
		if (m->intercept_bonus[id][to_x][to_y] >= 8.0 && new_unexp < 0.5)
			*is_ambush_move = 1;
	}
	score -= m->threat_risk[id][to_x][to_y];

	/* Bonus for moving a valuable threatened piece off its current square */
	if (p->announced_x != UNKNOWN_POS &&
	    m->threat_risk[id][p->x][p->y] > 3.0 &&
	    m->threat_risk[id][to_x][to_y] < m->threat_risk[id][p->x][p->y]) {
		score += (m->threat_risk[id][p->x][p->y] -
			  m->threat_risk[id][to_x][to_y]) * 0.8;
	}

	/* 4. Navigation shaping toward reachable unexplored enemy squares */
	if (m->unexplored_count > 0 && new_unexp < 0.5) {
		int target_parity = (p->type == PIECE_YFIGHTER) ?
				    ((to_x + to_y) & 1) : -1;

		for (x = 0; x < BOARD_SIZE; x++) {
			for (y = 0; y < BOARD_SIZE; y++) {
				int d;

				if (!m->unexplored_enemy[x][y])
					continue;
				if (target_parity >= 0 &&
				    ((x + y) & 1) != target_parity)
					continue;
				d = abs(to_x - x) + abs(to_y - y);
				if (d < min_dist)
					min_dist = d;
				if (p->type == PIECE_XFIGHTER &&
				    (to_x == x || to_y == y))
					aligned_unexp = 1;
				else if (p->type == PIECE_YFIGHTER &&
					 abs(to_x - x) == abs(to_y - y))
					aligned_unexp = 1;
			}
		}
		if (min_dist < 999) {
			score -= (double)min_dist * 1.4;
			if (aligned_unexp)
				score += 2.5;
		}
	}

	/* 5. Discourage stepping onto enemy side only to immediately retreat */
	if (dmost_side_owns_row(enemy_side, p->y) &&
	    dmost_side_owns_row(b->side, to_y) && prev_unexp < 0.5 &&
	    m->intercept_bonus[id][to_x][to_y] < 6.0)
		score -= 4.0;

	/* 6. Anti-oscillation penalty unless confirming unexplored transit */
	if (to_x == ctx->own_last_from_x[id] &&
	    to_y == ctx->own_last_from_y[id] && prev_unexp < 0.5)
		score -= 10.0;

	/* 7. Small jitter for unpredictable play */
	score += (double)rng_range(&ctx->rng_state, 150) / 100.0;

	return score;
}

static int ai_choose_and_initiate_move(struct ai_ctx *ctx,
				       struct dmost_msg *out_msg)
{
	struct ai_tactical_maps maps;
	int best_id = -1, best_x = -1, best_y = -1;
	int best_is_ambush = 0;
	double best_score = -1e18;
	int id, x, y;

	if (!dmost_has_any_legal_move(&ctx->board))
		return dmost_initiate_pass(&ctx->board, out_msg);

	build_tactical_maps(ctx, &maps);

	for (id = 0; id < NUM_PIECES; id++) {
		if (!ctx->board.own[id].alive ||
		    !dmost_piece_is_mobile(ctx->board.own[id].type))
			continue;

		for (x = 0; x < BOARD_SIZE; x++) {
			for (y = 0; y < BOARD_SIZE; y++) {
				int is_ambush = 0;
				double sc;

				if (!dmost_is_legal_move(&ctx->board, id, x, y))
					continue;
				sc = score_candidate_move(ctx, &maps, id, x, y,
							  &is_ambush);
				if (sc > best_score) {
					best_score = sc;
					best_id = id;
					best_x = x;
					best_y = y;
					best_is_ambush = is_ambush;
				}
			}
		}
	}

	if (best_id < 0)
		return dmost_initiate_pass(&ctx->board, out_msg);

	ctx->own_last_from_x[best_id] = ctx->board.own[best_id].x;
	ctx->own_last_from_y[best_id] = ctx->board.own[best_id].y;
	if (best_is_ambush)
		ctx->own_ambush_until_turn[best_id] = ctx->board.turn_number + 2;
	else
		ctx->own_ambush_until_turn[best_id] = 0;

	return dmost_initiate_move(&ctx->board, best_id, best_x, best_y,
				   out_msg);
}

static int ai_send_msg(struct dmost_opponent *opp, const struct dmost_msg *msg)
{
	struct ai_ctx *ctx = opp->priv;
	struct dmost_msg reply;
	int has_reply = 0;

	if (!dmost_handle_message(&ctx->board, msg, &reply, &has_reply))
		return 0;
	if (has_reply) {
		if (!outbox_push(ctx, &reply))
			return 0;
	}
	return 1;
}

static int ai_poll_msg(struct dmost_opponent *opp, struct dmost_msg *out_msg)
{
	struct ai_ctx *ctx = opp->priv;

	if (outbox_pop(ctx, out_msg))
		return 1;

	if (ctx->board.phase == PHASE_PLAYING &&
	    ctx->board.current_turn == ctx->board.side) {
		if (!ai_choose_and_initiate_move(ctx, out_msg))
			return -1;
		return 1;
	}
	return 0;
}

static int ai_get_fd(struct dmost_opponent *opp)
{
	(void)opp;
	return -1;
}

static void ai_destroy(struct dmost_opponent *opp)
{
	if (opp == NULL)
		return;
	free(opp->priv);
	free(opp);
}

static const struct dmost_opponent_ops ai_ops = {
	.send_msg = ai_send_msg,
	.poll_msg = ai_poll_msg,
	.get_fd = ai_get_fd,
	.destroy = ai_destroy,
};

struct dmost_opponent *dmost_ai_opponent_create(enum player_side ai_side,
						 enum player_side first_turn,
						 uint32_t seed)
{
	struct dmost_opponent *opp;
	struct ai_ctx *ctx;
	struct dmost_msg ready_msg;
	int i;

	opp = calloc(1, sizeof(*opp));
	ctx = calloc(1, sizeof(*ctx));
	if (opp == NULL || ctx == NULL) {
		free(ctx);
		free(opp);
		return NULL;
	}

	ctx->rng_state = (seed != 0) ? seed : 0x9e3779b9u;
	for (i = 0; i < NUM_PIECES; i++) {
		ctx->own_last_from_x[i] = UNKNOWN_POS;
		ctx->own_last_from_y[i] = UNKNOWN_POS;
	}

	dmost_board_init(&ctx->board, ai_side, first_turn);
	dmost_board_set_event_cb(&ctx->board, ai_on_event, ctx);

	if (!dmost_ai_setup_board(&ctx->board, &ctx->rng_state) ||
	    !dmost_mark_ready(&ctx->board, &ready_msg) ||
	    !outbox_push(ctx, &ready_msg)) {
		free(ctx);
		free(opp);
		return NULL;
	}

	opp->ops = &ai_ops;
	opp->priv = ctx;
	return opp;
}

const struct player_board *dmost_ai_get_board(const struct dmost_opponent *opp)
{
	const struct ai_ctx *ctx;

	if (opp == NULL || opp->ops != &ai_ops)
		return NULL;
	ctx = opp->priv;
	return &ctx->board;
}
