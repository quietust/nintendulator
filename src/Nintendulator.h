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

#ifndef NINTENDULATOR_H
#define NINTENDULATOR_H

#define	ENABLE_DEBUGGER
/*#define	CPU_BENCHMARK */

extern	HINSTANCE	hInst;	/* current instance */
extern	HWND		mWnd;	/* main window */
extern	HACCEL		hAccelTable;	/* accelerators */

extern	BOOL		MaskKeyboard;	/* mask keyboard accelerators (for when Family Basic Keyboard is active) */
extern	HWND		hDebug;		/* Debug Info window */
extern	BOOL		dbgVisible;	/* whether or not the Debug window is open */

extern	int		SizeMult;	/* window size multiplier */
extern	char		ProgPath[MAX_PATH];	/* program path */

extern	char		Path_ROM[MAX_PATH];
extern	char		Path_NMV[MAX_PATH];
extern	char		Path_AVI[MAX_PATH];
extern	char		Path_PAL[MAX_PATH];

extern	void		SetWindowClientArea (HWND hWnd, int w, int h);
extern	BOOL		ProcessMessages	(void);

extern	char		TitlebarBuffer[256];
extern	int		TitlebarDelay;
extern	void		UpdateTitlebar (void);
extern	void	__cdecl	PrintTitlebar (char *Text, ...);


#endif /* NINTENDULATOR_H */