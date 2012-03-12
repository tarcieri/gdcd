#include <gtk/gtk.h>
#include <cdaudio.h>
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pixmaps/gdcd.xpm>
#include <pixmaps/background.xpm>
#include <pixmaps/status.xpm>
#include <pixmaps/netops.xpm>

#include <config.h>

#include "numbers.h"
#include "buttons.h"
#include "letters.h"
#include "coverart.h"
#include "conf.h"

GtkWidget *window;
GdkPixmap *window_bg, *splash_pm;
GdkPixmap *play_ico, *pause_ico, *stop_ico, *error_ico, *null_ico;
GdkPixmap *light[5];
GdkBitmap *window_mk;

gboolean window_moving = FALSE;
gint window_x, window_y;

struct gdcd_button control_button[24];
struct gdcd_number large_number[10], small_number[10];
struct gdcd_letter letter[256];
struct disc_info disc;
struct disc_data data;
GdkPixmap *large_colon, *small_colon, *large_minus, *small_minus;

gint cd_fd = -1;
gint play_track = 1, disc_present = 0, disc_playing = 0;
gint old_play_track = 1, old_status = CDAUDIO_NOSTATUS;
gint window_mode = 1;

gint scroller_active[2];
gint scroller_x[2];
gint scroller_xsize[2];
gchar scroller_message[2][256];
GdkPixmap *scroller_pm[2];
gint statusbar_timer = -1;

pthread_t cddb_tid, coverart_tid;
gint cddb_lamp = 2, coverart_lamp = 2;
gint cddb_sock = -1, coverart_sock = -1;
gint cddb_tag = 0, coverart_tag = 0;
gint checked_cddb = 0, checked_coverart = 0;
gint disable_cddb = 0, disable_coverart = 0;
gint cddb_depressed = 0, coverart_depressed = 0;
gint valid_cddb = 0, valid_coverart = 0, cddb_info_displayed = 0;
gint cddb_thread_active = 0, coverart_thread_active = 0;

void
button_depress(struct gdcd_button *button)
{
   GdkGC *gc;
   GdkColor color;
   
   button->depressed = 1;
   
   gc = gdk_gc_new(window->window);
   color.pixel = 0;
   gdk_gc_set_foreground(gc, &color);
   
   gdk_draw_rectangle(window_bg, gc, TRUE, button->x, button->y, button->w + 1, button->h + 1);   
   gdk_draw_pixmap(window_bg, gc, button->image, 0, 0, button->x + 1, button->y + 1, button->w, button->h);
   
   gdk_gc_destroy(gc);
   
   if(button->press_func != NULL)
     button->press_func();
   
   gdk_window_clear_area(window->window, button->x, button->y, button->w + 1, button->h + 1);
}

void
button_release(struct gdcd_button *button)
{
   GdkGC *gc;
   GdkColor color;
   
   button->depressed = 0;

   gc = gdk_gc_new(window->window);
   color.pixel = 0;
   gdk_gc_set_foreground(gc, &color);

   gdk_draw_rectangle(window_bg, gc, TRUE, button->x, button->y, button->w + 1, button->h + 1);
   gdk_draw_pixmap(window_bg, gc, button->image, 0, 0, button->x, button->y, button->w, button->h);

   gdk_gc_destroy(gc);

   if(button->release_func != NULL)
     button->release_func();
   
   gdk_window_clear_area(window->window, button->x, button->y, button->w + 1, button->h + 1);
}

void destroy(GtkWidget * widget, gpointer data)
{
   gtk_main_quit();
}

void window_press_event(GtkWidget * widget, GdkEventButton * event, gpointer callback_data)
{
   gint index, ex, ey;

   gdk_window_raise(window->window);
   
   ex = event->x;
   ey = event->y;
   
   if(event->button == 1) {
      for(index = 0; index < 24; index++) {
	 if(control_button[index].image == NULL)
	   break;
	 
	 if(check_button(&control_button[index], ex, ey)) {
	    button_depress(&control_button[index]);
	    return;
	 }
      }
      
      window_moving = TRUE;
      window_x = event->x;
      window_y = event->y;
   }
}

void window_release_event(GtkWidget * widget, GdkEventButton * event, gpointer callback_data)
{
   gint index, ex, ey;
   
   ex = event->x;
   ey = event->y;
   
   if(window_moving)
     window_moving = FALSE;
   
   if(event->button == 1) {
      for(index = 0; index < 24; index++) {
	 if(control_button[index].image == NULL)
	   break;
	 
	 if(control_button[index].depressed) {
	    button_release(&control_button[index]);
	    return;
	 }
      }
   }
}

void window_motion_event(GtkWidget * widget, GdkEventMotion * event, gpointer callback_data)
{
   GdkModifierType modmask;
   gint i = 0, x, y, nx, ny;
   
   if(window_moving) {
      gdk_window_get_pointer(NULL, &x, &y, &modmask);
      nx = x - window_x;
      ny = y - window_y;
      
      gdk_window_move(window->window, nx, ny);
   }
   
   gdk_flush();
}

void
load_pixmaps(void)
{
   int index;
   
   GtkStyle *style;
   GdkGC *gc;
   GdkPixmap *temp;
   GdkBitmap *mask;
   
   gc = gdk_gc_new(window->window);
   style = gtk_widget_get_style(window);
   
   splash_pm = gdk_pixmap_create_from_xpm_d(window->window, &mask, &style->bg[GTK_STATE_NORMAL], gdcd_xpm);
   
   temp = gdk_pixmap_create_from_xpm_d(window->window, &mask, &style->bg[GTK_STATE_NORMAL], status_xpm);
   play_ico = gdk_pixmap_new(window->window, 12, 12, gdk_visual_get_best_depth());
   stop_ico = gdk_pixmap_new(window->window, 12, 12, gdk_visual_get_best_depth());
   pause_ico = gdk_pixmap_new(window->window, 12, 12, gdk_visual_get_best_depth());
   error_ico = gdk_pixmap_new(window->window, 12, 12, gdk_visual_get_best_depth());
   null_ico = gdk_pixmap_new(window->window, 12, 12, gdk_visual_get_best_depth());
   gdk_draw_pixmap(play_ico, gc, temp, 0, 0, 0, 0, 12, 12);
   gdk_draw_pixmap(stop_ico, gc, temp, 12, 0, 0, 0, 12, 12);
   gdk_draw_pixmap(pause_ico, gc, temp, 24, 0, 0, 0, 12, 12);
   gdk_draw_pixmap(error_ico, gc, temp, 36, 0, 0, 0, 12, 12);
   gdk_draw_pixmap(null_ico, gc, temp, 48, 0, 0, 0, 12, 12);
   gdk_pixmap_unref(temp);
   
   temp = gdk_pixmap_create_from_xpm_d(window->window, &mask, &style->bg[GTK_STATE_NORMAL], netops_xpm);
   for(index = 0; index < 5; index++) {
      light[index] = gdk_pixmap_new(window->window, 7, 9, gdk_visual_get_best_depth());
      gdk_draw_pixmap(light[index], gc, temp, index * 7, 15, 0, 0, 7, 9);
   }
   
   gdk_gc_destroy(gc);
}

void
display_cover(GdkPixmap *pm)
{
   GdkGC *gc;
   
   gc = gdk_gc_new(window->window);
   gdk_draw_pixmap(window_bg, gc, pm, 0, 0, 15, 15, 170, 170);
   gdk_gc_destroy(gc);
}

void
zero_counters(void)
{
   GdkGC *gc;
   
   gc = gdk_gc_new(window->window);   
   gdk_draw_pixmap(window_bg, gc, large_number[0].pixmap, 0, 0, 200, 12, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, large_number[0].pixmap, 0, 0, 200 + LARGE_NUMBER_XSIZE, 12, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   
   gdk_draw_pixmap(window_bg, gc, large_number[0].pixmap, 0, 0, 390 - LARGE_NUMBER_XSIZE * 4 - LARGE_COLON_XSIZE, 12, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, large_number[0].pixmap, 0, 0, 390 - LARGE_NUMBER_XSIZE * 3 - LARGE_COLON_XSIZE, 12, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, large_colon, 0, 0, 390 - LARGE_NUMBER_XSIZE * 2 - LARGE_COLON_XSIZE, 12, LARGE_COLON_XSIZE, LARGE_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, large_number[0].pixmap, 0, 0, 390 - LARGE_NUMBER_XSIZE * 2, 12, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, large_number[0].pixmap, 0, 0, 390 - LARGE_NUMBER_XSIZE, 12, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   
   gdk_draw_pixmap(window_bg, gc, small_number[0].pixmap, 0, 0, 390 - SMALL_NUMBER_XSIZE * 4 - SMALL_COLON_XSIZE, 12 + LARGE_NUMBER_YSIZE, SMALL_NUMBER_XSIZE, SMALL_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, small_number[0].pixmap, 0, 0, 390 - SMALL_NUMBER_XSIZE * 3 - SMALL_COLON_XSIZE, 12 + LARGE_NUMBER_YSIZE, SMALL_NUMBER_XSIZE, SMALL_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, small_colon, 0, 0, 390 - SMALL_NUMBER_XSIZE * 2 - SMALL_COLON_XSIZE, 12 + LARGE_NUMBER_YSIZE, SMALL_COLON_XSIZE, SMALL_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, small_number[0].pixmap, 0, 0, 390 - SMALL_NUMBER_XSIZE * 2, 12 + LARGE_NUMBER_YSIZE, SMALL_NUMBER_XSIZE, SMALL_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, small_number[0].pixmap, 0, 0, 390 - SMALL_NUMBER_XSIZE, 12 + LARGE_NUMBER_YSIZE, SMALL_NUMBER_XSIZE, SMALL_NUMBER_YSIZE); 
   gdk_gc_destroy(gc);
}

void
update_track_counter(gint track)
{
   GdkGC *gc;
   gchar buf[3];
   
   snprintf(buf, 3, "%02d", track);
   
   gc = gdk_gc_new(window->window);
   gdk_draw_pixmap(window_bg, gc, large_number[buf[0] - 48].pixmap, 0, 0, 200, 12, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, large_number[buf[1] - 48].pixmap, 0, 0, 200 + LARGE_NUMBER_XSIZE, 12, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   gdk_gc_destroy(gc);
}

void
display_large_minus(int toggle)
{
   GdkGC *gc;
   GdkColor color;
   
   gc = gdk_gc_new(window->window);
   
   color.pixel = 0;
   gdk_gc_set_foreground(gc, &color);
   
   if(toggle)
     gdk_draw_pixmap(window_bg, gc, large_minus, 0, 0, 257, 25, 17, 3);
   else
     gdk_draw_rectangle(window_bg, gc, TRUE, 257, 25, 17, 3);
   gdk_gc_destroy(gc);
}

void
display_small_minus(int toggle)
{
   GdkGC *gc;
   GdkColor color;
  
   gc = gdk_gc_new(window->window);
   
   color.pixel = 0;
   gdk_gc_set_foreground(gc, &color);
   
   if(toggle)
     gdk_draw_pixmap(window_bg, gc, small_minus, 0, 0, 293, 50, 12, 3);
   else
     gdk_draw_rectangle(window_bg, gc, TRUE, 293, 50, 12, 3);
   
   gdk_gc_destroy(gc);
}

void
update_track_time(struct disc_timeval *time)
{
   GdkGC *gc;
   gchar buf[4];

   gc = gdk_gc_new(window->window);

   if(time->seconds < 0)
     display_large_minus(1);
   else
     display_large_minus(0);
   
   snprintf(buf, 4, "%02d", abs(time->minutes));
   
   gdk_draw_pixmap(window_bg, gc, large_number[buf[0] - 48].pixmap, 0, 0, 390 - LARGE_NUMBER_XSIZE * 4 - LARGE_COLON_XSIZE, 12, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, large_number[buf[1] - 48].pixmap, 0, 0, 390 - LARGE_NUMBER_XSIZE * 3 - LARGE_COLON_XSIZE, 12, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, large_colon, 0, 0, 390 - LARGE_NUMBER_XSIZE * 2 - LARGE_COLON_XSIZE, 12, LARGE_COLON_XSIZE, LARGE_NUMBER_YSIZE);

   snprintf(buf, 3, "%02d", abs(time->seconds));
   gdk_draw_pixmap(window_bg, gc, large_number[buf[0] - 48].pixmap, 0, 0, 390 - LARGE_NUMBER_XSIZE * 2, 12, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, large_number[buf[1] - 48].pixmap, 0, 0, 390 - LARGE_NUMBER_XSIZE, 12, LARGE_NUMBER_XSIZE, LARGE_NUMBER_YSIZE);
   gdk_gc_destroy(gc);
}

void
update_disc_time(struct disc_timeval *time)
{
   GdkGC *gc;
   gchar buf[3];

   gc = gdk_gc_new(window->window);

   if(time->seconds < 0)
     display_small_minus(1);
   else
     display_small_minus(0);
   
   snprintf(buf, 3, "%02d", abs(time->minutes));
   gdk_draw_pixmap(window_bg, gc, small_number[buf[0] - 48].pixmap, 0, 0, 390 - SMALL_NUMBER_XSIZE * 4 - SMALL_COLON_XSIZE, 12 + LARGE_NUMBER_YSIZE, SMALL_NUMBER_XSIZE, SMALL_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, small_number[buf[1] - 48].pixmap, 0, 0, 390 - SMALL_NUMBER_XSIZE * 3 - SMALL_COLON_XSIZE, 12 + LARGE_NUMBER_YSIZE, SMALL_NUMBER_XSIZE, SMALL_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, small_colon, 0, 0, 390 - SMALL_NUMBER_XSIZE * 2 - SMALL_COLON_XSIZE, 12 + LARGE_NUMBER_YSIZE, SMALL_COLON_XSIZE, SMALL_NUMBER_YSIZE);

   snprintf(buf, 3, "%02d", abs(time->seconds));
   gdk_draw_pixmap(window_bg, gc, small_number[buf[0] - 48].pixmap, 0, 0, 390 - SMALL_NUMBER_XSIZE * 2, 12 + LARGE_NUMBER_YSIZE, SMALL_NUMBER_XSIZE, SMALL_NUMBER_YSIZE);
   gdk_draw_pixmap(window_bg, gc, small_number[buf[1] - 48].pixmap, 0, 0, 390 - SMALL_NUMBER_XSIZE, 12 + LARGE_NUMBER_YSIZE, SMALL_NUMBER_XSIZE, SMALL_NUMBER_YSIZE);
   gdk_gc_destroy(gc);
}

void
update_disc_status(int status)
{
   GdkGC *gc;
   gc = gdk_gc_new(window->window);
   
   switch(status) {
    case -1:
      gdk_draw_pixmap(window_bg, gc, null_ico, 0, 0, 200, 50, 12, 12);
      break;
    case CDAUDIO_INVALID:
    case CDAUDIO_ERROR:
      gdk_draw_pixmap(window_bg, gc, error_ico, 0, 0, 200, 50, 12, 12);
      break;
    case CDAUDIO_PLAYING:
      gdk_draw_pixmap(window_bg, gc, play_ico, 0, 0, 200, 50, 12, 12);
      break;
    case CDAUDIO_PAUSED:
      gdk_draw_pixmap(window_bg, gc, pause_ico, 0, 0, 200, 50, 12, 12);
      break;
    default:
      gdk_draw_pixmap(window_bg, gc, stop_ico, 0, 0, 200, 50, 12, 12);
   }
   
   gdk_gc_destroy(gc);
}

void
place_button(struct gdcd_button *button, gint x, gint y, gint width, gint height)
{
   GdkGC *gc;
   
   gc = gdk_gc_new(window->window);
   gdk_draw_pixmap(window_bg, gc, button->image, 0, 0, x, y, width, height);
   gdk_gc_destroy(gc);
   
   button->x = x;
   button->y = y;
   button->w = width;
   button->h = height;
}

void
place_buttons(void)
{
   GdkGC *gc;
   struct gdcd_button *button;
   
   gc = gdk_gc_new(window->window);
   
   button = get_button(gdcdPlayButton);
   place_button(button, 200, 200 - (BUTTON_HEIGHT + 1) * 2, BUTTON_WIDTH, BUTTON_HEIGHT);
 
   button = get_button(gdcdEjectButton);
   place_button(button, 200 + (BUTTON_WIDTH + 1), 200 - (BUTTON_HEIGHT + 1) * 2, BUTTON_WIDTH, BUTTON_HEIGHT);

   button = get_button(gdcdCloseButton);
   place_button(button, 200 + (BUTTON_WIDTH + 1) * 2, 200 - (BUTTON_HEIGHT + 1) * 2, BUTTON_WIDTH, BUTTON_HEIGHT);
  
   button = get_button(gdcdStopButton);
   place_button(button, 200 + (BUTTON_WIDTH + 1) * 3, 200 - (BUTTON_HEIGHT + 1) * 2, BUTTON_WIDTH, BUTTON_HEIGHT);
  
   button = get_button(gdcdPrevButton);
   place_button(button, 200, 200 - (BUTTON_HEIGHT + 1), BUTTON_WIDTH, BUTTON_HEIGHT);

   button = get_button(gdcdRewButton);
   place_button(button, 200 + (BUTTON_WIDTH + 1), 200 - (BUTTON_HEIGHT + 1), BUTTON_WIDTH, BUTTON_HEIGHT);

   button = get_button(gdcdFFButton);
   place_button(button, 200 + (BUTTON_WIDTH + 1) * 2, 200 - (BUTTON_HEIGHT + 1), BUTTON_WIDTH, BUTTON_HEIGHT);

   button = get_button(gdcdAdvButton);
   place_button(button, 200 + (BUTTON_WIDTH + 1) * 3, 200 - (BUTTON_HEIGHT + 1), BUTTON_WIDTH, BUTTON_HEIGHT);
   
   gdk_draw_rectangle(window_bg, gc, TRUE, 342, 70, 48, 17);
   
   button = get_button(gdcdLittleButton);
   place_button(button, 342, 70, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
   
   button = get_button(gdcdBigButton);
   place_button(button, 358, 70, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
   
   button = get_button(gdcdKillButton);
   place_button(button, 374, 70, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
   
   gdk_draw_rectangle(window_bg, gc, TRUE, 200, 70, 122, 16);
   
   button = get_button(gdcdCDDBButton);
   place_button(button, 200, 70, 60, 15);
   
   button = get_button(gdcdCoverArtButton);
   place_button(button, 261, 70, 60, 15);
   
   gdk_gc_destroy(gc);
}

void
change_cddb_lamp(int value)
{
   GdkGC *gc;
   
   gc = gdk_gc_new(window->window);
   if(!cddb_depressed)
     gdk_draw_pixmap(window_bg, gc, light[value], 0, 0, 203, 73, 7, 9);
   gdk_gc_destroy(gc);
}

void
change_coverart_lamp(int value)
{
   GdkGC *gc;
   
   gc = gdk_gc_new(window->window);
   if(!coverart_depressed)
     gdk_draw_pixmap(window_bg, gc, light[value], 0, 0, 264, 73, 7, 9);
   gdk_gc_destroy(gc);
}

GdkPixmap
*make_pixmap_message(gchar *message)
{
   gint index;
   GdkPixmap *pixmap;
   GdkGC *gc;
   GdkColor pattern;
   
   gc = gdk_gc_new(window->window);
   pixmap = gdk_pixmap_new(window->window, (LETTER_XSIZE + 1) * strlen(message), LETTER_YSIZE, gdk_visual_get_best_depth());
   
   pattern.pixel = 0;
   gdk_gc_set_foreground(gc, &pattern);
   gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, (LETTER_XSIZE + 1) * strlen(message), LETTER_YSIZE);
   
   for(index = 0; index < strlen(message); index++)
     if(letter[message[index]].pixmap != NULL)
      gdk_draw_pixmap(pixmap, gc, letter[message[index]].pixmap, 0, 0, (LETTER_XSIZE + 1) * index, 0, LETTER_XSIZE, LETTER_YSIZE);

   return pixmap;
}

GdkPixmap
*make_scroller_pixmap_message(gchar *message)
{
   gint index;
   GdkPixmap *pixmap;
   GdkGC *gc;
   GdkColor pattern;
   
   gc = gdk_gc_new(window->window);
   pixmap = gdk_pixmap_new(window->window, (LETTER_XSIZE + 1) * (strlen(message) * 2 + 5), LETTER_YSIZE, gdk_visual_get_best_depth());
   
   pattern.pixel = 0;
   gdk_gc_set_foreground(gc, &pattern);
   gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, (LETTER_XSIZE + 1) * (strlen(message) * 2 + 5), LETTER_YSIZE);
   
   for(index = 0; index < strlen(message); index++) {
      if(letter[message[index]].pixmap != NULL) { 
	 gdk_draw_pixmap(pixmap, gc, letter[message[index]].pixmap, 0, 0, (LETTER_XSIZE + 1) * index, 0, LETTER_XSIZE, LETTER_YSIZE);
	 gdk_draw_pixmap(pixmap, gc, letter[message[index]].pixmap, 0, 0, (LETTER_XSIZE + 1) * (strlen(message) + 5) + (LETTER_XSIZE + 1) * index, 0, LETTER_XSIZE, LETTER_YSIZE);
      }
   }
   
   for(index = 0; index < 5; index++) {
      if(index == 0 || index == 4)
	gdk_draw_pixmap(pixmap, gc, letter[' '].pixmap, 0, 0, (LETTER_XSIZE + 1) * strlen(message) + (LETTER_XSIZE + 1) * index, 0, LETTER_XSIZE, LETTER_YSIZE);
      else
	gdk_draw_pixmap(pixmap, gc, letter['*'].pixmap, 0, 0, (LETTER_XSIZE + 1) * strlen(message) + (LETTER_XSIZE + 1) * index, 0, LETTER_XSIZE, LETTER_YSIZE);
   }
   
   return pixmap;
}

gint
scroller_clear(gint index)
{
   GdkGC *gc;
   
   gc = gdk_gc_new(window->window);
   switch(index) {
    case 0:
      gdk_draw_rectangle(window_bg, gc, TRUE, 201, 115, 193, LETTER_YSIZE); 
      break;
    case 1:
      gdk_draw_rectangle(window_bg, gc, TRUE, 201, 127, 193, LETTER_YSIZE);
      break;
   }
   gdk_gc_destroy(gc);
}

gint
scroller_finish(index)
{
   scroller_active[index] = 0;
   gdk_pixmap_unref(scroller_pm[index]);
   
   scroller_clear(index);
}

gint
scroller_display(gint index, gint xoffset)
{
   gint xsize;
   GdkGC *gc;
   
   xsize = scroller_xsize[index];
   
   if(xsize > 193)
     xsize = 193;
   
   gc = gdk_gc_new(window->window);
   switch(index) {
    case 0:
      gdk_draw_pixmap(window_bg, gc, scroller_pm[index], xoffset, 0, 201, 115, xsize, LETTER_YSIZE);
      break;
    case 1:
      gdk_draw_pixmap(window_bg, gc, scroller_pm[index], xoffset, 0, 201, 127, xsize, LETTER_YSIZE);
      break;
   }
   gdk_gc_destroy(gc);
}

gint
scroll_messages(gpointer func_data)
{
   gint index;
   
   for(index = 0; index < 2; index++) {
      if(scroller_active[index] == -1) {
	 if(strlen(scroller_message[index]) <= 32) {
	    scroller_xsize[index] = (LETTER_XSIZE + 1) * strlen(scroller_message[index]);
	    scroller_pm[index] = make_pixmap_message(scroller_message[index]);
	 } else {
	    scroller_xsize[index] = (LETTER_XSIZE + 1) * (strlen(scroller_message[index]) * 2 + 5);
	    scroller_pm[index] = make_scroller_pixmap_message(scroller_message[index]);
	 }
	 
	 scroller_x[index] = 0;
	 scroller_active[index] = 1;
	 
	 scroller_clear(index);
	 scroller_display(index, 0);
	 gdk_window_clear_area(window->window, 201, 115, 193, 21);
      }
   }
   
   for(index = 0; index < 2; index++) {
      if(scroller_active[index]) {
	 if(strlen(scroller_message[index]) > 32) {
	    scroller_x[index] = (scroller_x[index] + 1) % ((strlen(scroller_message[index]) + 5) * (LETTER_XSIZE + 1));
	    scroller_display(index, scroller_x[index]);
	    gdk_window_clear_area(window->window, 201, 115, 193, 21);
	 }
      }
   }
   
   if(cddb_lamp != 0 && valid_cddb && !disable_cddb) {
      cddb_lamp = 0;
      change_cddb_lamp(0);
      gdk_window_clear(window->window);
   }
   
   if(cddb_lamp != 1 && !valid_cddb && cddb_sock < 0 && !disable_cddb) {
      cddb_lamp = 1;
      change_cddb_lamp(1);
      gdk_window_clear(window->window);
   }
   
   if(cddb_lamp != 2 && disable_cddb) { 
      cddb_lamp = 2;
      change_cddb_lamp(2);
      gdk_window_clear(window->window);
   }
   
   if(coverart_lamp != 0 && valid_coverart && !disable_coverart) {
      coverart_lamp = 0;
      change_coverart_lamp(0);
      gdk_window_clear(window->window);
   }
   
   if(coverart_lamp != 1 && !valid_coverart && coverart_sock < 0 && !disable_coverart) {
      coverart_lamp = 1;
      change_coverart_lamp(1);
      gdk_window_clear(window->window);
   }
   
   if(coverart_lamp != 2 && disable_coverart) {
      coverart_lamp = 2;
      change_coverart_lamp(2);
      gdk_window_clear(window->window);
   }
   
   return TRUE;
}

void
statusbar_clear()
{
   GdkGC *gc;
   
   if(window_mode != 2)
     return;
   
   gc = gdk_gc_new(window->window);
   gdk_draw_rectangle(window_bg, gc, TRUE, 3, 202, 390, LETTER_YSIZE);
   gdk_gc_destroy(gc);
}

void
statusbar_show_message(gchar *message_text)
{
   GdkPixmap *message;
   GdkGC *gc;
   
   if(window_mode != 2)
     return;
   
   statusbar_clear();
      
   gc = gdk_gc_new(window->window);
   message = make_pixmap_message(message_text);
   gdk_draw_pixmap(window_bg, gc, message, 0, 0, 3, 202, (LETTER_XSIZE + 1) * strlen(message_text), LETTER_YSIZE);
   gdk_pixmap_unref(message);
   gdk_gc_destroy(gc);
   statusbar_timer = 50;
}

void
print_message(gchar *message_text)
{
   statusbar_show_message(message_text);
   gdk_window_clear(window->window);
}

void
printf_message(const char *format, ...)
{
   va_list ap;
   char *outbuffer;
   
   va_start(ap, format);
   outbuffer = malloc(66);
   vsnprintf(outbuffer, 66, format, ap);
   va_end(ap);
   print_message(outbuffer);
   free(outbuffer);
}

void
cddb_thread_input(gpointer func_data, gint source, GdkInputCondition condition)
{
   gint len;
   gchar command[4], inbuffer[100];
   
   if(read(cddb_sock, command, 4) <= 0) {
      cddb_sock = -1;
      gdk_input_remove(cddb_tag);
      return;
   }

   if(strncmp(command, "MESG", 4) == 0) {
      read(cddb_sock, &len, sizeof(gint));
      memset(inbuffer, '\0', 100);
      read(cddb_sock, inbuffer, len);

      statusbar_show_message(inbuffer);
   }

   if(strncmp(command, "CDDB", 4) == 0) {
      cddb_read_disc_data(cd_fd, &data);
      valid_cddb = 1;
   }

   if(strncmp(command, "SPSH", 4) == 0)
     display_cover(splash_pm);
   
   gdk_window_clear(window->window);
}

void
coverart_thread_input(gpointer func_data, gint source, GdkInputCondition condition)
{
   GdkPixmap *pm;
   gint len;
   gchar command[4], inbuffer[100];

   if(read(coverart_sock, command, 4) <= 0) {
      coverart_sock = -1;
      gdk_input_remove(coverart_tag);
      return;
   }

   if(strncmp(command, "MESG", 4) == 0) {
      read(coverart_sock, &len, sizeof(gint));
      memset(inbuffer, '\0', 100);
      read(coverart_sock, inbuffer, len);

      statusbar_show_message(inbuffer);
   }

   if(strncmp(command, "COVR", 4) == 0) {
      read(coverart_sock, &len, sizeof(gint));
      memset(inbuffer, '\0', 100);
      read(coverart_sock, inbuffer, len);
     
      if(inbuffer[0] == 'T') {
	 valid_coverart = 1;
         pm = load_image(inbuffer + 1);
	 display_cover(pm);
	 free_image(pm);
         unlink(inbuffer + 1);
      } else {
         pm = load_local_image(inbuffer + 1);
	 display_cover(pm);
	 free_image(pm);
      }
   }

   if(strncmp(command, "SPSH", 4) == 0)
     display_cover(splash_pm);
   
   if(strncmp(command, "SDCA", 4) == 0)
     create_coverart_list();
   
   gdk_window_clear(window->window);
}

void
cddb_press(void)
{
   cddb_depressed = 1;
}

void
cddb_release(void)
{
   cddb_depressed = 0;
   change_cddb_lamp(cddb_lamp);
   
   if(cddb_sock >= 0) {
      pthread_cancel(cddb_tid);
      cddb_sock = -1;
      statusbar_show_message("CDDB action interrupted");
   } else {
      if(disable_cddb) {
	 disable_cddb = 0;
	 checked_cddb = 0;
	 valid_cddb = 0;
	 checked_coverart = 0;
	 valid_coverart = 0;
	 set_conf_value("CDDB-Disable", "No");
      } else {
	 disable_cddb = 1;
	 valid_cddb = 0;
	 valid_coverart = 0;
	 cddb_info_displayed = 0;
	 scroller_finish(0);
	 scroller_finish(1);
	 statusbar_show_message("CDDB lookup disabled");
	 set_conf_value("CDDB-Disable", "Yes");
      }
   }
}

void
coverart_press(void)
{
   coverart_depressed = 1;
}

void
coverart_release(void)
{
   coverart_depressed = 0;
   change_coverart_lamp(coverart_lamp);
   
   if(coverart_sock >= 0) {
      pthread_cancel(coverart_tid);
      coverart_sock = -1;
      statusbar_show_message("Cover art action interrupted");
   } else {
      if(disable_coverart) {
	 disable_coverart = 0;
	 checked_coverart = 0;
	 valid_coverart = 0;
	 set_conf_value("CoverArt-Disable", "No");
      } else {
	 disable_coverart = 1;
	 valid_coverart = 0;
	 statusbar_show_message("Cover art lookup disabled");
	 set_conf_value("CoverArt-Disable", "Yes");
      }
   }
}

gint
draw_lamps(gpointer func_data)
{
   if(cddb_sock >= 0 && !valid_cddb) {
      switch(cddb_lamp) {
       case 3:
         cddb_lamp = 4;
         change_cddb_lamp(4);
         break;
       case 4:
         cddb_lamp = 3;
         change_cddb_lamp(3);
         break;
       default:
         cddb_lamp = 3;
         change_cddb_lamp(3);
         break;
      }
   }

   if(coverart_sock >= 0 && !valid_coverart) {
      switch(coverart_lamp) {
       case 3:
         coverart_lamp = 4;
         change_coverart_lamp(4);
         break;
       case 4:
         coverart_lamp = 3;
         change_coverart_lamp(3);
         break;
       default:
         coverart_lamp = 3;
         change_coverart_lamp(3);
         break;
      }
   }

   gdk_window_clear_area(window->window, 200, 70, 122, 16);

   return TRUE;
}

gint
disc_poll(gpointer func_data)
{
   GdkPixmap *pm;
   gint redraw = 0, lba;
   struct disc_status status;
   struct disc_timeval dtv;
   
   if(cd_fd < 0)
     return TRUE;
   
   cd_poll(cd_fd, &status);
   
   if(status.status_present) {
      if(!disc_present) {
	 disc_present = 1;
	 cd_stat(cd_fd, &disc);
	 play_track = 1;
	 old_play_track = 1;
	 update_track_counter(play_track);
	 update_disc_status(status.status_mode);
	 old_status = status.status_mode;
	 redraw = 1;
	 gdk_window_clear(window->window);
      }
      
      if(disable_cddb || disable_coverart) {
	 display_cover(splash_pm);
	 gdk_window_clear(window->window);
      }
      
      if(!checked_cddb && !disable_cddb) {
	 checked_cddb = 1;
	 cddb_sock = cddb_thread_init(&cddb_tid);
	 pthread_detach(cddb_tid);
	 cddb_tag = gdk_input_add(cddb_sock, GDK_INPUT_READ, cddb_thread_input, NULL);
      }
      
      if(valid_cddb && !checked_coverart && !disable_coverart) {
	 checked_coverart = 1;
	 coverart_sock = coverart_thread_init(&coverart_tid);
	 pthread_detach(coverart_tid);
	 coverart_tag = gdk_input_add(coverart_sock, GDK_INPUT_READ, coverart_thread_input, NULL);
      }
      
      if(valid_cddb && !cddb_info_displayed) {
	 cddb_info_displayed = 1;
	 scroller_active[0] = -1;
	 scroller_active[1] = -1;
	 if((strlen(data.data_artist) + strlen(data.data_title)) < 29)
	   snprintf(scroller_message[0], 33, "%s / %s", data.data_artist, data.data_title);
	 else
	   strncpy(scroller_message[0], data.data_title, 256);
	 snprintf(scroller_message[1], 256, "%s (%d:%02d)", data.data_track[play_track - 1].track_name, disc.disc_track[play_track - 1].track_length.minutes, disc.disc_track[play_track - 1].track_length.seconds);
      }
      
      if(status.status_mode != old_status) {
	 update_disc_status(status.status_mode);
	 old_status = status.status_mode;
	 redraw = 1;
      }
   } else {
      if(disc_present) {
	 if(cddb_sock >= 0)
	   pthread_cancel(cddb_tid);
	 
	 if(coverart_sock >= 0)
	   pthread_cancel(coverart_tid);
	 
	 cddb_thread_active = 0;
	 coverart_thread_active = 0;
	 cddb_sock = -1;
	 coverart_sock = -1;
	 checked_cddb = 0;
	 checked_coverart = 0;
	 valid_cddb = 0;
	 valid_coverart = 0;
	 cddb_info_displayed = 0;
	 play_track = 0;
	 disc_present = 0;
	 scroller_finish(0);
	 scroller_finish(1);
	 zero_counters();
	 update_disc_status(-1);
	 if((pm = load_local_image("nodisc.png")) == NULL)
	   fprintf(stderr, "Error loading local image\n");
	 else {
	    display_cover(pm);
	    free_image(pm);
	 }
	 
	 gdk_window_clear(window->window);
      }
   }
   
   if(status.status_mode == CDAUDIO_PLAYING || status.status_mode == CDAUDIO_PAUSED) {
      if(!disc_playing) {
	 disc_playing = 1;
	 play_track = status.status_current_track;
	 old_play_track = play_track;
	 update_track_counter(play_track);
	 redraw = 1;
      }
      
      if(play_track != status.status_current_track) {
	 play_track = status.status_current_track;
	 old_play_track = play_track;
	 update_track_counter(play_track);
	 if(valid_cddb) {
	    scroller_active[1] = -1;
	    snprintf(scroller_message[1], 256, "%s (%d:%02d)", data.data_track[play_track - 1].track_name, disc.disc_track[play_track - 1].track_length.minutes, disc.disc_track[play_track - 1].track_length.seconds);
	 }
	 redraw = 1;
      }
      
      
      lba = disc.disc_track[status.status_current_track - 1].track_lba;
      lba -= lba % 75;      
      cd_frames_to_msf(&dtv, cd_msf_to_lba(status.status_disc_time) - lba);
      update_track_time(&dtv);
      cd_frames_to_msf(&dtv, cd_msf_to_lba(status.status_disc_time));
      update_disc_time(&dtv);
      redraw = 1;
   } else {
      if(disc_playing) {
	 if(disc_present)
	   play_track = 1;
	 else
	   play_track = 0;
	 
	 disc_playing = 0;
	 zero_counters();
	 update_track_counter(play_track);
	 redraw = 1;
      }
      
      if(play_track != old_play_track) {
	 old_play_track = play_track;
	 update_track_counter(play_track);
	 if(valid_cddb) {
	    scroller_active[1] = -1;
	    snprintf(scroller_message[1], 256, "%s (%d:%02d)", data.data_track[play_track - 1].track_name, disc.disc_track[play_track - 1].track_length.minutes, disc.disc_track[play_track - 1].track_length.seconds);
	 }
	 redraw = 1;
      }
   }
   
   if(statusbar_timer >= 0) {
      statusbar_timer--;
      if(statusbar_timer == -1) {
	 statusbar_clear();
	 gdk_window_clear(window->window);
      }
   }
   
   if(redraw)
     gdk_window_clear_area(window->window, 200, 12, 190, 60);
   
   return TRUE;
}

int
main(int argc, char **argv)
{
   GtkStyle *style;
   struct disc_status status;
   char *confitem;
   
   scroller_active[0] = 0;
   scroller_active[1] = 0;
   
   gtk_init(&argc, &argv);
   gdk_rgb_init();
   
   window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_widget_set_app_paintable(window, TRUE);
   gtk_window_set_title(GTK_WINDOW(window), "gdcd");
   gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);
   gtk_window_set_wmclass(GTK_WINDOW(window), "gdcd_app", "gdcd");
   gtk_widget_set_events(window, GDK_FOCUS_CHANGE_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_STRUCTURE_MASK);
   gtk_widget_realize(window);
 
   gdk_window_set_decorations(window->window, 0);
   gtk_widget_set_usize(window, 396, 200);
   
   style = gtk_widget_get_style(window);
   window_bg = gdk_pixmap_create_from_xpm_d(window->window, &window_mk, &style->bg[GTK_STATE_NORMAL], background_xpm);
   
   gtk_widget_shape_combine_mask(window, window_mk, 0, 0);
   gdk_window_set_back_pixmap(window->window, window_bg, 0);
   
   load_pixmaps();
   make_letters(window);
   make_numbers(window);
   make_buttons(window);
   
   gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(destroy), NULL);
   gtk_signal_connect(GTK_OBJECT(window), "button_press_event", GTK_SIGNAL_FUNC(window_press_event), NULL);
   gtk_signal_connect(GTK_OBJECT(window), "button_release_event", GTK_SIGNAL_FUNC(window_release_event), NULL);
   gtk_signal_connect(GTK_OBJECT(window), "motion_notify_event", GTK_SIGNAL_FUNC(window_motion_event), NULL);
   
   display_cover(splash_pm);
   zero_counters();
   place_buttons();
   
   gtk_timeout_add(25, scroll_messages, NULL);
   gtk_timeout_add(1000, draw_lamps, NULL);
   gtk_timeout_add(200, disc_poll, NULL);
   
   if((confitem = get_conf_value("Window-Size")) == NULL) {
      set_conf_value("Window-Size", "Max");
      max_draw_window();
   } else {
      if(strcmp(confitem, "Max") == 0)
	max_draw_window();
      free(confitem);
   }
   
   if((confitem = get_conf_value("CDDB-Disable")) != NULL) {
      if(strcmp(confitem, "Yes") == 0)
	disable_cddb = 1;
      free(confitem);
   }
   
   if((confitem = get_conf_value("CoverArt-Disable")) != NULL) {
      if(strcmp(confitem, "Yes") == 0)
	disable_coverart = 1;
      free(confitem);
   }
   
   statusbar_show_message("gdcd 0.2.0 (C)1998-2000 Tony Arcieri");
   
   if((confitem = get_conf_value("CD-Device")) == NULL) {
      set_conf_value("CD-Device", DEFAULT_DEVICE);
      confitem = get_conf_value("CD-Device");
   }
   
   if((cd_fd = cd_init_device(confitem)) < 0)
     create_device_dialog();
   else
     gtk_widget_show(window);
   
   free(confitem);
   
   gtk_main();
   
   return 0;
}
