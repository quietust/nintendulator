/*
Nintendulator - A Win32 NES emulator written in C.
Designed for maximum emulation accuracy.
Copyright (c) 2002  Quietust

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

#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#ifdef ENABLE_DEBUGGER

struct tDebugger
{
	BOOL	Enabled;
	int	Mode;

	BOOL	NTabChanged, PalChanged, PatChanged;

	BOOL	Logging;

	int	FontWidth, FontHeight;

	int	Depth;

	int	PatPalBase;

	//Palette
	HDC	PaletteWDC, PaletteDC;
	//Pattern Tables
	HDC	PatternWDC, PatternDC;
	//Name Tables
	HDC	NameWDC, NameDC;
	//Trace Window
	HDC	TraceWDC, TraceDC;
	//Registers
	HDC	RegWDC, RegDC;

	int	AddyLine[64];
	BOOL	BreakP[0x10000];
	int	TraceOffset;		//-1 to center on PC, otherwise center on TraceOffset

	HWND	DumpWnd;
	FILE	*LogFile;
	char	Out1[50], Out2[50];
	//Palette
	HWND	PaletteWnd;		//Window
	HBITMAP	PaletteBMP;							//Off-Screen Window
	unsigned char PaletteArray[(256*32)*4];		//  "
	//Pattern Tables
	HWND	PatternWnd;		//Window
	HBITMAP	PatternBMP;							//Off-Screen Window
	unsigned char PatternArray[(256*128)*4];		//  "
	//Name Tables
	HWND	NameWnd;		//Window
	HBITMAP	NameBMP;							//Off-Screen Window
	//Trace Window
	unsigned char NameArray[(512*480)*4];		//  "
	HWND	TraceWnd;		//Window
	HBITMAP	TraceBMP;							//Off-Screen Window
	//Registers
	HWND	RegWnd;			//Window
	HBITMAP	RegBMP;							//Off-Screen Window
};

extern struct tDebugger Debugger;

void	Debugger_Init (void);
void	Debugger_Release (void);
void	Debugger_SetMode(int NewMode);
void	Debugger_Update (void);
void	Debugger_UpdateGraphics (void);
void	Debugger_AddInst (void);

void	Debugger_DPrint (char * A, int B);
void	Debugger_StartLogging (void);
void	Debugger_StopLogging (void);
void	Debugger_DrawTraceLine (unsigned short Addy, short y);
unsigned char	Debugger_TraceMem (unsigned short Addy);

#endif	/* ENABLE_DEBUGGER */

#endif