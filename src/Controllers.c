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

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "MapperInterface.h"
#include "Movie.h"
#include "Controllers.h"
#include <commdlg.h>

static	HWND key;
TCHAR *KeyLookup[256] =
{
	_T(""), _T("Escape"), _T("1"), _T("2"), _T("3"), _T("4"), _T("5"), _T("6"), _T("7"), _T("8"), _T("9"), _T("0"), _T("- (Keyboard)"), _T("="), _T("Backspace"), _T("Tab"),
	_T("Q"), _T("W"), _T("E"), _T("R"), _T("T"), _T("Y"), _T("U"), _T("I"), _T("O"), _T("P"), _T("["), _T("]"), _T("Return"), _T("Left Control"), _T("A"), _T("S"),
	_T("D"), _T("F"), _T("G"), _T("H"), _T("J"), _T("K"), _T("L"), _T(";"), _T("'"), _T("???"), _T("Left Shift"), _T("\\"), _T("Z"), _T("X"), _T("C"), _T("V"),
	_T("B"), _T("N"), _T("M"), _T(","), _T(". (Keyboard)"), _T("/ (Keyboard)"), _T("Right Shift"), _T("* (keypad)"), _T("Left Alt"), _T("Space"), _T("Caps Lock"), _T("F1"), _T("F2"), _T("F3"), _T("F4"), _T("F5"),
	_T("F6"), _T("F7"), _T("F8"), _T("F9"), _T("F10"), _T("Num Lock"), _T("Scroll Lock"), _T("Numpad 7"), _T("Numpad 8"), _T("Numpad 9"), _T("- (Numpad)"), _T("Numpad 4"), _T("Numpad 5"), _T("Numpad 6"), _T("+ (Numpad)"), _T("Numpad 1"),
	_T("Numpad 2"), _T("Numpad 3"), _T("Numpad 0"), _T(". (Numpad)"), _T("???"), _T("???"), _T("???"), _T("F11"), _T("F12"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"),
	_T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), 
	_T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), 
	_T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), 
	_T("Previous Track"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("Next Track"), _T("???"), _T("???"), _T("Numpad Enter"), _T("Right Control"), _T("???"), _T("???"),
	_T("Mute"), _T("Calculator"), _T("Play/Pause"), _T("???"), _T("Media Stop"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("Volume Down"), _T("???"),
	_T("Volume Up"), _T("???"), _T("Web Home"), _T("???"), _T("???"), _T("/ (Numpad)"), _T("???"), _T("SysRq"), _T("Right Alt"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"),
	_T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("Pause"), _T("???"), _T("Home"), _T("Up Arrow"), _T("PgUp"), _T("???"), _T("Left Arrow"), _T("???"), _T("Right Arrow"), _T("???"), _T("End"), 
	_T("Down Arrow"), _T("PgDn"), _T("Insert"), _T("Delete"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("Left Win Key"), _T("Right Win Key"), _T("App Menu Key"), _T("Power Button"), _T("Sleep Button"),
	_T("???"), _T("???"), _T("???"), _T("Wake Key"), _T("???"), _T("Web Search"), _T("Web Favorites"), _T("Web Refresh"), _T("Web Stop"), _T("Web Forward"), _T("Web Back"), _T("My Computer"), _T("Mail"), _T("Media Select"),_T("???"),_T("???"),
	_T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???"), _T("???")
};

struct	tControllers	Controllers;

void	StdPort_SetUnconnected		(struct tStdPort *);
void	StdPort_SetStdController	(struct tStdPort *);
void	StdPort_SetZapper		(struct tStdPort *);
void	StdPort_SetArkanoidPaddle	(struct tStdPort *);
void	StdPort_SetPowerPad		(struct tStdPort *);
void	StdPort_SetFourScore		(struct tStdPort *);
void	StdPort_SetSnesController	(struct tStdPort *);

void	StdPort_SetControllerType (struct tStdPort *Cont, int Type)
{
	Cont->Unload(Cont);
	switch (Cont->Type = Type) 
	{
	case STD_UNCONNECTED:		StdPort_SetUnconnected(Cont);		break;
	case STD_STDCONTROLLER:		StdPort_SetStdController(Cont);		break;
	case STD_ZAPPER:		StdPort_SetZapper(Cont);		break;
	case STD_ARKANOIDPADDLE:	StdPort_SetArkanoidPaddle(Cont);	break;
	case STD_POWERPAD:		StdPort_SetPowerPad(Cont);		break;
	case STD_FOURSCORE:		StdPort_SetFourScore(Cont);		break;
	case STD_SNESCONTROLLER:	StdPort_SetSnesController(Cont);	break;
	default:MessageBox(mWnd,_T("Error: selected invalid controller type for standard port!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);	break;
	}
}
TCHAR	*StdPort_Mappings[STD_MAX];
void	StdPort_SetMappings (void)
{
	StdPort_Mappings[STD_UNCONNECTED] = _T("Unconnected");
	StdPort_Mappings[STD_STDCONTROLLER] = _T("Standard Controller");
	StdPort_Mappings[STD_ZAPPER] = _T("Zapper");
	StdPort_Mappings[STD_ARKANOIDPADDLE] = _T("Arkanoid Paddle");
	StdPort_Mappings[STD_POWERPAD] = _T("Power Pad");
	StdPort_Mappings[STD_FOURSCORE] = _T("Four Score");
	StdPort_Mappings[STD_SNESCONTROLLER] = _T("SNES Controller");
}

void	ExpPort_SetUnconnected		(struct tExpPort *);
void	ExpPort_SetFami4Play		(struct tExpPort *);
void	ExpPort_SetArkanoidPaddle	(struct tExpPort *);
void	ExpPort_SetFamilyBasicKeyboard	(struct tExpPort *);
void	ExpPort_SetAltKeyboard		(struct tExpPort *);
void	ExpPort_SetFamTrainer		(struct tExpPort *);
void	ExpPort_SetTablet		(struct tExpPort *);

void	ExpPort_SetControllerType (struct tExpPort *Cont, int Type)
{
	Cont->Unload(Cont);
	switch (Cont->Type = Type)
	{
	case EXP_UNCONNECTED:		ExpPort_SetUnconnected(Cont);		break;
	case EXP_FAMI4PLAY:		ExpPort_SetFami4Play(Cont);		break;
	case EXP_ARKANOIDPADDLE:	ExpPort_SetArkanoidPaddle(Cont);	break;
	case EXP_FAMILYBASICKEYBOARD:	ExpPort_SetFamilyBasicKeyboard(Cont);	break;
	case EXP_ALTKEYBOARD:		ExpPort_SetAltKeyboard(Cont);		break;
	case EXP_FAMTRAINER:		ExpPort_SetFamTrainer(Cont);		break;
	case EXP_TABLET:		ExpPort_SetTablet(Cont);		break;
	default:MessageBox(mWnd,_T("Error: selected invalid controller type for expansion port!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);	break;
	}
}
TCHAR	*ExpPort_Mappings[EXP_MAX];
void	ExpPort_SetMappings (void)
{
	ExpPort_Mappings[EXP_UNCONNECTED] = _T("Unconnected");
	ExpPort_Mappings[EXP_FAMI4PLAY] = _T("Famicom 4-Player Adapter");
	ExpPort_Mappings[EXP_ARKANOIDPADDLE] = _T("Famicom Arkanoid Paddle");
	ExpPort_Mappings[EXP_FAMILYBASICKEYBOARD] = _T("Family Basic Keyboard");
	ExpPort_Mappings[EXP_ALTKEYBOARD] = _T("Alternate Keyboard");
	ExpPort_Mappings[EXP_FAMTRAINER] = _T("Family Trainer");
	ExpPort_Mappings[EXP_TABLET] = _T("Oeka Kids Tablet");
}

LRESULT	CALLBACK	ControllerProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	int i;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		StdPort_SetMappings();
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_RESETCONTENT,0,0);
		for (i = 0; i < STD_MAX; i++)
		{
			SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)StdPort_Mappings[i]);
			SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)StdPort_Mappings[i]);
		}
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,Controllers.Port1.Type,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,Controllers.Port2.Type,0);

		ExpPort_SetMappings();
		SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_RESETCONTENT,0,0);
		for (i = 0; i < EXP_MAX; i++)
			SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_ADDSTRING,0,(LPARAM)ExpPort_Mappings[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_SETCURSEL,Controllers.ExpPort.Type,0);
		if (Movie.Mode)
		{
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SPORT1),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SPORT2),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SEXPPORT),FALSE);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SPORT1),TRUE);
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SPORT2),TRUE);
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SEXPPORT),TRUE);
		}
		return TRUE;
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		switch (wmId)
		{
		case IDOK:
			EndDialog(hDlg,1);
			break;
		case IDC_CONT_SPORT1:
			if (wmEvent == CBN_SELCHANGE)
			{
				int Type = (int)SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_GETCURSEL,0,0);
				if (Type == STD_FOURSCORE)
				{
					StdPort_SetControllerType(&Controllers.FSPort1,Controllers.Port1.Type == STD_STDCONTROLLER ? STD_STDCONTROLLER : STD_UNCONNECTED);
					StdPort_SetControllerType(&Controllers.FSPort2,Controllers.Port2.Type == STD_STDCONTROLLER ? STD_STDCONTROLLER : STD_UNCONNECTED);
					memcpy(Controllers.FSPort1.Buttons,Controllers.Port1.Buttons,sizeof(Controllers.Port1.Buttons));
					memcpy(Controllers.FSPort2.Buttons,Controllers.Port2.Buttons,sizeof(Controllers.Port2.Buttons));
					StdPort_SetControllerType(&Controllers.Port1,STD_FOURSCORE);
					StdPort_SetControllerType(&Controllers.Port2,STD_FOURSCORE);
					SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,STD_FOURSCORE,0);
				}
				else if (Controllers.Port1.Type	== STD_FOURSCORE)
				{
					StdPort_SetControllerType(&Controllers.Port1,Type);
					StdPort_SetControllerType(&Controllers.Port2,Controllers.FSPort2.Type);
					memcpy(Controllers.Port2.Buttons,Controllers.FSPort2.Buttons,sizeof(Controllers.FSPort2.Buttons));
					SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,Controllers.Port2.Type,0);
				}
				else	StdPort_SetControllerType(&Controllers.Port1,Type);
			}
			break;
		case IDC_CONT_SPORT2:
			if (wmEvent == CBN_SELCHANGE)
			{
				int Type = (int)SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_GETCURSEL,0,0);
				if (Type == STD_FOURSCORE)
				{
					StdPort_SetControllerType(&Controllers.FSPort1,Controllers.Port1.Type == STD_STDCONTROLLER ? STD_STDCONTROLLER : STD_UNCONNECTED);
					StdPort_SetControllerType(&Controllers.FSPort2,Controllers.Port2.Type == STD_STDCONTROLLER ? STD_STDCONTROLLER : STD_UNCONNECTED);
					memcpy(Controllers.FSPort1.Buttons,Controllers.Port1.Buttons,sizeof(Controllers.Port1.Buttons));
					memcpy(Controllers.FSPort2.Buttons,Controllers.Port2.Buttons,sizeof(Controllers.Port2.Buttons));
					StdPort_SetControllerType(&Controllers.Port1,STD_FOURSCORE);
					StdPort_SetControllerType(&Controllers.Port2,STD_FOURSCORE);
					SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,STD_FOURSCORE,0);
				}
				else if (Controllers.Port2.Type	== STD_FOURSCORE)
				{
					StdPort_SetControllerType(&Controllers.Port1,Controllers.FSPort1.Type);
					memcpy(Controllers.Port1.Buttons,Controllers.FSPort1.Buttons,sizeof(Controllers.FSPort1.Buttons));
					StdPort_SetControllerType(&Controllers.Port2,Type);
					SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,Controllers.Port1.Type,0);
				}
				else	StdPort_SetControllerType(&Controllers.Port2,Type);
			}
			break;
		case IDC_CONT_SEXPPORT:
			if (wmEvent == CBN_SELCHANGE)
			{
				int Type = (int)SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_GETCURSEL,0,0);
				ExpPort_SetControllerType(&Controllers.ExpPort,Type);
			}
			break;
		case IDC_CONT_CPORT1:
			Controllers.Port1.Config(&Controllers.Port1,hDlg);
			break;
		case IDC_CONT_CPORT2:
			Controllers.Port2.Config(&Controllers.Port2,hDlg);
			break;
		case IDC_CONT_CEXPPORT:
			Controllers.ExpPort.Config(&Controllers.ExpPort,hDlg);
			break;
		};
		break;
	}

	return FALSE;
}
void	Controllers_OpenConfig (void)
{
	Controllers_UnAcquire();
	DialogBox(hInst,(LPCTSTR)IDD_CONTROLLERS,mWnd,ControllerProc);
	Controllers_SetDeviceUsed();
	Controllers_Acquire();
}

BOOL CALLBACK	EnumJoysticksCallback (const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
	int DevNum = Controllers.NumDevices;
	if (SUCCEEDED(IDirectInput7_CreateDeviceEx(Controllers.DirectInput,&pdidInstance->guidInstance,&IID_IDirectInputDevice7,(LPVOID *)&Controllers.DIJoystick[DevNum-2],NULL)))
	{
		int i, j;
		HRESULT hr;
		DIDEVCAPS tpc;
		DIJOYSTATE2 joystate;
		if (FAILED(hr = IDirectInputDevice7_SetDataFormat(Controllers.DIJoystick[DevNum-2],&c_dfDIJoystick2)))
			return hr;
		if (FAILED(hr = IDirectInputDevice7_SetCooperativeLevel(Controllers.DIJoystick[DevNum-2],mWnd,DISCL_BACKGROUND | DISCL_NONEXCLUSIVE)))
			return hr;
		_tcscpy(Controllers.DeviceName[DevNum],pdidInstance->tszProductName);
		tpc.dwSize = sizeof(DIDEVCAPS);
		if (FAILED(hr = IDirectInputDevice7_GetCapabilities(Controllers.DIJoystick[DevNum-2],&tpc)))
			return hr;
		if (FAILED(hr = IDirectInputDevice7_Acquire(Controllers.DIJoystick[DevNum-2])))
			return hr;
		if (FAILED(hr = IDirectInputDevice7_Poll(Controllers.DIJoystick[DevNum-2])))
			return hr;
		if (FAILED(hr = IDirectInputDevice7_GetDeviceState(Controllers.DIJoystick[DevNum-2],sizeof(DIJOYSTATE2),&joystate)))
			return hr;
		if (FAILED(hr = IDirectInputDevice7_Unacquire(Controllers.DIJoystick[DevNum-2])))
			return hr;
		if (FAILED(hr = IDirectInputDevice7_SetCooperativeLevel(Controllers.DIJoystick[DevNum-2],mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
			return hr;

		Controllers.NumButtons[DevNum] = tpc.dwButtons;
		Controllers.NumAxes[DevNum] = 0;
		if (joystate.lX)
			Controllers.NumAxes[DevNum] |= 0x01;
		if (joystate.lY)
			Controllers.NumAxes[DevNum] |= 0x02;
		if (joystate.lZ)
			Controllers.NumAxes[DevNum] |= 0x04;
		if (joystate.lRx)
			Controllers.NumAxes[DevNum] |= 0x08;
		if (joystate.lRy)
			Controllers.NumAxes[DevNum] |= 0x10;
		if (joystate.lRz)
			Controllers.NumAxes[DevNum] |= 0x20;
		if (joystate.rglSlider[0])
			Controllers.NumAxes[DevNum] |= 0x40;
		if (joystate.rglSlider[1])
			Controllers.NumAxes[DevNum] |= 0x80;
		j = tpc.dwAxes;
		for (i = 0; i < 8; i++)
			if ((Controllers.NumAxes[DevNum] >> i) & 0x01)
				j--;
		if (j)	// our previous method failed, so assume it's the first N axes
			Controllers.NumAxes[DevNum] = 0xFF >> (8 - tpc.dwAxes);
		Controllers.NumDevices++;
	}
	return DIENUM_CONTINUE;
}
void	Controllers_Init (void)
{
	DIDEVCAPS tpc;
	int i;
	
	for (i = 0; i < Controllers.NumDevices; i++)
		Controllers.DeviceUsed[i] = FALSE;
	for (i = 2; i < Controllers.NumDevices; i++)
		Controllers.DIJoystick[i-2] = NULL;

	ZeroMemory(Controllers.Port1.Buttons,sizeof(Controllers.Port1.Buttons));
	ZeroMemory(Controllers.Port2.Buttons,sizeof(Controllers.Port2.Buttons));
	ZeroMemory(Controllers.FSPort1.Buttons,sizeof(Controllers.FSPort1.Buttons));
	ZeroMemory(Controllers.FSPort2.Buttons,sizeof(Controllers.FSPort2.Buttons));
	ZeroMemory(Controllers.FSPort3.Buttons,sizeof(Controllers.FSPort3.Buttons));
	ZeroMemory(Controllers.FSPort4.Buttons,sizeof(Controllers.FSPort4.Buttons));
	ZeroMemory(Controllers.ExpPort.Buttons,sizeof(Controllers.ExpPort.Buttons));

	StdPort_SetUnconnected(&Controllers.Port1);
	StdPort_SetUnconnected(&Controllers.Port2);
	StdPort_SetUnconnected(&Controllers.FSPort1);
	StdPort_SetUnconnected(&Controllers.FSPort2);
	StdPort_SetUnconnected(&Controllers.FSPort3);
	StdPort_SetUnconnected(&Controllers.FSPort4);
	ExpPort_SetUnconnected(&Controllers.ExpPort);
	
	if (FAILED(DirectInputCreateEx(hInst,DIRECTINPUT_VERSION,&IID_IDirectInput7,(LPVOID *)&Controllers.DirectInput,NULL)))
	{
		MessageBox(mWnd,_T("Unable to initialize DirectInput!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}
	if (FAILED(IDirectInput7_CreateDeviceEx(Controllers.DirectInput,&GUID_SysKeyboard,&IID_IDirectInputDevice7,(LPVOID *)&Controllers.DIKeyboard,NULL)))
	{
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,_T("Unable to initialize keyboard input device!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}
	if (FAILED(IDirectInputDevice7_SetDataFormat(Controllers.DIKeyboard,&c_dfDIKeyboard)))
	{
		IDirectInputDevice7_Release(Controllers.DIKeyboard);
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,_T("Unable to set keyboard input data format!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIKeyboard,mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		IDirectInputDevice7_Release(Controllers.DIKeyboard);
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,_T("Unable to set keyboard input cooperative level!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}
	Controllers.NumButtons[0] = 256;
	Controllers.NumAxes[0] = 0;
	_tcscpy(Controllers.DeviceName[0],_T("Keyboard"));

	if (FAILED(IDirectInput7_CreateDeviceEx(Controllers.DirectInput,&GUID_SysMouse,&IID_IDirectInputDevice7,(LPVOID *)&Controllers.DIMouse,NULL)))
	{
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,_T("Unable to initialize mouse input device!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}
	if (FAILED(IDirectInputDevice7_SetDataFormat(Controllers.DIMouse,&c_dfDIMouse2)))
	{
		IDirectInputDevice7_Release(Controllers.DIMouse);
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,_T("Unable to set mouse input data format!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIMouse,mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		IDirectInputDevice7_Release(Controllers.DIMouse);
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,_T("Unable to set mouse input cooperative level!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}
	tpc.dwSize = sizeof(DIDEVCAPS);
	if (FAILED(IDirectInputDevice7_GetCapabilities(Controllers.DIMouse,&tpc)))
	{
		IDirectInputDevice7_Release(Controllers.DIMouse);
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,_T("Unable to get mouse input capabilities!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}
	Controllers.NumButtons[1] = tpc.dwButtons;
	Controllers.NumAxes[1] = tpc.dwAxes;
	
	_tcscpy(Controllers.DeviceName[1],_T("Mouse"));
	
	Controllers.NumDevices = 2;
	IDirectInput7_EnumDevices(Controllers.DirectInput,DIDEVTYPE_JOYSTICK,EnumJoysticksCallback,NULL,DIEDFL_ATTACHEDONLY);
	Movie.Mode = 0;
	Controllers_Acquire();
}

void	Controllers_Release (void)
{
	int i;
	Controllers_UnAcquire();
	Controllers.Port1.Unload(&Controllers.Port1);
	Controllers.Port2.Unload(&Controllers.Port2);
	Controllers.FSPort1.Unload(&Controllers.FSPort1);
	Controllers.FSPort2.Unload(&Controllers.FSPort2);
	Controllers.FSPort3.Unload(&Controllers.FSPort3);
	Controllers.FSPort4.Unload(&Controllers.FSPort4);
	Controllers.ExpPort.Unload(&Controllers.ExpPort);
	IDirectInputDevice7_Unacquire(Controllers.DIKeyboard);
	IDirectInputDevice7_Release(Controllers.DIKeyboard);
	IDirectInputDevice7_Unacquire(Controllers.DIMouse);
	IDirectInputDevice7_Release(Controllers.DIMouse);
	for (i = 2; i < Controllers.NumDevices; i++)
		if (Controllers.DIJoystick[i-2])
		{
			IDirectInputDevice7_Unacquire(Controllers.DIJoystick[i-2]);
			IDirectInputDevice7_Release(Controllers.DIJoystick[i-2]);
		}
	IDirectInput7_Release(Controllers.DirectInput);
}

void	Controllers_Write (unsigned char Val)
{
	Controllers.Port1.Write(&Controllers.Port1,Val & 0x1);
	Controllers.Port2.Write(&Controllers.Port2,Val & 0x1);
	Controllers.ExpPort.Write(&Controllers.ExpPort,Val);
}

void	Controllers_SetDeviceUsed (void)
{
	int i;

	for (i = 0; i < Controllers.NumDevices; i++)
		Controllers.DeviceUsed[i] = FALSE;

	for (i = 0; i < CONTROLLERS_MAXBUTTONS; i++)
	{
		if (Controllers.Port1.Type == 5)
		{
			if (i < Controllers.FSPort1.NumButtons)
				Controllers.DeviceUsed[Controllers.FSPort1.Buttons[i] >> 16] = TRUE;
			if (i < Controllers.FSPort2.NumButtons)
				Controllers.DeviceUsed[Controllers.FSPort2.Buttons[i] >> 16] = TRUE;
			if (i < Controllers.FSPort3.NumButtons)
				Controllers.DeviceUsed[Controllers.FSPort3.Buttons[i] >> 16] = TRUE;
			if (i < Controllers.FSPort4.NumButtons)
				Controllers.DeviceUsed[Controllers.FSPort4.Buttons[i] >> 16] = TRUE;
		}
		else
		{
			if (i < Controllers.Port1.NumButtons)
				Controllers.DeviceUsed[Controllers.Port1.Buttons[i] >> 16] = TRUE;
			if (i < Controllers.Port2.NumButtons)
				Controllers.DeviceUsed[Controllers.Port2.Buttons[i] >> 16] = TRUE;
		}
		if (i < Controllers.ExpPort.NumButtons)
			Controllers.DeviceUsed[Controllers.ExpPort.Buttons[i] >> 16] = TRUE;
	}
}

void	Controllers_Acquire (void)
{
	int i;
	IDirectInputDevice7_Acquire(Controllers.DIKeyboard);
	IDirectInputDevice7_Acquire(Controllers.DIMouse);
	for (i = 2; i < Controllers.NumDevices; i++)
		IDirectInputDevice7_Acquire(Controllers.DIJoystick[i-2]);
	if ((Controllers.ExpPort.Type == EXP_FAMILYBASICKEYBOARD) || (Controllers.ExpPort.Type == EXP_ALTKEYBOARD))
		MaskKeyboard = 1;
}
void	Controllers_UnAcquire (void)
{
	int i;
	IDirectInputDevice7_Unacquire(Controllers.DIKeyboard);
	IDirectInputDevice7_Unacquire(Controllers.DIMouse);
	for (i = 2; i < Controllers.NumDevices; i++)
		IDirectInputDevice7_Unacquire(Controllers.DIJoystick[i-2]);
	MaskKeyboard = 0;
}


void	Controllers_UpdateInput (void)
{
	HRESULT hr;
	int i;
	unsigned char Cmd = 0;
	if (Controllers.DeviceUsed[0])
	{
		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIKeyboard,256,Controllers.KeyState);
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			IDirectInputDevice7_Acquire(Controllers.DIKeyboard);
			ZeroMemory(Controllers.KeyState,sizeof(Controllers.KeyState));
		}
	}
	if (Controllers.DeviceUsed[1])
	{
		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIMouse,sizeof(DIMOUSESTATE2),&Controllers.MouseState);
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			IDirectInputDevice7_Acquire(Controllers.DIMouse);
			ZeroMemory(&Controllers.MouseState,sizeof(Controllers.MouseState));
		}
	}
	for (i = 2; i < Controllers.NumDevices; i++)
		if (Controllers.DeviceUsed[i])
		{
			hr = IDirectInputDevice7_Poll(Controllers.DIJoystick[i-2]);
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
			{
				IDirectInputDevice7_Acquire(Controllers.DIJoystick[i-2]);
				ZeroMemory(&Controllers.JoyState[i-2],sizeof(DIJOYSTATE2));
				continue;
			}
			hr = IDirectInputDevice7_GetDeviceState(Controllers.DIJoystick[i-2],sizeof(DIJOYSTATE2),&Controllers.JoyState[i-2]);
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
				ZeroMemory(&Controllers.JoyState[i-2],sizeof(DIJOYSTATE2));
		}

	if (Movie.Mode & MOV_PLAY)
		Cmd = Movie_LoadInput();
	else
	{
		if ((MI) && (MI->Config))
			Cmd = MI->Config(CFG_QUERY,0);
	}
	Controllers.Port1.Frame(&Controllers.Port1,Movie.Mode);
	Controllers.Port2.Frame(&Controllers.Port2,Movie.Mode);
	Controllers.ExpPort.Frame(&Controllers.ExpPort,Movie.Mode);
	if ((Cmd) && (MI) && (MI->Config))
		MI->Config(CFG_CMD,Cmd);
	if (Movie.Mode & MOV_RECORD)
		Movie_SaveInput(Cmd);
}

int	Controllers_GetConfigKey (HWND hWnd)
{
	HRESULT hr;
	int i;
	int Key = -1;

	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIKeyboard,hWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(mWnd,_T("Unable to modify keyboard input cooperative level!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return Key;
	}

	IDirectInputDevice7_Acquire(Controllers.DIKeyboard);
	while (Key == -1)
	{
		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIKeyboard,256,Controllers.KeyState);
		for (i = 0; i < Controllers.NumButtons[0]; i++)
		{
			if (Controllers_IsPressed(i))
			{
				Key = i;
				break;
			}
		}
	}
	{
waitrelease:
		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIKeyboard,256,Controllers.KeyState);
		for (i = 0; i < Controllers.NumButtons[0]; i++)
			if (Controllers_IsPressed(i))
				goto waitrelease;
	}
	IDirectInputDevice7_Unacquire(Controllers.DIKeyboard);
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIKeyboard,mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(mWnd,_T("Unable to restore keyboard input cooperative level!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return Key;
	}

	return Key;
}


int	Controllers_GetConfigMouse (HWND hWnd)
{
	HRESULT hr;
	int i;
	int Key = -1;

	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIMouse,hWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(mWnd,_T("Unable to modify mouse input cooperative level!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return Key;
	}

	IDirectInputDevice7_Acquire(Controllers.DIMouse);
	while (Key == -1)
	{
		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIMouse,sizeof(DIMOUSESTATE2),&Controllers.MouseState);
		for (i = 0; i < Controllers.NumButtons[1]; i++)
		{
			if (Controllers_IsPressed((1 << 16) | i))
			{
				Key = i;
				break;
			}
		}
		if (Key != -1)	/* if we got a button, don't check for an axis */
			break;
		for (i = 0x08; i < (0x08 | (Controllers.NumAxes[1] << 1)); i++)
		{
			if (Controllers_IsPressed((1 << 16) | i))
			{
				Key = i;
				break;
			}
		}
	}
	{
waitrelease:
		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIMouse,sizeof(DIMOUSESTATE2),&Controllers.MouseState);
		for (i = 0; i < Controllers.NumButtons[1]; i++)
			if (Controllers_IsPressed((1 << 16) | i))
				goto waitrelease;
		for (i = 0x08; i < (0x08 | (Controllers.NumAxes[1] << 1)); i++)
			if (Controllers_IsPressed((1 << 16) | i))
				goto waitrelease;
	}
	IDirectInputDevice7_Unacquire(Controllers.DIMouse);
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIMouse,mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(mWnd,_T("Unable to restore mouse input cooperative level!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return Key;
	}

	return Key;
}

int	Controllers_GetConfigJoy (HWND hWnd, int Dev)
{
	HRESULT hr;
	int i;
	int Key = -1;

	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIJoystick[Dev],hWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(mWnd,_T("Unable to modify joystick input cooperative level!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return Key;
	}

	IDirectInputDevice7_Acquire(Controllers.DIJoystick[Dev]);
	while (Key == -1)
	{
		hr = IDirectInputDevice7_Poll(Controllers.DIJoystick[Dev]);
		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIJoystick[Dev],sizeof(DIJOYSTATE2),&Controllers.JoyState[Dev]);
		for (i = 0; i < Controllers.NumButtons[Dev+2]; i++)
		{
			if (Controllers_IsPressed(((Dev+2) << 16) | i))
			{
				Key = i;
				break;
			}
		}
		if (Key != -1)	/* if we got a button, don't check for an axis */
			break;
		for (i = 0x80; i < 0x90; i++)
		{
			if (Controllers_IsPressed(((Dev+2) << 16) | i))
			{
				Key = i;
				break;
			}
		}
	}
	{
waitrelease:
		hr = IDirectInputDevice7_Poll(Controllers.DIJoystick[Dev]);
		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIJoystick[Dev],sizeof(DIJOYSTATE2),&Controllers.JoyState[Dev]);
		for (i = 0; i < Controllers.NumButtons[Dev+2]; i++)
			if (Controllers_IsPressed(((Dev+2) << 16) | i))
				goto waitrelease;
		for (i = 0x80; i < 0x90; i++)
			if (Controllers_IsPressed(((Dev+2) << 16) | i))
				goto waitrelease;
	}
	IDirectInputDevice7_Unacquire(Controllers.DIJoystick[Dev]);
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIJoystick[Dev],mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(mWnd,_T("Unable to restore joystick input cooperative level!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return Key;
	}

	return Key;
}

TCHAR *	Controllers_GetButtonLabel (int DevNum, int Button)
{
	static TCHAR str[256];
	TCHAR *axes[16] = {_T("X-Axis"),_T("Y-Axis"),_T("Z-Axis"),_T("rX-Axis"),_T("rY-Axis"),_T("rZ-Axis"),_T("Slider 1"),_T("Slider 2")};
	if (DevNum == 0)
		return KeyLookup[Button];
	else if (DevNum == 1)
	{
		if (Button & 0x08)
		{
			Button &= 0x07;
			if (Button & 1)
				_stprintf(str,_T("%s (+)"),axes[Button >> 1]);
			else	_stprintf(str,_T("%s (-)"),axes[Button >> 1]);
			return str;
		}
		else
		{
			_stprintf(str,_T("Button %i"),Button + 1);
			return str;
		}
	}
	else
	{
		if (Button & 0x80)
		{
			Button &= 0x7F;
			if (Button & 1)
				_stprintf(str,_T("%s (+)"),axes[Button >> 1]);
			else	_stprintf(str,_T("%s (-)"),axes[Button >> 1]);
			return str;
		}
		else
		{
			_stprintf(str,_T("Button %i"),Button + 1);
			return str;
		}
	}
}

void	Controllers_ConfigButton (int *Button, int Device, HWND hDlg, BOOL getKey)
{
	*Button &= 0xFFFF;
	if (getKey)	/* this way, we can just re-label the button */
	{
		key = CreateDialog(hInst,(LPCTSTR)IDD_KEYCONFIG,hDlg,NULL);
		ShowWindow(key,TRUE);	/* FIXME - center this window properly */
		ProcessMessages();	/* let the "Press a key..." dialog display itself */
		if (Device == 0)
			*Button = Controllers_GetConfigKey(key);
		else if (Device == 1)
			*Button = Controllers_GetConfigMouse(key);
		else	*Button = Controllers_GetConfigJoy(key,Device-2);
		ProcessMessages();	/* flush all keypresses - don't want them going back to the parent dialog */
		DestroyWindow(key);	/* close the little window */
		key = NULL;
	}
	SetWindowText(hDlg,(LPCTSTR)Controllers_GetButtonLabel(Device,*Button));
	*Button |= Device << 16;	/* add the device ID */
}

static	BOOL	LockCursorPos (int x, int y)
{
	POINT pos;
	pos.x = 0;
	pos.y = 0;
	ClientToScreen(mWnd,&pos);
	pos.x += x * SizeMult;
	pos.y += y * SizeMult;
	SetCursorPos(pos.x,pos.y);
	return TRUE;
}

BOOL	Controllers_IsPressed (int Button)
{
	int DevNum = (Button & 0xFFFF0000) >> 16;
	if (DevNum == 0)
		return (Controllers.KeyState[Button & 0xFF] & 0x80) ? TRUE : FALSE;
	else if (DevNum == 1)
	{
		if (Button & 0x8)	/* axis selected */
		{
			if (!key)
				LockCursorPos(128,120);	/* if we're detecting mouse movement, lock the cursor in place */
			switch (Button & 0x7)
			{
			case 0x0:	return (Controllers.MouseState.lX < -1) ? TRUE : FALSE;	break;
			case 0x1:	return (Controllers.MouseState.lX > +1) ? TRUE : FALSE;	break;
			case 0x2:	return (Controllers.MouseState.lY < -1) ? TRUE : FALSE;	break;
			case 0x3:	return (Controllers.MouseState.lY > +1) ? TRUE : FALSE;	break;
			case 0x4:	return (Controllers.MouseState.lZ < -1) ? TRUE : FALSE;	break;
			case 0x5:	return (Controllers.MouseState.lZ > +1) ? TRUE : FALSE;	break;
			default:	return FALSE;
			}
		}
		else	return (Controllers.MouseState.rgbButtons[Button & 0x7] & 0x80) ? TRUE : FALSE;
	}
	else
	{
		if (Button & 0x80)
		{
			switch (Button & 0xF)
			{
			case 0x0:	return ((Controllers.NumAxes[DevNum] & 0x01) && (Controllers.JoyState[DevNum-2].lX < 0x4000)) ? TRUE : FALSE;	break;
			case 0x1:	return ((Controllers.NumAxes[DevNum] & 0x01) && (Controllers.JoyState[DevNum-2].lX > 0xC000)) ? TRUE : FALSE;	break;
			case 0x2:	return ((Controllers.NumAxes[DevNum] & 0x02) && (Controllers.JoyState[DevNum-2].lY < 0x4000)) ? TRUE : FALSE;	break;
			case 0x3:	return ((Controllers.NumAxes[DevNum] & 0x02) && (Controllers.JoyState[DevNum-2].lY > 0xC000)) ? TRUE : FALSE;	break;
			case 0x4:	return ((Controllers.NumAxes[DevNum] & 0x04) && (Controllers.JoyState[DevNum-2].lZ < 0x4000)) ? TRUE : FALSE;	break;
			case 0x5:	return ((Controllers.NumAxes[DevNum] & 0x04) && (Controllers.JoyState[DevNum-2].lZ > 0xC000)) ? TRUE : FALSE;	break;
			case 0x6:	return ((Controllers.NumAxes[DevNum] & 0x08) && (Controllers.JoyState[DevNum-2].lRx < 0x4000)) ? TRUE : FALSE;	break;
			case 0x7:	return ((Controllers.NumAxes[DevNum] & 0x08) && (Controllers.JoyState[DevNum-2].lRx > 0xC000)) ? TRUE : FALSE;	break;
			case 0x8:	return ((Controllers.NumAxes[DevNum] & 0x10) && (Controllers.JoyState[DevNum-2].lRy < 0x4000)) ? TRUE : FALSE;	break;
			case 0x9:	return ((Controllers.NumAxes[DevNum] & 0x10) && (Controllers.JoyState[DevNum-2].lRy > 0xC000)) ? TRUE : FALSE;	break;
			case 0xA:	return ((Controllers.NumAxes[DevNum] & 0x20) && (Controllers.JoyState[DevNum-2].lRz < 0x4000)) ? TRUE : FALSE;	break;
			case 0xB:	return ((Controllers.NumAxes[DevNum] & 0x20) && (Controllers.JoyState[DevNum-2].lRz > 0xC000)) ? TRUE : FALSE;	break;
			case 0xC:	return ((Controllers.NumAxes[DevNum] & 0x40) && (Controllers.JoyState[DevNum-2].rglSlider[0] < 0x4000)) ? TRUE : FALSE;	break;
			case 0xD:	return ((Controllers.NumAxes[DevNum] & 0x40) && (Controllers.JoyState[DevNum-2].rglSlider[0] > 0xC000)) ? TRUE : FALSE;	break;
			case 0xE:	return ((Controllers.NumAxes[DevNum] & 0x80) && (Controllers.JoyState[DevNum-2].rglSlider[1] < 0x4000)) ? TRUE : FALSE;	break;
			case 0xF:	return ((Controllers.NumAxes[DevNum] & 0x80) && (Controllers.JoyState[DevNum-2].rglSlider[1] > 0xC000)) ? TRUE : FALSE;	break;
			default:	return FALSE;
			}
		}
		else	return (Controllers.JoyState[DevNum-2].rgbButtons[Button & 0x7F] & 0x80) ? TRUE : FALSE;
	}
}
void	Controllers_ParseConfigMessages (HWND hDlg, int numItems, int *dlgDevices, int *dlgButtons, int *Buttons, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);
	int i;
	if (key)
		return;
	if (uMsg == WM_INITDIALOG)
	{
		for (i = 0; i < numItems; i++)
		{
			int j;
			SendDlgItemMessage(hDlg,dlgDevices[i],CB_RESETCONTENT,0,0);		/* clear the listbox */
			for (j = 0; j < Controllers.NumDevices; j++)
				SendDlgItemMessage(hDlg,dlgDevices[i],CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[j]);	/* add each device */
			SendDlgItemMessage(hDlg,dlgDevices[i],CB_SETCURSEL,Buttons[i] >> 16,0);	/* select the one we want */
			Controllers_ConfigButton(&Buttons[i],Buttons[i] >> 16,GetDlgItem(hDlg,dlgButtons[i]),FALSE);	/* and label the corresponding button */
		}
	}
	if (uMsg != WM_COMMAND)
		return;
	if (wmId == IDOK)
	{
		EndDialog(hDlg,1);
		return;
	}
	for (i = 0; i < numItems; i++)
	{
		if (wmId == dlgDevices[i])
		{
			if (wmEvent != CBN_SELCHANGE)
				break;
			Buttons[i] = 0;
			Controllers_ConfigButton(&Buttons[i],(int)SendMessage((HWND)lParam,CB_GETCURSEL,0,0),GetDlgItem(hDlg,dlgButtons[i]),FALSE);
		}
		if (wmId == dlgButtons[i])
			Controllers_ConfigButton(&Buttons[i],Buttons[i] >> 16,GetDlgItem(hDlg,dlgButtons[i]),TRUE);
	}
}