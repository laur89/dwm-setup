/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance.  Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag.  Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#include <limits.h>
/*#include <X11/Intrinsic.h>*/
#endif /* XINERAMA */

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
			       * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#define MAXCOLORS 17            // avoid circular reference to NUMCOLORS
#define occupiedColorIndex 12   // as in tag/tab is occupied, but NOT selected
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define TEXTW(X)                (textnw(X, strlen(X)) + dc.font.height)
#define SYSTEM_TRAY_REQUEST_DOCK    0
#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define MAXTABS 50

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY      0
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_FOCUS_IN             4
#define XEMBED_MODALITY_ON         10

#define XEMBED_MAPPED              (1 << 0)
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_WINDOW_DEACTIVATE    2
#define STDIN 0
#define STDOUT 1

#define VERSION_MAJOR               0
#define VERSION_MINOR               0
#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR

/* enums */
/*enum { CurNormal, CurResize, CurMove, CurLast };        [> cursor <]*/
enum { CurNormal, CurMove, CurRzUpCorLeft, CurRzUpCorRight, CurRzDnCorLeft,
      CurRzDnCorRight, CurRzMidUp, CurRzMidRight, CurRzMidDn, CurRzMidLeft, CurLast };        /* cursors */
enum { ColBorder, ColFG, ColBG, ColLast };              /* color */
enum { ColNorm, ColSel, ColUrg };              /* color */
enum { NetSupported, NetWMDemandsAttention, NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation,
      NetWMName, NetWMState, NetWMFullscreen, NetActiveWindow, NetWMWindowType,
      NetWMWindowTypeDialog, NetLast }; /* EWMH atoms */
enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { ClkTagBar, ClkTabBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast };             /* clicks */
enum { UpCorLeft, UpCenter, UpCorRight, MidSideLeft, MidCenter,
       MidSideRight, DnCorLeft, DnCenter, DnCorRight };             /* window sectors */

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
    char className[256];
	float mina, maxa;
	float cfact;
	int x, y, w, h;
	int sfx, sfy, sfw, sfh; /* stored float geometry, used on mode revert */
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, oldbw; // bar geomentry
	unsigned int tags;
	Bool isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen, iscentred, isInSkipList;
	Client *next;
	Client *snext;
	Monitor *mon;
	Window win;
};

typedef struct {
	int x, y, w, h;
    unsigned long colors[MAXCOLORS][ColLast];
	Drawable drawable;
	Drawable tabdrawable;
	Drawable celldrawable; // TODO: leave it?
	GC gc;
	struct {
		int ascent;
		int descent;
		int height;
		XFontSet set;
		XFontStruct *xfont;
	} font;
} DC; /* draw context */

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;


// alttab client stack:
const Client *clt[2];
unsigned int selclt;

struct Monitor {
	char ltsymbol[16];
	int num;
	int by;               /* bar y location*/
	int ty;               /* tab bar y location */
	int mx, my, mw, mh;   /* screen size, as in total */
	int wx, wy, ww, wh;   /* window area, ie where windows can be drawn  */
	unsigned int seltags;
	unsigned int sellt; // TODO: selected layout? saab olla 0 või 1
	unsigned int tagset[2];
	Bool showbar;
	Bool showtab;
	Bool topbar;
	Bool toptab;
	Client *clients;
	Client *sel; // != focused!
	Client *stack;
	Monitor *next;
	Window barwin;
	Window tabwin;
	Window cellwin; // alt+tab window
	int ntabs;
	int tab_widths[MAXTABS]; //TODO remove, as now all the tabs are of uniform width; // TODO: will be deprecated
	const Layout *lt[2]; // TODO: contains current and previous layout???
    // per monitor alttab client stack; we don't want per monitor, do we?
	/*const Client *clt[2]; // TODO: contains current and previous selected clients???*/
    /*unsigned int selclt;*/
	int curtag; // TODO: shows which *view* we're currently in? (yup, should be that)
	int prevtag;
	const Layout **lts;
	double *mfacts;
	int *nmasters;
};

 typedef struct {
	const char *name;
	const Layout *layout;
	float mfact;
	int nmaster;
} Tag;

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	unsigned int tags;
	Bool isfloating;
	Bool iscentred;
	int monitor;
} Rule;

typedef struct Systray   Systray;
struct Systray {
   Window win;
   Client *icons;
};

/* function declarations */
static void applyrules(Client *c);
static Bool applysizehints(Client *c, int *x, int *y, int *w, int *h, Bool interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachaside(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clearurgent(Client *c);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void deck(Monitor *m);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static void die(const char *errstr, ...);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void drawtab(Monitor *m);
static void drawtabs(void);
static void drawcoloredtext(char *text);
static void drawsquare(Bool filled, Bool empty, unsigned long col[ColLast]);
static void drawpoint(Bool filled, unsigned long col[ColLast]);
/*static void drawtext(const char *text, unsigned long col[ColLast], Bool pad);*/
static void drawtext(Drawable drawable, const char *text, unsigned long col[ColLast], Bool pad);
static void enternotify(XEvent *e);
static void enternotify_ffm(XEvent *e);
static void toggle_ffm(void);
static void toggle_mff(void);
static void expose(XEvent *e);
static void focus(Client *c);
static void focuswin(const Arg* arg);
static void focusin(XEvent *e);
/*static void altTab(const Arg *arg);*/
static void altTab(void);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static void focusstackwithoutrising(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop);
static unsigned int getsystraywidth();
static void removesystrayicon(Client *i);
static void resizebarwin(Monitor *m);
static void storeFloats(Client *c);
static void restoreFloats(Client *c);
static void resizerequest(XEvent *e);
static unsigned long getcolor(const char *colstr);
static Bool getrootptr(int *x, int *y);
static long getstate(Window w);
static Bool gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, Bool focused);
static void grabkeys(void);
static void incnmaster(const Arg *arg);
static void initfont(const char *fontstr);
static void keypress(XEvent *e);
static void keyrelease(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void resize(Client *c, int x, int y, int w, int h, Bool interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void restack(Monitor *m);
static void run(void);
static void runorraise(const Arg *arg);
static void scan(void);
static Bool sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
static void updatesystray(void);
static void updatesystrayicongeom(Client *i, int w, int h);
static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
static Client *wintosystrayicon(Window w);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, Bool fullscreen);
static void setlayout(const Arg *arg);
static void setcfact(const Arg *arg);
static void resetcfactall(void);
static void setmfact(const Arg *arg);
static void setup(void);
static void showhide(Client *c);
static void sigchld(int unused);
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static int textnw(const char *text, unsigned int len);
static void tile(Monitor *);
static void togglebar(const Arg *arg);
static void tabmode(const Arg *arg);
static void togglefloating(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unfocus(Client *c, Bool setfocus);
static void unmanage(Client *c, Bool destroyed);
static void unmapnotify(XEvent *e);
static Bool updategeom(void);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatewindowtype(Client *c);
static void updatetitle(Client *c);
static void updateClassName(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);
static void raise_floating_client(Client *c);
static void togglescratch(const Arg *arg);
static void reload(const Arg *arg);

/* variables */
static Systray *systray = NULL;
static unsigned long systrayorientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
static const char broken[] = "broken";
static char stext[256];
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh, blw = 0;      /* bar geometry */
static int th = 0;           /* tab bar geometry */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	/*[EnterNotify] = enternotify_ffm,*/
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	/*[KeyRelease] = keyrelease, // TODO: for alttab hack*/
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
    [ResizeRequest] = resizerequest,
	[UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast];
static Bool running = True;
static Cursor cursor[CurLast];
static Display *dpy;
static DC dc;
static DC cellDC;
static Monitor *mons = NULL, *selmon = NULL;
static Window root;
// these globals are used to store the target cursor coordinates when moved by the
// transferPointerToNextMon(), so that moving focus could be ignored by enternotify();
int txPointer_x = -1, txPointer_y = -1;


/* configuration, allows nested code to access above variables */
#include "config.h"

static unsigned int scratchtag = 1 << LENGTH(tags);

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

/* function implementations */
void
applyrules(Client *c) {
	const char *class, *instance;
	unsigned int i;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { NULL, NULL };

	/* rule matching */
	c->isfloating = c->tags = 0;
	c->iscentred = 1;
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;

	for(i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if((!r->title || strstr(c->name, r->title))
                && (!r->class || strstr(class, r->class))
                && (!r->instance || strstr(instance, r->instance))) {
			c->isfloating = r->isfloating;
			c->iscentred = r->iscentred;
			c->tags |= r->tags;
			for(m = mons; m && m->num != r->monitor; m = m->next);
			if(m)
				c->mon = m;
		}
	}
    // TODO: modifications!
	if(ch.res_class) {
        /*fprintf(stderr, "    !applyrule: class \"%s\"\n", ch.res_class);*/
		XFree(ch.res_class);
    }

	if(ch.res_name) {
        /*fprintf(stderr, "    !applyrule: name \"%s\"\n", ch.res_name);*/
		XFree(ch.res_name);
    }
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

Bool
applysizehints(Client *c, int *x, int *y, int *w, int *h, Bool interact) {
	Bool baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if(interact) {
		if(*x > sw)
			*x = sw - WIDTH(c);
		if(*y > sh)
			*y = sh - HEIGHT(c);
		if(*x + *w + 2 * c->bw < 0)
			*x = 0;
		if(*y + *h + 2 * c->bw < 0)
			*y = 0;
	}
	else {
		if(*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if(*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if(*x + *w + 2 * c->bw <= m->wx)
			*x = m->wx;
		if(*y + *h + 2 * c->bw <= m->wy)
			*y = m->wy;
	}
	if(*h < bh)
		*h = bh;
	if(*w < bh)
		*w = bh;
	if(resizehints || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if(!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if(c->mina > 0 && c->maxa > 0) {
			if(c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if(c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if(baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if(c->incw)
			*w -= *w % c->incw;
		if(c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if(c->maxw)
			*w = MIN(*w, c->maxw);
		if(c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m) {
	if(m)
		showhide(m->stack);
	else for(m = mons; m; m = m->next)
		showhide(m->stack);
	if(m)
		arrangemon(m);
	else for(m = mons; m; m = m->next)
		arrangemon(m);
}

void
arrangemon(Monitor *m) {
	updatebarpos(m);
	XMoveResizeWindow(dpy, m->tabwin, m->wx, m->ty, m->ww, th);

	strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
	if(m->lt[m->sellt]->arrange) // TODO: kontrollib, kas layoutil on funktsioon olemas? (floatil funktsioon puudub)
		m->lt[m->sellt]->arrange(m); // siin kutsutakse konkreetset lyt meetodit vist välja?
	restack(m);
}

void
attach(Client *c) {
	c->next = c->mon->clients;
	c->mon->clients = c;
}

void
attachaside(Client *c) {
	Client *at = nexttiled(c->mon->clients);;
	if(c->mon->sel == NULL || c->mon->sel->isfloating || !at) {
		attach(c);
		return;
	}
	c->next = at->next;
    at->next = c;
}

void
attachstack(Client *c) {
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void
buttonpress(XEvent *e) {
	unsigned int i, x, click;
	Arg arg = {0};
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;

	click = ClkRootWin;
	/* focus monitor if necessary */
	if((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, True);
		selmon = m;
		focus(NULL);
	}
	if(ev->window == selmon->barwin) {
		i = x = 0;
		do
			x += TEXTW(tags[i].name);
       while(ev->x >= x && ++i < (LENGTH(tags) - 1));
       if(i < (LENGTH(tags) - 1)) {
			click = ClkTagBar;
			arg.ui = 1 << i;
		}
		else if(ev->x < x + blw)
			click = ClkLtSymbol;
		else if(ev->x > selmon->ww - TEXTW(stext))
			click = ClkStatusText;
		else
			click = ClkWinTitle;
	}
	if(ev->window == selmon->tabwin) {
		i = 0; x = 0;
		for(c = selmon->clients; c; c = c->next){
          if(!ISVISIBLE(c)) continue;
		  x += selmon->tab_widths[i];
		  if (ev->x > x)
		    ++i;
		  else
		    break;
		  if(i >= m->ntabs) break;
		}
		if(c) {
		  click = ClkTabBar;
		  arg.ui = i;
		}
	}
	else if(c = wintoclient(ev->window)) {
		focus(c);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}
	for(i = 0; i < LENGTH(buttons); i++)
		if(click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		   && CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state)){
		  buttons[i].func(((click == ClkTagBar || click == ClkTabBar)
				   && buttons[i].arg.i == 0) ? &arg : &buttons[i].arg);
		}
}

// TODO: deleteme ver:
void sendKey2(KeySym keysym, KeySym modsym) {
    KeyCode keycode = 0, modcode = 0;

    keycode = XKeysymToKeycode (dpy, keysym);

    if (keycode == 0) return;
    XTestGrabControl (dpy, True);
    //[> Generate modkey press <]
    /*[>if (modsym != 0) {<]*/
        /*[>modcode = XKeysymToKeycode(dpy, modsym);<]*/
        /*[>XTestFakeKeyEvent (dpy, modcode, True, 0);<]*/
    /*[>}<]*/
    //[> Generate regular key press and release <]
    XTestFakeKeyEvent (dpy, keycode, True, 0);
    XTestFakeKeyEvent (dpy, keycode, False, 0);

    //[> Generate modkey release <]
    /*[>if (modsym != 0) {<]*/
        /*[>XTestFakeKeyEvent (dpy, modcode, False, 0);<]*/
    /*[>}<]*/

    XSync (dpy, False);
    XTestGrabControl (dpy, False);
}
void sendKey(KeySym keysym, KeySym modsym) {
    KeyCode keycode = 0, modcode = 0;

    /*Display* dpy;*/
	/*dpy = XOpenDisplay(NULL);*/
    keycode = XKeysymToKeycode (dpy, keysym);

	fprintf(stderr, "keycode: %d\n", keycode);

    if (keycode == 0) return;
    XTestGrabControl (dpy, True);
    //[> Generate modkey press <]
    if (modsym != 0) {
        modcode = XKeysymToKeycode(dpy, modsym);
        XTestFakeKeyEvent (dpy, modcode, True, CurrentTime);
        XFlush(dpy);
    }
    //[> Generate regular key press and release <]
    XTestFakeKeyEvent (dpy, keycode, True, CurrentTime);
        XFlush(dpy);
    XTestFakeKeyEvent (dpy, keycode, False, CurrentTime);
        XFlush(dpy);

    //[> Generate modkey release <]
    if (modsym != 0) {
        XTestFakeKeyEvent (dpy, modcode, False, CurrentTime);
        XFlush(dpy);
    }

    XSync (dpy, False);
    XTestGrabControl (dpy, False);
}

void sendKeyEvent(KeySym key, unsigned int mask, unsigned int pressOrReleaseMask) {
    Client *c = selmon->sel;
    Window w = selmon->sel->win;
    /*Window w = root;*/
    XKeyEvent e;
    /*XEvent e;*/
    int xx, yy; // cur pos relative to the selected windows point of origin;
    int di; // dummie
    unsigned int dui; // dummie
    Window dummy;

    // get the cursor location:
    XQueryPointer(dpy, c->win, &dummy, &dummy, &di, &di, &xx, &yy, &dui);

    // make pointer location absolute:
    xx += c->x;
    yy += c->y;

    /*ce.type = ConfigureNotify;*/
    e.display = dpy;
    e.window = w;
    e.subwindow = None;
    e.same_screen = True;
    e.x = xx;
    e.y = yy;
    e.x_root = xx;
    e.y_root = yy;
    e.type = pressOrReleaseMask;
    e.keycode = XKeysymToKeycode(dpy, key);
    e.state = mask;
    e.root = root;
    e.time = CurrentTime;
    XSendEvent(dpy, w, True, KeyPressMask|KeyReleaseMask, (XEvent *)&e);
    /*XSendEvent(dpy, w, True, KeyPressMask, (XEvent *)&e);*/
    /*XTestFakeKeyEvent(dpy, KeyPressMask, (XEvent *)&e);*/
    XSync (dpy, False);
}

void
checkotherwm(void) {
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void) {
	Arg a = {.ui = ~0};
	Layout foo = { "", NULL };
	Monitor *m;

	view(&a);
	selmon->lt[selmon->sellt] = &foo;
	for(m = mons; m; m = m->next)
		while(m->stack)
			unmanage(m->stack, False);
    // TODO: leave it like this?:
	if(dc.font.set) {
		XFreeFontSet(dpy, dc.font.set);
		XFreeFontSet(dpy, cellDC.font.set);
	 } else {
		XFreeFont(dpy, dc.font.xfont);
		XFreeFont(dpy, cellDC.font.xfont);
    }
	XUngrabKey(dpy, AnyKey, AnyModifier, root);

	XFreePixmap(dpy, dc.drawable);
	XFreePixmap(dpy, dc.tabdrawable);
	XFreePixmap(dpy, dc.celldrawable);
	XFreeGC(dpy, dc.gc);
	XFreePixmap(dpy, cellDC.drawable);
	XFreePixmap(dpy, cellDC.tabdrawable);
	XFreePixmap(dpy, cellDC.celldrawable);
	XFreeGC(dpy, cellDC.gc);

	XFreeCursor(dpy, cursor[CurNormal]);
	XFreeCursor(dpy, cursor[CurMove]);

	XFreeCursor(dpy, cursor[CurRzMidRight]);
	XFreeCursor(dpy, cursor[CurRzMidDn]);
	XFreeCursor(dpy, cursor[CurRzMidUp]);
	XFreeCursor(dpy, cursor[CurRzMidLeft]);
	XFreeCursor(dpy, cursor[CurRzUpCorRight]);
	XFreeCursor(dpy, cursor[CurRzUpCorLeft]);
	XFreeCursor(dpy, cursor[CurRzDnCorLeft]);
	XFreeCursor(dpy, cursor[CurRzDnCorRight]);
	while(mons)
		cleanupmon(mons);
    if(showsystray) {
        XUnmapWindow(dpy, systray->win);
        XDestroyWindow(dpy, systray->win);
        free(systray);
    }
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
}

void
cleanupmon(Monitor *mon) {
	Monitor *m;

	if(mon == mons)
		mons = mons->next;
	else {
		for(m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
	XUnmapWindow(dpy, mon->tabwin);
	XDestroyWindow(dpy, mon->tabwin);
	XUnmapWindow(dpy, mon->cellwin);
	XDestroyWindow(dpy, mon->cellwin);
	free(mon->mfacts);
	free(mon->nmasters);
	free(mon->lts);
	free(mon);
}

void
clearurgent(Client *c) {
	XWMHints *wmh;

	c->isurgent = False;
	if(!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags &= ~XUrgencyHint;
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
clientmessage(XEvent *e) {
    XWindowAttributes wa;
    XSetWindowAttributes swa;
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

   if(showsystray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
       /* add systray icons */
       if(cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
           if(!(c = (Client *)calloc(1, sizeof(Client))))
               die("fatal: could not malloc() %u bytes\n", sizeof(Client));
           c->win = cme->data.l[2];
           c->mon = selmon;
           c->next = systray->icons;
           systray->icons = c;
           XGetWindowAttributes(dpy, c->win, &wa);
           c->x = c->oldx = c->y = c->oldy = 0;
           c->w = c->oldw = wa.width;
           c->h = c->oldh = wa.height;
           c->oldbw = wa.border_width;
           c->bw = 0;
           c->isfloating = True;
           /* reuse tags field as mapped status */
           c->tags = 1;
           updatesizehints(c);
           updatesystrayicongeom(c, wa.width, wa.height);
           XAddToSaveSet(dpy, c->win);
           XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
           XReparentWindow(dpy, c->win, systray->win, 0, 0);
           /* use parents background pixmap */
           swa.background_pixmap = ParentRelative;
           swa.background_pixel  = dc.colors[0][ColBG];
           XChangeWindowAttributes(dpy, c->win, CWBackPixmap|CWBackPixel, &swa);
           sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
           /* FIXME not sure if I have to send these events, too */
           sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_FOCUS_IN, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
           sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
           sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_MODALITY_ON, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
           resizebarwin(selmon);
           updatesystray();
           setclientstate(c, NormalState);
       }
       return;
   }

	if(!c)
		return;
	if(cme->message_type == netatom[NetWMState]) {
		if(cme->data.l[1] == netatom[NetWMFullscreen] || cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				      || (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
        // unia's patch to enable qt applications (eg skype) to set urgency:
        else if(cme->data.l[1] == netatom[NetWMDemandsAttention]) {
            c->isurgent = (cme->data.l[0] == 1 || (cme->data.l[0] == 2 && !c->isurgent));
            /*drawbar(c->mon);*/

            XSetWindowBorder(dpy, c->win, dc.colors[ColUrg][ColBorder]);
			/*updatewmhints(c);*/
			drawbars();
			drawtabs();
        }
	}
    // TODO:
    // disable skype and iceweasel et al moving to master area by themselves:
	/*else if(cme->message_type == netatom[NetActiveWindow]) {*/
		/*if(!ISVISIBLE(c)) {*/
			/*c->mon->seltags ^= 1;*/
			/*c->mon->tagset[c->mon->seltags] = c->tags;*/
		/*}*/
		/*pop(c);*/
	/*}*/
}

void
configure(Client *c) {
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e) {
	Monitor *m;
	XConfigureEvent *ev = &e->xconfigure;
	Bool dirty;

	if(ev->window == root) {
		dirty = (sw != ev->width);
		sw = ev->width;
		sh = ev->height;
		if(updategeom() || dirty) {
			if(dc.drawable != 0)
				XFreePixmap(dpy, dc.drawable);
			dc.drawable = XCreatePixmap(dpy, root, sw, bh, DefaultDepth(dpy, screen));
			if(dc.tabdrawable != 0)
				XFreePixmap(dpy, dc.tabdrawable);
			dc.tabdrawable = XCreatePixmap(dpy, root, sw, th, DefaultDepth(dpy, screen));
            // TODO: is this necessary?
			if(dc.celldrawable != 0)
				XFreePixmap(dpy, dc.celldrawable);
            // TODO: cw, ch??? (cwch asendasin praegu 10-ga)
			dc.celldrawable = XCreatePixmap(dpy, root, cellWidth, 10, DefaultDepth(dpy, screen));
			/*dc.celldrawable = XCreatePixmap(dpy, root, cw, ch, DefaultDepth(dpy, screen));*/

			updatebars();
			for(m = mons; m; m = m->next){
                resizebarwin(m);
			}
			focus(NULL);
			arrange(NULL);
		}
	}
}

void
configurerequest(XEvent *e) {
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if((c = wintoclient(ev->window))) {
		if(ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if(c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
			m = c->mon;
			if(ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if(ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if(ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if(ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if(ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		}
		else
			configure(c);
	}
	else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

Monitor *
createmon(void) {
	Monitor *m;
	int i, numtags = LENGTH(tags) + 1;

	if(!(m = (Monitor *)calloc(1, sizeof(Monitor))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Monitor));
	if(!(m->mfacts = calloc(numtags, sizeof(double))))
		die("fatal: could not malloc() %u bytes\n", sizeof(double) * numtags);
	if(!(m->nmasters = calloc(numtags, sizeof(int))))
		die("fatal: could not malloc() %u bytes\n", sizeof(int) * numtags);
	if(!(m->lts = calloc(numtags, sizeof(Layout *))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Layout *) * numtags);
	m->tagset[0] = m->tagset[1] = 1;
	m->mfacts[0] = mfact;
	m->nmasters[0] = nmaster;
	m->lts[0] = &layouts[0];
	m->showbar = showbar;
	m->showtab = showtab;
	m->topbar = topbar;
	m->toptab = toptab;
	m->ntabs = 0;
	m->curtag = m->prevtag = 1;
	for(i = 1; i < numtags; i++) {
		m->mfacts[i] = tags[i - 1].mfact < 0 ? mfact : tags[i - 1].mfact;
		m->nmasters[i] = tags[i - 1].nmaster < 0 ? nmaster : tags[i - 1].nmaster;
		m->lts[i] = tags[i - 1].layout;
	}
	m->lt[0] = m->lts[m->curtag];

	m->lt[1] = &layouts[1 % LENGTH(layouts)];
	strncpy(m->ltsymbol, m->lt[0]->symbol, sizeof m->ltsymbol);
	return m;
}

void
deck(Monitor *m) {
	int dn;
	unsigned int i, n, h, mw, my, r;
	Client *c;
	float mfacts = 0, sfacts = 0;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++) {
		if(n < m->nmasters[m->curtag])
			mfacts += c->cfact;
		/*else*/
			/*sfacts += c->cfact;*/
    }

	if(n == 0)
		return;

	dn = n - m->nmasters[m->curtag]; // count of stacked slaves, ie clients in deck

	if(dn > 0) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, " D %d", dn);

	if(n > m->nmasters[m->curtag])
		mw = m->nmasters[m->curtag] ? m->ww * m->mfacts[m->curtag] : 0;
	else
		mw = m->ww;
	for(i = my = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++, r = 0)
        // TODO:
		/*if(n == 1) {*/
			/*if (c->bw) {*/
				/*[> remove border when only one window is on the current tag <]*/
				/*c->oldbw = c->bw;*/
				/*c->bw = 0;*/
				/*r = 1;*/
			/*}*/
		/*}*/
		/*else if(!c->bw && c->oldbw) {*/
			/*[> restore border when more than one window is displayed <]*/
			/*c->bw = c->oldbw;*/
			/*c->oldbw = 0;*/
			/*r = 1;*/
		/*}*/

		if(i < m->nmasters[m->curtag]) {
            h = (m->wh - my) * (c->cfact / mfacts); // oli enne
			resize(c,
                   m->wx,
                   m->wy + my,
                   mw - (2*c->bw),
                   h - (2*c->bw),    False);
			/*if(r)*/
				/*resizeclient(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw));*/
			my += HEIGHT(c);
			mfacts -= c->cfact;

            // before:
			/*h = (m->wh - my) / (MIN(n, m->nmasters[m->curtag]) - i);*/
			/*resize(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw), False);*/
            /*
			 *if(r)
			 *    resizeclient(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw));
             */
			/*my += HEIGHT(c);*/
		}
		else
			resize(c, m->wx + mw, m->wy, m->ww - mw - (2*c->bw), m->wh - (2*c->bw), False);
            /*
			 *if(r)
			 *    resizeclient(c, m->wx + mw, m->wy, m->ww - mw - (2*c->bw), m->wh - (2*c->bw));
             */
}

void
destroynotify(XEvent *e) {
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if((c = wintoclient(ev->window)))
		unmanage(c, True);
    else if((c = wintosystrayicon(ev->window))) {
       removesystrayicon(c);
       resizebarwin(selmon);
       updatesystray();
   }
}

void
detach(Client *c) {
	Client **tc;

	for(tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
detachstack(Client *c) {
	Client **tc, *t;

	for(tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if(c == c->mon->sel) {
		for(t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
}

void
die(const char *errstr, ...) {
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

Monitor *
dirtomon(int dir) {
	Monitor *m = NULL;

	if(dir > 0) {
		if(!(m = selmon->next))
			m = mons;
	}
	else if(selmon == mons)
		for(m = mons; m->next; m = m->next);
	else
		for(m = mons; m->next != selmon; m = m->next);
	return m;
}

// TODO: here is the synergy integration hack
Monitor *
dirtomon_synergy(int dir) {
	Monitor *m = NULL;

	if(dir > 0) {
        // fwd movement was required, but no more monitors are in stack; select the
        // first one in the stack:
		if(!(m = selmon->next)) {
            return m;
        }
	}
    // note: from here on, arg -1 is handled;
	else if(selmon == mons) { // first monitor selected, select the last mon:
        return m;
    }

    /*// either last nor first monitor wasn't selected, meaning dirtomon is to handle:*/
    return dirtomon(dir);
    /*for(m = mons; m->next != selmon; m = m->next);*/
	/*return m;*/
}

void
drawbar(Monitor *m) {
	int x;
	unsigned int i, occ = 0, urg = 0;
	unsigned long *col;
	unsigned int a = 0, s = 0;
	Client *c;

    resizebarwin(m);
	for(c = m->clients; c; c = c->next) {
		occ |= c->tags;
		if(c->isurgent)
			urg |= c->tags;
	}
	dc.x = 0;
    for(i = 0; i < (LENGTH(tags) - 1); i++) {
		dc.w = TEXTW(tags[i].name);
        /*col = dc.colors[ (m->tagset[m->seltags] & 1 << i) ? 1 : (urg & 1 << i ? 2:(occ & 1 << i ? occupiedColorIndex:0)) ];*/
        col = dc.colors[ (m->tagset[m->seltags] & 1 << i)
            /*? 1// tag is selected*/
            ? urg & 1 << i ? ColUrg : 1 // tag is selected
            // tag not selected:
            : (urg & 1 << i ? ColUrg : /* not urg: */ (occ & 1 << i ? occupiedColorIndex : 0)) ];
        /*drawtext(tags[i], col, True);*/
        drawtext(dc.drawable, tags[i].name, col, True);
		drawsquare(m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
		           occ & 1 << i, col);
        if (m == selmon && selmon->curtag == i+1)
            drawpoint(True, dc.colors[1]);
        else if(m == selmon && selmon->prevtag == i+1)
            drawpoint(False, dc.colors[occupiedColorIndex]);
		dc.x += dc.w;
	}
	if(m->lt[m->sellt]->arrange == monocle) {
		for(a = 0, s = 0, c= nexttiled(m->clients); c; c= nexttiled(c->next), a++)
			if(c == m->stack)
				s = a;
		if(!s && a)
			s = a;
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d/%d]", s, a);
	}
	dc.w = blw = TEXTW(m->ltsymbol);
	/*drawtext(m->ltsymbol, dc.colors[6], False);*/
	drawtext(dc.drawable, m->ltsymbol, dc.colors[6], False);
	dc.x += dc.w;
	x = dc.x;
	if(m == selmon) { /* status is only drawn on selected monitor */
        dc.w = textnw(stext, strlen(stext)); // no padding
		dc.x = m->ww - dc.w;
       if(showsystray && m == selmon) {
           dc.x -= getsystraywidth();
       }
		if(dc.x < x) {
			dc.x = x;
			dc.w = m->ww - x;
		}
        drawcoloredtext(stext);
	}
	else
		dc.x = m->ww;
	if((dc.w = dc.x - x) > bh) {
		dc.x = x;
		if(m->sel) {
        col = dc.colors[ m == selmon ? 1 : 0 ];
        // TODO: here is a fishy place
        drawtext(dc.drawable, m->sel->name, col, True);
        // potentially this needed:
        /*drawtext(dc.drawable, m->sel->name, dc.colors[0], True);*/
		}
		else
			drawtext(dc.drawable, NULL, dc.colors[0], False);
	}
	XCopyArea(dpy, dc.drawable, m->barwin, dc.gc, 0, 0, m->ww, bh, 0, 0);
	XSync(dpy, False);
}

void
drawbars(void) {
	Monitor *m;

	for(m = mons; m; m = m->next)
		drawbar(m);
    updatesystray();
}

void
drawcoloredtext(char *text) {
    char *buf = text, *ptr = buf, c = 1;
    unsigned long *col = dc.colors[0];
    int i, ox = dc.x;

    while( *ptr ) {
    for( i = 0; *ptr < 0 || *ptr > NUMCOLORS; i++, ptr++);
        if( !*ptr ) break;
        c=*ptr;
        *ptr=0;
            if( i ) {
            dc.w = selmon->ww - dc.x;
            drawtext(dc.drawable, buf, col, False);
            dc.x += textnw(buf, i);
            }
        *ptr = c;
        col = dc.colors[ c-1 ];
        buf = ++ptr;
        }
    drawtext(dc.drawable, buf, col, False);
    dc.x = ox;
}

drawtabs(void) {
   Monitor *m;

   for(m = mons; m; m = m->next)
       drawtab(m);
}

static int
cmpint(const void *p1, const void *p2) {
  /* The actual arguments to this function are "pointers to
     pointers to char", but strcmp(3) arguments are "pointers
     to char", hence the following cast plus dereference */
  return *((int*) p1) > * (int*) p2;
}


void
drawtab(Monitor *m) {
   unsigned long *col;
   Client *c;
   int i;
   int itag = -1;
   char view_info[50];
   int view_info_w = 0;
   int sorted_label_widths[MAXTABS];
   int tot_width;
   int maxsize = bh;
   int tab_starting_x = 0;
   dc.x = 0;

   //view_info: indicate the tag which is displayed in the view:
   /*
    *for(i = 0; i < LENGTH(tags); ++i){
    *  if((selmon->tagset[selmon->seltags] >> i) & 1) {
    *    if(itag >=0){ //more than one tag selected
    *      itag = -1;
    *      break;
    *    }
    *    itag = i;
    *  }
    *}
    *if(0 <= itag  && itag < LENGTH(tags)){
    *  snprintf(view_info, sizeof view_info, "[%s]", tags[itag]);
    *} else {
    *  strncpy(view_info, "[...]", sizeof view_info);
    *}
    */
   view_info[sizeof(view_info) - 1 ] = 0;
   view_info_w = TEXTW(view_info);
   tot_width = view_info_w;

   /* Calculates number of labels and their width */
   m->ntabs = 0;
   for(c = m->clients; c; c = c->next){
     if(!ISVISIBLE(c) || c->isInSkipList) continue;
     /*m->tab_widths[m->ntabs] = TEXTW(name);*/ // original
     m->tab_widths[m->ntabs] = tabWidth;
     tot_width += m->tab_widths[m->ntabs];
     ++m->ntabs;
     if(m->ntabs >= MAXTABS) break;
   }

   // TODO: this logic is deprecated, since all the tabs will be of constant width:
   // or is it deprecated?
   if(tot_width > m->ww){ //not enough space to display the labels/tabs, they need to be truncated
     memcpy(sorted_label_widths, m->tab_widths, sizeof(int) * m->ntabs);
     // TODO: why is sorting needed?:
     qsort(sorted_label_widths, m->ntabs, sizeof(int), cmpint);
     tot_width = view_info_w; // initializes tot_width with view info, tabs will be added
     for(i = 0; i < m->ntabs; ++i){
       if(tot_width + (m->ntabs - i) * sorted_label_widths[i] > m->ww)
         break;
       tot_width += sorted_label_widths[i];
     }

     // here is the trucated tab width set ??:
     maxsize = (m->ww - tot_width) / (m->ntabs - i);
   } else{
     maxsize = m->ww;
   }

   for( i = 0, c = m->clients; c; c = c->next ) {
     if(!ISVISIBLE(c) || c->isInSkipList) continue;
     if(i >= m->ntabs) break;
     if(m->tab_widths[i] >  maxsize) m->tab_widths[i] = maxsize;

     // TODO: here is the place to center-justify tab text:
     dc.w = m->tab_widths[i];
     /*dc.x = tab_starting_x*/
     /*dc.x = 0; // TODO: centerjustification should start with this one*/

     // orig:
     /*col = (c == m->sel)  ? dc.sel : dc.norm;*/
     if( m->nmasters[m->curtag] > 1 && i < m->nmasters[m->curtag]) // more than one master client
        // color masters differently:
        col = (c == m->sel) ? dc.colors[16] : dc.colors[15];
     else // single master client
        col = (c == m->sel) ? dc.colors[13] : dc.colors[12];
        /*
         *col = dc.colors[ (m->tagset[m->seltags] & 1 << i) ?
         *1 : (urg & 1 << i ? 2:0) ]
         */
     drawTabbarText(dc.tabdrawable, c->name, col, 0);
     /*drawtext(dc.tabdrawable, c->name, col, 0);*/
     dc.x += dc.w;
     /*tab_starting_x += m->tab_widths[i];*/
     i++; // may not be in loop control!
   }

   /* cleans interspace between window names and current viewed tag label */
   dc.w = m->ww - view_info_w - dc.x;
   drawTabbarText(dc.tabdrawable, NULL, dc.colors[0], 0);

   /* view info */
   dc.x += dc.w;
   dc.w = view_info_w;
   drawTabbarText(dc.tabdrawable, view_info, dc.colors[0], 0);

   XCopyArea(dpy, dc.tabdrawable, m->tabwin, dc.gc, 0, 0, m->ww, th, 0, 0);
   XSync(dpy, False);
}

void
drawpoint(Bool filled, unsigned long col[ColLast]) {
	int x;

	XSetForeground(dpy, dc.gc, col[ColFG]);
	x = (dc.font.ascent + dc.font.descent + 2) / 4;
    /*XDrawPoint(dpy, dc.drawable, dc.gc, dc.x+1, dc.y+1+x*2);*/
    /*XFillRectangle(dpy, dc.drawable, dc.gc, dc.x+1, dc.y+x*3, 2, 2);*/
	if(filled)
		XFillRectangle(dpy, dc.drawable, dc.gc, dc.x+1, dc.y+x*2, x+1, x+1);
	else
		XDrawRectangle(dpy, dc.drawable, dc.gc, dc.x+1, dc.y+x*2, x, x);
}

void
drawsquare(Bool filled, Bool empty, unsigned long col[ColLast]) {
	int x;

	XSetForeground(dpy, dc.gc, col[ColFG]);
	x = (dc.font.ascent + dc.font.descent + 2) / 4;
	if(filled)
		XFillRectangle(dpy, dc.drawable, dc.gc, dc.x+1, dc.y+1, x+1, x+1);
	else if(empty)
		XDrawRectangle(dpy, dc.drawable, dc.gc, dc.x+1, dc.y+1, x, x);
}

void
drawtext(Drawable drawable, const char *text, unsigned long col[ColLast], Bool pad) {
	char buf[256];
	int i, x, y, h, len, olen;

	XSetForeground(dpy, dc.gc, col[ColBG]);
	/*XFillRectangle(dpy, dc.drawable, dc.gc, dc.x, dc.y, dc.w, dc.h);*/
	XFillRectangle(dpy, drawable, dc.gc, dc.x, dc.y, dc.w, dc.h);
	if(!text) return;

	olen = strlen(text);
    h = pad ? (dc.font.ascent + dc.font.descent) : 0;
    y = dc.y + ((dc.h + dc.font.ascent - dc.font.descent) / 2);
	x = dc.x + (h / 2);
	/* shorten text if necessary */
	for(len = MIN(olen, sizeof buf); len && textnw(text, len) > dc.w - h; len--);
	if(!len)
		return;
	memcpy(buf, text, len);
	if(len < olen)
		for(i = len; i && i > len - 3; buf[--i] = '.');
	XSetForeground(dpy, dc.gc, col[ColFG]);
	if(dc.font.set)
		XmbDrawString(dpy, drawable, dc.font.set, dc.gc, x, y, buf, len);
	else
		XDrawString(dpy, drawable, dc.gc, x, y, buf, len);
}
void
drawTabbarText_ORIG(Drawable drawable, const char *text, unsigned long col[ColLast], Bool pad) {
	char buf[256];
	int i, x, y, h, len, olen;
    const short lenOfTrailingWhitespace = 4;
    const short lenOfTrailingSymbols = 3;
    const short lenOfTruncationDots = 3;
    const short isDefaultTabWidth = ( dc.w == tabWidth ) ? 1 : 0;
    const char trailingSymbol = '>';

	XSetForeground(dpy, dc.gc, col[ColBG]);
	/*XFillRectangle(dpy, dc.drawable, dc.gc, dc.x, dc.y, dc.w, dc.h);*/
	XFillRectangle(dpy, drawable, dc.gc, dc.x, dc.y, dc.w, dc.h);
	if(!text)
		return;
	olen = isDefaultTabWidth ? strlen(text)+lenOfTrailingWhitespace : strlen(text); // create 4-width buffer for tabs which do NOT need to be truncated;
    h = pad ? (dc.font.ascent + dc.font.descent) : 0;
    y = dc.y + ((dc.h + dc.font.ascent - dc.font.descent) / 2);
	x = dc.x + (h / 2);
	/* shorten text if necessary */
	for(len = MIN(olen, sizeof buf); len && textnw(text, len) > dc.w - h; len--);
	if(!len)
		return;
	memcpy(buf, text, len);

    /*if ( dc.w == 200 ) { // meaning default tab width, i.e. there's enough room for all the tabs on the bar;*/
        /*if(len < olen)*/
            /*for(i = len-7; i && i > len - 10; buf[--i] = '.');*/
        /*for(i = len-4; i && i > len - 7; buf[--i] = '>');*/
        /*for(i = len; i && i > len - 4; buf[--i] = ' ');*/
    /*} else {*/
        /*if(len < olen)*/
            /*for(i = len; i && i > len - 3; buf[--i] = '.');*/
    /*}*/

    if (isDefaultTabWidth) { // meaning default tab width, i.e. there's enough room for all the tabs on the bar;
        if(len < olen) { // truncate
            // locate starting point by absolute values...:
            /*for(i = len-(lenOfTrailingSymbols+lenOfTrailingWhitespace); i && i > len - (lenOfTrailingSymbols+lenOfTrailingWhitespace+lenOfTruncationDots); buf[--i] = '.');*/
            /*for(i = len-lenOfTrailingWhitespace; i && i > len - (lenOfTrailingSymbols+lenOfTrailingWhitespace); buf[--i] = trailingSymbol);*/

            for(i = len; i && i > len - (lenOfTrailingWhitespace); buf[--i] = ' ');
            // ...or relative:
            for(; i && i > len - (lenOfTrailingSymbols+lenOfTrailingWhitespace); buf[--i] = trailingSymbol);
            for(; i && i > len - (lenOfTrailingSymbols+lenOfTrailingWhitespace+lenOfTruncationDots); buf[--i] = '.');
        } else {
            for(i = len; i && i > len - (lenOfTrailingWhitespace-1); buf[--i] = trailingSymbol);
            buf[--i] = ' ';
        }
    } else {
        if(len < olen) // truncation for shorter-than-default tab widths:
            for(i = len; i && i > len - 3; buf[--i] = '.');
    }

	XSetForeground(dpy, dc.gc, col[ColFG]);
	if(dc.font.set)
		XmbDrawString(dpy, drawable, dc.font.set, dc.gc, x, y, buf, len);
	else
		XDrawString(dpy, drawable, dc.gc, x, y, buf, len);
}

/* only used for drawing text in the tab bar
 */
void
drawTabbarText(Drawable drawable, const char *text, unsigned long col[ColLast], Bool pad) {
	char buf[256];
	int i, x, y, h, len, olen;
    const short lenOfTruncationDots = 3;
    const short lenOfWhiteSpaceBufferEachSide = 2;
    const short isDefaultTabWidth = ( dc.w == tabWidth ) ? 1 : 0;

	XSetForeground(dpy, dc.gc, col[ColBG]);
	/*XFillRectangle(dpy, dc.drawable, dc.gc, dc.x, dc.y, dc.w, dc.h);*/
	XFillRectangle(dpy, drawable, dc.gc, dc.x, dc.y, dc.w, dc.h);
	if(!text)
		return;
	/*olen = isDefaultTabWidth ? strlen(text) + lenOfWhiteSpaceBufferEachSide*2 : strlen(text); // only add whitespace if default tabwidth???*/
	olen = strlen(text);
    h = pad ? (dc.font.ascent + dc.font.descent) : 0;
    y = dc.y + ((dc.h + dc.font.ascent - dc.font.descent) / 2);
	x = dc.x + (h / 2);

	/* shorten text if necessary */
    // defines new text length (len), if len > dc.w:
	for(len = MIN(olen, sizeof buf); len && textnw(text, len+2*lenOfWhiteSpaceBufferEachSide) > dc.w - h; len--);
	/*if(!len)*/
	if(len < lenOfTruncationDots)
		return;

    memcpy(&buf, text, len);

    if(len < olen) { // truncate
        for(i = len; i && i > len - lenOfTruncationDots; buf[--i] = '.');
    }

    int midPos = dc.w / 2;
    int txtLen = textnw(text, len);
    x += midPos - txtLen/2;

	XSetForeground(dpy, dc.gc, col[ColFG]);
	if(dc.font.set)
		XmbDrawString(dpy, drawable, dc.font.set, dc.gc, x, y, buf, len);
	else
		XDrawString(dpy, drawable, dc.gc, x, y, buf, len);
}

/* only used for drawing text in the tab bar
 */
void
drawTabbarText_OLD(Drawable drawable, const char *text, unsigned long col[ColLast], Bool pad) {
    // TODO: needs check to make sure buf size is enough to accommodate tabWidth worth of characters.
	char buf[256];
	char text_buf[256];
	int i, x, y, h, len, olen, oolen;
    const short lenOfTruncationDots = 3;
    const short lenOfWhiteSpaceBufferEachSide = 2;
    const Bool isDefaultTabWidth = ( dc.w == tabWidth ) ? True : False;

	XSetForeground(dpy, dc.gc, col[ColBG]);
	/*XFillRectangle(dpy, dc.drawable, dc.gc, dc.x, dc.y, dc.w, dc.h);*/
	XFillRectangle(dpy, drawable, dc.gc, dc.x, dc.y, dc.w, dc.h);
	if(!text) return;

    oolen = strlen(text);
	olen = isDefaultTabWidth ? oolen + lenOfWhiteSpaceBufferEachSide*2 : oolen;
    h = pad ? (dc.font.ascent + dc.font.descent) : 0;
    y = dc.y + ((dc.h + dc.font.ascent - dc.font.descent) / 2);
	x = dc.x + (h / 2);
	/* shorten text if necessary */
	for(len = MIN(olen, sizeof buf); len && textnw(text, len) > dc.w - h; len--);
	if(!len) return;

    /*if (isDefaultTabWidth) { // meaning default tab width, i.e. there's enough room for all the tabs on the bar;*/
        if(len < olen) { // truncate
            for( int i = 0; i < lenOfWhiteSpaceBufferEachSide; buf[i++] = ' ' ); // create whitespace buffer in the beginning;
            memcpy(&buf[lenOfWhiteSpaceBufferEachSide], text, oolen);
            for(i = len; i && i > len - (lenOfWhiteSpaceBufferEachSide); buf[--i] = ' ');
            for(; i && i > len - (lenOfWhiteSpaceBufferEachSide+lenOfTruncationDots); buf[--i] = '.');
        } else { // no truncation
            // reset len:
            len = oolen;
            // initialise buf:
            memcpy(buf, text, len);
            /*fprintf(stderr, "after len:%d\n", len );*/

            // grow the appended/prepended text as long as dc.w:
            while ( textnw(buf, len) < dc.w ) {
                // copy to temp buf and append+prepend with whitespace:
                text_buf[0] = ' ';
                memcpy( &text_buf[1], buf, len );
                text_buf[len+1] = ' ';
                len += 2;

                // copy back to main buf:
                memcpy(buf, text_buf, len);
            }
        }
    /*} else {*/
        /*memcpy(buf, text, len);*/
        /*if(len < olen) // truncation for shorter-than-default tab widths:*/
            /*for(i = len; i && i > len - 3; buf[--i] = '.');*/
    /*}*/

	XSetForeground(dpy, dc.gc, col[ColFG]);
	if(dc.font.set)
		XmbDrawString(dpy, drawable, dc.font.set, dc.gc, x, y, buf, len);
	else
		XDrawString(dpy, drawable, dc.gc, x, y, buf, len);
}

void
enternotify(XEvent *e) {
    if(!focus_follows_mouse) {
        return;
    }

	Client *c, *prevClient;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;
    prevClient = selmon->sel;

    // skip enternotify, if event was caused by the mouse movement generated by the transferPointerToNextMon():
    if (transfer_pointer && ev->x_root == txPointer_x && ev->y_root == txPointer_y) {
        /*txPointer_x = -1;*/
        /*txPointer_y = -1;*/
        return;
    }

	if((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if(m != selmon) {
		unfocus(selmon->sel, True);
		selmon = m;
	}
	else if(!c || c == selmon->sel)
		return;
    // TODO: if we want to use ffm and mff at the same time, we need to call focus()
    // with some another param notifying that it was called by enternotif() (since
    // mff cursorwarp is handled there)

    // same as in focusstack:
    /*selmon->clt[selmon->selclt] = selmon->sel;*/
    /*selmon->selclt ^= 1;*/
    /*selmon->clt[selmon->selclt] = c;*/
        if ( c != clt[selclt] ) {
            clt[selclt] = prevClient;
            selclt ^= 1;
            clt[selclt] = c;
        }
	focus(c);

    // store the new mouse pos for next roll of enternotify():
    txPointer_x = ev->x_root;
    txPointer_y = ev->y_root;
}

/*
 *void
 *toggle_ffm(const Arg *arg) {
 *  // Swap EnterNotify handler when first toggle is occured.
 *  if (handler[EnterNotify] == enternotify_ffm) {
 *    handler[EnterNotify] = enternotify;
 *} else if (handler[EnterNotify] == enternotify) {
 *    handler[EnterNotify] = enternotify_ffm;
 *  }
 *}
 */
void
toggle_ffm(void) {
    if (!focus_follows_mouse && mouse_follows_focus) mouse_follows_focus ^= 1; // since they both cannot be true at the same time!
    focus_follows_mouse ^= 1;
}

void
toggle_mff(void) {
    if (!mouse_follows_focus && focus_follows_mouse) focus_follows_mouse ^= 1; // since they both cannot be true at the same time!
    mouse_follows_focus ^= 1;
}

/*
 *void
 *enternotify_ffm(XEvent *e) {
 *  if (focus_follows_mouse)
 *    enternotify(e);
 *}
 */

void
expose(XEvent *e) {
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if(ev->count == 0 && (m = wintomon(ev->window))){
		drawbar(m);
		drawtab(m);
	}
}

void
focus(Client *c) {
	if(!c || !ISVISIBLE(c))
		for(c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	/* was if(selmon->sel) */
	if(selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, False);
	if(c) {
		if(c->mon != selmon)
			selmon = c->mon;
		if(c->isurgent)
			clearurgent(c);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, True);
		if(c->isfloating)
			XSetWindowBorder(dpy, c->win, dc.colors[14][0]);
		else
			XSetWindowBorder(dpy, c->win, dc.colors[1][ColBorder]);
		setfocus(c);

        // TODO: do want this?:
        if (mouse_follows_focus) {
            XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w/2, c->h/2);
        }

        // TODO: either this or try to grab B1 @ grabbuttons() and my own fun:
        // (if don't remember, this is about raising floating clients in relation to each
        // other)
        /*if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {*/
            /*XRaiseWindow(dpy, c->win);*/
        /*}*/
	}
	else
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
	selmon->sel = c;

	drawbars();
	drawtabs();
}

void
raise_floating_client(Client *c) {
    if (c && c->win && (c->isfloating || !selmon->lt[selmon->sellt]->arrange )) {
        XRaiseWindow(dpy, c->win);
    }
}

void
focusin(XEvent *e) { /* there are some broken focus acquiring clients */
	XFocusChangeEvent *ev = &e->xfocus;

	if(selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

/**
  * Note that this assumes monitors are side-by-side only.
  */
void
transferPointerToNextMon(Monitor *prevMon) {
    Monitor *m;
    int i, x, y, w, e; // w & e are prevMon & selmon x origins;
    i = x = y = w = e = 0;
    float cx, cy; //coefficients
    getrootptr(&x, &y);

    // find both monitors' x origins:
    // TODO: store these as Monitor properties during the setup!
    for(m = mons; m != prevMon; w+=m->mw, m = m->next);
    for(m = mons; m != selmon; e+=m->mw, m = m->next);

    x -= w; // cursor x pos in relation to the monitor;
    cx = (float) x / prevMon->mw; // coefficient
    cy = (float) y / prevMon->mh; // coefficient
    /*fprintf(stderr, "w: %d, e: %d\n", w, e);*/
    /*fprintf(stderr, "cx: %f, cy: %f\n", cx, cy);*/
    /*fprintf(stderr, "x: %d, y: %d\n", (int)(e + selmon->mw * cx), (int)(selmon->mh * cy));*/

    // store the new mouse pos for enternotify():
    txPointer_x = e + selmon->mw * cx;
    txPointer_y = selmon->mh * cy;

    XWarpPointer(dpy, None, root, 0, 0, 0, 0, txPointer_x, txPointer_y);
}

void
focusmon(const Arg *arg) {
    Monitor *m, *prevMon;
    Client *previousSel = selmon->sel;
    prevMon = selmon;

    if(!mons->next)
        return;
    if((m = dirtomon(arg->i)) == selmon)
        return;
    unfocus(selmon->sel, True);
    selmon = m;
    focus(NULL);

    if ( selmon->sel && selmon->sel != clt[selclt] ) {
        clt[selclt] = previousSel;
        selclt ^= 1;
        clt[selclt] = selmon->sel;
    }

    if (mouse_follows_focus || !transfer_pointer) return; // no point in moving cursor, if it's already following focus;
    transferPointerToNextMon(prevMon);
}

//TODO: synergy-dwm comobo:
void
focusmon_(const Arg *arg) {
    Monitor *m;
    Bool isSynergyServer; // TODO: prolly not needed, rite?

    /*if (isSynergyServer = getProcessId("synergys") [>|| getProcessId("synergyc")<]) { // synergy server (+or client?) running:*/
    if (isSynergyServer = getProcessId("synergys")) {
        /*[>getLastOccurrenceInLog(NULL); // TODO: deleteme<]*/
            /*sleep(30);*/

        if((m = dirtomon_synergy(arg->i))) { // ie monitor was found, do not switch to synergy client;
            unfocus(selmon->sel, True);
            selmon = m;
            focus(NULL);
        } else if (arg->i > 0) {
            fprintf(stderr, "entering syn's arg > 0 logic;");
            /*[>// switch in right direction:<]*/
            /*[>char *e[] = { "xdotool", "keyup", "period", "key", "control+alt+shift+F12", NULL };<]*/
            /*[>[>char *e[] = { "xdotool", "keyup", "period", "key", [>"--clearmodifiers",<] "control+alt+shift+F12", NULL };<]<]*/
            /*[>[>char *e[] = { "xdotool", "key", [>"--clearmodifiers",<] "control+alt+shift+F12", NULL };<]<]*/
            /*[>[>char *e[] = { "xdotool", "keyup", "period", "key", "9", NULL };<]<]*/
            /*[>Arg a = { .v = e }; // TODO: deleteme<]*/
            /*[>spawn(&a);<]*/
            system("xdotool keyup period key control+alt+shift+F12");

        } else if (arg->i < 0) {
            fprintf(stderr, "entering syn's arg < 0 logic;");
            // switch in left direction:
            /*[>char *i[] = { "xdotool", "keyup", "comma", "key", "control+alt+shift+F11", NULL };<]*/
            /*[>Arg b = { .v = i }; // TODO: deleteme<]*/
            /*[>spawn(&b);<]*/
            system("xdotool keyup Super_L");
            system("xdotool keyup comma");
            /*system("xdotool keyup comma key control+alt+shift+F11");*/
            system("xdotool key super+control+alt+shift+F11");

    /*[>sendKeyEvent(XK_F11, 0, KeyPress);<]*/
            /*[>sendKey(XK_F11, XK_Super_L );<]*/

            /*[>sendKey(XK_o, 0);<]*/
            /*[>sendKey(XK_9, XK_Super_L);<]*/
            /*[>sendKey(XK_F11, 0);<]*/
            /*[>sendKeyEvent(XK_F11, ControlMask|AltMask|ShiftMask, KeyPress);<]*/
            /*[>sendKeyEvent(XK_F11, Mod4Mask, KeyPress);<]*/
            /*[>sendKeyEvent(XK_F11, 0, KeyPress);<]*/

             /*TODO: this would be way better ,but doesn't work as of now:*/
            /*[>sendKeyEvent(XK_comma, 0, KeyRelease);<]*/
            /*[>sendKeyEvent(XK_9, 0, KeyPress);<]*/
        }
    } else {
        if(!mons->next)
            return;
        if((m = dirtomon(arg->i)) == selmon)
            return;
        unfocus(selmon->sel, True);
        selmon = m;
        focus(NULL);
    }
}
/*void*/
/*focusmon(const Arg *arg) {*/
	/*Monitor *m;*/

	/*if(!mons->next)*/
		/*return;*/
	/*if((m = dirtomon(arg->i)) == selmon)*/
		/*return;*/
	/*unfocus(selmon->sel, True);*/
	/*selmon = m;*/
	/*focus(NULL);*/
/*}*/

// same as focusstack, but doesn't raise the window
// by not calling restack() after focus().
void
focusstackwithoutrising(const Arg *arg) {
	Client *c = NULL, *i;

	if(!selmon->sel)
		return;
	if(arg->i > 0) {
		for(c = selmon->sel->next; c && (!ISVISIBLE(c) || c->isInSkipList); c = c->next);
		if(!c)
			for(c = selmon->clients; c && (!ISVISIBLE(c) || c->isInSkipList); c = c->next);
	}
	else {
		for(i = selmon->clients; i != selmon->sel; i = i->next)
			if(ISVISIBLE(i) && !i->isInSkipList)
				c = i;
		if(!c)
			for(; i; i = i->next)
                if(ISVISIBLE(i) && !i->isInSkipList)
					c = i;
	}
	if(c) {
		focus(c);
        // do not call:
		/*restack(selmon);*/
	}
}

void
focusstack(const Arg *arg) {
	Client *c = NULL, *i;

	if(!selmon->sel) {
		return;
    }

	if(arg->i > 0) {
		for(c = selmon->sel->next; c && (!ISVISIBLE(c) || c->isInSkipList); c = c->next);
		if(!c)
			for(c = selmon->clients; c && (!ISVISIBLE(c) || c->isInSkipList); c = c->next);
	}
	else {
		for(i = selmon->clients; i != selmon->sel; i = i->next)
			if(ISVISIBLE(i) && !i->isInSkipList)
				c = i;
		if(!c)
			for(; i; i = i->next)
                if(ISVISIBLE(i) && !i->isInSkipList)
					c = i;
	}

	if(c) {
        // same as in enternotify:
        /*selmon->clt[selmon->selclt] = selmon->sel;*/
        /*selmon->selclt ^= 1;*/
        /*selmon->clt[selmon->selclt] = c;*/

        if ( c != clt[selclt] ) {
            clt[selclt] = selmon->sel;
            selclt ^= 1;
            clt[selclt] = c;
        }

		focus(c);
		restack(selmon);
	}

}

void
focuswin(const Arg* arg){
  int iwin = arg->i;
  Client* c = NULL;

  // TODO: not quite sure why the tab-click hack is located here to be honest...:
  /*for(c = selmon->clients; c && (iwin || !ISVISIBLE(c)) ; c = c->next){*/ // original
  for(c = selmon->clients; c && (iwin || (!ISVISIBLE(c) || c->isInSkipList )) ; c = c->next) {
    if(ISVISIBLE(c) && !c->isInSkipList) --iwin;
    /*if(ISVISIBLE(c)) --iwin;*/ // original
  };
  if(c) {
    focus(c);
    restack(selmon);
  }
}

Atom
getatomprop(Client *c, Atom prop) {
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;
   /* FIXME getatomprop should return the number of items and a pointer to
    * the stored data instead of this workaround */
   Atom req = XA_ATOM;
   if(prop == xatom[XembedInfo])
       req = xatom[XembedInfo];

   if(XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req,
			      &da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
       if(da == xatom[XembedInfo] && dl == 2)
           atom = ((Atom *)p)[1];
		XFree(p);
	}
	return atom;
}

unsigned long
getcolor(const char *colstr) {
	Colormap cmap = DefaultColormap(dpy, screen);
	XColor color;

	if(!XAllocNamedColor(dpy, cmap, colstr, &color, &color))
		die("error, cannot allocate color '%s'\n", colstr);
	return color.pixel;
}

Bool
getrootptr(int *x, int *y) {
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w) {
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if(XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
			      &real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if(n != 0)
		result = *p;
	XFree(p);
	return result;
}

unsigned int
getsystraywidth() {
   unsigned int w = 0;
   Client *i;
   if(showsystray)
       for(i = systray->icons; i; w += i->w + systrayspacing, i = i->next) ;
   return w ? w + systrayspacing : 1;
}

Bool
gettextprop(Window w, Atom atom, char *text, unsigned int size) {
	char **list = NULL;
	int n;
	XTextProperty name;

	if(!text || size == 0)
		return False;
	text[0] = '\0';
	XGetTextProperty(dpy, w, &name, atom);
	if(!name.nitems)
		return False;
	if(name.encoding == XA_STRING)
		strncpy(text, (char *)name.value, size - 1);
	else {
		if(XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return True;
}

void
grabbuttons(Client *c, Bool focused) {
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);

        if(!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
					/*BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);*/
                    BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
        for(i = 0; i < LENGTH(buttons); i++)
            if(buttons[i].click == ClkClientWin)
                for(j = 0; j < LENGTH(modifiers); j++)
                    XGrabButton(dpy, buttons[i].button,
                            buttons[i].mask | modifiers[j],
                            c->win, False, BUTTONMASK,
                            GrabModeAsync, GrabModeSync, None, None);
	}
}

void
grabkeys(void) {
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		KeyCode code;
        const KeyCode altKeyCode = XKeysymToKeycode(dpy, XK_Alt_L);

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		for(i = 0; i < LENGTH(keys); i++)
			if((code = XKeysymToKeycode(dpy, keys[i].keysym)))
				for(j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
						 True, GrabModeAsync, GrabModeAsync);

        // TODO:! (alttab hackeroo)
        /*XGrabKey(dpy, altKeyCode, 0, root,*/
                /*True, GrabModeAsync, GrabModeAsync);*/
	}
}

void
incnmaster(const Arg *arg) {
    int j;
    // skip if in monocle, grid or float layout:
    if( selmon->lt[selmon->sellt]->arrange == monocle
            || selmon->lt[selmon->sellt]->arrange == NULL
            || selmon->lt[selmon->sellt]->arrange == gaplessgrid)
        return;

    j = countvisiblenonfloatingclients(selmon->clients);
    // don't allow increasing nmasters above the nr of current clients:
    if (arg->i > 0 && selmon->nmasters[selmon->curtag] == j)
        return;

	selmon->nmasters[selmon->curtag] = MAX(selmon->nmasters[selmon->curtag] + arg->i, 0);
	arrange(selmon);
}

/*void*/
/*initfont(const char *fontstr) {*/
	/*char *def, **missing;*/
	/*int n;*/

	/*dc.font.set = XCreateFontSet(dpy, fontstr, &missing, &n, &def);*/
	/*if(missing) {*/
		/*while(n--)*/
			/*fprintf(stderr, "dwm: missing fontset: %s\n", missing[n]);*/
		/*XFreeStringList(missing);*/
	/*}*/
	/*if(dc.font.set) {*/
		/*XFontStruct **xfonts;*/
		/*char **font_names;*/

		/*dc.font.ascent = dc.font.descent = 0;*/
		/*XExtentsOfFontSet(dc.font.set);*/
		/*n = XFontsOfFontSet(dc.font.set, &xfonts, &font_names);*/
		/*while(n--) {*/
			/*dc.font.ascent = MAX(dc.font.ascent, (*xfonts)->ascent);*/
			/*dc.font.descent = MAX(dc.font.descent,(*xfonts)->descent);*/
			/*xfonts++;*/
		/*}*/
	/*}*/
	/*else {*/
		/*if(!(dc.font.xfont = XLoadQueryFont(dpy, fontstr))*/
		/*&& !(dc.font.xfont = XLoadQueryFont(dpy, "fixed")))*/
			/*die("error, cannot load font: '%s'\n", fontstr);*/
		/*dc.font.ascent = dc.font.xfont->ascent;*/
		/*dc.font.descent = dc.font.xfont->descent;*/
	/*}*/
	/*dc.font.height = dc.font.ascent + dc.font.descent;*/
/*}*/

/// TODO: rename
void
initfont2(const char *fontstr, DC *dc) {
	char *def, **missing;
	int n;

	dc->font.set = XCreateFontSet(dpy, fontstr, &missing, &n, &def);
	if(missing) {
		while(n--)
			fprintf(stderr, "dwm: missing fontset: %s\n", missing[n]);
		XFreeStringList(missing);
	}
	if(dc->font.set) {
		XFontStruct **xfonts;
		char **font_names;

		dc->font.ascent = dc->font.descent = 0;
		XExtentsOfFontSet(dc->font.set);
		n = XFontsOfFontSet(dc->font.set, &xfonts, &font_names);
		while(n--) {
			dc->font.ascent = MAX(dc->font.ascent, (*xfonts)->ascent);
			dc->font.descent = MAX(dc->font.descent,(*xfonts)->descent);
			xfonts++;
		}
	}
	else {
		if(!(dc->font.xfont = XLoadQueryFont(dpy, fontstr))
		&& !(dc->font.xfont = XLoadQueryFont(dpy, "fixed")))
			die("error, cannot load font: '%s'\n", fontstr);
		dc->font.ascent = dc->font.xfont->ascent;
		dc->font.descent = dc->font.xfont->descent;
	}
	dc->font.height = dc->font.ascent + dc->font.descent;
}

#ifdef XINERAMA
static Bool
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info) {
	while(n--)
		if(unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return False;
	return True;
}
#endif /* XINERAMA */

/*void*/
/*grabkeys(void) {*/
	/*updatenumlockmask();*/
	/*{*/
		/*unsigned int i, j;*/
		/*unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };*/
		/*KeyCode code;*/
        /*const KeyCode altKeyCode = XKeysymToKeycode(dpy, XK_Alt_L);*/

		/*XUngrabKey(dpy, AnyKey, AnyModifier, root);*/
		/*for(i = 0; i < LENGTH(keys); i++)*/
			/*if((code = XKeysymToKeycode(dpy, keys[i].keysym)))*/
				/*for(j = 0; j < LENGTH(modifiers); j++)*/
					/*XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,*/
						 /*True, GrabModeAsync, GrabModeAsync);*/

        /*// TODO:! (alttab hackeroo)*/
        /*[>XGrabKey(dpy, altKeyCode, 0, root,<]*/
                /*[>True, GrabModeAsync, GrabModeAsync);<]*/
	/*}*/
/*}*/

void
keyrelease(XEvent *e) {
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

    fprintf(stderr, "     @keyrelease: in beginning.\n");

	ev = &e->xkey;
    keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	/*keysym = XK_Tab;*/

    if(keysym == XK_Tab
            && CLEANMASK(AltMask) == CLEANMASK(ev->state) ) {
        fprintf(stderr, "     @keyrelease: altTab release detected \n");
    }

}

void
keypress(XEvent *e) {
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;
    Arg a = { .v = e }; // TODO: deleteme

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);

	for(i = 0; i < LENGTH(keys); i++)
		if(keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		&& keys[i].func)
                keys[i].func(&(keys[i].arg));

    //TODO: ???
    // ungrab Alt so it could be sent...
    /*if ( keysym == XK_Alt_L ) {*/
        /*XUngrabKey(dpy, (KeyCode)ev->keycode, 0, root);*/

            /*[>KeySym keySym = XKeycodeToKeysym(dpy, e->xkey.keycode, 0);<]*/
            /*KeySym keySym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);*/
                    /*fprintf(stderr, "keypress event detected, keysym: %d\n", keySym);*/
        /*[>XSendEvent(dpy, e->xkey.window, True, KeyPressMask, e);<]*/
        /*XSendEvent(dpy, InputFocus, True, KeyPressMask, e);*/
        /*// ..and recover:*/
        /*XGrabKey(dpy, (KeyCode)ev->keycode, 0, root,*/
                /*True, GrabModeAsync, GrabModeAsync);*/
    /*}*/
}

void
killclient(const Arg *arg) {
	if(!selmon->sel)
		return;
   if(!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0 , 0, 0)) {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
manage(Window w, XWindowAttributes *wa) {
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;

	if(!(c = calloc(1, sizeof(Client))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Client));
	c->win = w;
	updatetitle(c);
    updateClassName(c);

    // TODO: mystuff:
    c->isInSkipList = isWindowInSkipList(c);

	if(XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
	}
	else {
		c->mon = selmon;
		applyrules(c);
	}
	/* geometry */
	if((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating) && c->iscentred) {
		c->x = c->oldx = c->mon->wx + (c->mon->ww / 2 - wa->width / 2);
		c->y = c->oldy = c->mon->wy + (c->mon->wh / 2 - wa->height / 2);
	}
	else {
		c->x = c->oldx = wa->x;
		c->y = c->oldy = wa->y;
	}
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;
	c->cfact = 1.0;

	if(c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
		c->x = c->mon->mx + c->mon->mw - WIDTH(c);
	if(c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
		c->y = c->mon->my + c->mon->mh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->mx);
	/* only fix client y-offset, if the client center might cover the bar */
	c->y = MAX(c->y, ((c->mon->by == c->mon->my) && (c->x + (c->w / 2) >= c->mon->wx)
		   && (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? bh : c->mon->my);
	c->bw = borderpx;

	if(!strcmp(c->name, scratchpadname)) {
		c->mon->tagset[c->mon->seltags] |= c->tags = scratchtag;
		c->isfloating = True;
		c->x = c->mon->wx + (c->mon->ww / 2 - WIDTH(c) / 2);
		c->y = c->mon->wy + (c->mon->wh / 2 - HEIGHT(c) / 2);
	}
	else
		c->tags &= TAGMASK;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
    XSetWindowBorder(dpy, w, dc.colors[0][ColBorder]);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	c->sfx = c->x;
	c->sfy = c->y;
	c->sfw = c->w;
	c->sfh = c->h;
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, False);
	if(!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	if(c->isfloating) {
		XRaiseWindow(dpy, c->win);
		XSetWindowBorder(dpy, w, dc.colors[14][1]);
    }
	attachaside(c);
	attachstack(c);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
    if(!c->isInSkipList) {
        if (c->mon == selmon)
            unfocus(selmon->sel, False);
        c->mon->sel = c;
    }

    arrange(c->mon);
	XMapWindow(dpy, c->win);

    if(!c->isInSkipList) {
        focus(NULL);
    }
}

void
mappingnotify(XEvent *e) {
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if(ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e) {
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;
   Client *i;
   if((i = wintosystrayicon(ev->window))) {
       sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
       resizebarwin(selmon);
       updatesystray();
   }

	if(!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	if(wa.override_redirect)
		return;
	if(!wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
monocle(Monitor *m) {
  unsigned int n = 0, r = 0;
	Client *c;

  for(c = nexttiled(m->clients); c; c = nexttiled(c->next)) {
    /* remove border when in monocle layout */
    if(c->bw) {
      c->oldbw = c->bw;
      c->bw = 0;
      r = 1;
    }
    resize(c, m->wx, m->wy, m->ww - (2 * c->bw), m->wh - (2 * c->bw), False);
    if(r)
      resizeclient(c, m->wx, m->wy, m->ww - (2 * c->bw), m->wh - (2 * c->bw));
  }
}

void
motionnotify(XEvent *e) {
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	if(ev->window != root)
		return;
	if((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		selmon = m;
		focus(NULL);
	}
	mon = m;
}

void
movemouse(const Arg *arg) {
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;

	if(!(c = selmon->sel))
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
            None, cursor[CurMove], CurrentTime) != GrabSuccess)
		return;
	if(!getrootptr(&x, &y))
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);

            // TODO: snap also along the axis? (when initial snapping has already
            // occurred);
            // TODO: refactor!

            // win2win snapping:
            for( Client *cc = c->mon->clients; cc; cc = cc->next ) {
                if ( cc == c || !ISVISIBLE(cc) ) continue;

                // snap x axis:
                if ( ( ( c->y-c->bw >= cc->y-c->bw && c->y-c->bw <= cc->y+c->bw + cc->h )
                        || ( c->y+c->h+c->bw <= cc->y+c->bw+cc->h && c->y+c->h+c->bw >= cc->y-c->bw ) ) // one corner is inside other client (cc)
                    || ( ( cc->y >= c->y && cc->y <= c->y + c->h )
                        || ( cc->y+cc->h <= c->y+c->h && cc->y+cc->h >= c->y )) ) { // ie there's contact along y-axis;
                    // snap to other client right side:
                    if( abs( cc->x+cc->w - nx ) < snap )
                        /*nx = cc->x + cc->w;*/
                        nx = cc->x + cc->w + 2*c->bw;
                    // snap to other client left side:
                    else if( abs( nx + c->w - cc->x ) < snap )
                        /*nx = cc->x - c->w;*/
                        nx = cc->x - c->w - 2*c->bw;
                }
                // snap y axis:
                if ( ( ( c->x-c->bw >= cc->x-c->bw && c->x-c->bw <= cc->x+cc->w+c->bw )
                        || ( c->x+c->w+c->bw <= cc->x+cc->w+c->bw && c->x+c->w+c->bw >= cc->x-c->bw ) ) // one corner is inside other client (cc)
                    || ( ( cc->x >= c->x && cc->x <= c->x+c->w )
                        || ( cc->x+cc->w <= c->x+c->w && cc->x+cc->w >= c->x )) ) {  // ie there's contact along x-axis;
                    // snap to other client bottom:
                    if( abs( cc->y+cc->h - ny ) < snap )
                        ny = cc->y + cc->h + 2*c->bw;
                    // snap to other client top:
                    else if( abs( ny + c->h - cc->y ) < snap )
                        ny = cc->y - c->h - 2*c->bw;
                }
            }

            // win2screen snapping:
			if(nx >= selmon->wx && nx <= selmon->wx + selmon->ww
                    && ny >= selmon->wy && ny <= selmon->wy + selmon->wh) {

				if(abs(selmon->wx - nx) < snap)
					nx = selmon->wx;
				else if(abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
					nx = selmon->wx + selmon->ww - WIDTH(c);

                // snap y axis:
				if(abs(selmon->wy - ny) < snap)
					ny = selmon->wy;
				else if(abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
					ny = selmon->wy + selmon->wh - HEIGHT(c);

				if(!c->isfloating && selmon->lt[selmon->sellt]->arrange
                        && (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
					togglefloating(NULL);

			}
			if(!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, nx, ny, c->w, c->h, True);
			break;
		}
	} while(ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

Client *
nexttiled(Client *c) {
	for(; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

void
pop(Client *c) {
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

void
propertynotify(XEvent *e) {
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

    //TODO: debug:
    /*fprintf(stderr, "  @ propnotif(): atom: %d\n", ev->atom);*/


   if((c = wintosystrayicon(ev->window))) {
       if(ev->atom == XA_WM_NORMAL_HINTS) {
           updatesizehints(c);
           updatesystrayicongeom(c, c->w, c->h);
       }
       else
           updatesystrayiconstate(c, ev);
       resizebarwin(selmon);
       updatesystray();
   }

	if((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
	else if(ev->state == PropertyDelete)
		return; /* ignore */
	else if((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:

            // TODO: this mofo is the IDEA auto-floating culprit!
            if ( strstr( c->className, client_class_idea ) ) return;

			if(!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
			   (c->isfloating = (wintoclient(trans)) != NULL))
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			updatesizehints(c);
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			drawtabs();
			break;
        // TODO: added atom to raise floating windows in relation to each other
		case 379:
            if ( c != clt[selclt] ) {
                clt[selclt] = selmon->sel;
                selclt ^= 1;
                clt[selclt] = c;
            }
			raise_floating_client(c);
			break;
		}
		if(ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
            // just in case, re-validate isInSkipList:
            c->isInSkipList = isWindowInSkipList(c);
			if(c == c->mon->sel)
				drawbar(c->mon);
			drawtab(c->mon);
		}
		if(ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

void
quit(const Arg *arg) {
	running = False;
}

Monitor *
recttomon(int x, int y, int w, int h) {
	Monitor *m, *r = selmon;
	int a, area = 0;

	for(m = mons; m; m = m->next)
		if((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

void
removesystrayicon(Client *i) {
   Client **ii;

   if(!showsystray || !i)
       return;
   for(ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
   if(ii)
       *ii = i->next;
   free(i);
}

void
resize(Client *c, int x, int y, int w, int h, Bool interact) {
	if(applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

resizebarwin(Monitor *m) {
   unsigned int w = m->ww;
   if(showsystray && m == selmon)
       w -= getsystraywidth();
   XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, w, bh);
}

void
resizeclient(Client *c, int x, int y, int w, int h) {
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

Bool
isInRect( int wx, int wy, int ww, int wh, int px, int py ) {
    if( px >= wx && px <= wx + ww
            && py <= wy + wh && py >= wy) {
        return True;
    }

    return False;
}

// returns the sector of the client cursor currently is occupying:
int
findSector( Client *c ) {
    int xx, yy; // cur pos relative to the selected windows point of origin;
    int di; // dummie
    unsigned int dui; // dummie
	Window dummy;
    const int cw = c->w * 0.333;
    const int ch = c->h * 0.333;

    // get the cursor location:
	XQueryPointer(dpy, c->win, &dummy, &dummy, &di, &di, &xx, &yy, &dui);

    /*fprintf(stderr, "win dimensions: %d x %d\n", c->w, c->h);*/
    /*fprintf(stderr, "at findSector(): cx: %d, cy: %d, cw: %d, ch: %d, px: %d, py: %d\n", c->x, c->y, cw, ch, xx, yy);*/

    // make pointer location absolute:
    xx += c->x;
    yy += c->y;

    if ( isInRect( c->x, c->y, cw, ch, xx, yy ) ) {
        return UpCorLeft;
    } else if ( isInRect( c->x + cw, c->y, cw, ch, xx, yy )
        || isInTriangle(c->x+cw, c->y+ch, c->x+2*cw, c->y+ch, c->x+c->w/2, c->y+c->h/2, xx, yy ) ) {
        return UpCenter;
    } else if ( isInRect( c->x + 2*cw, c->y, cw, ch, xx, yy ) ) {
        return UpCorRight;
    } else if ( isInRect( c->x, c->y + ch, cw, ch, xx, yy )
        || isInTriangle(c->x+cw, c->y+ch, c->x+cw, c->y+2*ch, c->x+c->w/2, c->y+c->h/2, xx, yy ) ) {
        return MidSideLeft;
        // TODO: what to do with the middle section?: (currently have separated
        //       middle section to 4 triangles)
    /*} else if ( isInRect( c->x + cw, c->y + ch, cw, ch, xx, yy ) ) {*/
        /*return MidCenter;*/
    } else if ( isInRect( c->x + 2*cw, c->y + ch, cw, ch, xx, yy )
        || isInTriangle( c->x+2*cw, c->y+ch, c->x+2*cw, c->y+2*ch, c->x+c->w/2, c->y+c->h/2, xx, yy ) ) {
        return MidSideRight;
    } else if ( isInRect( c->x, c->y + 2*ch, cw, ch, xx, yy ) ) {
        return DnCorLeft;
    } else if ( isInRect( c->x + cw, c->y + 2*ch, cw, ch, xx, yy )
        || isInTriangle(c->x+cw, c->y+2*ch, c->x+2*cw, c->y+2*ch, c->x+c->w/2, c->y+c->h/2, xx, yy ) ) {
        return DnCenter;
    } else if ( isInRect( c->x + 2*cw, c->y + 2*ch, cw, ch, xx, yy ) ) {
        return DnCorRight;
    } else {
       //TODO else return error?
       return -1;
    }
}

void initializeCursorResizePosition( Client *c, int sector ) {
    switch (sector) {
        case UpCorRight:
            XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, - c->bw);
            break;
        case DnCorRight:
            XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
            break;
        case DnCorLeft:
            XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, - c->bw, c->h + c->bw - 1);
            break;
        case UpCorLeft:
            XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, - c->bw, - c->bw);
            break;
        case UpCenter:
            XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w/2, - c->bw);
            break;
        case MidSideRight:
            XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h/2);
            break;
        case MidSideLeft:
            XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, - c->bw, c->h/2);
            break;
        case DnCenter:
            XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w/2, c->h + c->bw - 1);
            break;
        default:
            // no need, not a problem if execution reaches here;
            break;
    }
}

Bool grabPointer( int sector ) {
    unsigned int cursorIndex;

    switch (sector) {
        case UpCorRight:
            cursorIndex = CurRzUpCorRight;
            break;
        case DnCorRight:
            cursorIndex = CurRzDnCorRight;
            break;
        case DnCorLeft:
            cursorIndex = CurRzDnCorLeft;
            break;
        case UpCorLeft:
            cursorIndex = CurRzUpCorLeft;
            break;
        case UpCenter:
            cursorIndex = CurRzMidUp;
            break;
        case MidSideRight:
            cursorIndex = CurRzMidRight;
            break;
        case MidSideLeft:
            cursorIndex = CurRzMidLeft;
            break;
        case DnCenter:
            cursorIndex = CurRzMidDn;
            break;
        /*case MidCenter:*/
            /*return False;*/
            /*break;*/
        default:
            cursorIndex = CurNormal;
            break;
    }

    if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
            None, cursor[cursorIndex], CurrentTime) != GrabSuccess)
        return False;
    return True;
}

// triangle logic from http://www.geeksforgeeks.org/check-whether-a-given-point-lies-inside-a-triangle-or-not/
// TODO: note that this is far from perfect, since middle position gets undetected;
float
calculateTriangleArea(int x1, int y1, int x2, int y2, int x3, int y3) {
   return abs((x1*(y2-y3) + x2*(y3-y1)+ x3*(y1-y2))/2.0);
}

Bool
isInTriangle( int x1, int y1, int x2, int y2, int x3, int y3, int x, int y ) {
    /* Calculate area of triangle ABC */
   float A = calculateTriangleArea(x1, y1, x2, y2, x3, y3);

   /* Calculate area of triangle PBC */
   float A1 = calculateTriangleArea(x, y, x2, y2, x3, y3);

   /* Calculate area of triangle PAC */
   float A2 = calculateTriangleArea(x1, y1, x, y, x3, y3);

   /* Calculate area of triangle PAB */
   float A3 = calculateTriangleArea(x1, y1, x2, y2, x, y);

   /* Check if sum of A1, A2 and A3 is same as A */
   /*fprintf(stderr, "A: %f, A1: %f, A2: %f, A3: %f\n", A, A1, A2, A3);*/
   return (A == A1 + A2 + A3);
}

void
resizemouse(const Arg *arg) {
	int ocx, ocy, och, ocw; // olds/originals
	int ph, pw; // previous client dimesions; reset after every while loop cycle; used to detect clients that allow specific min/max dimensions;
	int nx, ny, nw, nh; // new
	Client *c;
	Monitor *m;
    int sector; // represents the part of window pointer was in at the initilaization;
	XEvent ev;

	if(!(c = selmon->sel))
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
    och = c->h;
    ocw = c->w;
    ph = pw = -1;
    sector = findSector(c);

    if (sector < 0 || sector == MidCenter || !grabPointer( sector) ) {
        XUngrabPointer(dpy, CurrentTime);
        return;
    }

    initializeCursorResizePosition( c, sector );
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
            case ConfigureRequest:
            case Expose:
            case MapRequest:
                handler[ev.type](&ev);
                break;
            case MotionNotify:
                // calculate new origin & dimesions:
                switch (sector) {
                    case UpCorRight:
                        nx = c->x;
                        ny = (c->h == ph) ? c->y : MIN(ev.xmotion.y, ocy + och);
                        nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
                        nh = MAX((ocy - ev.xmotion.y) + och, 1);
                        break;
                    case DnCorRight:
                        nx = c->x;
                        ny = c->y;
                        nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
                        nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
                        break;
                    case DnCorLeft:
                        nx = (c->w == pw) ? c->x : MIN(ev.xmotion.x, ocx + ocw);
                        ny = c->y;
                        nw = MAX((ocx - ev.xmotion.x) + ocw, 1);
                        nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
                        break;
                    case UpCorLeft:
                        nx = (c->w == pw) ? c->x : MIN(ev.xmotion.x, ocx + ocw);
                        ny = (c->h == ph) ? c->y : MIN(ev.xmotion.y, ocy + och);
                        nw = MAX((ocx - ev.xmotion.x) + ocw, 1);
                        nh = MAX( och - (ev.xmotion.y - ocy), 1);
                        break;
                    case UpCenter:
                        nx = c->x;
                        ny = (c->h == ph) ? c->y : MIN(ev.xmotion.y, ocy + och);
                        nw = c->w;
                        nh = MAX( och - (ev.xmotion.y - ocy), 1);
                        break;
                    case MidSideRight:
                        nx = c->x;
                        ny = c->y;
                        nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
                        nh = c->h;
                        break;
                    case MidSideLeft:
                        nx = (c->w == pw) ? c->x : MIN(ev.xmotion.x, ocx + ocw);
                        ny = c->y;
                        nw = MAX((ocx - ev.xmotion.x) + ocw, 1);
                        nh = c->h;
                        break;
                    case DnCenter:
                        nx = c->x;
                        ny = c->y;
                        nw = c->w;
                        nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
                        break;
                    default:
                        XUngrabPointer(dpy, CurrentTime);
                        return;
                }

                // TODO: what's going on here? (as in first if block)
                if(c->mon->wx + nw >= selmon->wx
                        && c->mon->wx + nw <= selmon->wx + selmon->ww
                        && c->mon->wy + nh >= selmon->wy
                        && c->mon->wy + nh <= selmon->wy + selmon->wh) {
                    if(!c->isfloating
                            && selmon->lt[selmon->sellt]->arrange
                            && (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
                        togglefloating(NULL);
                }

                //store previous dimensions:
                ph = c->h;
                pw = c->w;
                if(!selmon->lt[selmon->sellt]->arrange || c->isfloating)
                    resize(c, nx, ny, nw, nh, True);

                break;
		}
	} while(ev.type != ButtonRelease);

    initializeCursorResizePosition( c, sector );
	XUngrabPointer(dpy, CurrentTime);
	while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

void
resizerequest(XEvent *e) {
   XResizeRequestEvent *ev = &e->xresizerequest;
   Client *i;

   if((i = wintosystrayicon(ev->window))) {
       updatesystrayicongeom(i, ev->width, ev->height);
       resizebarwin(selmon);
       updatesystray();
   }
}

void
restack(Monitor *m) {
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	drawtab(m);
	if(!m->sel)
		return;
	if(m->sel->isfloating || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win); // raise the selected floating window
	if(m->lt[m->sellt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for(c = m->stack; c; c = c->snext) {
			if(!c->isfloating && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
        }
    }
	XSync(dpy, False);
	while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
run(void) {
	XEvent ev;
    /*XAnyEvent e;*/
    /*Client *c;*/

	/* main event loop */
	XSync(dpy, False);
	while(running && !XNextEvent(dpy, &ev)) {
        /*//TODO: deleteme:*/
        /*fprintf(stderr, "  @ run(): event type: %d\n", ev.type);*/
        /*fprintf(stderr, "  @ run(): curview: %d\n", selmon->curtag);*/
        /*e = ev.xany;*/
        /*if ( e.window && (c = wintoclient(e.window)) ) {*/
            /*if ( isIntelliJ(c) ) continue;*/
        /*}*/
		if(handler[ev.type])
			handler[ev.type](&ev); /* call handler */
    }
}

void
runorraise(const Arg *arg) {
    char *app = ((char **)arg->v)[4];
    Arg a = { .ui = ~0 };
    Monitor *mon;
    Client *c;
    XClassHint hint = { NULL, NULL };
    /* Tries to find the client by res_class hint */
    for (mon = mons; mon; mon = mon->next) {
        for (c = mon->clients; c; c = c->next) {
            XGetClassHint(dpy, c->win, &hint);
            if (hint.res_class && strcmp(app, hint.res_class) == 0) {
                a.ui = c->tags;
                view(&a);
                focus(c);
                XRaiseWindow(dpy, c->win);
                return;
            }
        }
    }
    /* Client not found: spawn it */
    spawn(arg);
}

void
scan(void) {
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if(XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for(i = 0; i < num; i++) {
			if(!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if(wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for(i = 0; i < num; i++) { /* now the transients */
			if(!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if(XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if(wins)
			XFree(wins);
	}
}

// orig:
/*void*/
/*sendmon(Client *c, Monitor *m) {*/
	/*if(c->mon == m)*/
		/*return;*/
	/*unfocus(c, True);*/
	/*detach(c);*/
	/*detachstack(c);*/
	/*c->mon = m;*/
    /*// TODO: instead of assigning tags of target monitor, perhaps*/
    /*// leave originals? or even rerun applyRules()?:*/
	/*c->tags = m->tagset[m->seltags]; [> assign tags of target monitor <]*/
	/*attach(c);*/
	/*attachstack(c);*/
	/*focus(NULL);*/
	/*arrange(NULL);*/
/*}*/
void
sendmon(Client *c, Monitor *m) {
	if(c->mon == m)
		return;
	unfocus(c, True);
	detach(c);
	detachstack(c);
	c->mon = m;
    // instead of assigning tags of target monitor, leave the old ones:
	/*c->tags = m->tagset[m->seltags]; [> assign tags of target monitor <]*/
	attach(c);
	attachstack(c);
	focus(NULL);
	arrange(NULL);
}

void
setclientstate(Client *c, long state) {
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
			PropModeReplace, (unsigned char *)data, 2);
}

Bool
sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4) {
	int n;
   Atom *protocols, mt;
	Bool exists = False;
	XEvent ev;

   if(proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
       mt = wmatom[WMProtocols];
       if(XGetWMProtocols(dpy, w, &protocols, &n)) {
           while(!exists && n--)
               exists = protocols[n] == proto;
           XFree(protocols);
       }
   }
   else {
       exists = True;
       mt = proto;
	}
	if(exists) {
		ev.type = ClientMessage;
       ev.xclient.window = w;
       ev.xclient.message_type = mt;
		ev.xclient.format = 32;
       ev.xclient.data.l[0] = d0;
       ev.xclient.data.l[1] = d1;
       ev.xclient.data.l[2] = d2;
       ev.xclient.data.l[3] = d3;
       ev.xclient.data.l[4] = d4;
       XSendEvent(dpy, w, False, mask, &ev);
	}
	return exists;
}

void
setfocus(Client *c) {
	if(!c->neverfocus)
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
   sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
}

void
setfullscreen(Client *c, Bool fullscreen) {
	if(fullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
				PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = True;
		c->oldstate = c->isfloating;
		c->oldbw = c->bw;
		c->bw = 0;
		c->isfloating = True;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
	}
	else {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
				PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = False;
		c->isfloating = c->oldstate;
		c->bw = c->oldbw;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		arrange(c->mon);
	}
}

void
setlayout(const Arg *arg) {
    /*
	 *if(!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
	 *    selmon->sellt ^= 1;
	 *if(arg && arg->v)
	 *    selmon->lt[selmon->sellt] = (Layout *)arg->v;
     */
    // this flip selects ALWAYS the previous layout as default:
 	selmon->sellt ^= 1;

    // if this check true, only then assign provided layout as current:
    //  TODO: here is exception, since NULL arg produces error in third expression:
	if(arg && arg->v && (arg->v != selmon->lt[selmon->sellt^1])) { // TODO: viimane check on if arg->v != ei ole eelmine? (eelmine on juba flipiga selekteeritud)
                                                                   // point ehk selles, et kuna esimese rea peal nagunii flipiti, siis eelmine on juba selekteeritud?*/
		selmon->lt[selmon->sellt] = selmon->lts[selmon->curtag] = (Layout *)arg->v;
    }
    strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);

    // store/restore floating clients' dimensions and positions; also restore borders;
    if (saveAndRestoreClientDimesionAndPositionForFloatMode) {
        if (!selmon->lt[selmon->sellt^1]->arrange)// previous mode was float:
            storeFloats(selmon->stack);
        else if(!selmon->lt[selmon->sellt]->arrange){ // new mode is float
            // try to restore borders:
            for(Client *c = selmon->clients; c; c = c->next) {
                if(!c->bw && c->oldbw) {
                    /* restore border when more than one window is displayed */
                    c->bw = c->oldbw;
                    c->oldbw = 0;
                }
            }
            // and now restore floaties' positions and sizes:
            restoreFloats(selmon->stack);
        }
    }
	if(selmon->sel) // if there is selected window:
		arrange(selmon);
	else
		drawbar(selmon);
}

void storeFloats(Client *c) {
    if (!c) return;
    /*[>[>save last known float dimensions<]<]*/
    c->sfx = c->x;
    c->sfy = c->y;
    c->sfw = c->w;
    c->sfh = c->h;
    storeFloats(c->snext);
}

void restoreFloats(Client *c) {
    if (!c) return;
    /*[>[>restore last known float dimensions<]<]*/
    resize(c, c->sfx, c->sfy, c->sfw, c->sfh, False);
    restoreFloats(c->snext);
}

int countvisiblenonfloatingclients(Client *c) {
    int i = 0;
    for(; c; i += (ISVISIBLE(c) && !c->isfloating) ? 1 : 0, c = c->next);
    return i;
}

void setcfact(const Arg *arg) {
    Client *c;
    int i, j;
    // count visible non-floating clients:
    j = countvisiblenonfloatingclients(selmon->clients);

    // block cfact modifications if result isn't instantaneously observable:
    if ( j < 2 || !selmon->sel ) return;
    if( selmon->lt[selmon->sellt]->arrange == monocle/* || selmon->lt[selmon->sellt]->arrange == NULL*/
            || selmon->lt[selmon->sellt]->arrange == gaplessgrid)
        return;

    for( i = 0, c = selmon->clients; c; c = c->next ) {
        if(!ISVISIBLE(c)) continue;
        if (c == selmon->sel) {
            // check whether master or slave client selected:
            if ( (i < selmon->nmasters[selmon->curtag] && selmon->nmasters[selmon->curtag] < 2) // ie we're processing master area client and there's less than 2 clients;
                    || (i >= selmon->nmasters[selmon->curtag] && ( j - selmon->nmasters[selmon->curtag] < 2 || selmon->lt[selmon->sellt]->arrange == deck)) // ie (processing slave client) AND (there's < 2 clients OR deck layout selected);
                    || c->isfloating) // this is why we don't need to check for floating layout above; // TODO: but if mode==float, then clients dont' have floating flag set?!
                return;
            break;
        }

        i++;
    }

    setcfactexec(arg, selmon->sel);
	arrange(selmon);
}

/* reset cfact for all clients */
void resetcfactall(void) {
	const Arg a = {.f = 0.00};

    for (Client *c = selmon->stack; c; c = c->snext)
        setcfactexec(&a, c);
	arrange(selmon);
}

// not to be called from config //
void setcfactexec(const Arg *arg, Client *c) {
	float f;

	if(!arg || !c || !selmon->lt[selmon->sellt]->arrange)
		return;
	f = arg->f + c->cfact;
	if(arg->f == 0.0)
		f = 1.0; // reset
	else if(f < 0.25 || f > 4.0)
		return;
	c->cfact = f;
    // arrange(m)  invoked by calling function!
}

/* arg > 1.0 will set mfact absolutly */
void
setmfact(const Arg *arg) {
	float f;
    int j;
    j = countvisiblenonfloatingclients(selmon->clients);

    // block mfact modifications if result isn't instantaneously observable:
    if(!arg || j < 2
            || !selmon->lt[selmon->sellt]->arrange
            || selmon->lt[selmon->sellt]->arrange == monocle
            || selmon->lt[selmon->sellt]->arrange == gaplessgrid
            // block mfact modifications if result isn't instantaneously observable,
            // as in either slave or master area are unpopulated:
            || selmon->nmasters[selmon->curtag] < 1
            || j - selmon->nmasters[selmon->curtag] < 1)
        return;

	f = ( arg->f < 1.0 ) ? arg->f + selmon->mfacts[selmon->curtag] : arg->f - 1.0;
	if(f < 0.1 || f > 0.9)
		return;
	selmon->mfacts[selmon->curtag] = f;
	arrange(selmon);
}

void
setup(void) {
	XSetWindowAttributes wa;

	/* clean up any zombies immediately */
	sigchld(0);

	/* init screen */
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	initfont2(font, &dc); // TODO - redone that fun
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	bh = dc.h = dc.font.height;
    th = bh;
	updategeom();
	/* init atoms */
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
    netatom[NetWMDemandsAttention] = XInternAtom(dpy, "_NET_WM_STATE_DEMANDS_ATTENTION", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
   netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
   netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
   netatom[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
   xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
   xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
   xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	/* init cursors */
	cursor[CurNormal] = XCreateFontCursor(dpy, XC_left_ptr);
	/*cursor[CurResize] = XCreateFontCursor(dpy, XC_sizing);*/
	cursor[CurMove] = XCreateFontCursor(dpy, XC_fleur);
	cursor[CurRzDnCorLeft] = XCreateFontCursor(dpy, XC_bottom_left_corner);
	cursor[CurRzDnCorRight] = XCreateFontCursor(dpy, XC_bottom_right_corner);
	cursor[CurRzMidDn] = XCreateFontCursor(dpy, XC_bottom_side);
	cursor[CurRzMidLeft] = XCreateFontCursor(dpy, XC_left_side);
	cursor[CurRzMidUp] = XCreateFontCursor(dpy, XC_top_side);
	cursor[CurRzMidRight] = XCreateFontCursor(dpy, XC_right_side);
	cursor[CurRzUpCorRight] = XCreateFontCursor(dpy, XC_top_right_corner);
	cursor[CurRzUpCorLeft] = XCreateFontCursor(dpy, XC_top_left_corner);
	/* init appearance */
    for(int i=0; i<NUMCOLORS; i++) {
        dc.colors[i][ColBorder] = getcolor( colors[i][ColBorder] );
        dc.colors[i][ColFG] = getcolor( colors[i][ColFG] );
        dc.colors[i][ColBG] = getcolor( colors[i][ColBG] );
    }
	dc.drawable = XCreatePixmap(dpy, root, DisplayWidth(dpy, screen), bh, DefaultDepth(dpy, screen));
	dc.tabdrawable = XCreatePixmap(dpy, root, DisplayWidth(dpy, screen), th, DefaultDepth(dpy, screen));
    // TODO: should the width be so huge?
	dc.celldrawable = XCreatePixmap(dpy, root, 300, 300, DefaultDepth(dpy, screen));
	dc.gc = XCreateGC(dpy, root, 0, NULL);
	XSetLineAttributes(dpy, dc.gc, 1, LineSolid, CapButt, JoinMiter);
	if(!dc.font.set)
		XSetFont(dpy, dc.gc, dc.font.xfont->fid);
   /* init system tray */
   updatesystray();
	/* init bars */
	updatebars();
	updatestatus();
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
			PropModeReplace, (unsigned char *) netatom, NetLast);
	/* select for events */
	wa.cursor = cursor[CurNormal];
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask|PointerMotionMask
			|EnterWindowMask|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
    cellDC = dc; // make a copy; //TODO, is it ok solution?

    initfont2(cellFont, &cellDC); // recall initfont on cellDC, so the original (dc's) font could be overwritten
    cellDC.h = cellDC.font.height;
	grabkeys();
}

void
showhide(Client *c) {
	if(!c)
		return;
	if(ISVISIBLE(c)) { /* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, False);
		showhide(c->snext);
	}
	else { /* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void
sigchld(int unused) {
	if(signal(SIGCHLD, sigchld) == SIG_ERR)
		die("Can't install SIGCHLD handler");
	while(0 < waitpid(-1, NULL, WNOHANG));
}

void
spawn(const Arg *arg) {
	if(fork() == 0) {
		if(dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}
}

void
tag(const Arg *arg) {
	if(selmon->sel && arg->ui & TAGMASK) {
		selmon->sel->tags = arg->ui & TAGMASK;
		focus(NULL);
		arrange(selmon);
	}
}

void
tagmon(const Arg *arg) {
	if(!selmon->sel || !mons->next)
		return;
	sendmon(selmon->sel, dirtomon(arg->i));
}

int
textnw(const char *text, unsigned int len) {
    // remove non-printing color codes before calculating width
    char *ptr = (char *) text;
    unsigned int i, ibuf, lenbuf=len;
    char buf[len+1];
	XRectangle r;

     for(i=0, ibuf=0; *ptr && i<len; i++, ptr++) {
    if(*ptr <= NUMCOLORS && *ptr > 0) {
        if (i < len) { lenbuf--; }
        } else {
        buf[ibuf]=*ptr;
        ibuf++;
        }
    }
 buf[ibuf]=0;

	if(dc.font.set) {
    XmbTextExtents(dc.font.set, buf, lenbuf, NULL, &r);
		return r.width;
	}
	return XTextWidth(dc.font.xfont, buf, lenbuf);
}

void
tile(Monitor *m) {
	unsigned int i, n, h, mw, my, ty, r;
	float mfacts = 0, sfacts = 0;
	Client *c;

    // Collect the cfacts:
	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++) {
        // TODO: siin vist korjatakse kõik vastava area (st master v stack)
        // factid kokku, et saaks kliente omavahel võrrelda;
		if(n < m->nmasters[m->curtag])
			mfacts += c->cfact;
		else
			sfacts += c->cfact;
	}
	if(n == 0)
		return;

	if(n > m->nmasters[m->curtag]) // if more clients than master slots
		mw = m->nmasters[m->curtag] ? m->ww * m->mfacts[m->curtag] : 0;
	else
		mw = m->ww;
	for(i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++, r = 0) {
		if(n == 1) {
			if (c->bw) {
				/* remove border when only one window is on the current tag */
				c->oldbw = c->bw;
				c->bw = 0;
				r = 1;
			}
		} else if(!c->bw && c->oldbw) {
			/* restore border when more than one window is displayed */
			c->bw = c->oldbw;
			c->oldbw = 0;
			r = 1;
		}
        // TODO: cfacts tekitab siin pertagi patchiga segadust
		/*if(i < m->nmaster) {*/ // oli enne
			/*h = (m->wh - my) * (c->cfact / mfacts);*/ // oli enne
/*-	          h = (m->wh - my) / (MIN(n, m->nmaster) - i);*/ // seda tahtis pertag patch eemaldada
		if(i < m->nmasters[m->curtag]) { // ie we're processing master area client
			/*h = (m->wh - my) / (MIN(n, m->nmasters[m->curtag]) - i);*/ // see oli koos pertagi ja ctagiga; ilmselgelt välistab see cfacti funktsionaalsuse;
            h = (m->wh - my) * (c->cfact / mfacts); // oli enne
			resize(c,
                   m->wx,
                   m->wy + my,
                   mw - (2*c->bw),
                   h - (2*c->bw),    False);
			if(r)
				resizeclient(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw));
			my += HEIGHT(c);
			mfacts -= c->cfact;
		} else { // slave stack;
			h = (m->wh - ty) * (c->cfact / sfacts);
			resize(c,
                   m->wx + mw,
                   m->wy + ty,
                   m->ww - mw - (2*c->bw),
                   h - (2*c->bw),    False);
			if(r)
				resizeclient(c, m->wx + mw, m->wy + ty, m->ww - mw - (2*c->bw), h - (2*c->bw));
			ty += HEIGHT(c);
			sfacts -= c->cfact;
		}
	}
}

void
togglebar(const Arg *arg) {
	selmon->showbar = !selmon->showbar;
	updatebarpos(selmon);
   resizebarwin(selmon);
   if(showsystray) {
       XWindowChanges wc;
       if(!selmon->showbar)
           wc.y = -bh;
       else if(selmon->showbar) {
           wc.y = 0;
           if(!selmon->topbar)
               wc.y = selmon->mh - bh;
       }
       XConfigureWindow(dpy, systray->win, CWY, &wc);
   }
	arrange(selmon);
}

void
tabmode(const Arg *arg) {
	if(arg && arg->i >= 0)
		selmon->showtab = arg->ui % showtab_nmodes;
	else
		selmon->showtab = (selmon->showtab + 1 ) % showtab_nmodes;
	arrange(selmon);
}


void
togglefloating(const Arg *arg) {
	if(!selmon->sel)
		return;
	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	if(selmon->sel->isfloating) {
		/* restore border when moving window into floating mode */
		if(!selmon->sel->bw && selmon->sel->oldbw) {
			selmon->sel->bw = selmon->sel->oldbw;
			selmon->sel->oldbw = 0;
		}
        XSetWindowBorder(dpy, selmon->sel->win, dc.colors[14][1]);
		/*restore last known float dimensions*/
		resize(selmon->sel, selmon->sel->sfx, selmon->sel->sfy,
		       selmon->sel->sfw, selmon->sel->sfh, False);
    } else {
        XSetWindowBorder(dpy, selmon->sel->win, dc.colors[1][ColBorder]);
		/*save last known float dimensions*/
		selmon->sel->sfx = selmon->sel->x;
		selmon->sel->sfy = selmon->sel->y;
		selmon->sel->sfw = selmon->sel->w;
		selmon->sel->sfh = selmon->sel->h;
    }
	arrange(selmon);
}

void
toggletag(const Arg *arg) {
	unsigned int i, newtags;

	if(!selmon->sel)
		return;
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
	if(newtags) {
		selmon->sel->tags = newtags;
		if(newtags == ~0) {
			selmon->prevtag = selmon->curtag;
			selmon->curtag = 0;
		}
		if(!(newtags & 1 << (selmon->curtag - 1))) {
			selmon->prevtag = selmon->curtag;
			for (i=0; !(newtags & 1 << i); i++);
			selmon->curtag = i + 1;
		}
		selmon->sel->tags = newtags;
		selmon->lt[selmon->sellt] = selmon->lts[selmon->curtag];
		focus(NULL);
		arrange(selmon);
	}
}

void
toggleview(const Arg *arg) {
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);

	if(newtagset) {
		selmon->tagset[selmon->seltags] = newtagset;
		focus(NULL);
		arrange(selmon);
	}
}

void
unfocus(Client *c, Bool setfocus) {
	if(!c)
		return;
	grabbuttons(c, False);
	if(c->isfloating)
        XSetWindowBorder(dpy, c->win, dc.colors[14][1]);
	else
        XSetWindowBorder(dpy, c->win, dc.colors[0][ColBorder]);
	if(setfocus)
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
}

void
unmanage(Client *c, Bool destroyed) {
	Monitor *m = c->mon;
	XWindowChanges wc;

	/* The server grab construct avoids race conditions. */
	detach(c);
	detachstack(c);
	if(!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
    // TODO: is this required?
    // clean client from alttab stack, if present:
    int arrLen = sizeof(clt) / sizeof(clt[0]);
    for (int i = 0; i < arrLen; i++) {
        if (clt[i] == c) {
            clt[i] = NULL;
            /*break;*/
        }
    }

	free(c);
	focus(NULL);
	arrange(m);
}

Bool isWindowClassByWin(Window w, char *class) {
    if (!w) return False;
    if (!class) return False;

    char w_class[256];
    /*char w_name[512];*/
    /*const char idea_name[] = "IntelliJ IDEA";*/

    gettextprop(w, XA_WM_CLASS, w_class, sizeof w_class);
    /*fprintf(stderr, "  @isWindowClass: was called for \"%s\" class\n", w_class);*/

    /*if(!gettextprop(c->win, netatom[NetWMName], w_name, sizeof w_name)) {*/
        /*gettextprop(c->win, XA_WM_NAME, w_name, sizeof w_name);*/
    /*}*/

    /*fprintf(stderr, "class: %s\n", w_class);*/
    /*fprintf(stderr, "name: %s\n", w_name);*/


    /*if ( strstr(w_class, idea_class) ||*/
            /*strstr(w_name, idea_name) ) {*/
        /*[>fprintf(stderr, "  !! idea class detected!\n");<]*/
        /*return True;*/
    /*}*/
    /*if ( strstr(w_name, idea_name) ) {*/
        /*return True;*/
    /*}*/
    if ( strstr( w_class, class ) ) {
        /*fprintf(stderr, "  @isWindowClass: %s class detected, returning true\n", w_class);*/
        return True;
    }

    return False;
}

Bool isWindowInSkipList(Client *c) {
    if (!c || !c->win) return False;
    /*char w_class[256];*/

    /*gettextprop(c->win, XA_WM_CLASS, w_class, sizeof w_class);*/

    for (const char **winClass = ignored_class_list; *winClass; winClass++ ) {
        /*if ( strstr( w_class, *winClass ) ) {*/
        if ( strstr( c->className, *winClass ) ) {
            /*fprintf(stderr, "  @isWindowClass: %s class detected, returning true\n", w_class);*/
            return True;
        }
    }

    return False;
    /*return isWindowClass(c, client_class_notifyd);*/
}

Bool isWindowClass(Client *c, const char *class) {
    if (!c || !c->win) return False;
    if (!class) return False;

    /*char w_class[256];*/
    /*char w_name[512];*/
    /*const char idea_name[] = "IntelliJ IDEA";*/

    /*gettextprop(c->win, XA_WM_CLASS, w_class, sizeof w_class);*/
    /*fprintf(stderr, "  @isWindowClass: was called for \"%s\" class\n", w_class);*/

    /*if(!gettextprop(c->win, netatom[NetWMName], w_name, sizeof w_name)) {*/
        /*gettextprop(c->win, XA_WM_NAME, w_name, sizeof w_name);*/
    /*}*/

    /*fprintf(stderr, "class: %s\n", w_class);*/
    /*fprintf(stderr, "name: %s\n", w_name);*/


    /*if ( strstr(w_class, idea_class) ||*/
            /*strstr(w_name, idea_name) ) {*/
        /*[>fprintf(stderr, "  !! idea class detected!\n");<]*/
        /*return True;*/
    /*}*/
    /*if ( strstr(w_name, idea_name) ) {*/
        /*return True;*/
    /*}*/
    if ( strstr( c->className, class ) ) {
        /*fprintf(stderr, "  @isWindowClass: %s class detected, returning true\n", w_class);*/
        return True;
    }

    return False;
}

Bool isWindowName(Client *c, char *name) {
    if (!c || !c->win) return False;
    if (!name) return False;

    char w_name[512];

    if(!gettextprop(c->win, netatom[NetWMName], w_name, sizeof w_name)) {
        gettextprop(c->win, XA_WM_NAME, w_name, sizeof w_name);
    }

    /*fprintf(stderr, "class: %s\n", w_class);*/
    /*fprintf(stderr, "name: %s\n", w_name);*/


    /*if ( strstr(w_class, idea_class) ||*/
            /*strstr(w_name, idea_name) ) {*/
        /*[>fprintf(stderr, "  !! idea class detected!\n");<]*/
        /*return True;*/
    /*}*/
    if ( strstr(w_name, name) ) {
        return True;
    }

    return False;
}

void
unmapnotify(XEvent *e) {
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if((c = wintoclient(ev->window))) {
		if(ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, False);
	}
   else if((c = wintosystrayicon(ev->window))) {
       removesystrayicon(c);
       resizebarwin(selmon);
       updatesystray();
   }
}

void
updatebars(void) {
   unsigned int w;
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask
	};
	for(m = mons; m; m = m->next) {
       w = m->ww;
       if(showsystray && m == selmon)
           w -= getsystraywidth();
       m->barwin = XCreateWindow(dpy, root, m->wx, m->by, w, bh, 0, DefaultDepth(dpy, screen),
					  CopyFromParent, DefaultVisual(dpy, screen),
					  CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]);
		XMapRaised(dpy, m->barwin);
		m->tabwin = XCreateWindow(dpy, root, m->wx, m->ty, m->ww, th, 0, DefaultDepth(dpy, screen),
					  CopyFromParent, DefaultVisual(dpy, screen),
					  CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		XDefineCursor(dpy, m->tabwin, cursor[CurNormal]);
		XMapRaised(dpy, m->tabwin);
        // TODO: need to define cellwin stuff here? seek for other places where
        // cellwin stuff needs to be addressed in!
	}
}

void
updatebarpos(Monitor *m) {
	Client *c;
	int nvis = 0;

	m->wy = m->my;
	m->wh = m->mh;
	if(m->showbar) {
		m->wh -= bh;
		m->by = m->topbar ? m->wy : m->wy + m->wh;
		if ( m->topbar )
			m->wy += bh;
	} else {
		m->by = -bh;
	}

	for(c = m->clients; c; c = c->next){
	  if(ISVISIBLE(c) && !c->isInSkipList) ++nvis;
	}

	if(m->showtab == showtab_always
	   || ((m->showtab == showtab_auto) &&
            ((nvis > 1 && m->lt[m->sellt]->arrange == monocle) ||
            (nvis > 2 && m->lt[m->sellt]->arrange == deck) ||
            /*(nvis > 1) &&*/m->lt[m->sellt]->arrange == NULL)
           ) ) {
		m->wh -= th;
		m->ty = m->toptab ? m->wy : m->wy + m->wh;
		if ( m->toptab )
			m->wy += th;
	} else {
		m->ty = -th;
	}
}

// TODO: along other things, here the monitors' get their properties:
Bool
updategeom(void) {
	Bool dirty = False;

#ifdef XINERAMA
	if(XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for(n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		if(!(unique = (XineramaScreenInfo *)malloc(sizeof(XineramaScreenInfo) * nn)))
			die("fatal: could not malloc() %u bytes\n", sizeof(XineramaScreenInfo) * nn);
		for(i = 0, j = 0; i < nn; i++)
			if(isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;
		if(n <= nn) {
			for(i = 0; i < (nn - n); i++) { /* new monitors available */
				for(m = mons; m && m->next; m = m->next);
				if(m)
					m->next = createmon();
				else
					mons = createmon();
			}
			for(i = 0, m = mons; i < nn && m; m = m->next, i++)
				if(i >= n
				|| (unique[i].x_org != m->mx || unique[i].y_org != m->my
				    || unique[i].width != m->mw || unique[i].height != m->mh))
				{
					dirty = True;
					m->num = i;
					m->mx = m->wx = unique[i].x_org;
					m->my = m->wy = unique[i].y_org;
					m->mw = m->ww = unique[i].width;
					m->mh = m->wh = unique[i].height;
					updatebarpos(m);
				}
		}
		else { /* less monitors available nn < n */
			for(i = nn; i < n; i++) {
				for(m = mons; m && m->next; m = m->next);
				while(m->clients) {
					dirty = True;
					c = m->clients;
					m->clients = c->next;
					detachstack(c);
					c->mon = mons;
					attach(c);
					attachstack(c);
				}
				if(m == selmon)
					selmon = mons;
				cleanupmon(m);
			}
		}
		free(unique);
	}
	else
#endif /* XINERAMA */
	/* default monitor setup */
	{
		if(!mons)
			mons = createmon();
		if(mons->mw != sw || mons->mh != sh) {
			dirty = True;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if(dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

void
updatenumlockmask(void) {
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for(i = 0; i < 8; i++)
		for(j = 0; j < modmap->max_keypermod; j++)
			if(modmap->modifiermap[i * modmap->max_keypermod + j]
			   == XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c) {
	long msize;
	XSizeHints size;

	if(!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if(size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	}
	else if(size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	}
	else
		c->basew = c->baseh = 0;
	if(size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	}
	else
		c->incw = c->inch = 0;
	if(size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	}
	else
		c->maxw = c->maxh = 0;
	if(size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	}
	else if(size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	}
	else
		c->minw = c->minh = 0;
	if(size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	}
	else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->minw && c->maxh && c->minh
		     && c->maxw == c->minw && c->maxh == c->minh);
}

void
updatetitle(Client *c) {
	if(!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if(c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
    // TODO: debug:
    /*fprintf(stderr, "    !updatetitle(): title \"%s\"\n", c->name);*/
}

void
updateClassName(Client *c) {
    gettextprop(c->win, XA_WM_CLASS, c->className, sizeof c->className);
	if(c->className[0] == '\0') /* hack to mark broken clients */
		strcpy(c->className, broken);
    // TODO: debug:
    /*fprintf(stderr, "    !updateClassName(): class \"%s\"\n", c->className);*/
}

void
updatestatus(void) {
	if(!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
		strcpy(stext, "dwm-"VERSION);
	drawbar(selmon);
}

void
updatesystrayicongeom(Client *i, int w, int h) {
   if(i) {
       i->h = bh;
       if(w == h)
           i->w = bh;
       else if(h == bh)
           i->w = w;
       else
           i->w = (int) ((float)bh * ((float)w / (float)h));
       applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
       /* force icons into the systray dimensions if they don't want to */
       if(i->h > bh) {
           if(i->w == i->h)
               i->w = bh;
           else
               i->w = (int) ((float)bh * ((float)i->w / (float)i->h));
           i->h = bh;
       }
   }
}

void
updatesystrayiconstate(Client *i, XPropertyEvent *ev) {
   long flags;
   int code = 0;

   if(!showsystray || !i || ev->atom != xatom[XembedInfo] ||
           !(flags = getatomprop(i, xatom[XembedInfo])))
       return;

   if(flags & XEMBED_MAPPED && !i->tags) {
       i->tags = 1;
       code = XEMBED_WINDOW_ACTIVATE;
       XMapRaised(dpy, i->win);
       setclientstate(i, NormalState);
   }
   else if(!(flags & XEMBED_MAPPED) && i->tags) {
       i->tags = 0;
       code = XEMBED_WINDOW_DEACTIVATE;
       XUnmapWindow(dpy, i->win);
       setclientstate(i, WithdrawnState);
   }
   else
       return;
   sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
           systray->win, XEMBED_EMBEDDED_VERSION);
}

void
updatesystray(void) {
   XSetWindowAttributes wa;
   Client *i;
   unsigned int x = selmon->mx + selmon->mw;
   unsigned int w = 1;

   if(!showsystray)
       return;
   if(!systray) {
       /* init systray */
       if(!(systray = (Systray *)calloc(1, sizeof(Systray))))
           die("fatal: could not malloc() %u bytes\n", sizeof(Systray));
       systray->win = XCreateSimpleWindow(dpy, root, x, selmon->by, w, bh, 0, 0, dc.colors[0][ColBG]);
       wa.event_mask        = ButtonPressMask | ExposureMask;
       wa.override_redirect = True;
       wa.background_pixmap = ParentRelative;
       wa.background_pixel  = dc.colors[0][ColBG];
       XSelectInput(dpy, systray->win, SubstructureNotifyMask);
       XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
               PropModeReplace, (unsigned char *)&systrayorientation, 1);
       XChangeWindowAttributes(dpy, systray->win, CWEventMask|CWOverrideRedirect|CWBackPixel|CWBackPixmap, &wa);
       XMapRaised(dpy, systray->win);
       XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
       if(XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
           sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
           XSync(dpy, False);
       }
       else {
           fprintf(stderr, "dwm: unable to obtain system tray.\n");
           free(systray);
           systray = NULL;
           return;
       }
   }
   for(w = 0, i = systray->icons; i; i = i->next) {
       XMapRaised(dpy, i->win);
       w += systrayspacing;
       XMoveResizeWindow(dpy, i->win, (i->x = w), 0, i->w, i->h);
       w += i->w;
       if(i->mon != selmon)
           i->mon = selmon;
   }
   w = w ? w + systrayspacing : 1;
   x -= w;
   XMoveResizeWindow(dpy, systray->win, x, selmon->by, w, bh);
   /* redraw background */
   XSetForeground(dpy, dc.gc, dc.colors[0][ColBG]);
   XFillRectangle(dpy, systray->win, dc.gc, 0, 0, w, bh);
   XSync(dpy, False);
}

void
updatewindowtype(Client *c) {
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if(state == netatom[NetWMFullscreen])
		setfullscreen(c, True);

	if(wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = True;
}

void
updatewmhints(Client *c) {
	XWMHints *wmh;

	if((wmh = XGetWMHints(dpy, c->win))) {
		if(c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		}
		else
			c->isurgent = (wmh->flags & XUrgencyHint) ? True : False;
            if (c->isurgent) {
                XSetWindowBorder(dpy, c->win, dc.colors[ColUrg][ColBorder]);
            }
		if(wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = False;
		XFree(wmh);
	}
}

void
view(const Arg *arg) {
    unsigned int i;

	if((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
		return;
	selmon->seltags ^= 1; /* toggle sel tagset */

	if(arg->ui & TAGMASK) {
		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
		selmon->prevtag = selmon->curtag;
		if(arg->ui == ~0)
			selmon->curtag = 0;
		else {
			for (i=0; !(arg->ui & 1 << i); i++);
			selmon->curtag = i + 1;
		}
    } else { // TODO: this block handles mod+tab functionality? (toggle *view*)
		selmon->prevtag= selmon->curtag ^ selmon->prevtag;
		selmon->curtag^= selmon->prevtag;
		selmon->prevtag= selmon->curtag ^ selmon->prevtag;
    }
	selmon->lt[selmon->sellt]= selmon->lts[selmon->curtag];
	focus(NULL);
	arrange(selmon);
}

Client *
wintoclient(Window w) {
	Client *c;
	Monitor *m;

	for(m = mons; m; m = m->next)
		for(c = m->clients; c; c = c->next)
			if(c->win == w)
				return c;
	return NULL;
}

Monitor *
wintomon(Window w) {
	int x, y;
	Client *c;
	Monitor *m;

	if(w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for(m = mons; m; m = m->next)
		if(w == m->barwin || w == m->tabwin)
			return m;
	if((c = wintoclient(w)))
		return c->mon;
	return selmon;
}

Client *
wintosystrayicon(Window w) {
   Client *i = NULL;

   if(!showsystray || !w)
       return i;
   for(i = systray->icons; i && i->win != w; i = i->next) ;
   return i;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.  */
int
xerror(Display *dpy, XErrorEvent *ee) {
	if(ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
			ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee) {
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee) {
	die("dwm: another window manager is already running\n");
	return -1;
}

void
zoom(const Arg *arg) {
	Client *c = selmon->sel;

	if(!selmon->lt[selmon->sellt]->arrange
	|| (selmon->sel && selmon->sel->isfloating))
		return;
	if(c == nexttiled(selmon->clients))
		if(!c || !(c = nexttiled(c->next)))
			return;
	pop(c);
}

void
togglescratch(const Arg *arg) {
	Client *c = NULL;
	unsigned int found = 0;

	for(c = selmon->clients; c && !(found = c->tags & scratchtag); c = c->next);
	if(found) {
		unsigned int newtagset = selmon->tagset[selmon->seltags] ^ scratchtag;
		if(newtagset) {
			selmon->tagset[selmon->seltags] = newtagset;
			focus(NULL);
			arrange(selmon);
		}
		if(ISVISIBLE(c)) {
			focus(c);
			restack(selmon);
		}
	}
	else
		spawn(arg);
}

int
main(int argc, char *argv[]) {
	if(argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-"VERSION", © 2006-2011 dwm engineers, see LICENSE for details\n");
	else if(argc != 1)
		die("usage: dwm [-v]\n");
	if(!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if(!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display\n");
	checkotherwm();
	setup();
	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}

void
reload(const Arg *arg) {
       if (arg->v) {
              execvp(((char **)arg->v)[0], (char **)arg->v);
       } else {
              execlp("dwm", "dwm", NULL);
       }
}

void
cycle(const Arg *arg) {
    int seltags = selmon->tagset[selmon->seltags];
    int numtags = (LENGTH(tags) - 1);
    int delta = arg->i;
    int curtag = -1;

    while(!(seltags & (1 << ++curtag)));
    curtag = (curtag + delta) % numtags;
    if(curtag < 0) curtag += numtags;

    const Arg a = { .i = 1 << curtag };
    view(&a);
}

void
tagcycle(const Arg *arg) {
    int seltags = selmon->tagset[selmon->seltags];
    int numtags = (LENGTH(tags) - 1);
    int delta = arg->i;
    int curtag = -1;

    while(!(seltags & (1 << ++curtag)));
    curtag = (curtag + delta) % numtags;
    if(curtag < 0) curtag += numtags;

    const Arg a = { .i = 1 << curtag };
    tag(&a);
    view(&a);
}


//////////////// ALT-TAB:
Window
getWindowIconWindow (Client *c) {
	XWMHints *wmh;
    Window icon_w = NULL;

	if((wmh = XGetWMHints(dpy, c->win))) {
        fprintf(stderr, "in getWinIconWindow: wmh is not null for %s :)\n ", c->name);
        icon_w = wmh->icon_window;
        if (!icon_w)
            fprintf(stderr, "in getWinIconWindow: %s icon_w is NULL :((((\n", c->name);
		/*if(wmh->flags & InputHint)*/
			/*c->neverfocus = !wmh->input;*/
		/*else*/
			/*c->neverfocus = False;*/
		XFree(wmh);
	} else {
        fprintf(stderr, "in getWinIconWindow: wmh IS NULL:(\n");
    }


    return icon_w;
}

Pixmap
getWindowIcon (Client *c) {
	XWMHints *wmh;
    Pixmap pxmp = NULL;

	if(wmh = XGetWMHints(dpy, c->win)) {
        fprintf(stderr, "in getWinIcon: wmh is not null for %s :)\n ", c->name);
        pxmp = wmh->icon_pixmap;
        if (!pxmp)
            fprintf(stderr, "in getWinIcon: %s pixmap is NULL :((((\n", c->name);
		/*if(wmh->flags & InputHint)*/
			/*c->neverfocus = !wmh->input;*/
		/*else*/
			/*c->neverfocus = False;*/
		XFree(wmh);
	}

    return pxmp;
}

void altTab() {
     /*selmon->selclt ^= 1;*/

	/*if(selmon->clt[selmon->selclt]) {*/
        /*focus(selmon->clt[selmon->selclt]);*/
		/*restack(selmon);*/
        /*return;*/
    /*}*/

     /*selmon->selclt ^= 1;*/
    /*return;*/


    Monitor *newMon, *prevMon;
    prevMon = selmon;

 	selclt ^= 1;
	if(clt[selclt]) {
        focus(clt[selclt]);
        newMon = wintomon(clt[selclt]->win);
        restack(newMon);
        if (newMon != prevMon) {
            if (mouse_follows_focus || !transfer_pointer) return; // no point in moving cursor, if it's already following focus;
            transferPointerToNextMon(prevMon);
        }

        return;
    }

    // reset:
 	selclt ^= 1;
    return;
}

void altTab_2() {
   fprintf(stderr, "\nsending mod-alt-c-s-ü:\n\n");


    /*writeToClipBoard("leeeeelbawks");*/

    /*sendKeyEvent(XK_F7, ControlMask|AltMask|ShiftMask, KeyPress);*/
    sendKeyEvent(XK_f, Mod4Mask, KeyPress);
    return;



    /*XAllowEvents(dpy, AsyncKeyboard, (*(XEvent*)arg->v).xkey.time);*/
    /*XAllowEvents(dpy, ReplayKeyboard, (*(XEvent*)arg->v).xkey.time);*/
    /*XFlush(dpy); // has to go with replaykeyboard!!!*/




   XEvent ev;
   KeySym keySym;
   int keyMod;
   /*KeySym tabKey = XKeysymToKeycode(dpy, XK_Tab);*/
   /*KeySym altKey = XKeysymToKeycode(dpy, XK_Alt_L);*/
   /*KeyCode tabKey = XKeysymToKeycode(dpy, XK_Tab);*/
   const KeyCode altKeyCode = XKeysymToKeycode(dpy, XK_Alt_L);
   const KeyCode tabKeyCode = XKeysymToKeycode(dpy, XK_Tab);
   fprintf(stderr, "\nstart of altTab function\n\n");

    if ( !selmon->cellwin ) {
        selmon->cellwin = XCreateSimpleWindow(dpy, root, 1, 1, 1, 1, 0, 0, dc.colors[0][ColBG]);
    }

                        // Prolly bad idea
                        /*XGrabKey(dpy, AnyKey, AnyModifier, root,*/
						 /*True, GrabModeAsync, GrabModeAsync);*/
    /*if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,*/
        /*None, cursor[CurMove], CurrentTime) != GrabSuccess)*/
        /*return;*/

    /*if(XGrabKeyboard(dpy, root, False, */
 /*XGrabKey( dpy, AnyKey, AnyModifier, root, False, GrabModeAsync, GrabModeAsync);*/
                /*fprintf(stderr, "could not grab \n");*/

					/*XGrabKey(dpy, altKeyCode, AnyModifier, root,*/
						 /*True, GrabModeAsync, GrabModeAsync);*/
                    // TODO:! (tries same grabber as in grabkeys(), ie the one confirmed to work)
                    // prolly doesn't work though since already-tried 'anyModifier'  includes not modifiers)
    /*return;*/
/*}*/


    // need to call first time outside of the loop:
    updateAndDrawAltTab(selmon);


    /*XUngrabKey(dpy, tabKeyCode, 0, root);*/
    /*XUngrabKey(dpy, tabKeyCode, AltMask, root);*/
    /*[>XUngrabKey(dpy, AnyKey, AnyModifier, root);<]*/

    /*XGrabKey(dpy, altKeyCode, 0, root, True, GrabModeAsync, GrabModeAsync);*/
    /*XGrabKey(dpy, tabKeyCode, AnyModifier, root, True, GrabModeAsync, GrabModeAsync);*/

    int c = 0; // TODO deleteme
    do {
        fprintf(stderr, "new cycle of do-loop.\n");
        /*XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);*/
        /*XMaskEvent(dpy, AltMask|Mod1Mask|ExposureMask|SubstructureRedirectMask|KeyPressMask|KeyReleaseMask, &ev);*/
        XMaskEvent(dpy, KeyPressMask|KeyReleaseMask, &ev);

        keySym = XKeycodeToKeysym(dpy, ev.xkey.keycode, 0);
        keyMod = ev.xkey.state;

        fprintf(stderr, "detected key: %d\n", keySym);
        switch (ev.type) {
            /*case ConfigureRequest:*/
            /*case Expose:*/
            /*[>case MotionNotify: // TODO: do we want to pass this one?<]*/
            /*case MapRequest:*/
                /*handler[ev.type](&ev);*/
                /*break;*/
            case KeyPress:
                fprintf(stderr, "keypress event detected\n");
                //TODO siia loogika?
                if ( keySym == XK_Tab ) {
                    fprintf(stderr, "keypress, tab, launching updateAndDrawAltTab...\n");
                    updateAndDrawAltTab(selmon);
                    fprintf(stderr, "... exited updateAndDrawAltTab\n");
                }
                break;
            // TODO: KeyRelease is only for debugging purposes:
            case KeyRelease:
                fprintf(stderr, " keyrelease event detected\n");
                if ( keySym == XK_Alt_L ) {
                    fprintf(stderr, "keyrelease, alt\n");
                }

                if(keySym == XK_Tab
                        && CLEANMASK(AltMask) == CLEANMASK(keyMod) ){
                    fprintf(stderr, "     @upddate: altTab release detected!!!!!! \n");
                }


                /*if ( keySym == XK_Alt_L ) return;*/
                break;
        }

        // TODO: just-in-case counter; deleteme:
        c++;
        if (c > 9) {
            fprintf(stderr, "     exiting because of safety counter count %d\n ", c);
            break;
        }
    } while(ev.type != KeyRelease || keySym != XK_Alt_L ); // until alt is released
    /*} while(ev.type != KeyRelease || ( keySym != XK_Alt_L && !(keySym & Mod1Mask) )); // until alt is released*/

   fprintf(stderr, "exited from the loop!\n");
    /*XUngrabKey(dpy, AnyKey, AnyModifier, root);*/


    /*XUngrabKey(dpy, altKeyCode, 0, root);*/
    /*XGrabKey(dpy, tabKeyCode, AltMask, root,*/
            /*True, GrabModeAsync, GrabModeAsync);*/




    /*grabkeys();*/





    /*XUngrabKey(dpy, tabKeyCode, Mod1Mask, root);*/

    // Finally, hide away the window:
   /*XLowerWindow( dpy, m->cellwin);*/
   // OR???:
   XMoveWindow(dpy, selmon->cellwin, cellWidth * -2, 0); // hide
   // TODO: is xsync needed here?:
   XSync(dpy, False);
}

void
updateAndDrawAltTab(Monitor *m) {
   int MAX_CLIENTS = 10; //TODO move out into config
   Pixmap clientIcon;
   Window clientIconWin;
   unsigned long *col;
   Client *c;
   int i;
   int itag = -1;
   char view_info[50];
   int view_info_w = 0;
   int sorted_label_widths[MAX_CLIENTS];
   int tot_width;
   int maxsize = bh;
   int nrOfCells;
   int singleCellHeight = cellDC.font.height; //TODO; kui mingieid vahealasid teha, siis siit alusta?
   int totalCellHeight = 0;
   int ch; // TODO needs to be calculated
   int cwy, cwx; // cell window origin


   /*if ( !m->cellwin )*/
       /*m->cellwin = XCreateSimpleWindow(dpy, root, 100, 100, 100, 100, 0, 0, dc.colors[0][ColBG]);*/

    //TODO: move this out? make const? what is it for lel?
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask
	};

   // Calculate total cell block height
   for( i = 0, c = m->clients; c; c = c->next, i++){
     if(!ISVISIBLE(c) || c->isInSkipList) continue;

     totalCellHeight += singleCellHeight;
     if(i >= MAX_CLIENTS) break;
   }
   if(!totalCellHeight) return;
   /*fprintf(stderr, "  single cell height: %d\n", singleCellHeight);*/
   /*fprintf(stderr, "  cellwin total height: %d\n", totalCellHeight);*/

    /*if(cellDC.celldrawable != 0)*/
        /*XFreePixmap(dpy, cellDC.celldrawable);*/
	/*cellDC.celldrawable = XCreatePixmap(dpy, root, cellWidth, totalCellHeight, DefaultDepth(dpy, screen));*/

   //TODO: better it
   /*if (!totalCellHeight) return;*/
   /*totalCellHeight=1;*/
   /*totalCellHeight = (totalCellHeight == 0) ? 50 : totalCellHeight;*/

   cwy = m->wh/2 - totalCellHeight/2;
   cwx = m->wx + m->ww/2 - cellWidth/2;


   /*dc.x = m->wx+300;*/
   /*dc.y = cy;*/
   cellDC.x = 0;
   cellDC.y = 0;
   cellDC.w = cellWidth;

   //view_info: indicate the tag which is displayed in the view:
   /*
    *for(i = 0; i < LENGTH(tags); ++i){
    *  if((selmon->tagset[selmon->seltags] >> i) & 1) {
    *    if(itag >=0){ //more than one tag selected
    *      itag = -1;
    *      break;
    *    }
    *    itag = i;
    *  }
    *}
    *if(0 <= itag  && itag < LENGTH(tags)){
    *  snprintf(view_info, sizeof view_info, "[%s]", tags[itag]);
    *} else {
    *  strncpy(view_info, "[...]", sizeof view_info);
    *}
    */
   /*view_info[sizeof(view_info) - 1 ] = 0;*/
   /*view_info_w = TEXTW(view_info);*/
   /*tot_width = view_info_w;*/

   /* Calculates number of labels and their width */
   /*for( i = 0, c = m->clients; c; c = c->next, i++){*/
     /*if(!ISVISIBLE(c)) continue;*/
     /*totalCellHeight += singleCellHeight;*/
     /*if(i >= MAX_CLIENTS) break;*/
   /*}*/

// TODO: do we need this?
   /*if(tot_width > m->ww){ //not enough space to display the labels, they need to be truncated*/
     /*memcpy(sorted_label_widths, m->tab_widths, sizeof(int) * m->ntabs);*/
     /*qsort(sorted_label_widths, m->ntabs, sizeof(int), cmpint);*/
     /*tot_width = view_info_w;*/
     /*for(i = 0; i < m->ntabs; ++i){*/
       /*if(tot_width + (m->ntabs - i) * sorted_label_widths[i] > m->ww)*/
         /*break;*/
       /*tot_width += sorted_label_widths[i];*/
     /*}*/
     /*maxsize = (m->ww - tot_width) / (m->ntabs - i);*/
   /*} else{*/
     /*maxsize = m->ww;*/
   /*}*/

   /*for( i = 0, c = m->clients; c; c = c->next ) {*/
        /*fprintf(stderr, "  !!! trying to fetx clientIconWIN___ for %s...\n: ", c->name);*/
        /*clientIconWin = getWindowIconWindow(c); // TODO*/

        /*if (clientIconWin) {*/
            /*fprintf(stderr, "  !!! found clientIconWIN___ for %s\n: ", c->name);*/
            /*break;*/
        /*}*/
    /*}*/
   for( i = 0, c = m->clients; c; c = c->next ) {
        fprintf(stderr, "  !!! trying to fetx clientIcon___ for %s...\n: ", c->name);
        clientIcon= getWindowIcon(c); // TODO

        if (clientIcon) {
            fprintf(stderr, "  !!! found clientIcon___ for %s\n: ", c->name);
            break;
        }
    }

   for( i = 0, c = m->clients; c; c = c->next ) {
     if(!ISVISIBLE(c) || c->isInSkipList) continue;
     if(i >= MAX_CLIENTS) break;

        clientIcon = getWindowIcon(c); // TODO
        /*clientIconWin = getWindowIconWindow(c); // TODO*/

        /*if (clientIconWin) {*/
            /*fprintf(stderr, "  !!! found clientIconWIN___ for %s\n: ", c->name);*/
            /*break;*/
        /*}*/
        if (clientIcon) {
            fprintf(stderr, "  !!! found clientIcon___ for %s\n: ", c->name);
            break;
        }

    // coloring:
     if( m->nmasters[m->curtag] > 1 && i < m->nmasters[m->curtag]) // more than one master client
        // color masters differently:
        col = (c == m->sel) ? cellDC.colors[16] : cellDC.colors[15];
     else // single master client
        col = (c == m->sel) ? cellDC.colors[13] : cellDC.colors[12];
        /*
         *col = dc.colors[ (m->tagset[m->seltags] & 1 << i) ?
         *1 : (urg & 1 << i ? 2:0) ]
         */





     /*drawCells(cellDC.celldrawable, c->name, col, 0);*/



     /*drawTabbarTextDELETEME(cellDC.celldrawable, c->name, col, 0);*/
     /*drawTabbarText(dc.celldrawable, c->name, col, 0);*/

     // XPutImage??

     cellDC.y += singleCellHeight;
     i++; // may not be in loop control!
   }





   /*[> cleans interspace between window names and current viewed tag label <]*/
   /*dc.w = m->ww - view_info_w - dc.x;*/
   /*drawCells(dc.tabdrawable, NULL, dc.colors[0], 0);*/

   /*[> view info <]*/
   /*dc.x += dc.w;*/
   /*dc.w = view_info_w;*/
   /*drawCells(dc.tabdrawable, view_info, dc.colors[0], 0);*/

   // first clean up:
   // TODO: moveresize instead?
   /*if ( m->cellwin ) {*/
	/*XUnmapWindow(dpy, m->cellwin);*/
	/*XDestroyWindow(dpy, m->cellwin);*/
   /*}*/

        // first create the xwindow; TODO: does it really need to be re-created errytime?
        // perhaps hide it and expose later if re-used?
        // TODO2: cellHeight here is the total, and should already be calculated by this moment;
        // TODO3: cellwin Window struct alt siia kohalikku tuua? ei näe pointi
        // monitoriga sidumiseks;
		/*m->cellwin = XCreateWindow(dpy, root, cwx, cwy, cellWidth, totalCellHeight, 10, DefaultDepth(dpy, screen),*/
					  /*CopyFromParent, DefaultVisual(dpy, screen),*/
					  /*CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);*/

   /*if ( !m->cellwin )*/
       /*m->cellwin = XCreateSimpleWindow(dpy, root, cwx, cwy, cellWidth, totalCellHeight, 0, 0, dc.colors[0][ColBG]);*/
    XMoveResizeWindow(dpy, m->cellwin, cwx, cwy, cellWidth, totalCellHeight);
    XDefineCursor(dpy, m->cellwin, cursor[CurNormal]);




    // this one is in works:
   /*XCopyArea(dpy, cellDC.celldrawable, m->cellwin, cellDC.gc, 0, 0, cellWidth, totalCellHeight, 0, 0);*/

   if(!clientIcon) {
        fprintf(stderr, "no clienticon for %s\n", c->name);
        XMapRaised(dpy, m->cellwin);
        XSync(dpy, False);
       return;
    }
   /*if(!clientIconWin) {*/
        /*fprintf(stderr, "no clienticonWIN for %s, returning\n", c->name);*/
        /*XMapRaised(dpy, m->cellwin);*/
        /*XSync(dpy, False);*/
        /*return;*/
    /*}*/




   /*int dummy_i;*/
    unsigned int px_w, px_h, dummy_i;
   Window icon_w, dummy_w;
   // find the pixmap dimensions:
   XGetGeometry(dpy, clientIcon, &dummy_w, &dummy_i, &dummy_i, &px_w, &px_h, &dummy_i, &dummy_i );
    fprintf(stderr, "pixmap w: %d, h: %d\n", px_w, px_h);

    // create new window with the same dimensions as is the pixmap (so it could be resized)
    icon_w = XCreateSimpleWindow(dpy, root, 0, 0, px_w, px_h, 0, 0, dc.colors[0][ColBG]);

    // sanity check on newly created win:
    /*XGetGeometry(dpy, icon_w, &dummy_w, &dummy_i, &dummy_i, &px_w, &px_h, &dummy_ii, &dummy_ii );*/
    /*fprintf(stderr, "sanity: new win w: %d, h: %d\n",px_w , px_h);*/


	XMapWindow(dpy, icon_w);
    /*XMoveResizeWindow(dpy, icon_w, 0, 0, 200, 200);*/ //TODO: if you resize before copying in the pixmap, then it's OK;
   XMapRaised(dpy, icon_w); // ! TODO: for some reason win has to be raised before xCopyArea.
    // copy pixmap to new win:
   XCopyArea(dpy, clientIcon, icon_w, cellDC.gc, 0, 0, px_w, px_h, 0, 0);

   // resize:
   // !!!!!!!!!!! TODO: this fucks shit up:
    /*XMoveResizeWindow(dpy, icon_w, 0, 0, 200, 200);*/
    /*XMoveResizeWindow(dpy, icon_w, 0, 0, px_w-1, px_h-1);*/
    XResizeWindow(dpy, icon_w, px_w+1, px_h+1);


   //TODO: is this raising nexessary?:
   XMapRaised(dpy, icon_w);

   /*XCopyArea(dpy, clientIcon, m->cellwin, cellDC.gc, 0, 0, 70, 70, 0, 0);*/
   XCopyArea(dpy, icon_w, m->cellwin, cellDC.gc, 0, 0, 50, 50, 0, 0);

    // !:
   /*XCopyArea(dpy, icon_w, m->cellwin, cellDC.gc, 0, 0, 70, 70, 0, 0);*/



   XMapRaised(dpy, m->cellwin);
   XSync(dpy, False);
   XFlush(dpy);

}

void
drawCells(Drawable drawable, const char *text, unsigned long col[ColLast], Bool pad) {
	char buf[256];
	int i, x, y, h, len, olen;
    const short lenOfTrailingWhitespace = 4;
    const short lenOfTrailingSymbols = 3;
    const short lenOfTruncationDots = 3;
    const short isDefaultTabWidth = ( cellDC.w == tabWidth ) ? 1 : 0;
    const char trailingSymbol = '>';

	XSetForeground(dpy, cellDC.gc, col[ColBG]);
	/*XFillRectangle(dpy, cellDC.drawable, cellDC.gc, cellDC.x, cellDC.y, cellDC.w, cellDC.h);*/
	XFillRectangle(dpy, drawable, cellDC.gc, cellDC.x, cellDC.y, cellDC.w, cellDC.h);
	if(!text)
		return;
	olen = strlen(text)+lenOfTrailingWhitespace; // create 4-width buffer for tabs which do NOT need to be truncated;
    h = pad ? (cellDC.font.ascent + cellDC.font.descent) : 0;
    y = cellDC.y + ((cellDC.h + cellDC.font.ascent - cellDC.font.descent) / 2);
	x = cellDC.x + (h / 2);
	/* shorten text if necessary */
	for(len = MIN(olen, sizeof buf); len && textnw(text, len) > cellDC.w - h; len--);
	if(!len)
		return;
	memcpy(buf, text, len);

    /*if ( dc.w == 200 ) { // meaning default tab width, i.e. there's enough room for all the tabs on the bar;*/
        /*if(len < olen)*/
            /*for(i = len-7; i && i > len - 10; buf[--i] = '.');*/
        /*for(i = len-4; i && i > len - 7; buf[--i] = '>');*/
        /*for(i = len; i && i > len - 4; buf[--i] = ' ');*/
    /*} else {*/
        /*if(len < olen)*/
            /*for(i = len; i && i > len - 3; buf[--i] = '.');*/
    /*}*/

    if (isDefaultTabWidth) { // meaning default tab width, i.e. there's enough room for all the tabs on the bar;
        if(len < olen) { // truncate
            // locate starting point by absolute values...:
            /*for(i = len-(lenOfTrailingSymbols+lenOfTrailingWhitespace); i && i > len - (lenOfTrailingSymbols+lenOfTrailingWhitespace+lenOfTruncationDots); buf[--i] = '.');*/
            /*for(i = len-lenOfTrailingWhitespace; i && i > len - (lenOfTrailingSymbols+lenOfTrailingWhitespace); buf[--i] = trailingSymbol);*/

            for(i = len; i && i > len - (lenOfTrailingWhitespace); buf[--i] = ' ');
            // ...or relative:
            for(; i && i > len - (lenOfTrailingSymbols+lenOfTrailingWhitespace); buf[--i] = trailingSymbol);
            for(; i && i > len - (lenOfTrailingSymbols+lenOfTrailingWhitespace+lenOfTruncationDots); buf[--i] = '.');
        } else {
            for(i = len; i && i > len - (lenOfTrailingWhitespace-1); buf[--i] = trailingSymbol);
            buf[--i] = ' ';
        }
    } else {
        if(len < olen) // truncation for shorter-than-default tab widths:
            for(i = len; i && i > len - 3; buf[--i] = '.');
    }

	XSetForeground(dpy, cellDC.gc, col[ColFG]);
	if(cellDC.font.set)
		XmbDrawString(dpy, drawable, cellDC.font.set, cellDC.gc, x, y, buf, len);
	else
    XDrawString(dpy, drawable, cellDC.gc, x, y, buf, len);
}

// file reading logic from http://stackoverflow.com/questions/14834267/reading-a-text-file-backwards-in-c
/* File must be open with 'b' in the mode parameter to fopen() */
long fsize(FILE* binaryStream) {
  long ofs, ofs2;
  int result;

  if (fseek(binaryStream, 0, SEEK_SET) != 0 ||
      fgetc(binaryStream) == EOF)
    return 0;

  ofs = 1;

  while ((result = fseek(binaryStream, ofs, SEEK_SET)) == 0 &&
         (result = (fgetc(binaryStream) == EOF)) == 0 &&
         ofs <= LONG_MAX / 4 + 1)
    ofs *= 2;

  /* If the last seek failed, back up to the last successfully seekable offset */
  if (result != 0)
    ofs /= 2;

  for (ofs2 = ofs / 2; ofs2 != 0; ofs2 /= 2)
    if (fseek(binaryStream, ofs + ofs2, SEEK_SET) == 0 &&
        fgetc(binaryStream) != EOF)
      ofs += ofs2;

  /* Return -1 for files longer than LONG_MAX */
  if (ofs == LONG_MAX)
    return -1;

  return ofs + 1;
}

/* File must be open with 'b' in the mode parameter to fopen() */
/* Set file position to size of file before reading last line of file */
char* fgetsr(char* buf, int n, FILE* binaryStream) {
  long fpos;
  int cpos;
  int first = 1;

  if (n <= 1 || (fpos = ftell(binaryStream)) == -1 || fpos == 0)
    return NULL;

  cpos = n - 1;
  buf[cpos] = '\0';

  for (;;) {
    int c;

    if (fseek(binaryStream, --fpos, SEEK_SET) != 0 ||
        (c = fgetc(binaryStream)) == EOF)
      return NULL;

    if (c == '\n' && first == 0) /* accept at most one '\n' */
      break;
    first = 0;

    if (c != '\r') /* ignore DOS/Windows '\r' */ {
      unsigned char ch = c;
      if (cpos == 0) {
        memmove(buf + 1, buf, n - 2);
        ++cpos;
      }
      memcpy(buf + --cpos, &ch, 1);
    }

    if (fpos == 0) {
      fseek(binaryStream, 0, SEEK_SET);
      break;
    }
  }

  memmove(buf, buf + cpos, n - cpos);

  return buf;
}

pid_t getProcessId(const char processName[]) {
    int MAX_COMMAND_NAME_LEN = 80;
    const char command_head[] = "pidof ";
    char *cmd_string[MAX_COMMAND_NAME_LEN];
    int LEN = 10;
    char pid_line[LEN];

    if (MAX_COMMAND_NAME_LEN - strlen(processName) - strlen(command_head) < 0) {
        // string wouldn't fit
        fprintf(stderr, "failed to get the pid for \'%s\', since it's name doesn't fit into the command_string array. Make the array larger.\n", processName);
        return 0;
    }

    strcpy(cmd_string, command_head);
    strcat(cmd_string, processName);
    FILE *cmd = popen(cmd_string, "r");
    /*FILE *cmd = popen("pidof synergys", "r");*/

    fgets(pid_line, sizeof(pid_line), cmd);
    pid_t pid = strtoul(pid_line, NULL, 10);

    return pid;
}


// TODO name and return type:
int getLastOccurrenceInLog(char line[]) {
  FILE* f;
  long sz;

    // get the pid; if nothing returned, then not running;
    /*if (!getProcessId("synergys")) {*/
        /*fprintf(stderr, "%s appears not to be running, since no PID was found for that process name\n", "synergys");*/
        /*// 0 was returned, meaning no process*/
        /*return 0;*/
    /*}*/

    // TODO check if file exists and is readable

  if ((f = fopen(synergy_log_file, "rb")) == NULL) {
    fprintf(stderr, "failed to open file \'%s\'\n", synergy_log_file);
    return -1;
  }

  sz = fsize(f);

//  printf("file size: %ld\n", sz);

  if (sz > 0) {
    char buf[256];
    fseek(f, sz, SEEK_SET);

    fprintf(stderr, "   !!!!!!!!!!!!!!  starting reading log backwards!!!!!!!!:\n");

    while (fgetsr(buf, sizeof(buf), f) != NULL)
      fprintf(stderr, "%s", buf);
  }

  fclose(f);
  return 0;
}

Bool writeToClipBoard(const char content[]) {
    int MAX_STR_LEN = 256;
    int fd[2];
    pid_t pid_a, pid_b;
    const char echo_command_head[] = "echo ";
    char *argv[2];
    char *cmd_string[MAX_STR_LEN];

    if (MAX_STR_LEN - strlen(content) - strlen(echo_command_head) < 0) {
        // string wouldn't fit
        fprintf(stderr, "failed copy \'%s\' to clipboard, since the content didn't fit into the array. Make the array larger.\n", content);
        return 0;
    }

    strcpy(cmd_string, *echo_command_head);
    strcat(cmd_string, content);
    int pid;
    char *lschar[20]={"ls",NULL};
    char *morechar[20]={"more", NULL};
    pid = fork();
    if (pid == 0) {
    /* child */
    int cpid;
    pipe(fd);
    cpid = fork();
    if(cpid == 0) {
      //printf("\n in ls \n");
      dup2(fd[1], STDOUT);
      close(fd[0]);
      close (fd[1]);
      execvp("ls",lschar);
    } else if(cpid>0) {
      dup2(fd[0],STDIN);
      close(fd[0]);
      close(fd[1]);
      execvp("more", morechar);
    }
  } else if (pid > 0) {
    /* Parent */
    waitpid(pid, NULL,0);
  }
  return 0;
}

Bool writeToClipBoard3(const char content[]) {
    int MAX_STR_LEN = 256;
    int fd[2];
    pid_t pid_a, pid_b;
    const char echo_command_head[] = "echo ";
    char *argv[2];
    char *cmd_string[MAX_STR_LEN];

    if (MAX_STR_LEN - strlen(content) - strlen(echo_command_head) < 0) {
        // string wouldn't fit
        fprintf(stderr, "failed copy \'%s\' to clipboard, since the content didn't fit into the array. Make the array larger.\n", content);
        return 0;
    }

    strcpy(cmd_string, *echo_command_head);
    strcat(cmd_string, content);

    pipe(fd);


    if (!(pid_a = fork())) {
        close(0); // child closing stdin
        dup(fd[0]); // copies fd of read end of pipe into its fd ie 0 (stdin)
        close(0); // child closing stdin
        close(1); // child closing stdin
        /*exec("/bin/wc", argv);*/
        run_sys_call("/bin/wc");
    } else {
        write(fd[1], "hello world\n", 12);
        close(fd[0]);
        close(fd[1]);
    }
}

Bool writeToClipBoard2(const char content[]) {
    int MAX_STR_LEN = 256;
    int filedes[2];
    pid_t pid_a, pid_b;
    const char echo_command_head[] = "echo ";
    char *cmd_string[MAX_STR_LEN];

    if (MAX_STR_LEN - strlen(content) - strlen(echo_command_head) < 0) {
        // string wouldn't fit
        fprintf(stderr, "failed copy \'%s\' to clipboard, since the content didn't fit into the array. Make the array larger.\n", content);
        return 0;
    }

    strcpy(cmd_string, *echo_command_head);
    strcat(cmd_string, content);
    pipe(filedes);


    if (!(pid_a = fork())) {
        dup2(filedes[1], 1);
        closepipes(filedes);
        run_sys_call(cmd_string);
    }

    if (!(pid_b = fork())) {
        dup2(filedes[0], 0);
        closepipes(filedes);
        run_sys_call("xclip");
    }

        /*closepipes(filedes);*/

    waitpid(pid_a, NULL, 0);
    waitpid(pid_b, NULL, 0);

    return 0;

}

void closepipes(int *fds) {
 close(fds[0]);
 close(fds[1]);
}

void run_sys_call(char *cmd) {
   char *args[2];
   args[0] = cmd;
   args[1] = NULL;

   execvp(cmd, args);
}
