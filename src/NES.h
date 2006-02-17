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

#ifndef __NES_H__
#define __NES_H__

#include "MapperInterface.h"
#include "resource.h"
#include <windows.h>

#define RESET_SOFT 1
#define RESET_HARD 2

struct tNES
{
	int SRAM_Size;

	int PRGMask, CHRMask;

	BOOL ROMLoaded;
	BOOL Stop, Stopped, NeedReset, NeedQuit, Scanline;
	BOOL IsNSF;
	BOOL GameGenie;
	BOOL SoundEnabled;
};
extern	struct tNES NES;

extern	unsigned char PRG_ROM[0x800][0x1000];	// 8192 KB
extern	unsigned char PRG_RAM[0x10][0x1000];	//   64 KB
extern	unsigned char CHR_ROM[0x1000][0x400];	// 4096 KB
extern	unsigned char CHR_RAM[0x20][0x400];	//   32 KB

void	NES_Init (void);
void	NES_Release (void);
void	NES_OpenFile (char *);
void	NES_CloseFile (void);
BOOL	NES_OpenFileiNES (char *);
BOOL	NES_OpenFileUNIF (char *);
BOOL	NES_OpenFileFDS (char *);
BOOL	NES_OpenFileNSF (char *);
BOOL	NES_OpenFileNRFF (char *);
void	NES_SetCPUMode (int);
void	NES_Reset (int);
void	NES_Run (void);
void	NES_UpdateInterface (void);
void	NES_LoadSettings (void);
void	NES_SaveSettings (void);
void	NES_Repaint (void);

#endif