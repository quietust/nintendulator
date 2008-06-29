/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2008 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef PPU_H
#define PPU_H

/* #define	ACCURATE_SPRITES	/* enable cycle-accurate sprite evaluation logic */

struct	tPPU
{
	FPPURead	ReadHandler[0x10];
	FPPUWrite	WriteHandler[0x10];
	int SLEndFrame;
	int Clockticks;
	int SLnum;
	unsigned char *CHRPointer[0x10];
	BOOL Writable[0x10];

#ifdef	ACCURATE_SPRITES
	unsigned char Sprite[0x120];
#else
	unsigned char Sprite[0x100];
#endif
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

#ifdef	ACCURATE_SPRITES
	unsigned char *SprBuff;
#else
	unsigned char SprBuff[32];
#endif
	BOOL Spr0InLine;
	int SprCount;
#ifdef	ACCURATE_SPRITES
	unsigned char SprData[8][10];
#else
	unsigned char SprData[8][8];
#endif
	unsigned short *GfxData;
	BOOL IsPAL;
	unsigned char PALsubticks;
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
int	MAPINT	PPU_IntRead (int,int);
void	MAPINT	PPU_IntWrite (int,int,int);
void	PPU_Run (void);
void	PPU_GetGFXPtr (void);

int	MAPINT	PPU_BusRead (int,int);
void	MAPINT	PPU_BusWriteCHR (int,int,int);
void	MAPINT	PPU_BusWriteNT (int,int,int);

#endif /* PPU_H */