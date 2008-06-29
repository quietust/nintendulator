/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2008 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef DEBUGGER_H
#define DEBUGGER_H

#ifdef ENABLE_DEBUGGER

#define DEBUG_TRACELINES 13
#define DEBUG_MEMLINES 8

struct tDebugger
{
	BOOL	Enabled;
	int	Mode;

	BOOL	NTabChanged, PalChanged, PatChanged;

	BOOL	Logging, Step;

	int	Depth;

	int	Palette, Nametable;

	HDC	PaletteDC;	/* Palette */
	HWND	PaletteWnd;
	HBITMAP	PaletteBMP;

	HDC	PatternDC;	/* Pattern tables */
	HWND	PatternWnd;
	HBITMAP	PatternBMP;

	HDC	NameDC;		/* Nametable */
	HWND	NameWnd;
	HBITMAP	NameBMP;

	HWND	CPUWnd;
	HWND	PPUWnd;

	unsigned char	BreakP[0x10000];
	int	TraceOffset;	/* -1 to center on PC, otherwise center on TraceOffset */
	int	MemOffset;

	FILE	*LogFile;
};

extern struct tDebugger Debugger;

void	Debugger_Init (void);
void	Debugger_Release (void);
void	Debugger_SetMode(int NewMode);
void	Debugger_Update (void);
void	Debugger_AddInst (void);

#endif	/* ENABLE_DEBUGGER */

#endif /* DEBUGGER_H */