/*
 * NESten Mapper Interface v2.1
 */

#ifndef __MAPPERINTERFACE_H__
#define __MAPPERINTERFACE_H__

/* Mapper Interface version (2.1) */

#define CurrentMapperInterface 0x00020001

/* Function types */

typedef	void	(__cdecl *PWriteFunc)	(int,int,int);
typedef	int	(__cdecl *PReadFunc)	(int,int);

/* The Mapper Interface Structure - Pointers to data and functions within the emulator */

typedef	struct	MapperParam
{
	/* .NES Header information */
	int		Flags;
	int		PRG_ROM_Size;
	int		CHR_ROM_Size;
	int		Unused;

	/* Functions for managing read/write handlers */
	void		(__cdecl *SetWriteHandler)	(int,PWriteFunc);
	void		(__cdecl *SetReadHandler)	(int,PReadFunc);
	PWriteFunc	(__cdecl *GetWriteHandler)	(int);
	PReadFunc	(__cdecl *GetReadHandler)	(int);

	/* Functions for mapping PRG */
	void		(__cdecl *SetPRG_ROM4)		(int,int);
	void		(__cdecl *SetPRG_ROM8)		(int,int);
	void		(__cdecl *SetPRG_ROM16)		(int,int);
	void		(__cdecl *SetPRG_ROM32)		(int,int);
	int		(__cdecl *GetPRG_ROM4)		(int);		/* -1 if no ROM mapped */
	void		(__cdecl *SetPRG_RAM4)		(int,int);
	void		(__cdecl *SetPRG_RAM8)		(int,int);
	void		(__cdecl *SetPRG_RAM16)		(int,int);
	void		(__cdecl *SetPRG_RAM32)		(int,int);
	int		(__cdecl *GetPRG_RAM4)		(int);		/* -1 if no RAM mapped */
	unsigned char *	(__cdecl *GetPRG_Ptr4)		(int);
		
	void		(__cdecl *SetPRG_OB4)		(int);		/* Open bus */

	/* Functions for mapping CHR */
	void 		(__cdecl *SetCHR_ROM1)		(int,int);
	void		(__cdecl *SetCHR_ROM2)		(int,int);
	void		(__cdecl *SetCHR_ROM4)		(int,int);
	void		(__cdecl *SetCHR_ROM8)		(int,int);
	int		(__cdecl *GetCHR_ROM1)		(int);		/* -1 if no ROM mapped */

	void		(__cdecl *SetCHR_RAM1)		(int,int);
	void		(__cdecl *SetCHR_RAM2)		(int,int);
	void		(__cdecl *SetCHR_RAM4)		(int,int);
	void		(__cdecl *SetCHR_RAM8)		(int,int);
	int		(__cdecl *GetCHR_RAM1)		(int);		/* -1 if no RAM mapped */
		
	unsigned char *	(__cdecl *GetCHR_Ptr1)		(int);
/*	void		(__cdecl *SetCHR_OB1)		(int);		/* Open bus */

	/* Functions for controlling mirroring */
	void		(__cdecl *Mirror_H)		(void);
	void		(__cdecl *Mirror_V)		(void);
	void		(__cdecl *Mirror_4)		(void);
	void		(__cdecl *Mirror_S0)		(void);
	void		(__cdecl *Mirror_S1)		(void);
	void		(__cdecl *Mirror_Custom)	(int,int,int,int);

	/* IRQ */
	void		(__cdecl *IRQ)			(void);
	/* Save RAM Handling */

	void		(__cdecl *Set_SRAMSize)		(int);		/* Sets the size of the SRAM (in bytes) */
	void		(__cdecl *Save_SRAM)		(void);		/* Saves SRAM to disk */
	void		(__cdecl *Load_SRAM)		(void);		/* Loads SRAM from disk */

	/* Misc Callbacks */
	void		(__cdecl *DbgOut)		(char *);	/* Echo text to debug window */
	void		(__cdecl *StatusOut)		(char *);	/* Echo text on status bar */
	int		(__cdecl *GetMenuRoot)		(void);		/* Gets root menu */
	int		(__cdecl *AddMenuItem)		(int,char *,int,int,int,int,int);

	/* Special Chip functions */
	/* MMC5: */
	void		(__cdecl *MMC5_UpdateAttributeCache)
							(void);		/* Update attribute cache for ExRAM gfx mode */
}	TMapperParam, *PMapperParam;

typedef	unsigned char	Ar128[128];

#define		MS_None		0
#define		MS_Partial	1
#define		MS_Nearly	2
#define		MS_Full		3

#define		MENU_NOCHECK	0
#define		MENU_UNCHECKED	1
#define		MENU_CHECKED	2

/* Mapper Information structure - Contains pointers to mapper functions, sent to the emulator on load mapper  */

typedef struct	MapperInfo
{
	/* Mapper Information */
	char *		BoardName;
	int		MapperNum;
	int		MapperSupport;
	int		BankSize;

	/* Mapper Functions */
	void		(__cdecl *InitMapper)	(const PMapperParam,int);
	void		(__cdecl *UnloadMapper)	(void);
	void		(__cdecl *HBlank)	(int,int);
	int		(__cdecl *TileHandler)	(int,int,int);
	void		(__cdecl *SaveMI)	(Ar128);
	void		(__cdecl *LoadMI)	(const Ar128);
	void		(__cdecl *MapperSnd)	(short *,int);
	void		(__cdecl *MenuClick)	(int,int,int,int);
}	TMapperInfo, *PMapperInfo;

/* DLL Information Structure:- Contains general information about the mapper DLL */

typedef	struct	DLLInfo
{
	char *		Author;
	int		Date;
	int		Version;
	PMapperInfo	(__cdecl *LoadMapper)	(int);
	PMapperInfo	(__cdecl *LoadBoard)	(char *);
}	TDLLInfo, *PDLLInfo;

typedef	PDLLInfo(__cdecl *PLoad_DLL)	(int);
typedef	void	(__cdecl *PUnload_DLL)	(void);

extern	TMapperParam MP;
extern	PMapperInfo	MI, MI2;

void	MapperInterface_Init (void);
void	MapperInterface_Release (void);
BOOL	MapperInterface_LoadByNum (int MapperNum);
BOOL	MapperInterface_LoadByBoard (char *Board);

#endif		/* __MAPPERINTERFACE_H__ */
