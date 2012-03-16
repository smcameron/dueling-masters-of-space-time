#include <stdlib.h>

#include <gtk/gtk.h>
#include <cairo.h>

struct gui {
	GtkWidget *window;
	GtkWidget *drawing_area;
	int xdim, ydim;
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

static int on_expose_drawing_area(GtkWidget *w, GdkEvent *event, gpointer p)
{
	struct gui *ui = p;
	cairo_t *cr;
	static cairo_surface_t *galaxy_image = NULL;
	static int xd, yd;
	static double dash[] = { 1.0, 2.0 };
	int i;

	if (galaxy_image == NULL) {
		galaxy_image = cairo_image_surface_create_from_png("pinwheel.png");
		xd = cairo_image_surface_get_width(galaxy_image);
		yd = cairo_image_surface_get_height(galaxy_image);
	}

	cr = gdk_cairo_create(w->window);

	cairo_save(cr);
	cairo_scale (cr, (double) ui->xdim / (double) xd, (double) ui->ydim / (double) yd);

	cairo_set_source_surface (cr, galaxy_image, 0, 0);
	cairo_paint(cr);
	cairo_restore(cr);

	cairo_set_dash(cr, dash, 2, 2.0);
	// cairo_set_line_width(cr, 0.5);
	cairo_set_source_rgb (cr, 0.5, 1, 0.5);
	for (i = 1; i <= 11; i++) {
		cairo_move_to(cr, ui->xdim / 12.0 * i, ui->ydim / 12.0);
		cairo_line_to(cr, ui->xdim / 12.0 * i, 11.0 * ui->ydim / 12.0);
		cairo_move_to(cr, ui->xdim / 12.0, i * ui->ydim / 12.0);
		cairo_line_to(cr, 11.0 * ui->xdim / 12.0, i *ui->ydim / 12.0);
	}
	for (i = 0; i < 10; i++) {
		char letter[2];
		sprintf(letter, "%c", 'A' + i);
		draw_centered_text(cr, ui->xdim / 24.0, (i + 1.5) * ui->ydim / 12.0, 30.0, letter);
	}
	for (i = 0; i < 10; i++) {
		char number[2];
		sprintf(number, "%d", i + 1);
		draw_centered_text(cr, (i + 1.5) * ui->xdim / 12.0, ui->ydim / 24.0, 30.0, number);
	}
	cairo_stroke(cr);
	cairo_destroy(cr);
}

static gint on_config_drawing_area(GtkWidget *w, GdkEventConfigure *event,
                                       gpointer p)
{
	struct gui *ui = p;

	ui->xdim = w->allocation.width;
	ui->ydim = w->allocation.height;
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

	gtk_widget_show_all(ui->window);
}

int main(int argc, char *argv[])
{
	init_ui(&argc, &argv, &ui);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
}
