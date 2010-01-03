/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2010 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef NES_H
#define NES_H

namespace NES
{
extern int SRAM_Size;

extern int PRGMask, CHRMask;

extern BOOL ROMLoaded;
extern BOOL DoStop, Running, Scanline;
extern BOOL GameGenie;
extern BOOL SoundEnabled;
extern BOOL AutoRun;
extern BOOL FrameStep, GotStep;
extern BOOL HasMenu;

#define	MAX_PRGROM_MASK	0x7FF
#define	MAX_PRGRAM_MASK	0xF
#define	MAX_CHRROM_MASK	0xFFF
#define	MAX_CHRRAM_MASK	0x1F
extern unsigned char PRG_ROM[MAX_PRGROM_MASK+1][0x1000];	/* 8192 KB */
extern unsigned char PRG_RAM[MAX_PRGRAM_MASK+1][0x1000];	/*   64 KB */
extern unsigned char CHR_ROM[MAX_CHRROM_MASK+1][0x400];	/* 4096 KB */
extern unsigned char CHR_RAM[MAX_CHRRAM_MASK+1][0x400];	/*   32 KB */

void	Init (void);
void	Release (void);
void	OpenFile (TCHAR *);
void	CloseFile (void);
int	FDSSave (FILE *);
int	FDSLoad (FILE *);
void	SaveSRAM (void);
void	LoadSRAM (void);
const TCHAR *	OpenFileiNES (FILE *);
const TCHAR *	OpenFileUNIF (FILE *);
const TCHAR *	OpenFileFDS (FILE *);
const TCHAR *	OpenFileNSF (FILE *);
void	SetCPUMode (int);
void	Reset (RESET_TYPE);

void	Start (BOOL);
void	Stop (void);

void	UpdateInterface (void);
void	LoadSettings (void);
void	SaveSettings (void);
void	SetupDataPath (void);
void	MapperConfig (void);

} // namespace NES
#endif	/* !NES_H */
