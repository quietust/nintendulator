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

#ifndef PPU_H
#define PPU_H

#include "MapperInterface.h"

struct	tPPU
{
	FPPURead	ReadHandler[0x10];
	FPPUWrite	WriteHandler[0x10];
	int SLEndFrame;
	int Clockticks;
	int SLnum;
	unsigned char *CHRPointer[0x10];
	BOOL Writable[0x10];

	unsigned char Sprite[0x100];
	unsigned char SprAddr;

	unsigned char Palette[0x20];

	unsigned char ppuLatch;

	unsigned char Reg2000, Reg2001, Reg2002;
	
	unsigned char HVTog, ShortSL, IsRendering, OnScreen;

	unsigned long VRAMAddr, IntReg;
	unsigned char IntX;
	unsigned char TileData[272];

	unsigned long GrayScale;	/* ANDed with palette index (0x30 if grayscale, 0x3F if not) */
	unsigned long ColorEmphasis;	/* ORed with palette index (upper 8 bits of $2001 shifted left 1 bit) */

	unsigned long RenderAddr;
	unsigned long IOAddr;
	unsigned char IOVal;
	unsigned char IOMode;	/* Start at 6 for writes, 5 for reads - counts down and eventually hits zero */
	unsigned char buf2007;

	unsigned char SprBuff[32];
	BOOL Spr0InLine;
	int SprCount;
	unsigned char SprData[8][8];
	unsigned short *GfxData;
	unsigned char IsPAL;
};
extern	struct	tPPU	PPU;
extern	unsigned char	PPU_VRAM[0x4][0x400];
extern	unsigned char	PPU_OpenBus[0x400];
extern	unsigned short	DrawArray[256*240];

void	PPU_GetHandlers (void);
void	PPU_PowerOn (void);
void	PPU_Reset (void);
int	PPU_Save (FILE *);
int	PPU_Load (FILE *);
int	_MAPINT	PPU_IntRead (int,int);
void	_MAPINT	PPU_IntWrite (int,int,int);
void	PPU_Run (void);
void	PPU_GetGFXPtr (void);

int	_MAPINT	PPU_BusRead (int,int);
void	_MAPINT	PPU_BusWriteCHR (int,int,int);
void	_MAPINT	PPU_BusWriteNT (int,int,int);

#endif /* PPU_H */