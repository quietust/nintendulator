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