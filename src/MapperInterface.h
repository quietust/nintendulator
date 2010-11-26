/* Nintendulator Mapper Interface
 * Copyright (C) 2002-2010 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef	MAPPERINTERFACE_H
#define	MAPPERINTERFACE_H

/* Mapper Interface version (3.8) */

#ifdef	UNICODE
#define	CurrentMapperInterface	0x80030008
#else	/* !UNICODE */
#define	CurrentMapperInterface	0x00030008
#endif	/* UNICODE */

/* Function types */

#define	MAPINT	__cdecl

typedef	void	(MAPINT *FCPUWrite)	(int Bank,int Addr,int Val);
typedef	int	(MAPINT *FCPURead)	(int Bank,int Addr);

typedef	void	(MAPINT *FPPUWrite)	(int Bank,int Addr,int Val);
typedef	int	(MAPINT *FPPURead)	(int Bank,int Addr);

/* Mapper Interface Structure - Pointers to data and functions within emulator */

typedef	struct	EmulatorInterface
{
/* Functions for managing read/write handlers */
	void		(MAPINT *SetCPUReadHandler)	(int,FCPURead);
	void		(MAPINT *SetCPUWriteHandler)	(int,FCPUWrite);
	FCPURead	(MAPINT *GetCPUReadHandler)	(int);
	FCPUWrite	(MAPINT *GetCPUWriteHandler)	(int);

	void		(MAPINT *SetPPUReadHandler)	(int,FPPURead);
	void		(MAPINT *SetPPUWriteHandler)	(int,FPPUWrite);
	FPPURead	(MAPINT *GetPPUReadHandler)	(int);
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
	void		(MAPINT *DbgOut)		(TCHAR *,...);	/* Echo text to debug window */
	void		(MAPINT *StatusOut)		(TCHAR *,...);	/* Echo text on status bar */
/* Data fields */
	unsigned char *	OpenBus;			/* pointer to last value on the CPU data bus */
}	TEmulatorInterface, *PEmulatorInterface;
typedef	const	TEmulatorInterface	CTEmulatorInterface, *CPEmulatorInterface;

typedef enum	{ COMPAT_NONE, COMPAT_PARTIAL, COMPAT_NEARLY, COMPAT_FULL, COMPAT_NUMTYPES } COMPAT_TYPE;

/* Mapper Information structure - Contains pointers to mapper functions, sent to emulator on load mapper  */

typedef	enum	{ RESET_NONE, RESET_SOFT, RESET_HARD } RESET_TYPE;

typedef	enum	{ STATE_SIZE, STATE_SAVE, STATE_LOAD } STATE_TYPE;

typedef	enum	{ CFG_WINDOW, CFG_QUERY, CFG_CMD } CFG_TYPE;

typedef	struct	MapperInfo
{
/* Mapper Information */
	void *		MapperId;
	TCHAR *		Description;
	COMPAT_TYPE	Compatibility;

/* Mapper Functions */
	void		(MAPINT *Load)		(void);
	void		(MAPINT *Reset)	(RESET_TYPE);		/* ResetType */
	void		(MAPINT *Unload)	(void);
	void		(MAPINT *CPUCycle)	(void);
	void		(MAPINT *PPUCycle)	(int,int,int,int);	/* Address, Scanline, Cycle, IsRendering */
	int		(MAPINT *SaveLoad)	(STATE_TYPE,int,unsigned char *);	/* Mode, Offset, Data */
	int		(MAPINT *GenSound)	(int);			/* Cycles */
	unsigned char	(MAPINT *Config)	(CFG_TYPE,unsigned char);	/* Mode, Data */
}	TMapperInfo, *PMapperInfo;
typedef	const	TMapperInfo	CTMapperInfo, *CPMapperInfo;

/* ROM Information Structure - Contains information about the ROM currently loaded */

typedef	enum	{ ROM_UNDEFINED, ROM_INES, ROM_UNIF, ROM_FDS, ROM_NSF, ROM_NUMTYPES } ROM_TYPE;

typedef	struct	ROMInfo
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
			BYTE	INES_Version;	/* 1 for standard .NES, 2 Denotes presence of iNES 2.0 data */
			BYTE	INES2_SubMapper;	/* iNES 2.0 - submapper */
			BYTE	INES2_TVMode;	/* iNES 2.0 - NTSC/PAL indicator */
			BYTE	INES2_PRGRAM;	/* iNES 2.0 - PRG RAM counts, batteried and otherwise */
			BYTE	INES2_CHRRAM;	/* iNES 2.0 - CHR RAM counts, batteried and otherwise */
			BYTE	INES2_VSDATA;	/* iNES 2.0 - VS Unisystem information */
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
}	TROMInfo, *PROMInfo;
typedef	const	TROMInfo	CTROMInfo, *CPROMInfo;

/* DLL Information Structure - Contains general information about the mapper DLL */

typedef	struct	DLLInfo
{
	TCHAR *		Description;
	int		Date;
	int		Version;
	CPMapperInfo	(MAPINT *LoadMapper)	(CPROMInfo);
	void		(MAPINT *UnloadMapper)	(void);
}	TDLLInfo, *PDLLInfo;

typedef	PDLLInfo(MAPINT *PLoadMapperDLL)	(HWND, CPEmulatorInterface, int);
typedef	void	(MAPINT *PUnloadMapperDLL)	(void);

extern	TEmulatorInterface	EI;
extern	TROMInfo		RI;
extern	PDLLInfo		DI;
extern	CPMapperInfo		MI, MI2;

namespace MapperInterface
{
void	Init (void);
void	Release (void);
BOOL	LoadMapper (CPROMInfo ROM);
void	UnloadMapper (void);
} // namespace MapperInterface
#endif	/* !MAPPERINTERFACE_H */