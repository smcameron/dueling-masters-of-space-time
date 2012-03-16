#include <stdlib.h>

#include <gtk/gtk.h>
#include <cairo.h>

struct gui {
	GtkWidget *window;
	GtkWidget *drawing_area;
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
}

static gint on_config_drawing_area(GtkWidget *w, GdkEventConfigure *event,
                                       gpointer data)
{
        /* use w->allocation.width, w->allocation.height to get new size */
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
        gtk_widget_set_size_request(GTK_WIDGET(ui->drawing_area), 700, 300);
        gtk_widget_modify_bg(ui->drawing_area, GTK_STATE_NORMAL, &black);
        gtk_container_add(GTK_CONTAINER(vbox), ui->drawing_area);
        g_signal_connect(G_OBJECT(ui->drawing_area), "expose_event",
			G_CALLBACK(on_expose_drawing_area), NULL);
        g_signal_connect(G_OBJECT(ui->drawing_area), "configure_event",
		G_CALLBACK(on_config_drawing_area), NULL);

	gtk_widget_show_all(ui->window);
}

int main(int argc, char *argv[])
{
	init_ui(&argc, &argv, &ui);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
}
