/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * $URL$
 * $Id$
 */

#pragma once

namespace NES
{
extern int SRAM_Size;

extern int PRGSizeROM, PRGSizeRAM, CHRSizeROM, CHRSizeRAM;
extern int PRGMaskROM, PRGMaskRAM, CHRMaskROM, CHRMaskRAM;

extern BOOL ROMLoaded;
extern BOOL DoStop, Running, Scanline;
extern BOOL GameGenie;
extern BOOL SoundEnabled;
extern BOOL AutoRun;
extern BOOL FrameStep, GotStep;
extern BOOL HasMenu;

enum Region { REGION_NONE, REGION_NTSC, REGION_PAL, REGION_DENDY };
extern Region CurRegion;

// Maximum supported data sizes, since it's far easier than dynamically allocating them

// 8192KB PRG ROM
#define	MAX_PRGROM_SIZE	0x800
#define	MAX_PRGROM_MASK	(MAX_PRGROM_SIZE - 1)

// 1024KB PRG RAM
#define	MAX_PRGRAM_SIZE	0x100
#define	MAX_PRGRAM_MASK	(MAX_PRGRAM_SIZE - 1)

// 4096KB CHR ROM
#define	MAX_CHRROM_SIZE	0x1000
#define	MAX_CHRROM_MASK	(MAX_CHRROM_SIZE - 1)

// 256KB CHR RAM
#define	MAX_CHRRAM_SIZE	0x100
#define	MAX_CHRRAM_MASK	(MAX_CHRRAM_SIZE - 1)

extern unsigned char PRG_ROM[MAX_PRGROM_SIZE][0x1000];
extern unsigned char PRG_RAM[MAX_PRGRAM_SIZE][0x1000];
extern unsigned char CHR_ROM[MAX_CHRROM_SIZE][0x400];
extern unsigned char CHR_RAM[MAX_CHRRAM_SIZE][0x400];

void	Init (void);
void	Destroy (void);
void	OpenFile (TCHAR *);
void	CloseFile (void);
int	FDSSave (FILE *);
int	FDSLoad (FILE *, int ver);
void	SaveSRAM (void);
void	LoadSRAM (void);
DWORD	getMask (unsigned int);
const TCHAR *	OpenFileiNES (FILE *);
const TCHAR *	OpenFileUNIF (FILE *);
const TCHAR *	OpenFileFDS (FILE *);
const TCHAR *	OpenFileNSF (FILE *);
void	SetRegion (Region);
void	InitHandlers (void);
void	Reset (RESET_TYPE);

void	Start (BOOL);
void	Stop (void);

void	UpdateInterface (void);
void	LoadSettings (void);
void	SaveSettings (void);
void	SetupDataPath (void);
void	MapperConfig (void);
} // namespace NES
