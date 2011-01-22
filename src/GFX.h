/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef GFX_H
#define GFX_H

#define	DIRECTDRAW_VERSION 0x0700
#include <ddraw.h>

namespace GFX
{
enum PALETTE { PALETTE_NTSC, PALETTE_PAL, PALETTE_PC10, PALETTE_VS1, PALETTE_VS2, PALETTE_VS3, PALETTE_VS4, PALETTE_EXT, PALETTE_MAX };

extern unsigned char RawPalette[8][64][3];
extern unsigned long Palette32[512];
extern BOOL Fullscreen, Scanlines;

extern int FPSnum, FPSCnt, FSkip;
extern BOOL aFSkip;

extern int WantFPS;

extern BOOL SlowDown;
extern int SlowRate;

extern PALETTE PaletteNTSC, PalettePAL;
extern int NTSChue, NTSCsat, PALsat;
extern TCHAR CustPaletteNTSC[MAX_PATH], CustPalettePAL[MAX_PATH];

extern LPDIRECTDRAW7 DirectDraw;

void	Init (void);
void	Create (void);
void	Release (void);
void	DrawScreen (void);
void	Draw1x (void);
void	Draw2x (void);
void	Update (void);
void	Repaint (void);
void	LoadPalette (PALETTE);
void	SetFrameskip (int);
void	PaletteConfig (void);
void	GetCursorPos (POINT *);
void	SetCursorPos (int, int);
BOOL	ZapperHit (int);
} // namespace GFX
#endif	/* !GFX_H */
