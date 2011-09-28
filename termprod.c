/*
 * termprod.c
 *
 * Compilation: cc -lX11 -lImlib2 termprod.c -o termprod
 */
/* Standard */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>

/* Xlib */
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* Imlib2 */
#include <Imlib2.h>

/* Options */
#define FONT "-*-fixed-*-*-*-*-20-*"
#define BGCOLOR 0xCCCCCC
#define BGTEXT  0xAAAAAA
#define FGTEXT  0x222222
#define PAD 10

#define LOGOP "logo.png"
#define LOGOW 300
#define LOGOH 130

#define SCRIPT "./tpquery.sh"

#define ERRMSG "Produit inexacte."

/* Structures */
struct termprod
{
     Display *dpy;
     Window root;
     int xscreen, xdepth;
     GC gc;
     Atom atom;
     char buffer[128];

     /* Font */
     struct
     {
          int as, de, width, height;
          XFontSet fontset;
     } font;


};

enum itype { TYPEID, TYPENAME, TYPEPRIX };

struct termprod *tp;

static void
draw_image(int x, int y, int w, int h, char *name)
{
     Imlib_Image image;

     imlib_context_set_display(tp->dpy);
     imlib_context_set_visual(DefaultVisual(tp->dpy, tp->xscreen));
     imlib_context_set_colormap(DefaultColormap(tp->dpy, tp->xscreen));
     imlib_context_set_drawable(tp->root);

     image = imlib_load_image(name);
     imlib_context_set_image(image);

     /*
        imlib_image_get_width();
        imlib_image_get_height();
     */

     imlib_render_image_on_drawable_at_size(x, y, w, h);

     imlib_free_image();
}

static inline void
draw_text(int x, int y, const char *str)
{
     XSetForeground(tp->dpy, tp->gc, FGTEXT);
     XmbDrawString(tp->dpy, tp->root, tp->font.fontset, tp->gc, x, y, str, strlen(str));
}

static inline unsigned short
textw(const char *str)
{
     XRectangle r;

     XmbTextExtents(tp->font.fontset, str, strlen(str), NULL, &r);

     return r.width;
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

     /* Xprop */
     tp->atom = XInternAtom(tp->dpy, "_TERMPROD", false);

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

     tp_render();
}

static char *
tp_fixstr(char *str, enum itype t)
{
     char *ret = malloc(strlen(str) + 1);
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
               for(i = p = 0; i < strlen(str); ++i)
                    if(isdigit(str[i]) || str[i] == ',')
                         ret[p++] = str[i];

               ret[p] = 'e';
               ret[p + 1] = '\0';
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

     tp_render();

     /* Parsage de l'infos */

     /* Code barre */
     if(r)
     {
          id = tp_fixstr(r, TYPEID);

          draw_rect(50, 180, textw(id) + (PAD << 2), 38, BGTEXT);
          draw_text(50 + PAD, 205, id);
     }

     /* Code inexacte */
     if(!strlen(id))
     {

          draw_rect(50, 180, textw(ERRMSG) + (PAD << 2), 38, BGTEXT);
          draw_text(50 + PAD, 205, ERRMSG);

          free(id);

          return;
     }

     /* Nom du produit */
     if((r = strtok(NULL, "\n")))
     {
          name = tp_fixstr(r, TYPENAME);

          draw_rect(50, 280, textw(name) + (PAD << 2), 38, BGTEXT);
          draw_text(50 + PAD, 305, name);
     }

     /* Prix */
     if((r = strtok(NULL, "\n")))
     {
          prix = tp_fixstr(r, TYPEPRIX);

          draw_rect(50, 380, textw(prix) + (PAD << 2), 38, BGTEXT);
          draw_text(50 + PAD, 405, prix);
     }

     printf("-> (%s);(%s);(%s)\n", id, name, prix);


     draw_rect(600, 50, 400, 500, BGTEXT);

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
               if(ev->xclient.message_type == tp->atom)
               {
                    Atom rt;
                    int rf;
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
                    printf("Code: %s\n", tp->buffer);

                    /* script codebare */
                    system(SCRIPT);

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
     tp_init();
     tp_render();
     tp_x_loop();

     /* Quit */
     XCloseDisplay(tp->dpy);
     free(tp);

     return 1;
}


