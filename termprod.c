/*
 * termprod.c
 *
 * Compilation: cc -lX11 -lImlib2 -lfreetype -lXft -I/usr/include/freetype2 termprod.c -o termprod
 */

/* Standard */
#define _GNU_SOURCE /* strcasecmp() */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include <unistd.h>

/* Xlib */
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* Imlib2 */
#include <Imlib2.h>

/* Xft */
#include <X11/Xft/Xft.h>

/* Options */
#define FONT     "mono-20"
#define FONTNAME "mono-22"
#define FONTPRIX "mono-60"
#define BGCOLOR   0xCCCCCC
#define BGTEXT    0xAAAAAA
#define BORDPRIX  0xFDF100
#define FGTEXT   "#222222"
#define PAD 3

#define LOGOP  "logo.png"
#define DEFIMG "noimg.jpg"
#define LOGOW 370
#define LOGOH 215

#define SCRIPT "./tpquery.sh"
#define SCRIPTIMG "./tpimg.sh"

#define ERRMSG  "Produit non disponible."
#define ERRNAME "Code Inex !"

/* Structures */
struct termprod
{
     Display *dpy;
     Window root;
     int xscreen, xdepth;
     GC gc;
     Atom atom, atomimg;
     char buffer[128];

     XftFont *font;
     XftFont *fontname;
     XftFont *fontprix;
};

enum itype { TYPEID, TYPENAME, TYPEPRIX };

struct termprod *tp;

static bool
draw_image(int x, int y, int w, int h, char *name)
{
     bool ret = true;
     Imlib_Image image;

     imlib_context_set_display(tp->dpy);
     imlib_context_set_visual(DefaultVisual(tp->dpy, tp->xscreen));
     imlib_context_set_colormap(DefaultColormap(tp->dpy, tp->xscreen));
     imlib_context_set_drawable(tp->root);

     if(!(image = imlib_load_image(name)))
     {
          image = imlib_load_image(DEFIMG);
          ret = false;
     }

     imlib_context_set_image(image);

     /*
        imlib_image_get_width();
        imlib_image_get_height();
     */

     imlib_render_image_on_drawable_at_size(x, y, w, h);

     imlib_free_image();

     return ret;
}

static inline void
draw_text(XftFont *f, int x, int y, const char *str)
{
     XftColor xftcolor;
     XftDraw *xftd;

     /* Transform X Drawable -> Xft Drawable */
     xftd = XftDrawCreate(tp->dpy, tp->root, DefaultVisual(tp->dpy, tp->xscreen), DefaultColormap(tp->dpy, tp->xscreen));

     /* Alloc text color */
     XftColorAllocName(tp->dpy, DefaultVisual(tp->dpy, tp->xscreen),
               DefaultColormap(tp->dpy, tp->xscreen), FGTEXT, &xftcolor);

     XftDrawStringUtf8(xftd, &xftcolor, f, x, y, (FcChar8 *)str, strlen(str));

     /* Free the text color and XftDraw */
     XftColorFree(tp->dpy, DefaultVisual(tp->dpy, tp->xscreen), DefaultColormap(tp->dpy, tp->xscreen), &xftcolor);

     XftDrawDestroy(xftd);
}

static inline unsigned short
textw(XftFont *f, const char *str)
{
     XGlyphInfo gl;

     XftTextExtentsUtf8(tp->dpy, f, (FcChar8 *)str, strlen(str), &gl);

     return gl.width + f->descent;
}

static inline void
draw_rect(int x, int y, int w, int h, unsigned int bg)
{
     XSetForeground(tp->dpy, tp->gc, bg);
     XFillRectangle(tp->dpy, tp->root, tp->gc, x, y, w, h);
}

static void
tp_render(void)
{
     XClearWindow(tp->dpy, tp->root);
     XSetWindowBackground(tp->dpy, tp->root, BGCOLOR);

     draw_image(0, 0, LOGOW, LOGOH, LOGOP);
}

static void
tp_init(void)
{
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

     XClearWindow(tp->dpy, tp->root);
     XChangeWindowAttributes(tp->dpy, tp->root, CWEventMask | CWCursor, &at);

     /* Xprop */
     tp->atom = XInternAtom(tp->dpy, "_TERMPROD", false);
     tp->atomimg = XInternAtom(tp->dpy, "_TERMPROD_IMG", false);

     tp->font     = XftFontOpenName(tp->dpy, tp->xscreen, FONT);
     tp->fontname = XftFontOpenName(tp->dpy, tp->xscreen, FONTNAME);
     tp->fontprix = XftFontOpenName(tp->dpy, tp->xscreen, FONTPRIX);

     memset(tp->buffer, 0, sizeof(tp->buffer));

     tp_render();
}

static char *
tp_fixstr(char *str, enum itype t)
{
     char *ret = calloc(strlen(str) + 8, sizeof(char));
     int i, p;

     memset(ret, 0, sizeof(ret));

     switch(t)
     {
          case TYPEID:
               for(i = p = 0; i < strlen(str); ++i)
                    if(isdigit(str[i]))
                         ret[p++] = str[i];
               break;

          case TYPENAME:
               for(i = p = 0; i < strlen(str); ++i)
                    if(isprint(str[i]) && isascii(str[i]))
                         ret[p++] = str[i];
               break;

          case TYPEPRIX:
               for(i = 2, p = 0; i < strlen(str); ++i)
                    if(isdigit(str[i]) || str[i] == ' ' || str[i] == ',')
                         ret[p++] = str[i];
               strcat(ret, "€");
               break;
     }

     return ret;
}

static void
tp_draw_infos(char *infos)
{
     char *id = NULL;
     char *name = NULL;
     char *prix = NULL;
     char *r = strtok(infos, "\n");
     char imgp[64] = { 0 };
     int x = 5, y;

     tp_render();

     /*
      * Code barre
      */
     if(r)
     {
          y = 250;
          id = tp_fixstr(r, TYPEID);

          draw_rect(x, y, textw(tp->font, id) + (PAD << 2), 40, BGTEXT);
          draw_text(tp->font, x + PAD, y + 31, id);
     }

     /*
      * Nom du produit
      */
     if((r = strtok(NULL, "\n")))
     {
          y = 350;
          name = tp_fixstr(r, TYPENAME);

          draw_rect(x, y, textw(tp->fontname, name) + (PAD << 2), 40, BGTEXT);
          draw_text(tp->fontname, x + PAD, y + 31, name);
     }

     /*
      * Prix
      */
     if((r = strtok(NULL, "\n")))
     {
          y = 467;
          prix = tp_fixstr(r, TYPEPRIX);

          draw_rect(x, y, textw(tp->fontprix, prix) + (PAD << 2) + 2, 84, BORDPRIX);
          draw_rect(x + 2, y + 2, textw(tp->fontprix, prix) + (PAD << 2) - 2, 80, BGTEXT);
          draw_text(tp->fontprix, (x - 2) + PAD, y + 69, prix);
     }

     /*
      * Image
      */
     sprintf(imgp, "cache/m%s.jpg", id);
     draw_image(575, 50, 575 - 1020, 500, imgp);


     /*
      * Code inexacte
      */
     if(!id || !strlen(id)              /* pas d'ID */
         || !strcasecmp(name, ERRNAME)) /* "Code Inex !" dans le nom */
     {
          y = 250;
          draw_rect(x, y, textw(tp->font, ERRMSG) + (PAD << 2), 40, BGTEXT);
          draw_text(tp->font, x + PAD, y + 31, ERRMSG);

          free(id);

          return;
     }

     /* DEBUG */
     printf("(%s) (%s) (%s)\n", id, name, prix);

     /* Desallocation */
     free(id);
     free(name);
     free(prix);
}

static void
tp_x_event(XEvent *ev)
{
     char tmp[32] = { 0 };

     KeySym ks;

     switch(ev->type)
     {
          /* Reception des changement au niveau de Xprops */
          case ClientMessage:
               /* oh, un atom _TERMPROD, c'est surement pour nous! */
               if(ev->xclient.message_type == tp->atom)
               {
                    Atom rt;
                    int rf;
                    char cmd[128] = { 0 };
                    unsigned long ir, il;
                    unsigned char *ret = NULL;

                    if(XGetWindowProperty(ev->xany.display, tp->root, tp->atom, 0, 4096,
                                   false, XA_STRING, &rt, &rf, &ir, &il, &ret) == Success)
                    {
                         tp_draw_infos((char*)ret);
                         XFree(ret);
                    }
               }
               break;

          /* Reception des events clavier (boitié code barre) */
          case KeyPress:
               XLookupString(&ev->xkey, tmp, sizeof(tmp), &ks, 0);

               /* Reception d'un '\n', le code est complet. */
               if(ks == XK_Return)
               {
                    char cmd[128] = { 0 };

                    printf("Code: '%s'\n", tp->buffer);

                    sprintf(cmd, SCRIPT" %s &", tp->buffer);
                    system(cmd);

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
          while(XPending(tp->dpy))
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
     tp_init();
     tp_render();
     tp_x_loop();

     /* Quit */
     XCloseDisplay(tp->dpy);
     free(tp);

     return 1;
}


