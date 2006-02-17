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

#include "Nintendulator.h"
#include "NES.h"
#include "CPU.h"
#include "PPU.h"
#include "GFX.h"
#include "Debugger.h"
#include "States.h"

TMapperParam	MP;
PMapperInfo	MI, MI2;

struct	tMapperDLL
{
	HINSTANCE		dInst;
	PLoad_DLL		LoadDLL;
	PDLLInfo		DI;
	PUnload_DLL		UnloadDLL;
	struct	tMapperDLL *	Next;
} *MapperDLLs = NULL;
/*
typedef	struct	tMenuItem
{
	HMENU			hMenu;
	char *			Caption;
	int			State;
	int			Cmd;
	int			Parm1;
	int			Parm2;
	int			Parm3;
	struct tMenuItem *	Child;
	struct tMenuItem *	Next;
}	TMenuItem, *PMenuItem;

TMenuItem MenuRoot = {0,"Game",MENU_NOCHECK,-1,-1,-1,-1,NULL,NULL};
*/
//******************************************************************************

static	void	__cdecl	SetWriteHandler (int Page, PWriteFunc New)
{
	CPU.WriteHandler[Page] = New;
}
static	void	__cdecl	SetReadHandler (int Page, PReadFunc New)
{
	CPU.ReadHandler[Page] = New;
}
static	PWriteFunc	__cdecl	GetWriteHandler (int Page)
{
	return CPU.WriteHandler[Page];
}
static	PReadFunc	__cdecl	GetReadHandler (int Page)
{
	return CPU.ReadHandler[Page];
}

//******************************************************************************

static	__inline void	__cdecl	SetPRG_ROM4 (int Where, int What)
{
	CPU.PRGPointer[Where] = PRG_ROM[What & NES.PRGMask];
	CPU.Readable[Where] = TRUE;
	CPU.Writable[Where] = FALSE;
}
static	void	__cdecl	SetPRG_ROM8 (int Where, int What)
{
	What <<= 1;
	SetPRG_ROM4(Where+0,What+0);
	SetPRG_ROM4(Where+1,What+1);
}
static	void	__cdecl	SetPRG_ROM16 (int Where, int What)
{
	What <<= 2;
	SetPRG_ROM4(Where+0,What+0);
	SetPRG_ROM4(Where+1,What+1);
	SetPRG_ROM4(Where+2,What+2);
	SetPRG_ROM4(Where+3,What+3);
}
static	void	__cdecl	SetPRG_ROM32 (int Where, int What)
{
	What <<= 3;
	SetPRG_ROM4(Where+0,What+0);
	SetPRG_ROM4(Where+1,What+1);
	SetPRG_ROM4(Where+2,What+2);
	SetPRG_ROM4(Where+3,What+3);
	SetPRG_ROM4(Where+4,What+4);
	SetPRG_ROM4(Where+5,What+5);
	SetPRG_ROM4(Where+6,What+6);
	SetPRG_ROM4(Where+7,What+7);
}
static	int	__cdecl	GetPRG_ROM4 (int Where)	/* -1 if no ROM mapped */
{
	int tpi = (CPU.PRGPointer[Where] - PRG_ROM[0]) >> 12;
	if ((tpi < 0) || (tpi >= (MP.PRG_ROM_Size >> 12))) return -1;
	else return tpi;
}

//******************************************************************************

static	__inline void	__cdecl	SetPRG_RAM4 (int Where, int What)
{
	CPU.PRGPointer[Where] = PRG_RAM[What & 0xF];
	CPU.Readable[Where] = TRUE;
	CPU.Writable[Where] = TRUE;
}
static	void	__cdecl	SetPRG_RAM8 (int Where, int What)
{
	What <<= 1;
	SetPRG_RAM4(Where+0,What+0);
	SetPRG_RAM4(Where+1,What+1);
}
static	void	__cdecl	SetPRG_RAM16 (int Where, int What)
{
	What <<= 2;
	SetPRG_RAM4(Where+0,What+0);
	SetPRG_RAM4(Where+1,What+1);
	SetPRG_RAM4(Where+2,What+2);
	SetPRG_RAM4(Where+3,What+3);
}
static	void	__cdecl	SetPRG_RAM32 (int Where, int What)
{
	What <<= 3;
	SetPRG_RAM4(Where+0,What+0);
	SetPRG_RAM4(Where+1,What+1);
	SetPRG_RAM4(Where+2,What+2);
	SetPRG_RAM4(Where+3,What+3);
	SetPRG_RAM4(Where+4,What+4);
	SetPRG_RAM4(Where+5,What+5);
	SetPRG_RAM4(Where+6,What+6);
	SetPRG_RAM4(Where+7,What+7);
}
static	int	__cdecl	GetPRG_RAM4 (int Where)	/* -1 if no RAM mapped */
{
	int tpi = (CPU.PRGPointer[Where] - PRG_RAM[0]) >> 12;
	if ((tpi < 0) || (tpi > 0xF)) return -1;
	else return tpi;
}

//******************************************************************************

static	unsigned char *	__cdecl	GetPRG_Ptr4 (int Where)
{
	return CPU.PRGPointer[Where];
}
static	void	__cdecl	SetPRG_OB4 (int Where)	/* Open bus */
{
	CPU.Readable[Where] = FALSE;
}

//******************************************************************************

static	__inline void	__cdecl	SetCHR_ROM1 (int Where, int What)
{
	if (!MP.CHR_ROM_Size)
		return;
	PPU.CHRPointer[Where] = CHR_ROM[What & NES.CHRMask];
	PPU.Writable[Where] = FALSE;
	Debugger.PatChanged = TRUE;
	Debugger.NTabChanged = TRUE;
}
static	void	__cdecl	SetCHR_ROM2 (int Where, int What)
{
	What <<= 1;
	SetCHR_ROM1(Where+0,What+0);
	SetCHR_ROM1(Where+1,What+1);
}
static	void	__cdecl	SetCHR_ROM4 (int Where, int What)
{
	What <<= 2;
	SetCHR_ROM1(Where+0,What+0);
	SetCHR_ROM1(Where+1,What+1);
	SetCHR_ROM1(Where+2,What+2);
	SetCHR_ROM1(Where+3,What+3);
}
static	void	__cdecl	SetCHR_ROM8 (int Where, int What)
{
	What <<= 3;
	SetCHR_ROM1(Where+0,What+0);
	SetCHR_ROM1(Where+1,What+1);
	SetCHR_ROM1(Where+2,What+2);
	SetCHR_ROM1(Where+3,What+3);
	SetCHR_ROM1(Where+4,What+4);
	SetCHR_ROM1(Where+5,What+5);
	SetCHR_ROM1(Where+6,What+6);
	SetCHR_ROM1(Where+7,What+7);
}
static	int	__cdecl	GetCHR_ROM1 (int Where)	/* -1 if no ROM mapped */
{
	int tpi = (PPU.CHRPointer[Where] - CHR_ROM[0]) >> 10;
	if ((tpi < 0) || (tpi >= (MP.CHR_ROM_Size >> 10))) return -1;
	else return tpi;
}

//******************************************************************************

static	__inline void	__cdecl	SetCHR_RAM1 (int Where, int What)
{
	PPU.CHRPointer[Where] = CHR_RAM[What & 0x1F];
	PPU.Writable[Where] = TRUE;
	Debugger.PatChanged = TRUE;
	Debugger.NTabChanged = TRUE;
}
static	void	__cdecl	SetCHR_RAM2 (int Where, int What)
{
	What <<= 1;
	SetCHR_RAM1(Where+0,What+0);
	SetCHR_RAM1(Where+1,What+1);
}
static	void	__cdecl	SetCHR_RAM4 (int Where, int What)
{
	What <<= 2;
	SetCHR_RAM1(Where+0,What+0);
	SetCHR_RAM1(Where+1,What+1);
	SetCHR_RAM1(Where+2,What+2);
	SetCHR_RAM1(Where+3,What+3);
}
static	void	__cdecl	SetCHR_RAM8 (int Where, int What)
{
	What <<= 3;
	SetCHR_RAM1(Where+0,What+0);
	SetCHR_RAM1(Where+1,What+1);
	SetCHR_RAM1(Where+2,What+2);
	SetCHR_RAM1(Where+3,What+3);
	SetCHR_RAM1(Where+4,What+4);
	SetCHR_RAM1(Where+5,What+5);
	SetCHR_RAM1(Where+6,What+6);
	SetCHR_RAM1(Where+7,What+7);
}
static	int	__cdecl	GetCHR_RAM1 (int Where)	/* -1 if no ROM mapped */
{
	int tpi = (PPU.CHRPointer[Where] - CHR_RAM[0]) >> 10;
	if ((tpi < 0) || (tpi > 0x1F)) return -1;
	else return tpi;
}

//******************************************************************************

static	unsigned char *	__cdecl	GetCHR_Ptr1 (int Where)
{
	return PPU.CHRPointer[Where];
}

//******************************************************************************

static	void	__cdecl	Mirror_H (void)
{
	PPU_SetMirroring(0,0,1,1);
}
static	void	__cdecl	Mirror_V (void)
{
	PPU_SetMirroring(0,1,0,1);
}
static	void	__cdecl	Mirror_4 (void)
{
	PPU_SetMirroring(0,1,2,3);
}
static	void	__cdecl	Mirror_S0 (void)
{
	PPU_SetMirroring(0,0,0,0);
}
static	void	__cdecl	Mirror_S1 (void)
{
	PPU_SetMirroring(1,1,1,1);
}
static	void	__cdecl	Mirror_Custom (int M1, int M2, int M3, int M4)
{
	PPU_SetMirroring(M1,M2,M3,M4);
}

//******************************************************************************

static	void	__cdecl	Set_SRAMSize (int Size)	/* Sets the size of the SRAM (in bytes) and clears PRG_RAM */
{
	NES.SRAM_Size = Size;
	memset(PRG_RAM,0,sizeof(PRG_RAM));
}
static	void	__cdecl	Save_SRAM (void)	/* Saves SRAM to disk */
{
	char Filename[MAX_PATH];
	FILE *SRAMFile;
	sprintf(Filename,"%s.sav",States.BaseFilename);
	SRAMFile = fopen(Filename,"wb");
	fwrite(PRG_RAM,1,NES.SRAM_Size,SRAMFile);
	fclose(SRAMFile);
}
static	void	__cdecl	Load_SRAM (void)	/* Loads SRAM from disk */
{
	char Filename[MAX_PATH];
	FILE *SRAMFile;
	sprintf(Filename,"%s.sav",States.BaseFilename);
	SRAMFile = fopen(Filename,"rb");
	if (!SRAMFile)
		return;
	fseek(SRAMFile,0,SEEK_END);
	if (ftell(SRAMFile) >= NES.SRAM_Size)
	{
		fseek(SRAMFile,0,SEEK_SET);
		fread(PRG_RAM,1,NES.SRAM_Size,SRAMFile);
	}
	fclose(SRAMFile);
}

//******************************************************************************

static	void	__cdecl	IRQ (void)
{
	CPU.WantIRQ = TRUE;
}

static	void	__cdecl	DbgOut (char *ToSay)
{
}

static	void	__cdecl	StatusOut (char *ToSay)
{
	GFX_ShowText(ToSay);
}

static	int	__cdecl	GetMenuRoot (void)
{
	return (int)NULL;
}

static	int	__cdecl	AddMenuItem (int root, char *text, int cmd, int parm1, int parm2, int parm3, int mode)
{
/*	if (MenuRoot.hMenu == 0)
	{
		MenuRoot.hMenu = CreateMenu
		InsertMenu(NULL,5,MF_BYPOSITION | MF_STRING | MF_POPUP,(HMENU)MenuRoot.hMenu,MenuRoot.Caption);
	}*/
	return (int)NULL;
}
/*
static	void	ClearMenuItem (PMenuItem Menu, PMenuItem MenuTop)
{
	if (Menu == NULL)
		return;
	ClearMenuItem(Menu->Child,MenuTop);
	Menu->Child = NULL;
	ClearMenuItem(Menu->Next,MenuTop);
	Menu->Next = NULL;
	if (Menu != MenuTop)
	{
		RemoveMenu(Menu->hMenu
		//remove menu from window
		free(Menu);
	}
}

static	void	ClearMenu ()
{
	ClearMenuItem(&MenuRoot,&MenuRoot);
}*/

//******************************************************************************

static	void	__cdecl MMC5_UpdateAttributeCache (void)
{
	/* Nothing to do here; there is no attribute cache */
}

//******************************************************************************

void	MapperInterface_Init (void)
{
	WIN32_FIND_DATA Data;
	HANDLE Handle;
	struct tMapperDLL *ThisDLL;
	char Filename[MAX_PATH], Path[MAX_PATH];
	strcpy(Path,ProgPath);
	strcat(Path,"Mappers\\");
	sprintf(Filename,"%s%s",Path,"*.dll");
	Handle = FindFirstFile(Filename,&Data);
	ThisDLL = malloc(sizeof(struct tMapperDLL));
	do
	{
		char Tmp[MAX_PATH];
		sprintf(Tmp,"%s%s",Path,Data.cFileName);
		ThisDLL->dInst = LoadLibrary(Tmp);
		ThisDLL->LoadDLL = (PLoad_DLL)GetProcAddress(ThisDLL->dInst,"Load_DLL");
		ThisDLL->UnloadDLL = (PUnload_DLL)GetProcAddress(ThisDLL->dInst,"Unload_DLL");
		if ((ThisDLL->LoadDLL) && (ThisDLL->UnloadDLL))
		{
			ThisDLL->DI = ThisDLL->LoadDLL(CurrentMapperInterface);
			if (ThisDLL->DI)
			{
				ThisDLL->Next = MapperDLLs;
				MapperDLLs = ThisDLL;
				ThisDLL = malloc(sizeof(struct tMapperDLL));
			}
			else	FreeLibrary(ThisDLL->dInst);
		}
		else	FreeLibrary(ThisDLL->dInst);
	}	while (FindNextFile(Handle,&Data));
	free(ThisDLL);
	FindClose(Handle);
	if (MapperDLLs == NULL)
		MessageBox(mWnd,"Fatal error: unable to locate any mapper DLLs!","Nintendulator",MB_OK | MB_ICONERROR);
	MP.SetWriteHandler = SetWriteHandler;
	MP.SetReadHandler = SetReadHandler;
	MP.GetWriteHandler = GetWriteHandler;
	MP.GetReadHandler = GetReadHandler;
	MP.SetPRG_ROM4 = SetPRG_ROM4;
	MP.SetPRG_ROM8 = SetPRG_ROM8;
	MP.SetPRG_ROM16 = SetPRG_ROM16;
	MP.SetPRG_ROM32 = SetPRG_ROM32;
	MP.GetPRG_ROM4 = GetPRG_ROM4;
	MP.SetPRG_RAM4 = SetPRG_RAM4;
	MP.SetPRG_RAM8 = SetPRG_RAM8;
	MP.SetPRG_RAM16 = SetPRG_RAM16;
	MP.SetPRG_RAM32 = SetPRG_RAM32;
	MP.GetPRG_RAM4 = GetPRG_RAM4;
	MP.GetPRG_Ptr4 = GetPRG_Ptr4;
	MP.SetPRG_OB4 = SetPRG_OB4;
	MP.SetCHR_ROM1 = SetCHR_ROM1;
	MP.SetCHR_ROM2 = SetCHR_ROM2;
	MP.SetCHR_ROM4 = SetCHR_ROM4;
	MP.SetCHR_ROM8 = SetCHR_ROM8;
	MP.GetCHR_ROM1 = GetCHR_ROM1;
	MP.SetCHR_RAM1 = SetCHR_RAM1;
	MP.SetCHR_RAM2 = SetCHR_RAM2;
	MP.SetCHR_RAM4 = SetCHR_RAM4;
	MP.SetCHR_RAM8 = SetCHR_RAM8;
	MP.GetCHR_RAM1 = GetCHR_RAM1;
	MP.GetCHR_Ptr1 = GetCHR_Ptr1;
	MP.Mirror_H = Mirror_H;
	MP.Mirror_V = Mirror_V;
	MP.Mirror_4 = Mirror_4;
	MP.Mirror_S0 = Mirror_S0;
	MP.Mirror_S1 = Mirror_S1;
	MP.Mirror_Custom = Mirror_Custom;
	MP.IRQ = IRQ;
	MP.Set_SRAMSize = Set_SRAMSize;
	MP.Save_SRAM = Save_SRAM;
	MP.Load_SRAM = Load_SRAM;
	MP.DbgOut = DbgOut;
	MP.StatusOut = StatusOut;
	MP.AddMenuItem = AddMenuItem;
	MP.GetMenuRoot = GetMenuRoot;
	MP.MMC5_UpdateAttributeCache = MMC5_UpdateAttributeCache;
	MI = MI2 = NULL;
}

BOOL	MapperInterface_LoadByNum (int MapperNum)
{
	struct tMapperDLL *ThisDLL = MapperDLLs;
//	ClearMenu();
	while (ThisDLL)
	{
		if (MI = ThisDLL->DI->LoadMapper(MapperNum))
			return TRUE;
		ThisDLL = ThisDLL->Next;
	}
	return FALSE;
}

BOOL	MapperInterface_LoadByBoard (char *Board)
{
	struct tMapperDLL *ThisDLL = MapperDLLs;
//	ClearMenu();
	while (ThisDLL)
	{
		if (MI = ThisDLL->DI->LoadBoard(Board))
			return TRUE;
		ThisDLL = ThisDLL->Next;
	}
	return FALSE;
}

void	MapperInterface_Release (void)
{
	struct tMapperDLL *ThisDLL = MapperDLLs;
//	ClearMenu();
	while (ThisDLL)
	{
		MapperDLLs = ThisDLL->Next;
		ThisDLL->UnloadDLL();
		FreeLibrary(ThisDLL->dInst);
		free(ThisDLL);
		ThisDLL = MapperDLLs;
	}
}
