// This file has been modified from Ken Silverman's original release

//#define NOSOUND

/***************************************************************************************************
WINMAIN.CPP & SYSMAIN.H

Windows layer code written by Ken Silverman (http://advsys.net/ken) (1997-2005)
Additional modifications by Tom Dobrowolski (http://ged.ax.pl/~tomkh)
You may use this code for non-commercial purposes as long as credit is maintained.
***************************************************************************************************/

	//To compile, link with ddraw.lib, dinput.lib AND dxguid.lib.  Be sure ddraw.h and dinput.h
	//are in include path

//#include "StdAfx.h"
#include "msvc.h"
//#include "Engine/Minidump.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef NODRAW
#define DIRECTDRAW_VERSION 0x0700
#include "ddraw.h"
#endif

#pragma warning(disable:4730)
#pragma warning(disable:4731)

extern long initapp (long argc, char **argv);
extern void initapp2();
extern void uninitapp ();
extern void doframe ();

//Global window variables
HWND ghwnd;

long xres = 1680, yres = 1050, colbits = 8, fullscreen = 0, maxpages = 8;

	// Note! Without clipper blit is faster but only with software bliter
//#define NO_CLIPPER

//======================== CPU detection code begins ========================

const char* Ttc(const wchar_t* str);
const wchar_t* Ctt(const char* str);

static long cputype = 0;
static OSVERSIONINFO osvi;

#ifdef __WATCOMC__

long testflag (long);
#pragma aux testflag =\
	"pushfd"\
	"pop eax"\
	"mov ebx, eax"\
	"xor eax, ecx"\
	"push eax"\
	"popfd"\
	"pushfd"\
	"pop eax"\
	"xor eax, ebx"\
	"mov eax, 1"\
	"jne menostinx"\
	"xor eax, eax"\
	"menostinx:"\
	parm nomemory [ecx]\
	modify exact [eax ebx]\
	value [eax]

void cpuid (long, long *);
#pragma aux cpuid =\
	".586"\
	"cpuid"\
	"mov dword ptr [esi], eax"\
	"mov dword ptr [esi+4], ebx"\
	"mov dword ptr [esi+8], ecx"\
	"mov dword ptr [esi+12], edx"\
	parm [eax][esi]\
	modify exact [eax ebx ecx edx]\
	value

#endif
#ifdef _MSC_VER

#pragma warning(disable:4799) //I know how to use EMMS

static _inline long testflag (long c)
{
	_asm
	{
		mov ecx, c
		pushfd
		pop eax
		mov edx, eax
		xor eax, ecx
		push eax
		popfd
		pushfd
		pop eax
		xor eax, edx
		mov eax, 1
		jne menostinx
		xor eax, eax
		menostinx:
	}
}

static _inline void cpuid (long a, long *s)
{
	_asm
	{
		push ebx
		push esi
		mov eax, a
		cpuid
		mov esi, s
		mov dword ptr [esi+0], eax
		mov dword ptr [esi+4], ebx
		mov dword ptr [esi+8], ecx
		mov dword ptr [esi+12], edx
		pop esi
		pop ebx
	}
}

#endif

	//Bit numbers of return value:
	//0:FPU, 4:RDTSC, 15:CMOV, 22:MMX+, 23:MMX, 25:SSE, 26:SSE2, 30:3DNow!+, 31:3DNow!
static long getcputype ()
{
	long i, cpb[4], cpid[4];
	if (!testflag(0x200000)) return(0);
	cpuid(0,cpid); if (!cpid[0]) return(0);
	cpuid(1,cpb); i = (cpb[3]&~((1<<22)|(1<<30)|(1<<31)));
	cpuid(0x80000000,cpb);
	if (((unsigned long)cpb[0]) > 0x80000000)
	{
		cpuid(0x80000001,cpb);
		i |= (cpb[3]&(1<<31));
		if (!((cpid[1]^0x68747541)|(cpid[3]^0x69746e65)|(cpid[2]^0x444d4163))) //AuthenticAMD
			i |= (cpb[3]&((1<<22)|(1<<30)));
	}
	if (i&(1<<25)) i |= (1<<22); //SSE implies MMX+ support
	return(i);
}

//========================= CPU detection code ends =========================

//================== Fast & accurate TIMER FUNCTIONS begins ==================

#if 0
#ifdef __WATCOMC__

__int64 rdtsc64 ();
#pragma aux rdtsc64 = "rdtsc" value [edx eax] modify nomemory parm nomemory;

#endif
#ifdef _MSC_VER

static __forceinline __int64 rdtsc64 () { _asm rdtsc }

#endif

static __int64 pertimbase, rdtimbase, nextimstep;
static double perfrq, klockmul, klockadd;

void initklock ()
{
	__int64 q;
	QueryPerformanceFrequency((LARGE_INTEGER *)&q);
	perfrq = (double)q;
	rdtimbase = rdtsc64();
	QueryPerformanceCounter((LARGE_INTEGER *)&pertimbase);
	nextimstep = 4194304; klockmul = 0.000000001; klockadd = 0.0;
}

void readklock (double *tim)
{
	__int64 q = rdtsc64()-rdtimbase;
	if (q > nextimstep)
	{
		__int64 p;
		double d;
		QueryPerformanceCounter((LARGE_INTEGER *)&p);
		d = klockmul; klockmul = ((double)(p-pertimbase))/(((double)q)*perfrq);
		klockadd += (d-klockmul)*((double)q);
		do { nextimstep <<= 1; } while (q > nextimstep);
	}
	(*tim) = ((double)q)*klockmul + klockadd;
}
#else

#endif

//=================== Fast & accurate TIMER FUNCTIONS ends ===================

//DirectDraw VARIABLES & CODE---------------------------------------------------------------
#ifndef NODRAW

PALETTEENTRY pal[256];
long ddrawuseemulation = 0;
long ddrawdebugmode = -1;    // -1 = off, old ddrawuseemulation = on

static __int64 mask8a = 0x00ff00ff00ff00ff;
static __int64 mask8b; //0x10000d0010000d00 (fullscreen), 0x10000c0010000c00 (windowed)
static __int64 mask8c = 0x0000f8000000f800;
static __int64 mask8d = 0xff000000ff000000;
static __int64 mask8e = 0x00ff000000ff0000;
static __int64 mask8f; //0x00d000d000d000d0 (fullscreen), 0x0aca0aca0aca0aca (windowed)
static __int64 mask8g; //0xd000d000d000d000 (fullscreen), 0xca0aca0aca0aca0a (windowed)
static long mask8cnt = 0;
static __int64 mask8h = 0x0007050500070505;
static __int64 mask8lut[8] =
{
	0x0000000000000000,0x0000001300000013,
	0x0000080000000800,0x0000081300000813,
	0x0010000000100000,0x0010001300100013,
	0x0010080000100800,0x0010081300100813
};
static __int64 maskrb15 = 0x00f800f800f800f8;
static __int64 maskgg15 = 0x0000f8000000f800;
static __int64 maskml15 = 0x2000000820000008;
static __int64 maskrb16 = 0x00f800f800f800f8;
static __int64 maskgg16 = 0x0000fc000000fc00;
static __int64 maskml16 = 0x2000000420000004;
static __int64 mask16a  = 0x0000800000008000;
static __int64 mask16b  = 0x8000800080008000;
static __int64 mask24a  = 0x00ffffff00000000;
static __int64 mask24b  = 0x00000000ffffff00;
static __int64 mask24c  = 0x0000000000ffffff;
static __int64 mask24d  = 0xffffff0000000000;

static long lpal[256];

	//Beware of alignment issues!!!
void kblit32 (long rplc, long rbpl, long rxsiz, long rysiz,
				  long wplc, long cdim, long wbpl)
{
#ifdef _MSC_VER
	if ((rxsiz <= 0) || (rysiz <= 0)) return;
	if (colbits == 8)
	{
		long x, y;
		switch(cdim)
		{
			case 15: case 16:
					//for(y=0;y<rysiz;y++)
					//   for(x=0;x<rxsiz;x++)
					//      *(short *)(wplc+y*wbpl+x*2) =
					//      lpal[*(unsigned char *)(rplc+y*rbpl+x)];
				if (cputype&(1<<22)) //MMX+
				{
					_asm
					{
						push ebx
						push esi
						push edi
						mov esi, rplc
						mov edi, wplc
begall8_16:       mov edx, edi
						mov eax, esi
						add esi, rbpl
						add edi, wbpl
						mov ecx, rxsiz
						test edx, 6
						jz short start_8_16
pre_8_16:         movzx ebx, byte ptr [eax]
						mov bx, word ptr lpal[ebx*4]
						mov [edx], bx
						sub ecx, 1
						jz short endall8_16
						add eax, 1
						add edx, 2
						test edx, 6
						jnz short pre_8_16
start_8_16:       sub ecx, 3
						add eax, ecx
						lea edx, [edx+ecx*2]
						neg ecx
						jge short skip8_16
						xor ebx, ebx
beg8_16:          mov bl, byte ptr [eax+ecx]
						movd mm0, lpal[ebx*4]
						mov bl, byte ptr [eax+ecx+1]
						movd mm1, lpal[ebx*4]
						punpcklwd mm0, mm1
						mov bl, byte ptr [eax+ecx+2]
						movd mm1, lpal[ebx*4]
						mov bl, byte ptr [eax+ecx+3]
						movd mm2, lpal[ebx*4]
						punpcklwd mm1, mm2
						punpckldq mm0, mm1
						movntq [edx+ecx*2], mm0
						add ecx, 4
						jl short beg8_16
skip8_16:         sub ecx, 3
						jz short endall8_16
end8_16:          movzx ebx, byte ptr [eax+ecx+3]
						mov bx, word ptr lpal[ebx*4]
						mov [edx+ecx*2+6], bx
						add ecx, 1
						jnz short end8_16
endall8_16:       sub rysiz, 1 ;/
						jnz short begall8_16
						pop edi
						pop esi
						pop ebx
						emms
					}
				}
				else
				{
					_asm
					{
						push ebx
						push esi
						push edi
						mov esi, rplc
						mov edi, wplc
begall8_16b:      mov edx, edi
						mov eax, esi
						add esi, rbpl
						add edi, wbpl
						mov ecx, rxsiz
						test edx, 6
						jz short start_8_16b
pre_8_16b:        movzx ebx, byte ptr [eax]
						mov bx, word ptr lpal[ebx*4]
						mov [edx], bx
						sub ecx, 1
						jz short endall8_16b
						add eax, 1
						add edx, 2
						test edx, 6
						jnz short pre_8_16b
start_8_16b:      sub ecx, 3
						add eax, ecx
						lea edx, [edx+ecx*2]
						neg ecx
						jge short skip8_16b
						xor ebx, ebx
beg8_16b:         mov bl, byte ptr [eax+ecx]
						movd mm0, lpal[ebx*4]
						mov bl, byte ptr [eax+ecx+1]
						movd mm1, lpal[ebx*4]
						punpcklwd mm0, mm1
						mov bl, byte ptr [eax+ecx+2]
						movd mm1, lpal[ebx*4]
						mov bl, byte ptr [eax+ecx+3]
						movd mm2, lpal[ebx*4]
						punpcklwd mm1, mm2
						punpckldq mm0, mm1
						movq [edx+ecx*2], mm0
						add ecx, 4
						jl short beg8_16b
skip8_16b:        sub ecx, 3
						jz short endall8_16b
end8_16b:         movzx ebx, byte ptr [eax+ecx+3]
						mov bx, word ptr lpal[ebx*4]
						mov [edx+ecx*2+6], bx
						add ecx, 1
						jnz short end8_16b
endall8_16b:      sub rysiz, 1 ;/
						jnz short begall8_16b
						pop edi
						pop esi
						pop ebx
						emms
					}
				}
				break;
			case 24:
					//Should work, but slow&ugly! :/
				for(y=0;y<rysiz;y++)
					for(x=0;x<rxsiz;x++)
						*(long *)(wplc+y*wbpl+x*3) = lpal[*(unsigned char *)(rplc+y*rbpl+x)];
				break;
			case 32:
					//for(y=0;y<rysiz;y++)
					//   for(x=0;x<rxsiz;x++)
					//      *(long *)(wplc+y*wbpl+(x<<2)) = lpal[*(unsigned char *)(rplc+y*rbpl+x)];
				if (cputype&(1<<22)) //MMX+
				{
					_asm
					{
						push ebx
						push esi
						push edi
						mov esi, rplc
						mov edi, wplc
begall8_32:       mov edx, edi
						mov eax, esi
						add esi, rbpl
						add edi, wbpl
						mov ecx, rxsiz
						xor ebx, ebx
						test edx, 4
						jz short start_8_32
						mov bl, byte ptr [eax]
						movd mm0, lpal[ebx*4]
						movd [edx], mm0
						sub ecx, 1
						jz short endall8_32
						add eax, 1
						add edx, 4
start_8_32:       sub ecx, 1
						add eax, ecx
						lea edx, [edx+ecx*4]
						neg ecx
						jge short skip8_32
beg8_32:          mov bl, byte ptr [eax+ecx]
						movd mm0, lpal[ebx*4]
						mov bl, byte ptr [eax+ecx+1]
						punpckldq mm0, lpal[ebx*4]
						movntq [edx+ecx*4], mm0
						add ecx, 2
						jl short beg8_32
skip8_32:         cmp ecx, 1
						jz short endall8_32
						movzx ebx, byte ptr [eax]
						movd mm0, lpal[ebx*4]
						movd [edx], mm0
endall8_32:       sub rysiz, 1 ;/
						jnz short begall8_32
						pop edi
						pop esi
						pop ebx
						emms
					}
				}
				else
				{
					_asm
					{
						push ebx
						push esi
						push edi
						mov esi, rplc
						mov edi, wplc
begall8_32b:      mov edx, edi
						mov eax, esi
						add esi, rbpl
						add edi, wbpl
						mov ecx, rxsiz
						xor ebx, ebx
						test edx, 4
						jz short start_8_32b
						mov bl, byte ptr [eax]
						movd mm0, lpal[ebx*4]
						movd [edx], mm0
						sub ecx, 1
						jz short endall8_32b
						add eax, 1
						add edx, 4
start_8_32b:      sub ecx, 1
						add eax, ecx
						lea edx, [edx+ecx*4]
						neg ecx
						jge short skip8_32b
beg8_32b:         mov bl, byte ptr [eax+ecx]
						movd mm0, lpal[ebx*4]
						mov bl, byte ptr [eax+ecx+1]
						punpckldq mm0, lpal[ebx*4]
						movq [edx+ecx*4], mm0
						add ecx, 2
						jl short beg8_32b
skip8_32b:        cmp ecx, 1
						jz short endall8_32b
						movzx ebx, byte ptr [eax]
						movd mm0, lpal[ebx*4]
						movd [edx], mm0
endall8_32b:      sub rysiz, 1 ;/
						jnz short begall8_32b
						pop edi
						pop esi
						pop ebx
						emms
					}
				}
				break;
		}
	}
	else if (colbits == 32)
	{
		switch (cdim)
		{
			case 8:
				rxsiz &= ~7; if (rxsiz <= 0) return;
				if (cputype&(1<<22)) //MMX+
				{
					_asm
					{
						push ebx
						push esi

						movq mm7, mask8a
						movq mm6, mask8b
						movq mm4, mask8d

						mov eax, rplc
						mov edx, wplc
						mov ecx, rxsiz
						mov ebx, rysiz
						shr ecx, 1
						lea eax, [eax+ecx*8]
						lea edx, [edx+ecx*2-8]
						neg ecx
						mov rxsiz, ecx

						mov ecx, mask8cnt
						xor ecx, 1
						mov mask8cnt, ecx
						add ecx, ebx
						test ecx, 1
						je into8b

			 begall8:psubd mm4, mask8h
						pmovmskb esi, mm4
						and esi, 7
						paddd mm4, mask8lut[esi*8]

						mov ecx, rxsiz
						movq mm5, mask8c
				beg8a:
							;src: AaaaaaaaRrrrrrrrGgggggggBbbbbbbbAaaaaaaaRrrrrrrrGgggggggBbbbbbbb
							;dst: RrrGggBbRrrGggBb
						movq mm3, [eax+ecx*8]   ;mm0: A1 R1 G1 B1 A0 R0 G0 B0
						movq mm1, [eax+ecx*8+8] ;mm1: A3 R3 G3 B3 A2 R2 G2 B2
						psubusb mm3, mm4
						psubusb mm1, mm4
						movq mm2, mm3      ;mm2: 00 R1 G1 B1 00 R0 G0 B0
						punpckldq mm3, mm1 ;mm3: 00 R2 G2 B2 A0 R0 G0 B0
						pand mm3, mm7      ;mm3: 00 R2 00 B2 00 R0 00 B0
						pmulhw mm3, mm6    ;mm3: 00 R2 00 B2 00 R0 00 B0 ;16<<8 13<<8 16<<8 13<<8
						punpckhdq mm2, mm1 ;mm2: 00 R3 G3 B3 00 R1 G1 B1
						pand mm2, mm5      ;mm2: 00 00 G3 00 00 00 G1 00
						pslld mm2, 5       ;mm2: 00 G3 00 00 00 G1 00 00
						movq mm1, mm3      ;mm1: 00 R2 00 B2 00 R0 00 B0
						pslld mm1, 20      ;mm1: 00 B2 00 00 00 B0 00 00
						paddd mm3, mm1     ;mm3: 00 C2 00 B2 00 C0 00 B0
						psrld mm3, 16      ;mm3: 00 00 00 C2 00 00 00 C0 ;(B2<<4)+R2  (B0<<4)+R0
						paddd mm3, mm2     ;mm3: 00 G3 00 C2 00 G1 00 C0

						movq mm0, [eax+ecx*8+16] ;mm0: A5 R5 G5 B5 A4 R4 G4 B4
						movq mm1, [eax+ecx*8+24] ;mm1: A7 R7 G7 B7 A6 R6 G6 B6
						psubusb mm0, mm4
						psubusb mm1, mm4
						movq mm2, mm0      ;mm2: 00 R5 G5 B5 00 R4 G4 B4
						punpckldq mm0, mm1 ;mm0: 00 R6 G6 B6 A4 R4 G4 B4
						pand mm0, mm7      ;mm0: 00 R6 00 B6 00 R4 00 B4
						pmulhw mm0, mm6    ;mm0: 00 R6 00 B6 00 R4 00 B4 ;16<<8 13<<8 16<<8 13<<8
						prefetchnta [eax+ecx*8+120]
						punpckhdq mm2, mm1 ;mm2: 00 R7 G7 B7 00 R5 G5 B5
						add ecx, 4
						pand mm2, mm5      ;mm2: 00 00 G7 00 00 00 G5 00
						pslld mm2, 5       ;mm2: 00 G7 00 00 00 G5 00 00
						movq mm1, mm0      ;mm1: 00 R6 00 B6 00 R4 00 B4
						pslld mm1, 20      ;mm1: 00 B6 00 00 00 B4 00 00
						paddd mm0, mm1     ;mm0: 00 C6 00 B6 00 C4 00 B4
						psrld mm0, 16      ;mm0: 00 00 00 C6 00 00 00 C4 ;(B6<<4)+R6  (B4<<4)+R4
						paddd mm0, mm2     ;mm0: 00 G7 00 C6 00 G5 00 C4

						packuswb mm3, mm0  ;mm3: G7 C6 G5 C4 G3 C2 G1 C0
						paddd mm3, mask8g  ;mm3: C7 C6 C5 C4 C3 C2 C1 C0
						movntq [edx+ecx*2], mm3
						js short beg8a

						add eax, rbpl
						add edx, wbpl
						sub ebx, 1
						jz endit8

			  into8b:psubd mm4, mask8h
						pmovmskb esi, mm4
						and esi, 7
						paddd mm4, mask8lut[esi*8]

						mov ecx, rxsiz
						movq mm5, mask8e
				beg8b:
							;src: AaaaaaaaRrrrrrrrGgggggggBbbbbbbbAaaaaaaaRrrrrrrrGgggggggBbbbbbbb
							;dst: RrrGggBbRrrGggBb
						movq mm3, [eax+ecx*8]   ;mm3: A1 R1 G1 B1 A0 R0 G0 B0
						movq mm1, [eax+ecx*8+8] ;mm1: A3 R3 G3 B3 A2 R2 G2 B2
						psubusb mm3, mm4
						psubusb mm1, mm4
						movq mm2, mm3      ;mm2: 00 R1 G1 B1 00 R0 G0 B0
						punpckhdq mm3, mm1 ;mm3: 00 R3 G3 B3 00 R1 G1 B1
						pand mm3, mm7      ;mm3: 00 R3 00 B3 00 R1 00 B1
						pmulhw mm3, mm6    ;mm3: 00 R3 00 B3 00 R1 00 B1 ;16<<8 13<<8 16<<8 13<<8
						punpckldq mm2, mm1 ;mm2: 00 R2 G2 B2 00 R0 G0 B0
						psrlw mm2, 11      ;mm2: 00 00 00 G2 00 00 00 G0
						movq mm1, mm3      ;mm1: 00 R3 00 B3 00 R1 00 B1
						pslld mm1, 20      ;mm1: 00 B3 00 00 00 B1 00 00
						paddd mm3, mm1     ;mm3: 00 C3 00 B3 00 C1 00 B1
						pand mm3, mm5      ;mm3: 00 C3 00 00 00 C1 00 00 ;(B3<<4)+R3  (B1<<4)+R1
						paddd mm3, mm2     ;mm3: 00 C3 00 G2 00 C1 00 G0

						movq mm0, [eax+ecx*8+16] ;mm0: A5 R5 G5 B5 A4 R4 G4 B4
						movq mm1, [eax+ecx*8+24] ;mm1: A7 R7 G7 B7 A6 R6 G6 B6
						psubusb mm0, mm4
						psubusb mm1, mm4
						movq mm2, mm0      ;mm2: 00 R5 G5 B5 00 R4 G4 B4
						punpckhdq mm0, mm1 ;mm0: 00 R7 G7 B7 00 R5 G5 B5
						pand mm0, mm7      ;mm0: 00 R7 00 B7 00 R5 00 B5
						pmulhw mm0, mm6    ;mm0: 00 R7 00 B7 00 R5 00 B5 ;16<<8 13<<8 16<<8 13<<8
						prefetchnta [eax+ecx*8+120]
						punpckldq mm2, mm1 ;mm2: 00 R6 G6 B6 00 R4 G4 B4
						add ecx, 4
						psrlw mm2, 11      ;mm2: 00 00 00 G6 00 00 00 G4
						movq mm1, mm0      ;mm1: 00 R7 00 B7 00 R5 00 B5
						pslld mm1, 20      ;mm1: 00 B7 00 00 00 B5 00 00
						paddd mm0, mm1     ;mm0: 00 C7 00 B7 00 C5 00 B5
						pand mm0, mm5      ;mm0: 00 C7 00 00 00 C5 00 00 ;(B7<<4)+R7  (B5<<4)+R5
						paddd mm0, mm2     ;mm0: 00 C7 00 G6 00 C5 00 G4

						packuswb mm3, mm0  ;mm3: C7 G6 C5 G4 C3 G2 C1 G0
						paddd mm3, mask8f  ;mm3: C7 C6 C5 C4 C3 C2 C1 C0
						movntq [edx+ecx*2], mm3
						js short beg8b

						add eax, rbpl
						add edx, wbpl
						sub ebx, 1
						jnz begall8

			  endit8:psubd mm4, mask8h
						pmovmskb esi, mm4
						and esi, 7
						paddd mm4, mask8lut[esi*8]
						movq mask8d, mm4

						pop esi
						pop ebx
						emms
					}
				}
				else
				{
					_asm
					{
						push ebx
						push esi

						movq mm7, mask8a
						movq mm6, mask8b
						movq mm4, mask8d

						mov eax, rplc
						mov edx, wplc
						mov ecx, rxsiz
						mov ebx, rysiz
						shr ecx, 1
						lea eax, [eax+ecx*8]
						lea edx, [edx+ecx*2-8]
						neg ecx
						mov rxsiz, ecx

						mov ecx, mask8cnt
						xor ecx, 1
						mov mask8cnt, ecx
						add ecx, ebx
						test ecx, 1
						je into8bb

			begall8b:psubd mm4, mask8h
						pmovmskb esi, mm4
						and esi, 7
						paddd mm4, mask8lut[esi*8]

						mov ecx, rxsiz
						movq mm5, mask8c
			  beg8ab:
							;src: AaaaaaaaRrrrrrrrGgggggggBbbbbbbbAaaaaaaaRrrrrrrrGgggggggBbbbbbbb
							;dst: RrrGggBbRrrGggBb
						movq mm3, [eax+ecx*8]   ;mm0: A1 R1 G1 B1 A0 R0 G0 B0
						movq mm1, [eax+ecx*8+8] ;mm1: A3 R3 G3 B3 A2 R2 G2 B2
						psubusb mm3, mm4
						psubusb mm1, mm4
						movq mm2, mm3      ;mm2: 00 R1 G1 B1 00 R0 G0 B0
						punpckldq mm3, mm1 ;mm3: 00 R2 G2 B2 A0 R0 G0 B0
						pand mm3, mm7      ;mm3: 00 R2 00 B2 00 R0 00 B0
						pmulhw mm3, mm6    ;mm3: 00 R2 00 B2 00 R0 00 B0 ;16<<8 13<<8 16<<8 13<<8
						punpckhdq mm2, mm1 ;mm2: 00 R3 G3 B3 00 R1 G1 B1
						pand mm2, mm5      ;mm2: 00 00 G3 00 00 00 G1 00
						pslld mm2, 5       ;mm2: 00 G3 00 00 00 G1 00 00
						movq mm1, mm3      ;mm1: 00 R2 00 B2 00 R0 00 B0
						pslld mm1, 20      ;mm1: 00 B2 00 00 00 B0 00 00
						paddd mm3, mm1     ;mm3: 00 C2 00 B2 00 C0 00 B0
						psrld mm3, 16      ;mm3: 00 00 00 C2 00 00 00 C0 ;(B2<<4)+R2  (B0<<4)+R0
						paddd mm3, mm2     ;mm3: 00 G3 00 C2 00 G1 00 C0

						movq mm0, [eax+ecx*8+16] ;mm0: A5 R5 G5 B5 A4 R4 G4 B4
						movq mm1, [eax+ecx*8+24] ;mm1: A7 R7 G7 B7 A6 R6 G6 B6
						psubusb mm0, mm4
						psubusb mm1, mm4
						movq mm2, mm0      ;mm2: 00 R5 G5 B5 00 R4 G4 B4
						punpckldq mm0, mm1 ;mm0: 00 R6 G6 B6 A4 R4 G4 B4
						pand mm0, mm7      ;mm0: 00 R6 00 B6 00 R4 00 B4
						pmulhw mm0, mm6    ;mm0: 00 R6 00 B6 00 R4 00 B4 ;16<<8 13<<8 16<<8 13<<8
						punpckhdq mm2, mm1 ;mm2: 00 R7 G7 B7 00 R5 G5 B5
						add ecx, 4
						pand mm2, mm5      ;mm2: 00 00 G7 00 00 00 G5 00
						pslld mm2, 5       ;mm2: 00 G7 00 00 00 G5 00 00
						movq mm1, mm0      ;mm1: 00 R6 00 B6 00 R4 00 B4
						pslld mm1, 20      ;mm1: 00 B6 00 00 00 B4 00 00
						paddd mm0, mm1     ;mm0: 00 C6 00 B6 00 C4 00 B4
						psrld mm0, 16      ;mm0: 00 00 00 C6 00 00 00 C4 ;(B6<<4)+R6  (B4<<4)+R4
						paddd mm0, mm2     ;mm0: 00 G7 00 C6 00 G5 00 C4

						packuswb mm3, mm0  ;mm3: G7 C6 G5 C4 G3 C2 G1 C0
						paddd mm3, mask8g  ;mm3: C7 C6 C5 C4 C3 C2 C1 C0
						movq [edx+ecx*2], mm3
						js short beg8ab

						add eax, rbpl
						add edx, wbpl
						sub ebx, 1
						jz endit8b

			 into8bb:psubd mm4, mask8h
						pmovmskb esi, mm4
						and esi, 7
						paddd mm4, mask8lut[esi*8]

						mov ecx, rxsiz
						movq mm5, mask8e
			  beg8bb:
							;src: AaaaaaaaRrrrrrrrGgggggggBbbbbbbbAaaaaaaaRrrrrrrrGgggggggBbbbbbbb
							;dst: RrrGggBbRrrGggBb
						movq mm3, [eax+ecx*8]   ;mm3: A1 R1 G1 B1 A0 R0 G0 B0
						movq mm1, [eax+ecx*8+8] ;mm1: A3 R3 G3 B3 A2 R2 G2 B2
						psubusb mm3, mm4
						psubusb mm1, mm4
						movq mm2, mm3      ;mm2: 00 R1 G1 B1 00 R0 G0 B0
						punpckhdq mm3, mm1 ;mm3: 00 R3 G3 B3 00 R1 G1 B1
						pand mm3, mm7      ;mm3: 00 R3 00 B3 00 R1 00 B1
						pmulhw mm3, mm6    ;mm3: 00 R3 00 B3 00 R1 00 B1 ;16<<8 13<<8 16<<8 13<<8
						punpckldq mm2, mm1 ;mm2: 00 R2 G2 B2 00 R0 G0 B0
						psrlw mm2, 11      ;mm2: 00 00 00 G2 00 00 00 G0
						movq mm1, mm3      ;mm1: 00 R3 00 B3 00 R1 00 B1
						pslld mm1, 20      ;mm1: 00 B3 00 00 00 B1 00 00
						paddd mm3, mm1     ;mm3: 00 C3 00 B3 00 C1 00 B1
						pand mm3, mm5      ;mm3: 00 C3 00 00 00 C1 00 00 ;(B3<<4)+R3  (B1<<4)+R1
						paddd mm3, mm2     ;mm3: 00 C3 00 G2 00 C1 00 G0

						movq mm0, [eax+ecx*8+16] ;mm0: A5 R5 G5 B5 A4 R4 G4 B4
						movq mm1, [eax+ecx*8+24] ;mm1: A7 R7 G7 B7 A6 R6 G6 B6
						psubusb mm0, mm4
						psubusb mm1, mm4
						movq mm2, mm0      ;mm2: 00 R5 G5 B5 00 R4 G4 B4
						punpckhdq mm0, mm1 ;mm0: 00 R7 G7 B7 00 R5 G5 B5
						pand mm0, mm7      ;mm0: 00 R7 00 B7 00 R5 00 B5
						pmulhw mm0, mm6    ;mm0: 00 R7 00 B7 00 R5 00 B5 ;16<<8 13<<8 16<<8 13<<8
						punpckldq mm2, mm1 ;mm2: 00 R6 G6 B6 00 R4 G4 B4
						add ecx, 4
						psrlw mm2, 11      ;mm2: 00 00 00 G6 00 00 00 G4
						movq mm1, mm0      ;mm1: 00 R7 00 B7 00 R5 00 B5
						pslld mm1, 20      ;mm1: 00 B7 00 00 00 B5 00 00
						paddd mm0, mm1     ;mm0: 00 C7 00 B7 00 C5 00 B5
						pand mm0, mm5      ;mm0: 00 C7 00 00 00 C5 00 00 ;(B7<<4)+R7  (B5<<4)+R5
						paddd mm0, mm2     ;mm0: 00 C7 00 G6 00 C5 00 G4

						packuswb mm3, mm0  ;mm3: C7 G6 C5 G4 C3 G2 C1 G0
						paddd mm3, mask8f  ;mm3: C7 C6 C5 C4 C3 C2 C1 C0
						movq [edx+ecx*2], mm3
						js short beg8bb

						add eax, rbpl
						add edx, wbpl
						sub ebx, 1
						jnz begall8b

			 endit8b:psubd mm4, mask8h
						pmovmskb esi, mm4
						and esi, 7
						paddd mm4, mask8lut[esi*8]
						movq mask8d, mm4

						pop esi
						pop ebx
						emms
					}

				}
				//mask8cnt ^= 1;

				break;
			case 15:
				if (cputype&(1<<22)) //MMX+
				{
					_asm
					{
						push ebx
						push esi

						movq mm7, maskrb15
						movq mm6, maskgg15
						movq mm5, maskml15

						mov eax, rplc
						mov edx, wplc
						mov ecx, rxsiz
						mov ebx, rysiz
						add ecx, 2
						shr ecx, 1
						lea eax, [eax+ecx*8]
						lea edx, [edx+ecx*4-8]
						neg ecx
						mov rysiz, ecx ;Use rysiz as scratch register
						jmp short into15
				beg15:   ;src: AaaaaaaaRrrrrrrrGgggggggBbbbbbbb """
							;     --------Rrrrr---Ggggg---Bbbbb--- """
							;dst: -RrrrrGggggBbbbb-RrrrrGggggBbbbb "
						movntq [edx+ecx*4], mm0
			  into15:movq mm0, [eax+ecx*8]   ;AaaaaaaaRrrrrrrrGgggggggBbbbbbbb "
						movq mm1, [eax+ecx*8+8] ;AaaaaaaaRrrrrrrrGgggggggBbbbbbbb "
						movq mm2, mm0
						movq mm3, mm1
						pand mm0, mm7      ;--------Rrrrr-----------Bbbbb--- "
						pand mm1, mm7      ;--------Rrrrr-----------Bbbbb--- "
						pmaddwd mm0, mm5   ;-----------Rrrrr-----Bbbbb------ " ;8192 8 8192 8
						pmaddwd mm1, mm5   ;-----------Rrrrr-----Bbbbb------ " ;8192 8 8192 8
						prefetchnta [eax+ecx*8+72]
						add ecx, 2
						pand mm2, mm6      ;----------------Ggggg----------- "
						pand mm3, mm6      ;----------------Ggggg----------- "
						por mm0, mm2       ;-----------RrrrrGggggBbbbb------ "
						por mm1, mm3       ;-----------RrrrrGggggBbbbb------ "
						psrld mm0, 6       ;-----------------RrrrrGggggBbbbb "
						psrld mm1, 6       ;-----------------RrrrrGggggBbbbb "
						packssdw mm0, mm1  ;RrrrrGggggBbbbb """
						js short beg15

						test rxsiz, 2
						jz short skip15a
						movd [edx+ecx*4], mm0
						psrlq mm0, 32
						add ecx, 1
			 skip15a:test rxsiz, 1
						jz short skip15b
						movd esi, mm0
						mov [edx+ecx*4], si
			 skip15b:

						add eax, rbpl
						add edx, wbpl
						sub ebx, 1
						mov ecx, rysiz
						jnz short into15
						emms

						pop esi
						pop ebx
					}
				}
				else
				{
					_asm
					{
						push ebx
						push esi

						movq mm7, maskrb15
						movq mm6, maskgg15
						movq mm5, maskml15

						mov eax, rplc
						mov edx, wplc
						mov ecx, rxsiz
						mov ebx, rysiz
						add ecx, 2
						shr ecx, 1
						lea eax, [eax+ecx*8]
						lea edx, [edx+ecx*4-8]
						neg ecx
						mov rysiz, ecx ;Use rysiz as scratch register
						jmp short into15b
			  beg15b:   ;src: AaaaaaaaRrrrrrrrGgggggggBbbbbbbb """
							;     --------Rrrrr---Ggggg---Bbbbb--- """
							;dst: -RrrrrGggggBbbbb-RrrrrGggggBbbbb "
						movq [edx+ecx*4], mm0
			 into15b:movq mm0, [eax+ecx*8]   ;AaaaaaaaRrrrrrrrGgggggggBbbbbbbb "
						movq mm1, [eax+ecx*8+8] ;AaaaaaaaRrrrrrrrGgggggggBbbbbbbb "
						movq mm2, mm0
						movq mm3, mm1
						pand mm0, mm7      ;--------Rrrrr-----------Bbbbb--- "
						pand mm1, mm7      ;--------Rrrrr-----------Bbbbb--- "
						pmaddwd mm0, mm5   ;-----------Rrrrr-----Bbbbb------ " ;8192 8 8192 8
						pmaddwd mm1, mm5   ;-----------Rrrrr-----Bbbbb------ " ;8192 8 8192 8
						add ecx, 2
						pand mm2, mm6      ;----------------Ggggg----------- "
						pand mm3, mm6      ;----------------Ggggg----------- "
						por mm0, mm2       ;-----------RrrrrGggggBbbbb------ "
						por mm1, mm3       ;-----------RrrrrGggggBbbbb------ "
						psrld mm0, 6       ;-----------------RrrrrGggggBbbbb "
						psrld mm1, 6       ;-----------------RrrrrGggggBbbbb "
						packssdw mm0, mm1  ;RrrrrGggggBbbbb """
						js short beg15b

						test rxsiz, 2
						jz short skip15ab
						movd [edx+ecx*4], mm0
						psrlq mm0, 32
						add ecx, 1
			skip15ab:test rxsiz, 1
						jz short skip15bb
						movd esi, mm0
						mov [edx+ecx*4], si
			skip15bb:

						add eax, rbpl
						add edx, wbpl
						sub ebx, 1
						mov ecx, rysiz
						jnz short into15b
						emms

						pop esi
						pop ebx
					}
				}
				break;
			case 16:
				if (cputype&(1<<22)) //MMX+
				{
					_asm
					{
						push ebx
						push esi

						movq mm7, maskrb16
						movq mm6, maskgg16
						movq mm5, maskml16
						mov eax, rplc
						mov edx, wplc
						mov ecx, rxsiz
						mov ebx, rysiz
						add ecx, 2
						shr ecx, 1
						lea eax, [eax+ecx*8]
						lea edx, [edx+ecx*4-8]
						neg ecx
						mov rysiz, ecx ;Use rysiz as scratch register
						jmp short into16
				beg16:   ;src: AaaaaaaaRrrrrrrrGgggggggBbbbbbbb """
							;     --------Rrrrr---Gggggg--Bbbbb--- """
							;dst: RrrrrGgggggBbbbbRrrrrGgggggBbbbb "
						movntq [edx+ecx*4], mm0
			  into16:movq mm0, [eax+ecx*8]   ;AaaaaaaaRrrrrrrrGgggggggBbbbbbbb "
						movq mm1, [eax+ecx*8+8] ;AaaaaaaaRrrrrrrrGgggggggBbbbbbbb "
						movq mm2, mm0
						movq mm3, mm1
						pand mm0, mm7      ;--------Rrrrr-----------Bbbbb--- "
						pand mm1, mm7      ;--------Rrrrr-----------Bbbbb--- "
						pmaddwd mm0, mm5   ;-----------Rrrrr------Bbbbb----- " ;8192 4 8192 4
						prefetchnta [eax+ecx*8+72]
						add ecx, 2
						pmaddwd mm1, mm5   ;-----------Rrrrr------Bbbbb----- " ;8192 4 8192 4
						pand mm2, mm6      ;----------------Gggggg---------- "
						pand mm3, mm6      ;----------------Gggggg---------- "
						por mm0, mm2       ;-----------RrrrrGgggggBbbbb----- "
						por mm1, mm3       ;-----------RrrrrGgggggBbbbb----- "
						psrld mm0, 5       ;----------------RrrrrGgggggBbbbb "
						psrld mm1, 5       ;----------------RrrrrGgggggBbbbb "
						pshufw mm0, mm0, 8 ;--------------------------------RrrrrGgggggBbbbbRrrrrGgggggBbbbb
						pshufw mm1, mm1, 8 ;--------------------------------RrrrrGgggggBbbbbRrrrrGgggggBbbbb
						punpckldq mm0, mm1 ;RrrrrGgggggBbbbbRrrrrGgggggBbbbb "
						js short beg16

						test rxsiz, 2
						jz short skip16a
						movd [edx+ecx*4], mm0
						psrlq mm0, 32
						add ecx, 1
			 skip16a:test rxsiz, 1
						jz short skip16b
						movd esi, mm0
						mov [edx+ecx*4], si
			 skip16b:

						add eax, rbpl
						add edx, wbpl
						sub ebx, 1
						mov ecx, rysiz
						jnz short into16
						emms

						pop esi
						pop ebx
					}
				}
				else
				{
					_asm
					{
						push ebx
						push esi

						movq mm7, maskrb16
						movq mm6, maskgg16
						movq mm5, maskml16
						movq mm4, mask16a
						mov eax, rplc
						mov edx, wplc
						mov ecx, rxsiz
						mov ebx, rysiz
						add ecx, 2
						shr ecx, 1
						lea eax, [eax+ecx*8]
						lea edx, [edx+ecx*4-8]
						neg ecx
						mov rysiz, ecx ;Use rysiz as scratch register
						jmp short into16b
			  beg16b:   ;src: AaaaaaaaRrrrrrrrGgggggggBbbbbbbb """
							;     --------Rrrrr---Gggggg--Bbbbb--- """
							;dst: RrrrrGgggggBbbbbRrrrrGgggggBbbbb "
						movq [edx+ecx*4], mm0
			 into16b:movq mm0, [eax+ecx*8]   ;AaaaaaaaRrrrrrrrGgggggggBbbbbbbb "
						movq mm1, [eax+ecx*8+8] ;AaaaaaaaRrrrrrrrGgggggggBbbbbbbb "
						movq mm2, mm0
						movq mm3, mm1
						pand mm0, mm7      ;--------Rrrrr-----------Bbbbb--- "
						pand mm1, mm7      ;--------Rrrrr-----------Bbbbb--- "
						pmaddwd mm0, mm5   ;-----------Rrrrr------Bbbbb----- " ;8192 4 8192 4
						add ecx, 2
						pmaddwd mm1, mm5   ;-----------Rrrrr------Bbbbb----- " ;8192 4 8192 4
						pand mm2, mm6      ;----------------Gggggg---------- "
						pand mm3, mm6      ;----------------Gggggg---------- "
						por mm0, mm2       ;-----------RrrrrGgggggBbbbb----- "
						por mm1, mm3       ;-----------RrrrrGgggggBbbbb----- "
						psrld mm0, 5       ;----------------RrrrrGgggggBbbbb "
						psrld mm1, 5       ;----------------RrrrrGgggggBbbbb "
						psubd mm0, mm4     ;subtract 32768 from all 4 dwords so packssdw doesn't saturate
						psubd mm1, mm4
						packssdw mm0, mm1
						paddw mm0, mask16b ;convert back to unsigned short
						js short beg16b

						test rxsiz, 2
						jz short skip16ab
						movd [edx+ecx*4], mm0
						psrlq mm0, 32
						add ecx, 1
			skip16ab:test rxsiz, 1
						jz short skip16bb
						movd esi, mm0
						mov [edx+ecx*4], si
			skip16bb:

						add eax, rbpl
						add edx, wbpl
						sub ebx, 1
						mov ecx, rysiz
						jnz short into16b
						emms

						pop esi
						pop ebx
					}
				}
				break;
			case 24:
				rxsiz &= ~3; if (rxsiz <= 0) return;
				if (cputype&(1<<22)) //MMX+
				{
					_asm
					{
						push ebx
						push edi

						movq mm7, mask24a
						movq mm6, mask24b
						movq mm5, mask24c
						movq mm4, mask24d
						mov eax, rplc
						mov edi, wplc
						mov ebx, rysiz
						mov ecx, rxsiz
						shr ecx, 1
						lea eax, [eax+ecx*8]
						sub edi, 12
						neg ecx
						mov rxsiz, ecx

			prebeg24:mov edx, edi
						mov ecx, rxsiz
				beg24:   ;src: aRGB aRGB aRGB aRGB
							;     RGBR GB-- --RG BRGB (intermediate step)
							;dst: RGBR GBRG BRGB
						movq mm0, [eax+ecx*8]   ;mm0: [aRGB aRGB]
						movq mm1, [eax+ecx*8+8] ;mm1: [aRGB aRGB]
						pslld mm1, 8       ;mm1: [RGB- RGB-]
						movq mm2, mm0      ;mm2: [aRGB aRGB]
						movq mm3, mm1      ;mm3: [RGB- RGB-]

						pand mm0, mm7      ;mm0: [-RGB ----]
						pand mm1, mm6      ;mm1: [---- RGB-]
						pand mm2, mm5      ;mm2: [---- -RGB]
						pand mm3, mm4      ;mm3: [RGB- ----]
						psrlq mm0, 8       ;mm0: [--RG B---]
						add edx, 12
						add ecx, 2
						psllq mm1, 8       ;mm1: [---R GB--]
						por mm0, mm2       ;mm0: [--RG BRGB]
						por mm1, mm3       ;mm1: [RGBR GB--]

						movd [edx], mm0    ;mm0: [--RG BRGB]
						psrlq mm0, 32      ;mm0: [---- --RG]
						por mm0, mm1       ;mm0: [RGBR GBRG]
						movd [edx+4], mm0
						psrlq mm0, 32      ;mm0: [---- RGBR]
						movd [edx+8], mm0

						prefetchnta [eax+ecx*8+128]
						js short beg24

						add eax, rbpl
						add edi, wbpl
						sub ebx, 1
						jnz short prebeg24
						emms

						pop edi
						pop ebx
					}
				}
				else
				{
					_asm
					{
						push ebx
						push edi

						movq mm7, mask24a
						movq mm6, mask24b
						movq mm5, mask24c
						movq mm4, mask24d
						mov eax, rplc
						mov edi, wplc
						mov ebx, rysiz
						mov ecx, rxsiz
						shr ecx, 1
						lea eax, [eax+ecx*8]
						sub edi, 12
						neg ecx
						mov rxsiz, ecx

		  prebeg24b:mov edx, edi
						mov ecx, rxsiz
			  beg24b:   ;src: aRGB aRGB aRGB aRGB
							;     RGBR GB-- --RG BRGB (intermediate step)
							;dst: RGBR GBRG BRGB
						movq mm0, [eax+ecx*8]   ;mm0: [aRGB aRGB]
						movq mm1, [eax+ecx*8+8] ;mm1: [aRGB aRGB]
						pslld mm1, 8       ;mm1: [RGB- RGB-]
						movq mm2, mm0      ;mm2: [aRGB aRGB]
						movq mm3, mm1      ;mm3: [RGB- RGB-]

						pand mm0, mm7      ;mm0: [-RGB ----]
						pand mm1, mm6      ;mm1: [---- RGB-]
						pand mm2, mm5      ;mm2: [---- -RGB]
						pand mm3, mm4      ;mm3: [RGB- ----]
						psrlq mm0, 8       ;mm0: [--RG B---]
						add edx, 12
						add ecx, 2
						psllq mm1, 8       ;mm1: [---R GB--]
						por mm0, mm2       ;mm0: [--RG BRGB]
						por mm1, mm3       ;mm1: [RGBR GB--]

						movd [edx], mm0    ;mm0: [--RG BRGB]
						psrlq mm0, 32      ;mm0: [---- --RG]
						por mm0, mm1       ;mm0: [RGBR GBRG]
						movd [edx+4], mm0
						psrlq mm0, 32      ;mm0: [---- RGBR]
						movd [edx+8], mm0

						js short beg24b

						add eax, rbpl
						add edi, wbpl
						sub ebx, 1
						jnz short prebeg24b
						emms

						pop edi
						pop ebx
					}
				}
				break;
			case 32:
				if (cputype&(1<<22)) //MMX+
				{
					_asm
					{
						push ebx
						push esi
						push edi

						mov eax, rplc
						mov edx, wplc
						mov ecx, rxsiz
						mov ebx, rysiz
						add ecx, 2
						shr ecx, 1
						lea eax, [eax+ecx*8]
						lea edx, [edx+ecx*8]
						neg ecx
						mov rysiz, ecx
						mov esi, rbpl
						mov edi, wbpl
						jmp short into32

				beg32:movntq qword ptr [edx+ecx*8-8], mm0
			  into32:movq mm0, qword ptr [eax+ecx*8]
						prefetchnta [eax+ecx*8+16]
						add ecx, 1
						jnz short beg32

						test rxsiz, 1
						jz short skip32
						movd dword ptr [edx+ecx*8-8], mm0
			  skip32:

						add eax, esi
						add edx, edi
						sub ebx, 1
						mov ecx, rysiz
						jnz short into32
						emms

						pop edi
						pop esi
						pop ebx
					}
				}
				else
				{
					_asm
					{
						push ebx
						push esi
						push edi

						mov eax, rplc
						mov edx, wplc
						mov ecx, rxsiz
						mov ebx, rysiz
						add ecx, 2
						shr ecx, 1
						lea eax, [eax+ecx*8]
						lea edx, [edx+ecx*8]
						neg ecx
						mov rysiz, ecx
						mov esi, rbpl
						mov edi, wbpl
						jmp short into32b

			  beg32b:movq qword ptr [edx+ecx*8-8], mm0
			 into32b:movq mm0, qword ptr [eax+ecx*8]
						add ecx, 1
						jnz short beg32b

						test rxsiz, 1
						jz short skip32b
						movd dword ptr [edx+ecx*8-8], mm0
			 skip32b:

						add eax, esi
						add edx, edi
						sub ebx, 1
						mov ecx, rysiz
						jnz short into32b
						emms

						pop edi
						pop esi
						pop ebx
					}
				}
				break;
		}
	}
#endif
}

		 LPDIRECTDRAW lpdd = 0;

		 LPDIRECTDRAWSURFACE ddsurf[3] = {0,0,0};
static LPDIRECTDRAWPALETTE ddpal = 0;
#ifndef NO_CLIPPER
static LPDIRECTDRAWCLIPPER ddclip = 0;
static RGNDATA *ddcliprd = 0;
static unsigned long ddcliprdbytes = 0;
#endif
static DDSURFACEDESC ddsd;
static long ddlocked = 0, ddrawemulbpl = 0;
static void *ddrawemulbuf = 0;
static long cantlockprimary = 1; //FUKFUKFUKFUK
#define OFFSCREENHACK 1 //FUKFUKFUKFUK

  //The official DirectDraw method for retrieving video modes!
#define MAXVALIDMODES 256
typedef struct { long x, y; char c, r0, g0, b0, a0, rn, gn, bn, an; } validmodetype;
static validmodetype validmodelist[MAXVALIDMODES];
static long validmodecnt = 0;
validmodetype curvidmodeinfo;

#ifdef _MSC_VER

static _inline long bsf (long a) { _asm bsf eax, a } //is it safe to assume eax is return value?
static _inline long bsr (long a) { _asm bsr eax, a } //is it safe to assume eax is return value?

#endif

static void grabmodeinfo (long x, long y, DDPIXELFORMAT *ddpf, validmodetype *valptr)
{
	memset(valptr,0,sizeof(validmodetype));

	valptr->x = x; valptr->y = y;
	if (ddpf->dwFlags&DDPF_PALETTEINDEXED8) { valptr->c = 8; return; }
	if (ddpf->dwFlags&DDPF_RGB)
	{
		valptr->c = (char)ddpf->dwRGBBitCount;
		if (ddpf->dwRBitMask)
		{
			valptr->r0 = (char)bsf(ddpf->dwRBitMask);
			valptr->rn = (char)(bsr(ddpf->dwRBitMask)-(valptr->r0)+1);
		}
		if (ddpf->dwGBitMask)
		{
			valptr->g0 = (char)bsf(ddpf->dwGBitMask);
			valptr->gn = (char)(bsr(ddpf->dwGBitMask)-(valptr->g0)+1);
		}
		if (ddpf->dwBBitMask)
		{
			valptr->b0 = (char)bsf(ddpf->dwBBitMask);
			valptr->bn = (char)(bsr(ddpf->dwBBitMask)-(valptr->b0)+1);
		}
		if (ddpf->dwRGBAlphaBitMask)
		{
			valptr->a0 = (char)bsf(ddpf->dwRGBAlphaBitMask);
			valptr->an = (char)(bsr(ddpf->dwRGBAlphaBitMask)-(valptr->a0)+1);
		}
	}
}

HRESULT WINAPI lpEnumModesCallback (LPDDSURFACEDESC dsd, LPVOID lpc)
{
	grabmodeinfo(dsd->dwWidth,dsd->dwHeight,&dsd->ddpfPixelFormat,&validmodelist[validmodecnt]); validmodecnt++;
	if (validmodecnt >= MAXVALIDMODES) return(DDENUMRET_CANCEL); else return(DDENUMRET_OK);
}

long getvalidmodelist (validmodetype **davalidmodelist)
{
	if (!lpdd) return(0);
	if (!validmodecnt) lpdd->EnumDisplayModes(0,0,0,lpEnumModesCallback);
	(*davalidmodelist) = validmodelist;
	return(validmodecnt);
}

void uninitdirectdraw ()
{
	if (ddpal) { ddpal->Release(); ddpal = 0; }
	if (lpdd)
	{
#ifndef NO_CLIPPER
		if (ddcliprd) { free(ddcliprd); ddcliprd = 0; ddcliprdbytes = 0; }
		if (ddclip) { ddclip->Release(); ddclip = 0; }
#endif
		if (ddsurf[0]) { ddsurf[0]->Release(); ddsurf[0] = 0; }
		if (ddrawemulbuf) { free(ddrawemulbuf); ddrawemulbuf = 0; }
		lpdd->Release(); lpdd = 0;
	}
}

void updatepalette (long start, long danum)
{
	long i;
	if (colbits == 8)
	{
		switch(ddrawuseemulation)
		{
			case 15:
				for(i=start+danum-1;i>=start;i--)
					lpal[i] = (((pal[i].peRed>>3)&31)<<10) + (((pal[i].peGreen>>3)&31)<<5) + ((pal[i].peBlue>>3)&31);
				return;
			case 16:
				for(i=start+danum-1;i>=start;i--)
					lpal[i] = (((pal[i].peRed>>3)&31)<<11) + (((pal[i].peGreen>>2)&63)<<5) + ((pal[i].peBlue>>3)&31);
				return;
			case 24: case 32:
				for(i=start+danum-1;i>=start;i--)
					lpal[i] = ((pal[i].peRed)<<16) + ((pal[i].peGreen)<<8) + pal[i].peBlue;
				return;
			default: break;
		}
	}
	if (!ddpal) return;
	if (ddpal->SetEntries(0,start,danum,&pal[start]) == DDERR_SURFACELOST)
		{ ddsurf[0]->Restore(); ddpal->SetEntries(0,start,danum,&pal[start]); }
}

	 //((z/(16-1))^.8)*255, ((z/(32-1))^.8)*255, ((z/((13 fullscreen/12 windowed)-1))^.8)*255
static char palr[16] = {0,29,51,70,89,106,123,139,154,169,184,199,213,227,241,255};
static char palg[32] = {0,16,28,39,50,59,69,78,86,95,103,111,119,127,135,143,
				 150,158,165,172,180,187,194,201,208,215,222,228,235,242,248,255};
static char palb[16] = {0,35,61,84,106,127,146,166,184,203,220,238,255,0,0,0};

void emul8setpal ()
{
	long i, r, g, b;

	if (fullscreen)
	{
			//13 shades of blue in fullscreen mode
		*(long *)&palb[0] =   0+( 35<<8)+( 61<<16)+( 84<<24);
		*(long *)&palb[4] = 106+(127<<8)+(146<<16)+(166<<24);
		*(long *)&palb[8] = 184+(203<<8)+(220<<16)+(238<<24);
		mask8b = 0x10000d0010000d00;
		mask8f = 0x00d000d000d000d0;
		mask8g = 0xd000d000d000d000;

		i = 0;
		for(b=0;b<13;b++)
			for(r=0;r<16;r++)
			{
				pal[i].peRed = palr[r];
				pal[i].peGreen = 0;
				pal[i].peBlue = palb[b];
				pal[i].peFlags = PC_NOCOLLAPSE;
				i++;
			} //224 cols
		for(g=0;g<32;g++)
		{
			pal[i].peRed = 0;
			pal[i].peGreen = palg[g];
			pal[i].peBlue = 0;
			pal[i].peFlags = PC_NOCOLLAPSE;
			i++;
		}
	}
	else
	{
			//12 shades of blue in fullscreen mode
		*(long *)&palb[0] =   0+( 37<<8)+( 65<<16)+( 90<<24);
		*(long *)&palb[4] = 114+(136<<8)+(157<<16)+(178<<24);
		*(long *)&palb[8] = 198+(217<<8)+(236<<16)+(255<<24);
		mask8b = 0x10000c0010000c00;
		mask8f = 0x0aca0aca0aca0aca;
		mask8g = 0xca0aca0aca0aca0a;

		for(i=0;i<10;i++) { pal[i].peFlags = PC_EXPLICIT; pal[i].peRed = (char)i; pal[i].peGreen = pal[i].peBlue = 0; }
		for(b=0;b<12;b++)
			for(r=0;r<16;r++)
			{
				pal[i].peRed = palr[r];
				pal[i].peGreen = 0;
				pal[i].peBlue = palb[b];
				pal[i].peFlags = PC_NOCOLLAPSE;
				i++;
			} //192 cols
		for(g=0;g<32;g++)
		{
			pal[i].peRed = 0;
			pal[i].peGreen = palg[g];
			pal[i].peBlue = 0;
			pal[i].peFlags = PC_NOCOLLAPSE;
			i++;
		}
		for(;i<256-10;i++) { pal[i].peFlags = PC_NOCOLLAPSE; pal[i].peRed = pal[i].peGreen = pal[i].peBlue = 0; }
		for(;i<256;i++) { pal[i].peFlags = PC_EXPLICIT; pal[i].peRed = (char)i; pal[i].peGreen = pal[i].peBlue = 0; }
	}
}

long initdirectdraw (long daxres, long dayres, long dacolbits)
{
	HRESULT hr;
	DDSCAPS ddscaps;
	char buf[256];
	long ncolbits;

	xres = daxres; yres = dayres; colbits = dacolbits;
	if ((hr = DirectDrawCreate(0,&lpdd,0)) >= 0)
	{
		if (fullscreen)
		{
			if ((hr = lpdd->SetCooperativeLevel(ghwnd,DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN)) >= 0)
			{
				ddrawdebugmode = -1;
				ncolbits = dacolbits; ddrawuseemulation = 0;
				//ncolbits = 8; //HACK FOR TESTING!
				//ncolbits = 16; //HACK FOR TESTING!
				do
				{
					if ((hr = lpdd->SetDisplayMode(daxres,dayres,ncolbits)) >= 0)
          {
						if (1)//(ncolbits != dacolbits) // FULLSCREEN ALPHA PERFORMANCE HACK!
						{
							ddrawemulbuf = malloc(((daxres*dayres+7)&~7)*((dacolbits+7)>>3)+16);
							if (!ddrawemulbuf) { ncolbits = 0; break; }
							ddrawemulbpl = ((dacolbits+7)>>3)*daxres;
							ddrawuseemulation = ncolbits;
						}
						ddsd.dwSize = sizeof(ddsd);
						ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
						ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE|DDSCAPS_COMPLEX|DDSCAPS_FLIP;
						if (maxpages > 2) ddsd.dwBackBufferCount = 2;
										 else ddsd.dwBackBufferCount = 1;
						if ((hr = lpdd->CreateSurface(&ddsd,&ddsurf[0],0)) >= 0)
						{
							DDPIXELFORMAT ddpf;
							ddpf.dwSize = sizeof(DDPIXELFORMAT);
							if (!ddsurf[0]->GetPixelFormat(&ddpf))
								grabmodeinfo(daxres,dayres,&ddpf,&curvidmodeinfo);

							ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
							if ((hr = ddsurf[0]->GetAttachedSurface(&ddscaps,&ddsurf[1])) >= 0)
							{
#if (OFFSCREENHACK)
								DDSURFACEDESC nddsd;
								ddsurf[2] = ddsurf[1];
								ZeroMemory(&nddsd,sizeof(nddsd));
								nddsd.dwSize = sizeof(nddsd);
								nddsd.dwFlags = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
								nddsd.dwWidth = (daxres|1); //This hack (|1) ensures pitch isn't near multiple of 4096 (slow on P4)!
								nddsd.dwHeight = dayres;
								nddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
								if ((hr = lpdd->CreateSurface(&nddsd,&ddsurf[1],0)) < 0) return(-1);
#endif
								if (ncolbits != 8) return(1);
								if (lpdd->CreatePalette(DDPCAPS_8BIT,pal,&ddpal,0) >= 0)
								{
									ddsurf[0]->SetPalette(ddpal);
									if (ddrawuseemulation)
										{ emul8setpal(); updatepalette(0,256); }
									return(1);
								}
							}
						}
					}
					switch(dacolbits)
					{
						case 8:
							switch (ncolbits)
							{
								case 8: ncolbits = 32; break;
								case 32: ncolbits = 24; break;
								case 24: ncolbits = 16; break;
								case 16: ncolbits = 15; break;
								default: ncolbits = 0; break;
							}
							break;
						case 32:
							switch (ncolbits)
							{
								case 32: ncolbits = 24; break;
								case 24: ncolbits = 16; break;
								case 16: ncolbits = 15; break;
								case 15: ncolbits = 8; break;
								default: ncolbits = 0; break;
							}
							break;
						default: ncolbits = 0; break;
					}
				} while (ncolbits);
				if (!ncolbits)
				{
					validmodetype *validmodelist;
					char vidlistbuf[4096];
					long i, j;

					validmodecnt = getvalidmodelist(&validmodelist);
					wsprintfA(vidlistbuf,"Valid fullscreen %d-bit DirectDraw modes:\n",colbits);
					j = 0;
					for(i=0;i<validmodecnt;i++)
						if (validmodelist[i].c == colbits)
						{
							wsprintfA(buf,"/%dx%d\n",validmodelist[i].x,validmodelist[i].y);
							strcat(vidlistbuf,buf);
							j++;
						}
					if (!j) strcat(vidlistbuf,"None! Try a different bit depth.");

					wsprintfA(buf,"initdirectdraw failed: 0x%x",hr);
					MessageBox(NULL, Ctt(vidlistbuf), Ctt(buf), MB_OK);

					uninitdirectdraw();
					return(0);
				}
			}
		}
		else
		{
			if ((hr = lpdd->SetCooperativeLevel(ghwnd,DDSCL_NORMAL)) >= 0)
			{
				ddsd.dwSize = sizeof(ddsd);
				ddsd.dwFlags = DDSD_CAPS;
				ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
				if ((hr = lpdd->CreateSurface(&ddsd,&ddsurf[0],0)) >= 0)
				{
#ifndef NO_CLIPPER
						//Create clipper object for windowed mode
					if ((hr = lpdd->CreateClipper(0,&ddclip,0)) == DD_OK)
					{
						ddclip->SetHWnd(0,ghwnd); //Associate clipper with window
						hr = ddsurf[0]->SetClipper(ddclip);
						if (hr == DD_OK)
						{
#endif
							ddsd.dwSize = sizeof(ddsd);
							ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
							ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
							ddsd.dwWidth = xres;
							ddsd.dwHeight = yres;
							if ((hr = lpdd->CreateSurface(&ddsd,&ddsurf[1],0)) >= 0)
							{
								DDCAPS ddcaps;
								HDC hDC = GetDC(0);
								ncolbits = GetDeviceCaps(hDC,BITSPIXEL);
								ReleaseDC(0,hDC);

								ddcaps.dwSize = sizeof(ddcaps);
								if (lpdd->GetCaps(&ddcaps,0) == DD_OK) //Better to catch DDERR_CANTLOCKSURFACE here than in startdirectdraw()
									if (ddcaps.dwCaps&DDCAPS_NOHARDWARE) cantlockprimary = 1; //For LCD screens mainly
								if (osvi.dwMajorVersion >= 6) cantlockprimary = 1; //Longhorn/Vista

									//03/08/2004: Now that clipper works for ddrawemulbuf, ddrawemulbuf is not only
									//faster, but it supports WM_SIZE dragging better than ddsurf[0]->Blt! :)
									//
									//kblit32 currently supports these modes:
									//   colbits ->    ncolbits
									//       8   ->  . 15 16 24 32
									//      15   ->  .  .  .  .  .
									//      16   ->  .  .  .  .  .
									//      24   ->  .  .  .  .  .
									//      32   ->  8 15 16 24 32
								if ((ncolbits != colbits) || ((colbits == 32) && (!cantlockprimary)))
								{
#if (OFFSCREENHACK)
									if (colbits != colbits)
#endif
									{
										ddrawemulbuf = malloc(((xres*yres+7)&~7)*((colbits+7)>>3)+16);
										if (ddrawemulbuf)
										{
											ddrawemulbpl = ((colbits+7)>>3)*xres;
											ddrawuseemulation = ncolbits;
											if ((ncolbits == 8) && (colbits == 32)) emul8setpal();
										}
									}
								}

								if ((ncolbits == 8) || (colbits == 8)) updatepalette(0,256);

								DDPIXELFORMAT ddpf;
								ddpf.dwSize = sizeof(DDPIXELFORMAT);
								if (!ddsurf[0]->GetPixelFormat(&ddpf)) //colbits = ddpf.dwRGBBitCount;
								{
									grabmodeinfo(daxres,dayres,&ddpf,&curvidmodeinfo);

										//If mode is 555 color (and not 565), use 15-bit emulation code...
									if ((colbits != 16) && (ncolbits == 16)
																&& (curvidmodeinfo.r0 == 10) && (curvidmodeinfo.rn == 5)
																&& (curvidmodeinfo.g0 ==  5) && (curvidmodeinfo.gn == 5)
																&& (curvidmodeinfo.b0 ==  0) && (curvidmodeinfo.bn == 5))
										ddrawuseemulation = 15;
								}

								if (ncolbits == 8)
									if (lpdd->CreatePalette(DDPCAPS_8BIT,pal,&ddpal,0) >= 0)
										ddsurf[0]->SetPalette(ddpal);

								return(1);
							}
#ifndef NO_CLIPPER
						}
					}
#endif
				}
			}
		}
	}
	uninitdirectdraw();
	wsprintfA(buf,"initdirectdraw failed: 0x%08lx",hr);
	MessageBox(NULL, Ctt(buf), Ctt("error"), MB_OK);
	return(0);
}

void stopdirectdraw ()
{
	if (!ddlocked) return;
	if (!ddrawuseemulation) ddsurf[1]->Unlock(ddsd.lpSurface);
	ddlocked = 0;
}

long startdirectdraw (long *vidplc, long *dabpl, long *daxres, long *dayres)
{
	HRESULT hr;

	if (ddlocked) { stopdirectdraw(); ddlocked = 0; }  //{ return(0); }

	if (ddrawuseemulation)
	{
		*vidplc = (long)ddrawemulbuf; *dabpl = xres*((colbits+7)>>3);
		*daxres = xres; *dayres = yres; ddlocked = 1;
		return(1);
	}

	while (1)
	{
		if ((hr = ddsurf[1]->Lock(0,&ddsd,DDLOCK_SURFACEMEMORYPTR|DDLOCK_WAIT,0)) == DD_OK) break;
		if (hr == DDERR_SURFACELOST)
		{
			if (ddsurf[0]->Restore() != DD_OK) return(0);
			if (ddsurf[1]->Restore() != DD_OK) return(0);
		}
		if (hr == DDERR_CANTLOCKSURFACE) return(-1); //if true, set cantlockprimary = 1;
		if (hr != DDERR_WASSTILLDRAWING) return(0);
	}

		//DDLOCK_WAIT MANDATORY! (to prevent sudden exit back to windows)!
	//if (hr = ddsurf[1]->Lock(0,&ddsd,DDLOCK_SURFACEMEMORYPTR|DDLOCK_WAIT,0) != DD_OK)
	//   return(0);

	*vidplc = (long)ddsd.lpSurface; *dabpl = ddsd.lPitch;
	*daxres = xres; *dayres = yres; ddlocked = 1;
	return(1);
}

static HDC ghdc;
HDC startdc ()
{
	ddsurf[1]->GetDC(&ghdc);
	return(ghdc);
}

void stopdc ()
{
	ddsurf[1]->ReleaseDC(ghdc);
}

void nextpage ()
{
	HRESULT hr;

	if (ddrawdebugmode != -1) return;

		//Note: windowed mode could be faster by having kblit go directly
		// to the ddsurf[0] (windows video memory)
	if (ddrawuseemulation)
	{
		long i, fplace, bpl, xsiz, ysiz;
		if (!fullscreen)
		{
			POINT topLeft, p2;
			long j;
#ifndef NO_CLIPPER
			RECT *r;
			unsigned long siz;
#endif

				//Note: this hack for windowed mode saves an unnecessary blit by
				//   going directly to primary buffer (ddsurf[0])
				//  Unfortunately, this means it ignores the clipper & you can't
				//   use the ddraw stretch function :/
			topLeft.x = 0; topLeft.y = 0;
			if (!cantlockprimary)
			{
				ddsurf[2] = ddsurf[0]; ddsurf[0] = ddsurf[1]; ddsurf[1] = ddsurf[2];
				ClientToScreen(ghwnd,&topLeft);
			}
			else
			{
				p2.x = 0; p2.y = 0;
				ClientToScreen(ghwnd,&p2);
			}

				//This is sort of a hack... but it works
			i = ddrawuseemulation; ddrawuseemulation = 0;
			j = startdirectdraw(&fplace,&bpl,&xsiz,&ysiz);
			if (j == -1)
			{
				if (!cantlockprimary)
				{
					ddsurf[2] = ddsurf[0]; ddsurf[0] = ddsurf[1]; ddsurf[1] = ddsurf[2]; //Undo earlier surface swap
					cantlockprimary = 1; ddrawuseemulation = i; return;
				}
			}
			else if (j)
			{
#ifndef NO_CLIPPER
				ddclip->GetClipList(0,0,&siz);
				if (siz > ddcliprdbytes)
				{
					ddcliprdbytes = siz;
					ddcliprd = (RGNDATA *)realloc(ddcliprd,siz);
				}
				if (!ddcliprd)
				{
#endif
					kblit32(max(-topLeft.y,0)*ddrawemulbpl +
							  max(-topLeft.x,0)*((colbits+7)>>3) + ((long)ddrawemulbuf),
							  ddrawemulbpl,
							  min(topLeft.x+xsiz,GetSystemMetrics(SM_CXSCREEN))-max(topLeft.x,0),
							  min(topLeft.y+ysiz,GetSystemMetrics(SM_CYSCREEN))-max(topLeft.y,0),
							  max(topLeft.y,0)*bpl+max(topLeft.x,0)*((i+7)>>3)+fplace,
							  i,bpl);
#ifndef NO_CLIPPER
				}
				else
				{
					ddclip->GetClipList(0,ddcliprd,&siz);

					r = (RECT *)ddcliprd->Buffer;
					for(j=0;j<(long)ddcliprd->rdh.nCount;j++)
					{
						if (cantlockprimary) { r[j].left  -= p2.x; r[j].top -= p2.y; r[j].right -= p2.x; r[j].bottom -= p2.y; }
						kblit32(max(r[j].top -topLeft.y,0)*ddrawemulbpl +
								  max(r[j].left-topLeft.x,0)*((colbits+7)>>3) + ((long)ddrawemulbuf),
								  ddrawemulbpl,
								  min(topLeft.x+xsiz,r[j].right )-max(topLeft.x,r[j].left),
								  min(topLeft.y+ysiz,r[j].bottom)-max(topLeft.y,r[j].top ),
								  max(topLeft.x,r[j].left)*((i+7)>>3) +
								  max(topLeft.y,r[j].top )*bpl + fplace,
								  i,bpl);
					}
				}
#endif
				stopdirectdraw();
			}
			ddrawuseemulation = i;

				//Finish windowed mode hack to primary buffer
			if (!cantlockprimary) { ddsurf[2] = ddsurf[0]; ddsurf[0] = ddsurf[1]; ddsurf[1] = ddsurf[2]; return; }
		}
		else
		{
				//This is sort of a hack... but it works
			i = ddrawuseemulation; ddrawuseemulation = 0;
			if (startdirectdraw(&fplace,&bpl,&xsiz,&ysiz))
			{
			kblit32((long)ddrawemulbuf,ddrawemulbpl,min(xsiz,xres),min(ysiz,yres),fplace,i,bpl);
			stopdirectdraw();
			}
			ddrawuseemulation = i;
		}
	}

	if (fullscreen)
	{
#if (OFFSCREENHACK)
		RECT r; r.left = 0; r.top = 0; r.right = xres; r.bottom = yres; //with width hack, src rect not always: xres,yres
		if (ddsurf[2]->GetBltStatus(DDGBS_CANBLT) == DDERR_WASSTILLDRAWING) return;
		if (ddsurf[2]->BltFast(0,0,ddsurf[1],&r,DDBLTFAST_NOCOLORKEY) == DDERR_SURFACELOST)
			{ ddsurf[0]->Restore(); if (ddsurf[2]->BltFast(0,0,ddsurf[1],&r,DDBLTFAST_NOCOLORKEY) != DD_OK) return; }
		if (ddsurf[0]->GetFlipStatus(DDGFS_CANFLIP) == DDERR_WASSTILLDRAWING) return; //wait for blit to complete
		if (ddsurf[0]->Flip(0,0) == DDERR_SURFACELOST) { ddsurf[0]->Restore(); ddsurf[0]->Flip(0,0); }
#endif
	}
	else
	{
		RECT rcSrc, rcDst;
		POINT topLeft;

		rcSrc.left = 0; rcSrc.top = 0;
		rcSrc.right = xres; rcSrc.bottom = yres;
		rcDst = rcSrc;
		topLeft.x = 0; topLeft.y = 0;
		ClientToScreen(ghwnd,&topLeft);
		rcDst.left += topLeft.x; rcDst.right += topLeft.x;
		rcDst.top += topLeft.y; rcDst.bottom += topLeft.y;
		if (ddsurf[0]->BltFast(rcDst.left,rcDst.top,ddsurf[1],&rcSrc,DDBLTFAST_WAIT|DDBLTFAST_NOCOLORKEY) < 0)
		{
			if (ddsurf[0]->IsLost() == DDERR_SURFACELOST) ddsurf[0]->Restore();
			if (ddsurf[1]->IsLost() == DDERR_SURFACELOST) ddsurf[1]->Restore();
			ddsurf[0]->Blt(&rcDst,ddsurf[1],&rcSrc,DDBLT_WAIT|DDBLT_ASYNC,0);
		}
	}
}

void ddflip2gdi ()
{
	if (lpdd) lpdd->FlipToGDISurface();
}

long clearscreen (long fillcolor)
{
	DDBLTFX blt;
	HRESULT hr;

	if (ddrawuseemulation)
	{
		long c = ((xres*yres+7)&~7)*((colbits+7)>>3); c >>= 3;
		if (cputype&(1<<25)) //SSE
		{
			_asm
			{
				mov edx, ddrawemulbuf
				mov ecx, c
				movd mm0, fillcolor
				punpckldq mm0, mm0
  clear32a: movntq qword ptr [edx], mm0
				add edx, 8
				sub ecx, 1
				jnz short clear32a
				emms
			}
		}
		else if (cputype&(1<<23)) //MMX
		{
			_asm
			{
				mov edx, ddrawemulbuf
				mov ecx, c
				movd mm0, fillcolor
				punpckldq mm0, mm0
  clear32b: movq qword ptr [edx], mm0
				add edx, 8
				sub ecx, 1
				jnz short clear32b
				emms
			}
		}
		else
		{
			long q[2]; q[0] = q[1] = fillcolor;
			_asm
			{
				mov edx, ddrawemulbuf
				mov ecx, c
  clear32c: fild qword ptr q
				fistp qword ptr [edx] ;NOTE: fist doesn't have a 64-bit form!
				add edx, 8
				sub ecx, 1
				jnz short clear32c
			}
		}
		return(1);
	}

		//Can't clear when locked: would SUPERCRASH!
	if (ddlocked)
	{
		long i, j, x, p, pe;

		p = (long)ddsd.lpSurface; pe = ddsd.lPitch*yres + p;
		if (colbits != 24)
		{
			switch(colbits)
			{
				case 8:           fillcolor = (fillcolor&255)*0x1010101; i = xres  ; break;
				case 15: case 16: fillcolor = (fillcolor&65535)*0x10001; i = xres*2; break;
				case 32:                                                 i = xres*4; break;
			}
			for(;p<pe;p+=ddsd.lPitch)
			{
				j = (i>>3);
				if (cputype&(1<<25)) //SSE
				{
					_asm
					{
						mov edx, p
						mov ecx, j
						movd mm0, fillcolor
						punpckldq mm0, mm0
		  clear32d: movntq qword ptr [edx], mm0
						add edx, 8
						sub ecx, 1
						jnz short clear32d
						emms
					}
				}
				else if (cputype&(1<<23)) //MMX
				{
					_asm
					{
						mov edx, p
						mov ecx, j
						movd mm0, fillcolor
						punpckldq mm0, mm0
		  clear32e: movq qword ptr [edx], mm0
						add edx, 8
						sub ecx, 1
						jnz short clear32e
						emms
					}
				}
				else
				{
					long q[2]; q[0] = q[1] = fillcolor;
					_asm
					{
						mov edx, p
						mov ecx, j
		  clear32f: fild qword ptr q
						fistp qword ptr [edx] ;NOTE: fist doesn't have a 64-bit form!
						add edx, 8
						sub ecx, 1
						jnz short clear32f
					}
				}
				x = (i&~7)+p;
				if (i&4) { *(long  *)x =        fillcolor; x += 4; }
				if (i&2) { *(short *)x = (short)fillcolor; x += 2; }
				if (i&1) { *(char  *)x =  (char)fillcolor;         }
			}
			if (cputype&(1<<25)) _asm emms //SSE
		}
		else
		{
			long dacol[3];
			fillcolor &= 0xffffff;
			dacol[0] = (fillcolor<<24)+fillcolor;      //BRGB
			dacol[1] = (fillcolor>>8)+(fillcolor<<16); //GBRG
			dacol[2] = (fillcolor>>16)+(fillcolor<<8); //RGBR
			i = xres*3;
			for(;p<pe;p+=ddsd.lPitch)
			{
				j = p+i-12;
				for(x=p;x<=j;x+=12)
				{                             //  x-j ³0123³4567³89AB
					*(long *)(x  ) = dacol[0]; // ÚÄÄÄÄÅÄÄÄÄÅÄÄÄÄÅÄÄÄÄ¿
					*(long *)(x+4) = dacol[1]; // ³ 12 ³    ³    ³    ³
					*(long *)(x+8) = dacol[2]; // ³  9 ³BGR ³    ³    ³
				}                             // ³  6 ³BGRB³GR  ³    ³
				switch(x-j)                   // ³  3 ³BGRB³GRBG³R   ³
				{                             // ÀÄÄÄÄÁÄÄÄÄÁÄÄÄÄÁÄÄÄÄÙ
					case 9: *(short *)x = (short)dacol[0]; *(char *)(x+2) = (char)dacol[2]; break;
					case 6: *(long *)x = dacol[0]; *(short *)(x+4) = (short)dacol[1]; break;
					case 3: *(long *)x = dacol[0]; *(long *)(x+4) = dacol[1]; *(char *)(x+8) = (char)dacol[2]; break;
				}
			}
		}
		return(1);
	}

	blt.dwSize = sizeof(blt);
	blt.dwFillColor = fillcolor;
	while (1)
	{
			//Try |DDBLT_ASYNC
		hr = ddsurf[1]->Blt(0,0,0,DDBLT_COLORFILL,&blt); //Try |DDBLT_WAIT
		if (hr == DD_OK) return(1);
		if (hr == DDERR_SURFACELOST)
		{
			if (ddsurf[0]->Restore() != DD_OK) return(0);
			if (ddsurf[1]->Restore() != DD_OK) return(0);
		}
	}
}

void evilquit (const char *str) //Evil because this function makes awful assumptions!!!
{
#ifndef NODRAW
	stopdirectdraw();
	ddflip2gdi();
#endif
	if (str) MessageBox(NULL, Ctt(str), Ctt("error"), MB_OK);

#ifndef NODRAW
	uninitdirectdraw();
#endif
	uninitapp();
	ExitProcess(0);
}
#endif