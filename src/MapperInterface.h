/* Nintendulator Mapper Interface
 * Copyright (C) 2002-2017 QMT Productions
 *
 * $URL$
 * $Id$
 */

#pragma once

/* Mapper Interface version (3.9) */

#ifdef	UNICODE
#define	CurrentMapperInterface	0x80030009
#else	/* !UNICODE */
#define	CurrentMapperInterface	0x00030009
#endif	/* UNICODE */

/* Function types */

#define	MAPINT	__cdecl

typedef	void	(MAPINT *FCPUWrite)	(int Bank,int Addr,int Val);
typedef	int	(MAPINT *FCPURead)	(int Bank,int Addr);

typedef	void	(MAPINT *FPPUWrite)	(int Bank,int Addr,int Val);
typedef	int	(MAPINT *FPPURead)	(int Bank,int Addr);

/* Mapper Interface Structure - Pointers to data and functions within emulator */

struct	EmulatorInterface
{
/* Functions for managing read/write handlers */
	void		(MAPINT *SetCPUReadHandler)	(int,FCPURead);
	FCPURead	(MAPINT *GetCPUReadHandler)	(int);

	void		(MAPINT *SetCPUReadHandlerDebug)(int,FCPURead);
	FCPURead	(MAPINT *GetCPUReadHandlerDebug)(int);

	void		(MAPINT *SetCPUWriteHandler)	(int,FCPUWrite);
	FCPUWrite	(MAPINT *GetCPUWriteHandler)	(int);

	void		(MAPINT *SetPPUReadHandler)	(int,FPPURead);
	FPPURead	(MAPINT *GetPPUReadHandler)	(int);

	void		(MAPINT *SetPPUReadHandlerDebug)(int,FPPURead);
	FPPURead	(MAPINT *GetPPUReadHandlerDebug)(int);

	void		(MAPINT *SetPPUWriteHandler)	(int,FPPUWrite);
	FPPUWrite	(MAPINT *GetPPUWriteHandler)	(int);

/* Functions for mapping PRG */
	void		(MAPINT *SetPRG_ROM4)		(int,int);
	void		(MAPINT *SetPRG_ROM8)		(int,int);
	void		(MAPINT *SetPRG_ROM16)		(int,int);
	void		(MAPINT *SetPRG_ROM32)		(int,int);
	int		(MAPINT *GetPRG_ROM4)		(int);		/* -1 if no ROM mapped */

	void		(MAPINT *SetPRG_RAM4)		(int,int);
	void		(MAPINT *SetPRG_RAM8)		(int,int);
	void		(MAPINT *SetPRG_RAM16)		(int,int);
	void		(MAPINT *SetPRG_RAM32)		(int,int);
	int		(MAPINT *GetPRG_RAM4)		(int);		/* -1 if no RAM mapped */

	unsigned char *	(MAPINT *GetPRG_Ptr4)		(int);
	void		(MAPINT *SetPRG_Ptr4)		(int,unsigned char *,BOOL);
	void		(MAPINT *SetPRG_OB4)		(int);		/* Open bus */

/* Functions for mapping CHR */
	void		(MAPINT *SetCHR_ROM1)		(int,int);
	void		(MAPINT *SetCHR_ROM2)		(int,int);
	void		(MAPINT *SetCHR_ROM4)		(int,int);
	void		(MAPINT *SetCHR_ROM8)		(int,int);
	int		(MAPINT *GetCHR_ROM1)		(int);		/* -1 if no ROM mapped */

	void		(MAPINT *SetCHR_RAM1)		(int,int);
	void		(MAPINT *SetCHR_RAM2)		(int,int);
	void		(MAPINT *SetCHR_RAM4)		(int,int);
	void		(MAPINT *SetCHR_RAM8)		(int,int);
	int		(MAPINT *GetCHR_RAM1)		(int);		/* -1 if no RAM mapped */

	void		(MAPINT *SetCHR_NT1)		(int,int);
	int		(MAPINT *GetCHR_NT1)		(int);		/* -1 if no nametable mapped */

	unsigned char *	(MAPINT *GetCHR_Ptr1)		(int);
	void		(MAPINT *SetCHR_Ptr1)		(int,unsigned char *,BOOL);
	void		(MAPINT *SetCHR_OB1)		(int);		/* Open bus */

/* Functions for controlling mirroring */
	void		(MAPINT *Mirror_H)		(void);
	void		(MAPINT *Mirror_V)		(void);
	void		(MAPINT *Mirror_4)		(void);
	void		(MAPINT *Mirror_S0)		(void);
	void		(MAPINT *Mirror_S1)		(void);
	void		(MAPINT *Mirror_Custom)	(int,int,int,int);

/* IRQ */
	void		(MAPINT *SetIRQ)		(int);		/* Sets the state of the /IRQ line */

/* Save RAM Handling */
	void		(MAPINT *Set_SRAMSize)		(int);		/* Sets the size of the SRAM (in bytes) */

/* Misc Callbacks */
	void		(MAPINT *DbgOut)		(const TCHAR *,...);	/* Echo text to debug window */
	void		(MAPINT *StatusOut)		(const TCHAR *,...);	/* Echo text on status bar */
/* Data fields */
	unsigned char *	OpenBus;			/* pointer to last value on the CPU data bus */
};

enum COMPAT_TYPE	{ COMPAT_NONE, COMPAT_PARTIAL, COMPAT_NEARLY, COMPAT_FULL, COMPAT_NUMTYPES };

/* Mapper Information structure - Contains pointers to mapper functions, sent to emulator on load mapper  */

enum RESET_TYPE	{ RESET_NONE, RESET_SOFT, RESET_HARD };

enum STATE_TYPE	{ STATE_SIZE, STATE_SAVE, STATE_LOAD };

enum CFG_TYPE	{ CFG_WINDOW, CFG_QUERY, CFG_CMD };

struct	MapperInfo
{
/* Mapper Information */
	void *		MapperId;
	TCHAR *		Description;
	COMPAT_TYPE	Compatibility;

/* Mapper Functions */
	BOOL		(MAPINT *Load)		(void);
	void		(MAPINT *Reset)	(RESET_TYPE);		/* ResetType */
	void		(MAPINT *Unload)	(void);
	void		(MAPINT *CPUCycle)	(void);
	void		(MAPINT *PPUCycle)	(int,int,int,int);	/* Address, Scanline, Cycle, IsRendering */
	int		(MAPINT *SaveLoad)	(STATE_TYPE,int,unsigned char *);	/* Mode, Offset, Data */
	int		(MAPINT *GenSound)	(int);			/* Cycles */
	unsigned char	(MAPINT *Config)	(CFG_TYPE,unsigned char);	/* Mode, Data */
};

/* ROM Information Structure - Contains information about the ROM currently loaded */

enum ROM_TYPE	{ ROM_UNDEFINED, ROM_INES, ROM_UNIF, ROM_FDS, ROM_NSF, ROM_NUMTYPES };

struct	ROMInfo
{
	TCHAR *		Filename;
	ROM_TYPE	ROMType;
	union
	{
		struct
		{
			WORD	INES_MapperNum;
			BYTE	INES_Flags;	/* byte 6 flags in lower 4 bits, byte 7 flags in upper 4 bits */
			WORD	INES_PRGSize;	/* number of 16KB banks */
			WORD	INES_CHRSize;	/* number of 8KB banks */
			BYTE	INES_Version;	/* 1 for standard .NES, 2 Denotes presence of NES 2.0 data */
			BYTE	INES2_SubMapper;	/* NES 2.0 - submapper */
			BYTE	INES2_TVMode;	/* NES 2.0 - NTSC/PAL indicator */
			BYTE	INES2_PRGRAM;	/* NES 2.0 - PRG RAM counts, batteried and otherwise */
			BYTE	INES2_CHRRAM;	/* NES 2.0 - CHR RAM counts, batteried and otherwise */
			BYTE	INES2_VSDATA;	/* NES 2.0 - VS Unisystem information */
		};	/* INES */
		struct
		{
			char *	UNIF_BoardName;
			BYTE	UNIF_Mirroring;
			BYTE	UNIF_NTSCPAL;	/* 0 for NTSC, 1 for PAL, 2 for either */
			BOOL	UNIF_Battery;
			BYTE	UNIF_NumPRG;	/* number of PRG# blocks */
			BYTE	UNIF_NumCHR;	/* number of CHR# blocks */
			DWORD	UNIF_PRGSize[16];	/* size of each PRG block, in bytes */
			DWORD	UNIF_CHRSize[16];	/* size of each CHR block, in bytes */
		};	/* UNIF */
		struct
		{
			BYTE	FDS_NumSides;
		};	/* FDS */
		struct
		{
			BYTE	NSF_NumSongs;
			BYTE	NSF_InitSong;
			WORD	NSF_InitAddr;
			WORD	NSF_PlayAddr;
			char *	NSF_Title;
			char *	NSF_Artist;
			char *	NSF_Copyright;
			WORD	NSF_NTSCSpeed;	/* Number of microseconds between PLAY calls for NTSC */
			WORD	NSF_PALSpeed;	/* Number of microseconds between PLAY calls for PAL */
			BYTE	NSF_NTSCPAL;	/* 0 for NTSC, 1 for PAL, 2 for either */
			BYTE	NSF_SoundChips;
			BYTE	NSF_InitBanks[8];
			DWORD	NSF_DataSize;	/* total amount of (PRG) data present */
		};	/* NSF */
		struct
		{
			BYTE	reserved[256];
		};	/* reserved for additional file types */
	};
};

/* DLL Information Structure - Contains general information about the mapper DLL */

struct	DLLInfo
{
	TCHAR *		Description;
	int		Date;
	int		Version;
	const MapperInfo *(MAPINT *LoadMapper)	(const ROMInfo *);
	void		(MAPINT *UnloadMapper)	(void);
};

typedef	DLLInfo *(MAPINT *FLoadMapperDLL)	(HWND, const EmulatorInterface *, int);
typedef	void	(MAPINT *FUnloadMapperDLL)	(void);

extern	EmulatorInterface	EI;
extern	ROMInfo		RI;
extern	DLLInfo		*DI;
extern	const MapperInfo	*MI, *MI2;

namespace MapperInterface
{
extern const TCHAR *CompatLevel[COMPAT_NUMTYPES];

void	Init (void);
void	Destroy (void);
BOOL	LoadMapper (const ROMInfo *ROM);
void	UnloadMapper (void);
} // namespace MapperInterface
