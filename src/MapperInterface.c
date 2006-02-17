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

#ifdef NSFPLAYER
# include "in_nintendulator.h"
# include "MapperInterface.h"
# include "CPU.h"
#else
# include "Nintendulator.h"
# include "NES.h"
# include "CPU.h"
# include "PPU.h"
# include "GFX.h"
# include "Debugger.h"
# include "States.h"
# include "Controllers.h"
#endif

TEmulatorInterface	EI;
TROMInfo		RI;

PDLLInfo		DI;
CPMapperInfo		MI;
#ifndef NSFPLAYER
CPMapperInfo		MI2;
#endif

#ifdef NSFPLAYER
HINSTANCE		dInst;
PLoadMapperDLL		LoadDLL;
PUnloadMapperDLL	UnloadDLL;
#else
struct	tMapperDLL
{
	HINSTANCE		dInst;
	PLoadMapperDLL		LoadDLL;
	PDLLInfo		DI;
	PUnloadMapperDLL	UnloadDLL;
	struct	tMapperDLL *	Next;
} *MapperDLLs = NULL;
#endif

/******************************************************************************/

static	void	_MAPINT	SetCPUWriteHandler (int Page, FCPUWrite New)
{
	CPU.WriteHandler[Page] = New;
}
static	void	_MAPINT	SetCPUReadHandler (int Page, FCPURead New)
{
	CPU.ReadHandler[Page] = New;
}
static	FCPUWrite	_MAPINT	GetCPUWriteHandler (int Page)
{
	return CPU.WriteHandler[Page];
}
static	FCPURead	_MAPINT	GetCPUReadHandler (int Page)
{
	return CPU.ReadHandler[Page];
}

static	void	_MAPINT	SetPPUWriteHandler (int Page, FPPUWrite New)
{
#ifndef NSFPLAYER
	PPU.WriteHandler[Page] = New;
#endif
}
static	void	_MAPINT	SetPPUReadHandler (int Page, FPPURead New)
{
#ifndef NSFPLAYER
	PPU.ReadHandler[Page] = New;
#endif
}
static	FPPUWrite	_MAPINT	GetPPUWriteHandler (int Page)
{
#ifndef NSFPLAYER
	return PPU.WriteHandler[Page];
#else
	return NULL;
#endif
}
static	FPPURead	_MAPINT	GetPPUReadHandler (int Page)
{
#ifndef NSFPLAYER
	return PPU.ReadHandler[Page];
#else
	return NULL;
#endif
}

/******************************************************************************/

static	__inline void	_MAPINT	SetPRG_ROM4 (int Where, int What)
{
	CPU.PRGPointer[Where] = PRG_ROM[What & NES.PRGMask];
	CPU.Readable[Where] = TRUE;
	CPU.Writable[Where] = FALSE;
}
static	void	_MAPINT	SetPRG_ROM8 (int Where, int What)
{
	What <<= 1;
	SetPRG_ROM4(Where+0,What+0);
	SetPRG_ROM4(Where+1,What+1);
}
static	void	_MAPINT	SetPRG_ROM16 (int Where, int What)
{
	What <<= 2;
	SetPRG_ROM4(Where+0,What+0);
	SetPRG_ROM4(Where+1,What+1);
	SetPRG_ROM4(Where+2,What+2);
	SetPRG_ROM4(Where+3,What+3);
}
static	void	_MAPINT	SetPRG_ROM32 (int Where, int What)
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
static	int	_MAPINT	GetPRG_ROM4 (int Where)	/* -1 if no ROM mapped */
{
	int tpi = (int)((CPU.PRGPointer[Where] - PRG_ROM[0]) >> 12);
	if ((tpi < 0) || (tpi > NES.PRGMask)) return -1;
	else return tpi;
}

/******************************************************************************/

static	__inline void	_MAPINT	SetPRG_RAM4 (int Where, int What)
{
	CPU.PRGPointer[Where] = PRG_RAM[What & 0xF];
	CPU.Readable[Where] = TRUE;
	CPU.Writable[Where] = TRUE;
}
static	void	_MAPINT	SetPRG_RAM8 (int Where, int What)
{
	What <<= 1;
	SetPRG_RAM4(Where+0,What+0);
	SetPRG_RAM4(Where+1,What+1);
}
static	void	_MAPINT	SetPRG_RAM16 (int Where, int What)
{
	What <<= 2;
	SetPRG_RAM4(Where+0,What+0);
	SetPRG_RAM4(Where+1,What+1);
	SetPRG_RAM4(Where+2,What+2);
	SetPRG_RAM4(Where+3,What+3);
}
static	void	_MAPINT	SetPRG_RAM32 (int Where, int What)
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
static	int	_MAPINT	GetPRG_RAM4 (int Where)	/* -1 if no RAM mapped */
{
	int tpi = (int)((CPU.PRGPointer[Where] - PRG_RAM[0]) >> 12);
	if ((tpi < 0) || (tpi > MAX_PRGRAM_MASK)) return -1;
	else return tpi;
}

/******************************************************************************/

static	unsigned char *	_MAPINT	GetPRG_Ptr4 (int Where)
{
	return CPU.PRGPointer[Where];
}
static	void	_MAPINT	SetPRG_Ptr4 (int Where, unsigned char *Data, BOOL Writable)
{
	CPU.PRGPointer[Where] = Data;
	CPU.Readable[Where] = TRUE;
	CPU.Writable[Where] = Writable;
}
static	void	_MAPINT	SetPRG_OB4 (int Where)	/* Open bus */
{
	CPU.Readable[Where] = FALSE;
	CPU.Writable[Where] = FALSE;
}

/******************************************************************************/

static	__inline void	_MAPINT	SetCHR_ROM1 (int Where, int What)
{
#ifndef NSFPLAYER
	if (!NES.CHRMask)
		return;
	PPU.CHRPointer[Where] = CHR_ROM[What & NES.CHRMask];
	PPU.Writable[Where] = FALSE;
#ifdef ENABLE_DEBUGGER
	Debugger.PatChanged = TRUE;
	Debugger.NTabChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif
}
static	void	_MAPINT	SetCHR_ROM2 (int Where, int What)
{
	What <<= 1;
	SetCHR_ROM1(Where+0,What+0);
	SetCHR_ROM1(Where+1,What+1);
}
static	void	_MAPINT	SetCHR_ROM4 (int Where, int What)
{
	What <<= 2;
	SetCHR_ROM1(Where+0,What+0);
	SetCHR_ROM1(Where+1,What+1);
	SetCHR_ROM1(Where+2,What+2);
	SetCHR_ROM1(Where+3,What+3);
}
static	void	_MAPINT	SetCHR_ROM8 (int Where, int What)
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
static	int	_MAPINT	GetCHR_ROM1 (int Where)	/* -1 if no ROM mapped */
{
#ifndef NSFPLAYER
	int tpi = (int)((PPU.CHRPointer[Where] - CHR_ROM[0]) >> 10);
	if ((tpi < 0) || (tpi > NES.CHRMask)) return -1;
	else return tpi;
#else
	return -1;
#endif
}

/******************************************************************************/

static	__inline void	_MAPINT	SetCHR_RAM1 (int Where, int What)
{
#ifndef NSFPLAYER
	PPU.CHRPointer[Where] = CHR_RAM[What & 0x1F];
	PPU.Writable[Where] = TRUE;
#ifdef ENABLE_DEBUGGER
	Debugger.PatChanged = TRUE;
	Debugger.NTabChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif
}
static	void	_MAPINT	SetCHR_RAM2 (int Where, int What)
{
	What <<= 1;
	SetCHR_RAM1(Where+0,What+0);
	SetCHR_RAM1(Where+1,What+1);
}
static	void	_MAPINT	SetCHR_RAM4 (int Where, int What)
{
	What <<= 2;
	SetCHR_RAM1(Where+0,What+0);
	SetCHR_RAM1(Where+1,What+1);
	SetCHR_RAM1(Where+2,What+2);
	SetCHR_RAM1(Where+3,What+3);
}
static	void	_MAPINT	SetCHR_RAM8 (int Where, int What)
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
static	int	_MAPINT	GetCHR_RAM1 (int Where)	/* -1 if no ROM mapped */
{
#ifndef NSFPLAYER
	int tpi = (int)((PPU.CHRPointer[Where] - CHR_RAM[0]) >> 10);
	if ((tpi < 0) || (tpi > MAX_CHRRAM_MASK)) return -1;
	else return tpi;
#else
	return -1;
#endif
}

/******************************************************************************/

static	__inline void	_MAPINT	SetCHR_NT1 (int Where, int What)
{
#ifndef NSFPLAYER
	PPU.CHRPointer[Where] = PPU_VRAM[What & 3];
	PPU.Writable[Where] = TRUE;
#ifdef ENABLE_DEBUGGER
	Debugger.NTabChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif
}
static	int	_MAPINT	GetCHR_NT1 (int Where)	/* -1 if no ROM mapped */
{
#ifndef NSFPLAYER
	int tpi = (int)((PPU.CHRPointer[Where] - PPU_VRAM[0]) >> 10);
	if ((tpi < 0) || (tpi > 4)) return -1;
	else return tpi;
#else
	return -1;
#endif
}

/******************************************************************************/

static	unsigned char *	_MAPINT	GetCHR_Ptr1 (int Where)
{
#ifndef NSFPLAYER
	return PPU.CHRPointer[Where];
#else
	return NULL;
#endif
}

static	void	_MAPINT	SetCHR_Ptr1 (int Where, unsigned char *Data, BOOL Writable)
{
#ifndef NSFPLAYER
	PPU.CHRPointer[Where] = Data;
	PPU.Writable[Where] = Writable;
#ifdef ENABLE_DEBUGGER
	Debugger.PatChanged = TRUE;
	Debugger.NTabChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif
}

static	void	_MAPINT	SetCHR_OB1 (int Where)
{
#ifndef NSFPLAYER
	PPU.CHRPointer[Where] = PPU_OpenBus;
	PPU.Writable[Where] = FALSE;
#ifdef ENABLE_DEBUGGER
	Debugger.PatChanged = TRUE;
	Debugger.NTabChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif
}

/******************************************************************************/

static	void	_MAPINT	Mirror_H (void)
{
#ifndef NSFPLAYER
	SetCHR_NT1(0x8,0);	SetCHR_NT1(0xC,0);
	SetCHR_NT1(0x9,0);	SetCHR_NT1(0xD,0);
	SetCHR_NT1(0xA,1);	SetCHR_NT1(0xE,1);
	SetCHR_NT1(0xB,1);	SetCHR_NT1(0xF,1);
#endif
}
static	void	_MAPINT	Mirror_V (void)
{
#ifndef NSFPLAYER
	SetCHR_NT1(0x8,0);	SetCHR_NT1(0xC,0);
	SetCHR_NT1(0x9,1);	SetCHR_NT1(0xD,1);
	SetCHR_NT1(0xA,0);	SetCHR_NT1(0xE,0);
	SetCHR_NT1(0xB,1);	SetCHR_NT1(0xF,1);
#endif
}
static	void	_MAPINT	Mirror_4 (void)
{
#ifndef NSFPLAYER
	SetCHR_NT1(0x8,0);	SetCHR_NT1(0xC,0);
	SetCHR_NT1(0x9,1);	SetCHR_NT1(0xD,1);
	SetCHR_NT1(0xA,2);	SetCHR_NT1(0xE,2);
	SetCHR_NT1(0xB,3);	SetCHR_NT1(0xF,3);
#endif
}
static	void	_MAPINT	Mirror_S0 (void)
{
#ifndef NSFPLAYER
	SetCHR_NT1(0x8,0);	SetCHR_NT1(0xC,0);
	SetCHR_NT1(0x9,0);	SetCHR_NT1(0xD,0);
	SetCHR_NT1(0xA,0);	SetCHR_NT1(0xE,0);
	SetCHR_NT1(0xB,0);	SetCHR_NT1(0xF,0);
#endif
}
static	void	_MAPINT	Mirror_S1 (void)
{
#ifndef NSFPLAYER
	SetCHR_NT1(0x8,1);	SetCHR_NT1(0xC,1);
	SetCHR_NT1(0x9,1);	SetCHR_NT1(0xD,1);
	SetCHR_NT1(0xA,1);	SetCHR_NT1(0xE,1);
	SetCHR_NT1(0xB,1);	SetCHR_NT1(0xF,1);
#endif
}
static	void	_MAPINT	Mirror_Custom (int M1, int M2, int M3, int M4)
{
#ifndef NSFPLAYER
	SetCHR_NT1(0x8,M1);	SetCHR_NT1(0xC,M1);
	SetCHR_NT1(0x9,M2);	SetCHR_NT1(0xD,M2);
	SetCHR_NT1(0xA,M3);	SetCHR_NT1(0xE,M3);
	SetCHR_NT1(0xB,M4);	SetCHR_NT1(0xF,M4);
#endif
}

/******************************************************************************/

static	void	_MAPINT	Set_SRAMSize (int Size)	/* Sets the size of the SRAM (in bytes) and clears PRG_RAM */
{
#ifndef NSFPLAYER
	if (Controllers.MovieMode)		/* Do not save SRAM when recording movies */
		NES.SRAM_Size = 0;
	else	NES.SRAM_Size = Size;
	memset(PRG_RAM,0,sizeof(PRG_RAM));
#endif
}

/******************************************************************************/

static	void	_MAPINT	SetIRQ (int IRQstate)
{
	if (IRQstate)
		CPU.WantIRQ &= ~IRQ_EXTERNAL;
	else	CPU.WantIRQ |= IRQ_EXTERNAL;
}

static	void	_MAPINT	DbgOut (char *text, ...)
{
#ifndef NSFPLAYER
	extern void AddDebug (char *txt);
	static char txt[1024];
	va_list marker;
	va_start(marker,text);
	vsprintf(txt,text,marker);
	va_end(marker);
	AddDebug(txt);
#endif
}

static	void	_MAPINT	StatusOut (char *text, ...)
{
#ifndef NSFPLAYER
	static char txt[1024];
	va_list marker;
	va_start(marker,text);
	vsprintf(txt,text,marker);
	va_end(marker);
	GFX_ShowText(txt);
#endif
}

/******************************************************************************/

void	MapperInterface_Init (void)
{
#ifndef NSFPLAYER
	WIN32_FIND_DATA Data;
	HANDLE Handle;
	struct tMapperDLL *ThisDLL;
	char Filename[MAX_PATH], Path[MAX_PATH];
	strcpy(Path,ProgPath);
	strcat(Path,"Mappers\\");
	sprintf(Filename,"%s%s",Path,"*.dll");
	Handle = FindFirstFile(Filename,&Data);
	ThisDLL = malloc(sizeof(struct tMapperDLL));
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			char Tmp[MAX_PATH];
			sprintf(Tmp,"%s%s",Path,Data.cFileName);
			ThisDLL->dInst = LoadLibrary(Tmp);
			ThisDLL->LoadDLL = (PLoadMapperDLL)GetProcAddress(ThisDLL->dInst,"LoadMapperDLL");
			ThisDLL->UnloadDLL = (PUnloadMapperDLL)GetProcAddress(ThisDLL->dInst,"UnloadMapperDLL");
			if ((ThisDLL->LoadDLL) && (ThisDLL->UnloadDLL))
			{
				ThisDLL->DI = ThisDLL->LoadDLL(mWnd,&EI,CurrentMapperInterface);
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
		FindClose(Handle);
	}
	free(ThisDLL);
	if (MapperDLLs == NULL)
		MessageBox(mWnd,"Fatal error: unable to locate any mapper DLLs!","Nintendulator",MB_OK | MB_ICONERROR);
#else
	dInst = LoadLibrary("Plugins\\nsf.dll");
	LoadDLL = (PLoadMapperDLL)GetProcAddress(dInst,"LoadMapperDLL");
	UnloadDLL = (PUnloadMapperDLL)GetProcAddress(dInst,"UnloadMapperDLL");
	if ((!LoadDLL) || (!UnloadDLL) || (!(DI = LoadDLL(mod.hMainWindow,&EI,CurrentMapperInterface))))
		MessageBox(mod.hMainWindow,"Fatal error: unable to locate any mapper DLLs!","Nintendulator",MB_OK | MB_ICONERROR);
#endif
	memset(&EI,0,sizeof(EI));
	memset(&RI,0,sizeof(RI));
	EI.SetCPUWriteHandler = SetCPUWriteHandler;
	EI.SetCPUReadHandler = SetCPUReadHandler;
	EI.GetCPUWriteHandler = GetCPUWriteHandler;
	EI.GetCPUReadHandler = GetCPUReadHandler;
	EI.SetPPUWriteHandler = SetPPUWriteHandler;
	EI.SetPPUReadHandler = SetPPUReadHandler;
	EI.GetPPUWriteHandler = GetPPUWriteHandler;
	EI.GetPPUReadHandler = GetPPUReadHandler;
	EI.SetPRG_ROM4 = SetPRG_ROM4;
	EI.SetPRG_ROM8 = SetPRG_ROM8;
	EI.SetPRG_ROM16 = SetPRG_ROM16;
	EI.SetPRG_ROM32 = SetPRG_ROM32;
	EI.GetPRG_ROM4 = GetPRG_ROM4;
	EI.SetPRG_RAM4 = SetPRG_RAM4;
	EI.SetPRG_RAM8 = SetPRG_RAM8;
	EI.SetPRG_RAM16 = SetPRG_RAM16;
	EI.SetPRG_RAM32 = SetPRG_RAM32;
	EI.GetPRG_RAM4 = GetPRG_RAM4;
	EI.GetPRG_Ptr4 = GetPRG_Ptr4;
	EI.SetPRG_Ptr4 = SetPRG_Ptr4;
	EI.SetPRG_OB4 = SetPRG_OB4;
	EI.SetCHR_ROM1 = SetCHR_ROM1;
	EI.SetCHR_ROM2 = SetCHR_ROM2;
	EI.SetCHR_ROM4 = SetCHR_ROM4;
	EI.SetCHR_ROM8 = SetCHR_ROM8;
	EI.GetCHR_ROM1 = GetCHR_ROM1;
	EI.SetCHR_RAM1 = SetCHR_RAM1;
	EI.SetCHR_RAM2 = SetCHR_RAM2;
	EI.SetCHR_RAM4 = SetCHR_RAM4;
	EI.SetCHR_RAM8 = SetCHR_RAM8;
	EI.GetCHR_RAM1 = GetCHR_RAM1;
	EI.SetCHR_NT1 = SetCHR_NT1;
	EI.GetCHR_NT1 = GetCHR_NT1;
	EI.GetCHR_Ptr1 = GetCHR_Ptr1;
	EI.SetCHR_Ptr1 = SetCHR_Ptr1;
	EI.SetCHR_OB1 = SetCHR_OB1;
	EI.Mirror_H = Mirror_H;
	EI.Mirror_V = Mirror_V;
	EI.Mirror_4 = Mirror_4;
	EI.Mirror_S0 = Mirror_S0;
	EI.Mirror_S1 = Mirror_S1;
	EI.Mirror_Custom = Mirror_Custom;
	EI.SetIRQ = SetIRQ;
	EI.Set_SRAMSize = Set_SRAMSize;
	EI.DbgOut = DbgOut;
	EI.StatusOut = StatusOut;
	EI.OpenBus = &CPU.LastRead;
	MI = NULL;
#ifndef NSFPLAYER
	MI2 = NULL;
#endif
}

BOOL	MapperInterface_LoadMapper (CPROMInfo ROM)
{
#ifndef NSFPLAYER
	struct tMapperDLL *ThisDLL = MapperDLLs;
	while (ThisDLL)
	{
		DI = ThisDLL->DI;
		if (MI = DI->LoadMapper(ROM))
			return TRUE;
		ThisDLL = ThisDLL->Next;
	}
	DI = NULL;
#else
	if (!DI)
		return FALSE;
	if (MI = DI->LoadMapper(ROM))
		return TRUE;
#endif
	return FALSE;
}

void	MapperInterface_UnloadMapper (void)
{
	if (MI)
	{
		if (MI->Unload)
			MI->Unload();
		MI = NULL;
	}
	if (DI)
	{
		if (DI->UnloadMapper)
			DI->UnloadMapper();
#ifndef	NSFPLAYER
		DI = NULL;
#endif
	}
}

void	MapperInterface_Release (void)
{
#ifndef NSFPLAYER
	struct tMapperDLL *ThisDLL = MapperDLLs;
	while (ThisDLL)
	{
		MapperDLLs = ThisDLL->Next;
		ThisDLL->UnloadDLL();
		FreeLibrary(ThisDLL->dInst);
		free(ThisDLL);
		ThisDLL = MapperDLLs;
	}
#else
	DI = NULL;
	UnloadDLL();
	FreeLibrary(dInst);
#endif
}
