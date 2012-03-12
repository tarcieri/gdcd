#define BUTTON_WIDTH 48
#define BUTTON_HEIGHT 30

#define SMALL_BUTTON_WIDTH 15
#define SMALL_BUTTON_HEIGHT 15

typedef enum { 
   gdcdPlayButton, gdcdStopButton, gdcdAdvButton, gdcdPrevButton, gdcdFFButton, gdcdRewButton, gdcdEjectButton, gdcdCloseButton, gdcdBigButton, gdcdLittleButton, gdcdKillButton, gdcdCDDBButton, gdcdCoverArtButton
} gdcd_button_type;

struct gdcd_button {
   gdcd_button_type type;
   gint depressed;
   gint x, y;
   gint w, h;
   GtkSignalFunc press_func;
   GtkSignalFunc release_func;
   GdkPixmap *image;
};


void make_buttons(GtkWidget *window);
struct gdcd_button *get_button(gdcd_button_type type);
