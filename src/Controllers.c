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
#include "Movie.h"
#include "Controllers.h"
#include <commdlg.h>

static	HWND key;

struct	tControllers	Controllers;

void	StdPort_SetUnconnected		(struct tStdPort *);
void	StdPort_SetStdController	(struct tStdPort *);
void	StdPort_SetZapper		(struct tStdPort *);
void	StdPort_SetArkanoidPaddle	(struct tStdPort *);
void	StdPort_SetPowerPad		(struct tStdPort *);
void	StdPort_SetFourScore		(struct tStdPort *);
void	StdPort_SetSnesController	(struct tStdPort *);
void	StdPort_SetVSZapper		(struct tStdPort *);

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
	case STD_VSZAPPER:		StdPort_SetVSZapper(Cont);		break;
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
	StdPort_Mappings[STD_VSZAPPER] = _T("VS Unisystem Zapper");
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

static	BOOL	POVAxis = FALSE;

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

		if (Controllers.EnableOpposites)
			CheckDlgButton(hDlg,IDC_CONT_UDLR,BST_CHECKED);
		else	CheckDlgButton(hDlg,IDC_CONT_UDLR,BST_UNCHECKED);

		return TRUE;
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		switch (wmId)
		{
		case IDC_CONT_POV:
			POVAxis = (IsDlgButtonChecked(hDlg,IDC_CONT_POV) == BST_CHECKED);
			break;
		case IDOK:
			Controllers.EnableOpposites = (IsDlgButtonChecked(hDlg,IDC_CONT_UDLR) == BST_CHECKED);
			POVAxis = FALSE;
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

static	BOOL CALLBACK	EnumKeyboardObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	int ItemNum = 0;
	if (IsEqualGUID(&lpddoi->guidType,&GUID_Key))
	{
		ItemNum = lpddoi->dwOfs;
		if ((ItemNum >= 0) && (ItemNum < 256))
			Controllers.KeyNames[ItemNum] = _tcsdup(lpddoi->tszName);
		else	MessageBox(mWnd, _T("Error - encountered invalid keyboard key ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}
	return DIENUM_CONTINUE;
}

static	BOOL CALLBACK	EnumMouseObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	int ItemNum = 0;
	if (IsEqualGUID(&lpddoi->guidType,&GUID_XAxis))
	{
		Controllers.AxisFlags[1] |= 0x01;
		Controllers.AxisNames[1][AXIS_X] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(&lpddoi->guidType,&GUID_YAxis))
	{
		Controllers.AxisFlags[1] |= 0x02;
		Controllers.AxisNames[1][AXIS_Y] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(&lpddoi->guidType,&GUID_ZAxis))
	{
		Controllers.AxisFlags[1] |= 0x04;
		Controllers.AxisNames[1][AXIS_Z] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(&lpddoi->guidType,&GUID_Button))
	{
		ItemNum = (lpddoi->dwOfs - ((DWORD)&Controllers.MouseState.rgbButtons - (DWORD)&Controllers.MouseState)) / sizeof(Controllers.MouseState.rgbButtons[0]);
		if ((ItemNum >= 0) && (ItemNum < 8))
			Controllers.ButtonNames[1][ItemNum] = _tcsdup(lpddoi->tszName);
		else	MessageBox(mWnd, _T("Error - encountered invalid mouse button ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}

	return DIENUM_CONTINUE;
}

static	BOOL CALLBACK	EnumJoystickObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	int DevNum = Controllers.NumDevices;
	int ItemNum = 0;
	if (IsEqualGUID(&lpddoi->guidType,&GUID_XAxis))
	{
		Controllers.AxisFlags[DevNum] |= 0x01;
		Controllers.AxisNames[DevNum][AXIS_X] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(&lpddoi->guidType,&GUID_YAxis))
	{
		Controllers.AxisFlags[DevNum] |= 0x02;
		Controllers.AxisNames[DevNum][AXIS_Y] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(&lpddoi->guidType,&GUID_ZAxis))
	{
		Controllers.AxisFlags[DevNum] |= 0x04;
		Controllers.AxisNames[DevNum][AXIS_Z] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(&lpddoi->guidType,&GUID_RxAxis))
	{
		Controllers.AxisFlags[DevNum] |= 0x08;
		Controllers.AxisNames[DevNum][AXIS_RX] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(&lpddoi->guidType,&GUID_RyAxis))
	{
		Controllers.AxisFlags[DevNum] |= 0x10;
		Controllers.AxisNames[DevNum][AXIS_RY] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(&lpddoi->guidType,&GUID_RzAxis))
	{
		Controllers.AxisFlags[DevNum] |= 0x20;
		Controllers.AxisNames[DevNum][AXIS_RZ] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(&lpddoi->guidType,&GUID_Slider))
	{
		ItemNum = (lpddoi->dwOfs - ((DWORD)&Controllers.JoyState[0].rglSlider - (DWORD)&Controllers.JoyState[0])) / sizeof(Controllers.JoyState[0].rglSlider[0]);
		if (ItemNum == 0)
		{
			Controllers.AxisFlags[DevNum] |= 0x40;
			Controllers.AxisNames[DevNum][AXIS_S0] = _tcsdup(lpddoi->tszName);
		}
		else if (ItemNum == 1)
		{
			Controllers.AxisFlags[DevNum] |= 0x80;
			Controllers.AxisNames[DevNum][AXIS_S1] = _tcsdup(lpddoi->tszName);
		}
		else	MessageBox(mWnd, _T("Error - encountered invalid slider ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}
	if (IsEqualGUID(&lpddoi->guidType,&GUID_POV))
	{
		ItemNum = (lpddoi->dwOfs - ((DWORD)&Controllers.JoyState[0].rgdwPOV - (DWORD)&Controllers.JoyState[0])) / sizeof(Controllers.JoyState[0].rgdwPOV[0]);
		if ((ItemNum >= 0) && (ItemNum < 4))
		{
			Controllers.POVFlags[DevNum] |= 0x01 << ItemNum;
			Controllers.POVNames[DevNum][ItemNum] = _tcsdup(lpddoi->tszName);
		}
		else	MessageBox(mWnd, _T("Error - encountered invalid POV hat ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}
	if (IsEqualGUID(&lpddoi->guidType,&GUID_Button))
	{
		ItemNum = (lpddoi->dwOfs - ((DWORD)&Controllers.JoyState[0].rgbButtons - (DWORD)&Controllers.JoyState[0])) / sizeof(Controllers.JoyState[0].rgbButtons[0]);
		if ((ItemNum >= 0) && (ItemNum < 128))
			Controllers.ButtonNames[DevNum][ItemNum] = _tcsdup(lpddoi->tszName);
		else	MessageBox(mWnd, _T("Error - encountered invalid joystick button ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}

	return DIENUM_CONTINUE;
}

static	BOOL CALLBACK	EnumJoysticksCallback (LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	HRESULT hr;
	int DevNum = Controllers.NumDevices;
	if (SUCCEEDED(IDirectInput7_CreateDeviceEx(Controllers.DirectInput,&lpddi->guidInstance,&IID_IDirectInputDevice7,(LPVOID *)&Controllers.DIDevices[DevNum],NULL)))
	{
		DIDEVCAPS caps;
		if (FAILED(hr = IDirectInputDevice7_SetDataFormat(Controllers.DIDevices[DevNum],&c_dfDIJoystick2)))
			goto end;
		if (FAILED(hr = IDirectInputDevice7_SetCooperativeLevel(Controllers.DIDevices[DevNum],mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
			goto end;
		caps.dwSize = sizeof(DIDEVCAPS);
		if (FAILED(hr = IDirectInputDevice7_GetCapabilities(Controllers.DIDevices[DevNum],&caps)))
			goto end;

		Controllers.NumButtons[DevNum] = caps.dwButtons;
		Controllers.AxisFlags[DevNum] = 0;
		Controllers.POVFlags[DevNum] = 0;
		Controllers.DeviceName[DevNum] = _tcsdup(lpddi->tszProductName);

		IDirectInputDevice7_EnumObjects(Controllers.DIDevices[DevNum],EnumJoystickObjectsCallback,NULL,DIDFT_ALL);
		EI.DbgOut(_T("Added input device '%s' with %i buttons, %i axes, %i POVs"), lpddi->tszProductName, caps.dwButtons, caps.dwAxes, caps.dwPOVs);
		Controllers.NumDevices++;
	}
	return DIENUM_CONTINUE;

end:
	IDirectInputDevice7_Release(Controllers.DIDevices[DevNum]);
	Controllers.DIDevices[DevNum] = NULL;
	return hr;
}

static	BOOL	InitKeyboard (void)
{
	DIDEVICEINSTANCE inst;
	DIDEVCAPS caps;
	if (FAILED(IDirectInput7_CreateDeviceEx(Controllers.DirectInput,&GUID_SysKeyboard,&IID_IDirectInputDevice7,(LPVOID *)&Controllers.DIDevices[0],NULL)))
		return FALSE;
	if (FAILED(IDirectInputDevice7_SetDataFormat(Controllers.DIDevices[0],&c_dfDIKeyboard)))
		goto end;
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIDevices[0],mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
		goto end;

	caps.dwSize = sizeof(DIDEVCAPS);
	if (FAILED(IDirectInputDevice7_GetCapabilities(Controllers.DIDevices[0],&caps)))
		goto end;

	inst.dwSize = sizeof(DIDEVICEINSTANCE);
	if (FAILED(IDirectInputDevice7_GetDeviceInfo(Controllers.DIDevices[0],&inst)))
		goto end;

	Controllers.NumButtons[0] = 256;// normally, I would use caps.dwButtons
	Controllers.AxisFlags[0] = 0;	// no axes
	Controllers.POVFlags[0] = 0;	// no POV hats
	Controllers.DeviceName[0] = _tcsdup(inst.tszProductName);

	IDirectInputDevice7_EnumObjects(Controllers.DIDevices[0],EnumKeyboardObjectsCallback,NULL,DIDFT_ALL);
	EI.DbgOut(_T("Added input device '%s' with %i buttons, %i axes, %i POVs"), inst.tszProductName, caps.dwButtons, caps.dwAxes, caps.dwPOVs);
	return TRUE;

end:
	IDirectInputDevice7_Release(Controllers.DIDevices[0]);
	Controllers.DIDevices[0] = NULL;
	return FALSE;
}

static	BOOL	InitMouse (void)
{
	DIDEVICEINSTANCE inst;
	DIDEVCAPS caps;
	if (FAILED(IDirectInput7_CreateDeviceEx(Controllers.DirectInput,&GUID_SysMouse,&IID_IDirectInputDevice7,(LPVOID *)&Controllers.DIDevices[1],NULL)))
		return FALSE;
	if (FAILED(IDirectInputDevice7_SetDataFormat(Controllers.DIDevices[1],&c_dfDIMouse2)))
		goto end;
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(Controllers.DIDevices[1],mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
		goto end;
	caps.dwSize = sizeof(DIDEVCAPS);
	if (FAILED(IDirectInputDevice7_GetCapabilities(Controllers.DIDevices[1],&caps)))
		goto end;

	inst.dwSize = sizeof(DIDEVICEINSTANCE);
	if (FAILED(IDirectInputDevice7_GetDeviceInfo(Controllers.DIDevices[1],&inst)))
		goto end;

	Controllers.NumButtons[1] = caps.dwButtons;
	Controllers.AxisFlags[1] = 0;
	Controllers.POVFlags[1] = 0;
	Controllers.DeviceName[1] = _tcsdup(inst.tszProductName);

	IDirectInputDevice7_EnumObjects(Controllers.DIDevices[1],EnumMouseObjectsCallback,NULL,DIDFT_ALL);
	EI.DbgOut(_T("Added input device '%s' with %i buttons, %i axes, %i POVs"), inst.tszProductName, caps.dwButtons, caps.dwAxes, caps.dwPOVs);
	return TRUE;

end:
	IDirectInputDevice7_Release(Controllers.DIDevices[1]);
	Controllers.DIDevices[1] = NULL;
	return FALSE;
}

void	Controllers_Init (void)
{
	int i;
	
	for (i = 0; i < Controllers.NumDevices; i++)
	{
		Controllers.DeviceUsed[i] = FALSE;
		Controllers.DIDevices[i] = NULL;
		Controllers.DeviceName[i] = NULL;
		memset(Controllers.AxisNames[i],0,sizeof(Controllers.AxisNames[i]));
		memset(Controllers.ButtonNames[i],0,sizeof(Controllers.ButtonNames[i]));
		memset(Controllers.POVNames[i],0,sizeof(Controllers.POVNames[i]));
	}
	memset(Controllers.KeyNames,0,sizeof(Controllers.KeyNames));

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

	if (!InitKeyboard())
		MessageBox(mWnd,_T("Failed to initialize keyboard!"),_T("Nintendulator"),MB_OK | MB_ICONWARNING);
	if (!InitMouse())
		MessageBox(mWnd,_T("Failed to initialize mouse!"),_T("Nintendulator"),MB_OK | MB_ICONWARNING);

	Controllers.NumDevices = 2;	// joysticks start at slot 2
	IDirectInput7_EnumDevices(Controllers.DirectInput,DIDEVTYPE_JOYSTICK,EnumJoysticksCallback,NULL,DIEDFL_ATTACHEDONLY);

	Movie.Mode = 0;
	Controllers_Acquire();
}

void	Controllers_Release (void)
{
	int i, j;
	Controllers_UnAcquire();
	Controllers.Port1.Unload(&Controllers.Port1);
	Controllers.Port2.Unload(&Controllers.Port2);
	Controllers.FSPort1.Unload(&Controllers.FSPort1);
	Controllers.FSPort2.Unload(&Controllers.FSPort2);
	Controllers.FSPort3.Unload(&Controllers.FSPort3);
	Controllers.FSPort4.Unload(&Controllers.FSPort4);
	Controllers.ExpPort.Unload(&Controllers.ExpPort);
	for (i = 0; i < Controllers.NumDevices; i++)
	{
		if (Controllers.DIDevices[i])
		{
			IDirectInputDevice7_Unacquire(Controllers.DIDevices[i]);
			IDirectInputDevice7_Release(Controllers.DIDevices[i]);
		}
		free(Controllers.DeviceName[i]);
		Controllers.DeviceName[i] = NULL;
		for (j = 0; j < 128; j++)
		{
			free(Controllers.ButtonNames[i][j]);
			Controllers.ButtonNames[i][j] = NULL;
		}
		for (j = 0; j < 8; j++)
		{
			free(Controllers.AxisNames[i][j]);
			Controllers.AxisNames[i][j] = NULL;
		}
		for (j = 0; j < 4; j++)
		{
			free(Controllers.POVNames[i][j]);
			Controllers.POVNames[i][j] = NULL;
		}
	}
	for (j = 0; j < 256; j++)
	{
		free(Controllers.KeyNames[j]);
		Controllers.KeyNames[j] = NULL;
	}

	IDirectInput7_Release(Controllers.DirectInput);
}

void	Controllers_Write (unsigned char Val)
{
	Controllers.Port1.Write(&Controllers.Port1,Val & 0x1);
	Controllers.Port2.Write(&Controllers.Port2,Val & 0x1);
	Controllers.ExpPort.Write(&Controllers.ExpPort,Val);
}

int	Controllers_Save (FILE *out)
{
	int clen = 0;
	fwrite(&Controllers.FSPort1.Type,4,1,out);	clen += 4;
	fwrite(&Controllers.FSPort2.Type,4,1,out);	clen += 4;
	fwrite(&Controllers.FSPort3.Type,4,1,out);	clen += 4;
	fwrite(&Controllers.FSPort4.Type,4,1,out);	clen += 4;
	fwrite(&Controllers.Port1.Type,4,1,out);	clen += 4;
	fwrite(&Controllers.Port2.Type,4,1,out);	clen += 4;
	fwrite(&Controllers.ExpPort.Type,4,1,out);	clen += 4;
	fwrite(Controllers.Port1.Data,Controllers.Port1.DataLen,1,out);		clen += Controllers.Port1.DataLen;
	fwrite(Controllers.Port2.Data,Controllers.Port2.DataLen,1,out);		clen += Controllers.Port2.DataLen;
	fwrite(Controllers.FSPort1.Data,Controllers.FSPort1.DataLen,1,out);	clen += Controllers.FSPort1.DataLen;
	fwrite(Controllers.FSPort2.Data,Controllers.FSPort2.DataLen,1,out);	clen += Controllers.FSPort2.DataLen;
	fwrite(Controllers.FSPort3.Data,Controllers.FSPort3.DataLen,1,out);	clen += Controllers.FSPort3.DataLen;
	fwrite(Controllers.FSPort4.Data,Controllers.FSPort4.DataLen,1,out);	clen += Controllers.FSPort4.DataLen;
	fwrite(Controllers.ExpPort.Data,Controllers.ExpPort.DataLen,1,out);	clen += Controllers.ExpPort.DataLen;
	return clen;
}

int	Controllers_Load (FILE *in)
{
	int clen = 0;
	int tpi;
	Movie.ControllerTypes[3] = 1;	// denotes that controller state has been loaded
					// if we're playing a movie, this means we should
					// SKIP the controller info in the movie block
	fread(&tpi,4,1,in);	StdPort_SetControllerType(&Controllers.FSPort1,tpi);	clen += 4;
	fread(&tpi,4,1,in);	StdPort_SetControllerType(&Controllers.FSPort2,tpi);	clen += 4;
	fread(&tpi,4,1,in);	StdPort_SetControllerType(&Controllers.FSPort3,tpi);	clen += 4;
	fread(&tpi,4,1,in);	StdPort_SetControllerType(&Controllers.FSPort4,tpi);	clen += 4;
	fread(&tpi,4,1,in);	StdPort_SetControllerType(&Controllers.Port1,tpi);	clen += 4;
	fread(&tpi,4,1,in);	StdPort_SetControllerType(&Controllers.Port2,tpi);	clen += 4;
	fread(&tpi,4,1,in);	ExpPort_SetControllerType(&Controllers.ExpPort,tpi);	clen += 4;

	fread(Controllers.Port1.Data,Controllers.Port1.DataLen,1,in);		clen += Controllers.Port1.DataLen;
	fread(Controllers.Port2.Data,Controllers.Port2.DataLen,1,in);		clen += Controllers.Port2.DataLen;
	fread(Controllers.FSPort1.Data,Controllers.FSPort1.DataLen,1,in);	clen += Controllers.FSPort1.DataLen;
	fread(Controllers.FSPort2.Data,Controllers.FSPort2.DataLen,1,in);	clen += Controllers.FSPort2.DataLen;
	fread(Controllers.FSPort3.Data,Controllers.FSPort3.DataLen,1,in);	clen += Controllers.FSPort3.DataLen;
	fread(Controllers.FSPort4.Data,Controllers.FSPort4.DataLen,1,in);	clen += Controllers.FSPort4.DataLen;
	fread(Controllers.ExpPort.Data,Controllers.ExpPort.DataLen,1,in);	clen += Controllers.ExpPort.DataLen;
	return clen;
}

void	Controllers_SetDeviceUsed (void)
{
	int i;

	for (i = 0; i < Controllers.NumDevices; i++)
		Controllers.DeviceUsed[i] = FALSE;

	for (i = 0; i < CONTROLLERS_MAXBUTTONS; i++)
	{
		if (Controllers.Port1.Type == STD_FOURSCORE)
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
	if ((Controllers.Port1.Type == STD_ARKANOIDPADDLE) || (Controllers.Port2.Type == STD_ARKANOIDPADDLE) || (Controllers.ExpPort.Type == EXP_ARKANOIDPADDLE))
		Controllers.DeviceUsed[1] = TRUE;
}

void	Controllers_Acquire (void)
{
	int i;
	for (i = 0; i < Controllers.NumDevices; i++)
		if (Controllers.DIDevices[i])
			IDirectInputDevice7_Acquire(Controllers.DIDevices[i]);
	if ((Controllers.ExpPort.Type == EXP_FAMILYBASICKEYBOARD) || (Controllers.ExpPort.Type == EXP_ALTKEYBOARD))
		MaskKeyboard = 1;
}
void	Controllers_UnAcquire (void)
{
	int i;
	for (i = 0; i < Controllers.NumDevices; i++)
		if (Controllers.DIDevices[i])
			IDirectInputDevice7_Unacquire(Controllers.DIDevices[i]);
	MaskKeyboard = 0;
}

void	Controllers_UpdateInput (void)
{
	HRESULT hr;
	int i;
	unsigned char Cmd = 0;
	if (Controllers.DeviceUsed[0])
	{
		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIDevices[0],256,Controllers.KeyState);
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			IDirectInputDevice7_Acquire(Controllers.DIDevices[0]);
			ZeroMemory(Controllers.KeyState,sizeof(Controllers.KeyState));
		}
	}
	if (Controllers.DeviceUsed[1])
	{
		hr = IDirectInputDevice7_GetDeviceState(Controllers.DIDevices[1],sizeof(DIMOUSESTATE2),&Controllers.MouseState);
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			IDirectInputDevice7_Acquire(Controllers.DIDevices[1]);
			ZeroMemory(&Controllers.MouseState,sizeof(Controllers.MouseState));
		}
	}
	for (i = 2; i < Controllers.NumDevices; i++)
		if (Controllers.DeviceUsed[i])
		{
			hr = IDirectInputDevice7_Poll(Controllers.DIDevices[i]);
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
			{
				IDirectInputDevice7_Acquire(Controllers.DIDevices[i]);
				ZeroMemory(&Controllers.JoyState[i],sizeof(DIJOYSTATE2));
				continue;
			}
			hr = IDirectInputDevice7_GetDeviceState(Controllers.DIDevices[i],sizeof(DIJOYSTATE2),&Controllers.JoyState[i]);
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
				ZeroMemory(&Controllers.JoyState[i],sizeof(DIJOYSTATE2));
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

int	Controllers_GetConfigButton (HWND hWnd, int DevNum)
{
	LPDIRECTINPUTDEVICE7 dev = Controllers.DIDevices[DevNum];
	HRESULT hr;
	int i;
	int Key = -1;
	int FirstAxis, LastAxis;
	int FirstPOV, LastPOV;
	if (DevNum == 0)
	{
		FirstAxis = LastAxis = 0;
		FirstPOV = LastPOV = 0;
	}
	else if (DevNum == 1)
	{
		FirstAxis = 0x08;
		LastAxis = 0x0E;
		FirstPOV = LastPOV = 0;
	}
	else
	{
		FirstAxis = 0x80;
		LastAxis = 0x90;
		FirstPOV = 0xC0;
		LastPOV = 0xF0;
	}

	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(dev,hWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(mWnd,_T("Unable to modify device input cooperative level!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return Key;
	}

	IDirectInputDevice7_Acquire(dev);
	while (Key == -1)
	{
		if (DevNum == 0)
			hr = IDirectInputDevice7_GetDeviceState(dev,256,Controllers.KeyState);
		else if (DevNum == 1)
			hr = IDirectInputDevice7_GetDeviceState(dev,sizeof(DIMOUSESTATE2),&Controllers.MouseState);
		else
		{
			hr = IDirectInputDevice7_Poll(dev);
			hr = IDirectInputDevice7_GetDeviceState(dev,sizeof(DIJOYSTATE2),&Controllers.JoyState[DevNum]);
		}
		for (i = 0; i < Controllers.NumButtons[DevNum]; i++)
		{
			if (Controllers_IsPressed((DevNum << 16) | i))
			{
				Key = i;
				break;
			}
		}
		if (Key != -1)	/* if we got a button, don't check for an axis */
			break;
		for (i = FirstAxis; i < LastAxis; i++)
		{
			if (Controllers_IsPressed((DevNum << 16) | i))
			{
				Key = i;
				break;
			}
		}
		if (Key != -1)	/* if we got an axis, don't check for a POV hat */
			break;
		for (i = FirstPOV; i < LastPOV; i++)
		{
			if (Controllers_IsPressed((DevNum << 16) | i))
			{
				Key = i;
				break;
			}
		}
	}
	{
waitrelease:
		if (DevNum == 0)
			hr = IDirectInputDevice7_GetDeviceState(dev,256,Controllers.KeyState);
		else if (DevNum == 1)
			hr = IDirectInputDevice7_GetDeviceState(dev,sizeof(DIMOUSESTATE2),&Controllers.MouseState);
		else
		{
			hr = IDirectInputDevice7_Poll(dev);
			hr = IDirectInputDevice7_GetDeviceState(dev,sizeof(DIJOYSTATE2),&Controllers.JoyState[DevNum]);
		}
		// don't need to reset FirstAxis/LastAxis or FirstPOV/LastPOV, since they were set in the previous loop
		for (i = 0; i < Controllers.NumButtons[DevNum]; i++)
			if (Controllers_IsPressed((DevNum << 16) | i))
				goto waitrelease;
		for (i = FirstAxis; i < LastAxis; i++)
			if (Controllers_IsPressed((DevNum << 16) | i))
				goto waitrelease;
		for (i = FirstPOV; i < LastPOV; i++)
			if (Controllers_IsPressed((DevNum << 16) | i))
				goto waitrelease;
	}
	IDirectInputDevice7_Unacquire(dev);
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(dev,mWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(mWnd,_T("Unable to restore device input cooperative level!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return Key;
	}
	return Key;
}

TCHAR *	Controllers_GetButtonLabel (int DevNum, int Button)
{
	static TCHAR str[256];
	_tcscpy(str, _T("???"));
	if (DevNum == 0)
	{
		if (Controllers.KeyNames[Button])
			_tcscpy(str,Controllers.KeyNames[Button]);
		return str;
	}
	else if (DevNum == 1)
	{
		if (Button & 0x08)
		{
			Button &= 0x07;
			if (Controllers.AxisNames[1][Button >> 1])
				_stprintf(str, _T("%s %s"), Controllers.AxisNames[1][Button >> 1], (Button & 1) ? _T("(+)") : _T("(-)"));
			return str;
		}
		else
		{
			if (Controllers.ButtonNames[1][Button])
				_tcscpy(str,Controllers.ButtonNames[1][Button]);
			return str;
		}
	}
	else
	{
		if ((Button & 0xE0) == 0x80)
		{
			Button &= 0x0F;
			if (Controllers.AxisNames[DevNum][Button >> 1])
				_stprintf(str,_T("%s %s"), Controllers.AxisNames[DevNum][Button >> 1], (Button & 1) ? _T("(+)") : _T("(-)"));
			return str;
		}
		else if ((Button & 0xE0) == 0xC0)
		{
			const TCHAR *POVs[8] = {_T("(N)"), _T("(NE)"), _T("(E)"), _T("(SE)"), _T("(S)"), _T("(SW)"), _T("(W)"), _T("(NW)") };
			Button &= 0x1F;
			if (Controllers.POVNames[DevNum][Button >> 3])
				_stprintf(str,_T("%s %s"), Controllers.POVNames[DevNum][Button >> 3], POVs[Button & 0x7]);
			return str;
		}
		else if ((Button & 0xE0) == 0xE0)
		{
			const TCHAR *POVs[4] = {_T("Y (-)"), _T("X (+)"), _T("Y (+)"), _T("X (-)") };
			Button &= 0xF;
			if (Controllers.POVNames[DevNum][Button >> 2])
				_stprintf(str,_T("%s %s"), Controllers.POVNames[DevNum][Button >> 2], POVs[Button & 0x3]);
			return str;
		}
		else
		{
			if (Controllers.ButtonNames[DevNum][Button])
				_tcscpy(str,Controllers.ButtonNames[DevNum][Button]);
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
		*Button = Controllers_GetConfigButton(key,Device);
		ProcessMessages();	/* flush all keypresses - don't want them going back to the parent dialog */
		DestroyWindow(key);	/* close the little window */
		key = NULL;
	}
	SetWindowText(hDlg,(LPCTSTR)Controllers_GetButtonLabel(Device,*Button));
	*Button |= Device << 16;	/* add the device ID */
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
			switch (Button & 0x7)
			{
			case 0x0:	return ((Controllers.AxisFlags[1] & (1 << AXIS_X)) && (Controllers.MouseState.lX < -1)) ? TRUE : FALSE;	break;
			case 0x1:	return ((Controllers.AxisFlags[1] & (1 << AXIS_X)) && (Controllers.MouseState.lX > +1)) ? TRUE : FALSE;	break;
			case 0x2:	return ((Controllers.AxisFlags[1] & (1 << AXIS_Y)) && (Controllers.MouseState.lY < -1)) ? TRUE : FALSE;	break;
			case 0x3:	return ((Controllers.AxisFlags[1] & (1 << AXIS_Y)) && (Controllers.MouseState.lY > +1)) ? TRUE : FALSE;	break;
			case 0x4:	return ((Controllers.AxisFlags[1] & (1 << AXIS_Z)) && (Controllers.MouseState.lZ < -1)) ? TRUE : FALSE;	break;
			case 0x5:	return ((Controllers.AxisFlags[1] & (1 << AXIS_Z)) && (Controllers.MouseState.lZ > +1)) ? TRUE : FALSE;	break;
			default:	return FALSE;
			}
		}
		else	return (Controllers.MouseState.rgbButtons[Button & 0x7] & 0x80) ? TRUE : FALSE;
	}
	else
	{
		if ((Button & 0xE0) == 0x80)
		{	/* axis */
			switch (Button & 0xF)
			{
			case 0x0:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_X)) && (Controllers.JoyState[DevNum].lX < 0x4000)) ? TRUE : FALSE;	break;
			case 0x1:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_X)) && (Controllers.JoyState[DevNum].lX > 0xC000)) ? TRUE : FALSE;	break;
			case 0x2:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_Y)) && (Controllers.JoyState[DevNum].lY < 0x4000)) ? TRUE : FALSE;	break;
			case 0x3:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_Y)) && (Controllers.JoyState[DevNum].lY > 0xC000)) ? TRUE : FALSE;	break;
			case 0x4:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_Z)) && (Controllers.JoyState[DevNum].lZ < 0x4000)) ? TRUE : FALSE;	break;
			case 0x5:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_Z)) && (Controllers.JoyState[DevNum].lZ > 0xC000)) ? TRUE : FALSE;	break;
			case 0x6:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_RX)) && (Controllers.JoyState[DevNum].lRx < 0x4000)) ? TRUE : FALSE;	break;
			case 0x7:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_RX)) && (Controllers.JoyState[DevNum].lRx > 0xC000)) ? TRUE : FALSE;	break;
			case 0x8:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_RY)) && (Controllers.JoyState[DevNum].lRy < 0x4000)) ? TRUE : FALSE;	break;
			case 0x9:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_RY)) && (Controllers.JoyState[DevNum].lRy > 0xC000)) ? TRUE : FALSE;	break;
			case 0xA:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_RZ)) && (Controllers.JoyState[DevNum].lRz < 0x4000)) ? TRUE : FALSE;	break;
			case 0xB:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_RZ)) && (Controllers.JoyState[DevNum].lRz > 0xC000)) ? TRUE : FALSE;	break;
			case 0xC:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_S0)) && (Controllers.JoyState[DevNum].rglSlider[0] < 0x4000)) ? TRUE : FALSE;	break;
			case 0xD:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_S0)) && (Controllers.JoyState[DevNum].rglSlider[0] > 0xC000)) ? TRUE : FALSE;	break;
			case 0xE:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_S1)) && (Controllers.JoyState[DevNum].rglSlider[1] < 0x4000)) ? TRUE : FALSE;	break;
			case 0xF:	return ((Controllers.AxisFlags[DevNum] & (1 << AXIS_S1)) && (Controllers.JoyState[DevNum].rglSlider[1] > 0xC000)) ? TRUE : FALSE;	break;
			default:	return FALSE;
			}
		}
		else if ((Button & 0xE0) == 0xC0)
		{	/* POV trigger (8-button mode) */
			if (POVAxis)
				return FALSE;
			switch (Button & 0x1F)
			{
			case 0x00:	return ((Controllers.POVFlags[DevNum] & (1 << 0)) && ((Controllers.JoyState[DevNum].rgdwPOV[0] > 33750) || (Controllers.JoyState[DevNum].rgdwPOV[0] <  2250)) && (Controllers.JoyState[DevNum].rgdwPOV[0] != -1)) ? TRUE : FALSE;	break;
			case 0x01:	return ((Controllers.POVFlags[DevNum] & (1 << 0)) && ((Controllers.JoyState[DevNum].rgdwPOV[0] >  2250) && (Controllers.JoyState[DevNum].rgdwPOV[0] <  6750))) ? TRUE : FALSE;	break;
			case 0x02:	return ((Controllers.POVFlags[DevNum] & (1 << 0)) && ((Controllers.JoyState[DevNum].rgdwPOV[0] >  6750) && (Controllers.JoyState[DevNum].rgdwPOV[0] < 11250))) ? TRUE : FALSE;	break;
			case 0x03:	return ((Controllers.POVFlags[DevNum] & (1 << 0)) && ((Controllers.JoyState[DevNum].rgdwPOV[0] > 11250) && (Controllers.JoyState[DevNum].rgdwPOV[0] < 15750))) ? TRUE : FALSE;	break;
			case 0x04:	return ((Controllers.POVFlags[DevNum] & (1 << 0)) && ((Controllers.JoyState[DevNum].rgdwPOV[0] > 15750) && (Controllers.JoyState[DevNum].rgdwPOV[0] < 20250))) ? TRUE : FALSE;	break;
			case 0x05:	return ((Controllers.POVFlags[DevNum] & (1 << 0)) && ((Controllers.JoyState[DevNum].rgdwPOV[0] > 20250) && (Controllers.JoyState[DevNum].rgdwPOV[0] < 24750))) ? TRUE : FALSE;	break;
			case 0x06:	return ((Controllers.POVFlags[DevNum] & (1 << 0)) && ((Controllers.JoyState[DevNum].rgdwPOV[0] > 24750) && (Controllers.JoyState[DevNum].rgdwPOV[0] < 29250))) ? TRUE : FALSE;	break;
			case 0x07:	return ((Controllers.POVFlags[DevNum] & (1 << 0)) && ((Controllers.JoyState[DevNum].rgdwPOV[0] > 29250) && (Controllers.JoyState[DevNum].rgdwPOV[0] < 33750))) ? TRUE : FALSE;	break;
			case 0x08:	return ((Controllers.POVFlags[DevNum] & (1 << 1)) && ((Controllers.JoyState[DevNum].rgdwPOV[1] > 33750) || (Controllers.JoyState[DevNum].rgdwPOV[1] <  2250)) && (Controllers.JoyState[DevNum].rgdwPOV[1] != -1)) ? TRUE : FALSE;	break;
			case 0x09:	return ((Controllers.POVFlags[DevNum] & (1 << 1)) && ((Controllers.JoyState[DevNum].rgdwPOV[1] >  2250) && (Controllers.JoyState[DevNum].rgdwPOV[1] <  6750))) ? TRUE : FALSE;	break;
			case 0x0A:	return ((Controllers.POVFlags[DevNum] & (1 << 1)) && ((Controllers.JoyState[DevNum].rgdwPOV[1] >  6750) && (Controllers.JoyState[DevNum].rgdwPOV[1] < 11250))) ? TRUE : FALSE;	break;
			case 0x0B:	return ((Controllers.POVFlags[DevNum] & (1 << 1)) && ((Controllers.JoyState[DevNum].rgdwPOV[1] > 11250) && (Controllers.JoyState[DevNum].rgdwPOV[1] < 15750))) ? TRUE : FALSE;	break;
			case 0x0C:	return ((Controllers.POVFlags[DevNum] & (1 << 1)) && ((Controllers.JoyState[DevNum].rgdwPOV[1] > 15750) && (Controllers.JoyState[DevNum].rgdwPOV[1] < 20250))) ? TRUE : FALSE;	break;
			case 0x0D:	return ((Controllers.POVFlags[DevNum] & (1 << 1)) && ((Controllers.JoyState[DevNum].rgdwPOV[1] > 20250) && (Controllers.JoyState[DevNum].rgdwPOV[1] < 24750))) ? TRUE : FALSE;	break;
			case 0x0E:	return ((Controllers.POVFlags[DevNum] & (1 << 1)) && ((Controllers.JoyState[DevNum].rgdwPOV[1] > 24750) && (Controllers.JoyState[DevNum].rgdwPOV[1] < 29250))) ? TRUE : FALSE;	break;
			case 0x0F:	return ((Controllers.POVFlags[DevNum] & (1 << 1)) && ((Controllers.JoyState[DevNum].rgdwPOV[1] > 29250) && (Controllers.JoyState[DevNum].rgdwPOV[1] < 33750))) ? TRUE : FALSE;	break;
			case 0x10:	return ((Controllers.POVFlags[DevNum] & (1 << 2)) && ((Controllers.JoyState[DevNum].rgdwPOV[2] > 33750) || (Controllers.JoyState[DevNum].rgdwPOV[2] <  2250)) && (Controllers.JoyState[DevNum].rgdwPOV[2] != -1)) ? TRUE : FALSE;	break;
			case 0x11:	return ((Controllers.POVFlags[DevNum] & (1 << 2)) && ((Controllers.JoyState[DevNum].rgdwPOV[2] >  2250) && (Controllers.JoyState[DevNum].rgdwPOV[2] <  6750))) ? TRUE : FALSE;	break;
			case 0x12:	return ((Controllers.POVFlags[DevNum] & (1 << 2)) && ((Controllers.JoyState[DevNum].rgdwPOV[2] >  6750) && (Controllers.JoyState[DevNum].rgdwPOV[2] < 11250))) ? TRUE : FALSE;	break;
			case 0x13:	return ((Controllers.POVFlags[DevNum] & (1 << 2)) && ((Controllers.JoyState[DevNum].rgdwPOV[2] > 11250) && (Controllers.JoyState[DevNum].rgdwPOV[2] < 15750))) ? TRUE : FALSE;	break;
			case 0x14:	return ((Controllers.POVFlags[DevNum] & (1 << 2)) && ((Controllers.JoyState[DevNum].rgdwPOV[2] > 15750) && (Controllers.JoyState[DevNum].rgdwPOV[2] < 20250))) ? TRUE : FALSE;	break;
			case 0x15:	return ((Controllers.POVFlags[DevNum] & (1 << 2)) && ((Controllers.JoyState[DevNum].rgdwPOV[2] > 20250) && (Controllers.JoyState[DevNum].rgdwPOV[2] < 24750))) ? TRUE : FALSE;	break;
			case 0x16:	return ((Controllers.POVFlags[DevNum] & (1 << 2)) && ((Controllers.JoyState[DevNum].rgdwPOV[2] > 24750) && (Controllers.JoyState[DevNum].rgdwPOV[2] < 29250))) ? TRUE : FALSE;	break;
			case 0x17:	return ((Controllers.POVFlags[DevNum] & (1 << 2)) && ((Controllers.JoyState[DevNum].rgdwPOV[2] > 29250) && (Controllers.JoyState[DevNum].rgdwPOV[2] < 33750))) ? TRUE : FALSE;	break;
			case 0x18:	return ((Controllers.POVFlags[DevNum] & (1 << 3)) && ((Controllers.JoyState[DevNum].rgdwPOV[3] > 33750) || (Controllers.JoyState[DevNum].rgdwPOV[3] <  2250)) && (Controllers.JoyState[DevNum].rgdwPOV[3] != -1)) ? TRUE : FALSE;	break;
			case 0x19:	return ((Controllers.POVFlags[DevNum] & (1 << 3)) && ((Controllers.JoyState[DevNum].rgdwPOV[3] >  2250) && (Controllers.JoyState[DevNum].rgdwPOV[3] <  6750))) ? TRUE : FALSE;	break;
			case 0x1A:	return ((Controllers.POVFlags[DevNum] & (1 << 3)) && ((Controllers.JoyState[DevNum].rgdwPOV[3] >  6750) && (Controllers.JoyState[DevNum].rgdwPOV[3] < 11250))) ? TRUE : FALSE;	break;
			case 0x1B:	return ((Controllers.POVFlags[DevNum] & (1 << 3)) && ((Controllers.JoyState[DevNum].rgdwPOV[3] > 11250) && (Controllers.JoyState[DevNum].rgdwPOV[3] < 15750))) ? TRUE : FALSE;	break;
			case 0x1C:	return ((Controllers.POVFlags[DevNum] & (1 << 3)) && ((Controllers.JoyState[DevNum].rgdwPOV[3] > 15750) && (Controllers.JoyState[DevNum].rgdwPOV[3] < 20250))) ? TRUE : FALSE;	break;
			case 0x1D:	return ((Controllers.POVFlags[DevNum] & (1 << 3)) && ((Controllers.JoyState[DevNum].rgdwPOV[3] > 20250) && (Controllers.JoyState[DevNum].rgdwPOV[3] < 24750))) ? TRUE : FALSE;	break;
			case 0x1E:	return ((Controllers.POVFlags[DevNum] & (1 << 3)) && ((Controllers.JoyState[DevNum].rgdwPOV[3] > 24750) && (Controllers.JoyState[DevNum].rgdwPOV[3] < 29250))) ? TRUE : FALSE;	break;
			case 0x1F:	return ((Controllers.POVFlags[DevNum] & (1 << 3)) && ((Controllers.JoyState[DevNum].rgdwPOV[3] > 29250) && (Controllers.JoyState[DevNum].rgdwPOV[3] < 33750))) ? TRUE : FALSE;	break;
			default:	return FALSE;
			}
		}
		else if ((Button & 0xE0) == 0xE0)
		{	/* POV trigger (axis mode) */
			switch (Button & 0x0F)
			{
			case 0x0:	return ((Controllers.POVFlags[DevNum] & (1 << 0)) && ((Controllers.JoyState[DevNum].rgdwPOV[0] > 29250) || (Controllers.JoyState[DevNum].rgdwPOV[0] <  6750)) && (Controllers.JoyState[DevNum].rgdwPOV[0] != -1)) ? TRUE : FALSE;	break;
			case 0x1:	return ((Controllers.POVFlags[DevNum] & (1 << 0)) && ((Controllers.JoyState[DevNum].rgdwPOV[0] >  2250) && (Controllers.JoyState[DevNum].rgdwPOV[0] < 15750))) ? TRUE : FALSE;	break;
			case 0x2:	return ((Controllers.POVFlags[DevNum] & (1 << 0)) && ((Controllers.JoyState[DevNum].rgdwPOV[0] > 11250) && (Controllers.JoyState[DevNum].rgdwPOV[0] < 24750))) ? TRUE : FALSE;	break;
			case 0x3:	return ((Controllers.POVFlags[DevNum] & (1 << 0)) && ((Controllers.JoyState[DevNum].rgdwPOV[0] > 20250) && (Controllers.JoyState[DevNum].rgdwPOV[0] < 33750))) ? TRUE : FALSE;	break;
			case 0x4:	return ((Controllers.POVFlags[DevNum] & (1 << 1)) && ((Controllers.JoyState[DevNum].rgdwPOV[1] > 29250) || (Controllers.JoyState[DevNum].rgdwPOV[1] <  6750)) && (Controllers.JoyState[DevNum].rgdwPOV[1] != -1)) ? TRUE : FALSE;	break;
			case 0x5:	return ((Controllers.POVFlags[DevNum] & (1 << 1)) && ((Controllers.JoyState[DevNum].rgdwPOV[1] >  2250) && (Controllers.JoyState[DevNum].rgdwPOV[1] < 15750))) ? TRUE : FALSE;	break;
			case 0x6:	return ((Controllers.POVFlags[DevNum] & (1 << 1)) && ((Controllers.JoyState[DevNum].rgdwPOV[1] > 11250) && (Controllers.JoyState[DevNum].rgdwPOV[1] < 24750))) ? TRUE : FALSE;	break;
			case 0x7:	return ((Controllers.POVFlags[DevNum] & (1 << 1)) && ((Controllers.JoyState[DevNum].rgdwPOV[1] > 20250) && (Controllers.JoyState[DevNum].rgdwPOV[1] < 33750))) ? TRUE : FALSE;	break;
			case 0x8:	return ((Controllers.POVFlags[DevNum] & (1 << 2)) && ((Controllers.JoyState[DevNum].rgdwPOV[2] > 29250) || (Controllers.JoyState[DevNum].rgdwPOV[2] <  6750)) && (Controllers.JoyState[DevNum].rgdwPOV[2] != -1)) ? TRUE : FALSE;	break;
			case 0x9:	return ((Controllers.POVFlags[DevNum] & (1 << 2)) && ((Controllers.JoyState[DevNum].rgdwPOV[2] >  2250) && (Controllers.JoyState[DevNum].rgdwPOV[2] < 15750))) ? TRUE : FALSE;	break;
			case 0xA:	return ((Controllers.POVFlags[DevNum] & (1 << 2)) && ((Controllers.JoyState[DevNum].rgdwPOV[2] > 11250) && (Controllers.JoyState[DevNum].rgdwPOV[2] < 24750))) ? TRUE : FALSE;	break;
			case 0xB:	return ((Controllers.POVFlags[DevNum] & (1 << 2)) && ((Controllers.JoyState[DevNum].rgdwPOV[2] > 20250) && (Controllers.JoyState[DevNum].rgdwPOV[2] < 33750))) ? TRUE : FALSE;	break;
			case 0xC:	return ((Controllers.POVFlags[DevNum] & (1 << 3)) && ((Controllers.JoyState[DevNum].rgdwPOV[3] > 29250) || (Controllers.JoyState[DevNum].rgdwPOV[3] <  6750)) && (Controllers.JoyState[DevNum].rgdwPOV[3] != -1)) ? TRUE : FALSE;	break;
			case 0xD:	return ((Controllers.POVFlags[DevNum] & (1 << 3)) && ((Controllers.JoyState[DevNum].rgdwPOV[3] >  2250) && (Controllers.JoyState[DevNum].rgdwPOV[3] < 15750))) ? TRUE : FALSE;	break;
			case 0xE:	return ((Controllers.POVFlags[DevNum] & (1 << 3)) && ((Controllers.JoyState[DevNum].rgdwPOV[3] > 11250) && (Controllers.JoyState[DevNum].rgdwPOV[3] < 24750))) ? TRUE : FALSE;	break;
			case 0xF:	return ((Controllers.POVFlags[DevNum] & (1 << 3)) && ((Controllers.JoyState[DevNum].rgdwPOV[3] > 20250) && (Controllers.JoyState[DevNum].rgdwPOV[3] < 33750))) ? TRUE : FALSE;	break;
			default:	return FALSE;
			}
		}
		else	return (Controllers.JoyState[DevNum].rgbButtons[Button & 0x7F] & 0x80) ? TRUE : FALSE;
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