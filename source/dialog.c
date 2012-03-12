#include <gtk/gtk.h>
#include <cdaudio.h>
#include <stdlib.h>

#include "conf.h"

extern int cd_fd, coverart_sock;
extern struct art_query art_query;
extern GtkWidget *window;

const gchar *cddb_list_data_key = "cddb_list_data";
const gchar *coverart_list_data_key = "coverart_list_data";

GtkWidget *ca_dialog, *dv_dialog;

void create_device_dialog();

void
coverart_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
   gint val = -1;
   
   write(coverart_sock, &val, sizeof(gint));
   
   gtk_widget_destroy(ca_dialog);
   return;
}

void
coverart_ok_button(GtkWidget *widget, gpointer data)
{
   GtkWidget *list = (GtkWidget *)data;
   GList *dlist;
   GtkObject *list_item;
   gchar *item_data, checkbuffer[100];
   gint index;
   
   dlist = GTK_LIST(list)->selection;
   
   if(!dlist)
     return;
   
   list_item = GTK_OBJECT(dlist->data);
   item_data = gtk_object_get_data(list_item, coverart_list_data_key);
   
   for(index = 0; index < art_query.query_matches; index++) {
      snprintf(checkbuffer, 100, "%s / %s", art_query.query_list[index].list_artist, art_query.query_list[index].list_album);
      if(strncmp(checkbuffer, item_data) == 0) {
	 write(coverart_sock, &index, sizeof(gint));
	 
	 gtk_widget_destroy(ca_dialog);
	 return;
      }
   }
   
   index = -1;
   
   write(coverart_sock, &index, sizeof(gint)); 
   
   gtk_widget_destroy(ca_dialog);
}

void
coverart_cancel_button(GtkWidget *widget, gpointer data)
{
   gint val = -1;
   
   write(coverart_sock, &val, sizeof(gint));
   
   gtk_widget_destroy(ca_dialog);
   return;
}

void
create_coverart_list(void)
{
   GtkWidget *scrolled_window, *label;
   GtkWidget *list, *list_item;
   GtkWidget *ok, *cancel;
   
   GList *dlist;
   
   guint index;
   gchar outtext[100], *string;
   
   ca_dialog = gtk_dialog_new();
   gtk_window_set_title(GTK_WINDOW(ca_dialog), "Select cover art");
   gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(ca_dialog)->vbox), 5);
   gtk_signal_connect(GTK_OBJECT(ca_dialog), "delete_event", GTK_SIGNAL_FUNC(coverart_delete_event), NULL);
   
   label = gtk_label_new("Please select the correct album from the following:");
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ca_dialog)->vbox), label, TRUE, TRUE, 0);
   gtk_widget_show(label);
   
   scrolled_window = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
   gtk_widget_set_usize(scrolled_window, 250, 150);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ca_dialog)->vbox), scrolled_window, TRUE, TRUE, 0);
   gtk_widget_show(scrolled_window);
   
   list = gtk_list_new();
   gtk_list_set_selection_mode(GTK_LIST(list), GTK_SELECTION_SINGLE);
   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), list);
   gtk_widget_show(list);

   ok = gtk_button_new_with_label("Ok");
   cancel = gtk_button_new_with_label("None of these");

   gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(coverart_ok_button), GTK_OBJECT(list));
   gtk_signal_connect(GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(coverart_cancel_button), NULL);
   
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ca_dialog)->action_area), ok, TRUE, TRUE, 0);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ca_dialog)->action_area), cancel, TRUE, TRUE, 0);
   
   gtk_widget_show(ok);
   gtk_widget_show(cancel);
   
   dlist = NULL;
   for(index = 0; index < art_query.query_matches; index++) {
      snprintf(outtext, 100, "%s / %s", art_query.query_list[index].list_artist, art_query.query_list[index].list_album);
      label = gtk_label_new(outtext);
      gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
      list_item = gtk_list_item_new();
      gtk_container_add(GTK_CONTAINER(list_item), label);
      gtk_widget_show(label);
      gtk_widget_show(list_item);
      gtk_label_get(GTK_LABEL(label), &string);
      gtk_object_set_data(GTK_OBJECT(list_item), coverart_list_data_key, string);
      dlist = g_list_append(dlist, list_item);
   }
   gtk_list_append_items(GTK_LIST(list), dlist);
   
   gtk_widget_show(ca_dialog);
}

void
device_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
   gtk_main_quit();
}

void
device_ok_button(GtkWidget *widget, gpointer data)
{
   GtkWidget *entry = (GtkWidget *)data;
   char *confitem;
   
   set_conf_value("CD-Device", gtk_entry_get_text(GTK_ENTRY(entry)));
   gtk_widget_destroy(dv_dialog);
   
   confitem = get_conf_value("CD-Device");
   if((cd_fd = cd_init_device(confitem)) < 0)
     create_device_dialog();
   else
     gtk_widget_show(window);
   
   free(confitem);
}

void
device_quit_button(GtkWidget *widget, gpointer data)
{
   gtk_main_quit();
}

void
create_device_dialog(void)
{
   GtkWidget *label, *entry, *ok, *quit;
   gchar *confitem;
   
   dv_dialog = gtk_dialog_new();
   gtk_window_set_title(GTK_WINDOW(dv_dialog), "Enter CD-ROM device");
   gtk_signal_connect(GTK_OBJECT(dv_dialog), "delete_event", GTK_SIGNAL_FUNC(device_delete_event), NULL);
   
   label = gtk_label_new("Please enter the location of your CD-ROM device:");
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dv_dialog)->vbox), label, TRUE, TRUE, 0);
   gtk_widget_show(label);
   
   entry = gtk_entry_new_with_max_length(108);
   confitem = get_conf_value("CD-Device");
   gtk_entry_set_text(GTK_ENTRY(entry), confitem);
   free(confitem);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dv_dialog)->vbox), entry, TRUE, TRUE, 0);
   gtk_widget_show(entry);
   
   ok = gtk_button_new_with_label("Ok");
   quit = gtk_button_new_with_label("Exit");
   
   gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(device_ok_button), GTK_OBJECT(entry));
   gtk_signal_connect(GTK_OBJECT(quit), "clicked", GTK_SIGNAL_FUNC(device_quit_button), NULL);
   
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dv_dialog)->action_area), ok, TRUE, TRUE, 0);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dv_dialog)->action_area), quit, TRUE, TRUE, 0);
   
   gtk_widget_show(ok);
   gtk_widget_show(quit);
   
   gtk_widget_show(dv_dialog);
}
