#include <stdlib.h>
#include <malloc.h>
#include <math.h>

#include <gtk/gtk.h>
#include <cairo.h>

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

struct piece;


struct piece {
	char *name;
	char *description;
	int x, y, prevx, prevy;
#define UNKNOWN (-1)
	int strength;
	double sx, sy;
	double r,g,b;
} full_set[] = {
	{ "W", "W fighter.  Moves like chess knight.  Strongest piece.",
			-1, -1, -1, -1, 6, -1, -1, 1.0, 0.3, 0.3 },
	{ "B", "B fighter.  Moves like chess queen.  Only W is stronger.",
			-1, -1, -1, -1, 5, -1, -1 , 1.0, 0.3, 0.3},
	{ "X1", "X1 fighter.  Moves like chess rook.  W and B are stronger",
			-1, -1, -1, -1, 4, -1, -1, 1.0, 0.3, 0.3 },
	{ "X2", "X2 fighter.  Moves like chess rook.  W and B are stronger",
			-1, -1, -1, -1, 4, -1, -1, 1.0, 0.3, 0.3 },
	{ "Y1", "Y1 fighter.  Moves like chess bishop.  W, X, and B are stronger",
			-1, -1, -1, -1, 3, -1, -1, 1.0, 0.3, 0.3 },
	{ "Y2", "Y2 fighter.  Moves like chess bishop.  W, X, and B are stronger",
			-1, -1, -1, -1, 3, -1, -1, 1.0, 0.3, 0.3 },
	{ "Z1", "Z1 fighter.  Moves like chess queen.  Weakest mobile piece",
			-1, -1, -1, -1, 2, -1, -1, 1.0, 0.3, 0.3 },
	{ "Z2", "Z2 fighter.  Moves like chess queen.  Weakest mobile piece",
			-1, -1, -1, -1, 2, -1, -1, 1.0, 0.3, 0.3 },
	{ "Z3", "Z3 fighter.  Moves like chess queen.  Weakest mobile piece",
			-1, -1, -1, -1, 2, -1, -1, 1.0, 0.3, 0.3 },
	{ "Z4", "Z4 fighter.  Moves like chess queen.  Weakest mobile piece",
			-1, -1, -1, -1, 2, -1, -1, 1.0, 0.3, 0.3 },
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.3, 0.3 },
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.3, 0.3 },
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.3, 0.3 },
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.3, 0.3 },
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.3, 0.3 },
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.3, 0.3 },
	{ "A", "Asteroid", -1, -1, -1, -1, 1, -1, -1, 1.0, 0.3, 0.3 },
	{ "HP", "Home Planet", -1, -1, -1, -1, 0, -1, -1, 1.0, 0.3, 0.3 },
	{ NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1.0, 0.3, 0.3 },
};

struct piece *p1, *p2;
	
struct gui {
	GtkWidget *window;
	GtkWidget *drawing_area;
	int xdim, ydim;
	double piece_box_open;
	int inverted;
	int mousex, mousey;
	struct piece *holding;
};

static GdkColor black;

static struct gui ui;

static void quit_clicked(__attribute__((unused)) GtkWidget *widget,
			__attribute__((unused)) gpointer data)
{
	gtk_main_quit();
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
		p[i].sy = p[i].y * ui.ydim / 12.0 + 1.5 * (ui.ydim / 12.0);
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
	cairo_set_source_rgba(cr, p->r, p->g, p->b, 1.0);    // partially translucent
	if (radius > 15.0)
		draw_centered_text(cr, p->sx, p->sy, MIN(ui.xdim, ui.ydim) / 32.0, p->name);
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
	cairo_paint_with_alpha(cr, 0.7);
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

static int on_mouse_motion(GtkWidget *w, GdkEvent *event, gpointer p)
{
	struct gui *ui = p;
	GdkEventMotion *e = (GdkEventMotion *) event;

	if (e) {
		ui->mousex = e->x;
		ui->mousey = e->y;
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
		if (ui->holding)
			gtk_widget_queue_draw(w);
	}
	return TRUE;
}

static struct piece *find_closest_piece(struct piece *p, double x, double y, double *min)
{
	int i;
	double dist, best_dist;
	struct piece *found = NULL;

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

	if (event->type == GDK_BUTTON_PRESS) {
		if (ui->holding != NULL) {
			if (ui->mousex > (11.0 * ui->xdim / (12.0 * ui->piece_box_open))) {
				ui->holding->x = UNKNOWN;
				ui->holding->sx = -100;
				ui->holding->sy = -100;
				ui->holding = NULL;
				gtk_widget_queue_draw(w);
			} else {
				if (ui->mousex > ui->xdim / (12.0 * ui->piece_box_open) &&
				    ui->mousex < 11.0 * ui->xdim / (12.0 * ui->piece_box_open) &&
				    ui->mousey > ui->ydim / 12.0 &&
				    ui->mousey < 11.0 * ui->ydim / 12.0) {

					ui->holding->y = (ui->mousey - ui->ydim / 12.0) / (ui->ydim / 12.0);
					ui->holding->x = (ui->mousex - (ui->xdim / (12.0 * ui->piece_box_open))) /
							(ui->xdim / (12.0 * ui->piece_box_open));
					ui->holding = NULL;
					gtk_widget_queue_draw(w);
				}
			}
			return;
		}

		min = 30.0;
		p = find_closest_piece(p1, ui->mousex, ui->mousey, &min); 
		px = find_closest_piece(p2, ui->mousex, ui->mousey, &min); 
		if (px)
			p = px;
		if (p != NULL)
			ui->holding = p;
		gtk_widget_queue_draw(ui->drawing_area);
	}
	return TRUE;
}

static void init_ui(int *argc, char **argv[], struct gui *ui)
{
	GtkWidget *vbox, *scrolled_window;

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

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(ui->window), vbox);
        ui->drawing_area = gtk_drawing_area_new();
	ui->xdim = 300;
	ui->ydim = 300;
        gtk_widget_set_size_request(GTK_WIDGET(ui->drawing_area), ui->xdim, ui->ydim);
        gtk_widget_modify_bg(ui->drawing_area, GTK_STATE_NORMAL, &black);
        gtk_container_add(GTK_CONTAINER(vbox), ui->drawing_area);
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
	ui->inverted = 0;
	ui->piece_box_open = 1;
	ui->holding = NULL;
	gtk_widget_show_all(ui->window);
}

void generic_draw_piece_on_board(struct piece *p, cairo_t *cr)
{
	double radius;

	if (ui.xdim / ui.piece_box_open < ui.ydim)
		radius = (0.75 * ui.xdim / (12.0 * ui.piece_box_open)) / 2.0;
	else
		radius = (0.75 * ui.ydim / 12.0) / 2.0;

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
	cairo_set_source_rgba(cr, p->r, p->g, p->b, 1.0);    // partially translucent
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
