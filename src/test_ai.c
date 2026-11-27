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
#include "game.h"
#include "opponent.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int count_adjacent_asteroids_to_hp(const struct player_board *b)
{
	int hp_x = b->own[PIECE_INDEX_HP].x;
	int hp_y = b->own[PIECE_INDEX_HP].y;
	int i, count = 0;

	for (i = 0; i < NUM_PIECES; i++) {
		int dx, dy;

		if (b->own[i].type != PIECE_ASTEROID)
			continue;
		dx = abs(b->own[i].x - hp_x);
		dy = abs(b->own[i].y - hp_y);
		if (dx <= 1 && dy <= 1 && (dx + dy) > 0)
			count++;
	}
	return count;
}

static void test_ai_setup_quality(void)
{
	struct player_board b1, b2;
	int seed, side_idx;
	int diff_count = 0;

	for (side_idx = 0; side_idx < 2; side_idx++) {
		enum player_side side = (side_idx == 0) ? SIDE_RED : SIDE_BLUE;
		int prev_hp_x = -1, prev_hp_y = -1;

		for (seed = 1; seed <= 100; seed++) {
			uint32_t rng = (uint32_t)(seed * 2654435761u);
			struct dmost_msg ready;
			int hp_y, adj, p1, p2, id;

			dmost_board_init(&b1, side, SIDE_RED);
			assert(dmost_ai_setup_board(&b1, &rng) == 1);
			assert(dmost_setup_is_complete(&b1) == 1);

			hp_y = b1.own[PIECE_INDEX_HP].y;
			if (side == SIDE_RED)
				assert(hp_y == 0 || hp_y == 1);
			else
				assert(hp_y == 8 || hp_y == 9);

			adj = count_adjacent_asteroids_to_hp(&b1);
			assert(adj >= 2);

			/* Y1 (id 4) and Y2 (id 5) must cover opposite square parities */
			p1 = (b1.own[4].x + b1.own[4].y) & 1;
			p2 = (b1.own[5].x + b1.own[5].y) & 1;
			assert(p1 != p2);

			if (prev_hp_x >= 0 &&
			    (b1.own[PIECE_INDEX_HP].x != prev_hp_x ||
			     hp_y != prev_hp_y))
				diff_count++;
			prev_hp_x = b1.own[PIECE_INDEX_HP].x;
			prev_hp_y = hp_y;

			/* Verify every mobile piece has at least 1 legal move once started */
			dmost_board_init(&b2, dmost_other_side(side), SIDE_RED);
			rng = (uint32_t)(seed + 999);
			assert(dmost_ai_setup_board(&b2, &rng) == 1);
			assert(dmost_mark_ready(&b2, &ready) == 1);
			{
				struct dmost_msg r2, rep;
				int has_rep = 0;

				assert(dmost_mark_ready(&b1, &r2) == 1);
				assert(dmost_handle_message(&b1, &ready, &rep, &has_rep) == 1);
			}
			b1.current_turn = side;
			for (id = 0; id < NUM_PIECES; id++) {
				int x, y, has_move = 0;

				if (!dmost_piece_is_mobile(b1.own[id].type))
					continue;
				for (x = 0; x < BOARD_SIZE && !has_move; x++) {
					for (y = 0; y < BOARD_SIZE && !has_move; y++) {
						if (dmost_is_legal_move(&b1, id, x, y))
							has_move = 1;
					}
				}
				assert(has_move == 1);
			}
		}
	}
	assert(diff_count > 50);
}

static void pump_turn_resolution(struct dmost_opponent *active,
				 struct dmost_opponent *passive)
{
	const struct player_board *ab = dmost_ai_get_board(active);
	const struct player_board *pb = dmost_ai_get_board(passive);
	int steps = 0;

	while (ab->phase == PHASE_RESOLVING || pb->phase == PHASE_RESOLVING ||
	       ab->phase != pb->phase) {
		struct dmost_msg msg;
		int progressed = 0;

		if ((pb->phase == PHASE_RESOLVING || ab->phase != pb->phase) &&
		    dmost_opponent_poll(passive, &msg) == 1) {
			assert(dmost_opponent_send(active, &msg) == 1);
			progressed = 1;
		}
		if ((ab->phase == PHASE_RESOLVING || ab->phase != pb->phase) &&
		    dmost_opponent_poll(active, &msg) == 1) {
			assert(dmost_opponent_send(passive, &msg) == 1);
			progressed = 1;
		}
		assert(progressed == 1);
		assert(++steps < 64);
	}
}

static int count_enemy_clear(const struct player_board *b,
			     enum player_side target_side)
{
	int x, y, count = 0;

	for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++) {
			if (dmost_side_owns_row(target_side, y) &&
			    b->enemy_clear[x][y])
				count++;
		}
	}
	return count;
}

static void test_ai_tactics_and_transit(void)
{
	struct dmost_opponent *ai_red;
	struct player_board blue;
	struct dmost_msg msg, reply;
	int has_reply = 0;
	int i, cleared;

	/* Test AI as Red vs passive Blue doing safe back-row shuffles */
	ai_red = dmost_ai_opponent_create(SIDE_RED, SIDE_RED, 12345u);
	assert(ai_red != NULL);

	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	for (i = 0; i < 10; i++)
		assert(dmost_place_piece(&blue, i, i, 9));
	for (i = 0; i < 7; i++)
		assert(dmost_place_piece(&blue, 10 + i, i, 8));
	assert(dmost_place_piece(&blue, PIECE_INDEX_HP, 8, 8));
	assert(dmost_mark_ready(&blue, &msg) == 1);
	assert(dmost_opponent_send(ai_red, &msg) == 1);

	/* Exchange READY and play 6 full turn pairs */
	for (i = 0; i < 6; i++) {
		int hops = 0;

		while (dmost_opponent_poll(ai_red, &msg) == 1) {
			assert(++hops < 32);
			assert(dmost_handle_message(&blue, &msg, &reply,
						    &has_reply) == 1);
			if (has_reply)
				assert(dmost_opponent_send(ai_red, &reply) == 1);
		}
		if (blue.phase == PHASE_GAME_OVER)
			break;
		if (blue.current_turn == SIDE_BLUE) {
			int dest_y = (i % 2 == 0) ? 7 : 9;

			if (dmost_is_legal_move(&blue, 9, 9, dest_y))
				assert(dmost_initiate_move(&blue, 9, 9, dest_y, &msg) == 1);
			else
				assert(dmost_initiate_pass(&blue, &msg) == 1);
			assert(dmost_opponent_send(ai_red, &msg) == 1);
			while (dmost_opponent_poll(ai_red, &msg) == 1) {
				assert(dmost_handle_message(&blue, &msg, &reply,
							    &has_reply) == 1);
				if (has_reply)
					assert(dmost_opponent_send(ai_red, &reply) == 1);
			}
		}
	}

	/* By turn 6, AI should have completed at least one transit into Blue territory */
	cleared = count_enemy_clear(dmost_ai_get_board(ai_red), SIDE_BLUE);
	assert(cleared >= 2);
	dmost_opponent_destroy(ai_red);
}

static void test_ai_vs_ai_games(void)
{
	int g;
	int red_wins = 0, blue_wins = 0, draws = 0;
	int total_turns = 0;

	for (g = 0; g < 20; g++) {
		enum player_side first = (g % 2 == 0) ? SIDE_RED : SIDE_BLUE;
		struct dmost_opponent *red_ai =
			dmost_ai_opponent_create(SIDE_RED, first,
						 (uint32_t)(1000 + g * 37));
		struct dmost_opponent *blue_ai =
			dmost_ai_opponent_create(SIDE_BLUE, first,
						 (uint32_t)(5000 + g * 53));
		const struct player_board *rb, *bb;
		int safety = 0;

		assert(red_ai != NULL && blue_ai != NULL);
		rb = dmost_ai_get_board(red_ai);
		bb = dmost_ai_get_board(blue_ai);

		/* Handshake READY messages before starting turn-by-turn play */
		{
			struct dmost_msg r1, r2;

			assert(dmost_opponent_poll(red_ai, &r1) == 1 &&
			       r1.type == MSG_READY);
			assert(dmost_opponent_poll(blue_ai, &r2) == 1 &&
			       r2.type == MSG_READY);
			assert(dmost_opponent_send(blue_ai, &r1) == 1);
			assert(dmost_opponent_send(red_ai, &r2) == 1);
		}
		assert(rb->phase == PHASE_PLAYING);
		assert(bb->phase == PHASE_PLAYING);
		assert(rb->current_turn == first);
		assert(bb->current_turn == first);

		while (rb->phase != PHASE_GAME_OVER &&
		       bb->phase != PHASE_GAME_OVER) {
			struct dmost_opponent *active =
				(rb->current_turn == SIDE_RED) ? red_ai : blue_ai;
			struct dmost_opponent *passive =
				(rb->current_turn == SIDE_RED) ? blue_ai : red_ai;
			struct dmost_msg msg;
			int rc;

			assert(++safety <= 300);

			rc = dmost_opponent_poll(active, &msg);
			assert(rc == 1);
			assert(dmost_opponent_send(passive, &msg) == 1);
			pump_turn_resolution(active, passive);

			assert(rb->phase == bb->phase);
			assert(rb->result == bb->result);
			assert(rb->current_turn == bb->current_turn);
			assert(rb->turn_number == bb->turn_number);
		}

		assert(rb->phase == PHASE_GAME_OVER);
		assert(bb->phase == PHASE_GAME_OVER);
		assert(rb->result == bb->result);
		assert(rb->own[PIECE_INDEX_HP].alive ==
		       bb->enemy[PIECE_INDEX_HP].alive);
		assert(bb->own[PIECE_INDEX_HP].alive ==
		       rb->enemy[PIECE_INDEX_HP].alive);

		total_turns += rb->turn_number - 1;
		if (rb->result == RESULT_RED_WINS)
			red_wins++;
		else if (rb->result == RESULT_BLUE_WINS)
			blue_wins++;
		else if (rb->result == RESULT_DRAW)
			draws++;
		else
			assert(0 && "unexpected RESULT_ONGOING");

		dmost_opponent_destroy(red_ai);
		dmost_opponent_destroy(blue_ai);
	}

	printf("AI vs AI (20 games): Red=%d, Blue=%d, Draw=%d, avg_turns=%.1f\n",
	       red_wins, blue_wins, draws, (double)total_turns / 20.0);
}

int main(void)
{
	test_ai_setup_quality();
	test_ai_tactics_and_transit();
	test_ai_vs_ai_games();
	printf("All AI opponent tests passed!\n");
	return 0;
}
