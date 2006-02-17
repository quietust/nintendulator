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
#include "Controllers.h"
#include "GFX.h"

char *KeyLookup[256] =
{
	"", "Escape", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "- (Keyboard)", "=", "Backspace", "Tab",
	"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Return", "Left Control", "A", "S",
	"D", "F", "G", "H", "J", "K", "L", ";", "'", "???", "Left Shift", "\\", "Z", "X", "C", "V",
	"B", "N", "M", ",", ". (Keyboard)", "/ (Keyboard)", "Right Shift", "* (keypad)", "Left Alt", "Space", "Caps Lock", "F1", "F2", "F3", "F4", "F5",
	"F6", "F7", "F8", "F9", "F10", "Num Lock", "Scroll Lock", "Numpad 7", "Numpad 8", "Numpad 9", "- (Numpad)", "Numpad 4", "Numpad 5", "Numpad 6", "+ (Numpad)", "Numpad 1",
	"Numpad 2", "Numpad 3", "Numpad 0", ". (Numpad)", "???", "???", "???", "F11", "F12", "???", "???", "???", "???", "???", "???", "???",
	"???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", 
	"???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", 
	"???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", 
	"Previous Track", "???", "???", "???", "???", "???", "???", "???", "???", "Next Track", "???", "???", "Numpad Enter", "Right Control", "???", "???",
	"Mute", "Calculator", "Play/Pause", "???", "Media Stop", "???", "???", "???", "???", "???", "???", "???", "???", "???", "Volume Down", "???",
	"Volume Up", "???", "Web Home", "???", "???", "/ (Numpad)", "???", "SysRq", "Right Alt", "???", "???", "???", "???", "???", "???", "???",
	"???", "???", "???", "???", "???", "Pause", "???", "Home", "Up Arrow", "PgUp", "???", "Left Arrow", "???", "Right Arrow", "???", "End", 
	"Down Arrow", "PgDn", "Insert", "Delete", "???", "???", "???", "???", "???", "???", "???", "Left Win Key", "Right Win Key", "App Menu Key", "Power Button", "Sleep Button",
	"???", "???", "???", "Wake Key", "???", "Web Search", "Web Favorites", "Web Refresh", "Web Stop", "Web Forward", "Web Back", "My Computer", "Mail", "Media Select","???","???",
	"???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???"
};

struct	tControllers	Controllers;

void	StdPort_SetUnconnected		(struct tStdPort *);
void	StdPort_SetStdController	(struct tStdPort *);
void	StdPort_SetZapper		(struct tStdPort *);
void	StdPort_SetArkanoidPaddle	(struct tStdPort *);
void	StdPort_SetPowerPad		(struct tStdPort *);
void	StdPort_SetFourScore		(struct tStdPort *);

void	StdPort_SetControllerType (struct tStdPort *Cont, int Type)
{
	Cont->Unload(Cont);
	switch (Cont->Type = Type)
	{
	case 0:	StdPort_SetUnconnected(Cont);		break;
	case 1:	StdPort_SetStdController(Cont);		break;
	case 2:	StdPort_SetZapper(Cont);		break;
	case 3:	StdPort_SetArkanoidPaddle(Cont);	break;
	case 4:	StdPort_SetPowerPad(Cont);		break;
	case 5:	StdPort_SetFourScore(Cont);		break;
	default:MessageBox(mWnd,"Error: selected invalid controller type for standard port!","Nintendulator",MB_OK | MB_ICONERROR);	break;
	}
}

void	ExpPort_SetUnconnected		(struct tExpPort *);
void	ExpPort_SetFami4Play		(struct tExpPort *);
void	ExpPort_SetArkanoidPaddle	(struct tExpPort *);
void	ExpPort_SetFamilyBasicKeyboard	(struct tExpPort *);

void	ExpPort_SetControllerType (struct tExpPort *Cont, int Type)
{
	Cont->Unload(Cont);
	switch (Cont->Type = Type)
	{
	case 0:	ExpPort_SetUnconnected(Cont);		break;
	case 1:	ExpPort_SetFami4Play(Cont);		break;
	case 2:	ExpPort_SetArkanoidPaddle(Cont);	break;
	case 3:	ExpPort_SetFamilyBasicKeyboard(Cont);	break;
	default:MessageBox(mWnd,"Error: selected invalid controller type for expansion port!","Nintendulator",MB_OK | MB_ICONERROR);	break;
	}
}

BOOL CALLBACK	ControllerProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)"Unconnected");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)"Unconnected");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)"Standard Controller");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)"Standard Controller");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)"Zapper");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)"Zapper");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)"Arkanoid Paddle");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)"Arkanoid Paddle");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)"Power Pad");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)"Power Pad");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)"Four Score");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)"Four Score");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,Controllers.Port1.Type,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,Controllers.Port2.Type,0);

		SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_ADDSTRING,0,(LPARAM)"Unconnected");
		SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_ADDSTRING,0,(LPARAM)"Famicom 4-Player Adapter");
		SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_ADDSTRING,0,(LPARAM)"Famicom Arkanoid Paddle");
		SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_ADDSTRING,0,(LPARAM)"Family Basic Keyboard");
		SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_SETCURSEL,Controllers.ExpPort.Type,0);
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case IDOK:
			EndDialog(hDlg,1);
			break;
		case IDC_CONT_SPORT1:	if (wmEvent == CBN_SELCHANGE)
					{
						int Type = SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_GETCURSEL,0,0);
						if (Type == 5)
						{
							StdPort_SetControllerType(&Controllers.FSPort1,Controllers.Port1.Type);
							StdPort_SetControllerType(&Controllers.FSPort2,Controllers.Port2.Type);
							memcpy(Controllers.FSPort1.Buttons,Controllers.Port1.Buttons,sizeof(Controllers.Port1.Buttons));
							memcpy(Controllers.FSPort2.Buttons,Controllers.Port2.Buttons,sizeof(Controllers.Port2.Buttons));
							StdPort_SetControllerType(&Controllers.Port2,5);
							SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,5,0);
						}
						else if (Controllers.Port1.Type	== 5)
						{
							StdPort_SetControllerType(&Controllers.Port1,Type);
							StdPort_SetControllerType(&Controllers.Port2,Controllers.FSPort2.Type);
							memcpy(Controllers.Port2.Buttons,Controllers.FSPort2.Buttons,sizeof(Controllers.FSPort2.Buttons));
							SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,Controllers.Port1.Type,0);
							SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,Controllers.Port2.Type,0);
						}
						else	StdPort_SetControllerType(&Controllers.Port1,Type);
					}
					break;
		case IDC_CONT_SPORT2:	if (wmEvent == CBN_SELCHANGE)
					{
						int Type = SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_GETCURSEL,0,0);
						if (Type == 5)
						{
							StdPort_SetControllerType(&Controllers.FSPort1,Controllers.Port1.Type);
							StdPort_SetControllerType(&Controllers.FSPort2,Controllers.Port2.Type);
							memcpy(Controllers.FSPort1.Buttons,Controllers.Port1.Buttons,sizeof(Controllers.Port1.Buttons));
							memcpy(Controllers.FSPort2.Buttons,Controllers.Port2.Buttons,sizeof(Controllers.Port2.Buttons));
							StdPort_SetControllerType(&Controllers.Port1,5);
							SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,5,0);
						}
						else if (Controllers.Port2.Type	== 5)
						{
							StdPort_SetControllerType(&Controllers.Port1,Controllers.FSPort1.Type);
							memcpy(Controllers.Port1.Buttons,Controllers.FSPort1.Buttons,sizeof(Controllers.FSPort1.Buttons));
							StdPort_SetControllerType(&Controllers.Port2,Type);
							SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,Controllers.Port1.Type,0);
							SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,Controllers.Port2.Type,0);
						}
						else	StdPort_SetControllerType(&Controllers.Port2,Type);
					}
					break;
		case IDC_CONT_SEXPPORT:	if (wmEvent == CBN_SELCHANGE) { ExpPort_SetControllerType(&Controllers.ExpPort,SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_GETCURSEL,0,0)); }	break;

		case IDC_CONT_CPORT1:	Controllers.Port1.Config(&Controllers.Port1,hDlg);	break;
		case IDC_CONT_CPORT2:	Controllers.Port2.Config(&Controllers.Port2,hDlg);	break;
		case IDC_CONT_CEXPPORT:	Controllers.ExpPort.Config(&Controllers.ExpPort,hDlg);	break;
		};
		break;
	}

	return FALSE;
}
void	Controllers_OpenConfig (void)
{
	DialogBox(hInst,(LPCTSTR)IDD_CONTROLLERS,mWnd,ControllerProc);
	Controllers_SetDeviceUsed();
}

BOOL CALLBACK	EnumJoysticksCallback (const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
	if (SUCCEEDED(IDirectInput7_CreateDeviceEx(Controllers.DirectInput,&pdidInstance->guidInstance,&IID_IDirectInputDevice7,(LPVOID *)&Controllers.DIJoystick[Controllers.NumDevices-2],NULL)))
	{
		HRESULT hr;
		DIDEVCAPS tpc;
		if (FAILED(hr = IDirectInputDevice7_SetDataFormat(Controllers.DIJoystick[Controllers.NumDevices-2],&c_dfDIJoystick2)))
			return hr;
		if (FAILED(hr = IDirectInputDevice7_SetCooperativeLevel(Controllers.DIJoystick[Controllers.NumDevices-2],mWnd,DISCL_FOREGROUND | DISCL_EXCLUSIVE)))
			return hr;
		strcpy(Controllers.DeviceName[Controllers.NumDevices],pdidInstance->tszProductName);
		tpc.dwSize = sizeof(DIDEVCAPS);
		if (FAILED(hr = IDirectInputDevice7_GetCapabilities(Controllers.DIJoystick[Controllers.NumDevices-2],&tpc)))
			return hr;
		Controllers.NumButtons[Controllers.NumDevices] = tpc.dwButtons;
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
		MessageBox(mWnd,"Unable to initialize DirectInput!","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}
	if (FAILED(IDirectInput7_CreateDeviceEx(Controllers.DirectInput,&GUID_SysKeyboard,&IID_IDirectInputDevice7,(LPVOID *)&Controllers.DIKeyboard,NULL)))
	{
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,"Unable to initialize keyboard input device!","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}
	if (FAILED(IDirectInputDevice7_SetDataFormat(Controllers.DIKeyboard,&c_dfDIKeyboard)))
	{
		IDirectInputDevice7_Release(Controllers.DIKeyboard);
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,"Unable to set keyboard input data format!","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIKeyboard,mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		IDirectInputDevice7_Release(Controllers.DIKeyboard);
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,"Unable to set keyboard input cooperative level!","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}
	Controllers.NumButtons[0] = 256;
	strcpy(Controllers.DeviceName[0],"Keyboard");

	if (FAILED(IDirectInput7_CreateDeviceEx(Controllers.DirectInput,&GUID_SysMouse,&IID_IDirectInputDevice7,(LPVOID *)&Controllers.DIMouse,NULL)))
	{
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,"Unable to initialize mouse input device!","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}
	if (FAILED(IDirectInputDevice7_SetDataFormat(Controllers.DIMouse,&c_dfDIMouse2)))
	{
		IDirectInputDevice7_Release(Controllers.DIMouse);
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,"Unable to set mouse input data format!","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIMouse,mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		IDirectInputDevice7_Release(Controllers.DIMouse);
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,"Unable to set mouse input cooperative level!","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}
	tpc.dwSize = sizeof(DIDEVCAPS);
	if (FAILED(IDirectInputDevice7_GetCapabilities(Controllers.DIMouse,&tpc)))
	{
		IDirectInputDevice7_Release(Controllers.DIMouse);
		IDirectInput7_Release(Controllers.DirectInput);
		MessageBox(mWnd,"Unable to get mouse input capabilities!","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}
	Controllers.NumButtons[1] = tpc.dwButtons;
	
	strcpy(Controllers.DeviceName[1],"Mouse");
	
	Controllers.NumDevices = 2;
	IDirectInput7_EnumDevices(Controllers.DirectInput,DIDEVTYPE_JOYSTICK,EnumJoysticksCallback,NULL,DIEDFL_ATTACHEDONLY);
}

void	Controllers_Release (void)
{
	int i;
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
}
void	Controllers_UnAcquire (void)
{
	int i;
	IDirectInputDevice7_Unacquire(Controllers.DIKeyboard);
	IDirectInputDevice7_Unacquire(Controllers.DIMouse);
	for (i = 2; i < Controllers.NumDevices; i++)
		IDirectInputDevice7_Unacquire(Controllers.DIJoystick[i-2]);
}

void	Controllers_UpdateInput (void)
{
	HRESULT hr;
	int i;

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
}

int	Controllers_GetConfigKey (HWND hWnd)
{
	HRESULT hr;
	int i;
	int Key = -1;

	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIKeyboard,hWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(mWnd,"Unable to modify keyboard input cooperative level!","Nintendulator",MB_OK | MB_ICONERROR);
		return Key;
	}

	IDirectInputDevice7_Acquire(Controllers.DIKeyboard);
	while (Key == -1)
	{
		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIKeyboard,256,Controllers.KeyState);
		while ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			IDirectInputDevice7_Acquire(Controllers.DIKeyboard);
			hr = IDirectInputDevice7_GetDeviceState(Controllers.DIKeyboard,256,Controllers.KeyState);
			break;
		}
		for (i = 0; i < Controllers.NumButtons[0]; i++)
		{
			if (Controllers.KeyState[i] & 0x80)
			{
				Key = i;
				break;
			}
		}
	}
	IDirectInputDevice7_Unacquire(Controllers.DIKeyboard);
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIKeyboard,mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(mWnd,"Unable to restore keyboard input cooperative level!","Nintendulator",MB_OK | MB_ICONERROR);
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
		MessageBox(mWnd,"Unable to modify mouse input cooperative level!","Nintendulator",MB_OK | MB_ICONERROR);
		return Key;
	}

	IDirectInputDevice7_Acquire(Controllers.DIMouse);
	while (Key == -1)
	{

		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIMouse,sizeof(DIMOUSESTATE2),&Controllers.MouseState);
		while ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			IDirectInputDevice7_Acquire(Controllers.DIMouse);
			hr = IDirectInputDevice7_GetDeviceState(Controllers.DIMouse,sizeof(DIMOUSESTATE2),&Controllers.MouseState);
			break;
		}
		for (i = 0; i < Controllers.NumButtons[1]; i++)
		{
			if (Controllers.MouseState.rgbButtons[i] & 0x80)
			{
				Key = i;
				break;
			}
		}
	}
	IDirectInputDevice7_Unacquire(Controllers.DIMouse);
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIMouse,mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(mWnd,"Unable to restore mouse input cooperative level!","Nintendulator",MB_OK | MB_ICONERROR);
		return Key;
	}

	return Key;
}

int	Controllers_GetConfigJoy (HWND hWnd, int Dev)
{
	HRESULT hr;
	int i;
	int Key = -1;

	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIJoystick[Dev],hWnd,DISCL_FOREGROUND | DISCL_EXCLUSIVE)))
	{
		MessageBox(mWnd,"Unable to modify joystick input cooperative level!","Nintendulator",MB_OK | MB_ICONERROR);
		return Key;
	}

	IDirectInputDevice7_Acquire(Controllers.DIJoystick[Dev]);
	while (Key == -1)
	{

		hr = IDirectInputDevice7_Poll(Controllers.DIJoystick[Dev]);
		while ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			IDirectInputDevice7_Acquire(Controllers.DIJoystick[Dev]);
			hr = IDirectInputDevice7_Poll(Controllers.DIJoystick[Dev]);
		}
		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIJoystick[Dev],sizeof(DIJOYSTATE2),&Controllers.JoyState[Dev]);
		for (i = 0; i < Controllers.NumButtons[Dev+2]; i++)
		{
			if (Controllers.JoyState[Dev].rgbButtons[i] & 0x80)
			{
				Key = i;
				break;
			}
		}
	}
	IDirectInputDevice7_Unacquire(Controllers.DIJoystick[Dev]);
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIJoystick[Dev],mWnd,DISCL_FOREGROUND | DISCL_EXCLUSIVE)))
	{
		MessageBox(mWnd,"Unable to restore joystick input cooperative level!","Nintendulator",MB_OK | MB_ICONERROR);
		return Key;
	}

	return Key;
}

void	Controllers_ConfigButton (int *Button, long Device, HWND hDlg, int dlgItem)
{
	char tmp[16] = "Undefined!";
	SendDlgItemMessage(hDlg,dlgItem,WM_SETTEXT,0,(LPARAM)(LPCTSTR)"...");
//	Controllers_ConfigButton(&Cont->Buttons[0],SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_GETCURSEL,0,0),GetDlgItem(hDlg,IDC_CONT_K0));
	if (Device == 0)
	{
		*Button = Controllers_GetConfigKey(hDlg);
		sprintf(tmp,"%i",*Button);
	}
	else if (Device == 1)
	{
		*Button = Controllers_GetConfigMouse(hDlg);
		sprintf(tmp,"Button %i",*Button);
	}
	else
	{
		*Button = Controllers_GetConfigJoy(hDlg,Device-2);
		sprintf(tmp,"Button %i",*Button);
	}
	SendDlgItemMessage(hDlg,dlgItem,WM_SETTEXT,0,(LPARAM)(LPCTSTR)tmp);
}


