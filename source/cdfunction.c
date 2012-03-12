#include <unistd.h>
#include <gtk/gtk.h>
#include <cdaudio.h> 

extern gint cd_fd, play_track, disc_present, disc_playing, valid_cddb;
extern struct disc_info disc;
extern struct disc_data data;

void adv_press()
{
}

void adv_release()
{
   if(!disc_present)
     return;

   if(disc_playing) {
      if(play_track + 1 > disc.disc_total_tracks) {
	 print_message("Stopped");
         cd_stop(cd_fd);
      } else {
	 if(valid_cddb)
	   printf_message("Playing track %d: %s", play_track + 1, data.data_track[play_track].track_name);
	 else
	   printf_message("Playing track %d", play_track + 1);
	 
         cd_play(cd_fd, play_track + 1);
      }
   } else {
      if(play_track + 1 <= disc.disc_total_tracks)
         play_track++;
   }
   
   usleep(200000);
}

void ff_press()
{
}

void ff_release()
{
   struct disc_timeval time;
   
   if(!disc_present || !disc_playing)
     return;
   
   print_message("Advancing 15 seconds");
   
   time.minutes = 0;
   time.seconds = 15;
   time.frames = 0;
   
   cd_advance(cd_fd, time);
}

void eject_press()
{
   print_message("Ejecting");
}

void eject_release()
{
   cd_eject(cd_fd);
}

void play_press()
{
   struct disc_status status;
   
   if(!disc_present)
     return;
   
   cd_poll(cd_fd, &status);
   
   switch(status.status_mode) {
    case CDAUDIO_PLAYING:
      print_message("Paused");
      break;
    case CDAUDIO_PAUSED:
      print_message("Resuming");
      break;
    default:
      if(valid_cddb)
	printf_message("Playing track %d: %s", play_track, data.data_track[play_track - 1].track_name);
      else
	printf_message("Playing track %d", play_track);
   }
}

void play_release()
{
   struct disc_status status;

   if(!disc_present)
     return;
   
   cd_poll(cd_fd, &status);

   switch(status.status_mode) {
    case CDAUDIO_PLAYING:
      cd_pause(cd_fd);
      return;
    case CDAUDIO_PAUSED:
      cd_resume(cd_fd);
      return;
    default:
      cd_play(cd_fd, play_track);
   }
}

void prev_press()
{
}

void prev_release()
{
   if(!disc_present)
     return;

   if(disc_playing) {
      if(play_track - 1 < disc.disc_first_track) {
	 print_message("Stopped");
         cd_stop(cd_fd);
      } else {
	 if(valid_cddb)
	   printf_message("Playing track %d: %s", play_track - 1, data.data_track[play_track - 2].track_name);
	 else
	   printf_message("Playing track %d", play_track - 1);
	 
         cd_play(cd_fd, play_track - 1);
      }
   } else {
      if(play_track - 1 >= disc.disc_first_track)
         play_track--;
   }
   
   usleep(200000);
}

void rew_press()
{
}

void rew_release()
{
   struct disc_timeval time;
   
   if(!disc_present || !disc_playing)
     return;
   
   print_message("Reversing 15 seconds");
   
   time.minutes = 0;
   time.seconds = -15;
   time.frames = 0;
   
   cd_advance(cd_fd, time);
}

void stop_press()
{
}

void stop_release()
{
   if(!disc_present)
      return;

   print_message("Stopped");
   
   cd_stop(cd_fd);
}

void close_press()
{
   print_message("Closing tray");
}

void close_release()
{  
   cd_close(cd_fd);
}
