/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * $URL$
 * $Id$
 */

#pragma once

namespace PPU
{
extern FPPURead	ReadHandler[0x10];
extern FPPUWrite	WriteHandler[0x10];
extern int Clockticks;
extern int SLnum;
extern unsigned char *CHRPointer[0x10];
extern BOOL Writable[0x10];

#ifdef	ACCURATE_SPRITES
extern unsigned char Sprite[0x120];
#else	/* !ACCURATE_SPRITES */
extern unsigned char Sprite[0x100];
#endif	/* ACCURATE_SPRITES */

extern unsigned char Palette[0x20];

extern unsigned char Reg2000;
	
extern unsigned char IsRendering, OnScreen;

extern unsigned long VRAMAddr;

extern BOOL IsPAL;
extern unsigned char	VRAM[0x4][0x400];
extern unsigned char	OpenBus[0x400];
extern unsigned short	DrawArray[256*240];

extern unsigned char VsSecurity;

void	GetHandlers (void);
void	SetRegion (void);
void	PowerOn (void);
void	Reset (void);
int	Save (FILE *);
int	Load (FILE *);
int	MAPINT	IntRead (int, int);
int	MAPINT	IntReadVs (int, int);
void	MAPINT	IntWrite (int, int, int);
void	MAPINT	IntWriteVs (int, int, int);
void	Run (void);
void	GetGFXPtr (void);

int	MAPINT	BusRead (int, int);
void	MAPINT	BusWriteCHR (int, int, int);
void	MAPINT	BusWriteNT (int, int, int);
} // namespace PPU
