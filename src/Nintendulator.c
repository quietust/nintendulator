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
#include "APU.h"
#include "GFX.h"
#include "AVI.h"
#include "Debugger.h"
#include "Controllers.h"
#include "States.h"
#include <commdlg.h>
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE	hInst;		// current instance
HWND		mWnd;		// main window
HACCEL		hAccelTable;	// accelerators
int		SizeMult;	// size multiplier
char		ProgPath[MAX_PATH];	// program path

TCHAR	szTitle[MAX_LOADSTRING];	// The title bar text
TCHAR	szWindowClass[MAX_LOADSTRING];	// The title bar text

// Foward declarations of functions included in this code module:
ATOM			MyRegisterClass(HINSTANCE hInstance);
BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY	WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	size_t i;
	MSG msg;

	// Initialize global strings
	LoadString(hInstance,IDS_APP_TITLE,szTitle,MAX_LOADSTRING);
	LoadString(hInstance,IDS_NINTENDULATOR,szWindowClass,MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	GetModuleFileName(NULL,ProgPath,MAX_PATH);
	for (i = strlen(ProgPath); (i > 0) && (ProgPath[i] != '\\'); i--)
		ProgPath[i] = 0;

	// Perform application initialization:
	if (!InitInstance (hInstance,nCmdShow)) 
		return FALSE;

	hAccelTable = LoadAccelerators(hInstance,(LPCTSTR)IDC_NINTENDULATOR);

	if (lpCmdLine[0])
	{
		char *cmdline = strdup(lpCmdLine);
		if (strchr(cmdline,'"'))
			cmdline = strchr(cmdline,'"') + 1;
		if (strchr(cmdline,'"'))
			*strchr(cmdline,'"') = 0;
		NES_OpenFile(cmdline);
		free(cmdline);
	}

	timeBeginPeriod(1);
	// Main message loop:
	while (GetMessage(&msg,NULL,0,0))
	{
		if (!TranslateAccelerator(msg.hwnd,hAccelTable,&msg)) 
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
	wcex.lpszMenuName	= (LPCSTR)IDC_NINTENDULATOR;
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
	if (!(mWnd = CreateWindow(szWindowClass,szTitle,WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,0,0,NULL,LoadMenu(hInst,(LPCTSTR)IDR_NINTENDULATOR),hInstance,NULL)))
		return FALSE;
	ShowWindow(mWnd,nCmdShow);
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
	HMENU MyMenu;
	HDC hdc;
	PAINTSTRUCT ps;
	int wmId, wmEvent;

	switch (message) 
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		MyMenu = GetMenu(mWnd);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst,(LPCTSTR)IDD_ABOUTBOX,hWnd,(DLGPROC)About);
			break;
		case IDM_EXIT:
			SendMessage(hWnd,WM_CLOSE,0,0);
			break;
		case ID_FILE_OPEN:
		{
			char FileName[256] = {0};
			OPENFILENAME ofn;

			if ((!NES.Stopped) && (NES.SoundEnabled))
				APU_SoundOFF();

			ZeroMemory(&ofn,sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = mWnd;
			ofn.hInstance = hInst;
			ofn.lpstrFilter =	"All supported files (*.NES, *.UNIF, *.UNF, *.FDS, *.NSF, *.ZIP)\0"
							"*.NES;*.UNIF;*.UNF;*.FDS;*.NSF;*.ZIP\0"
						"iNES ROM Images (*.NES)\0"
							"*.NES\0"
						"Universal NES Interchange Format ROM files (*.UNIF, *.UNF)\0"
							"*.UNF;*.UNIF\0"
						"Famicom Disk System Disk Images (*.FDS)\0"
							"*.FDS\0"
						"NES Sound Files (*.NSF)\0"
							"*.NSF\0"
						"Compressed ROM Image (*.ZIP)\0"
							"*.ZIP\0"
						"\0";
			ofn.lpstrCustomFilter = NULL;
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = FileName;
			ofn.nMaxFile = 256;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = "";
			ofn.lpstrTitle = "Load ROM";
			ofn.Flags = OFN_FILEMUSTEXIST;
			ofn.lpstrDefExt = NULL;
			ofn.lCustData = 0;
			ofn.lpfnHook = NULL;
			ofn.lpTemplateName = NULL;
			if (GetOpenFileName(&ofn))
			{
				NES.Stop = TRUE;
				NES_OpenFile(FileName);
			}
			else if ((!NES.Stopped) && (NES.SoundEnabled))
				APU_SoundON();
		}	break;
		case ID_FILE_CLOSE:
			if (!NES.ROMLoaded)
				break;
			if (!NES.Stopped)
			{
				NES.Stop = TRUE;
				NES.ROMLoaded = FALSE;
			}
			else	NES_CloseFile();
			break;
		case ID_CPU_RUN:
			if (!NES.ROMLoaded)
				break;
			if (NES.Stopped)
			{
				NES.Stop = FALSE;
				NES_Run();
			}
			break;
		case ID_CPU_STEP:
			if (!NES.ROMLoaded)
				break;
			NES.Stop = TRUE;
			if (NES.Stopped)
				NES_Run();
			break;
		case ID_CPU_STOP:
			if (!NES.ROMLoaded)
				break;
			NES.Stop = TRUE;
			break;
		case ID_CPU_SOFTRESET:
			if (!NES.ROMLoaded)
				break;
			if (NES.Stopped)
				NES_Reset(RESET_SOFT);
			else
			{
				NES.Stop = TRUE;
				NES.NeedReset = RESET_SOFT;
			}
			break;
		case ID_CPU_HARDRESET:
			if (!NES.ROMLoaded)
				break;
			if (NES.Stopped)
				NES_Reset(RESET_HARD);
			else
			{
				NES.Stop = TRUE;
				NES.NeedReset = RESET_HARD;
			}
			break;
		case ID_CPU_GAMEGENIE:
			NES.GameGenie = !NES.GameGenie;
			if (NES.GameGenie)
				CheckMenuItem(MyMenu,ID_CPU_GAMEGENIE,MF_CHECKED);
			else	CheckMenuItem(MyMenu,ID_CPU_GAMEGENIE,MF_UNCHECKED);
			break;
		case ID_CPU_AUTORUN:
			NES.AutoRun = !NES.AutoRun;
			if (NES.AutoRun)
				CheckMenuItem(MyMenu,ID_CPU_AUTORUN,MF_CHECKED);
			else	CheckMenuItem(MyMenu,ID_CPU_AUTORUN,MF_UNCHECKED);
			break;
		case ID_PPU_SIZE_1X:
			SizeMult = 1;
			NES_UpdateInterface();
			CheckMenuRadioItem(MyMenu,ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_1X,MF_BYCOMMAND);
			break;
		case ID_PPU_SIZE_2X:
			SizeMult = 2;
			NES_UpdateInterface();
			CheckMenuRadioItem(MyMenu,ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_2X,MF_BYCOMMAND);
			break;
		case ID_PPU_SIZE_3X:
			SizeMult = 3;
			NES_UpdateInterface();
			CheckMenuRadioItem(MyMenu,ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_3X,MF_BYCOMMAND);
			break;
		case ID_PPU_SIZE_4X:
			SizeMult = 4;
			NES_UpdateInterface();
			CheckMenuRadioItem(MyMenu,ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_4X,MF_BYCOMMAND);
			break;
		case ID_PPU_FRAMESKIP_AUTO:
			if (GFX.aFSkip = !GFX.aFSkip)
				CheckMenuItem(MyMenu,ID_PPU_FRAMESKIP_AUTO,MF_CHECKED);
			else	CheckMenuItem(MyMenu,ID_PPU_FRAMESKIP_AUTO,MF_UNCHECKED);
			break;
		case ID_PPU_FRAMESKIP_0:
			GFX.FSkip = 0;
			CheckMenuRadioItem(MyMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_0,MF_BYCOMMAND);
			break;
		case ID_PPU_FRAMESKIP_1:
			GFX.FSkip = 1;
			CheckMenuRadioItem(MyMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_1,MF_BYCOMMAND);
			break;
		case ID_PPU_FRAMESKIP_2:
			GFX.FSkip = 2;
			CheckMenuRadioItem(MyMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_2,MF_BYCOMMAND);
			break;
		case ID_PPU_FRAMESKIP_3:
			GFX.FSkip = 3;
			CheckMenuRadioItem(MyMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_3,MF_BYCOMMAND);
			break;
		case ID_PPU_FRAMESKIP_4:
			GFX.FSkip = 4;
			CheckMenuRadioItem(MyMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_4,MF_BYCOMMAND);
			break;
		case ID_PPU_FRAMESKIP_5:
			GFX.FSkip = 5;
			CheckMenuRadioItem(MyMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_5,MF_BYCOMMAND);
			break;
		case ID_PPU_FRAMESKIP_6:
			GFX.FSkip = 6;
			CheckMenuRadioItem(MyMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_6,MF_BYCOMMAND);
			break;
		case ID_PPU_FRAMESKIP_7:
			GFX.FSkip = 7;
			CheckMenuRadioItem(MyMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_7,MF_BYCOMMAND);
			break;
		case ID_PPU_FRAMESKIP_8:
			GFX.FSkip = 8;
			CheckMenuRadioItem(MyMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_8,MF_BYCOMMAND);
			break;
		case ID_PPU_FRAMESKIP_9:
			GFX.FSkip = 9;
			CheckMenuRadioItem(MyMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_9,MF_BYCOMMAND);
			break;
#ifdef ENABLE_DEBUGGER
		case ID_DEBUG_LEVEL1:
			Debugger_SetMode(0);
			CheckMenuRadioItem(MyMenu,ID_DEBUG_LEVEL1,ID_DEBUG_LEVEL4,ID_DEBUG_LEVEL1,MF_BYCOMMAND);
			break;
		case ID_DEBUG_LEVEL2:
			Debugger_SetMode(1);
			CheckMenuRadioItem(MyMenu,ID_DEBUG_LEVEL1,ID_DEBUG_LEVEL4,ID_DEBUG_LEVEL2,MF_BYCOMMAND);
			break;
		case ID_DEBUG_LEVEL3:
			Debugger_SetMode(2);
			CheckMenuRadioItem(MyMenu,ID_DEBUG_LEVEL1,ID_DEBUG_LEVEL4,ID_DEBUG_LEVEL3,MF_BYCOMMAND);
			break;
		case ID_DEBUG_LEVEL4:
			Debugger_SetMode(3);
			CheckMenuRadioItem(MyMenu,ID_DEBUG_LEVEL1,ID_DEBUG_LEVEL4,ID_DEBUG_LEVEL4,MF_BYCOMMAND);
			break;
#endif	/* ENABLE_DEBUGGER */
		case ID_CPU_SAVESTATE:
			if (!NES.ROMLoaded)
				break;
			States.NeedSave = TRUE;
			if (NES.Stopped)
			{
				do
				{
					do
					{
						CPU_ExecOp();
#ifdef ENABLE_DEBUGGER
						if (Debugger.Enabled)
							Debugger_AddInst();
#endif	/* ENABLE_DEBUGGER */
					} while (!NES.Scanline);
					NES.Scanline = FALSE;
				} while (PPU.SLnum <= 240);
#ifdef ENABLE_DEBUGGER
				if (Debugger.Enabled)
					Debugger_Update();
#endif	/* ENABLE_DEBUGGER */
				States_SaveState();
			}
			break;
		case ID_CPU_LOADSTATE:
			if (!NES.ROMLoaded)
				break;
			States.NeedLoad = TRUE;
			if (NES.Stopped)
				States_LoadState();
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
		case ID_SOUND_ENABLED:
			if (NES.SoundEnabled = !NES.SoundEnabled)
			{
				if (!NES.Stopped)
					APU_SoundON();
				CheckMenuItem(MyMenu,ID_SOUND_ENABLED,MF_CHECKED);
			}
			else
			{
				if (!NES.Stopped)
					APU_SoundOFF();
				CheckMenuItem(MyMenu,ID_SOUND_ENABLED,MF_UNCHECKED);
			}
			break;
		case ID_PPU_MODE_NTSC:
			NES_SetCPUMode(0);
			CheckMenuRadioItem(MyMenu,ID_PPU_MODE_NTSC,ID_PPU_MODE_PAL,ID_PPU_MODE_NTSC,MF_BYCOMMAND);
			break;
		case ID_PPU_MODE_PAL:
			NES_SetCPUMode(1);
			CheckMenuRadioItem(MyMenu,ID_PPU_MODE_NTSC,ID_PPU_MODE_PAL,ID_PPU_MODE_PAL,MF_BYCOMMAND);
			break;
		case ID_INPUT_SETUP:
			Controllers_OpenConfig();
			break;
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
			Controllers_PlayMovie();
			break;
		case ID_MISC_RECORDMOVIE:
			Controllers_RecordMovie();
			break;
		case ID_MISC_STOPMOVIE:
			Controllers_StopMovie();
			break;
		default:return DefWindowProc(hWnd,message,wParam,lParam);
			break;
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd,&ps);
		NES_Repaint();
		EndPaint(hWnd,&ps);
		break;
	case WM_ENTERMENULOOP:
	case WM_ENTERSIZEMOVE:
		if ((!NES.Stopped) && (NES.SoundEnabled))
			APU_SoundOFF();
		break;
	case WM_EXITMENULOOP:
	case WM_EXITSIZEMOVE:
		if ((!NES.Stopped) && (NES.SoundEnabled))
			APU_SoundON();
		break;
	case WM_CLOSE:
		if (NES.Stopped)
			NES_Release();
		else
		{
			NES.Stop = TRUE;
			NES.NeedQuit = TRUE;
		}
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

void	ProcessMessages (void)
{
	MSG msg;
	while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
//		if (!TranslateAccelerator(msg.hwnd,hAccelTable,&msg))
		if (!TranslateAccelerator(mWnd,hAccelTable,&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
}

void	SetWindowClientArea (HWND hWnd, int w, int h)
{
	RECT client;
	SetWindowPos(hWnd,hWnd,0,0,w,h,SWP_NOMOVE | SWP_NOZORDER);
	GetClientRect(hWnd,&client);
	SetWindowPos(hWnd,hWnd,0,0,2 * w - client.right,2 * h - client.bottom,SWP_NOMOVE | SWP_NOZORDER);
}