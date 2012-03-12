#include <gtk/gtk.h>
#include <cdaudio.h>
#include <pixmaps/background.xpm>
#include <pixmaps/sizemaps.xpm>
#include <pixmaps/max_mask.xpm>

extern gint cd_fd, window_mode, checked_coverart, cddb_info_displayed, play_track, cddb_lamp, coverart_lamp;
extern GtkWidget *window;
extern GdkPixmap *window_bg;
extern GdkBitmap *window_mk;

/* XXX Could someone please help me figure out how to do this without using a pixmap? */
void
max_alter_mask(void)
{
   GdkGC *gc;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   GdkColor color;
   
   gc = gdk_gc_new(window->window);
   
   color.pixel = 1;
   gdk_gc_set_foreground(gc, &color);
   
   pixmap = gdk_pixmap_create_from_xpm_d(window->window, &mask, 0, max_mask_xpm);
   gdk_pixmap_unref(pixmap);
   
   /*
   mask = gdk_pixmap_new(NULL, 396, 213, 1);
   gdk_draw_pixmap(mask, gc, window_mk, 0, 0, 0, 0, 396, 200);
   gdk_draw_rectangle(mask, gc, TRUE, 0, 0, 396, 213);*/
   
   gtk_widget_shape_combine_mask(window, mask, 0, 0);
   gdk_pixmap_unref(mask);
   
   gdk_gc_destroy(gc);
}

void
max_draw_window(void)
{
   GdkPixmap *sizemap_pm, *new_window_bg;
   GdkBitmap *sizemap_bm;
   GdkGC *gc;   
   struct disc_status status;
   
   cd_poll(cd_fd, &status);
   
   gc = gdk_gc_new(window->window);
   
   gtk_widget_set_usize(window, 396, 213);
   max_alter_mask();
   
   sizemap_pm = gdk_pixmap_create_from_xpm_d(window->window, &sizemap_bm, 0, sizemaps_xpm);
   new_window_bg = gdk_pixmap_new(window->window, 396, 213, gdk_visual_get_best_depth());
   gdk_draw_pixmap(new_window_bg, gc, window_bg, 0, 0, 0, 0, 396, 200);
   gdk_draw_pixmap(new_window_bg, gc, sizemap_pm, 0, 0, 0, 200, 396, 13);
   gdk_draw_pixmap(new_window_bg, gc, sizemap_pm, 396, 0, 0, 178, 24, 22);
   
   gdk_window_set_back_pixmap(window->window, new_window_bg, 0);
   gdk_pixmap_unref(window_bg);
   window_bg = new_window_bg;   
   update_disc_status(status.status_mode);
  
   gdk_pixmap_unref(sizemap_pm);
   gdk_pixmap_unref(sizemap_bm);
   gdk_gc_destroy(gc);
   
   if(status.status_present)
     update_track_counter(play_track);
   change_cddb_lamp(cddb_lamp);
   change_coverart_lamp(coverart_lamp);
   
   checked_coverart = 0;
   cddb_info_displayed = 0;
   window_mode = 2;
   
   set_conf_value("Window-Size", "Max");
}

void
med_draw_window(void)
{
   GdkPixmap *new_window_bg, *new_window_bm;
   GtkStyle *style;
   GdkGC *gc;
   struct disc_status status;
   
   cd_poll(cd_fd, &status);
   
   style = gtk_widget_get_style(window);
   gc = gdk_gc_new(window->window);
   
   gtk_widget_set_usize(window, 396, 200);
   
   new_window_bg = gdk_pixmap_create_from_xpm_d(window->window, &new_window_bm, &style->bg[GTK_STATE_NORMAL], background_xpm);
   gtk_widget_shape_combine_mask(window, new_window_bm, 0, 0);
   
   gdk_window_set_back_pixmap(window->window, new_window_bg, 0);
   gdk_pixmap_unref(window_bg);
   window_bg = new_window_bg;
   
   zero_counters();
   place_buttons();
   update_disc_status(status.status_mode);
   
   gdk_pixmap_unref(new_window_bm);
   gdk_gc_destroy(gc);
   
   if(status.status_present)
     update_track_counter(play_track);
   change_cddb_lamp(cddb_lamp);
   change_coverart_lamp(coverart_lamp);
   
   checked_coverart = 0;
   cddb_info_displayed = 0;
   window_mode = 1;
   
   set_conf_value("Window-Size", "Med");
}

void
enlarge_release(void)
{
   if(window_mode == 1)
     max_draw_window();
}

void
shrink_release(void)
{
   if(window_mode == 2)
     med_draw_window();
}
