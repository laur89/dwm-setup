/* See LICENSE file for copyright and license details. */

#include "bstack.c"
#include "gaplessgrid.c"
#include "push.c"

/* key definitions */
#define MODKEY Mod4Mask
#define AltMask Mod1Mask

/* btn definitions */
#define wheel_tilt_left 6 // wheel tilt left
#define wheel_tilt_right 7 // wheel tilt right
#define wheel_scroll_up 4 // wheel scroll up
#define wheel_scroll_dn 5 // wheel scroll down

#define NUMCOLORS 18 // needs to be re-defined in dwm.c.MAXCOLORS!
/* appearance */
static const char font[] = "-*-xbmicons-medium-r-*-*-12-*-*-*-*-*-*-*" ","
                           "-*-terminus-medium-r-*-*-12-*-*-*-*-*-*-*";
static const char cellFont[] = "-*-xbmicons-medium-r-*-*-12-*-*-*-*-*-*-*" ","
                               "-*-terminus-medium-r-*-*-24-*-*-*-*-*-*-*";
//static const char font[] = "-*-Terminus2-*-*-*-*-*-*-*-*-*-*-*-*";

static const char colors[NUMCOLORS][ColLast][9] = {
  //  border  foreground  background
  { "#282a2e", "#373b41", "#1d1f21" }, // 1 = normal (grey on black)
  //{ "#f0c674", "#c5c8c6", "#1d1f21" }, // 2 = selected (white on black)
  { "#060AE0", "#c5c8c6", "#1d1f21" }, // 2 = selected (white on black)
  { "#dc322f", "#dc322f", "#1d1f21" }, // 3 = urgent (black on yellow)
  { "#282a2e", "#282a2e", "#1d1f21" }, // 4 = darkgrey on black (for glyphs); and urgent
  { "#282a2e", "#1d1f21", "#282a2e" }, // 5 = black on darkgrey (for glyphs)
  { "#282a2e", "#cc6666", "#1d1f21" }, // 6 = red on black
  { "#282a2e", "#b5bd68", "#1d1f21" }, // 7 = green on black
  { "#282a2e", "#de935f", "#1d1f21" }, // 8 = orange on black
  { "#282a2e", "#f0c674", "#282a2e" }, // 9 = yellow on darkgrey
  { "#282a2e", "#81a2be", "#282a2e" }, // A = blue on darkgrey
  { "#282a2e", "#b294bb", "#282a2e" }, // B = magenta on darkgrey
  { "#282a2e", "#8abeb7", "#282a2e" }, // C = cyan on darkgrey
  { "#FFFFFF", "#7f8185", "#1d1f21" }, // D = a bit lighter gray than normal on black; for tab bar non-selected & tag occupied
  { "#FFFFFF", "#c5c8c6", "#5f6267" }, // E = used for selected title in tab bar
  //{ "#f0c674", "#b5bd68", "#FFFFFF" }, // F = for float client borders; index 0 is selected, 1 is normal, 2 unused
  { "#00A3CC", "#CCFFFF", "#FFFFFF" }, // F = for float client borders; index 0 is selected, 1 is normal, 2 unused
  { "#FFFFFF", "#1d1f21", "#87777C" }, // G = non-selected master client tab (if > 1 nmast)
  { "#FFFFFF", "#E2E4E2", "#9F9195" }, // H = selected master client tab (if > 1 nmast)
  { "#FFFFFF", "#808080", "#2F3133" }, // I = non-selected tab
};

static const unsigned int borderpx  = 2;        /* border pixel of windows */
static const unsigned int snap      = 25;       /* snap pixel */
static const Bool showbar           = True;     /* False means no bar */
static const Bool topbar            = True;     /* False means bottom bar */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const Bool showsystray       = True;     /* False means no systray */
static const Bool transfer_pointer  = True;     /* False means do not move pointer to another mon on monitor change (focusmon() & altTab()) */


// these two cannot be both set to true!: (at least until focus() gets another param
// to check wether enternotif walled it or not)
static /*!const*/ Bool focus_follows_mouse     = True;         /* toggle focus-follows-mouse default */
static /*!const*/ Bool mouse_follows_focus     = False;        /* again, default, since it's toggleable */

static const unsigned int cellWidth = 300;      /* altTab's window width;  TODO: deleteme? */
static const unsigned int tabWidth  = 200;      /* default tab width;
                                                   (if more tabs are added, width is
                                                   decreased so all tabs fit onto the bar) */

/*   Display modes of the tab bar: never shown, always shown, shown only in */
/*   monocle mode in presence of several windows.                           */
/*   Modes after showtab_nmodes are disabled                                */
enum showtab_modes { showtab_never, showtab_auto, showtab_nmodes, showtab_always };
static const int showtab            = showtab_auto; /* Default tab bar show mode */
static const Bool toptab            = True;         /* False means bottom tab bar */
static const Bool resizehints = False; /* True means respect size hints in tiled resizals */
static const Bool saveAndRestoreClientDimesionAndPositionForFloatMode = True;
/* layout(s) */
static const float mfact      = 0.55;  /* factor of master area size [0.05..0.95] */
static const int nmaster      = 1;     /* number of clients in master area */


enum lyts { Tile, Float, Monocle, Bstack, Grid, Deck };
static const Layout layouts[] = {
  /* symbol     arrange function */
  /*
   *{ "\uE019 \uE009 \uE019",    tile },    [> first entry is default <]
   *{ "\uE019 \uE00A \uE019",    NULL },    [> no layout function means floating behavior <]
   *{ "\uE019 \uE00B \uE019",    monocle },
   *{ "\uE019 \uE00C \uE019",    bstack },
   *{ "\uE019 \uE00D \uE019",    gaplessgrid },
   */
  { " \uE009 ",    tile },    /* first entry is default */
  { " \uE00A ",    NULL },    /* no layout function means floating behavior */
  { " \uE00B ",    monocle },
  { " \uE00C ",    bstack },
  { " \uE00D ",    gaplessgrid },
  { " D ",         deck },
};

/* Tagging */
//static const char *tags[] = { "/u011a", "ä", "ä", "ä", "ä", "BG"};
static const Tag tags[] = {
    /* name          layout            mfact   nmaster*/
    { "\uE000",    &layouts[Tile],         -1,        -1 },
    { "\uE008",    &layouts[Bstack],     0.63,      -1 },
    { "\uE001",    &layouts[Tile],         -1,        -1 },
    { "\uE002",    &layouts[Tile],         -1,        -1 },
    { "\uE005",    &layouts[Tile],         -1,        -1 },
    { "\uE003",    &layouts[Tile],         -1,        -1 },
    { "\uE006",    &layouts[Tile],         -1,        -1 },
    { "\uE007",    &layouts[Tile],         -1,        -1 },
    { "BG",           &layouts[Tile],        -1,        -1 }
};

static const Rule rules[] = {
    /* class              instance    title        tags mask    isfloating     iscenterd    monitor */
//    { NULL,        NULL,        "Private Browsing - Vimperator (Private Browsing)",        1 << 7,          False,        False,         -1 },

    { "Chromium",        NULL,        NULL,        1 << 0,          False,        False,         -1 },
    { "Iceweasel",        NULL,        NULL,        1 << 0,          False,        False,         -1 },
    { "Firefox",        NULL,        NULL,        1 << 0,          False,        False,         -1 },

    { "Icedove",        NULL,        "Write:",        1 << 0,          True,        False,         -1 },
    //{ "Icedove",        NULL,        "New Event",        1 << 0,          True,        False,         -1 },
    { "Geany",        NULL,        NULL,        1 << 1,          False,        False,         -1 },
    { "Gvim",        NULL,        NULL,        1 << 1,          False,        False,         -1 },
    { "Eclipse",        NULL,        NULL,        1 << 1,          False,        False,         -1 },

    // IDEA:
    { "jetbrains-idea",        NULL,        NULL,        1 << 1,          False,        False,         -1 },
    //{ "sun-awt-X11-XFramePeer",        NULL,        NULL,        1 << 1,          False,        False,         -1 },
    //{ "sun-awt-X11-XDialogPeer",        NULL,        NULL,        1 << 1,          False,        False,         -1 },

    { "Spring Tool Suite",        NULL,        NULL,        1 << 1,          False,        False,         -1 },
    { NULL,        NULL,        "Spring - Spring Tool Suite ",        1 << 1,          False,        False,         -1 },
    // Eclipse'i splash:
    { "Java",        NULL,        "Eclipse",        1 << 1,          True,        False,         -1 },
    // Android dev stuff:
    { "Android SDK Manager",        NULL,        NULL,        1 << 1,          True,        False,         -1 },
    { "emulator64-arm",        NULL,        NULL,        1 << 1,          True,        False,         -1 },
    //
    { "MonoDevelop",        NULL,        NULL,        1 << 1,          False,        False,         -1 },
    // SoapUI:
    { "FocusProxy",        NULL,        NULL,        1 << 1,          False,        False,         -1 },
    { "VirtualBox",        NULL,        NULL,        1 << 1,        False,        False,        -1 },
    { "Skype",        NULL,        NULL,        1 << 2,          False,        False,         -1 },
    { "Slack",        NULL,        NULL,        1 << 2,          False,        False,         -1 },
    { "Franz",        NULL,        NULL,        1 << 2,          False,        False,         -1 },
    { "Pidgin",        NULL,        NULL,        1 << 2,          False,        False,         -1 },
    { "URxvt",        NULL,        NULL,        1 << 3,          False,        False,         -1 },
    { "Spacefm",        NULL,        NULL,        1 << 4,          False,        False,         -1 },
    { "libreoffice",        NULL,        NULL,        1 << 5,          False,        False,         -1 },
    { "LibreOffice",        NULL,        NULL,        1 << 5,          False,        False,         -1 },
    //{ "libreoffice-calc",        NULL,        NULL,        1 << 5,          False,        False,         -1 },
    //{ "libreoffice-startcenter",        NULL,        NULL,        1 << 5,          False,        False,         -1 },
    { "Evince",        NULL,        NULL,        1 << 5,          False,        False,         -1 },
    { "Zathura",        NULL,        NULL,        1 << 5,          False,        False,         -1 },
    { "Xpdf",        NULL,        NULL,        1 << 5,          False,        False,         -1 },
    //{ NULL,        "calibre - || calibre_library ||",        NULL,        1 << 5,          False,        False,         -1 },
    //{ "Calibre-gui",        NULL,        NULL,        1 << 5,          False,        False,         -1 },
    // libprs500 is calibre; for some reason title matching is not working:
    { "libprs500",        NULL,        NULL,    1 << 5,          False,        False,         -1 },
    //{ "Calibre-ebook-viewer",        NULL,        NULL,        1 << 5,          False,        False,         -1 },
    { "Ario",        NULL,        NULL,        1 << 6,          False,        False,         -1 },
    { "Smplayer",        NULL,        NULL,        1 << 6,          False,        False,         -1 },
    { "mpv",        NULL,        NULL,        1 << 6,          False,        False,         -1 },
    { "mplayer",        NULL,        NULL,        1 << 6,          False,        False,         -1 },
    { "vlc",        NULL,        NULL,        1 << 6,          False,        False,         -1 },
    { "Spotify",        NULL,        NULL,        1 << 6,          False,        False,         -1 },
    { "Clementine",        NULL,        NULL,        1 << 6,          False,        False,         -1 },
//    { "Gimp",             "Layers - Brushes",       NULL,       1 << 6,     True,      False,        -1 },
//    { "Gimp",             "Toolbox - Tool Options",       NULL,       1 << 6,     True,      False,        -1 },
    { "Gimp",             NULL,       NULL,       1 << 6,     True,      False,        -1 },
    { "Steam",        NULL,        NULL,        1 << 6,          False,        False,         -1 },

    { "GitKraken",          NULL,       NULL,       1 << 7,     False,      False,      -1 },
    { "SWT",             NULL,       "CollabNet GitEye",       1 << 7,     False,      False,        -1 },

    //{ "keepassx",        "keepassx",        "KeePassX",        1 << 8,            True,        True,        -1 },
    { "keepassx",        "keepassx",        "passwd_db.kdbx",        1 << 8,            True,        True,        -1 },
    { "keepassx",        "keepassx",        "Laur's passes",        1 << 8,            True,        True,        -1 },
    { "Truecrypt",        NULL,        NULL,        1 << 8,            True,        True,        -1 },
    { "Deluge",        NULL,        NULL,        1 << 8,            True,        True,        -1 },
    { "Transmission-gtk",        NULL,        NULL,        1 << 8,            True,        True,        -1 },
    { NULL,        NULL,        "MonoDevelop External Console",        1 << 1,            True,        False,        -1 },


    //{ "Google-chrome",    NULL,        NULL,        1 << 0,        False,        False,        -1 },
    //{ "Qpaeq",            NULL,        NULL,        0,            True,        True,        -1 },
    //{ "Pavucontrol",    NULL,        NULL,        0,            True,        True,        -1 },
    //{ "Wxcam",            NULL,        NULL,        0,            True,        True,        -1 },
    //{ "Sonata",            NULL,        NULL,        0,            True,        True,        -1 },
    //{ "Dwb",            NULL,        NULL,        1 << 0,        False,        False,        -1 },
    { NULL,                NULL,        "dload",    1 << 5,        True,        True,          -1 },
    { NULL,                NULL,        "tail",        1 << 5,        True,        True,          -1 },
    { "Copyq",        NULL,        NULL,        NULL,            True,        False,        -1 },
    // qpaeq is pulseaudio-equalizer front end:
    { "Qpaeq",        NULL,        NULL,        NULL,            True,        True,        -1 },
    { "Pavucontrol",        NULL,        NULL,        NULL,            True,        True,        -1 },
    { "vokoscreen",        NULL,        NULL,        NULL,            True,        True,        -1 },
    { "Shutter",        NULL,        NULL,        NULL,            True,        False,        -1 },
    { "Main.py",        "guake",        "Guake!",        NULL,            True,        False,        -1 },

    { "Galculator",        NULL,        NULL,        0,            True,        True,        -1 },
    { "Gsimplecal",        NULL,        NULL,        0,            True,        False,        -1 },
    { "XCalc",        NULL,        NULL,        0,            True,        True,        -1 },
    { "keepassx",        "keepassx",        "Auto-Type - KeePassX",        NULL,            True,        True,        -1 },
    { "Screenkey",        "screenkey",        "screenkey",        NULL,            True,        False,        -1 }
};
/*
 *static const Rule rules[] = {
 *  [> class                      instance     title  tags mask isfloating  iscentred   monitor <]
 *  { "feh",                      NULL,        NULL,  0,        True,       True,       -1 },
 *  { "Gcolor2",                  NULL,        NULL,  0,        True,       True,       -1 },
 *  { "XFontSel",                 NULL,        NULL,  0,        True,       True,       -1 },
 *  { "Xfd",                      NULL,        NULL,  0,        True,       True,       -1 },
 *  { "Firefox",                  NULL,        NULL,  1,        False,      False,      -1 },
 *  { "URxvt",                    "ircmailbt", NULL,  1 << 1,   False,      False,      -1 },
 *  { "Gvim",                     NULL,        NULL,  1 << 2,   False,      False,      -1 },
 *  { "Zathura",                  NULL,        NULL,  1 << 3,   False,      False,      -1 },
 *  { "jetbrains-android-studio", NULL,        NULL,  1 << 3,   False,      False,      -1 },
 *  { "libreoffice-calc",         NULL,        NULL,  1 << 3,   False,      False,      -1 },
 *  { "libreoffice-impress",      NULL,        NULL,  1 << 3,   False,      False,      -1 },
 *  { "libreoffice-startcenter",  NULL,        NULL,  1 << 3,   False,      False,      -1 },
 *  { "libreoffice-writer",       NULL,        NULL,  1 << 3,   False,      False,      -1 },
 *  { "mpv",                      NULL,        NULL,  1 << 4,   False,      False,      -1 },
 *  { "fontforge",                NULL,        NULL,  1 << 5,   True,       True,       -1 },
 *  { "Gimp",                     NULL,        NULL,  1 << 5,   True,       True,       -1 },
 *  { "PacketTracer6",            NULL,        NULL,  1 << 5,   True,       True,       -1 },
 *  { "TeamViewer.exe",           NULL,        NULL,  1 << 5,   True,       True,       -1 },
 *  { "Wine",                     NULL,        NULL,  1 << 5,   True,       True,       -1 },
 *  { "URxvt",                    "filemgr",   NULL,  1 << 6,   False,      False,      -1 },
 *  { "Chromium",                 NULL,        NULL,  1 << 7,   False,      False,      -1 },
 *  { "Google-chrome-stable",     NULL,        NULL,  1 << 7,   False,      False,      -1 },
 *};
 */


#define TAGKEYS(KEY,TAG) \
    { MODKEY,                       KEY,      toggleview, {.ui = 1 << TAG} }, \
    { MODKEY|ShiftMask,               KEY,      view,       {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask,              KEY,      tag,        {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask|ShiftMask,    KEY,      toggletag,  {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
//static const char *dmenucmd[]      = { "dmenu_run", "-i", "-fn", font, "-nb", colors[12][ColBG], "-nf", colors[12][ColFG], "-sb", colors[1][ColBG], "-sf", colors[1][ColFG], NULL };
static const char *dmenucmd[]      = { "dmenu_recent_aliases", NULL };
static const char *termcmd[]       = { "urxvtc", NULL };
static const char scratchpadname[] = "scratchpad";
static const char *scratchpadcmd[] = { "urxvtc", "-name", scratchpadname, "-geometry", "100x25", NULL };
static const char *volupcmd[]      = { "amixer", "set", "Master", "5%+", NULL };
static const char *voldncmd[]      = { "amixer", "set", "Master", "5%-", NULL };
static const char *mpctog[]        = { "mpc", "-q", "toggle", NULL };
static const char *mpcprev[]       = { "mpc", "-q", "prev", NULL };
static const char *mpcnext[]       = { "mpc", "-q", "next", NULL };

// siit minu omad:
static const char *screenshot[]        =    { "/data/dev/scripts/system/screenshot.sh", "--auto", NULL };
static const char *calendar[]          =     { "gsimplecal", NULL };
static const char *lock_screen[]            =    { "xscreensaver-command", "--lock", NULL };

static const char *history_back[]            =    { "python", "/usr/local/bin/dynamic_button_remap.py", "history_back", NULL };
static const char *history_forward[]            =    { "python", "/usr/local/bin/dynamic_button_remap.py", "history_forward", NULL };
static const char *py_tab_back[]            =    { "python", "/usr/local/bin/dynamic_button_remap.py", "tab_back", NULL };
static const char *py_tab_forward[]            =    { "python", "/usr/local/bin/dynamic_button_remap.py", "tab_forward", NULL };
static const char *py_FF_tabgroup_back[]            =    { "python", "/usr/local/bin/dynamic_button_remap.py", "FF_tabgroup_back", NULL };
static const char *py_FF_tabgroup_fwd[]            =    { "python", "/usr/local/bin/dynamic_button_remap.py", "FF_tabgroup_fwd", NULL };
static const char *py_statusbar_prev[]            =    { "python3", "/home/laur/.dwm/bars/py_bar.py", "prev", NULL };
static const char *py_statusbar_next[]            =    { "python3", "/home/laur/.dwm/bars/py_bar.py", "next", NULL };

// runorraise compliant:
static const char *keepassx[] = { "keepassx", NULL, NULL, NULL, "Keepassx" };

static Key keys[] = {
  /* modifier               key               function        argument */
  //{ 0,                 XK_Alt_L,         0,          {0} },
  { MODKEY,                 XK_space,         spawn,          {.v = dmenucmd } },
  { MODKEY|ShiftMask,       XK_Return,        spawn,          {.v = termcmd } },
  { MODKEY,                 XK_s,             togglescratch,  {.v = scratchpadcmd} },
  { 0,                      XK_Print,         spawn,          {.v = screenshot}},
  { AltMask|ControlMask,    XK_Delete,        spawn,          {.v = lock_screen } },
  { MODKEY,                 XK_KP_Add,        spawn,          {.v = volupcmd } }, // numpad +
  { MODKEY,                 XK_plus,          spawn,          {.v = volupcmd } }, // numpad +
  { MODKEY,                 XK_KP_Subtract,   spawn,          {.v = voldncmd } }, // numpad -
  { MODKEY,                 XK_minus,         spawn,          {.v = voldncmd } }, // numpad -
  { MODKEY,                 XK_Pause,         spawn,          {.v = mpctog } },
  { 0,                      XK_Pause,         spawn,          {.v = mpctog } },
  { MODKEY,                 XK_Prior,         spawn,          {.v = mpcprev } },
  { MODKEY,                 XK_Next,          spawn,          {.v = mpcnext } },
  { MODKEY|ShiftMask,       XK_b,             togglebar,      {0} },
  { MODKEY,                 XK_j,             focusstack,     {.i = +1 } },
  { MODKEY,                 XK_k,             focusstack,     {.i = -1 } },
  { MODKEY|ControlMask|ShiftMask,                 XK_j,             focusstackwithoutrising,     {.i = +1 } },
  { MODKEY|ControlMask|ShiftMask,                 XK_k,             focusstackwithoutrising,     {.i = -1 } },
  { MODKEY|ControlMask,                 XK_j,             focusstackfloatingonly,     {.i = +1 } },
  { MODKEY|ControlMask,                 XK_k,             focusstackfloatingonly,     {.i = -1 } },
  // support exists, simply not using:
    /*
     *{ Modkey,                           XK_h,                   cycle,              {.i = -1} },
     *{ Modkey,                           XK_l,                      cycle,              {.i = +1} },
     *{ Modkey|Shiftkey,                    XK_h,                    tagcycle,            {.i = -1} },
     *{ Modkey|Shiftkey,                    XK_l,                    tagcycle,            {.i = +1} },
     */
  { MODKEY|ShiftMask,       XK_j,             pushdown,       {0} },
  { MODKEY|ShiftMask,       XK_k,             pushup,         {0} },
  { MODKEY,                 XK_i,             incnmaster,     {.i = +1 } },
  { MODKEY,                 XK_d,             incnmaster,     {.i = -1 } },
  { MODKEY,                 XK_h,             setmfact,       {.f = -0.05} },
  { MODKEY,                 XK_l,             setmfact,       {.f = +0.05} },
{ MODKEY|ShiftMask,             XK_l,      setcfact,       {.f = +0.25} },
{ MODKEY|ShiftMask,             XK_h,      setcfact,       {.f = -0.25} },
{ MODKEY|ShiftMask,             XK_o,      setcfact,       {.f =  0.00} },
{ MODKEY|ControlMask|ShiftMask, XK_o,      resetcfactall,       NULL },
  { MODKEY,                 XK_Return,        zoom,           {0} },
  { MODKEY,                 XK_Tab,           view,           {0} },
  { AltMask,                XK_Tab,           altTab,           {0} },
  { MODKEY,                     XK_c,             killclient,     {0} },
  { AltMask,                   XK_F4,            killclient,     {0} },
  { MODKEY,                 XK_t,             setlayout,      {.v = &layouts[Tile]} },
  { MODKEY,                 XK_f,             setlayout,      {.v = &layouts[Float]} },
  { MODKEY,                 XK_x,             setlayout,      {.v = &layouts[Monocle]} },
  { MODKEY,                 XK_b,             setlayout,      {.v = &layouts[Bstack]} },
  { MODKEY,                 XK_g,             setlayout,      {.v = &layouts[Grid]} },
  { MODKEY,                 XK_m,             setlayout,      {.v = &layouts[Deck]} },
  //{ MODKEY,                 XK_space,         setlayout,      {0} },
  //{ MODKEY|ShiftMask,       XK_space,         togglefloating, {0} },
  { MODKEY|ShiftMask,        XK_f,             togglefloating, {0} },
  { MODKEY,                 XK_0,             view,           {.ui = ~0 } },
  { MODKEY|ShiftMask,       XK_0,             tag,            {.ui = ~0 } },

  // server: (server-client separation was meant for synergy-dwm hack, which never worked)
  //{ MODKEY,                 XK_o,         focusmon,       {.i = -1 } },
  //{ MODKEY,                 XK_period,        focusmon,       {.i = +1 } },
  //{ MODKEY|ControlMask,                 XK_comma,         focusmon,       {.i = -1 } },
  //{ MODKEY|ControlMask,                 XK_period,        focusmon,       {.i = +1 } },

  // client:
  { MODKEY,                 XK_comma,         focusmon,       {.i = -1 } },
  { MODKEY,                 XK_period,        focusmon,       {.i = +1 } },
  // these two so that synergy could call focusmon():
  { ControlMask|MODKEY,                 XK_comma,         focusmon,       {.i = -1 } },
  { ControlMask|MODKEY,                 XK_period,        focusmon,       {.i = +1 } },

  { MODKEY|ShiftMask,       XK_comma,         tagmon,         {.i = -1 } },
  { MODKEY|ShiftMask,       XK_period,        tagmon,         {.i = +1 } },
  { MODKEY|ShiftMask,       XK_r,             reload,         {0} },
//{ MODKEY,                       XK_e,      runorraise,     {.v = keepassx } },
    { MODKEY|ShiftMask,                  XK_m,                      toggle_ffm,         NULL },          // toggle focus follows mouse
    { MODKEY|ShiftMask|ControlMask,        XK_m,                      toggle_mff,         NULL },          // toggle mouse follows focus
    { MODKEY,                           XK_z,                   toggleview,         {.ui = 1 << 8} },
    { MODKEY|ControlMask,               XK_z,                tag,                {.ui = 1 << 8} },
    TAGKEYS(                  XK_1,                             0)
    TAGKEYS(                  XK_2,                             1)
    TAGKEYS(                  XK_3,                             2)
    TAGKEYS(                  XK_4,                             3)
    TAGKEYS(                  XK_5,                             4)
    TAGKEYS(                  XK_6,                             5)
    TAGKEYS(                  XK_7,                             6)
    TAGKEYS(                  XK_8,                             7)
    //TAGKEYS(                  XK_9,                             8)
    { MODKEY|ShiftMask,       XK_q,             quit,           {0} },
};

#include "tilemovemouse.c"

/* button definitions */
/* click can be ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
  /* click                event mask      button          function        argument */
    { ClkLtSymbol,          0,              Button1,        setlayout,      {0} },                         // Swaps between previous and current
    { ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[Float]} },   // Right click = monocle and previous, monocle and previous...
    /* --------------------------------------menu-----------------------------------------*/
    { ClkRootWin,            0,                Button3,        spawn,            {.v = calendar } },         // root win == desktop nt
    { ClkWinTitle,            0,                Button2,        killclient,        {0} },
    { ClkWinTitle,          0,                 Button1,        focusstack,     {.i = +1 } },
    { ClkWinTitle,          0,              Button3,        focusstack,     {.i = -1 } },
    { ClkStatusText,        0,                Button3,        spawn,            {.v = calendar } },
    /*====================================================================================*/
    { ClkClientWin,         AltMask,         Button1,        movemouse,      {0} },
    //{ ClkClientWin,         Altkey,         Button2,        togglefloating, {0} },
    //{ ClkClientWin,         Altkey,         Button2,        setlayout, {.v = &layouts[1]} },
    { ClkClientWin,         AltMask,         Button2,        killclient,      {0} },
    { ClkClientWin,         AltMask,         Button3,        resizemouse,    {0} },
    { ClkTagBar,            0,              Button3,        view,           {0} },
    { ClkTagBar,            0,              Button1,        toggleview,     {0} },
    { ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
    { ClkClientWin,         MODKEY|ShiftMask,         Button1,        tilemovemouse,  {0} },
    { ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
    { ClkTabBar,            0,              Button1,        focuswin,      {0} },
    // Custom mouse wheel tilt&scroll commands:
    { ClkClientWin,         AltMask,         wheel_tilt_left,        hist_back,      {0} },
    { ClkClientWin,         AltMask,         wheel_tilt_right,        hist_fwd,      {0} },
    { ClkClientWin,         ControlMask,        wheel_tilt_left,        tab_back,      {0} },
    { ClkClientWin,         ControlMask,        wheel_tilt_right,        tab_fwd,      {0} },
    //{ ClkClientWin,         ControlMask,        wheel_tilt_left,        spawn,      {.v = py_tab_back } },
    //{ ClkClientWin,         ControlMask,        wheel_tilt_right,        spawn,      {.v = py_tab_forward} },
    //{ ClkClientWin,         AltMask,         wheel_tilt_left,        spawn,      {.v = history_back } },
    //{ ClkClientWin,         AltMask,         wheel_tilt_right,        spawn,      {.v = history_forward} },
    { ClkClientWin,         ControlMask|ShiftMask,         wheel_tilt_left,        spawn,      {.v = py_FF_tabgroup_back } },
    { ClkClientWin,         ControlMask|ShiftMask,         wheel_tilt_right,        spawn,      {.v = py_FF_tabgroup_fwd} },
    { ClkClientWin,         MODKEY,         wheel_tilt_left,        setmfact,      {.f = -0.05} },
    { ClkClientWin,         MODKEY,         wheel_tilt_right,        setmfact,      {.f = +0.05} },
    { ClkWinTitle,          MODKEY,         wheel_tilt_right,        incnmaster,      {.i = -1 } },
    { ClkWinTitle,          MODKEY,         wheel_tilt_left,        incnmaster,      {.i = +1 } },
    { ClkClientWin,         MODKEY,         Button1,        zoom,      {0} },
    { ClkClientWin,         MODKEY,          wheel_scroll_up,        focusstack,     {.i = +1 } },
    { ClkClientWin,         MODKEY,          wheel_scroll_dn,        focusstack,     {.i = -1 } },
    { ClkClientWin,         MODKEY|ControlMask,          wheel_scroll_up,        focusstackwithoutrising,     {.i = +1 } },
    { ClkClientWin,         MODKEY|ControlMask,          wheel_scroll_dn,        focusstackwithoutrising,     {.i = -1 } },

    { ClkStatusText,        MODKEY,            wheel_tilt_left,        spawn,            {.v = py_statusbar_prev } },   // selects previous mode for statusbar (py_bar)
    { ClkStatusText,        MODKEY,            wheel_tilt_right,        spawn,            {.v = py_statusbar_next } },
    { ClkStatusText,        0,              wheel_scroll_up,      spawn,          {.v = volupcmd } },
    { ClkStatusText,        0,              wheel_scroll_dn,      spawn,          {.v = voldncmd } },

/*
 *  { ClkLtSymbol,          0,              Button1,        setlayout,      {0} },  // Swaps between previous and current
 *  { ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[1]} },  // Right click = monocle and previous, monocle and previous...
 *
 *  //{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
 *  //{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
 *  [> --------------------------------------menu-----------------------------------------<]
 *  { ClkRootWin,           0,              Button3,        spawn,            {.v = calendar } },         // root win == desktop nt
 *  { ClkWinTitle,          0,              Button2,        killclient,        {0} },
 *  { ClkWinTitle,          0,              Button1,        focusstack,     {.i = +1 } },
 *  { ClkWinTitle,          0,              Button3,        focusstack,     {.i = -1 } },
 *  { ClkStatusText,          0,              Button3,      spawn,            {.v = calendar } },
 *  { ClkStatusText,        0,              Button2,      spawn,          {.v = termcmd } },
 *  [>====================================================================================<]
 *  //{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
 *  { ClkClientWin,         AltMask,         Button1,        movemouse,      {0} },
 *  { ClkClientWin,         MODKEY,         Button2,        killclient, {0} },
 *  { ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
 *  { ClkTagBar,            0,              Button1,        toggleview,     {0} },
 *  { ClkTagBar,            0,              Button3,        view,           {0} },
 *  { ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
 *  { ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
 */
};

/*
 *Window class definitions:
 */
const char client_class_idea[] = "sun-awt-X11-XFramePeer";
//const char client_class_idea_dialog[] = "sun-awt-X11-XDialogPeer";
const char client_class_notifyd[] = "xfce4-notifyd";
const char client_class_gsimplecal[] = "gsimplecal";
const char client_class_screenkey[] = "screenkey";
const char client_class_copyq[] = "copyq";

/*
 *List of window classes that should not be interactive, ie not be drawn onto tabbar,
 *not be selectable using meta+j/k keys...
 */
const char *ignored_class_list[] = {
    client_class_notifyd,
    client_class_gsimplecal,
    client_class_screenkey
};

// synergy logfile location: // TODO: delete
const char synergy_log_file[] = "/var/log/custom_logs/synergy_server.log";
