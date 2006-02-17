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
#include <shellapi.h>
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE	hInst;		// current instance
HWND		mWnd;		// main window
HACCEL		hAccelTable;	// accelerators
int		SizeMult;	// size multiplier
char		ProgPath[MAX_PATH];	// program path
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

	timeBeginPeriod(1);

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
	DragAcceptFiles(mWnd,TRUE);

	hDebug = CreateDialog(hInst,(LPCTSTR)IDD_DEBUG,mWnd,DebugWnd);
	SetWindowPos(hDebug,mWnd,0,0,0,0,SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOSIZE);

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
	char FileName[256];
	OPENFILENAME ofn;
	BOOL running = NES.Running;

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
			FileName[0] = 0;
			ZeroMemory(&ofn,sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = mWnd;
			ofn.hInstance = hInst;
			ofn.lpstrFilter =	"All supported files (*.NES, *.UNIF, *.UNF, *.FDS, *.NSF)\0"
							"*.NES;*.UNIF;*.UNF;*.FDS;*.NSF\0"
						"iNES ROM Images (*.NES)\0"
							"*.NES\0"
						"Universal NES Interchange Format ROM files (*.UNIF, *.UNF)\0"
							"*.UNF;*.UNIF\0"
						"Famicom Disk System Disk Images (*.FDS)\0"
							"*.FDS\0"
						"NES Sound Files (*.NSF)\0"
							"*.NSF\0"
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
			ofn.hwndOwner = mWnd;
			ofn.hInstance = hInst;
			ofn.lpstrFilter = "iNES ROM Images (*.NES)\0" "*.NES\0" "\0";
			ofn.lpstrCustomFilter = NULL;
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = FileName;
			ofn.nMaxFile = 256;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = "";
			ofn.lpstrTitle = "Edit Header";
			ofn.Flags = OFN_FILEMUSTEXIST;
			ofn.lpstrDefExt = NULL;
			ofn.lCustData = 0;
			ofn.lpfnHook = NULL;
			ofn.lpTemplateName = NULL;
			if (GetOpenFileName(&ofn))
				DialogBoxParam(hInst,(LPCTSTR)IDD_INESHEADER,hWnd,InesHeader,(LPARAM)FileName);
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
			if (Controllers.MovieMode)
				Controllers_StopMovie();
			NES_Reset(RESET_SOFT);
			if (running)
				NES_Start(FALSE);
			break;
		case ID_CPU_HARDRESET:
			NES_Stop();
			if (Controllers.MovieMode)
				Controllers_StopMovie();
			NES_Reset(RESET_HARD);
			if (running)
				NES_Start(FALSE);
			break;
		case ID_CPU_GAMEGENIE:
			NES.GameGenie = !NES.GameGenie;
			if (NES.GameGenie)
				CheckMenuItem(MyMenu,ID_CPU_GAMEGENIE,MF_CHECKED);
			else	CheckMenuItem(MyMenu,ID_CPU_GAMEGENIE,MF_UNCHECKED);
			break;
		case ID_FILE_AUTORUN:
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
		case ID_DEBUG_STATWND:
			ShowDebug();
			break;
#endif	/* ENABLE_DEBUGGER */
		case ID_CPU_SAVESTATE:
			NES_Stop();
			while (PPU.SLnum != 240)
			{
				if (NES.FrameStep && !NES.GotStep)
					MessageBox(mWnd,"Impossible: savestate is advancing to scanline 240 in framestep mode!","Nintendulator",MB_OK | MB_ICONERROR);
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
		case ID_SOUND_ENABLED:
			if (NES.SoundEnabled = !NES.SoundEnabled)
			{
				if (running)	APU_SoundON();
				CheckMenuItem(MyMenu,ID_SOUND_ENABLED,MF_CHECKED);
			}
			else
			{
				if (running)	APU_SoundOFF();
				CheckMenuItem(MyMenu,ID_SOUND_ENABLED,MF_UNCHECKED);
			}
			break;
		case ID_PPU_MODE_NTSC:
			NES_Stop();
			NES_SetCPUMode(0);
			CheckMenuRadioItem(MyMenu,ID_PPU_MODE_NTSC,ID_PPU_MODE_PAL,ID_PPU_MODE_NTSC,MF_BYCOMMAND);
			if (running)	NES_Start(FALSE);
			break;
		case ID_PPU_MODE_PAL:
			NES_Stop();
			NES_SetCPUMode(1);
			CheckMenuRadioItem(MyMenu,ID_PPU_MODE_NTSC,ID_PPU_MODE_PAL,ID_PPU_MODE_PAL,MF_BYCOMMAND);
			if (running)	NES_Start(FALSE);
			break;
		case ID_PPU_PALETTE:
			GFX_PaletteConfig();
			break;
		case ID_INPUT_SETUP:
			NES_Stop();
			Controllers_OpenConfig();
			if (running)	NES_Start(FALSE);
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
			Controllers_PlayMovie(FALSE);
			break;
		case ID_MISC_RESUMEMOVIE:
			Controllers_PlayMovie(TRUE);
			break;
		case ID_MISC_RECORDMOVIE:
			Controllers_RecordMovie(FALSE);
			break;
		case ID_MISC_RECORDSTATE:
			Controllers_RecordMovie(TRUE);
			break;
		case ID_MISC_STOPMOVIE:
			Controllers_StopMovie();
			break;
		case ID_CPU_FRAMESTEP_ENABLED:
			NES.FrameStep = !NES.FrameStep;
			if (NES.FrameStep)
				CheckMenuItem(MyMenu,ID_CPU_FRAMESTEP_ENABLED,MF_CHECKED);
			else	CheckMenuItem(MyMenu,ID_CPU_FRAMESTEP_ENABLED,MF_UNCHECKED);
			break;
		case ID_CPU_FRAMESTEP_STEP:
			NES.GotStep = TRUE;
			break;
		case ID_PPU_SLOWDOWN_ENABLED:
			GFX.SlowDown = !GFX.SlowDown;
			if (GFX.SlowDown)
				CheckMenuItem(MyMenu,ID_PPU_SLOWDOWN_ENABLED,MF_CHECKED);
			else	CheckMenuItem(MyMenu,ID_PPU_SLOWDOWN_ENABLED,MF_UNCHECKED);
			break;
		case ID_PPU_SLOWDOWN_2:
			GFX.SlowRate = 2;
			CheckMenuRadioItem(MyMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_2,MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_3:
			GFX.SlowRate = 3;
			CheckMenuRadioItem(MyMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_3,MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_4:
			GFX.SlowRate = 4;
			CheckMenuRadioItem(MyMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_4,MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_5:
			GFX.SlowRate = 5;
			CheckMenuRadioItem(MyMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_5,MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_10:
			GFX.SlowRate = 10;
			CheckMenuRadioItem(MyMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_10,MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_20:
			GFX.SlowRate = 20;
			CheckMenuRadioItem(MyMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_20,MF_BYCOMMAND);
			break;
		default:return DefWindowProc(hWnd,message,wParam,lParam);
			break;
		}
		break;
	case WM_DROPFILES:
		DragQueryFile((HDROP)wParam,0,FileName,256);
		DragFinish((HDROP)wParam);
		NES_Stop();
		NES_OpenFile(FileName);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd,&ps);
		NES_Repaint();
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

static char *DebugText;
static int DebugLen;
LRESULT CALLBACK	DebugWnd (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		dbgVisible = TRUE;
		SendDlgItemMessage(hDlg,IDC_DEBUGTEXT,EM_SETLIMITTEXT,0,0);
		DebugText = malloc(8192);
		DebugLen = 8192;
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
void	AddDebug (char *txt)
{
	int i = strlen(txt), j = GetWindowTextLength(GetDlgItem(hDebug,IDC_DEBUGTEXT));
	if (!dbgVisible)
		return;
	if (i + j + 2 > DebugLen)
	{
		while (i + j + 2 > DebugLen)
			DebugLen += 8192;
		DebugText = realloc(DebugText,DebugLen);
	}
	GetDlgItemText(hDebug,IDC_DEBUGTEXT,DebugText,DebugLen);
	strcat(DebugText,txt);
	strcat(DebugText,"\r\n");
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
	static char *filename;
	static char header[16];
	int i;
	FILE *rom;
	char name[256];
	switch (message)
	{
	case WM_INITDIALOG:
		filename = (char *)lParam;
		rom = fopen(filename,"rb");
		if (!rom)
		{
			MessageBox(mWnd,"Unable to open ROM!","Nintendulator",MB_OK | MB_ICONERROR);
			EndDialog(hDlg,0);
		}
		fread(header,16,1,rom);
		fclose(rom);
		if (memcmp(header,"NES\x1A",4))
		{
			MessageBox(mWnd,"Selected file is not an iNES ROM image!","Nintendulator",MB_OK | MB_ICONERROR);
			EndDialog(hDlg,0);
		}
		i = strlen(filename)-1;
		while ((i >= 0) && (filename[i] != '\\'))
			i--;
		strcpy(name,filename+i+1);
		name[strlen(name)-4] = 0;
		SetDlgItemText(hDlg,IDC_INESNAME,name);
		SetDlgItemInt(hDlg,IDC_INESPRG,header[4],FALSE);
		SetDlgItemInt(hDlg,IDC_INESCHR,header[5],FALSE);
		SetDlgItemInt(hDlg,IDC_INESMAP,((header[6] >> 4) & 0xF) | (header[7] & 0xF0),FALSE);
		CheckDlgButton(hDlg,IDC_INESBATT,(header[6] & 0x02) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_INESTRAIN,(header[6] & 0x04) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_INES4SCR,(header[6] & 0x08) ? BST_CHECKED : BST_UNCHECKED);
		if (header[6] & 0x01)
			CheckRadioButton(hDlg,IDC_INESHORIZ,IDC_INESVERT,IDC_INESVERT);
		else	CheckRadioButton(hDlg,IDC_INESHORIZ,IDC_INESVERT,IDC_INESHORIZ);
		CheckDlgButton(hDlg,IDC_INESVS,(header[7] & 0x01) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_INESPC10,(header[7] & 0x02) ? BST_CHECKED : BST_UNCHECKED);
		if (IsDlgButtonChecked(hDlg,IDC_INES4SCR))
		{
			EnableWindow(GetDlgItem(hDlg,IDC_INESHORIZ),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_INESVERT),FALSE);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg,IDC_INESHORIZ),TRUE);
			EnableWindow(GetDlgItem(hDlg,IDC_INESVERT),TRUE);
		}
		if (IsDlgButtonChecked(hDlg,IDC_INESVS))
			EnableWindow(GetDlgItem(hDlg,IDC_INESPC10),FALSE);
		else if (IsDlgButtonChecked(hDlg,IDC_INESPC10))
			EnableWindow(GetDlgItem(hDlg,IDC_INESVS),FALSE);
		else
		{
			EnableWindow(GetDlgItem(hDlg,IDC_INESVS),TRUE);
			EnableWindow(GetDlgItem(hDlg,IDC_INESPC10),TRUE);
		}
		return TRUE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_INESPRG:
			i = GetDlgItemInt(hDlg,IDC_INESPRG,NULL,FALSE);
			if ((i < 0) || (i > 255))
				SetDlgItemInt(hDlg,IDC_INESPRG,0,FALSE);
			header[4] = i & 0xFF;
			break;
		case IDC_INESCHR:
			i = GetDlgItemInt(hDlg,IDC_INESCHR,NULL,FALSE);
			if ((i < 0) || (i > 255))
				SetDlgItemInt(hDlg,IDC_INESCHR,0,FALSE);
			header[5] = i & 0xFF;
			break;
		case IDC_INESMAP:
			i = GetDlgItemInt(hDlg,IDC_INESMAP,NULL,FALSE);
			if ((i < 0) || (i > 255))
				SetDlgItemInt(hDlg,IDC_INESMAP,0,FALSE);
			header[6] = (header[6] & 0x0F) | ((i & 0x0F) << 4);
			header[7] = (header[7] & 0x0F) | (i & 0xF0);
			break;
		case IDC_INESBATT:
			if (IsDlgButtonChecked(hDlg,IDC_INESBATT))
				header[6] |= 0x02;
			else	header[6] &= ~0x02;
			break;
		case IDC_INESTRAIN:
			if (IsDlgButtonChecked(hDlg,IDC_INESTRAIN))
			{
				MessageBox(hDlg,"Trained ROMs are not supported in Nintendulator!","Nintendulator",MB_OK | MB_ICONWARNING);
				header[6] |= 0x04;
			}
			else	header[6] &= ~0x04;
			break;
		case IDC_INES4SCR:
			if (IsDlgButtonChecked(hDlg,IDC_INES4SCR))
			{
				EnableWindow(GetDlgItem(hDlg,IDC_INESHORIZ),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_INESVERT),FALSE);
				header[6] |= 0x08;
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg,IDC_INESHORIZ),TRUE);
				EnableWindow(GetDlgItem(hDlg,IDC_INESVERT),TRUE);
				header[6] &= ~0x08;
			}
			break;
		case IDC_INESHORIZ:
			if (IsDlgButtonChecked(hDlg,IDC_INESHORIZ))
				header[6] &= ~0x01;
			break;
		case IDC_INESVERT:
			if (IsDlgButtonChecked(hDlg,IDC_INESVERT))
				header[6] |= 0x01;
			break;
		case IDC_INESVS:
			if (IsDlgButtonChecked(hDlg,IDC_INESVS))
			{
				EnableWindow(GetDlgItem(hDlg,IDC_INESPC10),FALSE);
				header[7] |= 0x01;
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg,IDC_INESPC10),TRUE);
				header[7] &= ~0x01;
			}
			break;
		case IDC_INESPC10:
			if (IsDlgButtonChecked(hDlg,IDC_INESPC10))
			{
				EnableWindow(GetDlgItem(hDlg,IDC_INESVS),FALSE);
				header[7] |= 0x02;
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg,IDC_INESVS),TRUE);
				header[7] &= ~0x02;
			}
			break;
		case IDOK:
			if (((header[8] || header[9] || header[10] || header[11] || header[12] || header[13] || header[14] || header[15])) && 
				(MessageBox(hDlg,"Garbage data has been detected in this ROM's header! Do you wish to remove it?","Nintendulator",MB_YESNO | MB_ICONQUESTION) == IDYES))
				memset(&header[8],0,8);
			rom = fopen(filename,"r+b");
			if (rom)
			{
				fwrite(header,16,1,rom);
				fclose(rom);
			}
			else	MessageBox(mWnd,"Failed to open ROM!","Nintendulator",MB_OK | MB_ICONERROR);
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