#include <gtk/gtk.h>
#include <pixmaps/numbers.xpm>

#include "numbers.h"

extern struct gdcd_number large_number[10], small_number[10];
extern GdkPixmap *large_colon, *small_colon, *large_minus, *small_minus;

void
make_numbers(GtkWidget *window)
{
   gint index;
   GdkPixmap *numbers_pm;
   GdkBitmap *numbers_bm;
   GdkGC *gc;
   GtkStyle *style;

   style = gtk_widget_get_style(window);
   gc = gdk_gc_new(window->window);

   numbers_pm = gdk_pixmap_create_from_xpm_d(window->window, &numbers_bm, &style->bg[GTK_STATE_NORMAL], numbers_xpm);

   for(index = 0; index < 10; index++) {
      large_number[index].pixmap = gdk_pixmap_new(window->window, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE, gdk_visual_get_best_depth());
      gdk_draw_pixmap(large_number[index].pixmap, gc, numbers_pm, index * LARGE_NUMBER_XSIZE, 0, 0, 0, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   }

   large_colon = gdk_pixmap_new(window->window, LARGE_COLON_XSIZE, LARGE_NUMBER_YSIZE, gdk_visual_get_best_depth());
   gdk_draw_pixmap(large_colon, gc, numbers_pm, 10 * LARGE_NUMBER_XSIZE, 0, 0, 0, LARGE_COLON_XSIZE, LARGE_NUMBER_YSIZE);

   large_minus = gdk_pixmap_new(window->window, 17, 3, gdk_visual_get_best_depth());
   gdk_draw_pixmap(large_minus, gc, numbers_pm, 253, 46, 0, 0, 17, 3);
   
   for(index = 0; index < 10; index++) {
      small_number[index].pixmap = gdk_pixmap_new(window->window, SMALL_NUMBER_XSIZE, SMALL_NUMBER_YSIZE, gdk_visual_get_best_depth());
      gdk_draw_pixmap(small_number[index].pixmap, gc, numbers_pm, index * SMALL_NUMBER_XSIZE, LARGE_NUMBER_YSIZE, 0, 0, SMALL_NUMBER_XSIZE, SMALL_NUMBER_YSIZE);
   }

   small_colon = gdk_pixmap_new(window->window, SMALL_COLON_XSIZE, SMALL_NUMBER_YSIZE, gdk_visual_get_best_depth());
   gdk_draw_pixmap(small_colon, gc, numbers_pm, 10 * SMALL_NUMBER_XSIZE, LARGE_NUMBER_YSIZE, 0, 0, SMALL_COLON_XSIZE, SMALL_NUMBER_YSIZE);

   small_minus = gdk_pixmap_new(window->window, 17, 3, gdk_visual_get_best_depth());
   gdk_draw_pixmap(small_minus, gc, numbers_pm, 253, 49, 0, 0, 17, 3);
   
   gdk_gc_destroy(gc);
   gdk_pixmap_unref(numbers_pm);
   gdk_bitmap_unref(numbers_bm);
}
