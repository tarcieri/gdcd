## Process this file with automake to produce Makefile.in

noinst_HEADERS = buttons.h \
	         letters.h \
                 numbers.h \
	         conf.h \
                 cdfunction.h \
		 coverart.h
	         
bin_PROGRAMS = gdcd
gdcd_SOURCES = gdcd.c \
	       resize.c \
	       letters.c \
	       numbers.c \
	       buttons.c \
	       cdfunction.c \
	       cddb.c \
	       coverart.c \
	       thread.c \
	       dialog.c \
	       conf.c

CFLAGS = @CFLAGS@ $(GTK_CFLAGS) $(GNOME_CFLAGS) $(LIBART_CFLAGS) -DIMAGE_PATH=\"@IMAGE_PATH@\"
LDFLAGS = @LDFLAGS@ $(GTK_LIBS) $(GNOME_LIBS) $(LIBART_LIBS) $(GDKPIXBUF_LIBS) $(IMLIB_LIBS) $(PTHREAD_LIBS) $(CDAUDIO_LIBS)
