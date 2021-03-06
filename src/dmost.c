/* 
    (C) Copyright 2012, Stephen M. Cameron.

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
#include <stdlib.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <math.h>
#include <stdint.h>

#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdkkeysyms.h>

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

struct piece;


struct piece {
	char *name;
	char *description;
	int x, y, prevx, prevy;
#define UNKNOWN (-1)
	int strength;

#define WFIGHTER 6
#define BFIGHTER 5
#define XFIGHTER 4
#define YFIGHTER 3
#define ZFIGHTER 2
#define ASTEROID 1
#define HOMEPLANET 0

	double sx, sy;
	double r,g,b;
	int pickedx, pickedy; /* place piece was picked from but before move is final */
} full_set[] = {
	{ "W", "W fighter.  Moves like chess knight.  Strongest piece.",
			-1, -1, -1, -1, 6, -1, -1, 1.0, 0.1, 0.1, -1, -1 },
	{ "B", "B fighter.  Moves like chess queen.  Only W is stronger.",
			-1, -1, -1, -1, 5, -1, -1 , 1.0, 0.1, 0.1, -1, -1},
	{ "X1", "X1 fighter.  Moves like chess rook.  W and B are stronger",
			-1, -1, -1, -1, 4, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "X2", "X2 fighter.  Moves like chess rook.  W and B are stronger",
			-1, -1, -1, -1, 4, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "Y1", "Y1 fighter.  Moves like chess bishop.  W, X, and B are stronger",
			-1, -1, -1, -1, 3, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "Y2", "Y2 fighter.  Moves like chess bishop.  W, X, and B are stronger",
			-1, -1, -1, -1, 3, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "Z1", "Z1 fighter.  Moves like chess queen.  Weakest mobile piece",
			-1, -1, -1, -1, 2, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "Z2", "Z2 fighter.  Moves like chess queen.  Weakest mobile piece",
			-1, -1, -1, -1, 2, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "Z3", "Z3 fighter.  Moves like chess queen.  Weakest mobile piece",
			-1, -1, -1, -1, 2, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "Z4", "Z4 fighter.  Moves like chess queen.  Weakest mobile piece",
			-1, -1, -1, -1, 2, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ "HP", "Home Planet", -1, -1, -1, -1, 0, -1, -1, 1.0, 0.1, 0.1 , -1, -1},
	{ NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1.0, 0.3, 0.3 , 0, 0},
};

struct piece *p1, *p2;
	
struct gui {
	GtkWidget *window;
	GtkWidget *help;
	GtkWidget *hbox;
	GtkWidget *drawing_area;
	GtkWidget *mode_combo_box;
	GtkWidget *redblue_combo_box;
	int xdim, ydim;
	double piece_box_open;
	int inverted;
	int mousex, mousey;
	struct piece *holding;
	int playing_as;
	int buttons_showing;
	int fullscreen;
#define PLAYING_AS_RED 1
#define PLAYING_AS_BLUE 2
	int mode;
#define MODE_FREE 0
#define MODE_BOARD_SETUP 1
#define MODE_PLAYING 2
	uint8_t visited[10][10];
};

static GdkColor black;

static struct gui ui;

static void invert_clicked(__attribute__((unused)) GtkWidget *widget,
			__attribute__((unused)) gpointer data)
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
}

static void close_help_clicked(GtkWidget *widget, gpointer data)
{
	struct gui *ui = data;
	
	gtk_widget_hide(widget);
}

static void quit_clicked(__attribute__((unused)) GtkWidget *widget,
			__attribute__((unused)) gpointer data)
{
	gtk_main_quit();
}

static gint key_press_cb(GtkWidget* widget, GdkEventKey* event, gpointer data)
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

	switch(alignment) {
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
	cairo_select_font_face (cr, "SERIF", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	cairo_set_font_size(cr, fontsize);
	cairo_text_extents(cr, text, &extents);
	x = x + direction * (factor * extents.width  + extents.x_bearing);
	y = y - (extents.height / 2 + extents.y_bearing);

	cairo_move_to(cr, x, y);
	cairo_show_text(cr, text);
}

static inline void draw_centered_text(cairo_t *cr, double x, double y,
			       double fontsize, const char *text)
{
	draw_aligned_text(cr, x, y, fontsize, text, CENTERED);
}

static inline void draw_right_justified_text(cairo_t *cr,
				double x, double y,
				double fontsize, const char *text)
{
	draw_aligned_text(cr, x, y, fontsize, text, RIGHT_JUSTIFIED);
}

static inline void draw_left_justified_text(cairo_t *cr,
				double x, double y,
				double fontsize, const char *text)
{
	draw_aligned_text(cr, x, y, fontsize, text, LEFT_JUSTIFIED);
}

int invert(int y)
{
	if (!ui.inverted)
		return y;
	return 9 - y;
}

static void draw_pieces_on_board(cairo_t *cr, struct piece *p)
{
	int i;

	for (i = 0; p[i].name != NULL; i++) {
		if (p[i].x == UNKNOWN)
			continue;
		if (&p[i] == ui.holding)
			continue;
		p[i].sx = p[i].x * ui.xdim / (12.0 * ui.piece_box_open) + 
					1.5 * ui.xdim / (ui.piece_box_open * 12.0);
		p[i].sy = invert(p[i].y) * ui.ydim / 12.0 + 1.5 * (ui.ydim / 12.0);
		generic_draw_piece_on_board(&p[i], cr);
	}
}

void generic_draw_piece_off_board(struct piece *p, cairo_t *cr, double radius)
{
	cairo_save(cr);
	cairo_arc(cr, p->sx, p->sy, radius, 0.0, 2.0 * M_PI); // full circle
	cairo_set_source_rgba(cr, p->r, p->g, p->b, 0.25);    // partially translucent
	cairo_fill_preserve(cr);
	cairo_stroke(cr);
	cairo_restore(cr);
	cairo_save(cr);
	cairo_set_line_width (cr, MIN(ui.xdim, ui.ydim) / 200.0);
	cairo_set_source_rgba(cr, p->r, p->g, p->b, 1.0);    // partially translucent
	cairo_arc(cr, p->sx, p->sy, radius * 1.1, 0.0, 2.0 * M_PI); // full circle
	cairo_stroke(cr);
	cairo_restore(cr);  // back to opaque black
	cairo_save(cr);
#if 0
	cairo_set_source_rgba(cr, p->r, p->g, p->b, 1.0);    // partially translucent
	if (radius > 15.0)
		draw_centered_text(cr, p->sx, p->sy, MIN(ui.xdim, ui.ydim) / 32.0, p->name);
#endif
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	if (radius > 15.0 && radius < 23.0)
		draw_centered_text(cr, p->sx, p->sy, MIN(ui.xdim, ui.ydim) / 32.0, p->name);
	if (radius > 23.0)
		draw_centered_text(cr, p->sx, p->sy, MIN(ui.xdim, ui.ydim) / 20.0, p->name);
	cairo_stroke(cr);
	cairo_restore(cr);  // back to opaque black
}

static void draw_pieces_off_board(cairo_t *cr, struct piece *p, int *initial_count)
{
	int i, count;
	double x, y, leftx, xwidth, slotwidth;
	double radius;

	leftx = (11.0 * ui.xdim / (12.0 * ui.piece_box_open));
	xwidth = ui.xdim - leftx; 
	slotwidth = MIN(xwidth / 6.0, ui.xdim / (12.0)); 
	radius = (slotwidth) / 4.0;

	count = *initial_count;
	for (i = 0; p[i].name != NULL; i++) {
		if (p[i].x != UNKNOWN)
			continue;
		if (&p[i] == ui.holding)
			continue;
		p[i].sx = leftx + (count % 4) * slotwidth + slotwidth / 2.0;
		p[i].sy = (ui.ydim / 12.0) + (count / 4) * slotwidth / 1.5;
		generic_draw_piece_off_board(&p[i], cr, radius);
		count++;
	}
	*initial_count = count;
}

static int legal_move(struct gui *ui, struct piece *p, int x, int y)
{
	int fromx, fromy, legal;

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

	/* should not get here. */
	printf("unknown mode %d\n", ui->mode);
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
	highlight_square(cr, ui, x, y, 0.0, 1.0, 0.0, 0.1);
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
	double r, b;

	r = 0.0;
	b = 0.0;
	if (ui->playing_as == PLAYING_AS_RED)
		r = 1.0;
	if (ui->playing_as == PLAYING_AS_BLUE)
		b = 1.0;

	cairo_stroke(cr);
	cairo_save(cr);	
	for (x = 0; x < 10; x++)
		for (y = 0; y < 10; y++)
			if (ui->visited[x][y])
				highlight_square(cr, ui, x, y, r, 0.0, b, 0.2);
	cairo_stroke(cr);
	cairo_restore(cr);
}

static int on_expose_drawing_area(GtkWidget *w, GdkEvent *event, gpointer p)
{
	struct gui *ui = p;
	cairo_t *cr;
	static cairo_surface_t *galaxy_image = NULL;
	static int xd, yd;
	static double dash[] = { 1.0, 2.0 };
	int i, off_board_piece_count = 0;

	if (ui->holding != NULL) {
		ui->holding->sx = ui->mousex - 20;
		ui->holding->sy = ui->mousey - 20;
	}
	if (galaxy_image == NULL) {
		galaxy_image = cairo_image_surface_create_from_png("pinwheel.png");
		xd = cairo_image_surface_get_width(galaxy_image);
		yd = cairo_image_surface_get_height(galaxy_image);
	}

	cr = gdk_cairo_create(w->window);

	cairo_save(cr);
	if (ui->inverted) {
		cairo_scale(cr, 1.0, -1.0);
		cairo_translate(cr, 0.0, -ui->ydim);
	}

	cairo_scale (cr, (double) ui->xdim / (double) xd / ui->piece_box_open,
				(double) ui->ydim / (double) yd);


	cairo_set_source_surface (cr, galaxy_image, 0, 0);
	cairo_paint_with_alpha(cr, 0.5);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_set_dash(cr, dash, 2, 2.0);
	// cairo_set_line_width(cr, 0.5);
	cairo_set_source_rgb (cr, 0.5, 1, 0.5);
	for (i = 1; i <= 11; i++) {
		cairo_move_to(cr, ui->xdim / (12.0 * ui->piece_box_open) * i, ui->ydim / 12.0);
		cairo_line_to(cr, ui->xdim / (12.0 * ui->piece_box_open) * i, 11.0 * ui->ydim / 12.0);
		cairo_move_to(cr, ui->xdim / (12.0 * ui->piece_box_open), i * ui->ydim / 12.0);
		cairo_line_to(cr, 11.0 * ui->xdim / (12.0 * ui->piece_box_open), i *ui->ydim / 12.0);
	}

	switch (ui->playing_as) {
	case 1: cairo_set_source_rgb (cr, 1.0, 0.1, 0.1);
		break;
	case 2: cairo_set_source_rgb (cr, 0.1, 0.1, 1.0);
		break;
	}

	for (i = 0; i < 10; i++) {
		char letter[2];
		double y;
		sprintf(letter, "%c", 'A' + i);
		y = (i + 1.5) * ui->ydim / 12.0;
		if (ui->inverted)
			y = ui->ydim - y; 
		draw_centered_text(cr, ui->xdim / (24.0 * ui->piece_box_open), y, ui->ydim / 30.0, letter);
	}
	for (i = 0; i < 10; i++) {
		char number[2];
		sprintf(number, "%d", i + 1);
		draw_centered_text(cr, (i + 1.5) * ui->xdim / (12.0 * ui->piece_box_open),
					ui->ydim / 24.0, (ui->xdim / ui->piece_box_open) / 30.0,
					number);
	}
	cairo_stroke(cr);
	cairo_restore(cr);

	highlight_visited_squares(cr, ui);
	highlight_legal_moves(cr, ui);

	draw_pieces_on_board(cr, p1);
	draw_pieces_on_board(cr, p2);
	draw_pieces_off_board(cr, p1, &off_board_piece_count);
	draw_pieces_off_board(cr, p2, &off_board_piece_count);
	if (ui->holding)
		generic_draw_piece_off_board(ui->holding, cr, ui->xdim / 24.0);

	cairo_destroy(cr);
}

static gint on_config_drawing_area(GtkWidget *w, GdkEventConfigure *event,
                                       gpointer p)
{
	struct gui *ui = p;

	ui->xdim = w->allocation.width;
	ui->ydim = w->allocation.height;
}

static void hide_or_show_buttons(struct gui *ui)
{
	/* Hide or show buttons depending on if mouse is near top of screen */
	if (ui->mousey < ui->ydim / 15.0 && !ui->buttons_showing) {
		gtk_widget_show_all(ui->hbox);
		ui->buttons_showing = 1;
		gtk_widget_queue_draw(ui->window);
		return;
	}
	if (ui->mousey > ui->ydim / 15.0 && ui->buttons_showing) {
		gtk_widget_hide(ui->hbox);
		ui->buttons_showing = 0;
		gtk_widget_queue_draw(ui->window);
	}
}

static hide_or_show_piecebox(struct gui *ui, GtkWidget *w)
{
	if (ui->mode == MODE_BOARD_SETUP) {
		ui->piece_box_open = 2.0;
		return;
	}
	if (ui->piece_box_open > 1.0 && ui->mousex < ui->xdim / 2.2) {
		gtk_widget_queue_draw(w);
		ui->piece_box_open *= 0.8;
			if (ui->piece_box_open < 1.0)
			ui->piece_box_open = 1.0;
	} else {
		if (ui->mousex > 11.0 * (ui->xdim / (12.0 * ui->piece_box_open)) &&
					ui->piece_box_open < 2.0) {
			ui->piece_box_open *= 1.2;
			gtk_widget_queue_draw(w);
			if (ui->piece_box_open > 2.0)
				ui->piece_box_open = 2.0;
		}
	}
}

static int on_mouse_motion(GtkWidget *w, GdkEvent *event, gpointer p)
{
	struct gui *ui = p;
	GdkEventMotion *e = (GdkEventMotion *) event;

	if (!e)
		return TRUE;

	ui->mousex = e->x;
	ui->mousey = e->y;

	hide_or_show_buttons(ui);
	hide_or_show_piecebox(ui, w);
	if (ui->holding)
		gtk_widget_queue_draw(w);

	return TRUE;
}

static struct piece *find_closest_piece(struct piece *p, struct piece *prev,
					double x, double y, double *min)
{
	int i;
	double dist, best_dist;
	struct piece *found = NULL;
	
	if (prev) {
		best_dist = (prev->sx - x) * (prev->sx - x) + 
				(prev->sy - y) * (prev->sy - y);
		found = prev;
	}

	for (i = 0; p[i].name != NULL; i++) {
		dist = (p[i].sx - ui.mousex) * (p[i].sx - ui.mousex) +
			(p[i].sy - ui.mousey) * (p[i].sy - ui.mousey);
		if (dist < (*min) * (*min) && found == NULL) {
			found = &p[i];
			best_dist = dist;
		}
		if (found != NULL && dist < best_dist) {
			found = &p[i];
			best_dist = dist;
		}
	}
	if (found)
		*min = best_dist;
	return found;
}

static int on_button_clicked(GtkWidget *w, GdkEvent *event, gpointer ptr)
{
	struct gui *ui = ptr;
	int i;
	int first = 1;
	double min;
	struct piece *p = NULL, *px = NULL;

	if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
		/* attempt to pick up or drop a piece */
		if (ui->holding != NULL) {
			/* dropping a piece */
			if (ui->mousex > (11.0 * ui->xdim / (12.0 * ui->piece_box_open))) {

				/* off the board */
				ui->holding->x = UNKNOWN;
				ui->holding->sx = -100;
				ui->holding->sy = -100;
				ui->holding->pickedx = -1;
				ui->holding->pickedy = -1;
				ui->holding = NULL;
				gtk_widget_queue_draw(w);
			} else {
				if (ui->mousex - 10 > ui->xdim / (12.0 * ui->piece_box_open) &&
				    ui->mousex - 10 < 11.0 * ui->xdim / (12.0 * ui->piece_box_open) &&
				    ui->mousey - 10 > ui->ydim / 12.0 &&
				    ui->mousey - 10 < 11.0 * ui->ydim / 12.0) {
					/* on the board */
					int tmpx, tmpy;
					tmpy = invert((ui->mousey - 10 - ui->ydim / 12.0) / (ui->ydim / 12.0));
					tmpx = (ui->mousex - 10 - (ui->xdim / (12.0 * ui->piece_box_open))) /
							(ui->xdim / (12.0 * ui->piece_box_open));
					if (!legal_move(ui, ui->holding, tmpx, tmpy))
						return;
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
			}
			return;
		}

		/* picking up piece */
		min = 30.0;
		p = find_closest_piece(p1, NULL, ui->mousex, ui->mousey, &min); 
		px = find_closest_piece(p2, p, ui->mousex, ui->mousey, &min); 
		if (px)
			p = px;
		if (p != NULL)
			ui->holding = p;
		gtk_widget_queue_draw(ui->drawing_area);
		return TRUE;
	}

	if (event->type == GDK_BUTTON_PRESS && event->button.button == 3) {
		/* attept to mark a square as visited */
		if (ui->mousex > ui->xdim / (12.0 * ui->piece_box_open) &&
		    ui->mousex < 11.0 * ui->xdim / (12.0 * ui->piece_box_open) &&
		    ui->mousey > ui->ydim / 12.0 &&
		    ui->mousey < 11.0 * ui->ydim / 12.0) {
			/* on the board */
			int tmpx, tmpy;
			tmpy = invert((ui->mousey - ui->ydim / 12.0) / (ui->ydim / 12.0));
			tmpx = (ui->mousex - (ui->xdim / (12.0 * ui->piece_box_open))) /
					(ui->xdim / (12.0 * ui->piece_box_open));
			ui->visited[tmpx][tmpy] = !ui->visited[tmpx][tmpy];
			gtk_widget_queue_draw(ui->drawing_area);
		}
	}
}

static void mode_cb(GtkEntry *entry, gpointer data)
{
	struct gui *ui = data;
	gchar *ctext;
	
	ctext = gtk_combo_box_get_active_text(GTK_COMBO_BOX(ui->mode_combo_box));
	if (strcmp(ctext, "Free form") == 0) {
		ui->mode = MODE_FREE;
		return;
	}
	if (strcmp(ctext, "Board Setup") == 0) {
		ui->mode = MODE_BOARD_SETUP;
		return;
	}
	if (strcmp(ctext, "Play Game") == 0) {
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
	}
	gtk_widget_queue_draw(ui->drawing_area);
}

static void redblue_cb(GtkEntry *entry, gpointer data)
{
	struct gui *ui = data;
	gchar *ctext;
	
	ctext = gtk_combo_box_get_active_text(GTK_COMBO_BOX(ui->redblue_combo_box));
	ui->playing_as = -1;
	if (strcmp(ctext, "Play as Red") == 0) {
		ui->playing_as = PLAYING_AS_RED;
		ui->inverted = 1;
	} else {
		if (strcmp(ctext, "Play as Blue") == 0) {
			ui->playing_as = PLAYING_AS_BLUE;
			ui->inverted = 0;
		}
	}
	gtk_widget_queue_draw(ui->drawing_area);
}

static void setup_help_window(struct gui *ui)
{
	GtkWidget *vbox, *view, *scrolled_window;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	GtkTextIter iter;
	static char *instructions =
"   Instructions\n"
"   \n"
"   1. Establish communication with your opponent outside the gaem.\n"
"      This can be as simple as sitting across the table from them\n"
"      or talking to them on the phone, IRC, Skype, Google Hangouts,\n"
"      etc.\n"
"   \n"
"   2. Decide who will go first, and who will play as Red and who will\n"
"      play as Blue.  Select 'Play as Red' or 'Play as Blue' at the top\n"
"      of the window.\n"
"   \n"
"   3. Arrange your pieces on your half of the board.\n"
"   \n"
"   4. When all your pieces are arranged, select 'Play Game' from the top\n"
"      left of the menu at the top of the window.\n"
"   \n"
"   5. When it is your turn, select the piece you want to move (left click\n"
"      on it withe the mouse) and move it where you like.  Announce to your\n"
"      opponent (via the communications channel established in step 1) which\n"
"      piece you are moving, and where your are moving the piece FROM.  Do\n"
"      NOT REVEAL where you are moving the piece TO.\n"
"\n"
"   6. When it is your opponent's turn, he will announce to you that he has\n"
"      moved a certain one of his pieces from a certain location on the board.\n"
"      If this is the first time he has moved this piece, simply place it on the\n"
"      board in the location your opponent indicates.  If this is not the first\n"
"      time the piece has been moved, the piece will already be on the board.  In\n"
"      that case, check to see if the piece passes through any of your pieces. If\n"
"      So, there is a collision.  The stronger piece destroys the weaker piece.\n"
"      The pieces are, in order from strongest to weakest: A, W, B, X, Y, Z, HP.\n"
"      Asteroids destroy anything which touches them, and are destroyed by anythnig\n"
"      that touches them (think of them as mines).   In the event of a collision,\n"
"      when you tell your opponent that he has encountered one of your pieces, your\n"
"      opponent will check to see if your piece made it to the location you say it's\n"
"      at.  You may find that, in turn, your piece has also been destroyed elsewhere.\n"
"      This collision resolution is carried out recursively until there are no more\n"
"      collisions."
"\n"
"   7. The game is over one either player destroys the other player's Home Planet\n"
"      or when it is determined that neither player can destroy the other's Home\n"
"      planet (due to having too few pieces, or for example, only Y fighters which\n"
"      are on the wrong squares to reach the Home Planet (since they travel diagonally\n"
"      a single Y fighter can only reach half the squares on the board.)\n"
"   \n";

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
			GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
	ui->help = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(ui->help, "delete-event", G_CALLBACK(close_help_clicked), ui);
	g_signal_connect(ui->help, "destroy", G_CALLBACK(close_help_clicked), ui);
	vbox = gtk_vbox_new(FALSE, 0);
	view = gtk_text_view_new();
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
	gtk_text_buffer_insert(buffer, &iter, instructions, -1);
	gtk_scrolled_window_add_with_viewport(GTK_CONTAINER(scrolled_window), view);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(ui->help), vbox);
}

static void init_ui(int *argc, char **argv[], struct gui *ui)
{
	GtkWidget *vbox, *scrolled_window, *hbox, *bottom_align,
			*invert_button, *quit_button, *help_button;

	if (!g_thread_supported())
		g_thread_init(NULL);
	gdk_threads_init();

	gtk_init(argc, argv);

	gdk_color_parse("black", &black);

	ui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(ui->window), "Dueling Masters of Space Time");
	gtk_window_set_default_size(GTK_WINDOW(ui->window), 1024, 768);

	g_signal_connect(ui->window, "delete-event", G_CALLBACK(quit_clicked), NULL);
	g_signal_connect(ui->window, "destroy", G_CALLBACK(quit_clicked), NULL);
	g_signal_connect(G_OBJECT(ui->window), "key_press_event",
			G_CALLBACK(key_press_cb), ui);


	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(ui->window), vbox);
        ui->drawing_area = gtk_drawing_area_new();
	ui->xdim = 300;
	ui->ydim = 300;
        gtk_widget_set_size_request(GTK_WIDGET(ui->drawing_area), ui->xdim, ui->ydim);
        gtk_widget_modify_bg(ui->drawing_area, GTK_STATE_NORMAL, &black);
        g_signal_connect(G_OBJECT(ui->drawing_area), "expose_event",
			G_CALLBACK(on_expose_drawing_area), ui);
        g_signal_connect(G_OBJECT(ui->drawing_area), "configure_event",
			G_CALLBACK(on_config_drawing_area), ui);
	gtk_signal_connect(GTK_OBJECT(ui->drawing_area), "motion_notify_event",
			G_CALLBACK(on_mouse_motion), ui);
	gtk_signal_connect(GTK_OBJECT(ui->drawing_area), "button_press_event",
			G_CALLBACK(on_button_clicked), ui);

	gtk_widget_set_events(ui->drawing_area, GDK_EXPOSURE_MASK
				| GDK_LEAVE_NOTIFY_MASK
				| GDK_BUTTON_PRESS_MASK
				| GDK_POINTER_MOTION_MASK
				| GDK_POINTER_MOTION_HINT_MASK);

        /*
         * Set up alignments for widgets at the bottom of ui, 
         * align bottom left, expand horizontally but not vertically
         */
	bottom_align = gtk_alignment_new(0, 1, 1, 0);
	hbox = gtk_hbox_new(FALSE, 0);
	ui->hbox = hbox;
	gtk_container_add(GTK_CONTAINER(bottom_align), hbox);
        ui->mode_combo_box = gtk_combo_box_new_text();
        gtk_combo_box_append_text(GTK_COMBO_BOX(ui->mode_combo_box), "Board Setup");
        gtk_combo_box_append_text(GTK_COMBO_BOX(ui->mode_combo_box), "Play Game");
        gtk_combo_box_append_text(GTK_COMBO_BOX(ui->mode_combo_box), "Free form");
        gtk_combo_box_set_active(GTK_COMBO_BOX(ui->mode_combo_box), 0);
        gtk_container_add(GTK_CONTAINER(hbox), ui->mode_combo_box);
	g_signal_connect(GTK_OBJECT(ui->mode_combo_box), "changed",
				G_CALLBACK(mode_cb), ui);

        ui->redblue_combo_box = gtk_combo_box_new_text();
        gtk_combo_box_append_text(GTK_COMBO_BOX(ui->redblue_combo_box), "Play as Red");
        gtk_combo_box_append_text(GTK_COMBO_BOX(ui->redblue_combo_box), "Play as Blue");
        gtk_combo_box_set_active(GTK_COMBO_BOX(ui->redblue_combo_box), 0);
        gtk_container_add(GTK_CONTAINER(hbox), ui->redblue_combo_box);
	g_signal_connect(GTK_OBJECT(ui->redblue_combo_box), "changed",
				G_CALLBACK(redblue_cb), ui);

        gtk_widget_set_tooltip_text(ui->redblue_combo_box, "Choose to play as Red or Blue");

	/* add invert button */
	invert_button = gtk_button_new_with_label("Invert Board");
        g_signal_connect(invert_button, "clicked", G_CALLBACK(invert_clicked), ui);
        gtk_box_pack_start(GTK_BOX(hbox), invert_button, FALSE, FALSE, 3);
        gtk_widget_set_tooltip_text(invert_button, "Invert Board so that your half is on the bottom");

	/* add Help button */
	help_button = gtk_button_new_with_label("Help");
        g_signal_connect(help_button, "clicked", G_CALLBACK(help_clicked), ui);
        gtk_box_pack_start(GTK_BOX(hbox), help_button, FALSE, FALSE, 3);

	/* add Quit button */
	quit_button = gtk_button_new_with_label("Quit Dueling");
        g_signal_connect(quit_button, "clicked", G_CALLBACK(quit_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(hbox), quit_button, FALSE, FALSE, 3);
        gtk_widget_set_tooltip_text(quit_button, "Quit Dueling");


	gtk_box_pack_start(GTK_BOX(vbox), bottom_align, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(vbox), ui->drawing_area);

	ui->inverted = 1;
	ui->piece_box_open = 1;
	ui->holding = NULL;
	ui->playing_as = PLAYING_AS_RED;
	ui->buttons_showing = 1;
	ui->fullscreen = 0;
	ui->mode = MODE_BOARD_SETUP;
	memset(ui->visited, 0, sizeof(ui->visited));

	setup_help_window(ui);

	gtk_widget_show_all(ui->window);
}

void generic_draw_piece_on_board(struct piece *p, cairo_t *cr)
{
	double radius;
	double x1, y1, x2, y2;

	if (ui.xdim / ui.piece_box_open < ui.ydim)
		radius = (0.75 * ui.xdim / (12.0 * ui.piece_box_open)) / 2.0;
	else
		radius = (0.75 * ui.ydim / 12.0) / 2.0;

	/* draw trail */
	if (p->prevx != -1 && 
		((ui.playing_as == PLAYING_AS_RED && p->r > 0.9) ||
		(ui.playing_as == PLAYING_AS_BLUE && p->b > 0.9))) {
		cairo_save(cr);
		cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width (cr, MIN(ui.xdim / ui.piece_box_open, ui.ydim) / 64.0);
		cairo_set_source_rgba(cr, p->r, p->g, p->b, 0.35);    // partially translucent
		x1 = (p->x + 1.5) * ui.xdim / (12.0 * ui.piece_box_open);
		y1 = (invert(p->y) + 1.5) * (ui.ydim / 12.0);
		x2 = (p->prevx + 1.5) * ui.xdim / (12.0 * ui.piece_box_open);
		y2 = (invert(p->prevy) + 1.5) * (ui.ydim / 12.0);
		cairo_move_to(cr, x1, y1);
		if (p->strength == WFIGHTER) {
			double tx, ty;
			double factor;
			if (ui.inverted)
				factor = -1;
			else
				factor = 1;
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
	cairo_arc(cr, p->sx, p->sy, radius, 0.0, 2.0 * M_PI); // full circle
	cairo_set_source_rgba(cr, p->r, p->g, p->b, 0.25);    // partially translucent
	cairo_fill_preserve(cr);
	cairo_stroke(cr);
	cairo_restore(cr);
	cairo_save(cr);
	cairo_set_line_width (cr, MIN(ui.xdim, ui.ydim) / 200.0);
	cairo_set_source_rgba(cr, p->r, p->g, p->b, 1.0);    // partially translucent
	cairo_arc(cr, p->sx, p->sy, radius * 1.1, 0.0, 2.0 * M_PI); // full circle
	cairo_stroke(cr);
	cairo_restore(cr);  // back to opaque black
	cairo_save(cr);
#if 0
	cairo_set_source_rgba(cr, p->r, p->g, p->b, 1.0);    // partially translucent
	draw_centered_text(cr, p->sx, p->sy, MIN(ui.xdim, ui.ydim) / 32.0, p->name);
	cairo_stroke(cr);
#endif
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	if (radius > 15.0)
		draw_centered_text(cr, p->sx, p->sy, MIN(ui.xdim, ui.ydim) / 32.0, p->name);
	cairo_stroke(cr);
	cairo_restore(cr);  // back to opaque black
}


static void init_pieces(struct piece **p1, struct piece **p2)
{
	int i;

	*p1 = malloc(sizeof(full_set));
	*p2 = malloc(sizeof(full_set));
	memcpy(*p1, full_set, sizeof(full_set));
	memcpy(*p2, full_set, sizeof(full_set));

	/* Make player 2's pieces blue */
	for (i = 0; i < ARRAYSIZE(full_set); i++) {
		(*p2)[i].r = 0.3;
		(*p2)[i].g = 0.3;
		(*p2)[i].b = 1.0;
	}
}

int main(int argc, char *argv[])
{
	init_ui(&argc, &argv, &ui);
	init_pieces(&p1, &p2);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
}
