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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <math.h>
#include <stdint.h>

#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdkkeysyms.h>

#include "game.h"
#include "opponent.h"
#include "ai.h"

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

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
	GtkWidget *window;
	GtkWidget *help;
	GtkWidget *hbox;
	GtkWidget *drawing_area;
	GtkWidget *mode_combo_box;
	GtkWidget *redblue_combo_box;
	GtkWidget *opponent_combo_box;
	GtkWidget *auto_setup_button;
	GtkWidget *new_game_button;
	GtkWidget *pass_button;
	GtkWidget *status_label;
	GtkWidget *collision_label;
	GtkWidget *log_view;
	GtkTextBuffer *log_buffer;
	GtkTextTag *log_collision_tag;
	int xdim, ydim;
	double piece_box_open;
	int inverted;
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
	int suppress_combo_cb;
	struct player_board board;
	struct dmost_opponent *opponent;
	guint poll_timer_id;
	uint32_t setup_rng;
	uint32_t game_counter;
	struct square recent_collisions[32];
	int recent_collision_count;
	char last_collision_text[320];
};

static GdkColor black;
static struct gui ui;

static void generic_draw_piece_on_board(struct piece *p, cairo_t *cr,
					double radius_scale);
static gboolean opponent_turn_timer_cb(gpointer data);

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
	GtkTextIter end;
	gint start_offset;

	if (ui->log_buffer == NULL)
		return;
	gtk_text_buffer_get_end_iter(ui->log_buffer, &end);
	if (gtk_text_buffer_get_char_count(ui->log_buffer) > 0)
		gtk_text_buffer_insert(ui->log_buffer, &end, "\n", 1);
	gtk_text_buffer_get_end_iter(ui->log_buffer, &end);
	start_offset = gtk_text_iter_get_offset(&end);
	gtk_text_buffer_insert(ui->log_buffer, &end, text, -1);
	if (is_collision && ui->log_collision_tag != NULL) {
		GtkTextIter start;

		gtk_text_buffer_get_iter_at_offset(ui->log_buffer, &start,
						    start_offset);
		gtk_text_buffer_get_end_iter(ui->log_buffer, &end);
		gtk_text_buffer_apply_tag(ui->log_buffer, ui->log_collision_tag,
					  &start, &end);
	}
	if (ui->log_view != NULL) {
		gtk_text_buffer_get_end_iter(ui->log_buffer, &end);
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(ui->log_view),
					     &end, 0.0, FALSE, 0.0, 1.0);
	}
}

static void clear_log(struct gui *ui)
{
	if (ui->log_buffer != NULL)
		gtk_text_buffer_set_text(ui->log_buffer, "", 0);
}

static void clear_recent_collisions(struct gui *ui)
{
	ui->recent_collision_count = 0;
	ui->last_collision_text[0] = '\0';
	if (ui->collision_label != NULL)
		gtk_label_set_text(GTK_LABEL(ui->collision_label), "");
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
}

static void on_board_event(void *ctx, const struct dmost_event *ev)
{
	struct gui *ui = ctx;
	char buf[160];

	dmost_format_event(ev, ui->board.side, buf, sizeof(buf));
	if (ev->type == EVENT_COLLISION) {
		record_recent_collision(ui, ev->x, ev->y, buf);
		append_log(ui, buf, 1);
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
	char msg[160];

	if (ui->status_label == NULL)
		return;

	if (!ui->use_opponent) {
		if (ui->mode == MODE_FREE)
			snprintf(msg, sizeof(msg),
				 "Manual Free-form mode (no rules enforced)");
		else if (ui->mode == MODE_BOARD_SETUP)
			snprintf(msg, sizeof(msg),
				 "Manual Setup (%s): arrange your pieces",
				 ui->playing_as == PLAYING_AS_RED ? "Red" : "Blue");
		else
			snprintf(msg, sizeof(msg),
				 "Manual Play (%s): move pieces & communicate externally",
				 ui->playing_as == PLAYING_AS_RED ? "Red" : "Blue");
		gtk_label_set_text(GTK_LABEL(ui->status_label), msg);
		gtk_widget_set_sensitive(ui->auto_setup_button, FALSE);
		gtk_widget_set_sensitive(ui->pass_button, FALSE);
		return;
	}

	if (ui->mode == MODE_FREE) {
		gtk_label_set_text(GTK_LABEL(ui->status_label),
				   "Free-form mode (vs Computer paused)");
		gtk_widget_set_sensitive(ui->auto_setup_button, FALSE);
		gtk_widget_set_sensitive(ui->pass_button, FALSE);
		return;
	}

	if (ui->board.phase == PHASE_SETUP) {
		int placed = count_placed_own_pieces(&ui->board);
		snprintf(msg, sizeof(msg),
			 "Setup (%s): place all %d pieces (%d/%d placed) or click Auto Setup, then select Play Game",
			 ui->board.side == SIDE_RED ? "Red" : "Blue",
			 NUM_PIECES, placed, NUM_PIECES);
		gtk_widget_set_sensitive(ui->auto_setup_button, TRUE);
		gtk_widget_set_sensitive(ui->pass_button, FALSE);
	} else if (ui->board.phase == PHASE_GAME_OVER) {
		switch (ui->board.result) {
		case RESULT_RED_WINS:
			snprintf(msg, sizeof(msg), "Game Over: Red wins! (%s)",
				 ui->board.side == SIDE_RED ? "You win!" : "Computer wins.");
			break;
		case RESULT_BLUE_WINS:
			snprintf(msg, sizeof(msg), "Game Over: Blue wins! (%s)",
				 ui->board.side == SIDE_BLUE ? "You win!" : "Computer wins.");
			break;
		case RESULT_DRAW:
		default:
			snprintf(msg, sizeof(msg), "Game Over: Draw!");
			break;
		}
		gtk_widget_set_sensitive(ui->auto_setup_button, FALSE);
		gtk_widget_set_sensitive(ui->pass_button, FALSE);
	} else if (ui->board.phase == PHASE_RESOLVING) {
		snprintf(msg, sizeof(msg), "Resolving collisions...");
		gtk_widget_set_sensitive(ui->auto_setup_button, FALSE);
		gtk_widget_set_sensitive(ui->pass_button, FALSE);
	} else if (ui->board.current_turn == ui->board.side) {
		int can_move = dmost_has_any_legal_move(&ui->board);

		if (can_move) {
			snprintf(msg, sizeof(msg),
				 "Turn %d: Your turn (%s) - click a piece to move",
				 ui->board.turn_number,
				 ui->board.side == SIDE_RED ? "Red" : "Blue");
		} else {
			snprintf(msg, sizeof(msg),
				 "Turn %d: No legal moves available - click Pass Turn",
				 ui->board.turn_number);
		}
		gtk_widget_set_sensitive(ui->auto_setup_button, FALSE);
		gtk_widget_set_sensitive(ui->pass_button, !can_move);
	} else {
		snprintf(msg, sizeof(msg),
			 "Turn %d: Computer's turn (%s)...",
			 ui->board.turn_number,
			 ui->board.current_turn == SIDE_RED ? "Red" : "Blue");
		gtk_widget_set_sensitive(ui->auto_setup_button, FALSE);
		gtk_widget_set_sensitive(ui->pass_button, FALSE);
	}

	gtk_label_set_text(GTK_LABEL(ui->status_label), msg);
	if (ui->collision_label != NULL) {
		if (ui->last_collision_text[0] != '\0') {
			gchar *markup = g_markup_printf_escaped(
				"<span foreground=\"#d94e1f\" weight=\"bold\">%s</span>",
				ui->last_collision_text);
			gtk_label_set_markup(GTK_LABEL(ui->collision_label),
					     markup);
			g_free(markup);
		} else {
			gtk_label_set_text(GTK_LABEL(ui->collision_label), "");
		}
	}
}

static void cancel_poll_timer(struct gui *ui)
{
	if (ui->poll_timer_id != 0) {
		g_source_remove(ui->poll_timer_id);
		ui->poll_timer_id = 0;
	}
}

static void schedule_opponent_poll(struct gui *ui, guint delay_ms)
{
	if (ui->poll_timer_id == 0 && ui->opponent != NULL &&
	    ui->board.phase != PHASE_GAME_OVER)
		ui->poll_timer_id = gdk_threads_add_timeout(
			delay_ms, opponent_turn_timer_cb, ui);
}

static void set_mode_combo_silent(struct gui *ui, int active_idx)
{
	ui->suppress_combo_cb = 1;
	gtk_combo_box_set_active(GTK_COMBO_BOX(ui->mode_combo_box), active_idx);
	ui->suppress_combo_cb = 0;
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
	if (ui->opponent != NULL) {
		dmost_opponent_destroy(ui->opponent);
		ui->opponent = NULL;
	}

	reset_piece_arrays(ui);
	dmost_board_init(&ui->board, side, SIDE_RED);
	dmost_board_set_event_cb(&ui->board, on_board_event, ui);
	ui->mode = MODE_BOARD_SETUP;
	ui->piece_box_open = 2.0;
	set_mode_combo_silent(ui, 0);
	clear_log(ui);
	clear_recent_collisions(ui);
	if (ui->use_opponent)
		append_log(ui, "New game vs Computer. Place your pieces or click Auto Setup.", 0);
	else
		append_log(ui, "Manual offline mode.", 0);
	sync_ui_from_board(ui);
	update_status(ui);
	if (ui->drawing_area != NULL)
		gtk_widget_queue_draw(ui->drawing_area);
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

static gboolean opponent_turn_timer_cb(gpointer data)
{
	struct gui *ui = data;
	struct dmost_msg msg, reply;
	int has_reply = 0;
	int rc;

	if (!ui->use_opponent || ui->opponent == NULL ||
	    ui->board.phase == PHASE_GAME_OVER ||
	    ui->board.phase == PHASE_SETUP) {
		ui->poll_timer_id = 0;
		return FALSE;
	}

	if (ui->board.phase == PHASE_PLAYING &&
	    ui->board.current_turn == ui->board.side) {
		ui->poll_timer_id = 0;
		return FALSE;
	}

	rc = dmost_opponent_poll(ui->opponent, &msg);
	if (rc == 1) {
		if (dmost_handle_message(&ui->board, &msg, &reply, &has_reply)) {
			if (has_reply)
				dmost_opponent_send(ui->opponent, &reply);
			pump_resolution_messages(ui);
		}
		sync_ui_from_board(ui);
		update_status(ui);
		gtk_widget_queue_draw(ui->drawing_area);
	}

	if (ui->board.phase == PHASE_GAME_OVER ||
	    (ui->board.phase == PHASE_PLAYING &&
	     ui->board.current_turn == ui->board.side)) {
		ui->poll_timer_id = 0;
		return FALSE;
	}

	return TRUE;
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
		set_mode_combo_silent(ui, 0);
		update_status(ui);
		gtk_widget_queue_draw(ui->drawing_area);
		return;
	}

	if (ui->opponent != NULL) {
		sync_ui_from_board(ui);
		ui->mode = MODE_PLAYING;
		update_status(ui);
		gtk_widget_queue_draw(ui->drawing_area);
		return;
	}

	opp_side = dmost_other_side(ui->board.side);
	seed = (uint32_t)time(NULL) ^ (++ui->game_counter * 2654435761u);
	ui->opponent = dmost_ai_opponent_create(opp_side, SIDE_RED, seed);
	if (ui->opponent == NULL) {
		append_log(ui, "Failed to initialize computer opponent.", 0);
		ui->mode = MODE_BOARD_SETUP;
		set_mode_combo_silent(ui, 0);
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
	gtk_widget_queue_draw(ui->drawing_area);

	if (ui->board.phase == PHASE_PLAYING &&
	    ui->board.current_turn != ui->board.side)
		schedule_opponent_poll(ui, 300);
}

static void invert_clicked(__attribute__((unused)) GtkWidget *widget,
			   gpointer data)
{
	struct gui *ui = data;

	ui->inverted = !ui->inverted;
	gtk_widget_queue_draw(ui->drawing_area);
}

static void help_clicked(__attribute__((unused)) GtkWidget *widget,
			 gpointer data)
{
	struct gui *ui = data;

	gtk_widget_show_all(ui->help);
	gtk_window_present(GTK_WINDOW(ui->help));
}

static gboolean close_help_clicked(GtkWidget *widget,
				   __attribute__((unused)) GdkEvent *event,
				   __attribute__((unused)) gpointer data)
{
	gtk_widget_hide(widget);
	return TRUE;
}

static void quit_clicked(__attribute__((unused)) GtkWidget *widget,
			 gpointer data)
{
	struct gui *ui = data;

	if (ui != NULL) {
		cancel_poll_timer(ui);
		if (ui->opponent != NULL) {
			dmost_opponent_destroy(ui->opponent);
			ui->opponent = NULL;
		}
	}
	gtk_main_quit();
}

static void auto_setup_clicked(__attribute__((unused)) GtkWidget *widget,
			       gpointer data)
{
	struct gui *ui = data;

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
	gtk_widget_queue_draw(ui->drawing_area);
}

static void new_game_clicked(__attribute__((unused)) GtkWidget *widget,
			     gpointer data)
{
	struct gui *ui = data;

	reset_managed_game(ui);
}

static void pass_clicked(__attribute__((unused)) GtkWidget *widget,
			 gpointer data)
{
	struct gui *ui = data;
	struct dmost_msg msg;

	if (!is_managed_mode(ui) || ui->opponent == NULL)
		return;
	if (!dmost_has_any_legal_move(&ui->board))
		clear_recent_collisions(ui);
	if (!dmost_initiate_pass(&ui->board, &msg))
		return;

	dmost_opponent_send(ui->opponent, &msg);
	pump_resolution_messages(ui);
	sync_ui_from_board(ui);
	update_status(ui);
	gtk_widget_queue_draw(ui->drawing_area);

	if (ui->board.phase == PHASE_PLAYING &&
	    ui->board.current_turn != ui->board.side)
		schedule_opponent_poll(ui, ui->recent_collision_count > 0 ? 900 : 300);
}

static gint key_press_cb(__attribute__((unused)) GtkWidget *widget,
			 GdkEventKey *event, gpointer data)
{
	struct gui *ui = data;

	switch (event->keyval) {
	case GDK_F11:
		if (ui->fullscreen) {
			ui->fullscreen = 0;
			gtk_window_unfullscreen(GTK_WINDOW(ui->window));
		} else {
			ui->fullscreen = 1;
			gtk_window_fullscreen(GTK_WINDOW(ui->window));
		}
		break;
	case GDK_Escape:
		if (ui->holding != NULL) {
			ui->holding = NULL;
			gtk_widget_queue_draw(ui->drawing_area);
		}
		break;
	default:
		break;
	}
	return TRUE;
}

static void draw_aligned_text(cairo_t *cr, double x, double y,
			      double fontsize, const char *text, int alignment)
{
#define CENTERED 0
#define LEFT_JUSTIFIED 1
#define RIGHT_JUSTIFIED 2

	double factor, direction;
	cairo_text_extents_t extents;

	switch (alignment) {
	case CENTERED:
		direction = -1.0;
		factor = 0.5;
		break;
	case RIGHT_JUSTIFIED:
		direction = -1.0;
		factor = 1.0;
		break;
	case LEFT_JUSTIFIED:
	default:
		direction = 1.0;
		factor = 1.0;
		break;
	}
	cairo_select_font_face(cr, "SERIF", CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_NORMAL);

	cairo_set_font_size(cr, fontsize);
	cairo_text_extents(cr, text, &extents);
	x = x + direction * (factor * extents.width + extents.x_bearing);
	y = y - (extents.height / 2 + extents.y_bearing);

	cairo_move_to(cr, x, y);
	cairo_show_text(cr, text);
}

static inline void draw_centered_text(cairo_t *cr, double x, double y,
				      double fontsize, const char *text)
{
	draw_aligned_text(cr, x, y, fontsize, text, CENTERED);
}

static int invert(int y)
{
	if (!ui.inverted)
		return y;
	return 9 - y;
}

static void count_square_stack(const struct piece *target, struct piece *list,
			       int *stack_total, int *stack_idx, int before)
{
	int j;

	for (j = 0; list[j].name != NULL; j++) {
		if (list[j].x == UNKNOWN || !list[j].alive ||
		    &list[j] == ui.holding)
			continue;
		if (list[j].x == target->x && list[j].y == target->y) {
			if (before || &list[j] < target)
				(*stack_idx)++;
			(*stack_total)++;
		}
	}
}

static void draw_pieces_on_board(cairo_t *cr, struct piece *p)
{
	int i;
	double cell_w = ui.xdim / (12.0 * ui.piece_box_open);
	double cell_h = ui.ydim / 12.0;

	for (i = 0; p[i].name != NULL; i++) {
		int stack_total = 0, stack_idx = 0;
		double ox = 0.0, oy = 0.0;
		double scale = 1.0;

		if (p[i].x == UNKNOWN || !p[i].alive)
			continue;
		if (&p[i] == ui.holding)
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
		generic_draw_piece_on_board(&p[i], cr, scale);
	}
}

void generic_draw_piece_off_board(struct piece *p, cairo_t *cr, double radius)
{
	double alpha_fill = p->alive ? 0.25 : 0.08;
	double alpha_stroke = p->alive ? 1.0 : 0.35;

	cairo_save(cr);
	cairo_arc(cr, p->sx, p->sy, radius, 0.0, 2.0 * M_PI);
	cairo_set_source_rgba(cr, p->r, p->g, p->b, alpha_fill);
	cairo_fill_preserve(cr);
	cairo_stroke(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_set_line_width(cr, MIN(ui.xdim, ui.ydim) / 200.0);
	cairo_set_source_rgba(cr, p->r, p->g, p->b, alpha_stroke);
	cairo_arc(cr, p->sx, p->sy, radius * 1.1, 0.0, 2.0 * M_PI);
	cairo_stroke(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, p->alive ? 1.0 : 0.45);
	if (radius > 12.0 && radius < 23.0)
		draw_centered_text(cr, p->sx, p->sy,
				   MIN(ui.xdim, ui.ydim) / 32.0, p->name);
	else if (radius >= 23.0)
		draw_centered_text(cr, p->sx, p->sy,
				   MIN(ui.xdim, ui.ydim) / 20.0, p->name);
	if (!p->alive) {
		double d = radius * 0.75;

		cairo_set_source_rgba(cr, 0.9, 0.2, 0.2, 0.75);
		cairo_set_line_width(cr, 2.0);
		cairo_move_to(cr, p->sx - d, p->sy - d);
		cairo_line_to(cr, p->sx + d, p->sy + d);
		cairo_move_to(cr, p->sx + d, p->sy - d);
		cairo_line_to(cr, p->sx - d, p->sy + d);
		cairo_stroke(cr);
	}
	cairo_restore(cr);
}

static void draw_dead_piece_on_board(const struct piece *p, cairo_t *cr,
				     double cx, double cy, double radius_scale)
{
	double radius;
	double d;

	if (ui.xdim / ui.piece_box_open < ui.ydim)
		radius = (0.75 * ui.xdim / (12.0 * ui.piece_box_open)) / 2.0;
	else
		radius = (0.75 * ui.ydim / 12.0) / 2.0;
	radius *= radius_scale;
	d = radius * 0.72;

	cairo_save(cr);
	cairo_arc(cr, cx, cy, radius, 0.0, 2.0 * M_PI);
	cairo_set_source_rgba(cr, p->r, p->g, p->b, 0.14);
	cairo_fill_preserve(cr);
	cairo_stroke(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_set_line_width(cr, MIN(ui.xdim, ui.ydim) / 220.0);
	cairo_set_source_rgba(cr, p->r, p->g, p->b, 0.55);
	cairo_arc(cr, cx, cy, radius * 1.05, 0.0, 2.0 * M_PI);
	cairo_stroke(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.80);
	if (radius > 8.0)
		draw_centered_text(cr, cx, cy,
				   MIN(ui.xdim, ui.ydim) / 32.0 * radius_scale,
				   p->name);
	cairo_set_source_rgba(cr, 1.0, 0.35, 0.2, 0.85);
	cairo_set_line_width(cr, 2.0);
	cairo_move_to(cr, cx - d, cy - d);
	cairo_line_to(cr, cx + d, cy + d);
	cairo_move_to(cr, cx + d, cy - d);
	cairo_line_to(cr, cx - d, cy + d);
	cairo_stroke(cr);
	cairo_restore(cr);
}

static int has_live_piece_at(int x, int y)
{
	int j;

	for (j = 0; p1[j].name != NULL; j++) {
		if (p1[j].alive && p1[j].x == x && p1[j].y == y &&
		    &p1[j] != ui.holding)
			return 1;
	}
	for (j = 0; p2[j].name != NULL; j++) {
		if (p2[j].alive && p2[j].x == x && p2[j].y == y &&
		    &p2[j] != ui.holding)
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
		if (list[j].alive || list[j].x == UNKNOWN)
			continue;
		if (list[j].x == target->x && list[j].y == target->y) {
			if (before || &list[j] < target)
				(*idx)++;
			(*total)++;
		}
	}
}

static void draw_dead_pieces_on_board_list(cairo_t *cr, struct piece *p)
{
	int i;
	double cell_w = ui.xdim / (12.0 * ui.piece_box_open);
	double cell_h = ui.ydim / 12.0;

	for (i = 0; p[i].name != NULL; i++) {
		int dead_total = 0, dead_idx = 0, live_here;
		double cx, cy, scale;

		if (p[i].alive || p[i].x == UNKNOWN)
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

		draw_dead_piece_on_board(&p[i], cr, cx, cy, scale);
	}
}

static void draw_dead_pieces_on_board(cairo_t *cr)
{
	if (!is_managed_mode(&ui))
		return;
	draw_dead_pieces_on_board_list(cr, p1);
	draw_dead_pieces_on_board_list(cr, p2);
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

static void draw_pieces_off_board(cairo_t *cr, struct piece *p,
				  int *initial_count)
{
	int i, count;
	double leftx, xwidth, slotwidth;
	double radius;

	leftx = 11.0 * ui.xdim / (12.0 * ui.piece_box_open);
	xwidth = ui.xdim - leftx;
	slotwidth = MIN(xwidth / 6.0, ui.xdim / 12.0);
	radius = slotwidth / 4.0;

	count = *initial_count;
	for (i = 0; p[i].name != NULL; i++) {
		if (!should_show_in_tray(&ui, &p[i])) {
			if (p[i].x == UNKNOWN && &p[i] != ui.holding) {
				p[i].sx = -1000;
				p[i].sy = -1000;
			}
			continue;
		}
		p[i].sx = leftx + (count % 4) * slotwidth + slotwidth / 2.0;
		p[i].sy = (ui.ydim / 12.0) + (count / 4) * slotwidth / 1.5;
		generic_draw_piece_off_board(&p[i], cr, radius);
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

static void highlight_square(cairo_t *cr, struct gui *ui, int x, int y,
			     double r, double g, double b, double a)
{
	double x1, y1, x2, y2;

	x1 = (x + 1) * ui->xdim / (12.0 * ui->piece_box_open);
	x2 = (x + 2) * ui->xdim / (12.0 * ui->piece_box_open);
	y1 = (invert(y) + 1) * (ui->ydim / 12.0);
	y2 = (invert(y) + 2) * (ui->ydim / 12.0);

	cairo_save(cr);
	cairo_set_source_rgba(cr, r, g, b, a);
	cairo_move_to(cr, x1, y1);
	cairo_line_to(cr, x2, y1);
	cairo_line_to(cr, x2, y2);
	cairo_line_to(cr, x1, y2);
	cairo_close_path(cr);
	cairo_fill_preserve(cr);
	cairo_stroke(cr);
	cairo_restore(cr);
}

static void highlight_legal_square(cairo_t *cr, struct gui *ui, int x, int y)
{
	highlight_square(cr, ui, x, y, 0.0, 1.0, 0.0, 0.14);
}

static void highlight_legal_moves(cairo_t *cr, struct gui *ui)
{
	int x, y;

	if (ui->holding == NULL || ui->mode == MODE_FREE)
		return;

	if (ui->holding->pickedx == -1 && ui->mode != MODE_BOARD_SETUP)
		return;

	for (x = 0; x < 10; x++)
		for (y = 0; y < 10; y++)
			if (legal_move(ui, ui->holding, x, y))
				highlight_legal_square(cr, ui, x, y);
}

static void highlight_visited_squares(cairo_t *cr, struct gui *ui)
{
	int x, y;
	double r = 0.0, b = 0.0;
	enum player_side enemy_side = (ui->playing_as == PLAYING_AS_RED) ?
				      SIDE_BLUE : SIDE_RED;

	if (ui->playing_as == PLAYING_AS_RED)
		r = 1.0;
	if (ui->playing_as == PLAYING_AS_BLUE)
		b = 1.0;

	cairo_save(cr);
	if (is_managed_mode(ui)) {
		for (x = 0; x < BOARD_SIZE; x++) {
			for (y = 0; y < BOARD_SIZE; y++) {
				if (dmost_side_owns_row(enemy_side, y) &&
				    ui->board.enemy_clear[x][y])
					highlight_square(cr, ui, x, y,
							 0.1, 0.85, 0.55, 0.14);
			}
		}
	}
	for (x = 0; x < 10; x++)
		for (y = 0; y < 10; y++)
			if (ui->visited[x][y])
				highlight_square(cr, ui, x, y, r, 0.0, b, 0.2);
	cairo_restore(cr);
}

static void highlight_recent_collisions(cairo_t *cr, struct gui *ui)
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

		highlight_square(cr, ui, x, y, 1.0, 0.55, 0.0, 0.26);
		cairo_save(cr);
		cairo_set_source_rgba(cr, 1.0, 0.72, 0.15, 0.92);
		cairo_set_line_width(cr, 2.5);
		cairo_rectangle(cr, x1 + 1.5, y1 + 1.5,
				cell_w - 3.0, cell_h - 3.0);
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}

static gboolean on_expose_drawing_area(GtkWidget *w,
				       __attribute__((unused)) GdkEventExpose *event,
				       gpointer p)
{
	struct gui *ui = p;
	cairo_t *cr;
	static cairo_surface_t *galaxy_image = NULL;
	static int xd = 0, yd = 0;
	static double dash[] = { 1.0, 2.0 };
	int i, off_board_piece_count = 0;

	if (ui->holding != NULL) {
		ui->holding->sx = ui->mousex;
		ui->holding->sy = ui->mousey;
	}
	if (galaxy_image == NULL) {
		galaxy_image = cairo_image_surface_create_from_png("pinwheel.png");
		if (cairo_surface_status(galaxy_image) != CAIRO_STATUS_SUCCESS) {
			cairo_surface_destroy(galaxy_image);
			galaxy_image = cairo_image_surface_create_from_png("src/pinwheel.png");
		}
		if (cairo_surface_status(galaxy_image) == CAIRO_STATUS_SUCCESS) {
			xd = cairo_image_surface_get_width(galaxy_image);
			yd = cairo_image_surface_get_height(galaxy_image);
		}
	}

	cr = gdk_cairo_create(w->window);

	if (galaxy_image != NULL &&
	    cairo_surface_status(galaxy_image) == CAIRO_STATUS_SUCCESS &&
	    xd > 0 && yd > 0) {
		cairo_save(cr);
		if (ui->inverted) {
			cairo_scale(cr, 1.0, -1.0);
			cairo_translate(cr, 0.0, -ui->ydim);
		}
		cairo_scale(cr,
			    (double)ui->xdim / (double)xd / ui->piece_box_open,
			    (double)ui->ydim / (double)yd);
		cairo_set_source_surface(cr, galaxy_image, 0, 0);
		cairo_paint_with_alpha(cr, 0.5);
		cairo_restore(cr);
	}

	cairo_save(cr);
	cairo_set_dash(cr, dash, 2, 2.0);
	cairo_set_source_rgb(cr, 0.5, 1, 0.5);
	for (i = 1; i <= 11; i++) {
		cairo_move_to(cr, ui->xdim / (12.0 * ui->piece_box_open) * i,
			      ui->ydim / 12.0);
		cairo_line_to(cr, ui->xdim / (12.0 * ui->piece_box_open) * i,
			      11.0 * ui->ydim / 12.0);
		cairo_move_to(cr, ui->xdim / (12.0 * ui->piece_box_open),
			      i * ui->ydim / 12.0);
		cairo_line_to(cr, 11.0 * ui->xdim / (12.0 * ui->piece_box_open),
			      i * ui->ydim / 12.0);
	}

	switch (ui->playing_as) {
	case PLAYING_AS_RED:
		cairo_set_source_rgb(cr, 1.0, 0.25, 0.25);
		break;
	case PLAYING_AS_BLUE:
		cairo_set_source_rgb(cr, 0.3, 0.5, 1.0);
		break;
	}

	for (i = 0; i < 10; i++) {
		char letter[4];
		double y;

		snprintf(letter, sizeof(letter), "%c", 'A' + i);
		y = (i + 1.5) * ui->ydim / 12.0;
		if (ui->inverted)
			y = ui->ydim - y;
		draw_centered_text(cr, ui->xdim / (24.0 * ui->piece_box_open),
				   y, ui->ydim / 30.0, letter);
	}
	for (i = 0; i < 10; i++) {
		char number[4];

		snprintf(number, sizeof(number), "%d", i + 1);
		draw_centered_text(cr,
				   (i + 1.5) * ui->xdim /
				   (12.0 * ui->piece_box_open),
				   ui->ydim / 24.0,
				   (ui->xdim / ui->piece_box_open) / 30.0,
				   number);
	}
	cairo_stroke(cr);
	cairo_restore(cr);

	highlight_visited_squares(cr, ui);
	highlight_recent_collisions(cr, ui);
	highlight_legal_moves(cr, ui);

	draw_dead_pieces_on_board(cr);
	draw_pieces_on_board(cr, p1);
	draw_pieces_on_board(cr, p2);
	draw_pieces_off_board(cr, p1, &off_board_piece_count);
	draw_pieces_off_board(cr, p2, &off_board_piece_count);
	if (ui->holding != NULL)
		generic_draw_piece_off_board(ui->holding, cr, ui->xdim / 24.0);

	cairo_destroy(cr);
	return TRUE;
}

static gint on_config_drawing_area(GtkWidget *w,
				   __attribute__((unused)) GdkEventConfigure *event,
				   gpointer p)
{
	struct gui *ui = p;

	ui->xdim = w->allocation.width;
	ui->ydim = w->allocation.height;
	return FALSE;
}

static void hide_or_show_piecebox(struct gui *ui, GtkWidget *w)
{
	if (ui->mode == MODE_BOARD_SETUP) {
		if (ui->piece_box_open != 2.0) {
			ui->piece_box_open = 2.0;
			gtk_widget_queue_draw(w);
		}
		return;
	}
	if (ui->piece_box_open > 1.0 && ui->mousex < ui->xdim / 2.2) {
		gtk_widget_queue_draw(w);
		ui->piece_box_open *= 0.8;
		if (ui->piece_box_open < 1.0)
			ui->piece_box_open = 1.0;
	} else if (ui->mousex > 11.0 * (ui->xdim / (12.0 * ui->piece_box_open)) &&
		   ui->piece_box_open < 2.0) {
		ui->piece_box_open *= 1.2;
		gtk_widget_queue_draw(w);
		if (ui->piece_box_open > 2.0)
			ui->piece_box_open = 2.0;
	}
}

static gboolean on_mouse_motion(GtkWidget *w, GdkEvent *event, gpointer p)
{
	struct gui *ui = p;
	GdkEventMotion *e = (GdkEventMotion *)event;

	if (!e)
		return TRUE;

	ui->mousex = (int)e->x;
	ui->mousey = (int)e->y;

	hide_or_show_piecebox(ui, w);
	if (ui->holding != NULL)
		gtk_widget_queue_draw(w);

	return TRUE;
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

static gboolean on_button_clicked(GtkWidget *w, GdkEvent *event, gpointer ptr)
{
	struct gui *ui = ptr;
	int tmpx, tmpy;

	if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
		ui->mousex = (int)event->button.x;
		ui->mousey = (int)event->button.y;

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
					gtk_widget_queue_draw(w);
					return TRUE;
				}
				/* Off the board in manual/free mode */
				ui->holding->x = UNKNOWN;
				ui->holding->sx = -100;
				ui->holding->sy = -100;
				ui->holding->pickedx = -1;
				ui->holding->pickedy = -1;
				ui->holding = NULL;
				gtk_widget_queue_draw(w);
				return TRUE;
			}

			if (screen_to_board(ui, ui->mousex, ui->mousey,
					    &tmpx, &tmpy)) {
				if (!legal_move(ui, ui->holding, tmpx, tmpy))
					return TRUE;

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
							if (dmost_initiate_move(&ui->board,
										ui->holding->id,
										tmpx, tmpy,
										&msg)) {
								ui->holding = NULL;
								if (ui->opponent != NULL)
									dmost_opponent_send(ui->opponent, &msg);
								pump_resolution_messages(ui);
								sync_ui_from_board(ui);
								update_status(ui);
								gtk_widget_queue_draw(w);
								if (ui->board.phase == PHASE_RESOLVING ||
								    (ui->board.phase == PHASE_PLAYING &&
								     ui->board.current_turn != ui->board.side))
									schedule_opponent_poll(ui, ui->recent_collision_count > 0 ? 900 : 300);
								return TRUE;
							}
						}
					}
					ui->holding = NULL;
					gtk_widget_queue_draw(w);
					return TRUE;
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
				gtk_widget_queue_draw(w);
			}
			return TRUE;
		}

		/* Picking up a piece */
		if (is_managed_mode(ui) && ui->mode == MODE_PLAYING) {
			if (ui->board.phase != PHASE_PLAYING ||
			    ui->board.current_turn != ui->board.side)
				return TRUE;
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
					return TRUE;
				ui->holding = found;
				gtk_widget_queue_draw(ui->drawing_area);
			}
		}
		return TRUE;
	}

	if (event->type == GDK_BUTTON_PRESS && event->button.button == 3) {
		ui->mousex = (int)event->button.x;
		ui->mousey = (int)event->button.y;

		if (ui->holding != NULL) {
			ui->holding = NULL;
			gtk_widget_queue_draw(ui->drawing_area);
			return TRUE;
		}
		if (screen_to_board(ui, ui->mousex, ui->mousey, &tmpx, &tmpy)) {
			ui->visited[tmpx][tmpy] = !ui->visited[tmpx][tmpy];
			gtk_widget_queue_draw(ui->drawing_area);
		}
		return TRUE;
	}
	return FALSE;
}

static void mode_cb(__attribute__((unused)) GtkComboBox *combo, gpointer data)
{
	struct gui *ui = data;
	gchar *ctext;

	if (ui->suppress_combo_cb)
		return;

	ctext = gtk_combo_box_get_active_text(GTK_COMBO_BOX(ui->mode_combo_box));
	if (ctext == NULL)
		return;

	ui->holding = NULL;

	if (strcmp(ctext, "Free form") == 0) {
		ui->mode = MODE_FREE;
		update_status(ui);
		gtk_widget_queue_draw(ui->drawing_area);
		g_free(ctext);
		return;
	}

	if (strcmp(ctext, "Board Setup") == 0) {
		if (ui->use_opponent && ui->opponent != NULL) {
			/* Returning to setup after starting resets the match */
			g_free(ctext);
			reset_managed_game(ui);
			return;
		}
		ui->mode = MODE_BOARD_SETUP;
		ui->piece_box_open = 2.0;
		update_status(ui);
		gtk_widget_queue_draw(ui->drawing_area);
		g_free(ctext);
		return;
	}

	if (strcmp(ctext, "Play Game") == 0) {
		if (ui->use_opponent) {
			g_free(ctext);
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
	}
	g_free(ctext);
	gtk_widget_queue_draw(ui->drawing_area);
}

static void redblue_cb(__attribute__((unused)) GtkComboBox *combo,
		       gpointer data)
{
	struct gui *ui = data;
	gchar *ctext;

	if (ui->suppress_combo_cb)
		return;

	ctext = gtk_combo_box_get_active_text(GTK_COMBO_BOX(ui->redblue_combo_box));
	if (ctext == NULL)
		return;

	if (strcmp(ctext, "Play as Red") == 0) {
		ui->playing_as = PLAYING_AS_RED;
		ui->inverted = 1;
	} else if (strcmp(ctext, "Play as Blue") == 0) {
		ui->playing_as = PLAYING_AS_BLUE;
		ui->inverted = 0;
	}
	g_free(ctext);

	if (ui->use_opponent) {
		reset_managed_game(ui);
	} else {
		update_status(ui);
		gtk_widget_queue_draw(ui->drawing_area);
	}
}

static void opponent_cb(__attribute__((unused)) GtkComboBox *combo,
			gpointer data)
{
	struct gui *ui = data;
	gchar *ctext;

	if (ui->suppress_combo_cb)
		return;

	ctext = gtk_combo_box_get_active_text(GTK_COMBO_BOX(ui->opponent_combo_box));
	if (ctext == NULL)
		return;

	if (strcmp(ctext, "vs Computer") == 0)
		ui->use_opponent = 1;
	else
		ui->use_opponent = 0;
	g_free(ctext);

	reset_managed_game(ui);
}

static void setup_help_window(struct gui *ui)
{
	GtkWidget *vbox, *view, *scrolled_window;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	static const char *instructions =
"   Instructions — Dueling Masters of Space Time\n"
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

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	ui->help = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(ui->help), "DMOST Help");
	gtk_window_set_default_size(GTK_WINDOW(ui->help), 640, 480);
	g_signal_connect(ui->help, "delete-event",
			 G_CALLBACK(close_help_clicked), ui);
	vbox = gtk_vbox_new(FALSE, 0);
	view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
	gtk_text_buffer_insert(buffer, &iter, instructions, -1);
	gtk_container_add(GTK_CONTAINER(scrolled_window), view);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(ui->help), vbox);
}

static void init_ui(int *argc, char **argv[], struct gui *ui)
{
	GtkWidget *vbox, *hbox, *bottom_hbox, *log_scroll;
	GtkWidget *invert_button, *quit_button, *help_button;

#if !GLIB_CHECK_VERSION(2, 32, 0)
	if (!g_thread_supported())
		g_thread_init(NULL);
#endif
	gdk_threads_init();

	gtk_init(argc, argv);

	gdk_color_parse("black", &black);

	ui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(ui->window),
			     "Dueling Masters of Space Time");
	gtk_window_set_default_size(GTK_WINDOW(ui->window), 1024, 820);

	g_signal_connect(ui->window, "delete-event",
			 G_CALLBACK(quit_clicked), ui);
	g_signal_connect(ui->window, "destroy",
			 G_CALLBACK(quit_clicked), ui);
	g_signal_connect(G_OBJECT(ui->window), "key_press_event",
			 G_CALLBACK(key_press_cb), ui);

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(ui->window), vbox);

	hbox = gtk_hbox_new(FALSE, 4);
	ui->hbox = hbox;

	ui->mode_combo_box = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(ui->mode_combo_box), "Board Setup");
	gtk_combo_box_append_text(GTK_COMBO_BOX(ui->mode_combo_box), "Play Game");
	gtk_combo_box_append_text(GTK_COMBO_BOX(ui->mode_combo_box), "Free form");
	gtk_combo_box_set_active(GTK_COMBO_BOX(ui->mode_combo_box), 0);
	gtk_box_pack_start(GTK_BOX(hbox), ui->mode_combo_box, FALSE, FALSE, 2);
	g_signal_connect(G_OBJECT(ui->mode_combo_box), "changed",
			 G_CALLBACK(mode_cb), ui);

	ui->opponent_combo_box = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(ui->opponent_combo_box),
				  "vs Computer");
	gtk_combo_box_append_text(GTK_COMBO_BOX(ui->opponent_combo_box),
				  "Manual (Offline)");
	gtk_combo_box_set_active(GTK_COMBO_BOX(ui->opponent_combo_box), 0);
	gtk_box_pack_start(GTK_BOX(hbox), ui->opponent_combo_box, FALSE, FALSE, 2);
	gtk_widget_set_tooltip_text(ui->opponent_combo_box,
				    "Choose Computer AI or Manual offline board");
	g_signal_connect(G_OBJECT(ui->opponent_combo_box), "changed",
			 G_CALLBACK(opponent_cb), ui);

	ui->redblue_combo_box = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(ui->redblue_combo_box), "Play as Red");
	gtk_combo_box_append_text(GTK_COMBO_BOX(ui->redblue_combo_box), "Play as Blue");
	gtk_combo_box_set_active(GTK_COMBO_BOX(ui->redblue_combo_box), 0);
	gtk_box_pack_start(GTK_BOX(hbox), ui->redblue_combo_box, FALSE, FALSE, 2);
	gtk_widget_set_tooltip_text(ui->redblue_combo_box,
				    "Choose to play as Red or Blue");
	g_signal_connect(G_OBJECT(ui->redblue_combo_box), "changed",
			 G_CALLBACK(redblue_cb), ui);

	ui->auto_setup_button = gtk_button_new_with_label("Auto Setup");
	g_signal_connect(ui->auto_setup_button, "clicked",
			 G_CALLBACK(auto_setup_clicked), ui);
	gtk_box_pack_start(GTK_BOX(hbox), ui->auto_setup_button, FALSE, FALSE, 2);
	gtk_widget_set_tooltip_text(ui->auto_setup_button,
				    "Randomly arrange your pieces in a sensible formation");

	ui->new_game_button = gtk_button_new_with_label("New Game");
	g_signal_connect(ui->new_game_button, "clicked",
			 G_CALLBACK(new_game_clicked), ui);
	gtk_box_pack_start(GTK_BOX(hbox), ui->new_game_button, FALSE, FALSE, 2);
	gtk_widget_set_tooltip_text(ui->new_game_button,
				    "Reset and start a new game");

	invert_button = gtk_button_new_with_label("Invert Board");
	g_signal_connect(invert_button, "clicked",
			 G_CALLBACK(invert_clicked), ui);
	gtk_box_pack_start(GTK_BOX(hbox), invert_button, FALSE, FALSE, 2);
	gtk_widget_set_tooltip_text(invert_button,
				    "Invert Board so that your half is on the bottom");

	help_button = gtk_button_new_with_label("Help");
	g_signal_connect(help_button, "clicked", G_CALLBACK(help_clicked), ui);
	gtk_box_pack_start(GTK_BOX(hbox), help_button, FALSE, FALSE, 2);

	quit_button = gtk_button_new_with_label("Quit Dueling");
	g_signal_connect(quit_button, "clicked", G_CALLBACK(quit_clicked), ui);
	gtk_box_pack_start(GTK_BOX(hbox), quit_button, FALSE, FALSE, 2);
	gtk_widget_set_tooltip_text(quit_button, "Quit Dueling");

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

	ui->drawing_area = gtk_drawing_area_new();
	ui->xdim = 600;
	ui->ydim = 600;
	gtk_widget_set_size_request(GTK_WIDGET(ui->drawing_area),
				    ui->xdim, ui->ydim);
	gtk_widget_modify_bg(ui->drawing_area, GTK_STATE_NORMAL, &black);
	g_signal_connect(G_OBJECT(ui->drawing_area), "expose_event",
			 G_CALLBACK(on_expose_drawing_area), ui);
	g_signal_connect(G_OBJECT(ui->drawing_area), "configure_event",
			 G_CALLBACK(on_config_drawing_area), ui);
	g_signal_connect(G_OBJECT(ui->drawing_area), "motion_notify_event",
			 G_CALLBACK(on_mouse_motion), ui);
	g_signal_connect(G_OBJECT(ui->drawing_area), "button_press_event",
			 G_CALLBACK(on_button_clicked), ui);

	gtk_widget_set_events(ui->drawing_area,
			      GDK_EXPOSURE_MASK | GDK_LEAVE_NOTIFY_MASK |
			      GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK |
			      GDK_POINTER_MOTION_HINT_MASK);
	gtk_box_pack_start(GTK_BOX(vbox), ui->drawing_area, TRUE, TRUE, 0);

	/* Status bar + Pass button */
	bottom_hbox = gtk_hbox_new(FALSE, 6);
	ui->status_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui->status_label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(bottom_hbox), ui->status_label,
			   TRUE, TRUE, 6);

	ui->pass_button = gtk_button_new_with_label("Pass Turn");
	g_signal_connect(ui->pass_button, "clicked",
			 G_CALLBACK(pass_clicked), ui);
	gtk_widget_set_sensitive(ui->pass_button, FALSE);
	gtk_box_pack_end(GTK_BOX(bottom_hbox), ui->pass_button, FALSE, FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), bottom_hbox, FALSE, FALSE, 2);

	/* Collision banner above combat log */
	ui->collision_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui->collision_label), 0.0, 0.5);
	gtk_label_set_line_wrap(GTK_LABEL(ui->collision_label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), ui->collision_label, FALSE, FALSE, 2);

	/* Compact scrollable combat/event log */
	log_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(log_scroll),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(log_scroll, -1, 80);
	ui->log_view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(ui->log_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(ui->log_view), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(ui->log_view), GTK_WRAP_WORD);
	ui->log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ui->log_view));
	ui->log_collision_tag = gtk_text_buffer_create_tag(
		ui->log_buffer, "collision",
		"foreground", "#d94e1f",
		"weight", PANGO_WEIGHT_BOLD,
		NULL);
	gtk_container_add(GTK_CONTAINER(log_scroll), ui->log_view);
	gtk_box_pack_start(GTK_BOX(vbox), log_scroll, FALSE, FALSE, 2);

	ui->inverted = 1;
	ui->piece_box_open = 2.0;
	ui->holding = NULL;
	ui->playing_as = PLAYING_AS_RED;
	ui->fullscreen = 0;
	ui->mode = MODE_BOARD_SETUP;
	ui->use_opponent = 1;
	ui->suppress_combo_cb = 0;
	ui->opponent = NULL;
	ui->poll_timer_id = 0;
	ui->setup_rng = (uint32_t)time(NULL) ^ 0x5a5a5a5au;
	ui->game_counter = 0;
	memset(ui->visited, 0, sizeof(ui->visited));

	setup_help_window(ui);
	gtk_widget_show_all(ui->window);
}

static void generic_draw_piece_on_board(struct piece *p, cairo_t *cr,
					double radius_scale)
{
	double radius;
	double x1, y1, x2, y2;
	int show_trail;

	if (ui.xdim / ui.piece_box_open < ui.ydim)
		radius = (0.75 * ui.xdim / (12.0 * ui.piece_box_open)) / 2.0;
	else
		radius = (0.75 * ui.ydim / 12.0) / 2.0;
	radius *= radius_scale;

	show_trail = (p->prevx != -1) &&
		(is_managed_mode(&ui) ||
		 (ui.playing_as == PLAYING_AS_RED && p->r > 0.9) ||
		 (ui.playing_as == PLAYING_AS_BLUE && p->b > 0.9));

	/* draw trail */
	if (show_trail) {
		cairo_save(cr);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width(cr,
				     MIN(ui.xdim / ui.piece_box_open, ui.ydim) /
				     64.0);
		cairo_set_source_rgba(cr, p->r, p->g, p->b, 0.35);
		x1 = (p->x + 1.5) * ui.xdim / (12.0 * ui.piece_box_open);
		y1 = (invert(p->y) + 1.5) * (ui.ydim / 12.0);
		x2 = (p->prevx + 1.5) * ui.xdim / (12.0 * ui.piece_box_open);
		y2 = (invert(p->prevy) + 1.5) * (ui.ydim / 12.0);
		cairo_move_to(cr, x1, y1);
		if (p->strength == WFIGHTER) {
			double tx, ty;
			double factor = ui.inverted ? -1.0 : 1.0;

			if (abs(p->x - p->prevx) < abs(p->y - p->prevy)) {
				if (p->prevx < p->x)
					tx = x1 - ui.xdim / (12.0 * ui.piece_box_open);
				else
					tx = x1 + ui.xdim / (12.0 * ui.piece_box_open);
				ty = y1;
			} else {
				if (p->prevy < p->y)
					ty = y1 - factor * ui.ydim / 12.0;
				else
					ty = y1 + factor * ui.ydim / 12.0;
				tx = x1;
			}
			cairo_line_to(cr, tx, ty);
		}
		cairo_line_to(cr, x2, y2);
		cairo_stroke(cr);
		cairo_restore(cr);
	}

	cairo_save(cr);
	cairo_arc(cr, p->sx, p->sy, radius, 0.0, 2.0 * M_PI);
	cairo_set_source_rgba(cr, p->r, p->g, p->b, 0.25);
	cairo_fill_preserve(cr);
	cairo_stroke(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_set_line_width(cr, MIN(ui.xdim, ui.ydim) / 200.0);
	if (p->in_transit) {
		static double transit_dash[] = { 4.0, 3.0 };
		cairo_set_dash(cr, transit_dash, 2, 0.0);
	}
	cairo_set_source_rgba(cr, p->r, p->g, p->b, 1.0);
	cairo_arc(cr, p->sx, p->sy, radius * 1.1, 0.0, 2.0 * M_PI);
	cairo_stroke(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	if (radius > 12.0)
		draw_centered_text(cr, p->sx, p->sy,
				   MIN(ui.xdim, ui.ydim) / 32.0 * radius_scale,
				   p->name);
	cairo_stroke(cr);
	cairo_restore(cr);
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
	init_pieces(&p1, &p2);
	init_ui(&argc, &argv, &ui);
	reset_managed_game(&ui);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
	return 0;
}
