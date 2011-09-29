#!/bin/sh
echo "Compiling..."
cc -lX11 -lImlib2 -lfreetype -lXft -I/usr/include/freetype2 termprod.c -o termprod
echo "cc -lX11 -lImlib2 -lfreetype -lXft -I/usr/include/freetype2 termprod.c -o termprod"
cc -lX11 tpsend.c -o tpsend
echo "cc -lX11 tpsend.c -o tpsend"
