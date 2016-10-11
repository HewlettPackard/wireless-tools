/* Xwireless.c, status: experimental, do not distribute!! */
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Scrollbar.h>

#include <linux/wireless.h>

char* status[] = { "Scanning","Registering","Best AP","Good AP",
			  "Poor AP","Active Beacon Search","Static Load Balance",
			  "Balance Search" };

typedef struct privateData {
	char    *ifname;
	Pixel   currentColor;
	Pixel   highColor;
	Pixel   lowColor;
	Pixel   criticalColor;
	Pixel   foreground;
	int     highValue;
	int     lowValue;
	int     delay;
	String  geometry;
	struct  iw_statistics stats;
	struct  iw_range range;
} privateData;

static XtAppContext          app_context;
static Widget                scrollbar;
static Widget                topLevel;
static Widget                label;
static XtIntervalId          timerId;
static privateData    priv;

static int getstats(char *ifname, struct iw_statistics *stats)
{
	struct iwreq wrq;
	FILE *f=fopen("/proc/net/wireless","r");
	char buf[256];
	char *bp;
	if(f==NULL)
		return -1;
	while(fgets(buf,255,f))
	{
		bp=buf;
		while(*bp&&isspace(*bp))
			bp++;
		if( strncmp(bp,ifname,strlen(ifname))==0 && bp[strlen(ifname)]==':') {
			bp=strchr(bp,':');
			bp++;
			bp = strtok(bp, " .");
			sscanf(bp, "%X", &stats->status);
			bp = strtok(NULL, " .");
			sscanf(bp, "%d", &stats->qual.qual);
			bp = strtok(NULL, " .");
			sscanf(bp, "%d", &stats->qual.level);
			bp = strtok(NULL, " .");
			sscanf(bp, "%d", &stats->qual.noise);
			bp = strtok(NULL, " .");
			sscanf(bp, "%d", &stats->discard.nwid);
			bp = strtok(NULL, " .");
			sscanf(bp, "%d", &stats->discard.code);
			bp = strtok(NULL, " .");
			sscanf(bp, "%d", &stats->discard.misc);
			fclose(f);
			return 0;
		} else {
			stats->status = -1;
			stats->qual.qual = 0;
			stats->qual.level = 0;
			stats->qual.noise = 0;
		}
	}
	fclose(f);

	/*strcpy(wrq.ifr_name, ifname);
	wrq.u.data.pointer = (caddr_t) &range;
	wrq.u.data.length = 0;
	wrq.u.data.flags = 0;
	if(ioctl(skfd, SIOCGIWRANGE, &wrq) >= 0) {
		info->has_range = 1;
    }*/
	
	return 0;
}

static void update( XtPointer client_data, XtIntervalId *id )
{
	char       buf[128];
	static int pixel           = -1;
	static int lpixel          = -1;
	static int bpixel          = -1;

	getstats( priv.ifname, &(priv.stats));

	if(status < 8)
	  sprintf( buf, "%s", status[priv.stats.status] );
	else
	  sprintf( buf, "%s", "buggy" );
	XtVaSetValues( label, XtNlabel, buf, NULL );

	if (priv.stats.qual.qual <= priv.lowValue) {
		if (pixel != priv.criticalColor)
			XtVaSetValues( scrollbar, XtNforeground,
						   pixel = priv.criticalColor, NULL );
		if (bpixel != priv.criticalColor)
			XtVaSetValues( scrollbar, XtNborderColor,
						   bpixel = priv.criticalColor, NULL );
	} else if (priv.stats.qual.qual <= priv.highValue) {
		if (pixel != priv.lowColor)
			XtVaSetValues( scrollbar, 
						   XtNforeground, pixel = priv.lowColor, NULL );
		if (bpixel != priv.foreground)
			XtVaSetValues( scrollbar, XtNborderColor,
						   bpixel = priv.foreground, NULL );
	} else {
		if (pixel != priv.highColor )
			XtVaSetValues( scrollbar, 
						   XtNforeground, pixel = priv.highColor, NULL );
	}
	
	XawScrollbarSetThumb( scrollbar, 0.0, priv.stats.qual.qual / 255.0 );
 
 	timerId = XtAppAddTimeOut( app_context, 1000 , update, app_context );
}

#define offset(field) XtOffsetOf( privateData, field )
static XtResource resources[] = {
	{ "highColor", XtCForeground, XtRPixel, sizeof(Pixel),
	  offset(highColor), XtRString, "green" },
	{ "lowColor", XtCForeground, XtRPixel, sizeof(Pixel),
	  offset(lowColor), XtRString, "orange" },
	{ "criticalColor", XtCForeground, XtRPixel, sizeof(Pixel),
	  offset(criticalColor), XtRString, "red" },
	{ XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	  offset(foreground), XtRString, XtDefaultForeground },
	{ "highValue", XtCValue, XtRInt, sizeof(int),
	  offset(highValue), XtRImmediate, (XtPointer)50 },
	{ "lowValue", XtCValue, XtRInt, sizeof(int),
	  offset(lowValue), XtRImmediate, (XtPointer)10 },
	{ "geometry", XtCString, XtRString, sizeof( String ),
	  offset(geometry), XtRString, (XtPointer)"10x100" },
	{ "delay", XtCValue, XtRInt, sizeof(int),
	  offset(delay), XtRImmediate, (XtPointer)1 },
};

int main( int argc, char **argv ) {
	Cursor           cursor;
	int              c;
	Widget           form;
	XFontStruct      *fs;
	int              fontWidth, fontHeight;
	int              width = 120;
	
	/* The device name must be the first argument */
	if(argc < 2) {
		printf("Hmm\n");		
    }
	priv.ifname = argv[1];

	if( priv.ifname == (char *) NULL) {
		printf("Usage: xwireless <interface>\n");
		exit(-1);
	}

	topLevel = XtVaAppInitialize( &app_context, "Xwireless",
								  NULL, 0,
								  &argc, argv, NULL, NULL );

	XtGetApplicationResources( topLevel,
							   &priv,
							   resources,
							   XtNumber( resources ),
							   NULL, 0 );
	priv.lowValue = 85;
	priv.highValue = 170;

/*	printf( "highColor = %ld\n",     priv.highColor );
	printf( "lowColor = %ld\n",      priv.lowColor );
	printf( "criticalColor = %ld\n", priv.criticalColor );
	printf( "foreground = %ld\n",    priv.foreground );
	printf( "highValue = %d\n",      priv.highValue );
	printf( "lowValue = %d\n",       priv.lowValue );
	printf( "geometry = %s\n",       priv.geometry );*/

	cursor = XCreateFontCursor( XtDisplay( topLevel ), XC_top_left_arrow );
	
	form = XtVaCreateManagedWidget( "form",
									formWidgetClass, topLevel,
									XtNorientation, XtorientHorizontal,
									XtNborderWidth, 0,
									XtNdefaultDistance, 2,
									NULL );
   
    label = XtVaCreateManagedWidget( "label",
									 labelWidgetClass, form,
									 XtNleft, XtChainLeft,
									 XtNinternalHeight, 0,
									 XtNinternalWidth, 0,
									 XtNborderWidth, 0,
									 XtNlabel, "Status",
									 NULL );
	
	XtVaGetValues( label, XtNfont, &fs, NULL );
	fontWidth  = fs->max_bounds.width;
	fontHeight = fs->max_bounds.ascent + fs->max_bounds.descent;
	XtVaSetValues( label, XtNwidth, fontWidth * 8, NULL );
	
	scrollbar = XtVaCreateManagedWidget( "scrollbar",
										 scrollbarWidgetClass, form,
										 XtNhorizDistance, 3,
										 XtNfromHoriz, label,
										 XtNorientation, XtorientHorizontal,
										 XtNscrollHCursor, cursor,
										 XtNthickness, fontHeight,
										 XtNlength, (width > fontWidth*4 - 6)
										 ? width - fontWidth * 4 - 6
										 : fontWidth * 4,
										 NULL );
	
	XawScrollbarSetThumb( scrollbar, 0.0, 0.0 );
/*	XtVaSetValues( scrollbar,
				   XtNtranslations, XtParseTranslationTable( "" ), NULL );
				   */
	XtRealizeWidget( topLevel );
	timerId = XtAppAddTimeOut( app_context, 0, update, app_context );
	XtAppMainLoop( app_context );
	
	return 0;
}
