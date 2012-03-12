#include <gtk/gtk.h>
#include <cdaudio.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <config.h>

extern gint cd_fd;

void
*thread_cddb_lookup(void *arg)
{
   gint sock, display_sock = (gint)arg;
   guint discid;
   gchar outbuffer[256], http_string[256];
   struct disc_data *data;
   struct cddb_entry entry;
   struct cddb_hello hello;
   struct cddb_query query;
   struct cddb_conf conf;
   struct cddb_serverlist list;
   struct cddb_server *proxy;
   
   discid = cddb_discid(cd_fd);
   cddb_stat_disc_data(cd_fd, &entry);
   
   if(entry.entry_present) {
      snprintf(outbuffer, 256, "CDDB information for %08lx was read from local cache.", discid);
      thread_relay_message(display_sock, outbuffer);
      thread_invoke_operation(display_sock, "CDDB", NULL, 0);
      
      close(display_sock);
      return NULL;
   }
  
   thread_invoke_operation(display_sock, "SPSH", NULL, 0);
   
   proxy = (struct cddb_server *)malloc(sizeof(struct cddb_server));
   cddb_read_serverlist(&conf, &list, proxy);
   if(conf.conf_access == CDDB_ACCESS_LOCAL) {
      free(proxy);
      snprintf(outbuffer, 256, "No match for %08lx.", discid);
      thread_relay_message(display_sock, outbuffer);
      
      close(display_sock);
      return NULL;
   }
   
   if(!conf.conf_proxy) {
      free(proxy);
      proxy = NULL;
   }
   
   if(list.list_len < 1) {
      if(proxy != NULL)
	free(proxy);
      strncpy(list.list_host[0].host_server.server_name, "freedb.freedb.org", 256);
      list.list_host[0].host_server.server_port = 888;
      list.list_host[0].host_protocol = CDDB_MODE_CDDBP;
   }
   
   strncpy(hello.hello_program, PACKAGE, 256);
   strncpy(hello.hello_version, VERSION, 256);
   
   switch(list.list_host[0].host_protocol) {
    case CDDB_MODE_CDDBP:
      snprintf(outbuffer, 256, "Making CDDBP connection to %s:%d", list.list_host[0].host_server.server_name, list.list_host[0].host_server.server_port);
      thread_relay_message(display_sock, outbuffer);
      sock = cddb_connect_server(list.list_host[0], NULL, hello);
      break;
    case CDDB_MODE_HTTP:
      snprintf(outbuffer, 256, "Making HTTP connection to %s:%d", list.list_host[0].host_server.server_name, list.list_host[0].host_server.server_port);
      thread_relay_message(display_sock, outbuffer);
      sock = cddb_connect_server(list.list_host[0], proxy, hello, http_string, 512);
      break;
   }

   if(sock == -1) {
      if(proxy)
	free(proxy);
      
      snprintf(outbuffer, 256, "Couldn't connect to %s://%s:%d", list.list_host[0].host_protocol == CDDB_MODE_CDDBP ? "cddbp" : "http", list.list_host[0].host_server.server_name, list.list_host[0].host_server.server_port);
      return NULL;
   }
   
   switch(list.list_host[0].host_protocol) {
    case CDDB_MODE_CDDBP:
      snprintf(outbuffer, 256, "Retrieving information on %08lx.", discid);
      thread_relay_message(display_sock, outbuffer);
      if(cddb_query(cd_fd, sock, CDDB_MODE_CDDBP, &query) < 0) {
         snprintf(outbuffer, 256, "CDDB query error: %s", cddb_message);
         thread_relay_message(display_sock, outbuffer);

	 close(display_sock);
         return NULL;
      }
      break;
    case CDDB_MODE_HTTP:
      snprintf(outbuffer, 256, "Retrieving information on %08lx.", discid);
      thread_relay_message(display_sock, outbuffer);
      if(cddb_query(cd_fd, sock, CDDB_MODE_HTTP, &query, http_string) < 0) {
	 thread_relay_message(display_sock, "CDDB query error");
	 return NULL;
      }
      shutdown(sock, 2);
      close(sock);
      
      if((sock = cddb_connect_server(list.list_host[0], proxy, hello, http_string, 512)) < 0) {
	 thread_relay_message(display_sock, "HTTP server reconnection error");
	 if(proxy != NULL)
	   free(proxy);
	 return NULL;
      }
      break;
   }
   
   if(proxy != NULL)
     free(proxy);
   
   switch(query.query_match) {
    case QUERY_EXACT:
      if(strlen(query.query_list[0].list_artist) > 0) {
         snprintf(outbuffer, 256, "Match for %02lx: %s / %s", discid, query.query_list[0].list_artist, query.query_list[0].list_title);
         thread_relay_message(display_sock, outbuffer);
      } else {
         snprintf(outbuffer, 256, "Match for %02lx: %s", discid, query.query_list[0].list_title);
         thread_relay_message(display_sock, outbuffer);
      }
      entry.entry_genre = query.query_list[0].list_genre;
      entry.entry_id = query.query_list[0].list_id;
      break;
    case QUERY_INEXACT:
      snprintf(outbuffer, 256, "Inexact match for %02lx.", discid);
      thread_relay_message(display_sock, outbuffer);

      entry.entry_genre = query.query_list[0].list_genre;
      entry.entry_id = query.query_list[0].list_id;
      break;
    case QUERY_NOMATCH:
      snprintf(outbuffer, 256, "No match for %02lx.", discid);
      thread_relay_message(display_sock, outbuffer);

      close(display_sock);
      return NULL;
   }
   
   data = malloc(sizeof(struct disc_data));
   
   switch(list.list_host[0].host_protocol) {
    case CDDB_MODE_CDDBP:
      if(cddb_read(cd_fd, sock, CDDB_MODE_CDDBP, entry, data) < 0) {
         thread_relay_message(display_sock, "CDDB read error");

	 close(display_sock);
	 free(data);
	 
         return NULL;
      }

      cddb_quit(sock);
      break;
    case CDDB_MODE_HTTP:
      if(cddb_read(cd_fd, sock, CDDB_MODE_HTTP, entry, data, http_string) < 0) {
	 thread_relay_message(display_sock, "CDDB read error");
	 
	 close(display_sock);
	 free(data);
	 
	 return NULL;
      }
      break;
   }

   cddb_write_data(cd_fd, data);
   thread_invoke_operation(display_sock, "CDDB", NULL, 0);
   
   free(data);
   
   close(display_sock);
   return NULL;
}

gint
cddb_thread_init(pthread_t *tid)
{
   gint rc, sockets[2];
   char *message;

   if(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
      perror("sockpair");
      return -1;
   }

   if((rc = pthread_create(tid, NULL, thread_cddb_lookup, (void *)sockets[1])) != 0) {
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
