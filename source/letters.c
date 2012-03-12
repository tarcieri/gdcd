#include <gtk/gtk.h>
#include <pixmaps/letters.xpm>

#include "letters.h"

extern struct gdcd_letter letter[256];

void
make_letters(GtkWidget *window)
{
   gchar index, *misc_chars = "*.:-(),'?/\\[]^&%=$!_+#";
   gint x, num_misc_chars = 22;
   GdkPixmap *letters_pm;
   GdkBitmap *letters_bm;
   GdkGC *gc;
   GtkStyle *style;
   GdkColor pattern;
   
   style = gtk_widget_get_style(window);
   gc = gdk_gc_new(window->window);
   
   letters_pm = gdk_pixmap_create_from_xpm_d(window->window, &letters_bm, &style->bg[GTK_STATE_NORMAL], letters_xpm);
   
   for(x = 0; x < 256; x++)
     letter[index].pixmap = NULL;
   
   x = 0;
   for(index = 'A'; index <= 'Z'; index++) {
      letter[index].pixmap = gdk_pixmap_new(window->window, LETTER_XSIZE, LETTER_YSIZE, gdk_visual_get_best_depth());
      gdk_draw_pixmap(letter[index].pixmap, gc, letters_pm, x++ * LETTER_XSIZE, 0, 0, 0, LETTER_XSIZE, LETTER_YSIZE);
   }
   
   for(index = 'a'; index <= 'z'; index++) {
      letter[index].pixmap = gdk_pixmap_new(window->window, LETTER_XSIZE, LETTER_YSIZE, gdk_visual_get_best_depth());
      gdk_draw_pixmap(letter[index].pixmap, gc, letters_pm, x++ * LETTER_XSIZE,
0, 0, 0, LETTER_XSIZE, LETTER_YSIZE);
   }
     
   for(index = '0'; index <= '9'; index++) {
      letter[index].pixmap = gdk_pixmap_new(window->window, LETTER_XSIZE, LETTER_YSIZE, gdk_visual_get_best_depth());
      gdk_draw_pixmap(letter[index].pixmap, gc, letters_pm, x++ * LETTER_XSIZE,
0, 0, 0, LETTER_XSIZE, LETTER_YSIZE);
   }

   for(index = 0; index < num_misc_chars; index++) {
      letter[misc_chars[index]].pixmap = gdk_pixmap_new(window->window, LETTER_XSIZE, LETTER_YSIZE, gdk_visual_get_best_depth());
      gdk_draw_pixmap(letter[misc_chars[index]].pixmap, gc, letters_pm, x++ * LETTER_XSIZE,
0, 0, 0, LETTER_XSIZE, LETTER_YSIZE);
   }
   
   pattern.pixel = 0;
   gdk_gc_set_foreground(gc, &pattern);
   
   letter[' '].pixmap = gdk_pixmap_new(window->window, LETTER_XSIZE, LETTER_YSIZE, gdk_visual_get_best_depth());
   gdk_draw_rectangle(letter[' '].pixmap, gc, TRUE, 0, 0, LETTER_XSIZE, LETTER_YSIZE); 
   
   gdk_gc_destroy(gc);
}
