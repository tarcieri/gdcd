#include <gtk/gtk.h>
#include <gdk_imlib.h>
#include <pthread.h>
#include <cdaudio.h>
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#if IMLIB_IMAGING
#include <gdk_imlib.h>
#else
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#include "coverart.h"

extern GtkWidget *window;

extern gint cd_fd;
struct art_query art_query;

GdkPixmap
*load_image(gchar *filename)
{
   GdkPixmap *pixmap;
   GdkBitmap *bitmap;
   
#if IMLIB_IMAGING
   GdkImlibImage *image;
   
   if((image = gdk_imlib_load_image(filename)) == NULL)
     return NULL;
   
   gdk_imlib_render(image, 170, 170);
   
   pixmap = gdk_imlib_move_image(image);
   bitmap = gdk_imlib_move_mask(image);
#else
   GdkGC *gc;
   GdkPixbuf *pixbuf;
   
   if((pixbuf = gdk_pixbuf_new_from_file(filename)) == NULL)
     return NULL;
   
   if((pixmap = gdk_pixmap_new(window->window, 170, 170, gdk_visual_get_best_depth())) == NULL)
     return NULL;
       
   gc = gdk_gc_new(window->window);
   gdk_pixbuf_render_to_drawable(pixbuf, pixmap, gc, 0, 0, 0, 0, 170, 170, 0, 0, 0); 
   gdk_gc_destroy(gc);
   
   gdk_pixbuf_unref(pixbuf);
#endif
   
   gdk_flush();
   return pixmap;
}

void
free_image(GdkPixmap *image)
{
#if IMLIB_IMAGING
   gdk_imlib_free_pixmap(image);
#else
   gdk_pixmap_unref(image);
#endif
}

GdkPixmap
*load_local_image(gchar *filename)
{
   gchar real_filename[108];
   snprintf(real_filename, 108, "%s/%s", IMAGE_PATH, filename);
   
   return load_image(real_filename);
}

void
*thread_coverart_lookup(void *arg)
{
   gint fd, sock, selection = 0, display_sock = (gint)arg;
   gchar tempmask[20], filename[108], outbuffer[256], http_string[256], inpkt[4];
   struct art_data art;
   struct cddb_host host;
   struct disc_mc_data data;
   
   coverart_read_data(cd_fd, &art);
   if(art.art_present) {
      strncpy(tempmask, "T/tmp/gdcdXXXXXX", 20);
      strncpy(filename, mktemp(tempmask), 108);
      
      if((fd = creat(filename + 1, 0600)) < 0) {
	 close(display_sock);
	 return NULL;
      }
      
      write(fd, art.art_data, art.art_length);
      close(fd);
      
      thread_invoke_operation(display_sock, "COVR", filename, strlen(filename));
      close(display_sock);
      return NULL;
   }
   
   thread_invoke_operation(display_sock, "SPSH", NULL, 0);   
   cddb_mc_read_disc_data(cd_fd, &data);

   strncpy(host.host_server.server_name, "coverart.undergrid.net", 256);
   strncpy(host.host_addressing, "coverart/getart.php3", 256);
   host.host_server.server_port = 80;

   art.art_present = 0;

   snprintf(outbuffer, 256, "Connecting to %s:%d", host.host_server.server_name, host.host_server.server_port);
   thread_relay_message(display_sock, outbuffer);
   if((sock = coverart_connect_server(host, NULL, http_string, 256)) >= 0) {
      thread_relay_message(display_sock, "Locating cover art");
      if(coverart_name_query(sock, &art_query, http_string, data.data_title, data.data_artist) == 0) {
         close(sock);
	 if(art_query.query_matches > 1) {
	    thread_relay_message(display_sock, "Inexact match for cover art");
	    write(display_sock, "SDCA", 4);
	    read(display_sock, &selection, sizeof(gint));
	 }
         if(art_query.query_matches > 0 && selection >= 0) {
            thread_relay_message(display_sock, "Downloading cover art");
            coverart_read(&art, NULL, art_query.query_list[selection].list_host);
         } else {
            if((sock = coverart_connect_server(host, NULL, http_string, 256)) >= 0) {
               if(coverart_query(cd_fd, sock, &art_query, http_string) == 0) {
		  close(sock);
		  if(art_query.query_matches > 1) {
		     thread_invoke_operation(display_sock, "SDCA", NULL, 0);
		     read(display_sock, &selection, sizeof(gint));
	    
		     if(selection < 0) {
			snprintf(filename, 108, "L%s.png", cddb_genre(data.data_genre));
			thread_invoke_operation(display_sock, "COVR", filename, strlen(filename));
			close(display_sock);
			cddb_mc_free(&data);
			return NULL;
		     }
		  }
                  if(art_query.query_matches > 0) {
                     thread_relay_message(display_sock, "Downloading cover art");
                     coverart_read(&art, NULL, art_query.query_list[selection].list_host);
                  }
               }
            }
         }
      }
   }

   thread_relay_message(display_sock, "Done");

   if(art.art_present) {
      coverart_write_data(cd_fd, art);
      
      strncpy(tempmask, "T/tmp/gdcdXXXXXX", 20);
      strncpy(filename, mktemp(tempmask), 108);
      
      if((fd = creat(filename + 1, 0600)) < 0) {
	 close(display_sock);
	 cddb_mc_free(&data);
	 return NULL;
      }
      
      write(fd, art.art_data, art.art_length);
      close(fd);
   } else
      snprintf(filename, 108, "L%s.png", cddb_genre(data.data_genre));
   
   thread_invoke_operation(display_sock, "COVR", filename, strlen(filename));
   
   cddb_mc_free(&data);
   close(display_sock);
  
   return NULL;
}

gint
coverart_thread_init(pthread_t *tid)
{
   gint rc, sockets[2];
   char *message;
   
   if(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
      perror("sockpair");
      return -1;
   }

   if((rc = pthread_create(tid, NULL, thread_coverart_lookup, (void *)sockets[1])) != 0) {
      if((message = malloc(64)) == NULL) {
         perror("malloc");
         return -1;
      }

      strerror_r(rc, message, 64);
      fputs("Error invoking pthread_create: ", stderr);
      fputs(message, stderr);
      fputc('\n', stderr);

      free(message);
      return -1;
   }
   
   return sockets[0];
}

