/*
 * tpsend.c
 *
 * Compilation: cc -lX11 tpsend.c -o tpsend
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#define SCREEN  DefaultScreen(dpy)
#define ROOT    RootWindow(dpy, SCREEN)
#define ATOM(a) XInternAtom(dpy, (a), False)

int
main(int argc, char **argv)
{
     Display *dpy;
     XClientMessageEvent e;

      /* Ouverture du display X */
     if(!(dpy = XOpenDisplay(NULL)))
     {
          fprintf(stderr, " Can't open X server\n", argv[0]);
          exit(EXIT_FAILURE);
     }

     /* Arguments */
     if(argc != 2)
     {
          fprintf(stderr, "usage: tpsend information\n", argv[0]);
          exit(EXIT_FAILURE);
     }

     /* On modifie la xprop */
     XChangeProperty(dpy, ROOT, ATOM("_TERMPROD"), XA_STRING,
                         8, PropModeReplace, (unsigned char*)argv[1], strlen(argv[1]));

     /* On envoie l'event dans le display, le programme principale le recupere */
     e.type         = ClientMessage;
     e.message_type = ATOM("_TERMPROD");
     e.window       = ROOT;
     e.format       = 32;
     e.data.l[0]    = 0;
     e.data.l[1]    = 0;
     e.data.l[2]    = 0;
     e.data.l[3]    = 0;
     e.data.l[4]    = true;

     XSendEvent(dpy, ROOT, false, StructureNotifyMask, (XEvent*)&e);
     XSync(dpy, false);

     return 0;
}

