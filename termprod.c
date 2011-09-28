/*
 * termprod.c
 *
 * Compilation: cc -lX11 -lImlib2 termprod.c -o termprod
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <string.h>

/* Xlib */
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

/* Imlib2 */
#include <Imlib2.h>

/* Options */
#define FONT "fixed"
#define BGCOLOR 0xCCCCCC
#define BGTEXT  0xAAAAAA
#define FGTEXT  0x222222

#define LOGOP "logo.png"
#define LOGOW 300
#define LOGOH 130

/* Structures */
struct termprod
{
     Display *dpy;
     Window root;
     int xscreen, xdepth;
     GC gc;
     char buffer[128];

     /* Font */
     struct
     {
          int as, de, width, height;
          XFontSet fontset;
     } font;


};

struct termprod *tp;

static void
draw_image(Window dr, int x, int y, int w, int h, char *name)
{
     Imlib_Image image;

     imlib_context_set_display(tp->dpy);
     imlib_context_set_visual(DefaultVisual(tp->dpy, tp->xscreen));
     imlib_context_set_colormap(DefaultColormap(tp->dpy, tp->xscreen));
     imlib_context_set_drawable(dr);

     image = imlib_load_image(name);
     imlib_context_set_image(image);

     /*
        imlib_image_get_width();
        imlib_image_get_height();
     */

     imlib_render_image_on_drawable_at_size(x, y, w, h);

     imlib_free_image();
}

static void
tp_x_init(void)
{
     XFontStruct **xfs = NULL;
     char **misschar, **names, *defstring;
     int d;
     XSetWindowAttributes at =
     {
          .event_mask = (KeyPressMask
                    | KeyReleaseMask
                    | PropertyChangeMask
                    | SubstructureRedirectMask
                    | SubstructureNotifyMask
                    | StructureNotifyMask),
          .cursor = XCreateFontCursor(tp->dpy, XC_xterm)
     };

     /* element X importante */
     tp->xscreen = DefaultScreen(tp->dpy);
     tp->xdepth  = DefaultDepth(tp->dpy, tp->xscreen);
     tp->gc      = DefaultGC(tp->dpy, tp->xscreen);
     tp->root    = RootWindow(tp->dpy, tp->xscreen);

     XChangeWindowAttributes(tp->dpy, tp->root, CWEventMask | CWCursor, &at);

     /* fonte */
     setlocale(LC_CTYPE, "");

     tp->font.fontset = XCreateFontSet(tp->dpy, FONT, &misschar, &d, &defstring);

     XExtentsOfFontSet(tp->font.fontset);
     XFontsOfFontSet(tp->font.fontset, &xfs, &names);

     tp->font.as    = xfs[0]->max_bounds.ascent;
     tp->font.de    = xfs[0]->max_bounds.descent;
     tp->font.width = xfs[0]->max_bounds.width;

     tp->font.height = tp->font.as + tp->font.de;

     memset(tp->buffer, 0, sizeof(tp->buffer));

     if(misschar)
          XFreeStringList(misschar);
}

static void
tp_render(void)
{
     XClearWindow(tp->dpy, tp->root);
     XSetWindowBackground(tp->dpy, tp->root, BGCOLOR);
     draw_image(tp->root, 0, 0, LOGOW, LOGOH, LOGOP);
}

static void
tp_x_event(XEvent *ev)
{
     char tmp[32] = { 0 };
     KeySym ks;

     switch(ev->type)
     {
          case ClientMessage:
               break;

          case KeyPress:
               XLookupString(&ev->xkey, tmp, sizeof(tmp), &ks, 0);

               /* Reception d'un '\n', le code est complet. */
               if(ks == XK_Return)
               {
                    printf("CODE: %s\n", tp->buffer);

                    /* script codebare */

                    /* On vide le buffer */
                    memset(tp->buffer, 0, sizeof(tp->buffer));
               }
               else
                    /* Ajout du numero dans le buffer */
                    strncat(tp->buffer, tmp, sizeof(tmp));

          default:
               break;
     }
}

static void
tp_x_loop(void)
{
     XEvent ev;

     for(;;)
     {
          while(!XNextEvent(tp->dpy, &ev))
               tp_x_event(&ev);

     }
}

int
main(int argc, char **argv)
{
     /* Allocation de la variable global tp */
     tp = calloc(1, sizeof(struct termprod));

     /* Ouverture du display X */
     if(!(tp->dpy = XOpenDisplay(NULL)))
     {
          fprintf(stderr, "%s: Can't open X server\n", argv[0]);
          free(tp);
          exit(EXIT_FAILURE); /* fail */
     }

     /* Let's go */
     tp_x_init();
     tp_render();
     tp_x_loop();

     /* Quit */
     XCloseDisplay(tp->dpy);
     free(tp);

     return 1;
}


