/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2009 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifdef	NSFPLAYER
# include "in_nintendulator.h"
# include "MapperInterface.h"
# include "CPU.h"
#else	/* !NSFPLAYER */
# include "stdafx.h"
# include "Nintendulator.h"
# include "resource.h"
# include "MapperInterface.h"
# include "NES.h"
# include "CPU.h"
# include "PPU.h"
# include "GFX.h"
# include "Debugger.h"
#endif	/* NSFPLAYER */

TEmulatorInterface	EI;
TROMInfo		RI;

PDLLInfo		DI;
CPMapperInfo		MI;
#ifndef	NSFPLAYER
CPMapperInfo		MI2;
#endif	/* !NSFPLAYER */

#ifdef	NSFPLAYER
HINSTANCE		dInst;
PLoadMapperDLL		LoadDLL;
PUnloadMapperDLL	UnloadDLL;
#else	/* !NSFPLAYER */
struct	tMapperDLL
{
	HINSTANCE		dInst;
	PLoadMapperDLL		LoadDLL;
	PDLLInfo		DI;
	PUnloadMapperDLL	UnloadDLL;
	struct	tMapperDLL *	Next;
} *MapperDLLs = NULL;
#endif	/* NSFPLAYER */

/******************************************************************************/

static	void	MAPINT	SetCPUWriteHandler (int Page, FCPUWrite New)
{
	CPU::WriteHandler[Page] = New;
}
static	void	MAPINT	SetCPUReadHandler (int Page, FCPURead New)
{
	CPU::ReadHandler[Page] = New;
}
static	FCPUWrite	MAPINT	GetCPUWriteHandler (int Page)
{
	return CPU::WriteHandler[Page];
}
static	FCPURead	MAPINT	GetCPUReadHandler (int Page)
{
	return CPU::ReadHandler[Page];
}

static	void	MAPINT	SetPPUWriteHandler (int Page, FPPUWrite New)
{
#ifndef	NSFPLAYER
	PPU.WriteHandler[Page] = New;
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	SetPPUReadHandler (int Page, FPPURead New)
{
#ifndef	NSFPLAYER
	PPU.ReadHandler[Page] = New;
#endif	/* !NSFPLAYER */
}
static	FPPUWrite	MAPINT	GetPPUWriteHandler (int Page)
{
#ifndef	NSFPLAYER
	return PPU.WriteHandler[Page];
#else	/* NSFPLAYER */
	return NULL;
#endif	/* !NSFPLAYER */
}
static	FPPURead	MAPINT	GetPPUReadHandler (int Page)
{
#ifndef	NSFPLAYER
	return PPU.ReadHandler[Page];
#else	/* NSFPLAYER */
	return NULL;
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	__inline void	MAPINT	SetPRG_ROM4 (int Bank, int Val)
{
	CPU::PRGPointer[Bank] = PRG_ROM[Val & NES.PRGMask];
	CPU::Readable[Bank] = TRUE;
	CPU::Writable[Bank] = FALSE;
}
static	void	MAPINT	SetPRG_ROM8 (int Bank, int Val)
{
	Val <<= 1;
	SetPRG_ROM4(Bank+0,Val+0);
	SetPRG_ROM4(Bank+1,Val+1);
}
static	void	MAPINT	SetPRG_ROM16 (int Bank, int Val)
{
	Val <<= 2;
	SetPRG_ROM4(Bank+0,Val+0);
	SetPRG_ROM4(Bank+1,Val+1);
	SetPRG_ROM4(Bank+2,Val+2);
	SetPRG_ROM4(Bank+3,Val+3);
}
static	void	MAPINT	SetPRG_ROM32 (int Bank, int Val)
{
	Val <<= 3;
	SetPRG_ROM4(Bank+0,Val+0);
	SetPRG_ROM4(Bank+1,Val+1);
	SetPRG_ROM4(Bank+2,Val+2);
	SetPRG_ROM4(Bank+3,Val+3);
	SetPRG_ROM4(Bank+4,Val+4);
	SetPRG_ROM4(Bank+5,Val+5);
	SetPRG_ROM4(Bank+6,Val+6);
	SetPRG_ROM4(Bank+7,Val+7);
}
static	int	MAPINT	GetPRG_ROM4 (int Bank)	/* -1 if no ROM mapped */
{
	int tpi = (int)((CPU::PRGPointer[Bank] - PRG_ROM[0]) >> 12);
	if ((tpi < 0) || (tpi > NES.PRGMask)) return -1;
	else return tpi;
}

/******************************************************************************/

static	__inline void	MAPINT	SetPRG_RAM4 (int Bank, int Val)
{
	CPU::PRGPointer[Bank] = PRG_RAM[Val & 0xF];
	CPU::Readable[Bank] = TRUE;
	CPU::Writable[Bank] = TRUE;
}
static	void	MAPINT	SetPRG_RAM8 (int Bank, int Val)
{
	Val <<= 1;
	SetPRG_RAM4(Bank+0,Val+0);
	SetPRG_RAM4(Bank+1,Val+1);
}
static	void	MAPINT	SetPRG_RAM16 (int Bank, int Val)
{
	Val <<= 2;
	SetPRG_RAM4(Bank+0,Val+0);
	SetPRG_RAM4(Bank+1,Val+1);
	SetPRG_RAM4(Bank+2,Val+2);
	SetPRG_RAM4(Bank+3,Val+3);
}
static	void	MAPINT	SetPRG_RAM32 (int Bank, int Val)
{
	Val <<= 3;
	SetPRG_RAM4(Bank+0,Val+0);
	SetPRG_RAM4(Bank+1,Val+1);
	SetPRG_RAM4(Bank+2,Val+2);
	SetPRG_RAM4(Bank+3,Val+3);
	SetPRG_RAM4(Bank+4,Val+4);
	SetPRG_RAM4(Bank+5,Val+5);
	SetPRG_RAM4(Bank+6,Val+6);
	SetPRG_RAM4(Bank+7,Val+7);
}
static	int	MAPINT	GetPRG_RAM4 (int Bank)	/* -1 if no RAM mapped */
{
	int tpi = (int)((CPU::PRGPointer[Bank] - PRG_RAM[0]) >> 12);
	if ((tpi < 0) || (tpi > MAX_PRGRAM_MASK)) return -1;
	else return tpi;
}

/******************************************************************************/

static	unsigned char *	MAPINT	GetPRG_Ptr4 (int Bank)
{
	return CPU::PRGPointer[Bank];
}
static	void	MAPINT	SetPRG_Ptr4 (int Bank, unsigned char *Data, BOOL Writable)
{
	CPU::PRGPointer[Bank] = Data;
	CPU::Readable[Bank] = TRUE;
	CPU::Writable[Bank] = Writable;
}
static	void	MAPINT	SetPRG_OB4 (int Bank)	/* Open bus */
{
	CPU::Readable[Bank] = FALSE;
	CPU::Writable[Bank] = FALSE;
}

/******************************************************************************/

static	__inline void	MAPINT	SetCHR_ROM1 (int Bank, int Val)
{
#ifndef	NSFPLAYER
	if (!NES.CHRMask)
		return;
	PPU.CHRPointer[Bank] = CHR_ROM[Val & NES.CHRMask];
	PPU.Writable[Bank] = FALSE;
#ifdef	ENABLE_DEBUGGER
	if (Bank & 8)
		Debugger::NTabChanged = TRUE;
	else	Debugger::PatChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	SetCHR_ROM2 (int Bank, int Val)
{
	Val <<= 1;
	SetCHR_ROM1(Bank+0,Val+0);
	SetCHR_ROM1(Bank+1,Val+1);
}
static	void	MAPINT	SetCHR_ROM4 (int Bank, int Val)
{
	Val <<= 2;
	SetCHR_ROM1(Bank+0,Val+0);
	SetCHR_ROM1(Bank+1,Val+1);
	SetCHR_ROM1(Bank+2,Val+2);
	SetCHR_ROM1(Bank+3,Val+3);
}
static	void	MAPINT	SetCHR_ROM8 (int Bank, int Val)
{
	Val <<= 3;
	SetCHR_ROM1(Bank+0,Val+0);
	SetCHR_ROM1(Bank+1,Val+1);
	SetCHR_ROM1(Bank+2,Val+2);
	SetCHR_ROM1(Bank+3,Val+3);
	SetCHR_ROM1(Bank+4,Val+4);
	SetCHR_ROM1(Bank+5,Val+5);
	SetCHR_ROM1(Bank+6,Val+6);
	SetCHR_ROM1(Bank+7,Val+7);
}
static	int	MAPINT	GetCHR_ROM1 (int Bank)	/* -1 if no ROM mapped */
{
#ifndef	NSFPLAYER
	int tpi = (int)((PPU.CHRPointer[Bank] - CHR_ROM[0]) >> 10);
	if ((tpi < 0) || (tpi > NES.CHRMask)) return -1;
	else return tpi;
#else	/* NSFPLAYER */
	return -1;
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	__inline void	MAPINT	SetCHR_RAM1 (int Bank, int Val)
{
#ifndef	NSFPLAYER
	PPU.CHRPointer[Bank] = CHR_RAM[Val & 0x1F];
	PPU.Writable[Bank] = TRUE;
#ifdef	ENABLE_DEBUGGER
	if (Bank & 8)
		Debugger::NTabChanged = TRUE;
	else	Debugger::PatChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	SetCHR_RAM2 (int Bank, int Val)
{
	Val <<= 1;
	SetCHR_RAM1(Bank+0,Val+0);
	SetCHR_RAM1(Bank+1,Val+1);
}
static	void	MAPINT	SetCHR_RAM4 (int Bank, int Val)
{
	Val <<= 2;
	SetCHR_RAM1(Bank+0,Val+0);
	SetCHR_RAM1(Bank+1,Val+1);
	SetCHR_RAM1(Bank+2,Val+2);
	SetCHR_RAM1(Bank+3,Val+3);
}
static	void	MAPINT	SetCHR_RAM8 (int Bank, int Val)
{
	Val <<= 3;
	SetCHR_RAM1(Bank+0,Val+0);
	SetCHR_RAM1(Bank+1,Val+1);
	SetCHR_RAM1(Bank+2,Val+2);
	SetCHR_RAM1(Bank+3,Val+3);
	SetCHR_RAM1(Bank+4,Val+4);
	SetCHR_RAM1(Bank+5,Val+5);
	SetCHR_RAM1(Bank+6,Val+6);
	SetCHR_RAM1(Bank+7,Val+7);
}
static	int	MAPINT	GetCHR_RAM1 (int Bank)	/* -1 if no ROM mapped */
{
#ifndef	NSFPLAYER
	int tpi = (int)((PPU.CHRPointer[Bank] - CHR_RAM[0]) >> 10);
	if ((tpi < 0) || (tpi > MAX_CHRRAM_MASK)) return -1;
	else return tpi;
#else	/* NSFPLAYER */
	return -1;
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	__inline void	MAPINT	SetCHR_NT1 (int Bank, int Val)
{
#ifndef	NSFPLAYER
	PPU.CHRPointer[Bank] = PPU_VRAM[Val & 3];
	PPU.Writable[Bank] = TRUE;
#ifdef	ENABLE_DEBUGGER
	if (Bank & 8)
		Debugger::NTabChanged = TRUE;
	else	Debugger::PatChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif	/* !NSFPLAYER */
}
static	int	MAPINT	GetCHR_NT1 (int Bank)	/* -1 if no ROM mapped */
{
#ifndef	NSFPLAYER
	int tpi = (int)((PPU.CHRPointer[Bank] - PPU_VRAM[0]) >> 10);
	if ((tpi < 0) || (tpi > 4)) return -1;
	else return tpi;
#else	/* NSFPLAYER */
	return -1;
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	unsigned char *	MAPINT	GetCHR_Ptr1 (int Bank)
{
#ifndef	NSFPLAYER
	return PPU.CHRPointer[Bank];
#else	/* NSFPLAYER */
	return NULL;
#endif	/* !NSFPLAYER */
}

static	void	MAPINT	SetCHR_Ptr1 (int Bank, unsigned char *Data, BOOL Writable)
{
#ifndef	NSFPLAYER
	PPU.CHRPointer[Bank] = Data;
	PPU.Writable[Bank] = Writable;
#ifdef	ENABLE_DEBUGGER
	if (Bank & 8)
		Debugger::NTabChanged = TRUE;
	else	Debugger::PatChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif	/* !NSFPLAYER */
}

static	void	MAPINT	SetCHR_OB1 (int Bank)
{
#ifndef	NSFPLAYER
	PPU.CHRPointer[Bank] = PPU_OpenBus;
	PPU.Writable[Bank] = FALSE;
#ifdef	ENABLE_DEBUGGER
	if (Bank & 8)
		Debugger::NTabChanged = TRUE;
	else	Debugger::PatChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	void	MAPINT	Mirror_H (void)
{
#ifndef	NSFPLAYER
	SetCHR_NT1(0x8,0);	SetCHR_NT1(0xC,0);
	SetCHR_NT1(0x9,0);	SetCHR_NT1(0xD,0);
	SetCHR_NT1(0xA,1);	SetCHR_NT1(0xE,1);
	SetCHR_NT1(0xB,1);	SetCHR_NT1(0xF,1);
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	Mirror_V (void)
{
#ifndef	NSFPLAYER
	SetCHR_NT1(0x8,0);	SetCHR_NT1(0xC,0);
	SetCHR_NT1(0x9,1);	SetCHR_NT1(0xD,1);
	SetCHR_NT1(0xA,0);	SetCHR_NT1(0xE,0);
	SetCHR_NT1(0xB,1);	SetCHR_NT1(0xF,1);
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	Mirror_4 (void)
{
#ifndef	NSFPLAYER
	SetCHR_NT1(0x8,0);	SetCHR_NT1(0xC,0);
	SetCHR_NT1(0x9,1);	SetCHR_NT1(0xD,1);
	SetCHR_NT1(0xA,2);	SetCHR_NT1(0xE,2);
	SetCHR_NT1(0xB,3);	SetCHR_NT1(0xF,3);
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	Mirror_S0 (void)
{
#ifndef	NSFPLAYER
	SetCHR_NT1(0x8,0);	SetCHR_NT1(0xC,0);
	SetCHR_NT1(0x9,0);	SetCHR_NT1(0xD,0);
	SetCHR_NT1(0xA,0);	SetCHR_NT1(0xE,0);
	SetCHR_NT1(0xB,0);	SetCHR_NT1(0xF,0);
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	Mirror_S1 (void)
{
#ifndef	NSFPLAYER
	SetCHR_NT1(0x8,1);	SetCHR_NT1(0xC,1);
	SetCHR_NT1(0x9,1);	SetCHR_NT1(0xD,1);
	SetCHR_NT1(0xA,1);	SetCHR_NT1(0xE,1);
	SetCHR_NT1(0xB,1);	SetCHR_NT1(0xF,1);
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	Mirror_Custom (int M1, int M2, int M3, int M4)
{
#ifndef	NSFPLAYER
	SetCHR_NT1(0x8,M1);	SetCHR_NT1(0xC,M1);
	SetCHR_NT1(0x9,M2);	SetCHR_NT1(0xD,M2);
	SetCHR_NT1(0xA,M3);	SetCHR_NT1(0xE,M3);
	SetCHR_NT1(0xB,M4);	SetCHR_NT1(0xF,M4);
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	void	MAPINT	Set_SRAMSize (int Size)	/* Sets the size of the SRAM (in bytes) and clears PRG_RAM */
{
#ifndef	NSFPLAYER
	NES.SRAM_Size = Size;
	memset(PRG_RAM,0,sizeof(PRG_RAM));
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	void	MAPINT	SetIRQ (int IRQstate)
{
	if (IRQstate)
		CPU::WantIRQ &= ~IRQ_EXTERNAL;
	else	CPU::WantIRQ |= IRQ_EXTERNAL;
}

static	void	MAPINT	DbgOut (TCHAR *text, ...)
{
#ifndef	NSFPLAYER
	static TCHAR txt[1024];
	va_list marker;
	va_start(marker,text);
	_vstprintf(txt,text,marker);
	va_end(marker);
	AddDebug(txt);
#endif	/* !NSFPLAYER */
}

static	void	MAPINT	StatusOut (TCHAR *text, ...)
{
#ifndef	NSFPLAYER
	static TCHAR txt[1024];
	va_list marker;
	va_start(marker,text);
	_vstprintf(txt,text,marker);
	va_end(marker);
	PrintTitlebar(txt);
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

void	MapperInterface_Init (void)
{
#ifndef	NSFPLAYER
	WIN32_FIND_DATA Data;
	HANDLE Handle;
	struct tMapperDLL *ThisDLL;
	TCHAR Filename[MAX_PATH], Path[MAX_PATH];
	_tcscpy(Path,ProgPath);
	_tcscat(Path,_T("Mappers\\"));
	_stprintf(Filename,_T("%s%s"),Path,_T("*.dll"));
	Handle = FindFirstFile(Filename,&Data);
	ThisDLL = (struct tMapperDLL *)malloc(sizeof(struct tMapperDLL));
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR Tmp[MAX_PATH];
			_stprintf(Tmp,_T("%s%s"),Path,Data.cFileName);
			ThisDLL->dInst = LoadLibrary(Tmp);
			ThisDLL->LoadDLL = (PLoadMapperDLL)GetProcAddress(ThisDLL->dInst,"LoadMapperDLL");
			ThisDLL->UnloadDLL = (PUnloadMapperDLL)GetProcAddress(ThisDLL->dInst,"UnloadMapperDLL");
			if ((ThisDLL->LoadDLL) && (ThisDLL->UnloadDLL))
			{
				ThisDLL->DI = ThisDLL->LoadDLL(hMainWnd,&EI,CurrentMapperInterface);
				if (ThisDLL->DI)
				{
					DbgOut(_T("Added mapper pack %s: '%s' v%X.%X (%04X/%02X/%02X)"), Data.cFileName, ThisDLL->DI->Description, ThisDLL->DI->Version >> 16, ThisDLL->DI->Version & 0xFFFF, ThisDLL->DI->Date >> 16, (ThisDLL->DI->Date >> 8) & 0xFF, ThisDLL->DI->Date & 0xFF);
					ThisDLL->Next = MapperDLLs;
					MapperDLLs = ThisDLL;
					ThisDLL = (struct tMapperDLL *)malloc(sizeof(struct tMapperDLL));
				}
				else	FreeLibrary(ThisDLL->dInst);
			}
			else	FreeLibrary(ThisDLL->dInst);
		}	while (FindNextFile(Handle,&Data));
		FindClose(Handle);
	}
	free(ThisDLL);
	if (MapperDLLs == NULL)
		MessageBox(hMainWnd,_T("Fatal error: unable to locate any mapper DLLs!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
#else	/* NSFPLAYER */
	dInst = LoadLibrary("Plugins\\nsf.dll");
	LoadDLL = (PLoadMapperDLL)GetProcAddress(dInst,"LoadMapperDLL");
	UnloadDLL = (PUnloadMapperDLL)GetProcAddress(dInst,"UnloadMapperDLL");
	if (!LoadDLL)
		MessageBox(mod.hMainWindow,"Fatal error: unable to locate NSF player mapper DLL!","in_nintendulator",MB_OK | MB_ICONERROR);
	if (!UnloadDLL)
		MessageBox(mod.hMainWindow,"Fatal error: unable to locate NSF player mapper DLL!","in_nintendulator",MB_OK | MB_ICONERROR);
	DI = LoadDLL(mod.hMainWindow,&EI,CurrentMapperInterface);
	if (!DI)
		MessageBox(mod.hMainWindow,"Fatal error: unable to locate NSF player mapper DLL!","in_nintendulator",MB_OK | MB_ICONERROR);
#endif	/* !NSFPLAYER */
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
	EI.OpenBus = &CPU::LastRead;
	MI = NULL;
#ifndef	NSFPLAYER
	MI2 = NULL;
#endif	/* !NSFPLAYER */
}

#ifndef	NSFPLAYER
INT_PTR CALLBACK	DllSelect (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static PDLLInfo *DLLs;
	int i;
	switch (message)
	{
	case WM_INITDIALOG:
		DLLs = (PDLLInfo *)lParam;
		for (i = 0; DLLs[i] != NULL; i++)
		{
			TCHAR *desc = (TCHAR *)malloc(sizeof(TCHAR) * (_tcslen(DLLs[i]->Description) + 32));
			_stprintf(desc,_T("\"%s\" v%x.%x (%04x/%02x/%02x)"),
				DLLs[i]->Description,
				(DLLs[i]->Version >> 16) & 0xFFFF, DLLs[i]->Version & 0xFFFF,
				(DLLs[i]->Date >> 16) & 0xFFFF, (DLLs[i]->Date >> 8) & 0xFF, DLLs[i]->Date & 0xFF);
			SendDlgItemMessage(hDlg, IDC_DLL_LIST, LB_INSERTSTRING, i, (LPARAM)desc);
			free(desc);
		}
		return TRUE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_DLL_LIST:
			if (HIWORD(wParam) != LBN_DBLCLK)
				break;	// have double-clicks fall through
		case IDOK:
			i = SendDlgItemMessage(hDlg, IDC_DLL_LIST, LB_GETCURSEL, 0, 0);
			if (i == LB_ERR)
				EndDialog(hDlg, (INT_PTR)NULL);
			else	EndDialog(hDlg, (INT_PTR)DLLs[i]);
			break;
		case IDCANCEL:
			EndDialog(hDlg, (INT_PTR)NULL);
			break;
		}
	}
	return FALSE;
}
#endif	/* !NSFPLAYER */

BOOL	MapperInterface_LoadMapper (CPROMInfo ROM)
{
#ifndef	NSFPLAYER
	PDLLInfo *DLLs;
	int num = 1;
	struct tMapperDLL *ThisDLL = MapperDLLs;
	while (ThisDLL)
	{	// 1 - count how many DLLs we have (add 1 for null terminator)
		num++;
		ThisDLL = ThisDLL->Next;
	}

	DLLs = (PDLLInfo *)malloc(num * sizeof(PDLLInfo));
	num = 0;
	ThisDLL = MapperDLLs;
	while (ThisDLL)
	{	// 2 - see how many DLLs support our mapper
		DI = ThisDLL->DI;
		if (DI->LoadMapper(ROM))
		{
			DI->UnloadMapper();
			DLLs[num++] = DI;
		}
		ThisDLL = ThisDLL->Next;
	}
	if (num == 0)
	{	// none found
		DI = NULL;
		free(DLLs);
		return FALSE;
	}
	if (num == 1)
	{	// 1 found
		DI = DLLs[0];
		MI = DI->LoadMapper(ROM);
		if (MI->Load)
			MI->Load();
		free(DLLs);
		return TRUE;
	}
	// else more than one found
	DLLs[num] = NULL;
	DI = (PDLLInfo)DialogBoxParam(hInst,(LPCTSTR)IDD_DLLSELECT,hMainWnd,DllSelect,(LPARAM)DLLs);
	if (DI)
	{
		MI = DI->LoadMapper(ROM);
		if (MI->Load)
			MI->Load();
		free(DLLs);
		return TRUE;
	}
	free(DLLs);
#else	/* NSFPLAYER */
	if (!DI)
		return FALSE;
	MI = DI->LoadMapper(ROM);
	if (MI)
		return TRUE;
#endif	/* !NSFPLAYER */
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
#endif	/* !NSFPLAYER */
	}
}

void	MapperInterface_Release (void)
{
#ifndef	NSFPLAYER
	struct tMapperDLL *ThisDLL = MapperDLLs;
	while (ThisDLL)
	{
		MapperDLLs = ThisDLL->Next;
		ThisDLL->UnloadDLL();
		FreeLibrary(ThisDLL->dInst);
		free(ThisDLL);
		ThisDLL = MapperDLLs;
	}
#else	/* NSFPLAYER */
	DI = NULL;
	UnloadDLL();
	FreeLibrary(dInst);
#endif	/* !NSFPLAYER */
}
