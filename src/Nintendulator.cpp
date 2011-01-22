/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * Based on NinthStar, a portable Win32 NES Emulator written in C++
 * Copyright (C) 2000  David de Regt
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $URL$
 * $Id$
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

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winmm.lib")

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
TCHAR		DataPath[MAX_PATH];	// user data path
BOOL		MaskKeyboard = FALSE;	// mask keyboard accelerators (for when Family Basic Keyboard is active)
HWND		hDebug;		// Debug Info window
BOOL		dbgVisible;	// whether or not the Debug window is open

TCHAR	szTitle[MAX_LOADSTRING];	// The title bar text
TCHAR	szWindowClass[MAX_LOADSTRING];	// The title bar text


// Foward declarations of functions included in this code module:
ATOM			MyRegisterClass(HINSTANCE hInstance);
BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	DebugWnd(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	InesHeader(HWND, UINT, WPARAM, LPARAM);

TCHAR	TitlebarBuffer[256];
int	TitlebarDelay;

int APIENTRY	_tWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	size_t i;
	MSG msg;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDS_NINTENDULATOR, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	GetModuleFileName(NULL, ProgPath, MAX_PATH);
	for (i = _tcslen(ProgPath); (i > 0) && (ProgPath[i] != _T('\\')); i--)
		ProgPath[i] = 0;

	// find our folder in Application Data, if it exists
	if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, DataPath)))
	{
		// if we can't even find that, then there's a much bigger problem...
		MessageBox(NULL, _T("FATAL: unable to locate Application Data folder"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	PathAppend(DataPath, _T("Nintendulator"));
	if (GetFileAttributes(DataPath) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory(DataPath, NULL);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
		return FALSE;

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_NINTENDULATOR);

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
			if (_tcschr(cmdline, '"'))
				*_tcschr(cmdline, '"') = 0;
		}
		// otherwise just kill everything past the first space
		else if (_tcschr(cmdline, ' '))
			*_tcschr(cmdline, ' ') = 0;

		NES::OpenFile(cmdline);
		// Allocated using _tcsdup()
		free(bkptr);	// free up the memory from its original pointer
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		HWND focus = GetForegroundWindow();
		if ((focus != hMainWnd) && IsDialogMessage(focus, &msg))
			continue;
		if (MaskKeyboard || !TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
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
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon		= LoadIcon(hInstance, (LPCTSTR)IDI_NINTENDULATOR);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_NINTENDULATOR;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

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
	GFX::DirectDraw = NULL;	// gotta do this so we don't paint from nothing
	hInst = hInstance;
	hMenu = LoadMenu(hInst, (LPCTSTR)IDR_NINTENDULATOR);
	hMainWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, hMenu, hInstance, NULL);
	if (!hMainWnd)
		return FALSE;
	ShowWindow(hMainWnd, nCmdShow);
	DragAcceptFiles(hMainWnd, TRUE);

	hDebug = CreateDialog(hInst, (LPCTSTR)IDD_DEBUG, hMainWnd, DebugWnd);

	NES::Init();
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
	BOOL running = NES::Running;

	switch (message) 
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_FILE_EXIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		case ID_FILE_OPEN:
			FileName[0] = 0;
			ZeroMemory(&ofn, sizeof(ofn));
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
				_tcscpy(Path_ROM, FileName);
				Path_ROM[ofn.nFileOffset-1] = 0;
				NES::Stop();
				NES::OpenFile(FileName);
			}
			break;
		case ID_FILE_CLOSE:
			NES::Stop();
			NES::CloseFile();
			break;
		case ID_FILE_HEADER:
			FileName[0] = 0;
			ZeroMemory(&ofn, sizeof(ofn));
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
				DialogBoxParam(hInst, (LPCTSTR)IDD_INESHEADER, hWnd, InesHeader, (LPARAM)FileName);
			break;
		case ID_FILE_AUTORUN:
			NES::AutoRun = !NES::AutoRun;
			if (NES::AutoRun)
				CheckMenuItem(hMenu, ID_FILE_AUTORUN, MF_CHECKED);
			else	CheckMenuItem(hMenu, ID_FILE_AUTORUN, MF_UNCHECKED);
			break;
		case ID_FILE_BROWSESAVES:
			ShellExecute(hMainWnd, NULL, DataPath, NULL, NULL, SW_SHOWNORMAL);
			break;

		case ID_CPU_RUN:
			NES::Start(FALSE);
			break;
		case ID_CPU_STEP:
			NES::Stop();		// need to stop first
			NES::Start(TRUE);	// so the 'start' makes it through
			break;
		case ID_CPU_STOP:
			NES::Stop();
			break;
		case ID_CPU_SOFTRESET:
			NES::Stop();
			if (Movie::Mode)
				Movie::Stop();
			NES::Reset(RESET_SOFT);
			if (running)
				NES::Start(FALSE);
			break;
		case ID_CPU_HARDRESET:
			NES::Stop();
			if (Movie::Mode)
				Movie::Stop();
			NES::Reset(RESET_HARD);
			if (running)
				NES::Start(FALSE);
			break;
		case ID_CPU_SAVESTATE:
			NES::Stop();
			while (PPU::SLnum != 240)
			{
				if (NES::FrameStep && !NES::GotStep)
					MessageBox(hMainWnd, _T("Impossible: savestate is advancing to scanline 240 in framestep mode!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
				do
				{
#ifdef	ENABLE_DEBUGGER
					if (Debugger::Enabled)
						Debugger::AddInst();
#endif	/* ENABLE_DEBUGGER */
					CPU::ExecOp();
				} while (!NES::Scanline);
				NES::Scanline = FALSE;
			}
#ifdef	ENABLE_DEBUGGER
			if (Debugger::Enabled)
				Debugger::Update(DEBUG_MODE_CPU | DEBUG_MODE_PPU);
#endif	/* ENABLE_DEBUGGER */
			States::SaveState();
			if (running)	NES::Start(FALSE);
			break;
		case ID_CPU_LOADSTATE:
			NES::Stop();
			States::LoadState();
			if (running)	NES::Start(FALSE);
			break;
		case ID_CPU_PREVSTATE:
			States::SelSlot += 9;
			States::SelSlot %= 10;
			States::SetSlot(States::SelSlot);
			break;
		case ID_CPU_NEXTSTATE:
			States::SelSlot += 1;
			States::SelSlot %= 10;
			States::SetSlot(States::SelSlot);
			break;
		case ID_CPU_GAMEGENIE:
			NES::GameGenie = !NES::GameGenie;
			if (NES::GameGenie)
				CheckMenuItem(hMenu, ID_CPU_GAMEGENIE, MF_CHECKED);
			else	CheckMenuItem(hMenu, ID_CPU_GAMEGENIE, MF_UNCHECKED);
			break;
		case ID_CPU_FRAMESTEP_ENABLED:
			NES::FrameStep = !NES::FrameStep;
			if (NES::FrameStep)
				CheckMenuItem(hMenu, ID_CPU_FRAMESTEP_ENABLED, MF_CHECKED);
			else	CheckMenuItem(hMenu, ID_CPU_FRAMESTEP_ENABLED, MF_UNCHECKED);
			break;
		case ID_CPU_FRAMESTEP_STEP:
			NES::GotStep = TRUE;
			break;
		case ID_PPU_FRAMESKIP_AUTO:
			GFX::aFSkip = !GFX::aFSkip;
			GFX::SetFrameskip(-1);
			break;
		case ID_PPU_FRAMESKIP_0:
			GFX::SetFrameskip(0);
			break;
		case ID_PPU_FRAMESKIP_1:
			GFX::SetFrameskip(1);
			break;
		case ID_PPU_FRAMESKIP_2:
			GFX::SetFrameskip(2);
			break;
		case ID_PPU_FRAMESKIP_3:
			GFX::SetFrameskip(3);
			break;
		case ID_PPU_FRAMESKIP_4:
			GFX::SetFrameskip(4);
			break;
		case ID_PPU_FRAMESKIP_5:
			GFX::SetFrameskip(5);
			break;
		case ID_PPU_FRAMESKIP_6:
			GFX::SetFrameskip(6);
			break;
		case ID_PPU_FRAMESKIP_7:
			GFX::SetFrameskip(7);
			break;
		case ID_PPU_FRAMESKIP_8:
			GFX::SetFrameskip(8);
			break;
		case ID_PPU_FRAMESKIP_9:
			GFX::SetFrameskip(9);
			break;
		case ID_PPU_SIZE_1X:
			SizeMult = 1;
			NES::UpdateInterface();
			CheckMenuRadioItem(hMenu, ID_PPU_SIZE_1X, ID_PPU_SIZE_4X, ID_PPU_SIZE_1X, MF_BYCOMMAND);
			break;
		case ID_PPU_SIZE_2X:
			SizeMult = 2;
			NES::UpdateInterface();
			CheckMenuRadioItem(hMenu, ID_PPU_SIZE_1X, ID_PPU_SIZE_4X, ID_PPU_SIZE_2X, MF_BYCOMMAND);
			break;
		case ID_PPU_SIZE_3X:
			SizeMult = 3;
			NES::UpdateInterface();
			CheckMenuRadioItem(hMenu, ID_PPU_SIZE_1X, ID_PPU_SIZE_4X, ID_PPU_SIZE_3X, MF_BYCOMMAND);
			break;
		case ID_PPU_SIZE_4X:
			SizeMult = 4;
			NES::UpdateInterface();
			CheckMenuRadioItem(hMenu, ID_PPU_SIZE_1X, ID_PPU_SIZE_4X, ID_PPU_SIZE_4X, MF_BYCOMMAND);
			break;
		case ID_PPU_MODE_NTSC:
			NES::Stop();
			NES::SetCPUMode(0);
			CheckMenuRadioItem(hMenu, ID_PPU_MODE_NTSC, ID_PPU_MODE_PAL, ID_PPU_MODE_NTSC, MF_BYCOMMAND);
			if (running)	NES::Start(FALSE);
			break;
		case ID_PPU_MODE_PAL:
			NES::Stop();
			NES::SetCPUMode(1);
			CheckMenuRadioItem(hMenu, ID_PPU_MODE_NTSC, ID_PPU_MODE_PAL, ID_PPU_MODE_PAL, MF_BYCOMMAND);
			if (running)	NES::Start(FALSE);
			break;
		case ID_PPU_PALETTE:
			GFX::PaletteConfig();
			break;
		case ID_PPU_SLOWDOWN_ENABLED:
			GFX::SlowDown = !GFX::SlowDown;
			if (GFX::SlowDown)
				CheckMenuItem(hMenu, ID_PPU_SLOWDOWN_ENABLED, MF_CHECKED);
			else	CheckMenuItem(hMenu, ID_PPU_SLOWDOWN_ENABLED, MF_UNCHECKED);
			break;
		case ID_PPU_SLOWDOWN_2:
			GFX::SlowRate = 2;
			CheckMenuRadioItem(hMenu, ID_PPU_SLOWDOWN_2, ID_PPU_SLOWDOWN_20, ID_PPU_SLOWDOWN_2, MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_3:
			GFX::SlowRate = 3;
			CheckMenuRadioItem(hMenu, ID_PPU_SLOWDOWN_2, ID_PPU_SLOWDOWN_20, ID_PPU_SLOWDOWN_3, MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_4:
			GFX::SlowRate = 4;
			CheckMenuRadioItem(hMenu, ID_PPU_SLOWDOWN_2, ID_PPU_SLOWDOWN_20, ID_PPU_SLOWDOWN_4, MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_5:
			GFX::SlowRate = 5;
			CheckMenuRadioItem(hMenu, ID_PPU_SLOWDOWN_2, ID_PPU_SLOWDOWN_20, ID_PPU_SLOWDOWN_5, MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_10:
			GFX::SlowRate = 10;
			CheckMenuRadioItem(hMenu, ID_PPU_SLOWDOWN_2, ID_PPU_SLOWDOWN_20, ID_PPU_SLOWDOWN_10, MF_BYCOMMAND);
			break;
		case ID_PPU_SLOWDOWN_20:
			GFX::SlowRate = 20;
			CheckMenuRadioItem(hMenu, ID_PPU_SLOWDOWN_2, ID_PPU_SLOWDOWN_20, ID_PPU_SLOWDOWN_20, MF_BYCOMMAND);
			break;
		case ID_PPU_FULLSCREEN:
			NES::Stop();
			GFX::Release();
			GFX::Fullscreen = !GFX::Fullscreen;
			GFX::Create();
			if (running)
				NES::Start(FALSE);
			break;
		case ID_PPU_SCANLINES:
			NES::Stop();
			GFX::Release();
			GFX::Scanlines = !GFX::Scanlines;
			GFX::Create();
			if (running)
				NES::Start(FALSE);
			if (GFX::Scanlines)
				CheckMenuItem(hMenu, ID_PPU_SCANLINES, MF_CHECKED);
			else	CheckMenuItem(hMenu, ID_PPU_SCANLINES, MF_UNCHECKED);
			break;
		case ID_SOUND_ENABLED:
			NES::SoundEnabled = !NES::SoundEnabled;
			if (NES::SoundEnabled)
			{
				if (running)
					APU::SoundON();
				CheckMenuItem(hMenu, ID_SOUND_ENABLED, MF_CHECKED);
			}
			else
			{
				if (running)
					APU::SoundOFF();
				CheckMenuItem(hMenu, ID_SOUND_ENABLED, MF_UNCHECKED);
			}
			break;
		case ID_INPUT_SETUP:
			NES::Stop();
			Controllers::OpenConfig();
			if (running)	NES::Start(FALSE);
			break;
#ifdef	ENABLE_DEBUGGER
		case ID_DEBUG_CPU:
			Debugger::SetMode(Debugger::Mode ^ DEBUG_MODE_CPU);
			break;
		case ID_DEBUG_PPU:
			Debugger::SetMode(Debugger::Mode ^ DEBUG_MODE_PPU);
			break;
#endif	/* ENABLE_DEBUGGER */
		case ID_DEBUG_STATWND:
			dbgVisible = !dbgVisible;
			if (dbgVisible)
				CheckMenuItem(hMenu, ID_DEBUG_STATWND, MF_CHECKED);
			else	CheckMenuItem(hMenu, ID_DEBUG_STATWND, MF_UNCHECKED);
			ShowWindow(hDebug, dbgVisible ? SW_SHOW : SW_HIDE);
			break;
		case ID_GAME:
			NES::MapperConfig();
			break;
		case ID_MISC_STARTAVICAPTURE:
			AVI::Start();
			break;
		case ID_MISC_STOPAVICAPTURE:
			AVI::End();
			break;
		case ID_MISC_PLAYMOVIE:
			Movie::Play();
			break;
		case ID_MISC_RECORDMOVIE:
			Movie::Record();
			break;
		case ID_MISC_STOPMOVIE:
			Movie::Stop();
			break;
		case ID_HELP_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		default:return DefWindowProc(hWnd, message, wParam, lParam);
			break;
		}
		break;
	case WM_DROPFILES:
		DragQueryFile((HDROP)wParam, 0, FileName, MAX_PATH);
		DragFinish((HDROP)wParam);
		NES::Stop();
		NES::OpenFile(FileName);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		if (!NES::Running)
			GFX::Repaint();
		EndPaint(hWnd, &ps);
		break;
	case WM_CLOSE:
		NES::Stop();
		NES::Release();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SYSCOMMAND:
		// disallow screen saver while emulating (doesn't work if password protected)
		if (running && (((wParam & 0xFFF0) == SC_SCREENSAVE) || ((wParam & 0xFFF0) == SC_MONITORPOWER)))
			return 0;
		// else fall through
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
	return 0;
}

INT_PTR CALLBACK	About (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK	DebugWnd (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg, IDC_DEBUGTEXT, EM_SETLIMITTEXT, 0, 0);
		SetWindowPos(hDlg, hMainWnd, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOOWNERZORDER | SWP_NOSIZE);
		return FALSE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			dbgVisible = FALSE;
			ShowWindow(hDebug, SW_HIDE);
			CheckMenuItem(hMenu, ID_DEBUG_STATWND, MF_UNCHECKED);
			return TRUE;
		}
		break;
	case WM_SIZE:
		MoveWindow(GetDlgItem(hDlg, IDC_DEBUGTEXT), 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return TRUE;
	}
	return FALSE;
}
void	AddDebug (TCHAR *txt)
{
	int dbglen = GetWindowTextLength(GetDlgItem(hDebug, IDC_DEBUGTEXT));
//	if (!dbgVisible)
//		return;
	if (dbglen)
	{
		TCHAR *newline = _T("\r\n");
		SendDlgItemMessage(hDebug, IDC_DEBUGTEXT, EM_SETSEL, dbglen, dbglen);
		SendDlgItemMessage(hDebug, IDC_DEBUGTEXT, EM_REPLACESEL, FALSE, (LPARAM)newline);
		dbglen += 2;
	}
	SendDlgItemMessage(hDebug, IDC_DEBUGTEXT, EM_SETSEL, dbglen, dbglen);
	SendDlgItemMessage(hDebug, IDC_DEBUGTEXT, EM_REPLACESEL, FALSE, (LPARAM)txt);
}

INT_PTR CALLBACK	InesHeader (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
		rom = _tfopen(filename, _T("rb"));
		if (!rom)
		{
			MessageBox(hMainWnd, _T("Unable to open ROM!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
			EndDialog(hDlg, 0);
			break;
		}
		fread(header, 16, 1, rom);
		fclose(rom);
		if (memcmp(header, "NES\x1A", 4))
		{
			MessageBox(hMainWnd, _T("Selected file is not an iNES ROM image!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
			EndDialog(hDlg, 0);
			break;
		}
		if ((header[7] & 0x0C) == 0x08)
		{
			MessageBox(hMainWnd, _T("Selected ROM contains iNES 2.0 information,  which is not yet supported. Please use a stand-alone editor."), _T("Nintendulator"), MB_OK | MB_ICONERROR);
			EndDialog(hDlg, 0);
			break;
		}
		if ((header[7] & 0x0C) == 0x04)
		{
			MessageBox(hMainWnd, _T("Selected ROM appears to have been corrupted by \"DiskDude!\" - cleaning..."), _T("Nintendulator"), MB_OK | MB_ICONWARNING);
			for (i = 7; i < 16; i++)
				header[i] = 0;
		}
		else if (((header[8] || header[9] || header[10] || header[11] || header[12] || header[13] || header[14] || header[15])) && 
			(MessageBox(hDlg, _T("Unrecognized data has been detected in the reserved region of this ROM's header! Do you wish to clean it?"), _T("Nintendulator"), MB_YESNO | MB_ICONQUESTION) == IDYES))
		{
			for (i = 7; i < 16; i++)
				header[i] = 0;
		}

		i = (int)_tcslen(filename)-1;
		while ((i >= 0) && (filename[i] != _T('\\')))
			i--;
		_tcscpy(name, filename+i+1);
		name[_tcslen(name)-4] = 0;
		SetDlgItemText(hDlg, IDC_INES_NAME, name);
		SetDlgItemInt(hDlg, IDC_INES_PRG, header[4], FALSE);
		SetDlgItemInt(hDlg, IDC_INES_CHR, header[5], FALSE);
		SetDlgItemInt(hDlg, IDC_INES_MAP, ((header[6] >> 4) & 0xF) | (header[7] & 0xF0), FALSE);
		CheckDlgButton(hDlg, IDC_INES_BATT, (header[6] & 0x02) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_INES_TRAIN, (header[6] & 0x04) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_INES_4SCR, (header[6] & 0x08) ? BST_CHECKED : BST_UNCHECKED);
		if (header[6] & 0x01)
			CheckRadioButton(hDlg, IDC_INES_HORIZ, IDC_INES_VERT, IDC_INES_VERT);
		else	CheckRadioButton(hDlg, IDC_INES_HORIZ, IDC_INES_VERT, IDC_INES_HORIZ);
		CheckDlgButton(hDlg, IDC_INES_VS, (header[7] & 0x01) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_INES_PC10, (header[7] & 0x02) ? BST_CHECKED : BST_UNCHECKED);
		if (IsDlgButtonChecked(hDlg, IDC_INES_4SCR))
		{
			EnableWindow(GetDlgItem(hDlg, IDC_INES_HORIZ), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_INES_VERT), FALSE);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg, IDC_INES_HORIZ), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_INES_VERT), TRUE);
		}
		if (IsDlgButtonChecked(hDlg, IDC_INES_VS))
			EnableWindow(GetDlgItem(hDlg, IDC_INES_PC10), FALSE);
		else if (IsDlgButtonChecked(hDlg, IDC_INES_PC10))
			EnableWindow(GetDlgItem(hDlg, IDC_INES_VS), FALSE);
		else
		{
			EnableWindow(GetDlgItem(hDlg, IDC_INES_VS), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_INES_PC10), TRUE);
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_INES_PRG:
			i = GetDlgItemInt(hDlg, IDC_INES_PRG, NULL, FALSE);
			if ((i < 0) || (i > 255))
				SetDlgItemInt(hDlg, IDC_INES_PRG, i = 0, FALSE);
			header[4] = (char)(i & 0xFF);
			return TRUE;
		case IDC_INES_CHR:
			i = GetDlgItemInt(hDlg, IDC_INES_CHR, NULL, FALSE);
			if ((i < 0) || (i > 255))
				SetDlgItemInt(hDlg, IDC_INES_CHR, i = 0, FALSE);
			header[5] = (char)(i & 0xFF);
			return TRUE;
		case IDC_INES_MAP:
			i = GetDlgItemInt(hDlg, IDC_INES_MAP, NULL, FALSE);
			if ((i < 0) || (i > 255))
				SetDlgItemInt(hDlg, IDC_INES_MAP, i = 0, FALSE);
			header[6] = (char)((header[6] & 0x0F) | ((i & 0x0F) << 4));
			header[7] = (char)((header[7] & 0x0F) | (i & 0xF0));
			return TRUE;
		case IDC_INES_BATT:
			if (IsDlgButtonChecked(hDlg, IDC_INES_BATT))
				header[6] |= 0x02;
			else	header[6] &= ~0x02;
			return TRUE;
		case IDC_INES_TRAIN:
			if (IsDlgButtonChecked(hDlg, IDC_INES_TRAIN))
			{
				MessageBox(hDlg, _T("Trained ROMs are not supported in Nintendulator!"), _T("Nintendulator"), MB_OK | MB_ICONWARNING);
				header[6] |= 0x04;
			}
			else	header[6] &= ~0x04;
			return TRUE;
		case IDC_INES_4SCR:
			if (IsDlgButtonChecked(hDlg, IDC_INES_4SCR))
			{
				EnableWindow(GetDlgItem(hDlg, IDC_INES_HORIZ), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_INES_VERT), FALSE);
				header[6] |= 0x08;
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg, IDC_INES_HORIZ), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_INES_VERT), TRUE);
				header[6] &= ~0x08;
			}
			return TRUE;
		case IDC_INES_HORIZ:
			if (IsDlgButtonChecked(hDlg, IDC_INES_HORIZ))
				header[6] &= ~0x01;
			return TRUE;
		case IDC_INES_VERT:
			if (IsDlgButtonChecked(hDlg, IDC_INES_VERT))
				header[6] |= 0x01;
			return TRUE;
		case IDC_INES_VS:
			if (IsDlgButtonChecked(hDlg, IDC_INES_VS))
			{
				EnableWindow(GetDlgItem(hDlg, IDC_INES_PC10), FALSE);
				header[7] |= 0x01;
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg, IDC_INES_PC10), TRUE);
				header[7] &= ~0x01;
			}
			return TRUE;
		case IDC_INES_PC10:
			if (IsDlgButtonChecked(hDlg, IDC_INES_PC10))
			{
				EnableWindow(GetDlgItem(hDlg, IDC_INES_VS), FALSE);
				header[7] |= 0x02;
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg, IDC_INES_VS), TRUE);
				header[7] &= ~0x02;
			}
			return TRUE;
		case IDOK:
			rom = _tfopen(filename, _T("r+b"));
			if (rom)
			{
				fwrite(header, 16, 1, rom);
				fclose(rom);
			}
			else	MessageBox(hMainWnd, _T("Failed to open ROM!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
			// fall through
		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL	ProcessMessages (void)
{
	MSG msg;
	BOOL gotMessage = PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (MaskKeyboard || !TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return gotMessage;
}

void	UpdateTitlebar (void)
{
	TCHAR titlebar[256];
	if (NES::Running)
		_stprintf(titlebar, _T("Nintendulator - %i FPS (%i %sFSkip)"), GFX::FPSnum, GFX::FSkip, GFX::aFSkip?_T("Auto"):_T(""));
	else	_tcscpy(titlebar, _T("Nintendulator - Stopped"));
	if (TitlebarDelay > 0)
	{
		TitlebarDelay--;
		_tcscat(titlebar, _T(" - "));
		_tcscat(titlebar, TitlebarBuffer);
	}
	SetWindowText(hMainWnd, titlebar);
}
void	__cdecl	PrintTitlebar (TCHAR *Text, ...)
{
	va_list marker;
	va_start(marker, Text);
	_vstprintf(TitlebarBuffer, Text, marker);
	va_end(marker);
	TitlebarDelay = 15;
	UpdateTitlebar();
}
