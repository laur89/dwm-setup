/*
Configuration file for DWM.
Maintainer: illusionist
https://www.github.com/nixmeal
*/
/*Appearance*/
#include "push.c"
#include "moveresize.c"
#include "bstack.c"
#define NUMCOLORS 21
#define Button6 6
#define Button7 7
static const char colors[NUMCOLORS][ColLast][21] = {
    // border     fg         bg
    { "#ababab", "#ADADAD", "#020202" },  // 01 - normal
    //{ "#0086BB", "#20b2e7", "#020202" },  // old selected
    { "#060AE0", "#20b2e7", "#020202" },  // 02 - selected   (ka selected tag color)
    { "#B3354C", "#B3354C", "#020202" },  // 03 - urgent
    { "#118900", "#EFEFEF", "#020202" },  // 04 - (Occupied Color)
    { "#20b2e7", "#20b2e7", "#020202" },  // 05 - Light Blue
    { "#00dd00", "#00dd00", "#020202" },  // 06 - green
    { "#00dd00", "#FFFFFF", "#020202" },  // 07 - white
    { "#3995BF", "#FFFFFF", "#B3277E" },  // 08 - white text; magenta bg
    // STATUSBAR COLORS:
//    { "#000000", "#000000", "#000000" },  // unusable
    { "#877C43", "#020202", "#FFD73E" },  // 09 - yellow bg; black text
    { "#337373", "#FFD73E", "#020202" },  // 0A - yellow text; black bg

    { "#1C678C", "#020202", "#5555E8" },  // 0B - blue bg; black txt
    { "#808080", "#5555E8", "#020202" },  // 0C - blue txt; black bg
    { "#FFEE00", "#FFFFFF", "#5555E8" },  // 0D - white txt; blue bg

    { "#E300FF", "#020202", "#4E5C67" },  // 0E - grey bg; black txt
    { "#E300FF", "#4E5C67", "#020202" },  // 0F - grey txt; black bg
    { "#E300FF", "#FFFFFF", "#4E5C67" },  // 10 - grey bg; white txt

    { "#E300FF", "#FFFFFF", "#F15A25" },  // 11 - white txt; brght_orange_bg
    { "#E300FF", "#020202", "#F15A25" },  // 12 - brght_orange_bg; black text
    { "#4C4C4C", "#F15A25", "#020202" },  // 13 - brght_orange_txt; black bg

    { "#B1D354", "#020202", "#B3277E" },  // 14 - magenta bg; black text
    { "#BF9F5F", "#B3277E", "#020202" },  // 15 - magenta text; black bg
//    { "#3995BF", "#B3277E", "#FFFFFF" },  // 16 - white text; magenta bg
//    { "#A64286", "#A64286", "#020202" },  // 12 - light magenta
//    { "#6C98A6", "#6C98A6", "#020202" },  // 13 - light cyan
//    { "#FFA500", "#FFA500", "#020202" },  // 14 - orange
//    { "#0300ff", "#0300ff", "#802635" },  // 15 - warning
};

// Enda kopitud color-settings:
//static const char normbordercolor[] = "#444444";
//static const char selbordercolor[]  = "#005577";
static const char normbgcolor[]     = "#222222";    // see ja normfgcolor peaksid bar-i data olema
static const char normfgcolor[]     = "#bbbbbb";
static const char selbgcolor[]      = "#005577";
static const char selfgcolor[]      = "#eeeeee";
// [end]

static const char font[]					= "Terminus2 10";
static const unsigned int borderpx  		= 2;        	// border pixel of windows
static const unsigned int snap         		= 20;     	// snap pixel
static const unsigned int gappx				= 0;		// gap pixel between windows (uselessgaps patch)
static const Bool showbar               	= True;  	// False means no bar
static const Bool topbar                	= True;  	// False means bottom bar
static const unsigned int systrayspacing 	= 2;   		// systray spacing
static const Bool showsystray       		= True;     	// False means no systray
static const Bool transbar					= False;		// True means transparent status bar

/* Layout(s) */
static const float mfact      			= 0.55;  	// factor of master area size [0.05..0.95]
static const int nmaster      			= 1;     	// number of clients in master area
static const Bool resizehints 			= False; 	// True means respect size hints in tiled resizals
static const Layout layouts[] = {
	/* symbol	function */
    { "\uea01",	tile },
	{ "[ÿ]",	monocle },    		/* first entry is default */
	{ "\uea05",	NULL },    		/* no layout function means floating behavior */
	{ "\uea02",	bstack },
	{ "\uea06",	gaplessgrid },
};

/* Tagging */
//static const char *tags[] = { "/u011a", "ä", "ä", "ä", "ä", "BG"};
static const Tag tags[] = {
    /* name    layout       mfact  nmaster*/
    { "[web]",	&layouts[0], 	-1,    	-1 },
    { "[dev]", 	&layouts[3], 	0.63,  	-1 },
    { "[chat]",  &layouts[0], 	-1,    	-1 },
    { "[term]", 	&layouts[0], 	-1,    	-1 },
    { "[FM]",  &layouts[0], 	-1,    	-1 },
    { "[docs]", 	&layouts[0], 	-1,    	-1 },
    { "[media]", 	&layouts[0], 	-1,    	-1 },
    { "[misc]", 	&layouts[0], 	-1,    	-1 },
	{ "BG",	&layouts[0],	-1,		-1 },
};

static const Rule rules[] = {
	/* class      		instance	title		tags mask	isfloating 	iscenterd	monitor */
//	{ NULL,		NULL,		"Private Browsing - Vimperator (Private Browsing)",	    1 << 7,	  	False,		False, 		-1 },

	{ "Chromium",		NULL,		NULL,	    1 << 0,	  	False,		False, 		-1 },
	{ "Iceweasel",		NULL,		NULL,	    1 << 0,	  	False,		False, 		-1 },
	{ "Icedove",		NULL,		"Write:",	    1 << 0,	  	True,		False, 		-1 },
	//{ "Icedove",		NULL,		"New Event",	    1 << 0,	  	True,		False, 		-1 },
	{ "Geany",		NULL,		NULL,	    1 << 1,	  	False,		False, 		-1 },
	{ "Gvim",		NULL,		NULL,	    1 << 1,	  	False,		False, 		-1 },
	{ "Eclipse",		NULL,		NULL,	    1 << 1,	  	False,		False, 		-1 },
	{ "Spring Tool Suite",		NULL,		NULL,	    1 << 1,	  	False,		False, 		-1 },
    // Eclipse'i splash:
	{ "Java",		NULL,		"Eclipse",	    1 << 1,	  	True,		False, 		-1 },
    // Android dev stuff:
	{ "Android SDK Manager",		NULL,		NULL,	    1 << 1,	  	True,		False, 		-1 },
	{ "emulator64-arm",		NULL,		NULL,	    1 << 1,	  	True,		False, 		-1 },
    //
	{ "MonoDevelop",		NULL,		NULL,	    1 << 1,	  	False,		False, 		-1 },
    // SoapUI:
	{ "FocusProxy",		NULL,		NULL,	    1 << 1,	  	False,		False, 		-1 },
	{ "VirtualBox",		NULL,		NULL,		1 << 1,		False,		False,		-1 },
	{ "Skype",		NULL,		NULL,	    1 << 2,	  	False,		False, 		-1 },
	{ "Pidgin",		NULL,		NULL,	    1 << 2,	  	False,		False, 		-1 },
	{ "URxvt",		NULL,		NULL,	    1 << 3,	  	False,		False, 		-1 },
	{ "Spacefm",		NULL,		NULL,	    1 << 4,	  	False,		False, 		-1 },
    { "libreoffice-writer",		NULL,		NULL,	    1 << 5,	  	False,		False, 		-1 },
    { "libreoffice-calc",		NULL,		NULL,	    1 << 5,	  	False,		False, 		-1 },
    { "libreoffice-startcenter",		NULL,		NULL,	    1 << 5,	  	False,		False, 		-1 },
    { "Evince",		NULL,		NULL,	    1 << 5,	  	False,		False, 		-1 },
    { "Xpdf",		NULL,		NULL,	    1 << 5,	  	False,		False, 		-1 },
    { "Calibre-ebook-viewer",		NULL,		NULL,	    1 << 5,	  	False,		False, 		-1 },
    { "Calibre-gui",		NULL,		NULL,	    1 << 5,	  	False,		False, 		-1 },
	{ "Ario",		NULL,		NULL,	    1 << 6,	  	False,		False, 		-1 },
	{ "Smplayer",		NULL,		NULL,	    1 << 6,	  	False,		False, 		-1 },
	{ "mplayer",		NULL,		NULL,	    1 << 6,	  	False,		False, 		-1 },
	{ "Vlc",		NULL,		NULL,	    1 << 6,	  	False,		False, 		-1 },
	{ "Spotify",		NULL,		NULL,	    1 << 6,	  	False,		False, 		-1 },
	{ "Clementine",		NULL,		NULL,	    1 << 6,	  	False,		False, 		-1 },
//	{ "Gimp",     		"Layers - Brushes",       NULL,       1 << 6,     True,      False,		-1 },
//	{ "Gimp",     		"Toolbox - Tool Options",       NULL,       1 << 6,     True,      False,		-1 },
	{ "Gimp",     		NULL,       NULL,       1 << 6,     True,      False,		-1 },

	{ NULL,		NULL,		"KeePass Password Safe",	    1 << 8,	  	True,		False, 		-1 },
	{ "Keepassx",		NULL,		"Auto-Type - KeePassX",		NULL,			True,		True,		-1 },
	{ "Keepassx",		NULL,		NULL,		1 << 8,			True,		True,		-1 },
	{ "Truecrypt",		NULL,		NULL,		1 << 8,			True,		True,		-1 },
	{ "Deluge",		NULL,		NULL,		1 << 8,			True,		True,		-1 },
	{ "Transmission-gtk",		NULL,		NULL,		1 << 8,			True,		True,		-1 },

	{ "Galculator",		NULL,		NULL,		0,			True,		True,		-1 },
	{ NULL,		NULL,		"MonoDevelop External Console",		1 << 1,			True,		False,		-1 },

	{ "Gsimplecal",		NULL,		NULL,		0,			True,		False,		-1 },
	{ "XCalc",		NULL,		NULL,		0,			True,		False,		-1 },

	//{ NULL,		NULL,		"Sidewise",	    0,	  	True,		False, 		-1 },   // set Sidewise (chromium extension) floating
	//{ "Google-chrome",	NULL,		NULL,		1 << 0,		False,		False,		-1 },
	//{ "Qpaeq",			NULL,		NULL,		0,			True,		True,		-1 },
	//{ "Pavucontrol",	NULL,		NULL,		0,			True,		True,		-1 },
	//{ "Wxcam",			NULL,		NULL,		0,			True,		True,		-1 },
	//{ "Sonata",			NULL,		NULL,		0,			True,		True,		-1 },
	//{ "Dwb",			NULL,		NULL,		1 << 0,		False,		False,		-1 },
	{ NULL,				NULL,		"dload",	1 << 5,		True,		True,  		-1 },
	{ NULL,				NULL,		"tail",		1 << 5,		True,		True,  		-1 },
};

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
//static const char *dmenurun[] 		= 	{ "/home/garry/.scripts/system", "dmenurun", NULL };
//static const char *vollow[] 		=	{ "/home/garry/.scripts/system", "volume", "down", NULL };
//static const char *volhigh[] 		=	{ "/home/garry/.scripts/system", "volume", "up",  NULL };
//static const char *voltoggle[]		=	{ "/home/garry/.scripts/system", "volume", "toggle", NULL };
static const char *shutdown[]		=	{ "/home/garry/.scripts/system", "shutdown", NULL };
static const char *hibernate[]		=	{ "/home/garry/.scripts/system", "hibernate", NULL };
static const char *restart[]		=	{ "/home/garry/.scripts/system", "restart", NULL };
static const char *suspend[]		=	{ "/home/garry/.scripts/system", "suspend", NULL };
static const char *brightup[]		=	{ "/usr/local/bin/set_brightness.sh", "UP", NULL };
static const char *brightdown[]		=	{ "/usr/local/bin/set_brightness.sh", "DOWN", NULL };
//static const char *mouse[]			=	{ "/home/garry/.scripts/system", "mouse", "toggle", NULL };
//static const char *on[]				=	{ "/home/garry/.scripts/system", "net", "on", NULL };
//static const char *off[]			=	{ "/home/garry/.scripts/system", "net", "off", NULL };
//static const char *killnotify[]		=	{ "/home/garry/.scripts/system", "killnotify", NULL };
static const char *screenshot[]		=	{ "/data/dev/scripts/system/screenshot.sh", NULL };
//static const char *translate[]		=	{ "/home/garry/.scripts/dmenu-translate", NULL };
static const char *wallch[]			=	{ "/home/garry/.scripts/wallpaper", "next", NULL };
static const char *wallrev[]		=	{ "/home/garry/.scripts/wallpaper", "prev", NULL };
//static const char *type[]			=	{ "/home/garry/.scripts/type.sh", "kee", NULL };
//static const char *cursorspeed[]	=	{ "xset", "r", "rate", "350", "50", NULL };
//static const char *mpd[]			=	{ "urxvtc", "-e", "ncmpcpp", NULL };
//static const char *killdwm[]		=	{ "killall", "dwm", NULL };
//static const char *scrlock[]		=	{ "/usr/bin/slock", NULL };
static const char *mpdtoggle[] 		=	{ "mpc", "toggle", NULL };
static const char *mpdnext[]		=	{ "mpc", "next", NULL };
static const char *mpdprev[]		=	{ "mpc", "prev", NULL };
static const char *fileman[] 		= 	{ "spacefm", NULL };
//static const char *gmrun[] 			= 	{ "gmrun", NULL  };
static const char *terminal[]  		= 	{ "urxvt", NULL };
static const char *calendar[]  		= 	{ "gsimplecal", NULL };
static const char *menu[]			=	{ "mygtkmenu", "/home/garry/.scripts/menu.txt", NULL };
//static const char *thunarterm[]		=	{ "/home/garry/.scripts/thunarterm", NULL };
//static const char *composite[]		=	{ "/home/garry/.scripts/composite", NULL };
static const char *dmenucmd[] = { "dmenu_run", "-fn", font, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor, "-sf", selfgcolor, NULL };
static const char *keepass[]			=	{ "mono", "/usr/lib/keepass2/KeePass.exe", "--auto-type", NULL };
static const char *lock_screen[]			=	{ "xscreensaver-command", "--lock", NULL };
static const char *battery_status[]			=	{ "/usr/local/bin/battery_status.sh", NULL };
static const char *history_back[]			=	{ "python", "/usr/local/bin/dynamic_button_remap.py", "history_back", NULL };
static const char *history_forward[]			=	{ "python", "/usr/local/bin/dynamic_button_remap.py", "history_forward", NULL };
static const char *tab_back[]			=	{ "python", "/usr/local/bin/dynamic_button_remap.py", "tab_back", NULL };
static const char *tab_forward[]			=	{ "python", "/usr/local/bin/dynamic_button_remap.py", "tab_forward", NULL };
static const char *FF_tabgroup_back[]			=	{ "python", "/usr/local/bin/dynamic_button_remap.py", "FF_tabgroup_back", NULL };
static const char *FF_tabgroup_fwd[]			=	{ "python", "/usr/local/bin/dynamic_button_remap.py", "FF_tabgroup_fwd", NULL };
static const char *statusbar_prev[]			=	{ "python3", "/home/laur/Documents/comp/DWM/bars/py_bar.py", "prev", NULL };
static const char *statusbar_next[]			=	{ "python3", "/home/laur/Documents/comp/DWM/bars/py_bar.py", "next", NULL };
static const char *ask_shutdown[]			=	{ "python", "/usr/local/bin/powerbtn.py", "next", NULL };
//static const char *keyboard_tab_fwd[]			=	{ "python3", "/usr/local/bin/dynamic_button_remap.py", "keyboard_tab_fwd", NULL };

#define Modkey Mod4Mask
#define Altkey Mod1Mask
#define Ctrlkey ControlMask
#define Shiftkey ShiftMask
#define TAGKEYS(KEY,TAG) \
	{ Modkey,                       KEY,      toggleview,           {.ui = 1 << TAG} }, \
	{ Modkey|Shiftkey,           	KEY,      view,     {.ui = 1 << TAG} }, \
	{ Modkey|Ctrlkey,             	KEY,      tag,            {.ui = 1 << TAG} }, \
	{ Modkey|Ctrlkey|Shiftkey, 		KEY,      toggletag,      {.ui = 1 << TAG} },
/* Infoks TAGKEYS kohta:
 *  1. selekteerib viewd, mille tagiga aknaid kuvatakse
 *  2. vahetab view'd (ei tagi)
 *  3. tagib akna konkreetse view'ga (st ei luba mitut tagi per window)
 *  4. antud view'st lahkumata lisab/eemaldab tage (lubab mitu tagi per window)
*/
static Key keys[] = {
	/* modifier                     	key        				function        	argument */
//	{ Modkey,							XK_Escape,				spawn,				{.v = killnotify } },
	{ Altkey,							XK_Return,				spawn,				{.v = terminal } },
//	{ 0,               					XK_F1,      			spawn,      	    {.v = dmenurun } },
	{ Modkey,	 						XK_e,	   				spawn,	   			{.v = fileman} },
	{ Modkey,						 	XK_Pause, 				spawn,	   			{.v = shutdown } },
	{ 0,							    0x1008ff03,				spawn, 				{.v = brightdown} },
	{ 0, 							    0x1008ff02,				spawn,				{.v = brightup} } ,
//	{ Altkey,							XK_F2,      			spawn,	   			{.v = gmrun } },
	//{ Altkey|Ctrlkey,					XK_Delete,				spawn,				{.v = killdwm } },
	{ Altkey|Ctrlkey,					XK_Delete,				spawn,				{.v = lock_screen } },
//	{ 0,								XK_Pause,				spawn,				{.v = cursorspeed}},
	{ 0,								XK_Print,				spawn,				{.v = screenshot}},
//	{ 0,								XK_Scroll_Lock,			spawn,				{.v = scrlock}},
	{ Altkey|Ctrlkey,					XK_h, 					spawn,	   			{.v = hibernate } },
	{ Altkey|Ctrlkey,					XK_r,   				spawn,	   			{.v = restart } },
	{ Altkey|Ctrlkey,					XK_s,					spawn,				{.v = suspend } },
	{ 0,								0x1008ff2a,			    spawn,				{.v = ask_shutdown }},
    { 0,								0x1008ff93,			    spawn,				{.v = battery_status}},
//	{ 0,								0x1008ff11, 		 	spawn,	   			{.v = vollow } },
//	{ 0,								0x1008ff13, 			spawn,	   			{.v = volhigh } },
//	{ 0,								0x1008ff12,				spawn,				{.v = voltoggle } },
	{ Ctrlkey|Altkey,					XK_Right,				spawn,				{.v = wallch} },
	{ Ctrlkey|Altkey,					XK_Left,				spawn,				{.v = wallrev} },
//	{ 0,								0x1008ff1d,				spawn,				{.v = mpd} },
	{ Modkey,						    XK_Prior,				spawn,				{.v = mpdprev} },
	{ Modkey,						    XK_Next,				spawn,				{.v = mpdnext} },
	{ 0,								XK_Pause,				spawn,				{.v = mpdtoggle} },
//	{ 0,								0x1008ff2f,				spawn,				{.v = mouse} },
//	{ Modkey,							XK_s,					spawn,				{.v = type} },
//	{ Modkey,							XK_F11,					spawn,				{.v = on} },
//	{ Modkey,							XK_F12,					spawn,				{.v = off} },
//	{ Modkey,							XK_F7,					spawn,				{.v = translate} },
	{ Modkey|Shiftkey,           		XK_b,      				togglebar,  	   	{0} },
	{ Modkey,           			    XK_j,      				focusstack, 	    {.i = +1 } },
	{ Modkey,           			    XK_k,      				focusstack, 	    {.i = -1 } },
	{ Modkey,           			    XK_i,      				incnmaster, 	    {.i = +1 } },
	{ Modkey,           			    XK_d,      				incnmaster, 	    {.i = -1 } },
	{ Modkey,           			    XK_Left,    		  	setmfact,   	    {.f = -0.05} },
	{ Modkey,           			    XK_Right,   		   	setmfact,   	    {.f = +0.05} },
	{ Modkey,           			    XK_Return, 				zoom,       	    {0} },
	{ Modkey,           			    XK_Tab,    				view,       	    {0} },
	{ Modkey,           			  	XK_c,      				killclient, 	   	{0} },
    { Altkey,           			  	XK_F4,      		    killclient, 	   	{0} },
	{ Modkey,           			    XK_t,      				setlayout,  	    {.v = &layouts[0]} },
	{ Modkey,           			    XK_x,      				setlayout,  	    {.v = &layouts[1]} },
	{ Modkey,           			    XK_f,      				setlayout,  	    {.v = &layouts[2]} },
	{ Modkey,							XK_b,					setlayout,			{.v = &layouts[3]} },
	{ Modkey,							XK_g,					setlayout,			{.v = &layouts[4]} },
	//{ Modkey,         			   	XK_space,  				setlayout,  	    {0} },
    //{ Ctrlkey,					    XK_n,				    spawn,				{.v = keyboard_tab_fwd} },
	{ Modkey,							XK_space,				spawn,				{.v = dmenucmd} },
    { Ctrlkey|Altkey,					XK_a,				    spawn,				{.v = keepass} },
	{ Modkey|Shiftkey,  			    XK_f,  					togglefloating, 	{0} },
	{ Modkey,           			    XK_0,      				view,       	    {.ui = ~0 } },
	{ Modkey|Shiftkey,  			    XK_0,      				tag,        	    {.ui = ~0 } },
	{ Modkey,           			    XK_comma,  				focusmon,   	    {.i = -1 } },
	{ Modkey,           			    XK_period, 				focusmon,   	    {.i = +1 } },
	{ Modkey|Shiftkey,  			    XK_comma,  				tagmon,     	    {.i = -1 } },
	{ Modkey|Shiftkey,  			    XK_period, 				tagmon,     	    {.i = +1 } },
	{ Modkey|Shiftkey,					XK_q,					quit,				{0} },
	{ Modkey|Shiftkey,					XK_r,					reload,				{0} },
	{ Modkey,           			    XK_h,   				cycle,  			{.i = -1} },
	{ Modkey,           			    XK_l,  					cycle,  			{.i = +1} },
	{ Modkey|Shiftkey,					XK_h,					tagcycle,			{.i = -1} },
	{ Modkey|Shiftkey,					XK_l,					tagcycle,			{.i = +1} },
	{ Ctrlkey,          			    XK_Down,  				moveresize, 	   	{.v = (int []){ 0, 25, 0, 0 }}},
	{ Ctrlkey,          			    XK_Up,    				moveresize, 	    {.v = (int []){ 0, -25, 0, 0 }}},
	{ Ctrlkey,          			    XK_Right, 				moveresize, 	    {.v = (int []){ 25, 0, 0, 0 }}},
	{ Ctrlkey,          			    XK_Left,  				moveresize, 	    {.v = (int []){ -25, 0, 0, 0 }}},
	{ Ctrlkey|Shiftkey, 			    XK_Down,  				moveresize, 	    {.v = (int []){ 0, 0, 0, 25 }}},
	{ Ctrlkey|Shiftkey, 			    XK_Up,    				moveresize, 	    {.v = (int []){ 0, 0, 0, -25 }}},
	{ Ctrlkey|Shiftkey, 			    XK_Right, 				moveresize, 	    {.v = (int []){ 0, 0, 25, 0 }}},
	{ Ctrlkey|Shiftkey, 			    XK_Left,  				moveresize, 	    {.v = (int []){ 0, 0, -25, 0 }}},
	{ Modkey|Shiftkey,  			    XK_k,      				pushup,     	 	{.i = +1 } },
   	{ Modkey|Shiftkey,  			    XK_j,     				pushdown,   	   	{.i = -1 } },
	{ Altkey|Shiftkey,  			    XK_Down,   				spawn,      	    SHCMD("xdotool mousemove_relative 0 10")},
	{ Altkey|Shiftkey,  			    XK_Up,     				spawn,      	    SHCMD("xdotool mousemove_relative -- 0 -10")},
	{ Altkey|Shiftkey,  			    XK_Right,  				spawn,      	    SHCMD("xdotool mousemove_relative 10 0")},
	{ Altkey|Shiftkey,  			    XK_Left,   				spawn,      	    SHCMD("xdotool mousemove_relative -- -10 0")},
	//{ Altkey,           			  	XK_1, 					spawn,      	    SHCMD("iocane b 1")},
	//{ Altkey,							XK_2,					spawn,				SHCMD("iocane b 2")},
	//{ Altkey,							XK_3,					spawn,				SHCMD("iocane b 3")},
	//{ Altkey|Ctrlkey,   			   	XK_1, 					spawn,      	    SHCMD("iocane b 1")},
	{ Modkey|Shiftkey,  			    XK_m,      				toggle_ffm, 	    {0} },          // toggle focus follows mouse
	{ Modkey,           			    XK_z,   		   	    toggleview, 	    {.ui = 1 << 8} },
	{ Modkey|Shiftkey,					XK_z,				    tag,				{.ui = 1 << 8} },
	TAGKEYS(            			    XK_1,                      0)
	TAGKEYS(            			    XK_2,                      1)
	TAGKEYS(            			    XK_3,                      2)
	TAGKEYS(            			    XK_4,                      3)
	TAGKEYS(            			    XK_5,                      4)
    TAGKEYS(            			    XK_6,                      5)
    TAGKEYS(            			    XK_7,                      6)
    TAGKEYS(            			    XK_8,                      7)

};

/* button definitions */
/* click can be ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },                         // Swaps between previous and current
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[1]} },   // Right click = monocle and previous, monocle and previous...
	/* --------------------------------------menu-----------------------------------------*/
	{ ClkRootWin,			0,				Button3,		spawn,			{.v = calendar } },         // root win == desktop nt
	{ ClkWinTitle,			0,				Button2,		killclient,		{0} },
	{ ClkWinTitle,          0,  		   	Button1,	    focusstack,     {.i = +1 } },
	{ ClkWinTitle,          0,  		    Button3,	    focusstack,     {.i = -1 } },
	{ ClkStatusText,		0,				Button3,		spawn,			{.v = calendar } },
	/*====================================================================================*/
	{ ClkClientWin,         Altkey,         Button1,        movemouse,      {0} },
	//{ ClkClientWin,         Altkey,         Button2,        togglefloating, {0} },
    //{ ClkClientWin,         Altkey,         Button2,        setlayout, {.v = &layouts[1]} },
    { ClkClientWin,         Altkey,         Button2,        killclient,      {0} },
	{ ClkClientWin,         Altkey,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button3,        view,           {0} },
	{ ClkTagBar,            0,              Button1,        toggleview,     {0} },
	{ ClkTagBar,            Modkey,         Button1,        tag,            {0} },
	{ ClkTagBar,            Modkey,         Button3,        toggletag,      {0} },
    // Custom mouse wheel tilt commands:
    { ClkClientWin,         Altkey,         Button6,        spawn,      {.v = history_back } },
    { ClkClientWin,         Altkey,         Button7,        spawn,      {.v = history_forward} },
    { ClkClientWin,         Ctrlkey,        Button6,        spawn,      {.v = tab_back } },
    { ClkClientWin,         Ctrlkey,        Button7,        spawn,      {.v = tab_forward} },
    { ClkClientWin,         Ctrlkey|Shiftkey,         Button6,        spawn,      {.v = FF_tabgroup_back } },
    { ClkClientWin,         Ctrlkey|Shiftkey,         Button7,        spawn,      {.v = FF_tabgroup_fwd} },
    { ClkClientWin,         Modkey,         Button6,        setmfact,      {.f = -0.05} },
    { ClkClientWin,         Modkey,         Button7,        setmfact,      {.f = +0.05} },
    { ClkWinTitle,          Modkey,         Button7,        incnmaster,      {.i = -1 } },
    { ClkWinTitle,          Modkey,         Button6,        incnmaster,      {.i = +1 } },
    { ClkClientWin,         Modkey,         Button1,        zoom,      {0} },
    { ClkStatusText,		Modkey,		    Button6,		spawn,			{.v = statusbar_prev } },   // selects previous mode for statusbar
    { ClkStatusText,		Modkey,		    Button7,		spawn,			{.v = statusbar_next } },


};
/* vim: set ts=4 sw=4 tw=0: */
