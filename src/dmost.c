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
