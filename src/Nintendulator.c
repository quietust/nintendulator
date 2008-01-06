/*
Nintendulator - A Win32 NES emulator written in C.
Designed for maximum emulation accuracy.
Copyright (c) 2002-2006  QMT Productions

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

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "MapperInterface.h"
#include "NES.h"
#include "CPU.h"
#include "PPU.h"
#include "APU.h"
#include "GFX.h"
#include "AVI.h"
#include "Debugger.h"
#include "Movie.h"
#include "Controllers.h"
#include "States.h"
#include <shellapi.h>
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE	hInst;		// current instance
HWND		hMainWnd;		// main window
HMENU		hMenu;		// main window menu
HACCEL		hAccelTable;	// accelerators
int		SizeMult;	// size multiplier
TCHAR		ProgPath[MAX_PATH];	// program path
TCHAR		Path_ROM[MAX_PATH];	// current ROM directory
TCHAR		Path_NMV[MAX_PATH];	// current movie directory
TCHAR		Path_AVI[MAX_PATH];	// current AVI directory
TCHAR		Path_PAL[MAX_PATH];	// current palette directory
BOOL		MaskKeyboard = FALSE;	// mask keyboard accelerators (for when Family Basic Keyboard is active)
HWND		hDebug;		// Debug Info window
BOOL		dbgVisible;	// whether or not the Debug window is open

TCHAR	szTitle[MAX_LOADSTRING];	// The title bar text
TCHAR	szWindowClass[MAX_LOADSTRING];	// The title bar text


// Foward declarations of functions included in this code module:
ATOM			MyRegisterClass(HINSTANCE hInstance);
BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	DebugWnd(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	InesHeader(HWND, UINT, WPARAM, LPARAM);
void	ShowDebug (void);

TCHAR	TitlebarBuffer[256];
int	TitlebarDelay;

int APIENTRY	_tWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	size_t i;
	MSG msg;

	// Initialize global strings
	LoadString(hInstance,IDS_APP_TITLE,szTitle,MAX_LOADSTRING);
	LoadString(hInstance,IDS_NINTENDULATOR,szWindowClass,MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	GetModuleFileName(NULL,ProgPath,MAX_PATH);
	for (i = _tcslen(ProgPath); (i > 0) && (ProgPath[i] != _T('\\')); i--)
		ProgPath[i] = 0;

	// Perform application initialization:
	if (!InitInstance (hInstance,nCmdShow)) 
		return FALSE;

	hAccelTable = LoadAccelerators(hInstance,(LPCTSTR)IDC_NINTENDULATOR);

	timeBeginPeriod(1);

	if (lpCmdLine[0])
	{
		// grab a copy of the command line parms - we're gonna need to extract a single filename from it
		TCHAR *cmdline = _tcsdup(lpCmdLine);
		// and remember the pointer so we can safely free it at the end
		TCHAR *bkptr = cmdline;

		// if the filename is in quotes, strip them off (and ignore everything past it)
		if (cmdline[0] == _T('"'))
		{
			cmdline++;
			// yes, it IS possible for the second quote to not exist!
			if (_tcschr(cmdline,'"'))
				*_tcschr(cmdline,'"') = 0;
		}
		// otherwise just kill everything past the first space
		else if (_tcschr(cmdline,' '))
			*_tcschr(cmdline,' ') = 0;

		NES_OpenFile(cmdline);
		free(bkptr);	// free up the memory from its original pointer
	}

	// Main message loop:
	while (GetMessage(&msg,NULL,0,0))
	{
		if (MaskKeyboard || !TranslateAccelerator(msg.hwnd,hAccelTable,&msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	timeEndPeriod(1);

	return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM	MyRegisterClass (HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style		= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon		= LoadIcon(hInstance,(LPCTSTR)IDI_NINTENDULATOR);
	wcex.hCursor		= LoadCursor(NULL,IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_NINTENDULATOR;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance,(LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL	InitInstance (HINSTANCE hInstance, int nCmdShow)
{
	GFX.DirectDraw = NULL;	// gotta do this so we don't paint from nothing
	hInst = hInstance;
	hMenu = LoadMenu(hInst,(LPCTSTR)IDR_NINTENDULATOR);
	if (!(hMainWnd = CreateWindow(szWindowClass,szTitle,WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,0,0,NULL,hMenu,hInstance,NULL)))
		return FALSE;
	ShowWindow(hMainWnd,nCmdShow);
	DragAcceptFiles(hMainWnd,TRUE);

	hDebug = CreateDialog(hInst,(LPCTSTR)IDD_DEBUG,hMainWnd,DebugWnd);
	SetWindowPos(hDebug,hMainWnd,0,0,0,0,SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOSIZE);

	NES_Init();
	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK	WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	int wmId, wmEvent;
	TCHAR FileName[MAX_PATH];
	OPENFILENAME ofn;
	BOOL running = NES.Running;

	switch (message) 
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_FILE_EXIT:
			SendMessage(hWnd,WM_CLOSE,0,0);
			break;
		case ID_FILE_OPEN:
			FileName[0] = 0;
			ZeroMemory(&ofn,sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hMainWnd;
			ofn.hInstance = hInst;
			ofn.lpstrFilter =	_T("All supported files (*.NES, *.UNIF, *.UNF, *.FDS, *.NSF)\0")
							_T("*.NES;*.UNIF;*.UNF;*.FDS;*.NSF\0")
						_T("iNES ROM Images (*.NES)\0")
							_T("*.NES\0")
						_T("Universal NES Interchange Format ROM files (*.UNIF, *.UNF)\0")
							_T("*.UNF;*.UNIF\0")
						_T("Famicom Disk System Disk Images (*.FDS)\0")
							_T("*.FDS\0")
						_T("NES Sound Files (*.NSF)\0")
							_T("*.NSF\0")
						_T("\0");
			ofn.lpstrCustomFilter = NULL;
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = FileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = Path_ROM;
			ofn.lpstrTitle = _T("Load ROM");
			ofn.Flags = OFN_FILEMUSTEXIST;
			ofn.lpstrDefExt = NULL;
			ofn.lCustData = 0;
			ofn.lpfnHook = NULL;
			ofn.lpTemplateName = NULL;
			if (GetOpenFileName(&ofn))
			{
				_tcscpy(Path_ROM,FileName);
				Path_ROM[ofn.nFileOffset-1] = 0;
				NES_Stop();
				NES_OpenFile(FileName);
			}
			break;
		case ID_FILE_CLOSE:
			NES_Stop();
			NES_CloseFile();
			break;
		case ID_FILE_HEADER:
			FileName[0] = 0;
			ZeroMemory(&ofn,sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hMainWnd;
			ofn.hInstance = hInst;
			ofn.lpstrFilter = _T("iNES ROM Images (*.NES)\0") _T("*.NES\0") _T("\0");
			ofn.lpstrCustomFilter = NULL;
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = FileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = Path_ROM;
			ofn.lpstrTitle = _T("Edit Header");
			ofn.Flags = OFN_FILEMUSTEXIST;
			ofn.lpstrDefExt = NULL;
			ofn.lCustData = 0;
			ofn.lpfnHook = NULL;
			ofn.lpTemplateName = NULL;

			if (GetOpenFileName(&ofn))
				DialogBoxParam(hInst,(LPCTSTR)IDD_INESHEADER,hWnd,InesHeader,(LPARAM)FileName);
			break;
		case ID_FILE_AUTORUN:
			NES.AutoRun = !NES.AutoRun;
			if (NES.AutoRun)
				CheckMenuItem(hMenu,ID_FILE_AUTORUN,MF_CHECKED);
			else	CheckMenuItem(hMenu,ID_FILE_AUTORUN,MF_UNCHECKED);
			break;

		case ID_CPU_RUN:
			NES_Start(FALSE);
			break;
		case ID_CPU_STEP:
			NES_Stop();		// need to stop first
			NES_Start(TRUE);	// so the 'start' makes it through
			break;
		case ID_CPU_STOP:
			NES_Stop();
			break;
		case ID_CPU_SOFTRESET:
			NES_Stop();
			if (Movie.Mode)
				Movie_Stop();
			NES_Reset(RESET_SOFT);
			if (running)
				NES_Start(FALSE);
			break;
		case ID_CPU_HARDRESET:
			NES_Stop();
			if (Movie.Mode)
				Movie_Stop();
			NES_Reset(RESET_HARD);
			if (running)
				NES_Start(FALSE);
			break;
		case ID_CPU_SAVESTATE:
			NES_Stop();
			while (PPU.SLnum != 240)
			{
				if (NES.FrameStep && !NES.GotStep)
					MessageBox(hMainWnd,_T("Impossible: savestate is advancing to scanline 240 in framestep mode!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
				do
				{
#ifdef ENABLE_DEBUGGER
					if (Debugger.Enabled)
						Debugger_AddInst();
#endif	/* ENABLE_DEBUGGER */
					CPU_ExecOp();
				} while (!NES.Scanline);
				NES.Scanline = FALSE;
			}
#ifdef ENABLE_DEBUGGER
			if (Debugger.Enabled)
				Debugger_Update();
#endif	/* ENABLE_DEBUGGER */
			States_SaveState();
			if (running)	NES_Start(FALSE);
			break;
		case ID_CPU_LOADSTATE:
			NES_Stop();
			States_LoadState();
			if (running)	NES_Start(FALSE);
			break;
		case ID_CPU_PREVSTATE:
			States.SelSlot += 9;
			States.SelSlot %= 10;
			States_SetSlot(States.SelSlot);
			break;
		case ID_CPU_NEXTSTATE:
			States.SelSlot += 1;
			States.SelSlot %= 10;
			States_SetSlot(States.SelSlot);
			break;
		case ID_CPU_GAMEGENIE:
			NES.GameGenie = !NES.GameGenie;
			if (NES.GameGenie)
				CheckMenuItem(hMenu,ID_CPU_GAMEGENIE,MF_CHECKED);
			else	CheckMenuItem(hMenu,ID_CPU_GAMEGENIE,MF_UNCHECKED);
			break;
		case ID_CPU_FRAMESTEP_ENABLED:
			NES.FrameStep = !NES.FrameStep;
			if (NES.FrameStep)
				CheckMenuItem(hMenu,ID_CPU_FRAMESTEP_ENABLED,MF_CHECKED);
			else	CheckMenuItem(hMenu,ID_CPU_FRAMESTEP_ENABLED,MF_UNCHECKED);
			break;
		case ID_CPU_FRAMESTEP_STEP:
			NES.GotStep = TRUE;
			break;
		case ID_PPU_FRAMESKIP_AUTO:
			GFX.aFSkip = !GFX.aFSkip;
			GFX_SetFrameskip(-1);
			break;
		case ID_PPU_FRAMESKIP_0:
			GFX_SetFrameskip(0);
			break;
		case ID_PPU_FRAMESKIP_1:
			GFX_SetFrameskip(1);
			break;
		case ID_PPU_FRAMESKIP_2:
			GFX_SetFrameskip(2);
			break;
		case ID_PPU_FRAMESKIP_3:
			GFX_SetFrameskip(3);
			break;
		case ID_PPU_FRAMESKIP_4:
			GFX_SetFrameskip(4);
			break;
		case ID_PPU_FRAMESKIP_5:
			GFX_SetFrameskip(5);
			break;
		case ID_PPU_FRAMESKIP_6:
			GFX_SetFrameskip(6);
			break;
		case ID_PPU_FRAMESKIP_7:
			GFX_SetFrameskip(7);
			break;
		case ID_PPU_FRAMESKIP_8:
			GFX_SetFrameskip(8);
			break;
		case ID_PPU_FRAMESKIP_9:
			GFX_SetFrameskip(9);
			break;
		case ID_PPU_SIZE_1X:
			SizeMult = 1;
			NES_UpdateInterface();
			CheckMenuRadioItem(hMenu,ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_1X,MF_BYCOMMAND);
			break;
		case ID_PPU_SIZE_2X:
			SizeMult = 2;
			NES_UpdateInterface();
			CheckMenuRadioItem(hMenu,ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_2X,MF_BYCOMMAND);
			break;
		case ID_PPU_SIZE_3X:
			SizeMult = 3;
			NES_UpdateInterface();
			CheckMenuRadioItem(hMenu,ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_3X,MF_BYCOMMAND);
			break;
		case ID_PPU_SIZE_4X:
			SizeMult = 4;
			NES_UpdateInterface();
			CheckMenuRadioItem(hMenu,ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_4X,MF_BYCOMMAND);
			break;
		case ID_PPU_MODE_NTSC:
			NES_Stop();
			NES_SetCPUMode(0);
			CheckMenuRadioItem(hMenu,ID_PPU_MODE_NTSC,ID_PPU_MODE_PAL,ID_PPU_MODE_NTSC,MF_BYCOMMAND);
			if (running)	NES_Start(FALSE);
			break;
		case ID_PPU_MODE_PAL:
			NES_Stop();
			NES_SetCPUMode(1);
			CheckMenuRadioItem(hMenu,ID_PPU_MODE_NTSC,ID_PPU_MODE_PAL,ID_PPU_MODE_PAL,MF_BYCOMMAND);
			if (running)	NES_Start(FALSE);
			break;
		case ID_PPU_PALETTE:
			GFX_PaletteConfig();
			break;
		case ID_PPU_SLOWDOWN_ENABLED:
			GFX.SlowDown = !GFX.SlowDown;
			if (GFX.SlowDown)
				CheckMenuItem(hMenu,ID_PPU_SLOWDOWN_ENABLED,MF_CHECKED);
			else	CheckMenuItem(hMenu,ID_PPU_SLOWDOWN_ENABLED,MF_UNCHECKED);
			break;
		case ID_PPU_SLOWDOWN_2:
			GFX.SlowRate = 2;
			CheckMenuRadioItem(hMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_2,MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_3:
			GFX.SlowRate = 3;
			CheckMenuRadioItem(hMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_3,MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_4:
			GFX.SlowRate = 4;
			CheckMenuRadioItem(hMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_4,MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_5:
			GFX.SlowRate = 5;
			CheckMenuRadioItem(hMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_5,MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_10:
			GFX.SlowRate = 10;
			CheckMenuRadioItem(hMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_10,MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_20:
			GFX.SlowRate = 20;
			CheckMenuRadioItem(hMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_20,MF_BYCOMMAND);
			break;
		case ID_PPU_FULLSCREEN:
			NES_Stop();
			GFX_Release();
			GFX.Fullscreen = !GFX.Fullscreen;
			GFX_Create();
			if (running)
				NES_Start(FALSE);
			break;
		case ID_PPU_SCANLINES:
			NES_Stop();
			GFX_Release();
			GFX.Scanlines = !GFX.Scanlines;
			GFX_Create();
			if (running)
				NES_Start(FALSE);
			if (GFX.Scanlines)
				CheckMenuItem(hMenu,ID_PPU_SCANLINES,MF_CHECKED);
			else	CheckMenuItem(hMenu,ID_PPU_SCANLINES,MF_UNCHECKED);
			break;
		case ID_SOUND_ENABLED:
			if (NES.SoundEnabled = !NES.SoundEnabled)
			{
				if (running)	APU_SoundON();
				CheckMenuItem(hMenu,ID_SOUND_ENABLED,MF_CHECKED);
			}
			else
			{
				if (running)	APU_SoundOFF();
				CheckMenuItem(hMenu,ID_SOUND_ENABLED,MF_UNCHECKED);
			}
			break;
		case ID_INPUT_SETUP:
			NES_Stop();
			Controllers_OpenConfig();
			if (running)	NES_Start(FALSE);
			break;
#ifdef ENABLE_DEBUGGER
		case ID_DEBUG_LEVEL1:
			Debugger_SetMode(0);
			CheckMenuRadioItem(hMenu,ID_DEBUG_LEVEL1,ID_DEBUG_LEVEL4,ID_DEBUG_LEVEL1,MF_BYCOMMAND);
			break;
		case ID_DEBUG_LEVEL2:
			Debugger_SetMode(1);
			CheckMenuRadioItem(hMenu,ID_DEBUG_LEVEL1,ID_DEBUG_LEVEL4,ID_DEBUG_LEVEL2,MF_BYCOMMAND);
			break;
		case ID_DEBUG_LEVEL3:
			Debugger_SetMode(2);
			CheckMenuRadioItem(hMenu,ID_DEBUG_LEVEL1,ID_DEBUG_LEVEL4,ID_DEBUG_LEVEL3,MF_BYCOMMAND);
			break;
		case ID_DEBUG_LEVEL4:
			Debugger_SetMode(3);
			CheckMenuRadioItem(hMenu,ID_DEBUG_LEVEL1,ID_DEBUG_LEVEL4,ID_DEBUG_LEVEL4,MF_BYCOMMAND);
			break;
		case ID_DEBUG_STATWND:
			ShowDebug();
			break;
#endif	/* ENABLE_DEBUGGER */
		case ID_GAME:
			NES_MapperConfig();
			break;
		case ID_MISC_STARTAVICAPTURE:
			AVI_Start();
			break;
		case ID_MISC_STOPAVICAPTURE:
			AVI_End();
			break;
		case ID_MISC_PLAYMOVIE:
			Movie_Play(FALSE);
			break;
		case ID_MISC_RECORDMOVIE:
			Movie_Record(FALSE);
			break;
		case ID_MISC_RECORDSTATE:
			Movie_Record(TRUE);
			break;
		case ID_MISC_RESUMEMOVIE:
			Movie_Play(TRUE);
			break;
		case ID_MISC_STOPMOVIE:
			Movie_Stop();
			break;
		case ID_HELP_ABOUT:
			DialogBox(hInst,(LPCTSTR)IDD_ABOUTBOX,hWnd,(DLGPROC)About);
			break;
		default:return DefWindowProc(hWnd,message,wParam,lParam);
			break;
		}
		break;
	case WM_DROPFILES:
		DragQueryFile((HDROP)wParam,0,FileName,MAX_PATH);
		DragFinish((HDROP)wParam);
		NES_Stop();
		NES_OpenFile(FileName);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd,&ps);
		if (!NES.Running)
			GFX_Repaint();
		EndPaint(hWnd,&ps);
		break;
	case WM_CLOSE:
		NES_Stop();
		NES_Release();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd,message,wParam,lParam);
		break;
	}
	return 0;
}

LRESULT CALLBACK	About (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

static TCHAR *DebugText;
static int DebugLen;
LRESULT CALLBACK	DebugWnd (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		dbgVisible = TRUE;
		SendDlgItemMessage(hDlg,IDC_DEBUGTEXT,EM_SETLIMITTEXT,0,0);
		DebugLen = 8192;
		DebugText = malloc(DebugLen * sizeof(TCHAR));
		return FALSE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			dbgVisible = FALSE;
			ShowWindow(hDlg,SW_HIDE);
			return FALSE;
		}
		break;
	case WM_SIZE:
		MoveWindow(GetDlgItem(hDlg,IDC_DEBUGTEXT),0,0,LOWORD(lParam),HIWORD(lParam),TRUE);
		break;
	}
	return FALSE;
}
void	AddDebug (TCHAR *txt)
{
	int i = _tcsclen(txt), j = GetWindowTextLength(GetDlgItem(hDebug,IDC_DEBUGTEXT));
	if (!dbgVisible)
		return;
	if (i + j + 2 > DebugLen)
	{
		while (i + j + 2 > DebugLen)
			DebugLen += 8192;
		DebugText = realloc(DebugText,DebugLen * sizeof(TCHAR));
	}
	GetDlgItemText(hDebug,IDC_DEBUGTEXT,DebugText,DebugLen);
	_tcscat(DebugText,txt);
	_tcscat(DebugText,_T("\r\n"));
	SetDlgItemText(hDebug,IDC_DEBUGTEXT,DebugText);
	SendDlgItemMessage(hDebug,IDC_DEBUGTEXT,EM_SETSEL,i+j+2,-1);	/* select last char, move caret to end */
	SendDlgItemMessage(hDebug,IDC_DEBUGTEXT,EM_SCROLLCARET,0,0);	/* scroll caret onto screen */
}
void	ShowDebug (void)
{
	if (dbgVisible)
		return;
	dbgVisible = TRUE;
	ShowWindow(hDebug,SW_SHOW);
}

LRESULT CALLBACK	InesHeader (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static TCHAR *filename;
	static char header[16];
	int i;
	FILE *rom;
	TCHAR name[MAX_PATH];
	switch (message)
	{
	case WM_INITDIALOG:
		filename = (TCHAR *)lParam;
		rom = _tfopen(filename,_T("rb"));
		if (!rom)
		{
			MessageBox(hMainWnd,_T("Unable to open ROM!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
			EndDialog(hDlg,0);
		}
		fread(header,16,1,rom);
		fclose(rom);
		if (memcmp(header,"NES\x1A",4))
		{
			MessageBox(hMainWnd,_T("Selected file is not an iNES ROM image!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
			EndDialog(hDlg,0);
		}
		if ((header[7] & 0x0C) == 0x08)
		{
			MessageBox(hMainWnd,_T("Selected ROM contains iNES 2.0 information, which is not yet supported. Please use a stand-alone editor."),_T("Nintendulator"),MB_OK | MB_ICONERROR);
			EndDialog(hDlg,0);
		}
		if ((header[7] & 0x0C) == 0x04)
		{
			MessageBox(hMainWnd,_T("Selected ROM appears to have been corrupted by \"DiskDude!\" - cleaning..."),_T("Nintendulator"),MB_OK | MB_ICONWARNING);
			for (i = 7; i < 16; i++)
				header[i] = 0;
		}
		else if (((header[8] || header[9] || header[10] || header[11] || header[12] || header[13] || header[14] || header[15])) && 
			(MessageBox(hDlg,_T("Unrecognized data has been detected in the reserved region of this ROM's header! Do you wish to clean it?"),_T("Nintendulator"),MB_YESNO | MB_ICONQUESTION) == IDYES))
		{
			for (i = 7; i < 16; i++)
				header[i] = 0;
		}

		i = _tcslen(filename)-1;
		while ((i >= 0) && (filename[i] != _T('\\')))
			i--;
		_tcscpy(name,filename+i+1);
		name[_tcslen(name)-4] = 0;
		SetDlgItemText(hDlg,IDC_INES_NAME,name);
		SetDlgItemInt(hDlg,IDC_INES_PRG,header[4],FALSE);
		SetDlgItemInt(hDlg,IDC_INES_CHR,header[5],FALSE);
		SetDlgItemInt(hDlg,IDC_INES_MAP,((header[6] >> 4) & 0xF) | (header[7] & 0xF0),FALSE);
		CheckDlgButton(hDlg,IDC_INES_BATT,(header[6] & 0x02) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_INES_TRAIN,(header[6] & 0x04) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_INES_4SCR,(header[6] & 0x08) ? BST_CHECKED : BST_UNCHECKED);
		if (header[6] & 0x01)
			CheckRadioButton(hDlg,IDC_INES_HORIZ,IDC_INES_VERT,IDC_INES_VERT);
		else	CheckRadioButton(hDlg,IDC_INES_HORIZ,IDC_INES_VERT,IDC_INES_HORIZ);
		CheckDlgButton(hDlg,IDC_INES_VS,(header[7] & 0x01) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_INES_PC10,(header[7] & 0x02) ? BST_CHECKED : BST_UNCHECKED);
		if (IsDlgButtonChecked(hDlg,IDC_INES_4SCR))
		{
			EnableWindow(GetDlgItem(hDlg,IDC_INES_HORIZ),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_INES_VERT),FALSE);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg,IDC_INES_HORIZ),TRUE);
			EnableWindow(GetDlgItem(hDlg,IDC_INES_VERT),TRUE);
		}
		if (IsDlgButtonChecked(hDlg,IDC_INES_VS))
			EnableWindow(GetDlgItem(hDlg,IDC_INES_PC10),FALSE);
		else if (IsDlgButtonChecked(hDlg,IDC_INES_PC10))
			EnableWindow(GetDlgItem(hDlg,IDC_INES_VS),FALSE);
		else
		{
			EnableWindow(GetDlgItem(hDlg,IDC_INES_VS),TRUE);
			EnableWindow(GetDlgItem(hDlg,IDC_INES_PC10),TRUE);
		}
		return TRUE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_INES_PRG:
			i = GetDlgItemInt(hDlg,IDC_INES_PRG,NULL,FALSE);
			if ((i < 0) || (i > 255))
				SetDlgItemInt(hDlg,IDC_INES_PRG,i = 0,FALSE);
			header[4] = i & 0xFF;
			break;
		case IDC_INES_CHR:
			i = GetDlgItemInt(hDlg,IDC_INES_CHR,NULL,FALSE);
			if ((i < 0) || (i > 255))
				SetDlgItemInt(hDlg,IDC_INES_CHR,i = 0,FALSE);
			header[5] = i & 0xFF;
			break;
		case IDC_INES_MAP:
			i = GetDlgItemInt(hDlg,IDC_INES_MAP,NULL,FALSE);
			if ((i < 0) || (i > 255))
				SetDlgItemInt(hDlg,IDC_INES_MAP,i = 0,FALSE);
			header[6] = (header[6] & 0x0F) | ((i & 0x0F) << 4);
			header[7] = (header[7] & 0x0F) | (i & 0xF0);
			break;
		case IDC_INES_BATT:
			if (IsDlgButtonChecked(hDlg,IDC_INES_BATT))
				header[6] |= 0x02;
			else	header[6] &= ~0x02;
			break;
		case IDC_INES_TRAIN:
			if (IsDlgButtonChecked(hDlg,IDC_INES_TRAIN))
			{
				MessageBox(hDlg,_T("Trained ROMs are not supported in Nintendulator!"),_T("Nintendulator"),MB_OK | MB_ICONWARNING);
				header[6] |= 0x04;
			}
			else	header[6] &= ~0x04;
			break;
		case IDC_INES_4SCR:
			if (IsDlgButtonChecked(hDlg,IDC_INES_4SCR))
			{
				EnableWindow(GetDlgItem(hDlg,IDC_INES_HORIZ),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_INES_VERT),FALSE);
				header[6] |= 0x08;
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg,IDC_INES_HORIZ),TRUE);
				EnableWindow(GetDlgItem(hDlg,IDC_INES_VERT),TRUE);
				header[6] &= ~0x08;
			}
			break;
		case IDC_INES_HORIZ:
			if (IsDlgButtonChecked(hDlg,IDC_INES_HORIZ))
				header[6] &= ~0x01;
			break;
		case IDC_INES_VERT:
			if (IsDlgButtonChecked(hDlg,IDC_INES_VERT))
				header[6] |= 0x01;
			break;
		case IDC_INES_VS:
			if (IsDlgButtonChecked(hDlg,IDC_INES_VS))
			{
				EnableWindow(GetDlgItem(hDlg,IDC_INES_PC10),FALSE);
				header[7] |= 0x01;
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg,IDC_INES_PC10),TRUE);
				header[7] &= ~0x01;
			}
			break;
		case IDC_INES_PC10:
			if (IsDlgButtonChecked(hDlg,IDC_INES_PC10))
			{
				EnableWindow(GetDlgItem(hDlg,IDC_INES_VS),FALSE);
				header[7] |= 0x02;
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg,IDC_INES_VS),TRUE);
				header[7] &= ~0x02;
			}
			break;
		case IDOK:
			rom = _tfopen(filename,_T("r+b"));
			if (rom)
			{
				fwrite(header,16,1,rom);
				fclose(rom);
			}
			else	MessageBox(hMainWnd,_T("Failed to open ROM!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
			// fall through
		case IDCANCEL:
			EndDialog(hDlg,0);
			break;
		}
	}
	return FALSE;
}

BOOL	ProcessMessages (void)
{
	MSG msg;
	BOOL gotMessage = PeekMessage(&msg,NULL,0,0,PM_NOREMOVE);
	while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
	{
		if (MaskKeyboard || !TranslateAccelerator(msg.hwnd,hAccelTable,&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return gotMessage;
}

void	SetWindowClientArea (HWND hWnd, int w, int h)
{
	RECT client;
	SetWindowPos(hWnd,hWnd,0,0,w,h,SWP_NOMOVE | SWP_NOZORDER);
	GetClientRect(hWnd,&client);
	SetWindowPos(hWnd,hWnd,0,0,2 * w - client.right,2 * h - client.bottom,SWP_NOMOVE | SWP_NOZORDER);
}

void	UpdateTitlebar (void)
{
	TCHAR titlebar[256];
	if (NES.Running)
		_stprintf(titlebar,_T("Nintendulator - %i FPS (%i %sFSkip)"),GFX.FPSnum,GFX.FSkip,GFX.aFSkip?_T("Auto"):_T(""));
	else	_tcscpy(titlebar,_T("Nintendulator - Stopped"));
	if (TitlebarDelay > 0)
	{
		TitlebarDelay--;
		_tcscat(titlebar,_T(" - "));
		_tcscat(titlebar,TitlebarBuffer);
	}
	SetWindowText(hMainWnd,titlebar);
}
void	__cdecl	PrintTitlebar (TCHAR *Text, ...)
{
	va_list marker;
	va_start(marker,Text);
	_vstprintf(TitlebarBuffer,Text,marker);
	va_end(marker);
	TitlebarDelay = 15;
	UpdateTitlebar();
}
