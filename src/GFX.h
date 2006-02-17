/*
Nintendulator - A Win32 NES emulator written in C.
Designed for maximum emulation accuracy.
Copyright (c) 2002-2006  QMT Productions

Based on NinthStar, a portable Win32 NES Emulator written in C++
Copyright (C) 2000  David de Regt

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

For a copy of the GNU General Public License, go to:
http://www.gnu.org/copyleft/gpl.html#SEC1
*/

#ifndef GFX_H
#define GFX_H

#define	DIRECTDRAW_VERSION 0x0700
#include <ddraw.h>

struct tGFX
{
	unsigned short Palette15[512];
	unsigned short Palette16[512];
	unsigned long Palette32[512];
	char Depth;
	BOOL Fullscreen;

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
	int NTSChue, NTSCsat;
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
void	GFX_Update (void);
void	GFX_Repaint (void);
void	GFX_LoadPalette (int);
void	GFX_SetFrameskip (void);
void	GFX_PaletteConfig (void);

#endif /* GFX_H */