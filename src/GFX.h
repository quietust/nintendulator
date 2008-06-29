/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2008 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef GFX_H
#define GFX_H

#define	DIRECTDRAW_VERSION 0x0700
#include <ddraw.h>

struct tGFX
{
	unsigned char RawPalette[8][64][3];
	unsigned short Palette15[512];
	unsigned short Palette16[512];
	unsigned long Palette32[512];
	char Depth;
	BOOL Fullscreen, Scanlines;

	LARGE_INTEGER ClockFreq;
	LARGE_INTEGER LastClockVal;
	int FPSnum, FPSCnt, FSkip;
	BOOL aFSkip;

	int Pitch;
	int WantFPS;
	int aFPScnt, aFPSnum;

	BOOL SlowDown;
	int SlowRate;

	int PaletteNTSC, PalettePAL;
	int NTSChue, NTSCsat, PALsat;
	TCHAR CustPaletteNTSC[MAX_PATH], CustPalettePAL[MAX_PATH];

	LPDIRECTDRAW7		DirectDraw;
	LPDIRECTDRAWSURFACE7	PrimarySurf, SecondarySurf;
	LPDIRECTDRAWCLIPPER	Clipper;
	DDSURFACEDESC2		SurfDesc;
	DWORD			SurfSize;
};

extern struct tGFX GFX;

void	GFX_Init (void);
void	GFX_Create (void);
void	GFX_Release (void);
void	GFX_DrawScreen (void);
void	GFX_Draw1x (void);
void	GFX_Draw2x (void);
void	GFX_Update (void);
void	GFX_Repaint (void);
void	GFX_LoadPalette (int);
void	GFX_SetFrameskip (int);
void	GFX_PaletteConfig (void);
void	GFX_GetCursorPos (POINT *);
void	GFX_SetCursorPos (int,int);
BOOL	GFX_ZapperHit (int);

#endif /* GFX_H */