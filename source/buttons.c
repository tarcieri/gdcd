#include <gtk/gtk.h>
#include <pixmaps/control_buttons.xpm>
#include <pixmaps/misc_buttons.xpm>
#include <pixmaps/netops.xpm>

#include "buttons.h"
#include "cdfunction.h"

void destroy();
void cddb_press();
void cddb_release();
void coverart_press();
void coverart_release();

extern struct gdcd_button control_button[24];
gdcd_button_type command_type_order[8] = { gdcdPrevButton, gdcdAdvButton, gdcdRewButton, gdcdFFButton, gdcdEjectButton, gdcdCloseButton, gdcdStopButton, gdcdPlayButton };
gdcd_button_type misc_type_order[3] = { gdcdKillButton, gdcdBigButton, gdcdLittleButton };
gdcd_button_type netops_type_order[2] = { gdcdCDDBButton, gdcdCoverArtButton };

void
make_buttons(GtkWidget *window)
{
   gint index;
   GdkPixmap *buttons_pm;
   GdkBitmap *buttons_bm;
   GdkGC *gc;
   GtkStyle *style;
   
   for(index = 0; index < 24; index++) {
      control_button[index].depressed = 0;
      control_button[index].image = NULL;
   }
   
   style = gtk_widget_get_style(window);
   gc = gdk_gc_new(window->window);
   
   buttons_pm = gdk_pixmap_create_from_xpm_d(window->window, &buttons_bm, &style->bg[GTK_STATE_NORMAL], control_buttons_xpm);
   
   for(index = 0; index < 8; index++) {
      control_button[index].type = command_type_order[index];
      switch(command_type_order[index]) {
       case gdcdPrevButton:
	 control_button[index].press_func = prev_press;
	 control_button[index].release_func = prev_release;
	 break;
       case gdcdAdvButton:
	 control_button[index].press_func = adv_press;
	 control_button[index].release_func = adv_release;
	 break;
       case gdcdRewButton:
         control_button[index].press_func = rew_press;
         control_button[index].release_func = rew_release;
         break;
       case gdcdFFButton:
         control_button[index].press_func = ff_press;
         control_button[index].release_func = ff_release;
         break;
       case gdcdEjectButton:
         control_button[index].press_func = eject_press;
         control_button[index].release_func = eject_release;
         break;
       case gdcdCloseButton:
         control_button[index].press_func = close_press;
         control_button[index].release_func = close_release;
         break;
       case gdcdPlayButton:
         control_button[index].press_func = play_press;
         control_button[index].release_func = play_release;
         break;
       case gdcdStopButton:
         control_button[index].press_func = stop_press;
         control_button[index].release_func = stop_release;
         break;
      }
      control_button[index].image = gdk_pixmap_new(window->window, BUTTON_WIDTH, BUTTON_HEIGHT, gdk_visual_get_best_depth());
      gdk_draw_pixmap(control_button[index].image, gc, buttons_pm, index * BUTTON_WIDTH, 0, 0, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
   }
   
   gdk_pixmap_unref(buttons_pm);
   gdk_pixmap_unref(buttons_bm);

   buttons_pm = gdk_pixmap_create_from_xpm_d(window->window, &buttons_bm, &style->bg[GTK_STATE_NORMAL], misc_buttons_xpm);

   for(index = 0; index < 3; index++) {
      control_button[index + 8].type = misc_type_order[index];
      switch(control_button[index + 8].type) {
       case gdcdKillButton:
	 control_button[index + 8].press_func = NULL;
	 control_button[index + 8].release_func = destroy;
	 break;
       case gdcdLittleButton:
	 control_button[index + 8].press_func = NULL;
	 control_button[index + 8].release_func = shrink_release;
	 break;
       case gdcdBigButton:
	 control_button[index + 8].press_func = NULL;
	 control_button[index + 8].release_func = enlarge_release;
	 break;
      }
      control_button[index + 8].image = gdk_pixmap_new(window->window, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT, gdk_visual_get_best_depth());
      gdk_draw_pixmap(control_button[index + 8].image, gc, buttons_pm, index * SMALL_BUTTON_WIDTH, 0, 0, 0, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
   }
 
   gdk_pixmap_unref(buttons_pm);
   gdk_bitmap_unref(buttons_bm);
   
   buttons_pm = gdk_pixmap_create_from_xpm_d(window->window, &buttons_bm, &style->bg[GTK_STATE_NORMAL], netops_xpm);
   
   for(index = 0; index < 3; index++) {
      control_button[index + 11].type = netops_type_order[index];
      switch(control_button[index + 11].type) {
       case gdcdCDDBButton:
	 control_button[index + 11].press_func = cddb_press;
	 control_button[index + 11].release_func = cddb_release;
	 break;
       case gdcdCoverArtButton:
	 control_button[index + 11].press_func = coverart_press;
	 control_button[index + 11].release_func = coverart_release;
	 break;
      }
      control_button[index + 11].image = gdk_pixmap_new(window->window, 60, 15, gdk_visual_get_best_depth());
      gdk_draw_pixmap(control_button[index + 11].image, gc, buttons_pm, index * 60, 0, 0, 0, 60, 15);
   }
   
   gdk_gc_destroy(gc);
}

struct gdcd_button
*get_button(gdcd_button_type type)
{
   int index;
   
   for(index = 0; index < 24; index++)
     if(type == control_button[index].type)
       return &control_button[index];
   
   return NULL;
}

int
check_button(struct gdcd_button *button, gint x, gint y)
{
   if(x > button->x && x <= (button->x + button->w + 1) && y > button->y && y <= (button->y + button->h + 1))
     return 1;
   
   return 0;
}
