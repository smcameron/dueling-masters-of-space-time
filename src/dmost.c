/*
    (C) Copyright 2012, 2026 Stephen M. Cameron.

    This file is part of Dueling Masters of Space Time.

    Dueling Masters of Space Time is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Dueling Masters of Space Time is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Dueling Masters of Space Time; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL.h>

#include "game.h"
#include "opponent.h"
#include "ai.h"
#include "sdl_font.h"
#include "sdl_gfx.h"
#include "sdl_ui.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define ANIM_QUEUE_CAP 32
#define TOOLBAR_BTN_H 28.0f
#define TOOLBAR_PAD 6.0f
#define STATUS_BAR_H 34.0f
#define LOG_BOX_H 92.0f

enum anim_kind {
	ANIM_SLIDE = 1,
	ANIM_APPEAR = 2,
	ANIM_IMPACT = 3,
};

struct anim_step {
	enum anim_kind kind;
	struct piece *piece;
	struct square path[MAX_PATH_LEN];
	int path_len;
	int x, y;
	double r, g, b;
	int duration_ms;
	int elapsed_ms;
};

struct piece {
	char *name;
	char *description;
	int x, y, prevx, prevy;
#define UNKNOWN (-1)
	int strength;

#define WFIGHTER PIECE_WFIGHTER
#define BFIGHTER PIECE_BFIGHTER
#define XFIGHTER PIECE_XFIGHTER
#define YFIGHTER PIECE_YFIGHTER
#define ZFIGHTER PIECE_ZFIGHTER
#define ASTEROID PIECE_ASTEROID
#define HOMEPLANET PIECE_HOMEPLANET

	double sx, sy;
	double r, g, b;
	int pickedx, pickedy; /* place piece was picked from before move is final */
	int id;
	int alive;
	int in_transit;
} full_set[] = {
	{ "W", "W fighter.  Moves 2 then 1 (L-path).  Strongest piece.",
			-1, -1, -1, -1, WFIGHTER, -1, -1, 1.0, 0.1, 0.1, -1, -1, 0, 1, 0 },
	{ "B", "B fighter.  Moves like chess queen.  Only W is stronger.",
			-1, -1, -1, -1, BFIGHTER, -1, -1, 1.0, 0.1, 0.1, -1, -1, 1, 1, 0 },
	{ "X1", "X1 fighter.  Moves like chess rook.  W and B are stronger",
			-1, -1, -1, -1, XFIGHTER, -1, -1, 1.0, 0.1, 0.1, -1, -1, 2, 1, 0 },
	{ "X2", "X2 fighter.  Moves like chess rook.  W and B are stronger",
			-1, -1, -1, -1, XFIGHTER, -1, -1, 1.0, 0.1, 0.1, -1, -1, 3, 1, 0 },
	{ "Y1", "Y1 fighter.  Moves like chess bishop.  W, X, and B are stronger",
			-1, -1, -1, -1, YFIGHTER, -1, -1, 1.0, 0.1, 0.1, -1, -1, 4, 1, 0 },
	{ "Y2", "Y2 fighter.  Moves like chess bishop.  W, X, and B are stronger",
			-1, -1, -1, -1, YFIGHTER, -1, -1, 1.0, 0.1, 0.1, -1, -1, 5, 1, 0 },
	{ "Z1", "Z1 fighter.  Moves like chess queen.  Weakest mobile piece",
			-1, -1, -1, -1, ZFIGHTER, -1, -1, 1.0, 0.1, 0.1, -1, -1, 6, 1, 0 },
	{ "Z2", "Z2 fighter.  Moves like chess queen.  Weakest mobile piece",
			-1, -1, -1, -1, ZFIGHTER, -1, -1, 1.0, 0.1, 0.1, -1, -1, 7, 1, 0 },
	{ "Z3", "Z3 fighter.  Moves like chess queen.  Weakest mobile piece",
			-1, -1, -1, -1, ZFIGHTER, -1, -1, 1.0, 0.1, 0.1, -1, -1, 8, 1, 0 },
	{ "Z4", "Z4 fighter.  Moves like chess queen.  Weakest mobile piece",
			-1, -1, -1, -1, ZFIGHTER, -1, -1, 1.0, 0.1, 0.1, -1, -1, 9, 1, 0 },
	{ "A", "Asteroid", -1, -1, -1, -1, ASTEROID, -1, -1, 1.0, 0.1, 0.1, -1, -1, 10, 1, 0 },
	{ "A", "Asteroid", -1, -1, -1, -1, ASTEROID, -1, -1, 1.0, 0.1, 0.1, -1, -1, 11, 1, 0 },
	{ "A", "Asteroid", -1, -1, -1, -1, ASTEROID, -1, -1, 1.0, 0.1, 0.1, -1, -1, 12, 1, 0 },
	{ "A", "Asteroid", -1, -1, -1, -1, ASTEROID, -1, -1, 1.0, 0.1, 0.1, -1, -1, 13, 1, 0 },
	{ "A", "Asteroid", -1, -1, -1, -1, ASTEROID, -1, -1, 1.0, 0.1, 0.1, -1, -1, 14, 1, 0 },
	{ "A", "Asteroid", -1, -1, -1, -1, ASTEROID, -1, -1, 1.0, 0.1, 0.1, -1, -1, 15, 1, 0 },
	{ "A", "Asteroid", -1, -1, -1, -1, ASTEROID, -1, -1, 1.0, 0.1, 0.1, -1, -1, 16, 1, 0 },
	{ "HP", "Home Planet", -1, -1, -1, -1, HOMEPLANET, -1, -1, 1.0, 0.1, 0.1, -1, -1, 17, 1, 0 },
	{ NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1.0, 0.3, 0.3, 0, 0, -1, 0, 0 },
};

struct piece *p1, *p2;

struct gui {
	SDL_Window *window;
	SDL_Renderer *renderer;
	struct sdl_font font;
	SDL_Texture *galaxy_tex;
	int galaxy_w, galaxy_h;
	int win_w, win_h;

	SDL_FRect toolbar_rect;
	SDL_FRect board_rect;
	SDL_FRect status_rect;
	SDL_FRect collision_rect;

	struct sdl_dropdown mode_dd;
	struct sdl_dropdown opponent_dd;
	struct sdl_dropdown redblue_dd;
	struct sdl_button auto_setup_btn;
	struct sdl_button new_game_btn;
	struct sdl_button invert_btn;
	struct sdl_button help_btn;
	struct sdl_button quit_btn;
	struct sdl_button pass_btn;
	struct sdl_button play_again_btn;

	char status_text[256];
	struct sdl_log_view log_view;
	struct sdl_modal help_modal;
	struct sdl_modal game_over_modal;
	const char *hover_tooltip;

	int xdim, ydim;
	double piece_box_open;
	int inverted;
	int win_mousex, win_mousey;
	int mousex, mousey;
	struct piece *holding;
	int playing_as;
	int fullscreen;
#define PLAYING_AS_RED SIDE_RED
#define PLAYING_AS_BLUE SIDE_BLUE
	int mode;
#define MODE_FREE 0
#define MODE_BOARD_SETUP 1
#define MODE_PLAYING 2
	uint8_t visited[BOARD_SIZE][BOARD_SIZE];

	int use_opponent;
	struct player_board board;
	struct dmost_opponent *opponent;
	int poll_scheduled;
	Uint32 poll_due_ms;
	uint32_t setup_rng;
	uint32_t game_counter;
	struct square recent_collisions[32];
	int recent_collision_count;
	char last_collision_text[320];

	struct anim_step anim_queue[ANIM_QUEUE_CAP];
	int anim_head;
	int anim_count;
	Uint32 anim_last_tick_ms;

	int snap_own_ann_x[NUM_PIECES];
	int snap_own_ann_y[NUM_PIECES];
	int snap_own_x[NUM_PIECES];
	int snap_own_y[NUM_PIECES];
	int snap_own_transit[NUM_PIECES];
	int snap_enemy_x[NUM_PIECES];
	int snap_enemy_y[NUM_PIECES];
	int active_enemy_target_x[NUM_PIECES];
	int active_enemy_target_y[NUM_PIECES];
	uint8_t enemy_animated_this_step[NUM_PIECES];

	int game_over_scheduled;
	Uint32 game_over_due_ms;
	int game_over_pending;
	int game_over_shown;

	int dirty;
	int quit;
};

static struct gui ui;

static const char *const mode_items[] = {
	"Board Setup",
	"Play Game",
	"Free form",
	"Begin New Game",
};

static const char *const opponent_items[] = {
	"vs Computer",
	"Manual (Offline)",
};

static const char *const redblue_items[] = {
	"Play as Red",
	"Play as Blue",
};

static const char *const help_instructions =
"   Instructions \xE2\x80\x94 Dueling Masters of Space Time\n"
"   \n"
"   MODE 1: vs Computer (Default)\n"
"   1. Choose 'Play as Red' (moves first) or 'Play as Blue' (Computer moves first).\n"
"   2. Arrange all 18 of your pieces on your half of the board, or click\n"
"      'Auto Setup' for a randomized defensive/scouting formation.\n"
"      (Asteroids and your Home Planet must occupy squares by themselves;\n"
"      fighters may stack with other friendly fighters.)\n"
"   3. Select 'Play Game' from the top-left menu to begin.\n"
"   4. On your turn, left-click a piece and click a highlighted destination\n"
"      square. Pieces enter 'in transit' until moved again; collisions are\n"
"      resolved recursively and logged at the bottom of the window.\n"
"      Right-click any square to toggle a personal marker.\n"
"   5. Destroy the opponent's Home Planet to win!\n"
"   \n"
"   MODE 2: Manual (Offline)\n"
"   1. Select 'Manual (Offline)' from the opponent menu to play with a human\n"
"      opponent over voice/chat/across a screen using your own boards.\n"
"   2. Announce which piece you are moving and where it is moving FROM\n"
"      (never reveal where it is moving TO).\n"
"   3. Piece strengths (strongest to weakest): A, W, B, X, Y, Z, HP.\n"
"      Asteroids destroy anything touching them and are destroyed too.\n"
"   \n";

static void generic_draw_piece_on_board(struct piece *p, struct gui *ui,
					double radius_scale);
static void reset_managed_game(struct gui *ui);
static void layout_ui(struct gui *ui);

static struct piece *own_pieces(const struct gui *ui)
{
	return (ui->playing_as == PLAYING_AS_RED) ? p1 : p2;
}

static struct piece *enemy_pieces(const struct gui *ui)
{
	return (ui->playing_as == PLAYING_AS_RED) ? p2 : p1;
}

static int is_managed_mode(const struct gui *ui)
{
	return ui->use_opponent && ui->mode != MODE_FREE;
}

static void append_log(struct gui *ui, const char *text, int is_collision)
{
	sdl_log_append(&ui->log_view, text, is_collision);
	ui->dirty = 1;
}

static void clear_log(struct gui *ui)
{
	sdl_log_clear(&ui->log_view);
	ui->dirty = 1;
}

static void clear_recent_collisions(struct gui *ui)
{
	ui->recent_collision_count = 0;
	ui->last_collision_text[0] = '\0';
	layout_ui(ui);
	ui->dirty = 1;
}

static void record_recent_collision(struct gui *ui, int x, int y,
				    const char *text)
{
	int i, dup = 0;

	for (i = 0; i < ui->recent_collision_count; i++) {
		if (ui->recent_collisions[i].x == x &&
		    ui->recent_collisions[i].y == y) {
			dup = 1;
			break;
		}
	}
	if (!dup && ui->recent_collision_count <
	    (int)ARRAYSIZE(ui->recent_collisions)) {
		ui->recent_collisions[ui->recent_collision_count].x = x;
		ui->recent_collisions[ui->recent_collision_count].y = y;
		ui->recent_collision_count++;
	}

	if (ui->last_collision_text[0] == '\0') {
		snprintf(ui->last_collision_text,
			 sizeof(ui->last_collision_text), "%s", text);
	} else {
		size_t cur = strlen(ui->last_collision_text);

		if (cur + 6 < sizeof(ui->last_collision_text))
			snprintf(ui->last_collision_text + cur,
				 sizeof(ui->last_collision_text) - cur,
				 "  |  %s", text);
	}
	layout_ui(ui);
	ui->dirty = 1;
}

static void dismiss_game_over_dialog(struct gui *ui)
{
	ui->game_over_scheduled = 0;
	sdl_modal_close(&ui->game_over_modal);
	ui->game_over_pending = 0;
	ui->game_over_shown = 0;
}

static void open_game_over_modal(struct gui *ui)
{
	const char *title;
	const char *primary;
	char secondary[160];
	int human_won;

	ui->game_over_scheduled = 0;
	ui->game_over_pending = 0;
	ui->game_over_shown = 1;

	human_won = ((ui->board.side == SIDE_RED &&
		      ui->board.result == RESULT_RED_WINS) ||
		     (ui->board.side == SIDE_BLUE &&
		      ui->board.result == RESULT_BLUE_WINS));

	if (ui->board.result == RESULT_DRAW) {
		title = "Game Over \xE2\x80\x94 Draw";
		primary = "Game Over: Draw!";
		snprintf(secondary, sizeof(secondary),
			 "Neither player can destroy the other's Home Planet (turn %d).",
			 ui->board.turn_number);
	} else if (human_won) {
		title = "Game Over \xE2\x80\x94 Victory!";
		primary = (ui->board.result == RESULT_RED_WINS) ?
			  "You Win! (Red Wins)" : "You Win! (Blue Wins)";
		snprintf(secondary, sizeof(secondary),
			 "You destroyed the opponent's Home Planet on turn %d!",
			 ui->board.turn_number);
	} else {
		title = "Game Over \xE2\x80\x94 Defeat";
		primary = (ui->board.result == RESULT_RED_WINS) ?
			  "You Lose! (Red Wins)" : "You Lose! (Blue Wins)";
		snprintf(secondary, sizeof(secondary),
			 "Your Home Planet was destroyed on turn %d.",
			 ui->board.turn_number);
	}

	sdl_modal_open_message(&ui->game_over_modal, title, primary, secondary,
			       "Play Again", "View Board");
	sdl_modal_layout(&ui->game_over_modal, ui->win_w, ui->win_h);
	ui->dirty = 1;
}

static void schedule_game_over_dialog(struct gui *ui)
{
	if (ui->game_over_shown || ui->game_over_scheduled)
		return;
	ui->game_over_pending = 1;
	ui->game_over_scheduled = 1;
	ui->game_over_due_ms = SDL_GetTicks() + 300;
}

static void clear_animations(struct gui *ui)
{
	ui->anim_head = 0;
	ui->anim_count = 0;
}

static void enqueue_anim(struct gui *ui, const struct anim_step *step)
{
	int tail;

	if (ui->anim_count >= ANIM_QUEUE_CAP)
		return;
	if (ui->anim_count == 0)
		ui->anim_last_tick_ms = SDL_GetTicks();
	tail = (ui->anim_head + ui->anim_count) % ANIM_QUEUE_CAP;
	ui->anim_queue[tail] = *step;
	ui->anim_count++;
	ui->dirty = 1;
}

static int build_partial_path(enum piece_type type, int sx, int sy,
			      int full_ex, int full_ey,
			      int stop_x, int stop_y,
			      struct square *out_path)
{
	int len, i;

	if (full_ex != UNKNOWN_POS && full_ey != UNKNOWN_POS &&
	    (sx != full_ex || sy != full_ey)) {
		len = dmost_compute_path(type, sx, sy, full_ex, full_ey,
					 out_path, MAX_PATH_LEN);
		if (len >= 2) {
			for (i = 0; i < len; i++) {
				if (out_path[i].x == stop_x &&
				    out_path[i].y == stop_y)
					return (i >= 1) ? (i + 1) : 2;
			}
		}
	}

	if (sx != stop_x || sy != stop_y) {
		len = dmost_compute_path(type, sx, sy, stop_x, stop_y,
					 out_path, MAX_PATH_LEN);
		if (len >= 2)
			return len;
		out_path[0].x = sx;
		out_path[0].y = sy;
		out_path[1].x = stop_x;
		out_path[1].y = stop_y;
		return 2;
	}
	return 0;
}

static void enqueue_slide_anim(struct gui *ui, struct piece *piece,
			       enum piece_type type, int sx, int sy,
			       int full_ex, int full_ey,
			       int stop_x, int stop_y)
{
	struct anim_step step;
	int dur;

	memset(&step, 0, sizeof(step));
	step.path_len = build_partial_path(type, sx, sy, full_ex, full_ey,
					   stop_x, stop_y, step.path);
	if (step.path_len < 2)
		return;

	dur = 450 + 50 * (step.path_len - 1);
	if (dur > 750)
		dur = 750;

	step.kind = ANIM_SLIDE;
	step.piece = piece;
	step.x = stop_x;
	step.y = stop_y;
	step.r = piece->r;
	step.g = piece->g;
	step.b = piece->b;
	step.duration_ms = dur;
	enqueue_anim(ui, &step);
}

static void enqueue_appear_anim(struct gui *ui, struct piece *piece,
				int x, int y)
{
	struct anim_step step;

	memset(&step, 0, sizeof(step));
	step.kind = ANIM_APPEAR;
	step.piece = piece;
	step.x = x;
	step.y = y;
	step.r = piece->r;
	step.g = piece->g;
	step.b = piece->b;
	step.duration_ms = 500;
	enqueue_anim(ui, &step);
}

static void enqueue_impact_anim(struct gui *ui, int x, int y)
{
	struct anim_step step;

	memset(&step, 0, sizeof(step));
	step.kind = ANIM_IMPACT;
	step.x = x;
	step.y = y;
	step.r = 1.0;
	step.g = 0.65;
	step.b = 0.15;
	step.duration_ms = 450;
	enqueue_anim(ui, &step);
}

static void snapshot_board_for_anim(struct gui *ui)
{
	int i;

	for (i = 0; i < NUM_PIECES; i++) {
		ui->snap_own_ann_x[i] = ui->board.own[i].announced_x;
		ui->snap_own_ann_y[i] = ui->board.own[i].announced_y;
		ui->snap_own_x[i] = ui->board.own[i].x;
		ui->snap_own_y[i] = ui->board.own[i].y;
		ui->snap_own_transit[i] = ui->board.own[i].in_transit;
		ui->snap_enemy_x[i] = ui->board.enemy[i].known_x;
		ui->snap_enemy_y[i] = ui->board.enemy[i].known_y;
		ui->active_enemy_target_x[i] = UNKNOWN_POS;
		ui->active_enemy_target_y[i] = UNKNOWN_POS;
		ui->enemy_animated_this_step[i] = 0;
	}
}

static void finalize_enemy_move_anims(struct gui *ui)
{
	struct piece *enemy = enemy_pieces(ui);
	int i;

	for (i = 0; i < NUM_PIECES; i++) {
		int kx = ui->board.enemy[i].known_x;
		int ky = ui->board.enemy[i].known_y;
		int sx = ui->snap_enemy_x[i];
		int sy = ui->snap_enemy_y[i];

		if (ui->enemy_animated_this_step[i] || kx == UNKNOWN_POS)
			continue;
		if (kx == sx && ky == sy)
			continue;

		if (sx == UNKNOWN_POS)
			enqueue_appear_anim(ui, &enemy[i], kx, ky);
		else
			enqueue_slide_anim(ui, &enemy[i],
					   ui->board.enemy[i].type,
					   sx, sy, kx, ky, kx, ky);
	}
}

static void on_board_event(void *ctx, const struct dmost_event *ev)
{
	struct gui *ui = ctx;
	char buf[160];

	dmost_format_event(ev, ui->board.side, buf, sizeof(buf));
	if (ev->type == EVENT_DEPART && ev->actor != ui->board.side &&
	    ev->piece_id >= 0 && ev->piece_id < NUM_PIECES) {
		ui->active_enemy_target_x[ev->piece_id] = ev->x;
		ui->active_enemy_target_y[ev->piece_id] = ev->y;
	}
	if (ev->type == EVENT_COLLISION) {
		int id = ev->piece_id;

		if (ev->actor == ui->board.side &&
		    id >= 0 && id < NUM_PIECES) {
			if (ui->snap_own_transit[id] &&
			    ui->snap_own_ann_x[id] != UNKNOWN_POS &&
			    (ui->snap_own_ann_x[id] != ev->x ||
			     ui->snap_own_ann_y[id] != ev->y)) {
				enqueue_slide_anim(ui, &own_pieces(ui)[id],
						   ui->board.own[id].type,
						   ui->snap_own_ann_x[id],
						   ui->snap_own_ann_y[id],
						   ui->snap_own_x[id],
						   ui->snap_own_y[id],
						   ev->x, ev->y);
			}
			enqueue_impact_anim(ui, ev->x, ev->y);
		} else if (ev->actor != ui->board.side &&
			   id >= 0 && id < NUM_PIECES) {
			if (!ui->enemy_animated_this_step[id]) {
				int sx = ui->snap_enemy_x[id];
				int sy = ui->snap_enemy_y[id];
				int tx = ui->active_enemy_target_x[id];
				int ty = ui->active_enemy_target_y[id];

				ui->enemy_animated_this_step[id] = 1;
				if (sx != UNKNOWN_POS &&
				    (sx != ev->x || sy != ev->y)) {
					if (tx == UNKNOWN_POS) {
						tx = ev->x;
						ty = ev->y;
					}
					enqueue_slide_anim(ui,
							   &enemy_pieces(ui)[id],
							   ui->board.enemy[id].type,
							   sx, sy, tx, ty,
							   ev->x, ev->y);
				} else if (sx == UNKNOWN_POS) {
					enqueue_appear_anim(ui,
							    &enemy_pieces(ui)[id],
							    ev->x, ev->y);
				}
			}
			enqueue_impact_anim(ui, ev->x, ev->y);
		}

		record_recent_collision(ui, ev->x, ev->y, buf);
		append_log(ui, buf, 1);
	} else if (ev->type == EVENT_GAME_OVER) {
		append_log(ui, buf, 1);
		schedule_game_over_dialog(ui);
	} else {
		append_log(ui, buf, 0);
	}
}

static void sync_ui_from_board(struct gui *ui)
{
	struct piece *own = own_pieces(ui);
	struct piece *enemy = enemy_pieces(ui);
	int i;

	for (i = 0; i < NUM_PIECES; i++) {
		const struct own_piece *op = &ui->board.own[i];
		const struct known_enemy_piece *ep = &ui->board.enemy[i];

		own[i].id = i;
		own[i].x = op->x;
		own[i].y = op->y;
		own[i].prevx = op->prev_x;
		own[i].prevy = op->prev_y;
		own[i].pickedx = op->x;
		own[i].pickedy = op->y;
		own[i].alive = op->alive;
		own[i].in_transit = op->in_transit;

		enemy[i].id = i;
		enemy[i].x = ep->known_x;
		enemy[i].y = ep->known_y;
		enemy[i].prevx = ep->prev_x;
		enemy[i].prevy = ep->prev_y;
		enemy[i].pickedx = ep->known_x;
		enemy[i].pickedy = ep->known_y;
		enemy[i].alive = ep->alive;
		enemy[i].in_transit = 0;
	}
}

static int count_placed_own_pieces(const struct player_board *b)
{
	int i, count = 0;

	for (i = 0; i < NUM_PIECES; i++) {
		if (b->own[i].x != UNKNOWN_POS && b->own[i].y != UNKNOWN_POS)
			count++;
	}
	return count;
}

static void update_status(struct gui *ui)
{
	if (!ui->use_opponent) {
		if (ui->mode == MODE_FREE)
			snprintf(ui->status_text, sizeof(ui->status_text),
				 "Manual Free-form mode (no rules enforced)");
		else if (ui->mode == MODE_BOARD_SETUP)
			snprintf(ui->status_text, sizeof(ui->status_text),
				 "Manual Setup (%s): arrange your pieces",
				 ui->playing_as == PLAYING_AS_RED ? "Red" : "Blue");
		else
			snprintf(ui->status_text, sizeof(ui->status_text),
				 "Manual Play (%s): move pieces & communicate externally",
				 ui->playing_as == PLAYING_AS_RED ? "Red" : "Blue");
		ui->auto_setup_btn.enabled = 0;
		ui->pass_btn.enabled = 0;
		ui->play_again_btn.visible = 0;
		layout_ui(ui);
		ui->dirty = 1;
		return;
	}

	if (ui->mode == MODE_FREE) {
		snprintf(ui->status_text, sizeof(ui->status_text),
			 "Free-form mode (vs Computer paused)");
		ui->auto_setup_btn.enabled = 0;
		ui->pass_btn.enabled = 0;
		ui->play_again_btn.visible = 0;
		layout_ui(ui);
		ui->dirty = 1;
		return;
	}

	if (ui->board.phase == PHASE_SETUP) {
		int placed = count_placed_own_pieces(&ui->board);
		snprintf(ui->status_text, sizeof(ui->status_text),
			 "Setup (%s): place all %d pieces (%d/%d placed) or click Auto Setup, then select Play Game",
			 ui->board.side == SIDE_RED ? "Red" : "Blue",
			 NUM_PIECES, placed, NUM_PIECES);
		ui->auto_setup_btn.enabled = 1;
		ui->pass_btn.enabled = 0;
	} else if (ui->board.phase == PHASE_GAME_OVER) {
		switch (ui->board.result) {
		case RESULT_RED_WINS:
			snprintf(ui->status_text, sizeof(ui->status_text),
				 "Game Over: Red wins! (%s)",
				 ui->board.side == SIDE_RED ? "You win!" : "Computer wins.");
			break;
		case RESULT_BLUE_WINS:
			snprintf(ui->status_text, sizeof(ui->status_text),
				 "Game Over: Blue wins! (%s)",
				 ui->board.side == SIDE_BLUE ? "You win!" : "Computer wins.");
			break;
		case RESULT_DRAW:
		default:
			snprintf(ui->status_text, sizeof(ui->status_text),
				 "Game Over: Draw!");
			break;
		}
		ui->auto_setup_btn.enabled = 0;
		ui->pass_btn.enabled = 0;
	} else if (ui->board.phase == PHASE_RESOLVING) {
		snprintf(ui->status_text, sizeof(ui->status_text),
			 "Resolving collisions...");
		ui->auto_setup_btn.enabled = 0;
		ui->pass_btn.enabled = 0;
	} else if (ui->board.current_turn == ui->board.side) {
		int can_move = dmost_has_any_legal_move(&ui->board);

		if (can_move) {
			snprintf(ui->status_text, sizeof(ui->status_text),
				 "Turn %d: Your turn (%s) - click a piece to move",
				 ui->board.turn_number,
				 ui->board.side == SIDE_RED ? "Red" : "Blue");
		} else {
			snprintf(ui->status_text, sizeof(ui->status_text),
				 "Turn %d: No legal moves available - click Pass Turn",
				 ui->board.turn_number);
		}
		ui->auto_setup_btn.enabled = 0;
		ui->pass_btn.enabled = !can_move;
	} else {
		snprintf(ui->status_text, sizeof(ui->status_text),
			 "Turn %d: Computer's turn (%s)...",
			 ui->board.turn_number,
			 ui->board.current_turn == SIDE_RED ? "Red" : "Blue");
		ui->auto_setup_btn.enabled = 0;
		ui->pass_btn.enabled = 0;
	}

	ui->play_again_btn.visible = (ui->board.phase == PHASE_GAME_OVER);
	layout_ui(ui);
	ui->dirty = 1;
}

static void cancel_poll_timer(struct gui *ui)
{
	ui->poll_scheduled = 0;
}

static void schedule_opponent_poll(struct gui *ui, Uint32 delay_ms)
{
	if (!ui->poll_scheduled && ui->opponent != NULL &&
	    ui->board.phase != PHASE_GAME_OVER) {
		ui->poll_scheduled = 1;
		ui->poll_due_ms = SDL_GetTicks() + delay_ms;
	}
}

static void reset_piece_arrays(struct gui *ui)
{
	int i;

	for (i = 0; i < NUM_PIECES; i++) {
		p1[i].x = UNKNOWN;
		p1[i].y = UNKNOWN;
		p1[i].prevx = UNKNOWN;
		p1[i].prevy = UNKNOWN;
		p1[i].pickedx = UNKNOWN;
		p1[i].pickedy = UNKNOWN;
		p1[i].alive = 1;
		p1[i].in_transit = 0;
		p1[i].id = i;

		p2[i].x = UNKNOWN;
		p2[i].y = UNKNOWN;
		p2[i].prevx = UNKNOWN;
		p2[i].prevy = UNKNOWN;
		p2[i].pickedx = UNKNOWN;
		p2[i].pickedy = UNKNOWN;
		p2[i].alive = 1;
		p2[i].in_transit = 0;
		p2[i].id = i;
	}
	ui->holding = NULL;
	memset(ui->visited, 0, sizeof(ui->visited));
}

static void reset_managed_game(struct gui *ui)
{
	enum player_side side = (ui->playing_as == PLAYING_AS_RED) ?
				SIDE_RED : SIDE_BLUE;

	cancel_poll_timer(ui);
	clear_animations(ui);
	dismiss_game_over_dialog(ui);
	if (ui->opponent != NULL) {
		dmost_opponent_destroy(ui->opponent);
		ui->opponent = NULL;
	}

	reset_piece_arrays(ui);
	dmost_board_init(&ui->board, side, SIDE_RED);
	dmost_board_set_event_cb(&ui->board, on_board_event, ui);
	ui->mode = MODE_BOARD_SETUP;
	ui->piece_box_open = 2.0;
	ui->mode_dd.active_index = 0;
	clear_log(ui);
	clear_recent_collisions(ui);
	if (ui->use_opponent)
		append_log(ui, "New game vs Computer. Place your pieces or click Auto Setup.", 0);
	else
		append_log(ui, "Manual offline mode.", 0);
	sync_ui_from_board(ui);
	update_status(ui);
	ui->dirty = 1;
}

static void pump_resolution_messages(struct gui *ui)
{
	int steps = 0;

	while (ui->opponent != NULL && ui->board.phase == PHASE_RESOLVING &&
	       steps < 64) {
		struct dmost_msg msg, reply;
		int has_reply = 0;
		int rc = dmost_opponent_poll(ui->opponent, &msg);

		if (rc <= 0)
			break;
		if (!dmost_handle_message(&ui->board, &msg, &reply, &has_reply))
			break;
		if (has_reply)
			dmost_opponent_send(ui->opponent, &reply);
		steps++;
	}
}

static void process_opponent_poll(struct gui *ui)
{
	struct dmost_msg msg, reply;
	int has_reply = 0;
	int rc;

	if (!ui->use_opponent || ui->opponent == NULL ||
	    ui->board.phase == PHASE_GAME_OVER ||
	    ui->board.phase == PHASE_SETUP) {
		ui->poll_scheduled = 0;
		return;
	}

	if (ui->board.phase == PHASE_PLAYING &&
	    ui->board.current_turn == ui->board.side) {
		ui->poll_scheduled = 0;
		return;
	}

	if (ui->anim_count > 0) {
		ui->poll_due_ms = SDL_GetTicks() + 50;
		return;
	}

	snapshot_board_for_anim(ui);
	rc = dmost_opponent_poll(ui->opponent, &msg);
	if (rc == 1) {
		if (dmost_handle_message(&ui->board, &msg, &reply, &has_reply)) {
			if (has_reply)
				dmost_opponent_send(ui->opponent, &reply);
			pump_resolution_messages(ui);
		}
		finalize_enemy_move_anims(ui);
		sync_ui_from_board(ui);
		update_status(ui);
		ui->dirty = 1;
	}

	if (ui->board.phase == PHASE_GAME_OVER ||
	    (ui->board.phase == PHASE_PLAYING &&
	     ui->board.current_turn == ui->board.side)) {
		ui->poll_scheduled = 0;
	} else {
		ui->poll_due_ms = SDL_GetTicks() + 300;
	}
}

static void start_managed_game(struct gui *ui)
{
	struct dmost_msg human_ready, opp_msg, reply;
	int has_reply = 0;
	enum player_side opp_side;
	uint32_t seed;

	if (!dmost_setup_is_complete(&ui->board)) {
		append_log(ui, "Place all 18 pieces before starting (or click Auto Setup).", 0);
		ui->mode = MODE_BOARD_SETUP;
		ui->piece_box_open = 2.0;
		ui->mode_dd.active_index = 0;
		update_status(ui);
		ui->dirty = 1;
		return;
	}

	if (ui->opponent != NULL) {
		sync_ui_from_board(ui);
		ui->mode = MODE_PLAYING;
		update_status(ui);
		ui->dirty = 1;
		return;
	}

	opp_side = dmost_other_side(ui->board.side);
	seed = (uint32_t)time(NULL) ^ (++ui->game_counter * 2654435761u);
	ui->opponent = dmost_ai_opponent_create(opp_side, SIDE_RED, seed);
	if (ui->opponent == NULL) {
		append_log(ui, "Failed to initialize computer opponent.", 0);
		ui->mode = MODE_BOARD_SETUP;
		ui->mode_dd.active_index = 0;
		update_status(ui);
		return;
	}

	/* Poll opponent's MSG_READY before sending ours so AI waits for start */
	if (dmost_opponent_poll(ui->opponent, &opp_msg) == 1) {
		dmost_handle_message(&ui->board, &opp_msg, &reply, &has_reply);
		if (has_reply)
			dmost_opponent_send(ui->opponent, &reply);
	}

	if (dmost_mark_ready(&ui->board, &human_ready))
		dmost_opponent_send(ui->opponent, &human_ready);

	ui->mode = MODE_PLAYING;
	ui->piece_box_open = 1.0;
	append_log(ui, "Game started! Red moves first.", 0);
	sync_ui_from_board(ui);
	update_status(ui);
	ui->dirty = 1;

	if (ui->board.phase == PHASE_PLAYING &&
	    ui->board.current_turn != ui->board.side)
		schedule_opponent_poll(ui, 300);
}

static void invert_clicked(struct gui *ui)
{
	ui->inverted = !ui->inverted;
	ui->dirty = 1;
}

static void help_clicked(struct gui *ui)
{
	sdl_modal_open_help(&ui->help_modal, "DMOST Help", help_instructions);
	sdl_modal_layout(&ui->help_modal, ui->win_w, ui->win_h);
	ui->dirty = 1;
}

static void quit_clicked(struct gui *ui)
{
	cancel_poll_timer(ui);
	clear_animations(ui);
	dismiss_game_over_dialog(ui);
	if (ui->opponent != NULL) {
		dmost_opponent_destroy(ui->opponent);
		ui->opponent = NULL;
	}
	ui->quit = 1;
}

static void auto_setup_clicked(struct gui *ui)
{
	if (!ui->use_opponent || ui->board.phase != PHASE_SETUP ||
	    ui->board.own_ready)
		return;

	ui->holding = NULL;
	ui->setup_rng ^= (uint32_t)time(NULL);
	if (ui->setup_rng == 0)
		ui->setup_rng = 0x12345678u;
	dmost_ai_setup_board(&ui->board, &ui->setup_rng);
	sync_ui_from_board(ui);
	update_status(ui);
	append_log(ui, "Board automatically arranged. Click Play Game when ready.", 0);
	ui->dirty = 1;
}

static void pass_clicked(struct gui *ui)
{
	struct dmost_msg msg;

	if (!is_managed_mode(ui) || ui->opponent == NULL)
		return;
	if (!dmost_has_any_legal_move(&ui->board))
		clear_recent_collisions(ui);
	snapshot_board_for_anim(ui);
	if (!dmost_initiate_pass(&ui->board, &msg))
		return;

	dmost_opponent_send(ui->opponent, &msg);
	pump_resolution_messages(ui);
	finalize_enemy_move_anims(ui);
	sync_ui_from_board(ui);
	update_status(ui);
	ui->dirty = 1;

	if (ui->board.phase == PHASE_PLAYING &&
	    ui->board.current_turn != ui->board.side)
		schedule_opponent_poll(ui, 350);
}

static int invert(int y)
{
	if (!ui.inverted)
		return y;
	return 9 - y;
}

static int is_piece_queued_in_anim(const struct gui *ui, const struct piece *p)
{
	int i;

	for (i = 0; i < ui->anim_count; i++) {
		int idx = (ui->anim_head + i) % ANIM_QUEUE_CAP;
		const struct anim_step *s = &ui->anim_queue[idx];

		if (s->kind == ANIM_SLIDE && s->piece == p)
			return 1;
	}
	return 0;
}

static void count_square_stack(const struct piece *target, struct piece *list,
			       int *stack_total, int *stack_idx, int before)
{
	int j;

	for (j = 0; list[j].name != NULL; j++) {
		if (list[j].x == UNKNOWN || !list[j].alive ||
		    &list[j] == ui.holding ||
		    is_piece_queued_in_anim(&ui, &list[j]))
			continue;
		if (list[j].x == target->x && list[j].y == target->y) {
			if (before || &list[j] < target)
				(*stack_idx)++;
			(*stack_total)++;
		}
	}
}

static void draw_pieces_on_board(struct gui *ui, struct piece *p)
{
	int i;
	double cell_w = ui->xdim / (12.0 * ui->piece_box_open);
	double cell_h = ui->ydim / 12.0;

	for (i = 0; p[i].name != NULL; i++) {
		int stack_total = 0, stack_idx = 0;
		double ox = 0.0, oy = 0.0;
		double scale = 1.0;

		if (p[i].x == UNKNOWN || !p[i].alive)
			continue;
		if (&p[i] == ui->holding || is_piece_queued_in_anim(ui, &p[i]))
			continue;

		if (p == p1) {
			count_square_stack(&p[i], p1, &stack_total, &stack_idx, 0);
			count_square_stack(&p[i], p2, &stack_total, &stack_idx, 0);
		} else {
			count_square_stack(&p[i], p1, &stack_total, &stack_idx, 1);
			count_square_stack(&p[i], p2, &stack_total, &stack_idx, 0);
		}

		if (stack_total > 1) {
			double angle = 2.0 * M_PI * (double)stack_idx /
				       (double)stack_total;
			ox = cos(angle) * cell_w * 0.18;
			oy = sin(angle) * cell_h * 0.18;
			scale = 0.78;
		}

		p[i].sx = p[i].x * cell_w + 1.5 * cell_w + ox;
		p[i].sy = invert(p[i].y) * cell_h + 1.5 * cell_h + oy;
		generic_draw_piece_on_board(&p[i], ui, scale);
	}
}

static void generic_draw_piece_off_board(struct piece *p, struct gui *ui,
					 double radius)
{
	double alpha_fill = p->alive ? 0.25 : 0.08;
	double alpha_stroke = p->alive ? 1.0 : 0.35;
	float lw = (float)(MIN(ui->xdim, ui->ydim) / 200.0);

	sdl_gfx_fill_circle(ui->renderer, (float)p->sx, (float)p->sy,
			    (float)radius,
			    sdl_rgba_f(p->r, p->g, p->b, alpha_fill));
	sdl_gfx_stroke_circle(ui->renderer, (float)p->sx, (float)p->sy,
			      (float)(radius * 1.1), lw,
			      sdl_rgba_f(p->r, p->g, p->b, alpha_stroke),
			      0.0f, 0.0f);

	if (radius > 12.0 && radius < 23.0) {
		sdl_font_draw(ui->renderer, &ui->font,
			      (float)p->sx, (float)p->sy,
			      (float)(MIN(ui->xdim, ui->ydim) / 32.0),
			      p->name, SDL_ALIGN_CENTER,
			      sdl_rgba_f(1.0, 1.0, 1.0, p->alive ? 1.0 : 0.45));
	} else if (radius >= 23.0) {
		sdl_font_draw(ui->renderer, &ui->font,
			      (float)p->sx, (float)p->sy,
			      (float)(MIN(ui->xdim, ui->ydim) / 20.0),
			      p->name, SDL_ALIGN_CENTER,
			      sdl_rgba_f(1.0, 1.0, 1.0, p->alive ? 1.0 : 0.45));
	}

	if (!p->alive) {
		float d = (float)(radius * 0.75);
		SDL_Color xcol = sdl_rgba_f(0.9, 0.2, 0.2, 0.75);

		sdl_gfx_thick_line(ui->renderer,
				   (float)p->sx - d, (float)p->sy - d,
				   (float)p->sx + d, (float)p->sy + d,
				   2.0f, xcol, 0);
		sdl_gfx_thick_line(ui->renderer,
				   (float)p->sx + d, (float)p->sy - d,
				   (float)p->sx - d, (float)p->sy + d,
				   2.0f, xcol, 0);
	}
}

static void draw_dead_piece_on_board(const struct piece *p, struct gui *ui,
				     double cx, double cy, double radius_scale)
{
	double radius;
	float d;
	SDL_Color xcol;

	if (ui->xdim / ui->piece_box_open < ui->ydim)
		radius = (0.75 * ui->xdim / (12.0 * ui->piece_box_open)) / 2.0;
	else
		radius = (0.75 * ui->ydim / 12.0) / 2.0;
	radius *= radius_scale;
	d = (float)(radius * 0.72);

	sdl_gfx_fill_circle(ui->renderer, (float)cx, (float)cy, (float)radius,
			    sdl_rgba_f(p->r, p->g, p->b, 0.14));
	sdl_gfx_stroke_circle(ui->renderer, (float)cx, (float)cy,
			      (float)(radius * 1.05),
			      (float)(MIN(ui->xdim, ui->ydim) / 220.0),
			      sdl_rgba_f(p->r, p->g, p->b, 0.55), 0.0f, 0.0f);

	if (radius > 8.0) {
		double fh = MIN(ui->xdim, ui->ydim) / 32.0 * radius_scale;
		if (fh < 8.0)
			fh = 8.0;
		sdl_font_draw(ui->renderer, &ui->font, (float)cx, (float)cy,
			      (float)fh, p->name, SDL_ALIGN_CENTER,
			      sdl_rgba_f(1.0, 1.0, 1.0, 0.80));
	}

	xcol = sdl_rgba_f(1.0, 0.35, 0.2, 0.85);
	sdl_gfx_thick_line(ui->renderer,
			   (float)cx - d, (float)cy - d,
			   (float)cx + d, (float)cy + d, 2.0f, xcol, 0);
	sdl_gfx_thick_line(ui->renderer,
			   (float)cx + d, (float)cy - d,
			   (float)cx - d, (float)cy + d, 2.0f, xcol, 0);
}

static int has_live_piece_at(int x, int y)
{
	int j;

	for (j = 0; p1[j].name != NULL; j++) {
		if (p1[j].alive && p1[j].x == x && p1[j].y == y &&
		    &p1[j] != ui.holding &&
		    !is_piece_queued_in_anim(&ui, &p1[j]))
			return 1;
	}
	for (j = 0; p2[j].name != NULL; j++) {
		if (p2[j].alive && p2[j].x == x && p2[j].y == y &&
		    &p2[j] != ui.holding &&
		    !is_piece_queued_in_anim(&ui, &p2[j]))
			return 1;
	}
	return 0;
}

static void count_dead_square_stack(const struct piece *target,
				    struct piece *list, int *total, int *idx,
				    int before)
{
	int j;

	for (j = 0; list[j].name != NULL; j++) {
		if (list[j].alive || list[j].x == UNKNOWN ||
		    is_piece_queued_in_anim(&ui, &list[j]))
			continue;
		if (list[j].x == target->x && list[j].y == target->y) {
			if (before || &list[j] < target)
				(*idx)++;
			(*total)++;
		}
	}
}

static void draw_dead_pieces_on_board_list(struct gui *ui, struct piece *p)
{
	int i;
	double cell_w = ui->xdim / (12.0 * ui->piece_box_open);
	double cell_h = ui->ydim / 12.0;

	for (i = 0; p[i].name != NULL; i++) {
		int dead_total = 0, dead_idx = 0, live_here;
		double cx, cy, scale;

		if (p[i].alive || p[i].x == UNKNOWN ||
		    is_piece_queued_in_anim(ui, &p[i]))
			continue;

		if (p == p1) {
			count_dead_square_stack(&p[i], p1, &dead_total,
						&dead_idx, 0);
			count_dead_square_stack(&p[i], p2, &dead_total,
						&dead_idx, 0);
		} else {
			count_dead_square_stack(&p[i], p1, &dead_total,
						&dead_idx, 1);
			count_dead_square_stack(&p[i], p2, &dead_total,
						&dead_idx, 0);
		}

		live_here = has_live_piece_at(p[i].x, p[i].y);
		cx = p[i].x * cell_w + 1.5 * cell_w;
		cy = invert(p[i].y) * cell_h + 1.5 * cell_h;

		if (live_here) {
			cx += cell_w * 0.26 - dead_idx * cell_w * 0.22;
			cy += cell_h * 0.26;
			scale = 0.48;
		} else if (dead_total > 1) {
			double angle = 2.0 * M_PI * (double)dead_idx /
				       (double)dead_total;
			cx += cos(angle) * cell_w * 0.20;
			cy += sin(angle) * cell_h * 0.16;
			scale = 0.68;
		} else {
			scale = 0.80;
		}

		draw_dead_piece_on_board(&p[i], ui, cx, cy, scale);
	}
}

static void draw_dead_pieces_on_board(struct gui *ui)
{
	if (!is_managed_mode(ui))
		return;
	draw_dead_pieces_on_board_list(ui, p1);
	draw_dead_pieces_on_board_list(ui, p2);
}

static int should_show_in_tray(const struct gui *ui, const struct piece *p)
{
	int is_own;

	if (p->x != UNKNOWN && p->alive)
		return 0;
	if (&p[0] == ui->holding)
		return 0;

	if (!is_managed_mode(ui))
		return 1;

	/* Destroyed pieces with known squares stay rendered on the board */
	if (!p->alive && p->x != UNKNOWN)
		return 0;

	is_own = ((ui->playing_as == PLAYING_AS_RED && p->r > 0.9) ||
		  (ui->playing_as == PLAYING_AS_BLUE && p->b > 0.9));
	return is_own;
}

static void draw_pieces_off_board(struct gui *ui, struct piece *p,
				  int *initial_count)
{
	int i, count;
	double leftx, xwidth, slotwidth;
	double radius;

	leftx = 11.0 * ui->xdim / (12.0 * ui->piece_box_open);
	xwidth = ui->xdim - leftx;
	slotwidth = MIN(xwidth / 6.0, ui->xdim / 12.0);
	radius = slotwidth / 4.0;

	count = *initial_count;
	for (i = 0; p[i].name != NULL; i++) {
		if (!should_show_in_tray(ui, &p[i])) {
			if (p[i].x == UNKNOWN && &p[i] != ui->holding) {
				p[i].sx = -1000;
				p[i].sy = -1000;
			}
			continue;
		}
		p[i].sx = leftx + (count % 4) * slotwidth + slotwidth / 2.0;
		p[i].sy = (ui->ydim / 12.0) + (count / 4) * slotwidth / 1.5;
		generic_draw_piece_off_board(&p[i], ui, radius);
		count++;
	}
	*initial_count = count;
}

static int is_own_piece(const struct gui *ui, const struct piece *p)
{
	if (ui->playing_as == PLAYING_AS_RED)
		return p->r > 0.9;
	return p->b > 0.9;
}

static int legal_move(struct gui *ui, struct piece *p, int x, int y)
{
	int fromx, fromy;

	if (is_managed_mode(ui)) {
		if (!is_own_piece(ui, p))
			return 0;
		if (ui->mode == MODE_BOARD_SETUP)
			return dmost_can_place_piece(&ui->board, p->id, x, y);
		if (ui->mode == MODE_PLAYING) {
			if (x == p->pickedx && y == p->pickedy)
				return 1;
			return dmost_is_legal_move(&ui->board, p->id, x, y);
		}
		return 0;
	}

	fromx = p->pickedx;
	fromy = p->pickedy;

	/* no move at all is legal always */
	if (x == fromx && y == fromy)
		return 1;

	if (ui->mode == MODE_PLAYING && fromx == -1)
		return 1;

	if (ui->mode == MODE_PLAYING) {
		switch (p->strength) {
		case ZFIGHTER:
		case BFIGHTER:
			return (abs(fromx - x) == abs(fromy - y) ||
				(fromx == x) || (fromy == y));
		case XFIGHTER:
			return (fromx == x) || (fromy == y);
		case YFIGHTER:
			return (abs(fromx - x) == abs(fromy - y));
		case HOMEPLANET:
		case ASTEROID:
			return 0;
		case WFIGHTER:
			return ((abs(fromx - x) == 1 && abs(fromy - y) == 2) ||
				(abs(fromx - x) == 2 && abs(fromy - y) == 1));
		default:
			break;
		}
		return 0;
	}

	if (ui->mode == MODE_BOARD_SETUP) {
		if (p->r > 0.9) /* red piece */
			return (y < 5 && ui->playing_as == PLAYING_AS_RED);
		else
			return (y >= 5 && ui->playing_as == PLAYING_AS_BLUE);
	}

	/* must be free mode, anything goes */
	if (ui->mode == MODE_FREE)
		return 1;

	return 0;
}

static void highlight_square(struct gui *ui, int x, int y,
			     double r, double g, double b, double a)
{
	double x1, y1, x2, y2;
	SDL_Color col = sdl_rgba_f(r, g, b, a);

	x1 = (x + 1) * ui->xdim / (12.0 * ui->piece_box_open);
	x2 = (x + 2) * ui->xdim / (12.0 * ui->piece_box_open);
	y1 = (invert(y) + 1) * (ui->ydim / 12.0);
	y2 = (invert(y) + 2) * (ui->ydim / 12.0);

	sdl_gfx_fill_rect(ui->renderer, (float)x1, (float)y1,
			  (float)(x2 - x1), (float)(y2 - y1), col);
}

static void highlight_legal_square(struct gui *ui, int x, int y)
{
	highlight_square(ui, x, y, 0.0, 1.0, 0.0, 0.14);
}

static void highlight_legal_moves(struct gui *ui)
{
	int x, y;

	if (ui->holding == NULL || ui->mode == MODE_FREE)
		return;

	if (ui->holding->pickedx == -1 && ui->mode != MODE_BOARD_SETUP)
		return;

	for (x = 0; x < 10; x++)
		for (y = 0; y < 10; y++)
			if (legal_move(ui, ui->holding, x, y))
				highlight_legal_square(ui, x, y);
}

static void highlight_visited_squares(struct gui *ui)
{
	int x, y;
	double r = 0.0, b = 0.0;
	enum player_side enemy_side = (ui->playing_as == PLAYING_AS_RED) ?
				      SIDE_BLUE : SIDE_RED;

	if (ui->playing_as == PLAYING_AS_RED)
		r = 1.0;
	if (ui->playing_as == PLAYING_AS_BLUE)
		b = 1.0;

	if (is_managed_mode(ui)) {
		for (x = 0; x < BOARD_SIZE; x++) {
			for (y = 0; y < BOARD_SIZE; y++) {
				if (dmost_side_owns_row(enemy_side, y) &&
				    ui->board.enemy_clear[x][y])
					highlight_square(ui, x, y,
							 0.1, 0.85, 0.55, 0.14);
			}
		}
	}
	for (x = 0; x < 10; x++)
		for (y = 0; y < 10; y++)
			if (ui->visited[x][y])
				highlight_square(ui, x, y, r, 0.0, b, 0.2);
}

static void highlight_recent_collisions(struct gui *ui)
{
	int i;
	double cell_w = ui->xdim / (12.0 * ui->piece_box_open);
	double cell_h = ui->ydim / 12.0;

	if (!is_managed_mode(ui) || ui->recent_collision_count <= 0)
		return;

	for (i = 0; i < ui->recent_collision_count; i++) {
		int x = ui->recent_collisions[i].x;
		int y = ui->recent_collisions[i].y;
		double x1 = (x + 1) * cell_w;
		double y1 = (invert(y) + 1) * cell_h;

		highlight_square(ui, x, y, 1.0, 0.55, 0.0, 0.26);
		sdl_gfx_stroke_rect(ui->renderer,
				    (float)(x1 + 1.5), (float)(y1 + 1.5),
				    (float)(cell_w - 3.0),
				    (float)(cell_h - 3.0),
				    2.5f, sdl_rgba_f(1.0, 0.72, 0.15, 0.92));
	}
}

static void draw_active_animation(struct gui *ui)
{
	const struct anim_step *step;
	double cell_w = ui->xdim / (12.0 * ui->piece_box_open);
	double cell_h = ui->ydim / 12.0;
	double radius = MIN(cell_w, cell_h) * 0.375;
	double t, ease;

	if (ui->anim_count <= 0)
		return;

	step = &ui->anim_queue[ui->anim_head];
	t = (step->duration_ms > 0) ?
	    ((double)step->elapsed_ms / (double)step->duration_ms) : 1.0;
	if (t < 0.0)
		t = 0.0;
	if (t > 1.0)
		t = 1.0;
	ease = t * t * (3.0 - 2.0 * t);

	if (step->kind == ANIM_SLIDE && step->path_len >= 2 &&
	    step->piece != NULL) {
		double total_segs = (double)(step->path_len - 1);
		double pos = ease * total_segs;
		int seg = (int)pos;
		double frac, x0, y0, x1, y1, ix, iy;
		SDL_FPoint pts[MAX_PATH_LEN + 1];
		int k;

		if (seg >= step->path_len - 1)
			seg = step->path_len - 2;
		frac = pos - (double)seg;

		x0 = (step->path[seg].x + 1.5) * cell_w;
		y0 = (invert(step->path[seg].y) + 1.5) * cell_h;
		x1 = (step->path[seg + 1].x + 1.5) * cell_w;
		y1 = (invert(step->path[seg + 1].y) + 1.5) * cell_h;
		ix = x0 + (x1 - x0) * frac;
		iy = y0 + (y1 - y0) * frac;

		/* Animated glowing trail along path */
		for (k = 0; k <= seg; k++) {
			pts[k].x = (float)((step->path[k].x + 1.5) * cell_w);
			pts[k].y = (float)((invert(step->path[k].y) + 1.5) * cell_h);
		}
		pts[seg + 1].x = (float)ix;
		pts[seg + 1].y = (float)iy;
		sdl_gfx_polyline(ui->renderer, pts, seg + 2,
				 (float)(MIN(cell_w, cell_h) * 0.14),
				 sdl_rgba_f(step->r, step->g, step->b, 0.72));

		/* Bright moving halo */
		sdl_gfx_stroke_circle(ui->renderer, (float)ix, (float)iy,
				      (float)(radius * 1.32), 2.5f,
				      sdl_rgba_f(1.0, 1.0, 0.7, 0.6),
				      0.0f, 0.0f);

		/* Piece disk & label */
		sdl_gfx_fill_circle(ui->renderer, (float)ix, (float)iy,
				    (float)radius,
				    sdl_rgba_f(step->r, step->g, step->b, 0.40));
		sdl_gfx_stroke_circle(ui->renderer, (float)ix, (float)iy,
				      (float)(radius * 1.1),
				      (float)(MIN(ui->xdim, ui->ydim) / 180.0),
				      sdl_rgba_f(step->r, step->g, step->b, 1.0),
				      0.0f, 0.0f);

		if (radius > 12.0) {
			sdl_font_draw(ui->renderer, &ui->font,
				      (float)ix, (float)iy,
				      (float)(MIN(ui->xdim, ui->ydim) / 32.0),
				      step->piece->name, SDL_ALIGN_CENTER,
				      sdl_rgba_f(1.0, 1.0, 1.0, 1.0));
		}
	} else if (step->kind == ANIM_APPEAR) {
		double cx = (step->x + 1.5) * cell_w;
		double cy = (invert(step->y) + 1.5) * cell_h;
		double alpha = 0.92 * (1.0 - t);

		sdl_gfx_stroke_circle(ui->renderer, (float)cx, (float)cy,
				      (float)(radius * (1.05 + 1.35 * ease)),
				      3.0f,
				      sdl_rgba_f(step->r, step->g, step->b, alpha),
				      0.0f, 0.0f);
		sdl_gfx_stroke_circle(ui->renderer, (float)cx, (float)cy,
				      (float)(radius * (0.95 + 0.70 * ease)),
				      2.0f,
				      sdl_rgba_f(1.0, 1.0, 1.0, alpha * 0.85),
				      0.0f, 0.0f);
	} else if (step->kind == ANIM_IMPACT) {
		double cx = (step->x + 1.5) * cell_w;
		double cy = (invert(step->y) + 1.5) * cell_h;
		double alpha = 0.95 * (1.0 - t);
		double inner_r = radius * 0.35;
		double outer_r = radius * (0.85 + 1.25 * ease);
		SDL_Color ray_col = sdl_rgba_f(1.0, 0.92, 0.55, alpha);
		int k;

		sdl_gfx_stroke_circle(ui->renderer, (float)cx, (float)cy,
				      (float)(radius * (0.55 + 1.75 * ease)),
				      3.5f,
				      sdl_rgba_f(1.0, 0.72, 0.15, alpha),
				      0.0f, 0.0f);

		for (k = 0; k < 8; k++) {
			double ang = 2.0 * M_PI * (double)k / 8.0;

			sdl_gfx_thick_line(ui->renderer,
					   (float)(cx + cos(ang) * inner_r),
					   (float)(cy + sin(ang) * inner_r),
					   (float)(cx + cos(ang) * outer_r),
					   (float)(cy + sin(ang) * outer_r),
					   2.2f, ray_col, 0);
		}
	}
}

static void draw_game_over_banner(struct gui *ui)
{
	double cell_h = ui->ydim / 12.0;
	double board_w = 10.0 * ui->xdim / (12.0 * ui->piece_box_open);
	double left_x = ui->xdim / (12.0 * ui->piece_box_open);
	double cx = left_x + board_w * 0.5;
	double top_y = 11.08 * cell_h;
	double bh = 0.84 * cell_h;
	double cy = top_y + bh * 0.5;
	double br = 0.7, bg = 0.7, bb = 0.8;
	const char *headline = "GAME OVER";
	const char *subline = "";
	char full_text[160];
	int human_won;

	if (!is_managed_mode(ui) || ui->board.phase != PHASE_GAME_OVER ||
	    ui->anim_count > 0 || ui->game_over_modal.active)
		return;

	human_won = ((ui->board.side == SIDE_RED &&
		      ui->board.result == RESULT_RED_WINS) ||
		     (ui->board.side == SIDE_BLUE &&
		      ui->board.result == RESULT_BLUE_WINS));

	if (ui->board.result == RESULT_DRAW) {
		br = 0.55;
		bg = 0.75;
		bb = 0.95;
		headline = "GAME OVER \xE2\x80\x94 DRAW";
		subline = "Neither player can reach the other's Home Planet";
	} else if (human_won) {
		br = 0.25;
		bg = 0.95;
		bb = 0.45;
		headline = (ui->board.result == RESULT_RED_WINS) ?
			   "VICTORY! RED WINS!" : "VICTORY! BLUE WINS!";
		subline = "Opponent's Home Planet destroyed";
	} else {
		br = 1.0;
		bg = 0.30;
		bb = 0.25;
		headline = (ui->board.result == RESULT_RED_WINS) ?
			   "DEFEAT \xE2\x80\x94 RED WINS" :
			   "DEFEAT \xE2\x80\x94 BLUE WINS";
		subline = "Your Home Planet was destroyed";
	}

	snprintf(full_text, sizeof(full_text), "%s  (%s)", headline, subline);

	sdl_gfx_fill_rect(ui->renderer, (float)left_x, (float)top_y,
			  (float)board_w, (float)bh,
			  sdl_rgba_f(0.05, 0.05, 0.10, 0.88));
	sdl_gfx_stroke_rect(ui->renderer, (float)left_x, (float)top_y,
			    (float)board_w, (float)bh, 2.5f,
			    sdl_rgba_f(br, bg, bb, 0.95));

	{
		double fs = MIN(board_w / 28.0, bh * 0.46);
		if (fs < 10.0)
			fs = 10.0;
		sdl_font_draw(ui->renderer, &ui->font, (float)cx, (float)cy,
			      (float)fs, full_text, SDL_ALIGN_CENTER,
			      sdl_rgba_f(br, bg, bb, 1.0));
	}
}

static void render_board(struct gui *ui)
{
	SDL_Rect vp;
	SDL_Color grid_col, label_col;
	int i, off_board_piece_count = 0;

	if (ui->xdim <= 0 || ui->ydim <= 0)
		return;

	if (ui->holding != NULL) {
		ui->holding->sx = ui->mousex;
		ui->holding->sy = ui->mousey;
	}

	vp.x = (int)ui->board_rect.x;
	vp.y = (int)ui->board_rect.y;
	vp.w = ui->xdim;
	vp.h = ui->ydim;
	SDL_RenderSetViewport(ui->renderer, &vp);

	sdl_gfx_fill_rect(ui->renderer, 0.0f, 0.0f,
			  (float)ui->xdim, (float)ui->ydim,
			  sdl_rgba_f(0.0, 0.0, 0.0, 1.0));

	if (ui->galaxy_tex != NULL && ui->galaxy_w > 0 && ui->galaxy_h > 0) {
		SDL_FRect dst;

		dst.x = 0.0f;
		dst.y = 0.0f;
		dst.w = (float)((double)ui->xdim / ui->piece_box_open);
		dst.h = (float)ui->ydim;
		SDL_SetTextureAlphaMod(ui->galaxy_tex, 128);
		SDL_RenderCopyExF(ui->renderer, ui->galaxy_tex, NULL, &dst,
				  0.0, NULL,
				  ui->inverted ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);
	}

	grid_col = sdl_rgba_f(0.5, 1.0, 0.5, 1.0);
	for (i = 1; i <= 11; i++) {
		float gx = (float)(ui->xdim / (12.0 * ui->piece_box_open) * i);
		float gy0 = (float)(ui->ydim / 12.0);
		float gy1 = (float)(11.0 * ui->ydim / 12.0);
		float gy = (float)(i * ui->ydim / 12.0);
		float gx0 = (float)(ui->xdim / (12.0 * ui->piece_box_open));
		float gx1 = (float)(11.0 * ui->xdim / (12.0 * ui->piece_box_open));

		sdl_gfx_dashed_line(ui->renderer, gx, gy0, gx, gy1,
				    1.5f, 2.0f, 3.0f, grid_col);
		sdl_gfx_dashed_line(ui->renderer, gx0, gy, gx1, gy,
				    1.5f, 2.0f, 3.0f, grid_col);
	}

	if (ui->playing_as == PLAYING_AS_RED)
		label_col = sdl_rgba_f(1.0, 0.25, 0.25, 1.0);
	else
		label_col = sdl_rgba_f(0.3, 0.5, 1.0, 1.0);

	for (i = 0; i < 10; i++) {
		char letter[4];
		double y;

		snprintf(letter, sizeof(letter), "%c", 'A' + i);
		y = (i + 1.5) * ui->ydim / 12.0;
		if (ui->inverted)
			y = ui->ydim - y;
		sdl_font_draw(ui->renderer, &ui->font,
			      (float)(ui->xdim / (24.0 * ui->piece_box_open)),
			      (float)y, (float)(ui->ydim / 30.0),
			      letter, SDL_ALIGN_CENTER, label_col);
	}
	for (i = 0; i < 10; i++) {
		char number[4];

		snprintf(number, sizeof(number), "%d", i + 1);
		sdl_font_draw(ui->renderer, &ui->font,
			      (float)((i + 1.5) * ui->xdim /
				      (12.0 * ui->piece_box_open)),
			      (float)(ui->ydim / 24.0),
			      (float)((ui->xdim / ui->piece_box_open) / 30.0),
			      number, SDL_ALIGN_CENTER, label_col);
	}

	highlight_visited_squares(ui);
	highlight_recent_collisions(ui);
	highlight_legal_moves(ui);

	draw_dead_pieces_on_board(ui);
	draw_pieces_on_board(ui, p1);
	draw_pieces_on_board(ui, p2);
	draw_active_animation(ui);
	draw_game_over_banner(ui);
	draw_pieces_off_board(ui, p1, &off_board_piece_count);
	draw_pieces_off_board(ui, p2, &off_board_piece_count);
	if (ui->holding != NULL)
		generic_draw_piece_off_board(ui->holding, ui, ui->xdim / 24.0);

	SDL_RenderSetViewport(ui->renderer, NULL);
}

static void generic_draw_piece_on_board(struct piece *p, struct gui *ui,
					double radius_scale)
{
	double radius;
	double x1, y1, x2, y2;
	int show_trail;

	if (ui->xdim / ui->piece_box_open < ui->ydim)
		radius = (0.75 * ui->xdim / (12.0 * ui->piece_box_open)) / 2.0;
	else
		radius = (0.75 * ui->ydim / 12.0) / 2.0;
	radius *= radius_scale;

	show_trail = (p->prevx != -1) &&
		(is_managed_mode(ui) ||
		 (ui->playing_as == PLAYING_AS_RED && p->r > 0.9) ||
		 (ui->playing_as == PLAYING_AS_BLUE && p->b > 0.9));

	/* draw trail */
	if (show_trail) {
		float line_w = (float)(MIN(ui->xdim / ui->piece_box_open,
					   ui->ydim) / 64.0);
		SDL_Color trail_col = sdl_rgba_f(p->r, p->g, p->b, 0.35);

		x1 = (p->x + 1.5) * ui->xdim / (12.0 * ui->piece_box_open);
		y1 = (invert(p->y) + 1.5) * (ui->ydim / 12.0);
		x2 = (p->prevx + 1.5) * ui->xdim / (12.0 * ui->piece_box_open);
		y2 = (invert(p->prevy) + 1.5) * (ui->ydim / 12.0);
		if (p->strength == WFIGHTER) {
			double tx, ty;
			double factor = ui->inverted ? -1.0 : 1.0;
			SDL_FPoint pts[3];

			if (abs(p->x - p->prevx) < abs(p->y - p->prevy)) {
				if (p->prevx < p->x)
					tx = x1 - ui->xdim / (12.0 * ui->piece_box_open);
				else
					tx = x1 + ui->xdim / (12.0 * ui->piece_box_open);
				ty = y1;
			} else {
				if (p->prevy < p->y)
					ty = y1 - factor * ui->ydim / 12.0;
				else
					ty = y1 + factor * ui->ydim / 12.0;
				tx = x1;
			}
			pts[0].x = (float)x1;
			pts[0].y = (float)y1;
			pts[1].x = (float)tx;
			pts[1].y = (float)ty;
			pts[2].x = (float)x2;
			pts[2].y = (float)y2;
			sdl_gfx_polyline(ui->renderer, pts, 3,
					 line_w, trail_col);
		} else {
			sdl_gfx_thick_line(ui->renderer,
					   (float)x1, (float)y1,
					   (float)x2, (float)y2,
					   line_w, trail_col, 1);
		}
	}

	sdl_gfx_fill_circle(ui->renderer, (float)p->sx, (float)p->sy,
			    (float)radius,
			    sdl_rgba_f(p->r, p->g, p->b, 0.25));

	{
		float lw = (float)(MIN(ui->xdim, ui->ydim) / 200.0);
		float dash = p->in_transit ? 4.0f : 0.0f;
		float gap = p->in_transit ? 3.0f : 0.0f;

		sdl_gfx_stroke_circle(ui->renderer, (float)p->sx, (float)p->sy,
				      (float)(radius * 1.1), lw,
				      sdl_rgba_f(p->r, p->g, p->b, 1.0),
				      dash, gap);
	}

	if (radius > 12.0) {
		double fh = MIN(ui->xdim, ui->ydim) / 32.0 * radius_scale;
		if (fh < 8.0)
			fh = 8.0;
		sdl_font_draw(ui->renderer, &ui->font,
			      (float)p->sx, (float)p->sy, (float)fh,
			      p->name, SDL_ALIGN_CENTER,
			      sdl_rgba_f(1.0, 1.0, 1.0, 1.0));
	}
}

static void hide_or_show_piecebox(struct gui *ui)
{
	if (ui->mode == MODE_BOARD_SETUP) {
		if (ui->piece_box_open != 2.0) {
			ui->piece_box_open = 2.0;
			ui->dirty = 1;
		}
		return;
	}
	if (ui->piece_box_open > 1.0 && ui->mousex < ui->xdim / 2.2) {
		ui->piece_box_open *= 0.8;
		if (ui->piece_box_open < 1.0)
			ui->piece_box_open = 1.0;
		ui->dirty = 1;
	} else if (ui->mousex > 11.0 * (ui->xdim / (12.0 * ui->piece_box_open)) &&
		   ui->piece_box_open < 2.0) {
		ui->piece_box_open *= 1.2;
		if (ui->piece_box_open > 2.0)
			ui->piece_box_open = 2.0;
		ui->dirty = 1;
	}
}

static struct piece *find_closest_piece(const struct gui *ui, struct piece *p,
					struct piece *prev, double x, double y,
					double *best_dist_sq, int own_only,
					int board_only)
{
	int i;
	struct piece *found = prev;

	for (i = 0; p[i].name != NULL; i++) {
		double dx, dy, dist_sq;

		if (own_only && !is_own_piece(ui, &p[i]))
			continue;
		if (is_managed_mode(ui) && !p[i].alive)
			continue;
		if (board_only && (p[i].x == UNKNOWN || !p[i].alive))
			continue;
		if (!board_only && p[i].x == UNKNOWN && !p[i].alive)
			continue;
		if (p[i].sx < -500.0 || p[i].sy < -500.0)
			continue;

		dx = p[i].sx - x;
		dy = p[i].sy - y;
		dist_sq = dx * dx + dy * dy;
		if (dist_sq < *best_dist_sq) {
			found = &p[i];
			*best_dist_sq = dist_sq;
		}
	}
	return found;
}

static int screen_to_board(const struct gui *ui, double mx, double my,
			   int *out_x, int *out_y)
{
	double cell_w = ui->xdim / (12.0 * ui->piece_box_open);
	double cell_h = ui->ydim / 12.0;
	int bx, by;

	if (mx <= cell_w || mx >= 11.0 * cell_w ||
	    my <= cell_h || my >= 11.0 * cell_h)
		return 0;

	bx = (int)((mx - cell_w) / cell_w);
	by = invert((int)((my - cell_h) / cell_h));
	if (bx < 0 || bx >= BOARD_SIZE || by < 0 || by >= BOARD_SIZE)
		return 0;

	*out_x = bx;
	*out_y = by;
	return 1;
}

static void handle_board_button_down(struct gui *ui, Uint8 button,
				     int bx, int by)
{
	int tmpx, tmpy;

	ui->mousex = bx;
	ui->mousey = by;

	if (button == SDL_BUTTON_LEFT) {
		if (ui->holding != NULL) {
			/* Dropping a piece */
			if (ui->mousex >
			    (11.0 * ui->xdim / (12.0 * ui->piece_box_open))) {
				if (is_managed_mode(ui)) {
					if (ui->mode == MODE_BOARD_SETUP) {
						dmost_unplace_piece(&ui->board,
								    ui->holding->id);
						sync_ui_from_board(ui);
						update_status(ui);
					}
					ui->holding = NULL;
					ui->dirty = 1;
					return;
				}
				/* Off the board in manual/free mode */
				ui->holding->x = UNKNOWN;
				ui->holding->sx = -100;
				ui->holding->sy = -100;
				ui->holding->pickedx = -1;
				ui->holding->pickedy = -1;
				ui->holding = NULL;
				ui->dirty = 1;
				return;
			}

			if (screen_to_board(ui, ui->mousex, ui->mousey,
					    &tmpx, &tmpy)) {
				if (!legal_move(ui, ui->holding, tmpx, tmpy))
					return;

				if (is_managed_mode(ui)) {
					if (ui->mode == MODE_BOARD_SETUP) {
						dmost_place_piece(&ui->board,
								  ui->holding->id,
								  tmpx, tmpy);
						sync_ui_from_board(ui);
						update_status(ui);
					} else if (ui->mode == MODE_PLAYING) {
						if (tmpx != ui->holding->pickedx ||
						    tmpy != ui->holding->pickedy) {
							struct dmost_msg msg;

							clear_recent_collisions(ui);
							snapshot_board_for_anim(ui);
							if (dmost_initiate_move(&ui->board,
										ui->holding->id,
										tmpx, tmpy,
										&msg)) {
								ui->holding = NULL;
								if (ui->opponent != NULL)
									dmost_opponent_send(ui->opponent, &msg);
								pump_resolution_messages(ui);
								finalize_enemy_move_anims(ui);
								sync_ui_from_board(ui);
								update_status(ui);
								ui->dirty = 1;
								if (ui->board.phase == PHASE_RESOLVING ||
								    (ui->board.phase == PHASE_PLAYING &&
								     ui->board.current_turn != ui->board.side))
									schedule_opponent_poll(ui, 350);
								return;
							}
						}
					}
					ui->holding = NULL;
					ui->dirty = 1;
					return;
				}

				/* Manual / Free-form drop */
				ui->holding->y = tmpy;
				ui->holding->x = tmpx;
				if (ui->mode == MODE_PLAYING) {
					ui->holding->prevx = ui->holding->pickedx;
					ui->holding->prevy = ui->holding->pickedy;
					ui->holding->pickedx = ui->holding->x;
					ui->holding->pickedy = ui->holding->y;
				} else {
					ui->holding->prevx = -1;
				}
				ui->holding = NULL;
				ui->dirty = 1;
			}
			return;
		}

		/* Picking up a piece */
		if (is_managed_mode(ui) && ui->mode == MODE_PLAYING) {
			if (ui->board.phase != PHASE_PLAYING ||
			    ui->board.current_turn != ui->board.side)
				return;
			clear_animations(ui);
		}

		{
			double best_dist_sq = 32.0 * 32.0;
			int own_only = is_managed_mode(ui);
			int board_only = (is_managed_mode(ui) &&
					  ui->mode == MODE_PLAYING);
			struct piece *found = NULL;

			found = find_closest_piece(ui, p1, found,
						   ui->mousex, ui->mousey,
						   &best_dist_sq, own_only,
						   board_only);
			found = find_closest_piece(ui, p2, found,
						   ui->mousex, ui->mousey,
						   &best_dist_sq, own_only,
						   board_only);
			if (found != NULL) {
				if (board_only &&
				    !dmost_piece_is_mobile(found->strength))
					return;
				ui->holding = found;
				ui->dirty = 1;
			}
		}
		return;
	}

	if (button == SDL_BUTTON_RIGHT) {
		if (ui->holding != NULL) {
			ui->holding = NULL;
			ui->dirty = 1;
			return;
		}
		if (screen_to_board(ui, ui->mousex, ui->mousey, &tmpx, &tmpy)) {
			ui->visited[tmpx][tmpy] = !ui->visited[tmpx][tmpy];
			ui->dirty = 1;
		}
	}
}

static void on_mode_selected(struct gui *ui, int idx)
{
	if (idx < 0 || idx >= (int)ARRAYSIZE(mode_items))
		return;

	ui->holding = NULL;

	if (idx == 3) { /* Begin New Game */
		reset_managed_game(ui);
		return;
	}

	if (idx == 2) { /* Free form */
		ui->mode = MODE_FREE;
		update_status(ui);
		ui->dirty = 1;
		return;
	}

	if (idx == 0) { /* Board Setup */
		if (ui->use_opponent && ui->opponent != NULL) {
			/* Returning to setup after starting resets the match */
			reset_managed_game(ui);
			return;
		}
		ui->mode = MODE_BOARD_SETUP;
		ui->piece_box_open = 2.0;
		update_status(ui);
		ui->dirty = 1;
		return;
	}

	if (idx == 1) { /* Play Game */
		if (ui->use_opponent) {
			start_managed_game(ui);
			return;
		}
		if (ui->mode != MODE_PLAYING) {
			int i;

			for (i = 0; p1[i].name; i++) {
				p1[i].pickedx = p1[i].x;
				p1[i].pickedy = p1[i].y;
			}
			for (i = 0; p2[i].name; i++) {
				p2[i].pickedx = p2[i].x;
				p2[i].pickedy = p2[i].y;
			}
		}
		ui->mode = MODE_PLAYING;
		update_status(ui);
		ui->dirty = 1;
	}
}

static void on_redblue_selected(struct gui *ui, int idx)
{
	if (idx == 0) {
		ui->playing_as = PLAYING_AS_RED;
		ui->inverted = 1;
	} else if (idx == 1) {
		ui->playing_as = PLAYING_AS_BLUE;
		ui->inverted = 0;
	} else {
		return;
	}

	if (ui->use_opponent) {
		reset_managed_game(ui);
	} else {
		update_status(ui);
		ui->dirty = 1;
	}
}

static void on_opponent_selected(struct gui *ui, int idx)
{
	if (idx == 0)
		ui->use_opponent = 1;
	else if (idx == 1)
		ui->use_opponent = 0;
	else
		return;

	reset_managed_game(ui);
}

static void place_toolbar_item(SDL_FRect *rect, float width,
			       float *cx, float *cy, int win_w)
{
	if (*cx > TOOLBAR_PAD && *cx + width > (float)win_w - TOOLBAR_PAD) {
		*cx = TOOLBAR_PAD;
		*cy += TOOLBAR_BTN_H + TOOLBAR_PAD;
	}
	rect->x = *cx;
	rect->y = *cy;
	rect->w = width;
	rect->h = TOOLBAR_BTN_H;
	*cx += width + TOOLBAR_PAD;
}

static void layout_ui(struct gui *ui)
{
	float cx = TOOLBAR_PAD;
	float cy = TOOLBAR_PAD;
	float toolbar_h;
	float bottom_y;
	float col_h = 0.0f;
	float status_right;

	if (ui->win_w < 320)
		ui->win_w = 320;
	if (ui->win_h < 320)
		ui->win_h = 320;

	place_toolbar_item(&ui->mode_dd.rect,
			   sdl_dropdown_preferred_width(mode_items,
							ARRAYSIZE(mode_items)),
			   &cx, &cy, ui->win_w);
	place_toolbar_item(&ui->opponent_dd.rect,
			   sdl_dropdown_preferred_width(opponent_items,
							ARRAYSIZE(opponent_items)),
			   &cx, &cy, ui->win_w);
	place_toolbar_item(&ui->redblue_dd.rect,
			   sdl_dropdown_preferred_width(redblue_items,
							ARRAYSIZE(redblue_items)),
			   &cx, &cy, ui->win_w);
	place_toolbar_item(&ui->auto_setup_btn.rect,
			   sdl_button_preferred_width(ui->auto_setup_btn.label),
			   &cx, &cy, ui->win_w);
	place_toolbar_item(&ui->new_game_btn.rect,
			   sdl_button_preferred_width(ui->new_game_btn.label),
			   &cx, &cy, ui->win_w);
	place_toolbar_item(&ui->invert_btn.rect,
			   sdl_button_preferred_width(ui->invert_btn.label),
			   &cx, &cy, ui->win_w);
	place_toolbar_item(&ui->help_btn.rect,
			   sdl_button_preferred_width(ui->help_btn.label),
			   &cx, &cy, ui->win_w);
	place_toolbar_item(&ui->quit_btn.rect,
			   sdl_button_preferred_width(ui->quit_btn.label),
			   &cx, &cy, ui->win_w);

	toolbar_h = cy + TOOLBAR_BTN_H + TOOLBAR_PAD;
	ui->toolbar_rect = (SDL_FRect){ 0.0f, 0.0f, (float)ui->win_w, toolbar_h };

	bottom_y = (float)ui->win_h - 4.0f;
	ui->log_view.rect = (SDL_FRect){
		4.0f, bottom_y - LOG_BOX_H,
		(float)ui->win_w - 8.0f, LOG_BOX_H
	};
	bottom_y = ui->log_view.rect.y - 2.0f;

	if (ui->last_collision_text[0] != '\0') {
		SDL_Color dummy = { 255, 255, 255, 255 };
		int lines = sdl_font_draw_wrapped(
			NULL, NULL, 0.0f, 0.0f, (float)ui->win_w - 16.0f,
			SDL_UI_FONT_H, 18.0f, ui->last_collision_text,
			dummy, 0, 0);
		if (lines < 1)
			lines = 1;
		col_h = (float)lines * 18.0f + 6.0f;
		ui->collision_rect = (SDL_FRect){
			0.0f, bottom_y - col_h, (float)ui->win_w, col_h
		};
		bottom_y = ui->collision_rect.y;
	} else {
		ui->collision_rect = (SDL_FRect){ 0.0f, bottom_y, 0.0f, 0.0f };
	}

	ui->status_rect = (SDL_FRect){
		0.0f, bottom_y - STATUS_BAR_H, (float)ui->win_w, STATUS_BAR_H
	};

	status_right = (float)ui->win_w - 8.0f;
	{
		float pw = sdl_button_preferred_width(ui->pass_btn.label);
		ui->pass_btn.rect = (SDL_FRect){
			status_right - pw,
			ui->status_rect.y + (STATUS_BAR_H - TOOLBAR_BTN_H) * 0.5f,
			pw, TOOLBAR_BTN_H
		};
		status_right = ui->pass_btn.rect.x - 6.0f;
	}
	if (ui->play_again_btn.visible) {
		float gw = sdl_button_preferred_width(ui->play_again_btn.label);
		ui->play_again_btn.rect = (SDL_FRect){
			status_right - gw,
			ui->status_rect.y + (STATUS_BAR_H - TOOLBAR_BTN_H) * 0.5f,
			gw, TOOLBAR_BTN_H
		};
	}

	{
		float bh = ui->status_rect.y - toolbar_h;
		if (bh < 100.0f)
			bh = 100.0f;
		ui->board_rect = (SDL_FRect){
			0.0f, toolbar_h, (float)ui->win_w, bh
		};
		ui->xdim = (int)ui->board_rect.w;
		ui->ydim = (int)ui->board_rect.h;
	}

	if (ui->help_modal.active)
		sdl_modal_layout(&ui->help_modal, ui->win_w, ui->win_h);
	if (ui->game_over_modal.active)
		sdl_modal_layout(&ui->game_over_modal, ui->win_w, ui->win_h);
}

static void close_all_dropdowns(struct gui *ui)
{
	ui->mode_dd.open = 0;
	ui->mode_dd.hover_index = -1;
	ui->opponent_dd.open = 0;
	ui->opponent_dd.hover_index = -1;
	ui->redblue_dd.open = 0;
	ui->redblue_dd.hover_index = -1;
}

static struct sdl_dropdown *active_open_dropdown(struct gui *ui)
{
	if (ui->mode_dd.open)
		return &ui->mode_dd;
	if (ui->opponent_dd.open)
		return &ui->opponent_dd;
	if (ui->redblue_dd.open)
		return &ui->redblue_dd;
	return NULL;
}

static void update_hover_tooltip(struct gui *ui, float mx, float my)
{
	const char *tip = NULL;

	if (active_open_dropdown(ui) != NULL ||
	    ui->help_modal.active || ui->game_over_modal.active) {
		if (ui->hover_tooltip != NULL) {
			ui->hover_tooltip = NULL;
			ui->dirty = 1;
		}
		return;
	}

	if (sdl_point_in_rect(mx, my, ui->opponent_dd.rect))
		tip = ui->opponent_dd.tooltip;
	else if (sdl_point_in_rect(mx, my, ui->redblue_dd.rect))
		tip = ui->redblue_dd.tooltip;
	else if (sdl_point_in_rect(mx, my, ui->auto_setup_btn.rect))
		tip = ui->auto_setup_btn.tooltip;
	else if (sdl_point_in_rect(mx, my, ui->new_game_btn.rect))
		tip = ui->new_game_btn.tooltip;
	else if (sdl_point_in_rect(mx, my, ui->invert_btn.rect))
		tip = ui->invert_btn.tooltip;
	else if (sdl_point_in_rect(mx, my, ui->quit_btn.rect))
		tip = ui->quit_btn.tooltip;

	if (tip != ui->hover_tooltip) {
		ui->hover_tooltip = tip;
		ui->dirty = 1;
	}
}

static void render_ui(struct gui *ui)
{
	struct sdl_dropdown *open_dd;

	SDL_SetRenderDrawColor(ui->renderer, 16, 18, 24, 255);
	SDL_RenderClear(ui->renderer);

	render_board(ui);

	/* Toolbar */
	sdl_gfx_fill_rect(ui->renderer, ui->toolbar_rect.x, ui->toolbar_rect.y,
			  ui->toolbar_rect.w, ui->toolbar_rect.h,
			  sdl_rgba_f(0.11, 0.13, 0.17, 1.0));
	sdl_gfx_fill_rect(ui->renderer, 0.0f, ui->toolbar_rect.h - 1.0f,
			  (float)ui->win_w, 1.0f,
			  sdl_rgba_f(0.28, 0.32, 0.40, 1.0));

	sdl_dropdown_draw_closed(ui->renderer, &ui->font, &ui->mode_dd);
	sdl_dropdown_draw_closed(ui->renderer, &ui->font, &ui->opponent_dd);
	sdl_dropdown_draw_closed(ui->renderer, &ui->font, &ui->redblue_dd);
	sdl_button_draw(ui->renderer, &ui->font, &ui->auto_setup_btn);
	sdl_button_draw(ui->renderer, &ui->font, &ui->new_game_btn);
	sdl_button_draw(ui->renderer, &ui->font, &ui->invert_btn);
	sdl_button_draw(ui->renderer, &ui->font, &ui->help_btn);
	sdl_button_draw(ui->renderer, &ui->font, &ui->quit_btn);

	/* Status bar */
	sdl_gfx_fill_rect(ui->renderer, ui->status_rect.x, ui->status_rect.y,
			  ui->status_rect.w, ui->status_rect.h,
			  sdl_rgba_f(0.10, 0.12, 0.16, 1.0));
	sdl_gfx_fill_rect(ui->renderer, 0.0f, ui->status_rect.y,
			  (float)ui->win_w, 1.0f,
			  sdl_rgba_f(0.26, 0.30, 0.38, 1.0));

	{
		float right_limit = ui->play_again_btn.visible
				    ? ui->play_again_btn.rect.x - 8.0f
				    : ui->pass_btn.rect.x - 8.0f;
		SDL_Rect clip = {
			8, (int)ui->status_rect.y,
			(int)(right_limit - 8.0f), (int)ui->status_rect.h
		};
		if (clip.w > 0) {
			SDL_RenderSetClipRect(ui->renderer, &clip);
			sdl_font_draw(ui->renderer, &ui->font,
				      10.0f,
				      ui->status_rect.y +
				      ui->status_rect.h * 0.5f,
				      SDL_UI_FONT_H, ui->status_text,
				      SDL_ALIGN_LEFT,
				      sdl_rgba_f(0.90, 0.93, 0.97, 1.0));
			SDL_RenderSetClipRect(ui->renderer, NULL);
		}
	}
	sdl_button_draw(ui->renderer, &ui->font, &ui->play_again_btn);
	sdl_button_draw(ui->renderer, &ui->font, &ui->pass_btn);

	/* Collision banner */
	if (ui->last_collision_text[0] != '\0' && ui->collision_rect.h > 0.0f) {
		SDL_Color col_text = { 242, 110, 56, 255 };

		sdl_gfx_fill_rect(ui->renderer, ui->collision_rect.x,
				  ui->collision_rect.y, ui->collision_rect.w,
				  ui->collision_rect.h,
				  sdl_rgba_f(0.14, 0.09, 0.08, 1.0));
		sdl_font_draw_wrapped(ui->renderer, &ui->font, 8.0f,
				      ui->collision_rect.y + 3.0f,
				      (float)ui->win_w - 16.0f,
				      SDL_UI_FONT_H, 18.0f,
				      ui->last_collision_text, col_text,
				      0, 0);
	}

	/* Scrollable log */
	sdl_log_draw(ui->renderer, &ui->font, &ui->log_view);

	/* Open dropdown popup on top of toolbar/board */
	open_dd = active_open_dropdown(ui);
	if (open_dd != NULL)
		sdl_dropdown_draw_popup(ui->renderer, &ui->font,
					open_dd, ui->win_w);

	/* Tooltip */
	if (ui->hover_tooltip != NULL && open_dd == NULL &&
	    !ui->help_modal.active && !ui->game_over_modal.active) {
		sdl_ui_draw_tooltip(ui->renderer, &ui->font, ui->hover_tooltip,
				    (float)ui->win_mousex,
				    (float)ui->win_mousey,
				    ui->win_w, ui->win_h);
	}

	/* Modals last */
	if (ui->help_modal.active)
		sdl_modal_draw(ui->renderer, &ui->font, &ui->help_modal,
			       ui->win_w, ui->win_h);
	if (ui->game_over_modal.active)
		sdl_modal_draw(ui->renderer, &ui->font, &ui->game_over_modal,
			       ui->win_w, ui->win_h);

	SDL_RenderPresent(ui->renderer);
	ui->dirty = 0;
}

static void update_timers(struct gui *ui, Uint32 now)
{
	if (ui->anim_count > 0) {
		int dt = (int)(now - ui->anim_last_tick_ms);
		struct anim_step *step;

		if (dt < 0)
			dt = 0;
		if (dt > 100)
			dt = 100;
		ui->anim_last_tick_ms = now;

		if (dt > 0) {
			step = &ui->anim_queue[ui->anim_head];
			step->elapsed_ms += dt;
			while (ui->anim_count > 0) {
				step = &ui->anim_queue[ui->anim_head];
				if (step->elapsed_ms < step->duration_ms)
					break;
				ui->anim_head = (ui->anim_head + 1) %
						ANIM_QUEUE_CAP;
				ui->anim_count--;
			}
			ui->dirty = 1;
		}
		if (ui->anim_count <= 0 && ui->game_over_pending &&
		    !ui->game_over_shown && !ui->game_over_scheduled) {
			schedule_game_over_dialog(ui);
		}
	}

	if (ui->poll_scheduled &&
	    (Sint32)(now - ui->poll_due_ms) >= 0) {
		process_opponent_poll(ui);
	}

	if (ui->game_over_scheduled &&
	    (Sint32)(now - ui->game_over_due_ms) >= 0) {
		if (!is_managed_mode(ui) ||
		    ui->board.phase != PHASE_GAME_OVER ||
		    ui->game_over_shown) {
			ui->game_over_scheduled = 0;
		} else if (ui->anim_count > 0) {
			ui->game_over_due_ms = now + 50;
		} else {
			open_game_over_modal(ui);
		}
	}
}

static void handle_event(struct gui *ui, const SDL_Event *ev)
{
	int consumed = 0, modal_redraw = 0;
	enum sdl_modal_result mres;

	if (ev->type == SDL_QUIT) {
		quit_clicked(ui);
		return;
	}

	if (ev->type == SDL_WINDOWEVENT) {
		if (ev->window.event == SDL_WINDOWEVENT_RESIZED ||
		    ev->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
			SDL_GetWindowSize(ui->window, &ui->win_w, &ui->win_h);
			layout_ui(ui);
			ui->dirty = 1;
		} else if (ev->window.event == SDL_WINDOWEVENT_EXPOSED) {
			ui->dirty = 1;
		}
		return;
	}

	if (ui->game_over_modal.active) {
		mres = sdl_modal_handle_event(&ui->game_over_modal, ev,
					      ui->win_w, ui->win_h,
					      &consumed, &modal_redraw);
		if (modal_redraw)
			ui->dirty = 1;
		if (mres == SDL_MODAL_PRIMARY)
			reset_managed_game(ui);
		else if (mres == SDL_MODAL_SECONDARY ||
			 mres == SDL_MODAL_DISMISSED)
			ui->dirty = 1;
		if (consumed)
			return;
	}

	if (ui->help_modal.active) {
		mres = sdl_modal_handle_event(&ui->help_modal, ev,
					      ui->win_w, ui->win_h,
					      &consumed, &modal_redraw);
		if (modal_redraw || mres != SDL_MODAL_NONE)
			ui->dirty = 1;
		if (consumed)
			return;
	}

	if (ev->type == SDL_KEYDOWN) {
		switch (ev->key.keysym.sym) {
		case SDLK_F11:
			ui->fullscreen = !ui->fullscreen;
			SDL_SetWindowFullscreen(ui->window,
						ui->fullscreen
						? SDL_WINDOW_FULLSCREEN_DESKTOP
						: 0);
			SDL_GetWindowSize(ui->window, &ui->win_w, &ui->win_h);
			layout_ui(ui);
			ui->dirty = 1;
			break;
		case SDLK_ESCAPE:
			if (active_open_dropdown(ui) != NULL) {
				close_all_dropdowns(ui);
				ui->dirty = 1;
			} else if (ui->holding != NULL) {
				ui->holding = NULL;
				ui->dirty = 1;
			}
			break;
		default:
			break;
		}
		return;
	}

	if (ev->type == SDL_MOUSEMOTION) {
		float mx = (float)ev->motion.x;
		float my = (float)ev->motion.y;
		struct sdl_dropdown *open_dd;

		ui->win_mousex = ev->motion.x;
		ui->win_mousey = ev->motion.y;

		if (sdl_log_handle_mouse_motion(&ui->log_view, mx, my))
			ui->dirty = 1;

		open_dd = active_open_dropdown(ui);
		if (open_dd != NULL) {
			if (sdl_dropdown_handle_motion(open_dd, mx, my,
						       ui->win_w))
				ui->dirty = 1;
			update_hover_tooltip(ui, mx, my);
			return;
		}

		ui->dirty |= sdl_dropdown_handle_motion(&ui->mode_dd, mx, my, ui->win_w);
		ui->dirty |= sdl_dropdown_handle_motion(&ui->opponent_dd, mx, my, ui->win_w);
		ui->dirty |= sdl_dropdown_handle_motion(&ui->redblue_dd, mx, my, ui->win_w);
		ui->dirty |= sdl_button_handle_motion(&ui->auto_setup_btn, mx, my);
		ui->dirty |= sdl_button_handle_motion(&ui->new_game_btn, mx, my);
		ui->dirty |= sdl_button_handle_motion(&ui->invert_btn, mx, my);
		ui->dirty |= sdl_button_handle_motion(&ui->help_btn, mx, my);
		ui->dirty |= sdl_button_handle_motion(&ui->quit_btn, mx, my);
		ui->dirty |= sdl_button_handle_motion(&ui->pass_btn, mx, my);
		ui->dirty |= sdl_button_handle_motion(&ui->play_again_btn, mx, my);

		update_hover_tooltip(ui, mx, my);

		if (sdl_point_in_rect(mx, my, ui->board_rect)) {
			ui->mousex = (int)(mx - ui->board_rect.x);
			ui->mousey = (int)(my - ui->board_rect.y);
			hide_or_show_piecebox(ui);
			if (ui->holding != NULL)
				ui->dirty = 1;
		}
		return;
	}

	if (ev->type == SDL_MOUSEWHEEL) {
		if (sdl_log_handle_wheel(&ui->log_view, ev->wheel.y,
					 (float)ui->win_mousex,
					 (float)ui->win_mousey))
			ui->dirty = 1;
		return;
	}

	if (ev->type == SDL_MOUSEBUTTONDOWN) {
		float mx = (float)ev->button.x;
		float my = (float)ev->button.y;

		if (ev->button.button == SDL_BUTTON_LEFT) {
			struct sdl_dropdown *open_dd = active_open_dropdown(ui);
			int sel = -1;

			if (open_dd != NULL) {
				sdl_dropdown_handle_click(open_dd, mx, my,
							  ui->win_w, &sel);
				ui->dirty = 1;
				if (sel >= 0) {
					if (open_dd == &ui->mode_dd)
						on_mode_selected(ui, sel);
					else if (open_dd == &ui->opponent_dd)
						on_opponent_selected(ui, sel);
					else if (open_dd == &ui->redblue_dd)
						on_redblue_selected(ui, sel);
				}
				return;
			}

			if (sdl_dropdown_handle_click(&ui->mode_dd, mx, my,
						      ui->win_w, &sel) ||
			    sdl_dropdown_handle_click(&ui->opponent_dd, mx, my,
						      ui->win_w, &sel) ||
			    sdl_dropdown_handle_click(&ui->redblue_dd, mx, my,
						      ui->win_w, &sel)) {
				ui->dirty = 1;
				return;
			}

			if (sdl_button_handle_mouse_down(&ui->auto_setup_btn, mx, my) ||
			    sdl_button_handle_mouse_down(&ui->new_game_btn, mx, my) ||
			    sdl_button_handle_mouse_down(&ui->invert_btn, mx, my) ||
			    sdl_button_handle_mouse_down(&ui->help_btn, mx, my) ||
			    sdl_button_handle_mouse_down(&ui->quit_btn, mx, my) ||
			    sdl_button_handle_mouse_down(&ui->pass_btn, mx, my) ||
			    sdl_button_handle_mouse_down(&ui->play_again_btn, mx, my) ||
			    sdl_log_handle_mouse_down(&ui->log_view, mx, my)) {
				ui->dirty = 1;
				return;
			}
		}

		if (sdl_point_in_rect(mx, my, ui->board_rect)) {
			handle_board_button_down(
				ui, ev->button.button,
				(int)(mx - ui->board_rect.x),
				(int)(my - ui->board_rect.y));
		}
		return;
	}

	if (ev->type == SDL_MOUSEBUTTONUP &&
	    ev->button.button == SDL_BUTTON_LEFT) {
		float mx = (float)ev->button.x;
		float my = (float)ev->button.y;
		int c_auto = 0, c_new = 0, c_inv = 0, c_help = 0;
		int c_quit = 0, c_pass = 0, c_again = 0;

		if (sdl_log_handle_mouse_up(&ui->log_view))
			ui->dirty = 1;

		ui->dirty |= sdl_button_handle_mouse_up(&ui->auto_setup_btn, mx, my, &c_auto);
		ui->dirty |= sdl_button_handle_mouse_up(&ui->new_game_btn, mx, my, &c_new);
		ui->dirty |= sdl_button_handle_mouse_up(&ui->invert_btn, mx, my, &c_inv);
		ui->dirty |= sdl_button_handle_mouse_up(&ui->help_btn, mx, my, &c_help);
		ui->dirty |= sdl_button_handle_mouse_up(&ui->quit_btn, mx, my, &c_quit);
		ui->dirty |= sdl_button_handle_mouse_up(&ui->pass_btn, mx, my, &c_pass);
		ui->dirty |= sdl_button_handle_mouse_up(&ui->play_again_btn, mx, my, &c_again);

		if (c_auto)
			auto_setup_clicked(ui);
		else if (c_new)
			reset_managed_game(ui);
		else if (c_inv)
			invert_clicked(ui);
		else if (c_help)
			help_clicked(ui);
		else if (c_quit)
			quit_clicked(ui);
		else if (c_pass)
			pass_clicked(ui);
		else if (c_again)
			reset_managed_game(ui);
	}
}

static int init_ui(struct gui *ui)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return -1;
	}

	ui->win_w = 1024;
	ui->win_h = 820;
	ui->window = SDL_CreateWindow("Dueling Masters of Space Time",
				      SDL_WINDOWPOS_CENTERED,
				      SDL_WINDOWPOS_CENTERED,
				      ui->win_w, ui->win_h,
				      SDL_WINDOW_RESIZABLE);
	if (ui->window == NULL) {
		fprintf(stderr, "SDL_CreateWindow failed: %s\n",
			SDL_GetError());
		SDL_Quit();
		return -1;
	}
	SDL_SetWindowMinimumSize(ui->window, 640, 520);

	ui->renderer = SDL_CreateRenderer(
		ui->window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ui->renderer == NULL) {
		ui->renderer = SDL_CreateRenderer(ui->window, -1,
						  SDL_RENDERER_SOFTWARE);
	}
	if (ui->renderer == NULL) {
		fprintf(stderr, "SDL_CreateRenderer failed: %s\n",
			SDL_GetError());
		SDL_DestroyWindow(ui->window);
		SDL_Quit();
		return -1;
	}

	if (sdl_font_init(&ui->font, ui->renderer) != 0) {
		fprintf(stderr, "sdl_font_init failed\n");
		SDL_DestroyRenderer(ui->renderer);
		SDL_DestroyWindow(ui->window);
		SDL_Quit();
		return -1;
	}

	ui->galaxy_tex = sdl_load_png_texture(ui->renderer, "pinwheel.png",
					      &ui->galaxy_w, &ui->galaxy_h);
	if (ui->galaxy_tex == NULL) {
		ui->galaxy_tex = sdl_load_png_texture(ui->renderer,
						      "src/pinwheel.png",
						      &ui->galaxy_w,
						      &ui->galaxy_h);
	}

	sdl_dropdown_init(&ui->mode_dd, mode_items, ARRAYSIZE(mode_items),
			  0, NULL);
	sdl_dropdown_init(&ui->opponent_dd, opponent_items,
			  ARRAYSIZE(opponent_items), 0,
			  "Choose Computer AI or Manual offline board");
	sdl_dropdown_init(&ui->redblue_dd, redblue_items,
			  ARRAYSIZE(redblue_items), 0,
			  "Choose to play as Red or Blue");

	sdl_button_init(&ui->auto_setup_btn, "Auto Setup",
			"Randomly arrange your pieces in a sensible formation");
	sdl_button_init(&ui->new_game_btn, "Begin New Game",
			"Reset and begin a new game");
	sdl_button_init(&ui->invert_btn, "Invert Board",
			"Invert Board so that your half is on the bottom");
	sdl_button_init(&ui->help_btn, "Help", NULL);
	sdl_button_init(&ui->quit_btn, "Quit Dueling", "Quit Dueling");
	sdl_button_init(&ui->pass_btn, "Pass Turn", NULL);
	ui->pass_btn.enabled = 0;
	sdl_button_init(&ui->play_again_btn, "Begin New Game", NULL);
	ui->play_again_btn.visible = 0;
	ui->play_again_btn.accent = 1;

	sdl_log_init(&ui->log_view);
	sdl_modal_init(&ui->help_modal);
	sdl_modal_init(&ui->game_over_modal);

	ui->inverted = 1;
	ui->piece_box_open = 2.0;
	ui->holding = NULL;
	ui->playing_as = PLAYING_AS_RED;
	ui->fullscreen = 0;
	ui->mode = MODE_BOARD_SETUP;
	ui->use_opponent = 1;
	ui->opponent = NULL;
	ui->poll_scheduled = 0;
	ui->setup_rng = (uint32_t)time(NULL) ^ 0x5a5a5a5au;
	ui->game_counter = 0;
	memset(ui->visited, 0, sizeof(ui->visited));
	ui->dirty = 1;
	ui->quit = 0;

	layout_ui(ui);
	return 0;
}

static void cleanup_ui(struct gui *ui)
{
	cancel_poll_timer(ui);
	clear_animations(ui);
	dismiss_game_over_dialog(ui);
	if (ui->opponent != NULL) {
		dmost_opponent_destroy(ui->opponent);
		ui->opponent = NULL;
	}
	if (ui->galaxy_tex != NULL) {
		SDL_DestroyTexture(ui->galaxy_tex);
		ui->galaxy_tex = NULL;
	}
	sdl_font_destroy(&ui->font);
	if (ui->renderer != NULL) {
		SDL_DestroyRenderer(ui->renderer);
		ui->renderer = NULL;
	}
	if (ui->window != NULL) {
		SDL_DestroyWindow(ui->window);
		ui->window = NULL;
	}
	SDL_Quit();
}

static void dmost_frame_step(struct gui *ui, int wait_ms)
{
	SDL_Event ev;

	if (wait_ms > 0) {
		if (SDL_WaitEventTimeout(&ev, wait_ms))
			handle_event(ui, &ev);
	}
	while (!ui->quit && SDL_PollEvent(&ev))
		handle_event(ui, &ev);

	if (!ui->quit)
		update_timers(ui, SDL_GetTicks());

	if (!ui->quit && ui->dirty)
		render_ui(ui);
}

static void init_pieces(struct piece **p1_out, struct piece **p2_out)
{
	int i;

	*p1_out = malloc(sizeof(full_set));
	*p2_out = malloc(sizeof(full_set));
	memcpy(*p1_out, full_set, sizeof(full_set));
	memcpy(*p2_out, full_set, sizeof(full_set));

	/* Make player 2's pieces blue */
	for (i = 0; i < (int)ARRAYSIZE(full_set); i++) {
		(*p2_out)[i].r = 0.3;
		(*p2_out)[i].g = 0.3;
		(*p2_out)[i].b = 1.0;
	}
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	init_pieces(&p1, &p2);
	if (init_ui(&ui) != 0) {
		free(p1);
		free(p2);
		return 1;
	}
	reset_managed_game(&ui);
	render_ui(&ui);

	while (!ui.quit) {
		int wait_ms = 250;

		if (ui.anim_count > 0)
			wait_ms = 16;
		else if (ui.poll_scheduled || ui.game_over_scheduled)
			wait_ms = 50;
		dmost_frame_step(&ui, wait_ms);
	}

	cleanup_ui(&ui);
	free(p1);
	free(p2);
	return 0;
}
