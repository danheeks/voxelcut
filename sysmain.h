// This file has been modified from Ken Silverman's original release

/***************************************************************************************************
WINMAIN.CPP & SYSMAIN.H

Windows layer code written by Ken Silverman (http://advsys.net/ken) (1997-2005)
Additional modifications by Tom Dobrowolski (http://ged.ax.pl/~tomkh)
You may use this code for non-commercial purposes as long as credit is maintained.
***************************************************************************************************/


#ifndef KEN_SYSMAIN_H
#define KEN_SYSMAIN_H

	//System specific:
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
extern HWND ghwnd;
extern HDC startdc ();
extern void stopdc ();
extern void ddflip2gdi ();
extern long canrender ();
extern long ddrawuseemulation;
#else
#pragma pack(push,1)
typedef struct { unsigned char peRed, peGreen, peBlue, peFlags; } PALETTEENTRY;
#pragma pack(pop)
#define MAX_PATH 260
#endif

	//Program Flow:
extern long progresiz;
long initapp (long argc, char **argv);
void doframe ();
extern void quitloop ();
void uninitapp ();

	//Video:
typedef struct { long x, y; char c, r0, g0, b0, a0, rn, gn, bn, an; } validmodetype;
extern validmodetype curvidmodeinfo;
extern long xres, yres, colbits, fullscreen, maxpages;
extern PALETTEENTRY pal[256];
extern long startdirectdraw (long *, long *, long *, long *);
extern void stopdirectdraw ();
extern void nextpage ();
extern long clearscreen (long fillcolor);
extern void updatepalette (long start, long danum);
extern long getvalidmodelist (validmodetype **validmodelist);
extern long changeres (long, long, long, long);

#endif
