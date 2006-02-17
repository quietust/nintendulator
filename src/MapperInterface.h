/*
 * Nintendulator Mapper Interface
 */

/* So this file only gets included once */

#ifndef MAPPERINTERFACE_H
#define MAPPERINTERFACE_H

/* Standard header files, used by all mappers */

/* Mapper Interface version (3.6) */

#define	CurrentMapperInterface 0x00030006

/* Function types */

#define	_MAPINT	__cdecl

typedef	void	(_MAPINT *FCPUWrite)	(int Bank,int Addr,int Val);
typedef	int	(_MAPINT *FCPURead)	(int Bank,int Addr);

typedef	void	(_MAPINT *FPPUWrite)	(int Bank,int Addr,int Val);
typedef	int	(_MAPINT *FPPURead)	(int Bank,int Addr);

/* Mapper Interface Structure - Pointers to data and functions within emulator */

typedef	struct	EmulatorInterface
{
/* Functions for managing read/write handlers */
	void		(_MAPINT *SetCPUReadHandler)	(int,FCPURead);
	void		(_MAPINT *SetCPUWriteHandler)	(int,FCPUWrite);
	FCPURead	(_MAPINT *GetCPUReadHandler)	(int);
	FCPUWrite	(_MAPINT *GetCPUWriteHandler)	(int);

	void		(_MAPINT *SetPPUReadHandler)	(int,FPPURead);
	void		(_MAPINT *SetPPUWriteHandler)	(int,FPPUWrite);
	FPPURead	(_MAPINT *GetPPUReadHandler)	(int);
	FPPUWrite	(_MAPINT *GetPPUWriteHandler)	(int);

/* Functions for mapping PRG */
	void		(_MAPINT *SetPRG_ROM4)		(int,int);
	void		(_MAPINT *SetPRG_ROM8)		(int,int);
	void		(_MAPINT *SetPRG_ROM16)		(int,int);
	void		(_MAPINT *SetPRG_ROM32)		(int,int);
	int		(_MAPINT *GetPRG_ROM4)		(int);		/* -1 if no ROM mapped */
	void		(_MAPINT *SetPRG_RAM4)		(int,int);
	void		(_MAPINT *SetPRG_RAM8)		(int,int);
	void		(_MAPINT *SetPRG_RAM16)		(int,int);
	void		(_MAPINT *SetPRG_RAM32)		(int,int);
	int		(_MAPINT *GetPRG_RAM4)		(int);		/* -1 if no RAM mapped */

	unsigned char *	(_MAPINT *GetPRG_Ptr4)		(int);
	void		(_MAPINT *SetPRG_Ptr4)		(int,unsigned char *,BOOL);
	void		(_MAPINT *SetPRG_OB4)		(int);		/* Open bus */

/* Functions for mapping CHR */
	void		(_MAPINT *SetCHR_ROM1)		(int,int);
	void		(_MAPINT *SetCHR_ROM2)		(int,int);
	void		(_MAPINT *SetCHR_ROM4)		(int,int);
	void		(_MAPINT *SetCHR_ROM8)		(int,int);
	int		(_MAPINT *GetCHR_ROM1)		(int);		/* -1 if no ROM mapped */

	void		(_MAPINT *SetCHR_RAM1)		(int,int);
	void		(_MAPINT *SetCHR_RAM2)		(int,int);
	void		(_MAPINT *SetCHR_RAM4)		(int,int);
	void		(_MAPINT *SetCHR_RAM8)		(int,int);
	int		(_MAPINT *GetCHR_RAM1)		(int);		/* -1 if no RAM mapped */

	void		(_MAPINT *SetCHR_NT1)		(int,int);
	int		(_MAPINT *GetCHR_NT1)		(int);		/* -1 if no nametable mapped */

	unsigned char *	(_MAPINT *GetCHR_Ptr1)		(int);
	void		(_MAPINT *SetCHR_Ptr1)		(int,unsigned char *,BOOL);
	void		(_MAPINT *SetCHR_OB1)		(int);		/* Open bus */

/* Functions for controlling mirroring */
	void		(_MAPINT *Mirror_H)		(void);
	void		(_MAPINT *Mirror_V)		(void);
	void		(_MAPINT *Mirror_4)		(void);
	void		(_MAPINT *Mirror_S0)		(void);
	void		(_MAPINT *Mirror_S1)		(void);
	void		(_MAPINT *Mirror_Custom)	(int,int,int,int);

/* IRQ */
	void		(_MAPINT *SetIRQ)		(int);		/* Sets the state of the /IRQ line */

/* Save RAM Handling */
	void		(_MAPINT *Set_SRAMSize)		(int);		/* Sets the size of the SRAM (in bytes) */

/* Misc Callbacks */
	void		(_MAPINT *DbgOut)		(char *,...);	/* Echo text to debug window */
	void		(_MAPINT *StatusOut)		(char *,...);	/* Echo text on status bar */
/* Data fields */
	unsigned char *	OpenBus;			/* pointer to last value on the CPU data bus */
	
}	TEmulatorInterface, *PEmulatorInterface;
typedef	const	TEmulatorInterface	CTEmulatorInterface, *CPEmulatorInterface;

typedef enum	{ COMPAT_FULL, COMPAT_NEARLY, COMPAT_PARTIAL, COMPAT_NONE } COMPAT_TYPE;

/* Mapper Information structure - Contains pointers to mapper functions, sent to emulator on load mapper  */

typedef	enum	{ RESET_NONE, RESET_SOFT, RESET_HARD, RESET_FULL } RESET_TYPE;

typedef	enum	{ STATE_SAVE, STATE_LOAD, STATE_SIZE } STATE_TYPE;

typedef	enum	{ CFG_WINDOW, CFG_QUERY, CFG_CMD } CFG_TYPE;

typedef	struct	MapperInfo
{
/* Mapper Information */
	void *		MapperId;
	char *		Description;
	COMPAT_TYPE	Compatibility;

/* Mapper Functions */
	void		(_MAPINT *Load)		(void);
	void		(_MAPINT *Reset)	(RESET_TYPE);
	void		(_MAPINT *Unload)	(void);
	void		(_MAPINT *CPUCycle)	(void);
	void		(_MAPINT *PPUCycle)	(int,int,int,int);
	int		(_MAPINT *SaveLoad)	(STATE_TYPE,int,unsigned char *);
	int		(_MAPINT *GenSound)	(int);
	unsigned char	(_MAPINT *Config)	(CFG_TYPE,unsigned char);
}	TMapperInfo, *PMapperInfo;
typedef	const	TMapperInfo	CTMapperInfo, *CPMapperInfo;

/* DLL Information Structure:- Contains general information about the mapper DLL */

typedef	enum	{ ROM_INES, ROM_UNIF, ROM_FDS, ROM_NSF, ROM_UNDEFINED } ROM_TYPE;

typedef	struct	ROMInfo
{
	char *		Filename;
	ROM_TYPE	ROMType;
	union
	{
		struct
		{
			BYTE	INES_MapperNum;
			BYTE	INES_Flags;	/* byte 6 flags in lower 4 bits, byte 7 flags in upper 4 bits */
			BYTE	INES_PRGSize;	/* number of 16KB banks */
			BYTE	INES_CHRSize;	/* number of 8KB banks */
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

/* DLL Information Structure:- Contains general information about the mapper DLL */

typedef	struct	DLLInfo
{
	char *		Author;
	int		Date;
	int		Version;
	CPMapperInfo	(_MAPINT *LoadMapper)	(CPROMInfo);
	void		(_MAPINT *UnloadMapper)	(void);
}	TDLLInfo, *PDLLInfo;

typedef	PDLLInfo(_MAPINT *PLoadMapperDLL)	(HWND, CPEmulatorInterface, int);
typedef	void	(_MAPINT *PUnloadMapperDLL)	(void);

extern	TEmulatorInterface	EI;
extern	TROMInfo		RI;
extern	PDLLInfo		DI;
extern	CPMapperInfo		MI, MI2;

void	MapperInterface_Init (void);
void	MapperInterface_Release (void);
BOOL	MapperInterface_LoadMapper (CPROMInfo ROM);
void	MapperInterface_UnloadMapper (void);

#endif /* MAPPERINTERFACE_H */