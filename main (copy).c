#include <gtk/gtk.h>
#include <fontconfig.h>
#include <cairo.h>
#include <freetype/ftbitmap.h>
#include <pango/pangocairo.h>
#include <pango/pangoft2.h> 
 
#define CANVAS_WIDTH  640
#define CANVAS_HEIGHT 480

#define LABEL_TEXT  "Click the button to change the font."

gchar buf[256];
PangoFontDescription  *g_font_desc = NULL;
 
void destroy( GtkWidget *widget,gpointer   data )
{
   gtk_main_quit ();
}

gboolean on_expose_event(GtkWidget *widget,
    GdkEventExpose *event,
    gpointer data) {
        
  cairo_t *cr;
  gint size = 12;
  const char * family = "Sans";
  cairo_font_slant_t slant = CAIRO_FONT_SLANT_NORMAL;
  cairo_font_weight_t weight = CAIRO_FONT_WEIGHT_NORMAL;
  PangoStyle pstyle;
  PangoWeight pweight;
  
  
  if( g_font_desc )
  {
	  size = pango_font_description_get_size ( g_font_desc );
	  size /= PANGO_SCALE;
	  family = pango_font_description_get_family( g_font_desc );
	  pstyle =  pango_font_description_get_style ( g_font_desc );
	  switch( pstyle ) {
		case PANGO_STYLE_NORMAL: slant=CAIRO_FONT_SLANT_NORMAL; break;
		case PANGO_STYLE_OBLIQUE: slant=CAIRO_FONT_SLANT_OBLIQUE; break;
		case PANGO_STYLE_ITALIC: slant=CAIRO_FONT_SLANT_ITALIC; break;
		default: slant=CAIRO_FONT_SLANT_NORMAL; break;
	}

	 pweight =  pango_font_description_get_weight ( g_font_desc ) ;
	 weight =  pweight<= 500?CAIRO_FONT_WEIGHT_NORMAL:CAIRO_FONT_WEIGHT_BOLD;
  }

  cr = gdk_cairo_create(widget->window);

  cairo_move_to(cr, 0, 30);
  cairo_set_font_size(cr, size);
  cairo_select_font_face(cr, family, slant, weight);
  
  cairo_show_text(cr, buf);

  cairo_destroy(cr);

  return FALSE;
}

gboolean on_expose_event2(GtkWidget *widget,
    GdkEventExpose *event,   gpointer data) 
{
	cairo_surface_t* surf = NULL;
	cairo_t* cr = NULL;
	cairo_status_t status;
	PangoContext* context = NULL;
	PangoLayout* layout = NULL;
	PangoFontDescription* font_desc = NULL;
	PangoFontMap* font_map = NULL;
	FT_Bitmap bmp = {0};
 
	int stride = 0;
	int width = CANVAS_WIDTH;
	int height = CANVAS_HEIGHT;
        
	/* ------------------------------------------------------------ */
	/*                   I N I T I A L I Z E                        */
	/* ------------------------------------------------------------ */

	/* FT buffer */
	FT_Bitmap_New(&bmp);
	bmp.rows = height;
	bmp.width = width;
	bmp.buffer = (unsigned char*)calloc(bmp.rows * bmp.width, 1);
	if (NULL == bmp.buffer) {
		printf("+ error: cannot allocate the buffer for the output bitmap.\n");
		exit(EXIT_FAILURE);
	}
 
	/* create our "canvas" */
	bmp.pitch = (width + 3) & -4;
	bmp.pixel_mode = FT_PIXEL_MODE_GRAY; /*< Grayscale*/
	bmp.num_grays = 256;
	stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, width);
	surf = cairo_image_surface_create_for_data(bmp.buffer, CAIRO_FORMAT_A8, width, height, stride);
 
	if (CAIRO_STATUS_SUCCESS != cairo_surface_status(surf)) {
		printf("+ error: couldn't create the surface.\n");
		exit(EXIT_FAILURE);
	}
 
	/* create our cairo context object that tracks state. */
	cr = cairo_create(surf);
	if (CAIRO_STATUS_NO_MEMORY == cairo_status(cr)) {
		printf("+ error: out of memory, cannot create cairo_t*\n");
		exit(EXIT_FAILURE);
	}
	
	
	
  /* create our cairo context object that tracks state. */
  cr = cairo_create(surf);
  if (CAIRO_STATUS_NO_MEMORY == cairo_status(cr)) {
    printf("+ error: out of memory, cannot create cairo_t*\n");
    exit(EXIT_FAILURE);
  }
 
  /* ------------------------------------------------------------ */
  /*               D R A W   I N T O  C A N V A S                 */
  /* ------------------------------------------------------------ */
 
  font_map = pango_ft2_font_map_new();
  if (NULL == font_map) {
    printf("+ error: cannot create the pango font map.\n");
    exit(EXIT_FAILURE);
  }
 
  context = pango_font_map_create_context(font_map);
  if (NULL == context) {
    printf("+ error: cannot create pango font context.\n");
    exit(EXIT_FAILURE);
  }
 
  /* create layout object. */
  layout = pango_layout_new(context);
  if (NULL == layout) {
    printf("+ error: cannot create the pango layout.\n");
    exit(EXIT_FAILURE);
  }
 
  /* create the font description @todo the reference does not tell how/when to free this */
  font_desc = pango_font_description_from_string("Station 35");
  pango_layout_set_font_description(layout, font_desc);
  pango_font_map_load_font(font_map, context, font_desc);
  pango_font_description_free(font_desc);
 
  /* set the width around which pango will wrap */
  pango_layout_set_width(layout, 150 * PANGO_SCALE);
 
  /* write using the markup feature */
  const gchar* text = ""
    "<span foreground=\"blue\" font_family=\"Station\">"
    "   <b> bold </b>"
    "   <u> is </u>"
    "   <i> nice </i>"
    "</span>"
    "<tt> hello </tt>"
    "<span font_family=\"sans\" font_stretch=\"ultracondensed\" letter_spacing=\"500\" font_weight=\"light\"> SANS</span>"
    "<span foreground=\"#FFCC00\"> colored</span>"
    "";
 
//  gchar* plaintext ;
//  PangoAttrList* attr_list;
  pango_layout_set_markup(layout, text, -1);
 
  /* render */
  pango_ft2_render_layout(&bmp, layout, 30, 100);
  pango_cairo_update_layout(cr, layout);
 
  /* ------------------------------------------------------------ */
  /*               O U T P U T  A N D  C L E A N U P              */
  /* ------------------------------------------------------------ */
 
  /* write to png */
  status = cairo_surface_write_to_png(surf, "test_font.png");
  if (CAIRO_STATUS_SUCCESS != status) {
    printf("+ error: couldn't write to png\n");
    exit(EXIT_FAILURE);
  }
 
  cairo_surface_destroy(surf);
  cairo_destroy(cr);
 
  g_object_unref(layout);
  g_object_unref(font_map);
  g_object_unref(context);
 


	return FALSE;
}

gboolean time_handler(GtkWidget *widget) {
    
  if (widget->window == NULL) return FALSE;

  GDateTime *now = g_date_time_new_now_local(); 
  gchar *my_time = g_date_time_format(now, "%H:%M:%S");
  
  sprintf(buf, "%s", my_time);
 // printf("%s\n", my_time);
  
  g_free(my_time);
  g_date_time_unref(now);

  gtk_widget_queue_draw(widget);
  
  return TRUE;
}

void showfontlist1()
{
	FcPattern *pat;
	FcFontSet *fs;
	FcObjectSet *os;
	FcChar8 *s, *file;
	FcConfig *config;
	FcBool result;
	int i;

	result = FcInit();
	config = FcConfigGetCurrent();
	FcConfigSetRescanInterval(config, 0);

	// show the fonts (debugging)
	pat = FcPatternCreate();
	os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, FC_LANG, (char *) 0);
	fs = FcFontList(config, pat, os);
	printf("Total fonts: %d", fs->nfont);
	for (i=0; fs && i < fs->nfont; i++) {
	    FcPattern *font = fs->fonts[i];//FcFontSetFont(fs, i);
	    FcPatternPrint(font);
	    s = FcNameUnparse(font);
	    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
		printf("Filename: %s", file);
	    }
	    printf("Font: %s", s);
	    free(s);
	}
	if (fs) FcFontSetDestroy(fs);
}

void showfontlist2()
{
	FcConfig* config = FcInitLoadConfigAndFonts();
	FcPattern* pat = FcPatternCreate();
	FcObjectSet* os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, (char *) 0);
	FcFontSet* fs = FcFontList(config, pat, os);
	printf("Total matching fonts: %d\n", fs->nfont);
	for (int i=0; fs && i < fs->nfont; ++i) {
	   FcPattern* font = fs->fonts[i];
	   FcChar8 *file, *style, *family;
	   if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
	       FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch &&
	       FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch)
	   {
	      printf("Filename: %s (family %s, style %s)\n", file, family, style);
	   }
	}
	if (fs) FcFontSetDestroy(fs);
}

GdkPixbuf *create_pixbuf(const gchar * filename) {
    
   GdkPixbuf *pixbuf;
   GError *error = NULL;
   pixbuf = gdk_pixbuf_new_from_file(filename, &error);
   
   if (!pixbuf) {
       
      fprintf(stderr, "%s\n", error->message);
      g_error_free(error);
   }

   return pixbuf;
}

void on_font_changed (GtkFontButton *button, GtkWidget *label)
{
	const gchar *fontname;
	
	fontname = gtk_font_button_get_font_name ( button );
	g_font_desc = pango_font_description_from_string ( fontname );
	gtk_widget_modify_font (label, g_font_desc);
}

int main( int   argc,char *argv[] )
{
 
   GtkWidget *window;
   GtkWidget *fontbutton;
   GdkPixbuf *icon;
   GtkWidget *fixed;
   GtkWidget *dataarea, *dataarea2;
   GtkWidget *label = gtk_label_new (LABEL_TEXT);

   // showfontlist1();
   gtk_init (&argc, &argv);
 
   window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   fixed = gtk_fixed_new();

	
   /* Set the window title */
   gtk_window_set_title (GTK_WINDOW (window), "Scala Font Viewer using Pango");
   gtk_window_set_default_size(GTK_WINDOW(window), CANVAS_WIDTH, CANVAS_HEIGHT);
   gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
   gtk_container_set_border_width(GTK_CONTAINER (window), 20);

   icon = create_pixbuf("scala.png");  
   gtk_window_set_icon(GTK_WINDOW(window), icon);
 
   g_signal_connect (window, "destroy",
   G_CALLBACK (destroy), NULL);
  
   fontbutton = gtk_font_button_new();
   gtk_widget_set_tooltip_text(fontbutton, "Font Button widget. Click to select font!");
 
   g_signal_connect (GTK_OBJECT(fontbutton),
      "font_set",G_CALLBACK (on_font_changed),
      (gpointer) label);

  // Create draw area here
   gtk_fixed_put(GTK_FIXED(fixed), fontbutton, 0, 0);
   gtk_widget_set_size_request(fontbutton, CANVAS_WIDTH / 2.5, 30);

   dataarea = gtk_drawing_area_new();
   gtk_fixed_put(GTK_FIXED(fixed), dataarea, CANVAS_WIDTH / 2, -10);
   gtk_widget_set_size_request(dataarea, CANVAS_WIDTH / 2, CANVAS_HEIGHT - 35);
   g_signal_connect(dataarea, "expose-event", G_CALLBACK(on_expose_event), NULL);

   dataarea2 = gtk_drawing_area_new();
   gtk_fixed_put(GTK_FIXED(fixed), dataarea2, 0, 40);
   gtk_widget_set_size_request(dataarea2, CANVAS_WIDTH, CANVAS_HEIGHT - 35);
   g_signal_connect(dataarea2, "expose-event", G_CALLBACK(on_expose_event2), NULL);
   
   
   gtk_container_add(GTK_CONTAINER(window), fixed);
 
   g_timeout_add(1000, (GSourceFunc) time_handler, (gpointer) window);
   gtk_widget_show_all(window); 
   time_handler(window);

   g_object_unref(icon); 

   gtk_main ();
 
   return 0;
}
