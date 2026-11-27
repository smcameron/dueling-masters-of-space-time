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
#include "game.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Piece IDs matching dmost_piece_info[] */
enum {
	ID_W = 0,
	ID_B = 1,
	ID_X1 = 2,
	ID_X2 = 3,
	ID_Y1 = 4,
	ID_Y2 = 5,
	ID_Z1 = 6,
	ID_Z2 = 7,
	ID_Z3 = 8,
	ID_Z4 = 9,
	ID_A0 = 10,
	ID_A1 = 11,
	ID_A2 = 12,
	ID_A3 = 13,
	ID_A4 = 14,
	ID_A5 = 15,
	ID_A6 = 16,
	ID_HP = 17,
};

struct event_log {
	int count;
	struct dmost_event items[64];
};

static void record_event(void *ctx, const struct dmost_event *ev)
{
	struct event_log *log = ctx;

	if (log->count < 64)
		log->items[log->count++] = *ev;
}

static void deliver_msg(struct player_board *sender,
			struct player_board *receiver,
			const struct dmost_msg *initial_msg)
{
	struct dmost_msg cur = *initial_msg;
	struct dmost_msg reply;
	struct player_board *from = sender;
	struct player_board *to = receiver;
	int has_reply = 0;
	int hops = 0;

	for (;;) {
		struct player_board *tmp;
		int ok;

		assert(hops < 64);
		ok = dmost_handle_message(to, &cur, &reply, &has_reply);
		assert(ok == 1);
		if (!has_reply)
			break;
		cur = reply;
		tmp = from;
		from = to;
		to = tmp;
		hops++;
	}
}

static void setup_standard_board(struct player_board *b)
{
	int base_y = (b->side == SIDE_RED) ? 0 : 5;
	int i;

	/* Place 10 fighters on row base_y + 2, 7 asteroids on base_y + 1, HP at base_y */
	assert(dmost_place_piece(b, ID_W,  0, base_y + 3));
	assert(dmost_place_piece(b, ID_B,  1, base_y + 3));
	assert(dmost_place_piece(b, ID_X1, 2, base_y + 3));
	assert(dmost_place_piece(b, ID_X2, 3, base_y + 3));
	assert(dmost_place_piece(b, ID_Y1, 4, base_y + 3));
	assert(dmost_place_piece(b, ID_Y2, 5, base_y + 3));
	assert(dmost_place_piece(b, ID_Z1, 6, base_y + 3));
	assert(dmost_place_piece(b, ID_Z2, 7, base_y + 3));
	assert(dmost_place_piece(b, ID_Z3, 8, base_y + 3));
	assert(dmost_place_piece(b, ID_Z4, 9, base_y + 3));
	for (i = 0; i < 7; i++)
		assert(dmost_place_piece(b, ID_A0 + i, i, base_y + 1));
	assert(dmost_place_piece(b, ID_HP, 9, base_y));
	assert(dmost_setup_is_complete(b));
}

static void start_game_with_boards(struct player_board *red,
				   struct player_board *blue)
{
	struct dmost_msg r1, r2;

	assert(dmost_setup_is_complete(red));
	assert(dmost_setup_is_complete(blue));
	assert(dmost_mark_ready(red, &r1));
	deliver_msg(red, blue, &r1);
	assert(dmost_mark_ready(blue, &r2));
	deliver_msg(blue, red, &r2);
	assert(red->phase == PHASE_PLAYING);
	assert(blue->phase == PHASE_PLAYING);
	assert(red->current_turn == blue->current_turn);
}

static void play_move(struct player_board *mover,
		      struct player_board *other,
		      int piece_id, int to_x, int to_y)
{
	struct dmost_msg msg;

	assert(dmost_initiate_move(mover, piece_id, to_x, to_y, &msg));
	deliver_msg(mover, other, &msg);
}

static void assert_boards_in_sync(const struct player_board *red,
				  const struct player_board *blue)
{
	assert(red->phase == blue->phase);
	assert(red->result == blue->result);
	assert(red->current_turn == blue->current_turn);
	assert(red->turn_number == blue->turn_number);
}

static void test_path_geometry(void)
{
	struct square path[MAX_PATH_LEN];
	int len;
	static const int w_deltas[8][2] = {
		{  1,  2 }, { -1,  2 }, {  1, -2 }, { -1, -2 },
		{  2,  1 }, {  2, -1 }, { -2,  1 }, { -2, -1 },
	};
	int i;

	/* All 8 W-fighter L-paths from (4, 4) */
	for (i = 0; i < 8; i++) {
		int dx = w_deltas[i][0];
		int dy = w_deltas[i][1];

		len = dmost_compute_path(PIECE_WFIGHTER, 4, 4, 4 + dx, 4 + dy,
					 path, MAX_PATH_LEN);
		assert(len == 4);
		assert(path[0].x == 4 && path[0].y == 4);
		assert(path[3].x == 4 + dx && path[3].y == 4 + dy);
		if (dy == 2 || dy == -2) {
			int sy = (dy > 0) ? 1 : -1;

			assert(path[1].x == 4 && path[1].y == 4 + sy);
			assert(path[2].x == 4 && path[2].y == 4 + 2 * sy);
		} else {
			int sx = (dx > 0) ? 1 : -1;

			assert(path[1].x == 4 + sx && path[1].y == 4);
			assert(path[2].x == 4 + 2 * sx && path[2].y == 4);
		}
	}

	/* Illegal W-fighter moves */
	assert(dmost_compute_path(PIECE_WFIGHTER, 4, 4, 4, 6, path, MAX_PATH_LEN) == 0);
	assert(dmost_compute_path(PIECE_WFIGHTER, 4, 4, 6, 6, path, MAX_PATH_LEN) == 0);
	assert(dmost_compute_path(PIECE_WFIGHTER, 0, 0, -1, 2, path, MAX_PATH_LEN) == 0);

	/* B & Z queen moves */
	len = dmost_compute_path(PIECE_BFIGHTER, 2, 2, 5, 5, path, MAX_PATH_LEN);
	assert(len == 4);
	assert(path[1].x == 3 && path[1].y == 3);
	assert(path[2].x == 4 && path[2].y == 4);
	len = dmost_compute_path(PIECE_ZFIGHTER, 2, 2, 2, 7, path, MAX_PATH_LEN);
	assert(len == 6);
	assert(dmost_compute_path(PIECE_ZFIGHTER, 2, 2, 3, 4, path, MAX_PATH_LEN) == 0);

	/* X rook moves */
	len = dmost_compute_path(PIECE_XFIGHTER, 1, 3, 4, 3, path, MAX_PATH_LEN);
	assert(len == 4);
	assert(dmost_compute_path(PIECE_XFIGHTER, 1, 3, 2, 4, path, MAX_PATH_LEN) == 0);

	/* Y bishop moves */
	len = dmost_compute_path(PIECE_YFIGHTER, 1, 1, 4, 4, path, MAX_PATH_LEN);
	assert(len == 4);
	assert(dmost_compute_path(PIECE_YFIGHTER, 1, 1, 1, 4, path, MAX_PATH_LEN) == 0);

	/* Immobile pieces and zero-distance */
	assert(dmost_compute_path(PIECE_ASTEROID, 1, 1, 1, 2, path, MAX_PATH_LEN) == 0);
	assert(dmost_compute_path(PIECE_HOMEPLANET, 1, 1, 1, 2, path, MAX_PATH_LEN) == 0);
	assert(dmost_compute_path(PIECE_BFIGHTER, 1, 1, 1, 1, path, MAX_PATH_LEN) == 0);
}

static void test_combat_matrix(void)
{
	enum piece_type types[] = {
		PIECE_HOMEPLANET, PIECE_ASTEROID, PIECE_ZFIGHTER,
		PIECE_YFIGHTER, PIECE_XFIGHTER, PIECE_BFIGHTER, PIECE_WFIGHTER
	};
	int i, j;

	for (i = 0; i < 7; i++) {
		for (j = 0; j < 7; j++) {
			enum piece_type m = types[i];
			enum piece_type d = types[j];
			enum combat_outcome res = dmost_resolve_combat(m, d);

			if (m == PIECE_ASTEROID || d == PIECE_ASTEROID || m == d)
				assert(res == COMBAT_BOTH_DIE);
			else if (m > d)
				assert(res == COMBAT_MOVER_WINS);
			else
				assert(res == COMBAT_DEFENDER_WINS);
		}
	}
}

static void test_setup_validation(void)
{
	struct player_board red, blue;
	struct dmost_msg msg;

	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);

	/* Red cannot place in Blue half, Blue cannot place in Red half */
	assert(dmost_can_place_piece(&red, ID_HP, 0, 5) == 0);
	assert(dmost_can_place_piece(&blue, ID_HP, 0, 4) == 0);

	/* HP and Asteroid must be alone */
	assert(dmost_place_piece(&red, ID_HP, 0, 0) == 1);
	assert(dmost_can_place_piece(&red, ID_Z1, 0, 0) == 0);
	assert(dmost_can_place_piece(&red, ID_A0, 0, 0) == 0);

	assert(dmost_place_piece(&red, ID_A0, 1, 0) == 1);
	assert(dmost_can_place_piece(&red, ID_Z1, 1, 0) == 0);

	/* Two mobile fighters CAN share a square; asteroid cannot join them */
	assert(dmost_place_piece(&red, ID_Z1, 2, 0) == 1);
	assert(dmost_can_place_piece(&red, ID_Z2, 2, 0) == 1);
	assert(dmost_place_piece(&red, ID_Z2, 2, 0) == 1);
	assert(dmost_can_place_piece(&red, ID_A1, 2, 0) == 0);

	/* Incomplete setup cannot mark ready */
	assert(dmost_setup_is_complete(&red) == 0);
	assert(dmost_mark_ready(&red, &msg) == 0);

	/* Complete both and verify transition */
	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	setup_standard_board(&red);
	setup_standard_board(&blue);
	start_game_with_boards(&red, &blue);
}

static void test_own_obstacle_blocking(void)
{
	struct player_board red, blue;

	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);

	setup_standard_board(&red);
	setup_standard_board(&blue);

	/* Reconfigure Red for obstacle checks */
	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	assert(dmost_place_piece(&red, ID_HP, 0, 0));
	assert(dmost_place_piece(&red, ID_A0, 2, 2));
	assert(dmost_place_piece(&red, ID_A1, 1, 4));
	assert(dmost_place_piece(&red, ID_A2, 2, 4));
	assert(dmost_place_piece(&red, ID_A3, 3, 4));
	assert(dmost_place_piece(&red, ID_A4, 4, 4));
	assert(dmost_place_piece(&red, ID_A5, 5, 4));
	assert(dmost_place_piece(&red, ID_A6, 6, 4));

	/* Put W at (2, 0) so (2, 1)->(2, 2)->(3, 2) passes through Asteroid at (2, 2) */
	assert(dmost_place_piece(&red, ID_W,  2, 0));
	/* Put Z1 at (1, 1) and B at (2, 1) so W's path (2,0)->(2,1)->(3,1)->(4,1) is clear */
	assert(dmost_place_piece(&red, ID_B,  2, 1));
	assert(dmost_place_piece(&red, ID_Z1, 1, 1));
	assert(dmost_place_piece(&red, ID_X1, 0, 2));
	assert(dmost_place_piece(&red, ID_X2, 3, 0));
	assert(dmost_place_piece(&red, ID_Y1, 4, 0));
	assert(dmost_place_piece(&red, ID_Y2, 5, 0));
	assert(dmost_place_piece(&red, ID_Z2, 6, 0));
	assert(dmost_place_piece(&red, ID_Z3, 7, 0));
	assert(dmost_place_piece(&red, ID_Z4, 8, 0));

	start_game_with_boards(&red, &blue);

	/* W at (2,0) -> (3,2) goes (2,0)->(2,1)->(2,2)->(3,2): blocked by own A0 at (2,2) */
	assert(dmost_is_legal_move(&red, ID_W, 3, 2) == 0);
	/* W at (2,0) -> (1,2) also passes (2,2): blocked */
	assert(dmost_is_legal_move(&red, ID_W, 1, 2) == 0);
	/* W at (2,0) -> (4,1) goes (2,0)->(3,0)->(4,0)->(4,1): passes over own X2/Y1, legal! */
	assert(dmost_is_legal_move(&red, ID_W, 4, 1) == 1);
	/* X1 at (0,2) -> (4,2) blocked by own A0 at (2,2), but (0,2)->(1,2) is legal */
	assert(dmost_is_legal_move(&red, ID_X1, 4, 2) == 0);
	assert(dmost_is_legal_move(&red, ID_X1, 1, 2) == 1);
	/* X1 at (0,2) -> (0,0) blocked by own HP at (0,0) */
	assert(dmost_is_legal_move(&red, ID_X1, 0, 0) == 0);
}

static void test_basic_turn_and_delayed_transit(void)
{
	struct player_board red, blue;

	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	setup_standard_board(&red);
	setup_standard_board(&blue);
	start_game_with_boards(&red, &blue);

	/* Red moves Z1 from (6,3) to (6,8) — enters transit, Blue does NOT die yet */
	play_move(&red, &blue, ID_Z1, 6, 8);
	assert_boards_in_sync(&red, &blue);
	assert(red.current_turn == SIDE_BLUE);
	assert(red.own[ID_Z1].x == 6 && red.own[ID_Z1].y == 8);
	assert(red.own[ID_Z1].in_transit == 1);
	assert(blue.enemy[ID_Z1].known_x == 6 && blue.enemy[ID_Z1].known_y == 3);
	assert(blue.own[ID_Z1].alive == 1);

	/* Blue moves Z1 from (6,8) to (7,7) — no collision yet (avoids own asteroid at (6,6)) */
	play_move(&blue, &red, ID_Z1, 7, 7);
	assert_boards_in_sync(&red, &blue);
	assert(red.current_turn == SIDE_RED);
}

static void test_html_three_way_collision(void)
{
	/*
	 * Exact 3-way collision from dueling-masters-of-space-time.html:
	 * - Red X1 at C2 (1, 2)
	 * - Blue Y1 moves D3 (2, 3) -> B1 (0, 1)
	 * - Red Z1 moves E1 (0, 4) -> A1 (0, 0)
	 * - Red attempts to move Z1 from A1 (0, 0) -> A5 (4, 0)
	 * Expected: Z1 dies at B1 (0, 1) to Y1; Y1 dies at C2 (1, 2) to X1;
	 *           X1 survives and is revealed at C2 (1, 2); Z1's A1->A5 move is cancelled.
	 */
	struct player_board red, blue;
	struct event_log red_log = { 0 }, blue_log = { 0 };
	int i;

	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	dmost_board_set_event_cb(&red, record_event, &red_log);
	dmost_board_set_event_cb(&blue, record_event, &blue_log);

	/* Setup Red */
	assert(dmost_place_piece(&red, ID_X1, 1, 2)); /* C2 */
	assert(dmost_place_piece(&red, ID_Z1, 0, 4)); /* E1 */
	assert(dmost_place_piece(&red, ID_W,  0, 3));
	assert(dmost_place_piece(&red, ID_B,  2, 0));
	assert(dmost_place_piece(&red, ID_X2, 3, 0));
	assert(dmost_place_piece(&red, ID_Y1, 4, 0));
	assert(dmost_place_piece(&red, ID_Y2, 5, 0));
	assert(dmost_place_piece(&red, ID_Z2, 6, 0));
	assert(dmost_place_piece(&red, ID_Z3, 7, 0));
	assert(dmost_place_piece(&red, ID_Z4, 8, 0));
	for (i = 0; i < 7; i++)
		assert(dmost_place_piece(&red, ID_A0 + i, 2 + i, 1));
	assert(dmost_place_piece(&red, ID_HP, 9, 0));

	/* Setup Blue (put Y1 at F5 (4, 5) first so it can step to D3 (2, 3), then B1 (0, 1)) */
	assert(dmost_place_piece(&blue, ID_Y1, 4, 5));
	assert(dmost_place_piece(&blue, ID_W,  6, 9));
	assert(dmost_place_piece(&blue, ID_B,  1, 9));
	assert(dmost_place_piece(&blue, ID_X1, 2, 9));
	assert(dmost_place_piece(&blue, ID_X2, 3, 9));
	assert(dmost_place_piece(&blue, ID_Y2, 5, 9));
	assert(dmost_place_piece(&blue, ID_Z1, 6, 9));
	assert(dmost_place_piece(&blue, ID_Z2, 7, 9));
	assert(dmost_place_piece(&blue, ID_Z3, 8, 9));
	assert(dmost_place_piece(&blue, ID_Z4, 9, 9));
	for (i = 0; i < 7; i++)
		assert(dmost_place_piece(&blue, ID_A0 + i, 2 + i, 8));
	assert(dmost_place_piece(&blue, ID_HP, 0, 8));

	start_game_with_boards(&red, &blue);

	/* Turn 1 (Red): dummy move Z4 (8,0)->(9,0)? No, HP at (9,0). Move Z1/W/etc: W (0,3)->(1,1) blocked? Move Z4 (8,0)->(9,1) */
	play_move(&red, &blue, ID_Z4, 9, 1);
	/* Turn 2 (Blue): Y1 F5 (4,5) -> D3 (2,3) */
	play_move(&blue, &red, ID_Y1, 2, 3);
	/* Turn 3 (Red): Z1 E1 (0,4) -> A1 (0,0) */
	play_move(&red, &blue, ID_Z1, 0, 0);
	/* Turn 4 (Blue): Y1 D3 (2,3) -> B1 (0,1) */
	play_move(&blue, &red, ID_Y1, 0, 1);

	/* Turn 5 (Red): Z1 attempts A1 (0,0) -> A5 (4,0) -> triggers 3-way collision! */
	play_move(&red, &blue, ID_Z1, 4, 0);

	assert_boards_in_sync(&red, &blue);
	assert(red.current_turn == SIDE_BLUE);
	assert(red.phase == PHASE_PLAYING);

	/* Both Red and Blue agree: Z1 dead at B1 (0,1), Y1 dead at C2 (1,2), X1 alive at C2 (1,2) */
	assert(red.own[ID_Z1].alive == 0);
	assert(red.own[ID_Z1].x == 0 && red.own[ID_Z1].y == 1);
	assert(blue.enemy[ID_Z1].alive == 0);
	assert(blue.enemy[ID_Z1].known_x == 0 && blue.enemy[ID_Z1].known_y == 1);

	assert(blue.own[ID_Y1].alive == 0);
	assert(blue.own[ID_Y1].x == 1 && blue.own[ID_Y1].y == 2);
	assert(red.enemy[ID_Y1].alive == 0);
	assert(red.enemy[ID_Y1].known_x == 1 && red.enemy[ID_Y1].known_y == 2);

	assert(red.own[ID_X1].alive == 1);
	assert(red.own[ID_X1].x == 1 && red.own[ID_X1].y == 2);
	assert(blue.enemy[ID_X1].alive == 1);
	assert(blue.enemy[ID_X1].known_x == 1 && blue.enemy[ID_X1].known_y == 2);
	assert(red_log.count > 0 && blue_log.count > 0);
}

static void test_four_way_collision_chain(void)
{
	/*
	 * 4-way chain:
	 * Red W sitting at C4 (3, 2).
	 * Blue B moves from F7 (6, 5) to C4 (3, 2), passing through D5 (4, 3).
	 * Red Y1 moves from E4 (3, 4) to D5 (4, 3).
	 * Blue Z1 moves from F4 (3, 5) to D2 (1, 3), passing through E3 (2, 4)? No, let's make:
	 *   - Red Z1: (0,4) -> (0,0) (hits Blue Y1 at (0,1))
	 *   - Blue Y1: (2,3) -> (0,1) (hits Red X1 at (1,2))
	 *   - Red X1: (1,4) -> (1,2) (hits Blue B at (1,3))
	 *   - Blue B at (1,3) (kills X1)
	 * Chain: Z1 -> Y1 at (0,1) -> X1 at (1,2) -> B at (1,3)!
	 * Expected: Z1 dies at (0,1), Y1 dies at (1,2), X1 dies at (1,3), B survives & revealed at (1,3).
	 */
	struct player_board red, blue;
	int i;

	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);

	assert(dmost_place_piece(&red, ID_Z1, 0, 4));
	assert(dmost_place_piece(&red, ID_X1, 1, 4));
	assert(dmost_place_piece(&red, ID_W,  2, 0));
	assert(dmost_place_piece(&red, ID_B,  3, 0));
	assert(dmost_place_piece(&red, ID_X2, 4, 0));
	assert(dmost_place_piece(&red, ID_Y1, 5, 0));
	assert(dmost_place_piece(&red, ID_Y2, 6, 0));
	assert(dmost_place_piece(&red, ID_Z2, 7, 0));
	assert(dmost_place_piece(&red, ID_Z3, 8, 0));
	assert(dmost_place_piece(&red, ID_Z4, 9, 0));
	for (i = 0; i < 7; i++)
		assert(dmost_place_piece(&red, ID_A0 + i, 2 + i, 1));
	assert(dmost_place_piece(&red, ID_HP, 9, 3));

	assert(dmost_place_piece(&blue, ID_Y1, 4, 5));
	assert(dmost_place_piece(&blue, ID_B,  1, 5));
	assert(dmost_place_piece(&blue, ID_W,  6, 9));
	assert(dmost_place_piece(&blue, ID_X1, 2, 9));
	assert(dmost_place_piece(&blue, ID_X2, 3, 9));
	assert(dmost_place_piece(&blue, ID_Y2, 5, 9));
	assert(dmost_place_piece(&blue, ID_Z1, 6, 9));
	assert(dmost_place_piece(&blue, ID_Z2, 7, 9));
	assert(dmost_place_piece(&blue, ID_Z3, 8, 9));
	assert(dmost_place_piece(&blue, ID_Z4, 9, 9));
	for (i = 0; i < 7; i++)
		assert(dmost_place_piece(&blue, ID_A0 + i, 2 + i, 8));
	assert(dmost_place_piece(&blue, ID_HP, 0, 8));

	start_game_with_boards(&red, &blue);

	/* Setup positions in transit */
	play_move(&red, &blue, ID_X1, 1, 2); /* X1 (1,4)->(1,2) */
	play_move(&blue, &red, ID_Y1, 2, 3); /* Y1 (4,5)->(2,3) */
	play_move(&red, &blue, ID_Z1, 0, 0); /* Z1 (0,4)->(0,0) */
	play_move(&blue, &red, ID_Y1, 0, 1); /* Y1 (2,3)->(0,1) */
	play_move(&red, &blue, ID_Z4, 9, 2); /* dummy Red move (avoids y=1 asteroids) */
	play_move(&blue, &red, ID_B,  1, 3); /* Blue B (1,5)->(1,3) */

	/* Trigger chain by attempting Z1 (0,0)->(4,0) */
	play_move(&red, &blue, ID_Z1, 4, 0);

	assert_boards_in_sync(&red, &blue);
	assert(red.own[ID_Z1].alive == 0 && blue.enemy[ID_Z1].alive == 0);
	assert(blue.own[ID_Y1].alive == 0 && red.enemy[ID_Y1].alive == 0);
	assert(red.own[ID_X1].alive == 0 && blue.enemy[ID_X1].alive == 0);
	assert(red.own[ID_X1].x == 1 && red.own[ID_X1].y == 3);
	assert(blue.own[ID_B].alive == 1 && red.enemy[ID_B].alive == 1);
	assert(red.enemy[ID_B].known_x == 1 && red.enemy[ID_B].known_y == 3);
}

static void test_path_stopping_at_asteroid_vs_surviving(void)
{
	struct player_board red, blue;
	int i;

	/* Part A: Red B hits Blue Asteroid at (0,6) before Blue HP at (0,8) -> stops at (0,6), HP lives */
	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	setup_standard_board(&red);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	assert(dmost_place_piece(&blue, ID_A0, 0, 6));
	assert(dmost_place_piece(&blue, ID_HP, 0, 8));
	for (i = 1; i < 7; i++)
		assert(dmost_place_piece(&blue, ID_A0 + i, i, 6));
	assert(dmost_place_piece(&blue, ID_W,  6, 9));
	assert(dmost_place_piece(&blue, ID_B,  1, 9));
	assert(dmost_place_piece(&blue, ID_X1, 2, 9));
	assert(dmost_place_piece(&blue, ID_X2, 3, 9));
	assert(dmost_place_piece(&blue, ID_Y1, 4, 9));
	assert(dmost_place_piece(&blue, ID_Y2, 5, 9));
	assert(dmost_place_piece(&blue, ID_Z1, 6, 9));
	assert(dmost_place_piece(&blue, ID_Z2, 7, 9));
	assert(dmost_place_piece(&blue, ID_Z3, 8, 9));
	assert(dmost_place_piece(&blue, ID_Z4, 9, 9));

	start_game_with_boards(&red, &blue);

	play_move(&red, &blue, ID_B, 0, 4);  /* B (1,3)->(0,4) */
	play_move(&blue, &red, ID_Z4, 9, 7);
	play_move(&red, &blue, ID_B, 0, 9);  /* B (0,4)->(0,9) crosses (0,6) A0 and (0,8) HP */
	play_move(&blue, &red, ID_Z4, 9, 6);
	play_move(&red, &blue, ID_B, 5, 9);  /* Triggers check of B (0,4)->(0,9) */

	assert_boards_in_sync(&red, &blue);
	assert(red.phase == PHASE_PLAYING);
	assert(red.own[ID_B].alive == 0);
	assert(red.own[ID_B].x == 0 && red.own[ID_B].y == 6);
	assert(blue.own[ID_A0].alive == 0);
	assert(blue.own[ID_HP].alive == 1);
	assert(red.enemy[ID_HP].known_x == UNKNOWN_POS);

	/* Part B: Surviving B kills Z1 at (0,6) and continues to kill HP at (0,8) */
	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	setup_standard_board(&red);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	assert(dmost_place_piece(&blue, ID_Z1, 0, 6));
	assert(dmost_place_piece(&blue, ID_HP, 0, 8));
	for (i = 0; i < 7; i++)
		assert(dmost_place_piece(&blue, ID_A0 + i, 1 + i, 6));
	assert(dmost_place_piece(&blue, ID_W,  6, 9));
	assert(dmost_place_piece(&blue, ID_B,  1, 9));
	assert(dmost_place_piece(&blue, ID_X1, 2, 9));
	assert(dmost_place_piece(&blue, ID_X2, 3, 9));
	assert(dmost_place_piece(&blue, ID_Y1, 4, 9));
	assert(dmost_place_piece(&blue, ID_Y2, 5, 9));
	assert(dmost_place_piece(&blue, ID_Z2, 7, 9));
	assert(dmost_place_piece(&blue, ID_Z3, 8, 9));
	assert(dmost_place_piece(&blue, ID_Z4, 9, 9));

	start_game_with_boards(&red, &blue);

	play_move(&red, &blue, ID_B, 0, 4);
	play_move(&blue, &red, ID_Z4, 9, 7);
	play_move(&red, &blue, ID_B, 0, 9);
	play_move(&blue, &red, ID_Z4, 9, 6);
	play_move(&red, &blue, ID_B, 5, 9);

	assert_boards_in_sync(&red, &blue);
	assert(red.phase == PHASE_GAME_OVER);
	assert(red.result == RESULT_RED_WINS);
	assert(blue.own[ID_Z1].alive == 0);
	assert(blue.own[ID_HP].alive == 0);
	assert(red.own[ID_B].alive == 1);
}

static void test_w_fighter_intermediate_collision(void)
{
	struct player_board red, blue;
	int i;

	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);

	assert(dmost_place_piece(&red, ID_W, 4, 4));
	assert(dmost_place_piece(&red, ID_B, 1, 3));
	assert(dmost_place_piece(&red, ID_X1, 2, 3));
	assert(dmost_place_piece(&red, ID_X2, 3, 3));
	assert(dmost_place_piece(&red, ID_Y1, 4, 3));
	assert(dmost_place_piece(&red, ID_Y2, 5, 3));
	assert(dmost_place_piece(&red, ID_Z1, 6, 3));
	assert(dmost_place_piece(&red, ID_Z2, 7, 3));
	assert(dmost_place_piece(&red, ID_Z3, 8, 3));
	assert(dmost_place_piece(&red, ID_Z4, 9, 3));
	for (i = 0; i < 7; i++)
		assert(dmost_place_piece(&red, ID_A0 + i, i, 1));
	assert(dmost_place_piece(&red, ID_HP, 9, 0));

	/* Blue Asteroid at (4, 6) — intermediate square of W (4,4)->(5,6) */
	assert(dmost_place_piece(&blue, ID_A0, 4, 6));
	for (i = 1; i < 7; i++)
		assert(dmost_place_piece(&blue, ID_A0 + i, i, 8));
	assert(dmost_place_piece(&blue, ID_HP, 5, 6));
	assert(dmost_place_piece(&blue, ID_W,  6, 9));
	assert(dmost_place_piece(&blue, ID_B,  1, 9));
	assert(dmost_place_piece(&blue, ID_X1, 2, 9));
	assert(dmost_place_piece(&blue, ID_X2, 3, 9));
	assert(dmost_place_piece(&blue, ID_Y1, 4, 9));
	assert(dmost_place_piece(&blue, ID_Y2, 5, 9));
	assert(dmost_place_piece(&blue, ID_Z1, 6, 9));
	assert(dmost_place_piece(&blue, ID_Z2, 7, 9));
	assert(dmost_place_piece(&blue, ID_Z3, 8, 9));
	assert(dmost_place_piece(&blue, ID_Z4, 9, 9));

	start_game_with_boards(&red, &blue);

	play_move(&red, &blue, ID_W, 5, 6); /* (4,4)->(4,5)->(4,6)->(5,6) */
	play_move(&blue, &red, ID_Z4, 9, 7);
	play_move(&red, &blue, ID_W, 6, 8); /* triggers check at (4,6) */

	assert_boards_in_sync(&red, &blue);
	assert(red.phase == PHASE_PLAYING);
	assert(red.own[ID_W].alive == 0);
	assert(red.own[ID_W].x == 4 && red.own[ID_W].y == 6);
	assert(blue.own[ID_A0].alive == 0);
	assert(blue.own[ID_HP].alive == 1);
}

static void test_first_move_departure_collision(void)
{
	struct player_board red, blue;

	/* Direction 1: unmoved Red W departs (0,4) where Blue Z1 is parked -> Z1 dies, W completes move */
	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	setup_standard_board(&red);
	setup_standard_board(&blue);
	assert(dmost_place_piece(&red, ID_W, 0, 4));
	assert(dmost_place_piece(&blue, ID_Z1, 0, 5));
	start_game_with_boards(&red, &blue);

	play_move(&red, &blue, ID_Z1, 6, 4); /* Red dummy */
	play_move(&blue, &red, ID_Z1, 0, 4); /* Blue parks Z1 at (0,4) */
	play_move(&red, &blue, ID_W,  1, 2); /* Unmoved W departs (0,4) -> hits Blue Z1 at (0,4) */

	assert_boards_in_sync(&red, &blue);
	assert(blue.own[ID_Z1].alive == 0);
	assert(red.enemy[ID_Z1].alive == 0 && red.enemy[ID_Z1].known_x == 0 && red.enemy[ID_Z1].known_y == 4);
	assert(red.own[ID_W].alive == 1);
	assert(red.own[ID_W].x == 1 && red.own[ID_W].y == 2);
	assert(blue.enemy[ID_W].alive == 1 && blue.enemy[ID_W].known_x == 0 && blue.enemy[ID_W].known_y == 4);

	/* Direction 2: unmoved Red Z1 departs (6,4) where Blue W is parked -> Z1 dies, move cancelled */
	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	setup_standard_board(&red);
	setup_standard_board(&blue);
	assert(dmost_place_piece(&red, ID_Z1, 6, 4));
	/* Put Blue W at (7,6) -> moves (7,6)->(7,5)->(7,4)->(6,4), avoiding (0..6,6) asteroids */
	assert(dmost_place_piece(&blue, ID_W, 7, 6));
	start_game_with_boards(&red, &blue);

	play_move(&red, &blue, ID_Z2, 7, 4); /* Red dummy */
	play_move(&blue, &red, ID_W,  6, 4); /* Blue W (7,6)->(7,5)->(7,4)->(6,4) */
	play_move(&red, &blue, ID_Z1, 6, 8); /* Unmoved Z1 departs (6,4) -> dies to W at (6,4) */

	assert_boards_in_sync(&red, &blue);
	assert(red.own[ID_Z1].alive == 0);
	assert(red.own[ID_Z1].x == 6 && red.own[ID_Z1].y == 4);
	assert(blue.own[ID_W].alive == 1);
	assert(red.enemy[ID_W].alive == 1 && red.enemy[ID_W].known_x == 6 && red.enemy[ID_W].known_y == 4);
}

static void test_stacked_friendly_pieces(void)
{
	struct player_board red, blue;

	/* Blue stacks X1 and Z1 at (4,6). Red Y1 passes through (4,6).
	 * Move Blue's A4 off (4,6) to (7,6) so (4,6) is free for fighters.
	 * Because X1 > Y1 > Z1, X1 intercepts Y1 first: Y1 dies at (4,6),
	 * X1 survives and is revealed, and Z1 stays alive and unrevealed.
	 */
	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	setup_standard_board(&red);
	setup_standard_board(&blue);
	assert(dmost_place_piece(&red, ID_Y1, 2, 4));
	assert(dmost_place_piece(&blue, ID_A4, 7, 6));
	assert(dmost_place_piece(&blue, ID_X1, 4, 6));
	assert(dmost_place_piece(&blue, ID_Z1, 4, 6));
	start_game_with_boards(&red, &blue);

	play_move(&red, &blue, ID_Y1, 6, 8);
	play_move(&blue, &red, ID_Z4, 9, 7);
	play_move(&red, &blue, ID_Y1, 7, 7);

	assert_boards_in_sync(&red, &blue);
	assert(red.own[ID_Y1].alive == 0);
	assert(red.own[ID_Y1].x == 4 && red.own[ID_Y1].y == 6);
	assert(blue.own[ID_X1].alive == 1);
	assert(red.enemy[ID_X1].alive == 1 && red.enemy[ID_X1].known_x == 4 && red.enemy[ID_X1].known_y == 6);
	assert(blue.own[ID_Z1].alive == 1);
	assert(red.enemy[ID_Z1].known_x == UNKNOWN_POS);

	/* Now Red W (stronger than both X1 and Z1) hits the stack at (4,6): kills BOTH X1 and Z1 */
	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	setup_standard_board(&red);
	setup_standard_board(&blue);
	assert(dmost_place_piece(&red, ID_W, 3, 4));
	assert(dmost_place_piece(&blue, ID_A3, 8, 6));
	assert(dmost_place_piece(&blue, ID_A4, 7, 6));
	assert(dmost_place_piece(&blue, ID_X1, 4, 6));
	assert(dmost_place_piece(&blue, ID_Z1, 4, 6));
	start_game_with_boards(&red, &blue);

	play_move(&red, &blue, ID_W, 4, 6); /* (3,4)->(3,5)->(3,6)->(4,6) */
	play_move(&blue, &red, ID_Z4, 9, 7);
	play_move(&red, &blue, ID_W, 5, 4);

	assert_boards_in_sync(&red, &blue);
	assert(red.own[ID_W].alive == 1);
	assert(blue.own[ID_X1].alive == 0 && red.enemy[ID_X1].alive == 0);
	assert(blue.own[ID_Z1].alive == 0 && red.enemy[ID_Z1].alive == 0);
}

static void kill_all_non_y_mobile(struct player_board *red,
				    struct player_board *blue)
{
	int id;

	for (id = 0; id < NUM_PIECES; id++) {
		enum piece_type t = dmost_piece_info[id].type;
		if (dmost_piece_is_mobile(t) && t != PIECE_YFIGHTER) {
			red->own[id].alive = 0;
			red->enemy[id].alive = 0;
			blue->own[id].alive = 0;
			blue->enemy[id].alive = 0;
		}
	}
}

static void test_win_and_draws(void)
{
	struct player_board red, blue;
	struct dmost_msg msg;
	int i;

	/* 1. Blue wins when Red HP destroyed */
	dmost_board_init(&red, SIDE_RED, SIDE_BLUE);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_BLUE);
	setup_standard_board(&red);
	setup_standard_board(&blue);
	assert(dmost_place_piece(&red, ID_HP, 9, 4));
	assert(dmost_place_piece(&blue, ID_HP, 0, 5));
	start_game_with_boards(&red, &blue);

	play_move(&blue, &red, ID_Z4, 9, 4);
	play_move(&red, &blue, ID_Z1, 6, 4);
	play_move(&blue, &red, ID_Z4, 8, 4);

	assert_boards_in_sync(&red, &blue);
	assert(red.phase == PHASE_GAME_OVER && red.result == RESULT_BLUE_WINS);

	/* 2. Pass when no legal moves -> consecutive pass draw */
	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	setup_standard_board(&red);
	setup_standard_board(&blue);
	/* Keep only Z1 alive on Red, surrounded in corner (0,0) by own A0, A1, A2 */
	assert(dmost_place_piece(&red, ID_Z1, 0, 0));
	assert(dmost_place_piece(&red, ID_A0, 1, 0));
	assert(dmost_place_piece(&red, ID_A1, 0, 1));
	assert(dmost_place_piece(&red, ID_A2, 1, 1));
	assert(dmost_place_piece(&red, ID_HP, 9, 0));
	/* Also trap Blue Z1 at (9,9) (move Blue Z3, Z4 off (8,8), (9,8) first) */
	assert(dmost_place_piece(&blue, ID_Z3, 6, 7));
	assert(dmost_place_piece(&blue, ID_Z4, 7, 7));
	assert(dmost_place_piece(&blue, ID_Z1, 9, 9));
	assert(dmost_place_piece(&blue, ID_A0, 8, 9));
	assert(dmost_place_piece(&blue, ID_A1, 9, 8));
	assert(dmost_place_piece(&blue, ID_A2, 8, 8));
	assert(dmost_place_piece(&blue, ID_HP, 0, 9));
	start_game_with_boards(&red, &blue);
	for (i = 0; i < NUM_PIECES; i++) {
		if (dmost_piece_is_mobile(dmost_piece_info[i].type) && i != ID_Z1) {
			red.own[i].alive = 0;
			red.enemy[i].alive = 0;
			blue.own[i].alive = 0;
			blue.enemy[i].alive = 0;
		}
	}

	assert(dmost_has_any_legal_move(&red) == 0);
	assert(dmost_initiate_pass(&red, &msg) == 1);
	deliver_msg(&red, &blue, &msg);
	assert_boards_in_sync(&red, &blue);
	assert(red.phase == PHASE_PLAYING && red.current_turn == SIDE_BLUE);

	assert(dmost_has_any_legal_move(&blue) == 0);
	assert(dmost_initiate_pass(&blue, &msg) == 1);
	deliver_msg(&blue, &red, &msg);
	assert_boards_in_sync(&red, &blue);
	assert(red.phase == PHASE_GAME_OVER && red.result == RESULT_DRAW);

	/* 3. Y-fighter opposite-parity draw (non-cheating) */
	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	setup_standard_board(&red);
	setup_standard_board(&blue);
	/* Red Y1,Y2 on even squares (0+2=2, 2+2=4); Red HP on even square (0+0=0) */
	assert(dmost_place_piece(&red, ID_Y1, 0, 2));
	assert(dmost_place_piece(&red, ID_Y2, 2, 2));
	assert(dmost_place_piece(&red, ID_HP, 0, 0));
	/* Blue Y1,Y2 on odd squares (0+9=9, 2+9=11); Blue HP on odd square (0+7=7) */
	assert(dmost_place_piece(&blue, ID_Y1, 0, 9));
	assert(dmost_place_piece(&blue, ID_Y2, 2, 9));
	assert(dmost_place_piece(&blue, ID_HP, 0, 7));
	kill_all_non_y_mobile(&red, &blue);
	start_game_with_boards(&red, &blue);

	/* Move Red Y1 and Blue Y1 -> parity masks exchanged -> automatic draw! */
	play_move(&red, &blue, ID_Y1, 1, 3);
	if (red.phase == PHASE_PLAYING)
		play_move(&blue, &red, ID_Y1, 1, 8);
	if (red.phase == PHASE_PLAYING)
		play_move(&red, &blue, ID_Y1, 2, 4);

	assert_boards_in_sync(&red, &blue);
	assert(red.phase == PHASE_GAME_OVER && red.result == RESULT_DRAW);

	/* 4. Y-fighter matching-parity does NOT falsely draw */
	dmost_board_init(&red, SIDE_RED, SIDE_RED);
	dmost_board_init(&blue, SIDE_BLUE, SIDE_RED);
	setup_standard_board(&red);
	setup_standard_board(&blue);
	assert(dmost_place_piece(&red, ID_Y1, 0, 2)); /* even */
	assert(dmost_place_piece(&red, ID_Y2, 2, 2)); /* even */
	assert(dmost_place_piece(&red, ID_HP, 1, 0)); /* odd */
	assert(dmost_place_piece(&blue, ID_Y1, 0, 9)); /* odd */
	assert(dmost_place_piece(&blue, ID_Y2, 2, 9)); /* odd */
	assert(dmost_place_piece(&blue, ID_HP, 0, 7)); /* odd -> Blue Y (odd) CAN hit Red HP (odd) */
	kill_all_non_y_mobile(&red, &blue);
	start_game_with_boards(&red, &blue);

	play_move(&red, &blue, ID_Y1, 1, 3);
	play_move(&blue, &red, ID_Y1, 1, 8);
	play_move(&red, &blue, ID_Y1, 2, 4);

	assert_boards_in_sync(&red, &blue);
	assert(red.phase == PHASE_PLAYING && red.result == RESULT_ONGOING);
}

int main(void)
{
	test_path_geometry();
	test_combat_matrix();
	test_setup_validation();
	test_own_obstacle_blocking();
	test_basic_turn_and_delayed_transit();
	test_html_three_way_collision();
	test_four_way_collision_chain();
	test_path_stopping_at_asteroid_vs_surviving();
	test_w_fighter_intermediate_collision();
	test_first_move_departure_collision();
	test_stacked_friendly_pieces();
	test_win_and_draws();
	printf("All core game engine tests passed!\n");
	return 0;
}
