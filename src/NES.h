/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2008 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef NES_H
#define NES_H

struct tNES
{
	int SRAM_Size;

	int PRGMask, CHRMask;

	BOOL ROMLoaded;
	BOOL Stop, Running, Scanline;
	BOOL GameGenie;
	BOOL SoundEnabled;
	BOOL AutoRun;
	BOOL FrameStep, GotStep;
	BOOL HasMenu;
};
extern	struct tNES NES;

#define	MAX_PRGROM_MASK	0x7FF
#define	MAX_PRGRAM_MASK	0xF
#define	MAX_CHRROM_MASK	0xFFF
#define	MAX_CHRRAM_MASK	0x1F
extern	unsigned char PRG_ROM[MAX_PRGROM_MASK+1][0x1000];	/* 8192 KB */
extern	unsigned char PRG_RAM[MAX_PRGRAM_MASK+1][0x1000];	/*   64 KB */
extern	unsigned char CHR_ROM[MAX_CHRROM_MASK+1][0x400];	/* 4096 KB */
extern	unsigned char CHR_RAM[MAX_CHRRAM_MASK+1][0x400];	/*   32 KB */

void	NES_Init (void);
void	NES_Release (void);
void	NES_OpenFile (TCHAR *);
void	NES_CloseFile (void);
int	NES_FDSSave (FILE *);
int	NES_FDSLoad (FILE *);
void	NES_SaveSRAM (void);
void	NES_LoadSRAM (void);
const TCHAR *	NES_OpenFileiNES (TCHAR *);
const TCHAR *	NES_OpenFileUNIF (TCHAR *);
const TCHAR *	NES_OpenFileFDS (TCHAR *);
const TCHAR *	NES_OpenFileNSF (TCHAR *);
void	NES_SetCPUMode (int);
void	NES_Reset (RESET_TYPE);

void	NES_Start (BOOL);
void	NES_Stop (void);

void	NES_UpdateInterface (void);
void	NES_LoadSettings (void);
void	NES_SaveSettings (void);
void	NES_SetupDataPath (void);
void	NES_MapperConfig (void);

#endif	/* !NES_H */
