#include <gtk/gtk.h>
#include <pthread.h>
#include <unistd.h>

void
thread_invoke_operation(gint sock, gchar *operation, gchar *data, gint len)
{
   write(sock, operation, 4);

   if(len > 0) {
      write(sock, &len, sizeof(gint));
      write(sock, data, len);
   }
}

void
thread_relay_message(gint sock, gchar *message)
{
   thread_invoke_operation(sock, "MESG", message, strlen(message));
}

