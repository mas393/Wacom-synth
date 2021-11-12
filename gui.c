#include <gtk/gtk.h>
#include "make_sound.h"
/*
 * GTK graph sample https://github.com/liberforce/gtk-samples/blob/master/c/gtk3-graph/main.c
 * Use GdkPixbuf new_from_file_at_size to set background of cairo with cairo_set_source_pixbuf in drawing area
 */

//TODO: draw dot prior to make_sound call
//TODO: send paramater describing button to make_sound
//TODO: implement wacom support

static cairo_surface_t *surface = NULL;

static void
clear_surface (void)
{
  cairo_t *cr;

  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_destroy (cr); 
}

static void
resize_cb (GtkWidget *widget,
	   int width,
	   int height,
	   gpointer data)
{
  if (surface)
    {
      cairo_surface_destroy (surface);
      surface = NULL;
    }

  if (gtk_native_get_surface (gtk_widget_get_native (widget)))
    {
      surface = gdk_surface_create_similar_surface (gtk_native_get_surface (gtk_widget_get_native (widget)),
						    CAIRO_CONTENT_COLOR,
						    gtk_widget_get_width (widget),
						    gtk_widget_get_height (widget));

      clear_surface();			    
    }
}

static void
draw_cb (GtkDrawingArea *drawing_area,
	 cairo_t *cr,
	 int width,
	 int height,
	 gpointer data)
{
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);
}

static void
draw_brush (GtkWidget *widget,
	    double x,
	    double y)
{
  cairo_t *cr;

  cr = cairo_create (surface);

  cairo_rectangle (cr, x - 3, y - 3, 6 ,6);
  cairo_fill (cr);

  cairo_destroy (cr);

  gtk_widget_queue_draw(widget);
}

//static double start_x;
//static double start_y;

static void
drag_begin (GtkGestureDrag *guesture,
	    double x,
	    double y,
	    GtkWidget *area)
{
  //start_x = x;
  //start_y = y;
  draw_brush (area, x, y);
  printf("x: %f y: %f\n", x, y);
  make_sound(200., 200., x, y);
  /* Need to:
     translate x and y coordinates to our bounds for freq and whatever the y axis is
     pass these translated coords into our python functions (or c translations of them)
   */
  

    
}

static void
close_window(void){
  if (surface)
    {
      cairo_surface_destroy (surface);
    }
}

static void
activate (GtkApplication *app,
	  gpointer user_data)
{
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *drawing_area;
  GtkGesture *drag;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Wacom Synth");

  g_signal_connect (window, "destroy", G_CALLBACK (close_window), NULL);
  
  grid = gtk_grid_new ();
  gtk_window_set_child (GTK_WINDOW (window), grid);
  
  //button = gtk_button_new_with_label ("button1");
  //  g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);
  //gtk_grid_attach (GTK_GRID (grid), button, 0, 0, 1, 1);
  
  //button = gtk_button_new_with_label ("button2");
  //  g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);
  //gtk_grid_attach (GTK_GRID (grid), button, 1, 0, 1, 1);

  frame = gtk_frame_new (NULL);
  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, 200, 200);
  
  gtk_frame_set_child (GTK_FRAME (frame), drawing_area);

  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (drawing_area), draw_cb, NULL, NULL);

  g_signal_connect_after (drawing_area, "resize", G_CALLBACK (resize_cb), NULL);

  //perhaps the button is delayed in appearing because it is a drag gesture
  drag = gtk_gesture_drag_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (drag), GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller (drawing_area, GTK_EVENT_CONTROLLER (drag));
  g_signal_connect (drag, "drag-begin", G_CALLBACK (drag_begin), drawing_area);
  
  gtk_grid_attach (GTK_GRID (grid), frame, 0, 1, 2, 1);
  gtk_widget_show (window);
}

int main(int argc, char **argv)
{

  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  
  return status;
}

