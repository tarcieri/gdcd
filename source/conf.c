#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char
*get_conf_file(void)
{
   char *ret = malloc(strlen(getenv("HOME")) + 9);
   sprintf(ret, "%s/.gdcdrc", getenv("HOME"));
   return ret;
}

char
*get_conf_value(char *label)
{
   FILE *conf;
   char *ret, *tempbuffer, *file;
   
   file = get_conf_file();
   if((conf = fopen(file, "r")) == NULL) {
      free(file);
      return NULL;
   }
   
   tempbuffer = malloc(256);
   
   while(!feof(conf)) {
      fgets(tempbuffer, 256, conf);
      if(strchr(tempbuffer, '#') != NULL)
	*strchr(tempbuffer, '#') = '\0';
      if(strncmp(tempbuffer, label, strlen(label)) == 0 && strstr(tempbuffer, ": ") != NULL) {
	 ret = malloc(strlen(strstr(tempbuffer, ": ")) - 1);
	 strncpy(ret, strstr(tempbuffer, ": ") + 2, strlen(strstr(tempbuffer, ": ")) - 1);
	 if(ret[strlen(ret) - 1] = '\n')
	   ret[strlen(ret) - 1] = '\0';
	 return ret;
      }
   }
   
   free(tempbuffer);
   free(file);
   
   return NULL;
}

int
set_conf_value(char *label, char *value)
{
   FILE *conf;
   char *in, *temp, *tempbuffer, *file;
   struct stat st;
   
   file = get_conf_file();
   if(stat(file, &st) >= 0) {
      if((conf = fopen(file, "r")) == NULL) {
	 free(file);
	 return -1;
      }
      
      tempbuffer = malloc(256);
      
      in = malloc(st.st_size + 1);
      temp = in;
      while(!feof(conf)) {
	 if(fgets(tempbuffer, 256, conf) != NULL) {
	    if(strncmp(tempbuffer, label, strlen(label)) != 0) {
	       memcpy(temp, tempbuffer, strlen(tempbuffer));
	       temp += strlen(tempbuffer);
	    }
	 }
      } 
      
      free(tempbuffer);
      
      temp[0] = '\0';
      fclose(conf);
      
      if((conf = fopen(file, "w")) == NULL) {
	 free(file);
	 free(tempbuffer);
	 free(in);
	 return -1;
      }
      
      fputs(in, conf);
      free(in);
   } else
      if((conf = fopen(file, "w")) == NULL) {
	 free(file);
	 return -1;
      }
   free(file);
   
   fprintf(conf, "%s: %s\n", label, value);
   fclose(conf);
   
   return 0;
}
