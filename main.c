#include <gtk/gtk.h>
#include <fontconfig.h>
#include <cairo.h>
#include <freetype/ftbitmap.h>
#include <pango/pangocairo.h>
#include <pango/pangoft2.h> 
 
#define CANVAS_WIDTH  800
#define CANVAS_HEIGHT 600
#define OFFSET  45
#define LABEL_TEXT  "Click the button to change the font."

#define USE_MARKUP   0
//#define USE_COLOR_BMP

#ifdef _MBCS
#pragma warning(disable : 4996)
#endif

gchar buf[256];
PangoFontDescription  *g_font_desc = NULL;
#if 1
const gchar* plaintext  =  "AVWA This is a list of answers to questions that are frequently asked by new users to cairo.  😀 ⺁ ⻤ 🥰 🦖"
	"";
#else
const gchar* plaintext =  ""
    "<span foreground=\"blue\" font_family=\"Station\">"
    "   <b> bold </b>"
    "   <u> is </u>"
    "   <i> nice </i>"
    "</span>"
    "<tt> hello </tt>"
    "<span font_family=\"sans\" font_stretch=\"ultracondensed\" letter_spacing=\"500\" font_weight=\"light\"> SANS</span>"
    "<span foreground=\"#FFCC00\"> colored  😀 ⺁ ⻤ 🥰 🦖</span>"
    "";
#endif

#ifdef GTKV2
#define EXPOSE_EVENT_STR "expose-event"
#else
#define EXPOSE_EVENT_STR "draw"
#endif

float bgcolor[] = {0.0, 0.0, 0.0};
float fgcolor[] = {1.0, 1.0, 1.0};


////////////////////////// Read and Write a bmp file on disk ////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define DATA_OFFSET_OFFSET 0x000A
#define WIDTH_OFFSET 0x0012
#define HEIGHT_OFFSET 0x0016
#define BITS_PER_PIXEL_OFFSET 0x001C
#define HEADER_SIZE 14
#define INFO_HEADER_SIZE 40
#define NO_COMPRESION 0
#define MAX_NUMBER_OF_COLORS 0
#define ALL_COLORS_REQUIRED 0

typedef unsigned int int32;
typedef short int16;
typedef unsigned char byte;

//***Inputs*****
//fileName: The name of the file to open 
//***Outputs****
//pixels: A pointer to a byte array. This will contain the pixel data
//width: An int pointer to store the width of the image in pixels
//height: An int pointer to store the height of the image in pixels
//bytesPerPixel: An int pointer to store the number of bytes per pixel that are used in the image
void ReadImage(const char *fileName,byte **pixels, int32 *width, int32 *height, int32 *bytesPerPixel)
{
        //Open the file for reading in binary mode
        FILE *imageFile = fopen(fileName, "rb");
        //Read data offset
        int32 dataOffset;
        fseek(imageFile, DATA_OFFSET_OFFSET, SEEK_SET);
        fread(&dataOffset, 4, 1, imageFile);
        //Read width
        fseek(imageFile, WIDTH_OFFSET, SEEK_SET);
        fread(width, 4, 1, imageFile);
        //Read height
        fseek(imageFile, HEIGHT_OFFSET, SEEK_SET);
        fread(height, 4, 1, imageFile);
        //Read bits per pixel
        int16 bitsPerPixel;
        fseek(imageFile, BITS_PER_PIXEL_OFFSET, SEEK_SET);
        fread(&bitsPerPixel, 2, 1, imageFile);
        //Allocate a pixel array
        *bytesPerPixel = ((int32)bitsPerPixel) / 8;

        //Rows are stored bottom-up
        //Each row is padded to be a multiple of 4 bytes. 
        //We calculate the padded row size in bytes
        int paddedRowSize = (int)(4 * ceil((float)(*width) / 4.0f))*(*bytesPerPixel);
        //We are not interested in the padded bytes, so we allocate memory just for
        //the pixel data
        int unpaddedRowSize = (*width)*(*bytesPerPixel);
        //Total size of the pixel data in bytes
        int totalSize = unpaddedRowSize*(*height);
        *pixels = (byte*)malloc(totalSize);
        //Read the pixel data Row by Row.
        //Data is padded and stored bottom-up
        int i = 0;
        //point to the last row of our pixel array (unpadded)
        byte *currentRowPointer = *pixels+((*height-1)*unpaddedRowSize);
        for (i = 0; i < *height; i++)
        {
                //put file cursor in the next row from top to bottom
	        fseek(imageFile, dataOffset+(i*paddedRowSize), SEEK_SET);
	        //read only unpaddedRowSize bytes (we can ignore the padding bytes)
	        fread(currentRowPointer, 1, unpaddedRowSize, imageFile);
	        //point to the next row (from bottom to top)
	        currentRowPointer -= unpaddedRowSize;
        }

        fclose(imageFile);
}

//***Inputs*****
//fileName: The name of the file to save 
//pixels: Pointer to the pixel data array
//width: The width of the image in pixels
//height: The height of the image in pixels
//bytesPerPixel: The number of bytes per pixel that are used in the image
void WriteImage(const char *fileName, byte *pixels, int32 width, int32 height,int32 bytesPerPixel)
{
        //Open file in binary mode
        FILE *outputFile = fopen(fileName, "wb");
        //*****HEADER************//
        //write signature
        const char *BM = "BM";
        fwrite(&BM[0], 1, 1, outputFile);
        fwrite(&BM[1], 1, 1, outputFile);
        //Write file size considering padded bytes
        int paddedRowSize = (int)(4 * ceil((float)width/4.0f))*bytesPerPixel;
        int32 fileSize = paddedRowSize*height + HEADER_SIZE + INFO_HEADER_SIZE;
        fwrite(&fileSize, 4, 1, outputFile);
        //Write reserved
        int32 reserved = 0x0000;
        fwrite(&reserved, 4, 1, outputFile);
        //Write data offset
        int32 dataOffset = HEADER_SIZE+INFO_HEADER_SIZE;
        fwrite(&dataOffset, 4, 1, outputFile);

        //*******INFO*HEADER******//
        //Write size
        int32 infoHeaderSize = INFO_HEADER_SIZE;
        fwrite(&infoHeaderSize, 4, 1, outputFile);
        //Write width and height
        fwrite(&width, 4, 1, outputFile);
        fwrite(&height, 4, 1, outputFile);
        //Write planes
        int16 planes = 1; //always 1
        fwrite(&planes, 2, 1, outputFile);
        //write bits per pixel
        int16 bitsPerPixel = bytesPerPixel * 8;
        fwrite(&bitsPerPixel, 2, 1, outputFile);
        //write compression
        int32 compression = NO_COMPRESION;
        fwrite(&compression, 4, 1, outputFile);
        //write image size (in bytes)
        int32 imageSize = width*height*bytesPerPixel;
        fwrite(&imageSize, 4, 1, outputFile);
        //write resolution (in pixels per meter)
        int32 resolutionX = 11811; //300 dpi
        int32 resolutionY = 11811; //300 dpi
        fwrite(&resolutionX, 4, 1, outputFile);
        fwrite(&resolutionY, 4, 1, outputFile);
        //write colors used 
        int32 colorsUsed = MAX_NUMBER_OF_COLORS;
        fwrite(&colorsUsed, 4, 1, outputFile);
        //Write important colors
        int32 importantColors = ALL_COLORS_REQUIRED;
        fwrite(&importantColors, 4, 1, outputFile);
        //write data
        int i = 0;
        int unpaddedRowSize = width*bytesPerPixel;
        for ( i = 0; i < height; i++)
        {
                //start writing from the beginning of last row in the pixel array
                int pixelOffset = ((height - i) - 1)*unpaddedRowSize;
                fwrite(&pixels[pixelOffset], 1, paddedRowSize, outputFile);	
        }
        fclose(outputFile);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
	
void destroy( GtkWidget *widget,gpointer   data )
{
   gtk_main_quit ();
}

gboolean on_expose_event(GtkWidget *widget,
    GdkEventExpose *event,
    gpointer data) {
        
  cairo_t *cr;
  gint size = 20;
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

  cr = gdk_cairo_create(gtk_widget_get_window(widget));

  cairo_move_to(cr, 0, 30);
  cairo_set_font_size(cr, size);
  cairo_select_font_face(cr, family, slant, weight);
  cairo_show_text(cr, buf);
  cairo_destroy(cr);

  return FALSE;
}

// use pango-ft2 to draw text
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
	int height = CANVAS_HEIGHT - OFFSET;
	
	GdkPixbuf *pixbuf;
	GError *err = NULL;
    cairo_t *cr2;
        
	/* ------------------------------------------------------------ */
	/*                   I N I T I A L I Z E                        */
	/* ------------------------------------------------------------ */
	/* FT buffer */
	FT_Bitmap_New(&bmp);
	bmp.rows = height;
	bmp.width = width;
#ifdef USE_COLOR_BMP
	bmp.buffer = (unsigned char*)calloc(bmp.rows * bmp.width, 4);
#else
	bmp.buffer = (unsigned char*)calloc(bmp.rows * bmp.width, 1);
#endif	
	if (NULL == bmp.buffer) {
		printf("+ error: cannot allocate the buffer for the output bitmap.\n");
		exit(EXIT_FAILURE);
	}
 
	/* create our "canvas" */
#ifdef USE_COLOR_BMP	
	bmp.pitch = ((width + 3) & -4) * 4; 
	bmp.pixel_mode = FT_PIXEL_MODE_BGRA; /*< Grayscale*/
#else
	bmp.pitch = (width + 3) & -4;
	bmp.pixel_mode = FT_PIXEL_MODE_GRAY; /*< Grayscale*/
#endif	
	bmp.num_grays = 256;
#ifdef USE_COLOR_BMP	
	stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	surf = cairo_image_surface_create_for_data(bmp.buffer, CAIRO_FORMAT_ARGB32, width, height, stride);
#else
	stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, width);
	surf = cairo_image_surface_create_for_data(bmp.buffer, CAIRO_FORMAT_A8, width, height, stride);
#endif
 
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
 
	/* set the width around which pango will wrap */
	pango_layout_set_width(layout, (CANVAS_WIDTH - OFFSET) * PANGO_SCALE);
 
	// This sets the resolution of the device
	//pango_ft2_font_map_set_resolution(PANGO_FT2_FONT_MAP(font_map), width, height);

 	/* create the font description @todo the reference does not tell how/when to free this */
	if(g_font_desc)
		font_desc = g_font_desc;
	else
		font_desc = pango_font_description_from_string("Sans 20");
	pango_layout_set_font_description(layout, font_desc);
	pango_font_map_load_font(font_map, context, font_desc);
	if(g_font_desc == NULL)
		pango_font_description_free(font_desc);

 #if USE_MARKUP
	/* write using the markup feature */
	const gchar* text = ""
    "<span foreground=\"blue\" font_family=\"Station\">"
    "   <b> bold </b>"
    "   <u> is </u>"
    "   <i> nice </i>"
    "</span>"
    "<tt> hello </tt>"
    "<span font_family=\"sans\" font_stretch=\"ultracondensed\" letter_spacing=\"500\" font_weight=\"light\"> SANS</span>"
    "<span foreground=\"#FFCC00\"> colored  😀 ⺁ ⻤ 🥰 🦖</span>"
    "";
 
//  gchar* plaintext ;
//  PangoAttrList* attr_list;
	pango_layout_set_markup(layout, text, -1);	
#else
	pango_layout_set_text(layout, plaintext, -1);
#endif
	//pango_layout_context_changed(layout);

	/* render */
	pango_ft2_render_layout(&bmp, layout, 0, 0);
	pango_cairo_update_layout(cr, layout);
 
	/* ------------------------------------------------------------ */
	/*               O U T P U T  A N D  C L E A N U P              */
	/* ------------------------------------------------------------ */

	//Write to bmp
#ifdef USE_COLOR_BMP
	 WriteImage("img2.bmp", bmp.buffer, width, height, 4);
#else
	unsigned char *pRawBuffer = (unsigned char *)calloc(width * height, 4);
	for(int j=0; j<height; j++)
	{
		for(int i = 0; i < width; i++)
		{
			int k = j * width + i;
			unsigned char *p = &pRawBuffer[k * 4];
			p[0]=p[1]=p[2]=bmp.buffer[k];
			p[3] = 0xFF;
		}
	} 
	 
	WriteImage("img2.bmp", pRawBuffer, width, height, 4);
#endif

	/* write to png */
	status = cairo_surface_write_to_png(surf, "test_font.png");
	if (CAIRO_STATUS_SUCCESS != status) {
		printf("+ error: couldn't write to png\n");
		exit(EXIT_FAILURE);
	}

////////////////////////////////////////////////// Output to GtkWidget ///////////////////////////////////////////////////////
#if 1	
	pixbuf = gdk_pixbuf_new_from_file("test_font.png", &err);
    if(err)
    {
        printf("Error : %s\n", err->message);
        g_error_free(err);
        return FALSE;
    }
    cr2 = gdk_cairo_create (gtk_widget_get_window(widget));
    gdk_cairo_set_source_pixbuf(cr2, pixbuf, 0, 0);
    cairo_paint(cr2);
    //    cairo_fill (cr);
    cairo_destroy (cr2);
#else
	// copy source surface to dest surface
	cr2 = gdk_cairo_create (gtk_widget_get_window(widget));
	cairo_set_source_surface (cr2, surf, 0, 0);
	cairo_paint (cr2);	
	cairo_destroy(cr2);
#endif		
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	cairo_surface_destroy(surf);
	cairo_destroy(cr);

	g_object_unref(layout);
	g_object_unref(font_map);
	g_object_unref(context);



	return FALSE;
}

// Use simple cairo cairo_show_text to draw  text
gboolean on_expose_event3(GtkWidget *widget,
    GdkEventExpose *event,
    gpointer data) {
        
  cairo_t *cr;
  gint size = 20;
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
  
  
	cr = gdk_cairo_create(gtk_widget_get_window(widget));

	/* Set surface to translucent color (r, g, b, a) */
	cairo_set_source_rgb (cr, bgcolor[0], bgcolor[1],  bgcolor[2]);
	cairo_paint (cr);
	cairo_set_source_rgb (cr, fgcolor[0], fgcolor[1], fgcolor[2]);

	cairo_move_to(cr, 0, 40);
	cairo_set_font_size(cr, size);
	cairo_select_font_face(cr, family, slant, weight);
	cairo_show_text(cr, plaintext);
	cairo_destroy(cr);

	return FALSE;
}

// use pango-cairo to draw text 
gboolean on_expose_event4(GtkWidget *widget,
	GdkEventExpose *event,
    gpointer data) {
        
	cairo_t *cr;
	PangoLayout *layout;


	if( g_font_desc ) {
		cr = gdk_cairo_create(gtk_widget_get_window(widget));
		/* Set surface to translucent color (r, g, b, a) */
		cairo_set_source_rgb (cr, bgcolor[0], bgcolor[1],  bgcolor[2]);
		cairo_paint (cr);
		cairo_set_source_rgb (cr, fgcolor[0], fgcolor[1], fgcolor[2]);
		layout = pango_cairo_create_layout (cr);  // This function is the most convenient way to use Cairo with Pango, however 
								//  it is slightly inefficient since it creates a separate PangoContext object for each layout.
		// Make sure the DPI (resolution) same on both Cairo context and Pango context
		PangoContext *pCtx = pango_layout_get_context(layout);
		pango_cairo_context_set_resolution(pCtx, 72);
		
		/* set the width around which pango will wrap */
		pango_layout_set_width(layout, (CANVAS_WIDTH - OFFSET) * PANGO_SCALE);
		pango_layout_set_text (layout, plaintext, -1);
		pango_layout_set_font_description (layout, g_font_desc);
		pango_cairo_update_layout (cr, layout);
		pango_cairo_show_layout (cr, layout);
		
		g_object_unref (layout);
	}
	
	return FALSE;
}

// use pango-cairo to draw text with markup and in memory destination
gboolean on_expose_event5(GtkWidget *widget,
	GdkEventExpose *event,
    gpointer data) {
        
	cairo_t *cr;
	cairo_t *cr2;
	PangoLayout *layout;

	cairo_surface_t *surface;
	unsigned char * buf = NULL;
	int stride = 0;
	int width = CANVAS_WIDTH;
	int height = CANVAS_HEIGHT - OFFSET;

	stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	buf = (unsigned char*)calloc(stride * height, 1);
	surface = cairo_image_surface_create_for_data(buf, CAIRO_FORMAT_ARGB32, width, height, stride);

	if( g_font_desc ) {
		cr  = cairo_create(surface);

		/* Set surface to translucent color (r, g, b, a) */
		cairo_set_source_rgb (cr, bgcolor[0], bgcolor[1],  bgcolor[2]);
		cairo_paint (cr);
		cairo_set_source_rgb (cr, fgcolor[0], fgcolor[1], fgcolor[2]);
		layout = pango_cairo_create_layout (cr);  // This function is the most convenient way to use Cairo with Pango, however 
								//  it is slightly inefficient since it creates a separate PangoContext object for each layout.
		// Make sure the DPI (resolution) same on both Cairo context and Pango context
		PangoContext *pCtx = pango_layout_get_context(layout);
		pango_cairo_context_set_resolution(pCtx, 72);
		
		/* set the width around which pango will wrap */
		pango_layout_set_width(layout, (CANVAS_WIDTH - OFFSET) * PANGO_SCALE);
		pango_layout_set_markup(layout, plaintext, -1);
		pango_layout_set_font_description (layout, g_font_desc);
		pango_cairo_update_layout (cr, layout);
		pango_cairo_show_layout (cr, layout);
		
		// Save to bmp file
		WriteImage("img5.bmp", buf, width, height, 4);

		// copy source surface to dest surface
		cr2 = gdk_cairo_create (gtk_widget_get_window(widget));
		cairo_set_source_surface (cr2, surface, 0, 0);
		cairo_paint (cr2);	

		cairo_destroy(cr2);
		cairo_destroy(cr);
		g_object_unref (layout);
	}

	cairo_surface_destroy(surface);
	free(buf);
	
	return FALSE;
}

gboolean time_handler(GtkWidget *widget) {
    
  if (gtk_widget_get_window(widget) == NULL) return FALSE;

  GDateTime *now = g_date_time_new_now_local(); 
  gchar *my_time = g_date_time_format(now, "%H:%M:%S");
  
  sprintf(buf, "%s", my_time);
 // printf("%s\n", my_time);
  
  g_free(my_time);
  g_date_time_unref(now);

  gtk_widget_queue_draw(widget);
  
  return TRUE;
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
   GtkWidget *dataarea, *dataarea2, *dataarea3;
#ifdef GTKV2
   GtkWidget *label = gtk_label_new (LABEL_TEXT);
#endif   

   // showfontlist1();
   gtk_init (&argc, &argv);
 
   window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   fixed = gtk_fixed_new();

	
   /* Set the window title */
   gtk_window_set_title (GTK_WINDOW (window), "Scala Font Viewer using Pango");
   gtk_window_set_default_size(GTK_WINDOW(window), CANVAS_WIDTH, CANVAS_HEIGHT);
   gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
   gtk_container_set_border_width(GTK_CONTAINER (window),  6);

   icon = create_pixbuf("scala.png");  
   gtk_window_set_icon(GTK_WINDOW(window), icon);
 
   g_signal_connect (window, "destroy",
   G_CALLBACK (destroy), NULL);
  
   g_font_desc = pango_font_description_from_string("Sans 20");
   fontbutton = gtk_font_button_new_with_font("Sans 20");
   gtk_widget_set_tooltip_text(fontbutton, "Font Button widget. Click to select font!");
 
#ifdef GTKV2	  
   g_signal_connect (GTK_OBJECT(fontbutton),
      "font_set",G_CALLBACK (on_font_changed),
      (gpointer) label);
#else
   g_signal_connect (fontbutton,
      "font_set",G_CALLBACK (on_font_changed),
	   NULL);
#endif	   	  

  // Create draw area here
   gtk_fixed_put(GTK_FIXED(fixed), fontbutton, 0, 0);
   gtk_widget_set_size_request(fontbutton, CANVAS_WIDTH / 2.5, 30);

   dataarea = gtk_drawing_area_new();
   gtk_fixed_put(GTK_FIXED(fixed), dataarea, CANVAS_WIDTH / 2, -10);
   gtk_widget_set_size_request(dataarea, CANVAS_WIDTH / 2, 100);
   g_signal_connect(dataarea, EXPOSE_EVENT_STR, G_CALLBACK(on_expose_event), NULL);

   dataarea2 = gtk_drawing_area_new();
   gtk_fixed_put(GTK_FIXED(fixed), dataarea2, 0, OFFSET);
   gtk_widget_set_size_request(dataarea2, CANVAS_WIDTH, CANVAS_HEIGHT / 2 - OFFSET / 2 - 5 );
   g_signal_connect(dataarea2, EXPOSE_EVENT_STR, G_CALLBACK(on_expose_event4), NULL);

   dataarea3 = gtk_drawing_area_new();
   gtk_fixed_put(GTK_FIXED(fixed), dataarea3, 0, CANVAS_HEIGHT / 2 + OFFSET / 2 );
   gtk_widget_set_size_request(dataarea3, CANVAS_WIDTH, CANVAS_HEIGHT / 2 - OFFSET / 2 - 0.5);
   g_signal_connect(dataarea3, EXPOSE_EVENT_STR, G_CALLBACK(on_expose_event5), NULL);  
   
   gtk_container_add(GTK_CONTAINER(window), fixed);
 
   g_timeout_add(1000, (GSourceFunc) time_handler, (gpointer) window);
   gtk_widget_show_all(window); 
   time_handler(window);

   g_object_unref(icon); 

   gtk_main ();
 
   return 0;
}
