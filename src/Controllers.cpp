/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2009 QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "MapperInterface.h"
#include "Movie.h"
#include "Controllers.h"
#include <commdlg.h>

namespace Controllers
{
static	HWND key;

struct tStdPort Port1, Port2;
struct tStdPort FSPort1, FSPort2, FSPort3, FSPort4;
struct tExpPort ExpPort;

void	StdPort_SetUnconnected		(struct tStdPort *);
void	StdPort_SetStdController	(struct tStdPort *);
void	StdPort_SetZapper		(struct tStdPort *);
void	StdPort_SetArkanoidPaddle	(struct tStdPort *);
void	StdPort_SetPowerPad		(struct tStdPort *);
void	StdPort_SetFourScore		(struct tStdPort *);
void	StdPort_SetSnesController	(struct tStdPort *);
void	StdPort_SetVSZapper		(struct tStdPort *);

void	ExpPort_SetUnconnected		(struct tExpPort *);
void	ExpPort_SetFami4Play		(struct tExpPort *);
void	ExpPort_SetArkanoidPaddle	(struct tExpPort *);
void	ExpPort_SetFamilyBasicKeyboard	(struct tExpPort *);
void	ExpPort_SetAltKeyboard		(struct tExpPort *);
void	ExpPort_SetFamTrainer		(struct tExpPort *);
void	ExpPort_SetTablet		(struct tExpPort *);

BOOL	EnableOpposites;

int	NumDevices;
BOOL	DeviceUsed[MAX_CONTROLLERS];
TCHAR	*DeviceName[MAX_CONTROLLERS];

int	NumButtons[MAX_CONTROLLERS];
BYTE	AxisFlags[MAX_CONTROLLERS],
	POVFlags[MAX_CONTROLLERS];
	
TCHAR	*ButtonNames[MAX_CONTROLLERS][128],
	*AxisNames[MAX_CONTROLLERS][8],
	*POVNames[MAX_CONTROLLERS][4],
	*KeyNames[256];

LPDIRECTINPUT7		DirectInput;
LPDIRECTINPUTDEVICE7	DIDevices[MAX_CONTROLLERS];

BYTE		KeyState[256];
DIMOUSESTATE2	MouseState;
DIJOYSTATE2	JoyState[MAX_CONTROLLERS];	// first 2 entries are unused

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
	default:MessageBox(hMainWnd,_T("Error: selected invalid controller type for standard port!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);	break;
	}
}
const TCHAR	*StdPort_Mappings[STD_MAX];
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
	default:MessageBox(hMainWnd,_T("Error: selected invalid controller type for expansion port!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);	break;
	}
}
const TCHAR	*ExpPort_Mappings[EXP_MAX];
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

static	INT_PTR	CALLBACK	ControllerProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	int i;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_RESETCONTENT,0,0);
		for (i = 0; i < STD_MAX; i++)
		{
			SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)StdPort_Mappings[i]);
			SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)StdPort_Mappings[i]);
		}
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,Port1.Type,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,Port2.Type,0);

		SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_RESETCONTENT,0,0);
		for (i = 0; i < EXP_MAX; i++)
			SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_ADDSTRING,0,(LPARAM)ExpPort_Mappings[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_SETCURSEL,ExpPort.Type,0);
		if (Movie::Mode)
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

		if (EnableOpposites)
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
			EnableOpposites = (IsDlgButtonChecked(hDlg,IDC_CONT_UDLR) == BST_CHECKED);
			POVAxis = FALSE;
			EndDialog(hDlg,1);
			break;
		case IDC_CONT_SPORT1:
			if (wmEvent == CBN_SELCHANGE)
			{
				int Type = (int)SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_GETCURSEL,0,0);
				if (Type == STD_FOURSCORE)
				{
					StdPort_SetControllerType(&FSPort1,Port1.Type == STD_STDCONTROLLER ? STD_STDCONTROLLER : STD_UNCONNECTED);
					StdPort_SetControllerType(&FSPort2,Port2.Type == STD_STDCONTROLLER ? STD_STDCONTROLLER : STD_UNCONNECTED);
					memcpy(FSPort1.Buttons,Port1.Buttons,sizeof(Port1.Buttons));
					memcpy(FSPort2.Buttons,Port2.Buttons,sizeof(Port2.Buttons));
					StdPort_SetControllerType(&Port1,STD_FOURSCORE);
					StdPort_SetControllerType(&Port2,STD_FOURSCORE);
					SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,STD_FOURSCORE,0);
				}
				else if (Port1.Type	== STD_FOURSCORE)
				{
					StdPort_SetControllerType(&Port1,Type);
					StdPort_SetControllerType(&Port2,FSPort2.Type);
					memcpy(Port2.Buttons,FSPort2.Buttons,sizeof(FSPort2.Buttons));
					SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,Port2.Type,0);
				}
				else	StdPort_SetControllerType(&Port1,Type);
			}
			break;
		case IDC_CONT_SPORT2:
			if (wmEvent == CBN_SELCHANGE)
			{
				int Type = (int)SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_GETCURSEL,0,0);
				if (Type == STD_FOURSCORE)
				{
					StdPort_SetControllerType(&FSPort1,Port1.Type == STD_STDCONTROLLER ? STD_STDCONTROLLER : STD_UNCONNECTED);
					StdPort_SetControllerType(&FSPort2,Port2.Type == STD_STDCONTROLLER ? STD_STDCONTROLLER : STD_UNCONNECTED);
					memcpy(FSPort1.Buttons,Port1.Buttons,sizeof(Port1.Buttons));
					memcpy(FSPort2.Buttons,Port2.Buttons,sizeof(Port2.Buttons));
					StdPort_SetControllerType(&Port1,STD_FOURSCORE);
					StdPort_SetControllerType(&Port2,STD_FOURSCORE);
					SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,STD_FOURSCORE,0);
				}
				else if (Port2.Type	== STD_FOURSCORE)
				{
					StdPort_SetControllerType(&Port1,FSPort1.Type);
					memcpy(Port1.Buttons,FSPort1.Buttons,sizeof(FSPort1.Buttons));
					StdPort_SetControllerType(&Port2,Type);
					SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,Port1.Type,0);
				}
				else	StdPort_SetControllerType(&Port2,Type);
			}
			break;
		case IDC_CONT_SEXPPORT:
			if (wmEvent == CBN_SELCHANGE)
			{
				int Type = (int)SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_GETCURSEL,0,0);
				ExpPort_SetControllerType(&ExpPort,Type);
			}
			break;
		case IDC_CONT_CPORT1:
			Port1.Config(&Port1,hDlg);
			break;
		case IDC_CONT_CPORT2:
			Port2.Config(&Port2,hDlg);
			break;
		case IDC_CONT_CEXPPORT:
			ExpPort.Config(&ExpPort,hDlg);
			break;
		};
		break;
	}

	return FALSE;
}
void	OpenConfig (void)
{
	UnAcquire();
	DialogBox(hInst,(LPCTSTR)IDD_CONTROLLERS,hMainWnd,ControllerProc);
	SetDeviceUsed();
	Acquire();
}

static	BOOL CALLBACK	EnumKeyboardObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	int ItemNum = 0;
	if (IsEqualGUID(lpddoi->guidType,GUID_Key))
	{
		ItemNum = lpddoi->dwOfs;
		if ((ItemNum >= 0) && (ItemNum < 256))
			KeyNames[ItemNum] = _tcsdup(lpddoi->tszName);
		else	MessageBox(hMainWnd, _T("Error - encountered invalid keyboard key ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}
	return DIENUM_CONTINUE;
}

static	BOOL CALLBACK	EnumMouseObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	int ItemNum = 0;
	if (IsEqualGUID(lpddoi->guidType,GUID_XAxis))
	{
		AxisFlags[1] |= 0x01;
		AxisNames[1][AXIS_X] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType,GUID_YAxis))
	{
		AxisFlags[1] |= 0x02;
		AxisNames[1][AXIS_Y] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType,GUID_ZAxis))
	{
		AxisFlags[1] |= 0x04;
		AxisNames[1][AXIS_Z] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType,GUID_Button))
	{
		ItemNum = (lpddoi->dwOfs - ((BYTE *)&MouseState.rgbButtons - (BYTE *)&MouseState)) / sizeof(MouseState.rgbButtons[0]);
		if ((ItemNum >= 0) && (ItemNum < 8))
			ButtonNames[1][ItemNum] = _tcsdup(lpddoi->tszName);
		else	MessageBox(hMainWnd, _T("Error - encountered invalid mouse button ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}

	return DIENUM_CONTINUE;
}

static	BOOL CALLBACK	EnumJoystickObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	int DevNum = NumDevices;
	int ItemNum = 0;
	if (IsEqualGUID(lpddoi->guidType,GUID_XAxis))
	{
		AxisFlags[DevNum] |= 0x01;
		AxisNames[DevNum][AXIS_X] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType,GUID_YAxis))
	{
		AxisFlags[DevNum] |= 0x02;
		AxisNames[DevNum][AXIS_Y] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType,GUID_ZAxis))
	{
		AxisFlags[DevNum] |= 0x04;
		AxisNames[DevNum][AXIS_Z] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType,GUID_RxAxis))
	{
		AxisFlags[DevNum] |= 0x08;
		AxisNames[DevNum][AXIS_RX] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType,GUID_RyAxis))
	{
		AxisFlags[DevNum] |= 0x10;
		AxisNames[DevNum][AXIS_RY] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType,GUID_RzAxis))
	{
		AxisFlags[DevNum] |= 0x20;
		AxisNames[DevNum][AXIS_RZ] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType,GUID_Slider))
	{
		ItemNum = (lpddoi->dwOfs - ((BYTE *)&JoyState[0].rglSlider - (BYTE *)&JoyState[0])) / sizeof(JoyState[0].rglSlider[0]);
		if (ItemNum == 0)
		{
			AxisFlags[DevNum] |= 0x40;
			AxisNames[DevNum][AXIS_S0] = _tcsdup(lpddoi->tszName);
		}
		else if (ItemNum == 1)
		{
			AxisFlags[DevNum] |= 0x80;
			AxisNames[DevNum][AXIS_S1] = _tcsdup(lpddoi->tszName);
		}
		else	MessageBox(hMainWnd, _T("Error - encountered invalid slider ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}
	if (IsEqualGUID(lpddoi->guidType,GUID_POV))
	{
		ItemNum = (lpddoi->dwOfs - ((BYTE *)&JoyState[0].rgdwPOV - (BYTE *)&JoyState[0])) / sizeof(JoyState[0].rgdwPOV[0]);
		if ((ItemNum >= 0) && (ItemNum < 4))
		{
			POVFlags[DevNum] |= 0x01 << ItemNum;
			POVNames[DevNum][ItemNum] = _tcsdup(lpddoi->tszName);
		}
		else	MessageBox(hMainWnd, _T("Error - encountered invalid POV hat ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}
	if (IsEqualGUID(lpddoi->guidType,GUID_Button))
	{
		ItemNum = (lpddoi->dwOfs - ((BYTE *)&JoyState[0].rgbButtons - (BYTE *)&JoyState[0])) / sizeof(JoyState[0].rgbButtons[0]);
		if ((ItemNum >= 0) && (ItemNum < 128))
			ButtonNames[DevNum][ItemNum] = _tcsdup(lpddoi->tszName);
		else	MessageBox(hMainWnd, _T("Error - encountered invalid joystick button ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}

	return DIENUM_CONTINUE;
}

static	BOOL CALLBACK	EnumJoysticksCallback (LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	HRESULT hr;
	int DevNum = NumDevices;
	if (SUCCEEDED(IDirectInput7_CreateDeviceEx(DirectInput,lpddi->guidInstance,IID_IDirectInputDevice7,(LPVOID *)&DIDevices[DevNum],NULL)))
	{
		DIDEVCAPS caps;
		if (FAILED(hr = IDirectInputDevice7_SetDataFormat(DIDevices[DevNum],&c_dfDIJoystick2)))
			goto end;
		if (FAILED(hr = IDirectInputDevice7_SetCooperativeLevel(DIDevices[DevNum],hMainWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
			goto end;
		caps.dwSize = sizeof(DIDEVCAPS);
		if (FAILED(hr = IDirectInputDevice7_GetCapabilities(DIDevices[DevNum],&caps)))
			goto end;

		NumButtons[DevNum] = caps.dwButtons;
		AxisFlags[DevNum] = 0;
		POVFlags[DevNum] = 0;
		DeviceName[DevNum] = _tcsdup(lpddi->tszProductName);

		IDirectInputDevice7_EnumObjects(DIDevices[DevNum],EnumJoystickObjectsCallback,NULL,DIDFT_ALL);
		EI.DbgOut(_T("Added input device '%s' with %i buttons, %i axes, %i POVs"), lpddi->tszProductName, caps.dwButtons, caps.dwAxes, caps.dwPOVs);
		NumDevices++;
	}
	return DIENUM_CONTINUE;

end:
	IDirectInputDevice7_Release(DIDevices[DevNum]);
	DIDevices[DevNum] = NULL;
	return hr;
}

static	BOOL	InitKeyboard (void)
{
	DIDEVICEINSTANCE inst;
	DIDEVCAPS caps;
	if (FAILED(IDirectInput7_CreateDeviceEx(DirectInput,GUID_SysKeyboard,IID_IDirectInputDevice7,(LPVOID *)&DIDevices[0],NULL)))
		return FALSE;
	if (FAILED(IDirectInputDevice7_SetDataFormat(DIDevices[0],&c_dfDIKeyboard)))
		goto end;
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(DIDevices[0],hMainWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
		goto end;

	caps.dwSize = sizeof(DIDEVCAPS);
	if (FAILED(IDirectInputDevice7_GetCapabilities(DIDevices[0],&caps)))
		goto end;

	inst.dwSize = sizeof(DIDEVICEINSTANCE);
	if (FAILED(IDirectInputDevice7_GetDeviceInfo(DIDevices[0],&inst)))
		goto end;

	NumButtons[0] = 256;// normally, I would use caps.dwButtons
	AxisFlags[0] = 0;	// no axes
	POVFlags[0] = 0;	// no POV hats
	DeviceName[0] = _tcsdup(inst.tszProductName);

	IDirectInputDevice7_EnumObjects(DIDevices[0],EnumKeyboardObjectsCallback,NULL,DIDFT_ALL);
	EI.DbgOut(_T("Added input device '%s' with %i buttons, %i axes, %i POVs"), inst.tszProductName, caps.dwButtons, caps.dwAxes, caps.dwPOVs);
	return TRUE;

end:
	IDirectInputDevice7_Release(DIDevices[0]);
	DIDevices[0] = NULL;
	return FALSE;
}

static	BOOL	InitMouse (void)
{
	DIDEVICEINSTANCE inst;
	DIDEVCAPS caps;
	if (FAILED(IDirectInput7_CreateDeviceEx(DirectInput,GUID_SysMouse,IID_IDirectInputDevice7,(LPVOID *)&DIDevices[1],NULL)))
		return FALSE;
	if (FAILED(IDirectInputDevice7_SetDataFormat(DIDevices[1],&c_dfDIMouse2)))
		goto end;
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(DIDevices[1],hMainWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
		goto end;
	caps.dwSize = sizeof(DIDEVCAPS);
	if (FAILED(IDirectInputDevice7_GetCapabilities(DIDevices[1],&caps)))
		goto end;

	inst.dwSize = sizeof(DIDEVICEINSTANCE);
	if (FAILED(IDirectInputDevice7_GetDeviceInfo(DIDevices[1],&inst)))
		goto end;

	NumButtons[1] = caps.dwButtons;
	AxisFlags[1] = 0;
	POVFlags[1] = 0;
	DeviceName[1] = _tcsdup(inst.tszProductName);

	IDirectInputDevice7_EnumObjects(DIDevices[1],EnumMouseObjectsCallback,NULL,DIDFT_ALL);
	EI.DbgOut(_T("Added input device '%s' with %i buttons, %i axes, %i POVs"), inst.tszProductName, caps.dwButtons, caps.dwAxes, caps.dwPOVs);
	return TRUE;

end:
	IDirectInputDevice7_Release(DIDevices[1]);
	DIDevices[1] = NULL;
	return FALSE;
}

void	Init (void)
{
	int i;
	
	for (i = 0; i < NumDevices; i++)
	{
		DeviceUsed[i] = FALSE;
		DIDevices[i] = NULL;
		DeviceName[i] = NULL;
		memset(AxisNames[i],0,sizeof(AxisNames[i]));
		memset(ButtonNames[i],0,sizeof(ButtonNames[i]));
		memset(POVNames[i],0,sizeof(POVNames[i]));
	}
	memset(KeyNames,0,sizeof(KeyNames));

	StdPort_SetMappings();
	ExpPort_SetMappings();

	ZeroMemory(Port1.Buttons,sizeof(Port1.Buttons));
	ZeroMemory(Port2.Buttons,sizeof(Port2.Buttons));
	ZeroMemory(FSPort1.Buttons,sizeof(FSPort1.Buttons));
	ZeroMemory(FSPort2.Buttons,sizeof(FSPort2.Buttons));
	ZeroMemory(FSPort3.Buttons,sizeof(FSPort3.Buttons));
	ZeroMemory(FSPort4.Buttons,sizeof(FSPort4.Buttons));
	ZeroMemory(ExpPort.Buttons,sizeof(ExpPort.Buttons));

	StdPort_SetUnconnected(&Port1);
	StdPort_SetUnconnected(&Port2);
	StdPort_SetUnconnected(&FSPort1);
	StdPort_SetUnconnected(&FSPort2);
	StdPort_SetUnconnected(&FSPort3);
	StdPort_SetUnconnected(&FSPort4);
	ExpPort_SetUnconnected(&ExpPort);
	
	if (FAILED(DirectInputCreateEx(hInst,DIRECTINPUT_VERSION,IID_IDirectInput7,(LPVOID *)&DirectInput,NULL)))
	{
		MessageBox(hMainWnd,_T("Unable to initialize DirectInput!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}

	if (!InitKeyboard())
		MessageBox(hMainWnd,_T("Failed to initialize keyboard!"),_T("Nintendulator"),MB_OK | MB_ICONWARNING);
	if (!InitMouse())
		MessageBox(hMainWnd,_T("Failed to initialize mouse!"),_T("Nintendulator"),MB_OK | MB_ICONWARNING);

	NumDevices = 2;	// joysticks start at slot 2
	IDirectInput7_EnumDevices(DirectInput,DIDEVTYPE_JOYSTICK,EnumJoysticksCallback,NULL,DIEDFL_ATTACHEDONLY);

	Movie::Mode = 0;
	Acquire();
}

void	Release (void)
{
	int i, j;
	UnAcquire();
	Port1.Unload(&Port1);
	Port2.Unload(&Port2);
	FSPort1.Unload(&FSPort1);
	FSPort2.Unload(&FSPort2);
	FSPort3.Unload(&FSPort3);
	FSPort4.Unload(&FSPort4);
	ExpPort.Unload(&ExpPort);
	for (i = 0; i < NumDevices; i++)
	{
		if (DIDevices[i])
		{
			IDirectInputDevice7_Unacquire(DIDevices[i]);
			IDirectInputDevice7_Release(DIDevices[i]);
		}
		free(DeviceName[i]);
		DeviceName[i] = NULL;
		for (j = 0; j < 128; j++)
		{
			free(ButtonNames[i][j]);
			ButtonNames[i][j] = NULL;
		}
		for (j = 0; j < 8; j++)
		{
			free(AxisNames[i][j]);
			AxisNames[i][j] = NULL;
		}
		for (j = 0; j < 4; j++)
		{
			free(POVNames[i][j]);
			POVNames[i][j] = NULL;
		}
	}
	for (j = 0; j < 256; j++)
	{
		free(KeyNames[j]);
		KeyNames[j] = NULL;
	}

	IDirectInput7_Release(DirectInput);
}

void	Write (unsigned char Val)
{
	Port1.Write(&Port1,Val & 0x1);
	Port2.Write(&Port2,Val & 0x1);
	ExpPort.Write(&ExpPort,Val);
}

int	Save (FILE *out)
{
	int clen = 0;
	fwrite(&FSPort1.Type,4,1,out);	clen += 4;
	fwrite(&FSPort2.Type,4,1,out);	clen += 4;
	fwrite(&FSPort3.Type,4,1,out);	clen += 4;
	fwrite(&FSPort4.Type,4,1,out);	clen += 4;
	fwrite(&Port1.Type,4,1,out);	clen += 4;
	fwrite(&Port2.Type,4,1,out);	clen += 4;
	fwrite(&ExpPort.Type,4,1,out);	clen += 4;
	fwrite(Port1.Data,Port1.DataLen,1,out);		clen += Port1.DataLen;
	fwrite(Port2.Data,Port2.DataLen,1,out);		clen += Port2.DataLen;
	fwrite(FSPort1.Data,FSPort1.DataLen,1,out);	clen += FSPort1.DataLen;
	fwrite(FSPort2.Data,FSPort2.DataLen,1,out);	clen += FSPort2.DataLen;
	fwrite(FSPort3.Data,FSPort3.DataLen,1,out);	clen += FSPort3.DataLen;
	fwrite(FSPort4.Data,FSPort4.DataLen,1,out);	clen += FSPort4.DataLen;
	fwrite(ExpPort.Data,ExpPort.DataLen,1,out);	clen += ExpPort.DataLen;
	return clen;
}

int	Load (FILE *in)
{
	int clen = 0;
	int tpi;
	Movie::ControllerTypes[3] = 1;	// denotes that controller state has been loaded
					// if we're playing a movie, this means we should
					// SKIP the controller info in the movie block
	fread(&tpi,4,1,in);	StdPort_SetControllerType(&FSPort1,tpi);	clen += 4;
	fread(&tpi,4,1,in);	StdPort_SetControllerType(&FSPort2,tpi);	clen += 4;
	fread(&tpi,4,1,in);	StdPort_SetControllerType(&FSPort3,tpi);	clen += 4;
	fread(&tpi,4,1,in);	StdPort_SetControllerType(&FSPort4,tpi);	clen += 4;
	fread(&tpi,4,1,in);	StdPort_SetControllerType(&Port1,tpi);	clen += 4;
	fread(&tpi,4,1,in);	StdPort_SetControllerType(&Port2,tpi);	clen += 4;
	fread(&tpi,4,1,in);	ExpPort_SetControllerType(&ExpPort,tpi);	clen += 4;

	fread(Port1.Data,Port1.DataLen,1,in);		clen += Port1.DataLen;
	fread(Port2.Data,Port2.DataLen,1,in);		clen += Port2.DataLen;
	fread(FSPort1.Data,FSPort1.DataLen,1,in);	clen += FSPort1.DataLen;
	fread(FSPort2.Data,FSPort2.DataLen,1,in);	clen += FSPort2.DataLen;
	fread(FSPort3.Data,FSPort3.DataLen,1,in);	clen += FSPort3.DataLen;
	fread(FSPort4.Data,FSPort4.DataLen,1,in);	clen += FSPort4.DataLen;
	fread(ExpPort.Data,ExpPort.DataLen,1,in);	clen += ExpPort.DataLen;
	return clen;
}

void	SetDeviceUsed (void)
{
	int i;

	for (i = 0; i < NumDevices; i++)
		DeviceUsed[i] = FALSE;

	for (i = 0; i < CONTROLLERS_MAXBUTTONS; i++)
	{
		if (Port1.Type == STD_FOURSCORE)
		{
			if (i < FSPort1.NumButtons)
				DeviceUsed[FSPort1.Buttons[i] >> 16] = TRUE;
			if (i < FSPort2.NumButtons)
				DeviceUsed[FSPort2.Buttons[i] >> 16] = TRUE;
			if (i < FSPort3.NumButtons)
				DeviceUsed[FSPort3.Buttons[i] >> 16] = TRUE;
			if (i < FSPort4.NumButtons)
				DeviceUsed[FSPort4.Buttons[i] >> 16] = TRUE;
		}
		else
		{
			if (i < Port1.NumButtons)
				DeviceUsed[Port1.Buttons[i] >> 16] = TRUE;
			if (i < Port2.NumButtons)
				DeviceUsed[Port2.Buttons[i] >> 16] = TRUE;
		}
		if (i < ExpPort.NumButtons)
			DeviceUsed[ExpPort.Buttons[i] >> 16] = TRUE;
	}
	if ((Port1.Type == STD_ARKANOIDPADDLE) || (Port2.Type == STD_ARKANOIDPADDLE) || (ExpPort.Type == EXP_ARKANOIDPADDLE))
		DeviceUsed[1] = TRUE;

	if ((ExpPort.Type == EXP_FAMILYBASICKEYBOARD) || (ExpPort.Type == EXP_ALTKEYBOARD))
		DeviceUsed[0] = TRUE;
}

void	Acquire (void)
{
	int i;
	for (i = 0; i < NumDevices; i++)
		if (DIDevices[i])
			IDirectInputDevice7_Acquire(DIDevices[i]);
	if ((ExpPort.Type == EXP_FAMILYBASICKEYBOARD) || (ExpPort.Type == EXP_ALTKEYBOARD))
		MaskKeyboard = 1;
}
void	UnAcquire (void)
{
	int i;
	for (i = 0; i < NumDevices; i++)
		if (DIDevices[i])
			IDirectInputDevice7_Unacquire(DIDevices[i]);
	MaskKeyboard = 0;
}

void	UpdateInput (void)
{
	HRESULT hr;
	int i;
	unsigned char Cmd = 0;
	if (DeviceUsed[0])
	{
		hr = IDirectInputDevice7_GetDeviceState(DIDevices[0],256,KeyState);
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			IDirectInputDevice7_Acquire(DIDevices[0]);
			ZeroMemory(KeyState,sizeof(KeyState));
		}
	}
	if (DeviceUsed[1])
	{
		hr = IDirectInputDevice7_GetDeviceState(DIDevices[1],sizeof(DIMOUSESTATE2),&MouseState);
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			IDirectInputDevice7_Acquire(DIDevices[1]);
			ZeroMemory(&MouseState,sizeof(MouseState));
		}
	}
	for (i = 2; i < NumDevices; i++)
		if (DeviceUsed[i])
		{
			hr = IDirectInputDevice7_Poll(DIDevices[i]);
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
			{
				IDirectInputDevice7_Acquire(DIDevices[i]);
				ZeroMemory(&JoyState[i],sizeof(DIJOYSTATE2));
				continue;
			}
			hr = IDirectInputDevice7_GetDeviceState(DIDevices[i],sizeof(DIJOYSTATE2),&JoyState[i]);
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
				ZeroMemory(&JoyState[i],sizeof(DIJOYSTATE2));
		}

	if (Movie::Mode & MOV_PLAY)
		Cmd = Movie::LoadInput();
	else
	{
		if ((MI) && (MI->Config))
			Cmd = MI->Config(CFG_QUERY,0);
	}
	Port1.Frame(&Port1,Movie::Mode);
	Port2.Frame(&Port2,Movie::Mode);
	ExpPort.Frame(&ExpPort,Movie::Mode);
	if ((Cmd) && (MI) && (MI->Config))
		MI->Config(CFG_CMD,Cmd);
	if (Movie::Mode & MOV_RECORD)
		Movie::SaveInput(Cmd);
}

int	GetConfigButton (HWND hWnd, int DevNum)
{
	LPDIRECTINPUTDEVICE7 dev = DIDevices[DevNum];
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
		MessageBox(hMainWnd,_T("Unable to modify device input cooperative level!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return Key;
	}

	IDirectInputDevice7_Acquire(dev);
	while (Key == -1)
	{
		if (DevNum == 0)
			hr = IDirectInputDevice7_GetDeviceState(dev,256,KeyState);
		else if (DevNum == 1)
			hr = IDirectInputDevice7_GetDeviceState(dev,sizeof(DIMOUSESTATE2),&MouseState);
		else
		{
			hr = IDirectInputDevice7_Poll(dev);
			hr = IDirectInputDevice7_GetDeviceState(dev,sizeof(DIJOYSTATE2),&JoyState[DevNum]);
		}
		for (i = 0; i < NumButtons[DevNum]; i++)
		{
			if (IsPressed((DevNum << 16) | i))
			{
				Key = i;
				break;
			}
		}
		if (Key != -1)	/* if we got a button, don't check for an axis */
			break;
		for (i = FirstAxis; i < LastAxis; i++)
		{
			if (IsPressed((DevNum << 16) | i))
			{
				Key = i;
				break;
			}
		}
		if (Key != -1)	/* if we got an axis, don't check for a POV hat */
			break;
		for (i = FirstPOV; i < LastPOV; i++)
		{
			if (IsPressed((DevNum << 16) | i))
			{
				Key = i;
				break;
			}
		}
	}
	{
waitrelease:
		if (DevNum == 0)
			hr = IDirectInputDevice7_GetDeviceState(dev,256,KeyState);
		else if (DevNum == 1)
			hr = IDirectInputDevice7_GetDeviceState(dev,sizeof(DIMOUSESTATE2),&MouseState);
		else
		{
			hr = IDirectInputDevice7_Poll(dev);
			hr = IDirectInputDevice7_GetDeviceState(dev,sizeof(DIJOYSTATE2),&JoyState[DevNum]);
		}
		// don't need to reset FirstAxis/LastAxis or FirstPOV/LastPOV, since they were set in the previous loop
		for (i = 0; i < NumButtons[DevNum]; i++)
			if (IsPressed((DevNum << 16) | i))
				goto waitrelease;
		for (i = FirstAxis; i < LastAxis; i++)
			if (IsPressed((DevNum << 16) | i))
				goto waitrelease;
		for (i = FirstPOV; i < LastPOV; i++)
			if (IsPressed((DevNum << 16) | i))
				goto waitrelease;
	}
	IDirectInputDevice7_Unacquire(dev);
	if (FAILED(IDirectInputDevice7_SetCooperativeLevel(dev,hMainWnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(hMainWnd,_T("Unable to restore device input cooperative level!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return Key;
	}
	return Key;
}

TCHAR *	GetButtonLabel (int DevNum, int Button)
{
	static TCHAR str[256];
	_tcscpy(str, _T("???"));
	if (DevNum == 0)
	{
		if (KeyNames[Button])
			_tcscpy(str,KeyNames[Button]);
		return str;
	}
	else if (DevNum == 1)
	{
		if (Button & 0x08)
		{
			Button &= 0x07;
			if (AxisNames[1][Button >> 1])
				_stprintf(str, _T("%s %s"), AxisNames[1][Button >> 1], (Button & 1) ? _T("(+)") : _T("(-)"));
			return str;
		}
		else
		{
			if (ButtonNames[1][Button])
				_tcscpy(str,ButtonNames[1][Button]);
			return str;
		}
	}
	else
	{
		if ((Button & 0xE0) == 0x80)
		{
			Button &= 0x0F;
			if (AxisNames[DevNum][Button >> 1])
				_stprintf(str,_T("%s %s"), AxisNames[DevNum][Button >> 1], (Button & 1) ? _T("(+)") : _T("(-)"));
			return str;
		}
		else if ((Button & 0xE0) == 0xC0)
		{
			const TCHAR *POVs[8] = {_T("(N)"), _T("(NE)"), _T("(E)"), _T("(SE)"), _T("(S)"), _T("(SW)"), _T("(W)"), _T("(NW)") };
			Button &= 0x1F;
			if (POVNames[DevNum][Button >> 3])
				_stprintf(str,_T("%s %s"), POVNames[DevNum][Button >> 3], POVs[Button & 0x7]);
			return str;
		}
		else if ((Button & 0xE0) == 0xE0)
		{
			const TCHAR *POVs[4] = {_T("Y (-)"), _T("X (+)"), _T("Y (+)"), _T("X (-)") };
			Button &= 0xF;
			if (POVNames[DevNum][Button >> 2])
				_stprintf(str,_T("%s %s"), POVNames[DevNum][Button >> 2], POVs[Button & 0x3]);
			return str;
		}
		else
		{
			if (ButtonNames[DevNum][Button])
				_tcscpy(str,ButtonNames[DevNum][Button]);
			return str;
		}
	}
}

void	ConfigButton (int *Button, int Device, HWND hDlg, BOOL getKey)
{
	*Button &= 0xFFFF;
	if (getKey)	/* this way, we can just re-label the button */
	{
		key = CreateDialog(hInst,(LPCTSTR)IDD_KEYCONFIG,hDlg,NULL);
		ShowWindow(key,TRUE);	/* FIXME - center this window properly */
		ProcessMessages();	/* let the "Press a key..." dialog display itself */
		*Button = GetConfigButton(key,Device);
		ProcessMessages();	/* flush all keypresses - don't want them going back to the parent dialog */
		DestroyWindow(key);	/* close the little window */
		key = NULL;
	}
	SetWindowText(hDlg,(LPCTSTR)GetButtonLabel(Device,*Button));
	*Button |= Device << 16;	/* add the device ID */
}

BOOL	IsPressed (int Button)
{
	int DevNum = (Button & 0xFFFF0000) >> 16;
	if (DevNum == 0)
		return (KeyState[Button & 0xFF] & 0x80) ? TRUE : FALSE;
	else if (DevNum == 1)
	{
		if (Button & 0x8)	/* axis selected */
		{
			switch (Button & 0x7)
			{
			case 0x0:	return ((AxisFlags[1] & (1 << AXIS_X)) && (MouseState.lX < -1)) ? TRUE : FALSE;	break;
			case 0x1:	return ((AxisFlags[1] & (1 << AXIS_X)) && (MouseState.lX > +1)) ? TRUE : FALSE;	break;
			case 0x2:	return ((AxisFlags[1] & (1 << AXIS_Y)) && (MouseState.lY < -1)) ? TRUE : FALSE;	break;
			case 0x3:	return ((AxisFlags[1] & (1 << AXIS_Y)) && (MouseState.lY > +1)) ? TRUE : FALSE;	break;
			case 0x4:	return ((AxisFlags[1] & (1 << AXIS_Z)) && (MouseState.lZ < -1)) ? TRUE : FALSE;	break;
			case 0x5:	return ((AxisFlags[1] & (1 << AXIS_Z)) && (MouseState.lZ > +1)) ? TRUE : FALSE;	break;
			default:	return FALSE;
			}
		}
		else	return (MouseState.rgbButtons[Button & 0x7] & 0x80) ? TRUE : FALSE;
	}
	else
	{
		if ((Button & 0xE0) == 0x80)
		{	/* axis */
			switch (Button & 0xF)
			{
			case 0x0:	return ((AxisFlags[DevNum] & (1 << AXIS_X)) && (JoyState[DevNum].lX < 0x4000)) ? TRUE : FALSE;	break;
			case 0x1:	return ((AxisFlags[DevNum] & (1 << AXIS_X)) && (JoyState[DevNum].lX > 0xC000)) ? TRUE : FALSE;	break;
			case 0x2:	return ((AxisFlags[DevNum] & (1 << AXIS_Y)) && (JoyState[DevNum].lY < 0x4000)) ? TRUE : FALSE;	break;
			case 0x3:	return ((AxisFlags[DevNum] & (1 << AXIS_Y)) && (JoyState[DevNum].lY > 0xC000)) ? TRUE : FALSE;	break;
			case 0x4:	return ((AxisFlags[DevNum] & (1 << AXIS_Z)) && (JoyState[DevNum].lZ < 0x4000)) ? TRUE : FALSE;	break;
			case 0x5:	return ((AxisFlags[DevNum] & (1 << AXIS_Z)) && (JoyState[DevNum].lZ > 0xC000)) ? TRUE : FALSE;	break;
			case 0x6:	return ((AxisFlags[DevNum] & (1 << AXIS_RX)) && (JoyState[DevNum].lRx < 0x4000)) ? TRUE : FALSE;	break;
			case 0x7:	return ((AxisFlags[DevNum] & (1 << AXIS_RX)) && (JoyState[DevNum].lRx > 0xC000)) ? TRUE : FALSE;	break;
			case 0x8:	return ((AxisFlags[DevNum] & (1 << AXIS_RY)) && (JoyState[DevNum].lRy < 0x4000)) ? TRUE : FALSE;	break;
			case 0x9:	return ((AxisFlags[DevNum] & (1 << AXIS_RY)) && (JoyState[DevNum].lRy > 0xC000)) ? TRUE : FALSE;	break;
			case 0xA:	return ((AxisFlags[DevNum] & (1 << AXIS_RZ)) && (JoyState[DevNum].lRz < 0x4000)) ? TRUE : FALSE;	break;
			case 0xB:	return ((AxisFlags[DevNum] & (1 << AXIS_RZ)) && (JoyState[DevNum].lRz > 0xC000)) ? TRUE : FALSE;	break;
			case 0xC:	return ((AxisFlags[DevNum] & (1 << AXIS_S0)) && (JoyState[DevNum].rglSlider[0] < 0x4000)) ? TRUE : FALSE;	break;
			case 0xD:	return ((AxisFlags[DevNum] & (1 << AXIS_S0)) && (JoyState[DevNum].rglSlider[0] > 0xC000)) ? TRUE : FALSE;	break;
			case 0xE:	return ((AxisFlags[DevNum] & (1 << AXIS_S1)) && (JoyState[DevNum].rglSlider[1] < 0x4000)) ? TRUE : FALSE;	break;
			case 0xF:	return ((AxisFlags[DevNum] & (1 << AXIS_S1)) && (JoyState[DevNum].rglSlider[1] > 0xC000)) ? TRUE : FALSE;	break;
			default:	return FALSE;
			}
		}
		else if ((Button & 0xE0) == 0xC0)
		{	/* POV trigger (8-button mode) */
			if (POVAxis)
				return FALSE;
			switch (Button & 0x1F)
			{
			case 0x00:	return ((POVFlags[DevNum] & (1 << 0)) && ((JoyState[DevNum].rgdwPOV[0] > 33750) || (JoyState[DevNum].rgdwPOV[0] <  2250)) && (JoyState[DevNum].rgdwPOV[0] != -1)) ? TRUE : FALSE;	break;
			case 0x01:	return ((POVFlags[DevNum] & (1 << 0)) && ((JoyState[DevNum].rgdwPOV[0] >  2250) && (JoyState[DevNum].rgdwPOV[0] <  6750))) ? TRUE : FALSE;	break;
			case 0x02:	return ((POVFlags[DevNum] & (1 << 0)) && ((JoyState[DevNum].rgdwPOV[0] >  6750) && (JoyState[DevNum].rgdwPOV[0] < 11250))) ? TRUE : FALSE;	break;
			case 0x03:	return ((POVFlags[DevNum] & (1 << 0)) && ((JoyState[DevNum].rgdwPOV[0] > 11250) && (JoyState[DevNum].rgdwPOV[0] < 15750))) ? TRUE : FALSE;	break;
			case 0x04:	return ((POVFlags[DevNum] & (1 << 0)) && ((JoyState[DevNum].rgdwPOV[0] > 15750) && (JoyState[DevNum].rgdwPOV[0] < 20250))) ? TRUE : FALSE;	break;
			case 0x05:	return ((POVFlags[DevNum] & (1 << 0)) && ((JoyState[DevNum].rgdwPOV[0] > 20250) && (JoyState[DevNum].rgdwPOV[0] < 24750))) ? TRUE : FALSE;	break;
			case 0x06:	return ((POVFlags[DevNum] & (1 << 0)) && ((JoyState[DevNum].rgdwPOV[0] > 24750) && (JoyState[DevNum].rgdwPOV[0] < 29250))) ? TRUE : FALSE;	break;
			case 0x07:	return ((POVFlags[DevNum] & (1 << 0)) && ((JoyState[DevNum].rgdwPOV[0] > 29250) && (JoyState[DevNum].rgdwPOV[0] < 33750))) ? TRUE : FALSE;	break;
			case 0x08:	return ((POVFlags[DevNum] & (1 << 1)) && ((JoyState[DevNum].rgdwPOV[1] > 33750) || (JoyState[DevNum].rgdwPOV[1] <  2250)) && (JoyState[DevNum].rgdwPOV[1] != -1)) ? TRUE : FALSE;	break;
			case 0x09:	return ((POVFlags[DevNum] & (1 << 1)) && ((JoyState[DevNum].rgdwPOV[1] >  2250) && (JoyState[DevNum].rgdwPOV[1] <  6750))) ? TRUE : FALSE;	break;
			case 0x0A:	return ((POVFlags[DevNum] & (1 << 1)) && ((JoyState[DevNum].rgdwPOV[1] >  6750) && (JoyState[DevNum].rgdwPOV[1] < 11250))) ? TRUE : FALSE;	break;
			case 0x0B:	return ((POVFlags[DevNum] & (1 << 1)) && ((JoyState[DevNum].rgdwPOV[1] > 11250) && (JoyState[DevNum].rgdwPOV[1] < 15750))) ? TRUE : FALSE;	break;
			case 0x0C:	return ((POVFlags[DevNum] & (1 << 1)) && ((JoyState[DevNum].rgdwPOV[1] > 15750) && (JoyState[DevNum].rgdwPOV[1] < 20250))) ? TRUE : FALSE;	break;
			case 0x0D:	return ((POVFlags[DevNum] & (1 << 1)) && ((JoyState[DevNum].rgdwPOV[1] > 20250) && (JoyState[DevNum].rgdwPOV[1] < 24750))) ? TRUE : FALSE;	break;
			case 0x0E:	return ((POVFlags[DevNum] & (1 << 1)) && ((JoyState[DevNum].rgdwPOV[1] > 24750) && (JoyState[DevNum].rgdwPOV[1] < 29250))) ? TRUE : FALSE;	break;
			case 0x0F:	return ((POVFlags[DevNum] & (1 << 1)) && ((JoyState[DevNum].rgdwPOV[1] > 29250) && (JoyState[DevNum].rgdwPOV[1] < 33750))) ? TRUE : FALSE;	break;
			case 0x10:	return ((POVFlags[DevNum] & (1 << 2)) && ((JoyState[DevNum].rgdwPOV[2] > 33750) || (JoyState[DevNum].rgdwPOV[2] <  2250)) && (JoyState[DevNum].rgdwPOV[2] != -1)) ? TRUE : FALSE;	break;
			case 0x11:	return ((POVFlags[DevNum] & (1 << 2)) && ((JoyState[DevNum].rgdwPOV[2] >  2250) && (JoyState[DevNum].rgdwPOV[2] <  6750))) ? TRUE : FALSE;	break;
			case 0x12:	return ((POVFlags[DevNum] & (1 << 2)) && ((JoyState[DevNum].rgdwPOV[2] >  6750) && (JoyState[DevNum].rgdwPOV[2] < 11250))) ? TRUE : FALSE;	break;
			case 0x13:	return ((POVFlags[DevNum] & (1 << 2)) && ((JoyState[DevNum].rgdwPOV[2] > 11250) && (JoyState[DevNum].rgdwPOV[2] < 15750))) ? TRUE : FALSE;	break;
			case 0x14:	return ((POVFlags[DevNum] & (1 << 2)) && ((JoyState[DevNum].rgdwPOV[2] > 15750) && (JoyState[DevNum].rgdwPOV[2] < 20250))) ? TRUE : FALSE;	break;
			case 0x15:	return ((POVFlags[DevNum] & (1 << 2)) && ((JoyState[DevNum].rgdwPOV[2] > 20250) && (JoyState[DevNum].rgdwPOV[2] < 24750))) ? TRUE : FALSE;	break;
			case 0x16:	return ((POVFlags[DevNum] & (1 << 2)) && ((JoyState[DevNum].rgdwPOV[2] > 24750) && (JoyState[DevNum].rgdwPOV[2] < 29250))) ? TRUE : FALSE;	break;
			case 0x17:	return ((POVFlags[DevNum] & (1 << 2)) && ((JoyState[DevNum].rgdwPOV[2] > 29250) && (JoyState[DevNum].rgdwPOV[2] < 33750))) ? TRUE : FALSE;	break;
			case 0x18:	return ((POVFlags[DevNum] & (1 << 3)) && ((JoyState[DevNum].rgdwPOV[3] > 33750) || (JoyState[DevNum].rgdwPOV[3] <  2250)) && (JoyState[DevNum].rgdwPOV[3] != -1)) ? TRUE : FALSE;	break;
			case 0x19:	return ((POVFlags[DevNum] & (1 << 3)) && ((JoyState[DevNum].rgdwPOV[3] >  2250) && (JoyState[DevNum].rgdwPOV[3] <  6750))) ? TRUE : FALSE;	break;
			case 0x1A:	return ((POVFlags[DevNum] & (1 << 3)) && ((JoyState[DevNum].rgdwPOV[3] >  6750) && (JoyState[DevNum].rgdwPOV[3] < 11250))) ? TRUE : FALSE;	break;
			case 0x1B:	return ((POVFlags[DevNum] & (1 << 3)) && ((JoyState[DevNum].rgdwPOV[3] > 11250) && (JoyState[DevNum].rgdwPOV[3] < 15750))) ? TRUE : FALSE;	break;
			case 0x1C:	return ((POVFlags[DevNum] & (1 << 3)) && ((JoyState[DevNum].rgdwPOV[3] > 15750) && (JoyState[DevNum].rgdwPOV[3] < 20250))) ? TRUE : FALSE;	break;
			case 0x1D:	return ((POVFlags[DevNum] & (1 << 3)) && ((JoyState[DevNum].rgdwPOV[3] > 20250) && (JoyState[DevNum].rgdwPOV[3] < 24750))) ? TRUE : FALSE;	break;
			case 0x1E:	return ((POVFlags[DevNum] & (1 << 3)) && ((JoyState[DevNum].rgdwPOV[3] > 24750) && (JoyState[DevNum].rgdwPOV[3] < 29250))) ? TRUE : FALSE;	break;
			case 0x1F:	return ((POVFlags[DevNum] & (1 << 3)) && ((JoyState[DevNum].rgdwPOV[3] > 29250) && (JoyState[DevNum].rgdwPOV[3] < 33750))) ? TRUE : FALSE;	break;
			default:	return FALSE;
			}
		}
		else if ((Button & 0xE0) == 0xE0)
		{	/* POV trigger (axis mode) */
			switch (Button & 0x0F)
			{
			case 0x0:	return ((POVFlags[DevNum] & (1 << 0)) && ((JoyState[DevNum].rgdwPOV[0] > 29250) || (JoyState[DevNum].rgdwPOV[0] <  6750)) && (JoyState[DevNum].rgdwPOV[0] != -1)) ? TRUE : FALSE;	break;
			case 0x1:	return ((POVFlags[DevNum] & (1 << 0)) && ((JoyState[DevNum].rgdwPOV[0] >  2250) && (JoyState[DevNum].rgdwPOV[0] < 15750))) ? TRUE : FALSE;	break;
			case 0x2:	return ((POVFlags[DevNum] & (1 << 0)) && ((JoyState[DevNum].rgdwPOV[0] > 11250) && (JoyState[DevNum].rgdwPOV[0] < 24750))) ? TRUE : FALSE;	break;
			case 0x3:	return ((POVFlags[DevNum] & (1 << 0)) && ((JoyState[DevNum].rgdwPOV[0] > 20250) && (JoyState[DevNum].rgdwPOV[0] < 33750))) ? TRUE : FALSE;	break;
			case 0x4:	return ((POVFlags[DevNum] & (1 << 1)) && ((JoyState[DevNum].rgdwPOV[1] > 29250) || (JoyState[DevNum].rgdwPOV[1] <  6750)) && (JoyState[DevNum].rgdwPOV[1] != -1)) ? TRUE : FALSE;	break;
			case 0x5:	return ((POVFlags[DevNum] & (1 << 1)) && ((JoyState[DevNum].rgdwPOV[1] >  2250) && (JoyState[DevNum].rgdwPOV[1] < 15750))) ? TRUE : FALSE;	break;
			case 0x6:	return ((POVFlags[DevNum] & (1 << 1)) && ((JoyState[DevNum].rgdwPOV[1] > 11250) && (JoyState[DevNum].rgdwPOV[1] < 24750))) ? TRUE : FALSE;	break;
			case 0x7:	return ((POVFlags[DevNum] & (1 << 1)) && ((JoyState[DevNum].rgdwPOV[1] > 20250) && (JoyState[DevNum].rgdwPOV[1] < 33750))) ? TRUE : FALSE;	break;
			case 0x8:	return ((POVFlags[DevNum] & (1 << 2)) && ((JoyState[DevNum].rgdwPOV[2] > 29250) || (JoyState[DevNum].rgdwPOV[2] <  6750)) && (JoyState[DevNum].rgdwPOV[2] != -1)) ? TRUE : FALSE;	break;
			case 0x9:	return ((POVFlags[DevNum] & (1 << 2)) && ((JoyState[DevNum].rgdwPOV[2] >  2250) && (JoyState[DevNum].rgdwPOV[2] < 15750))) ? TRUE : FALSE;	break;
			case 0xA:	return ((POVFlags[DevNum] & (1 << 2)) && ((JoyState[DevNum].rgdwPOV[2] > 11250) && (JoyState[DevNum].rgdwPOV[2] < 24750))) ? TRUE : FALSE;	break;
			case 0xB:	return ((POVFlags[DevNum] & (1 << 2)) && ((JoyState[DevNum].rgdwPOV[2] > 20250) && (JoyState[DevNum].rgdwPOV[2] < 33750))) ? TRUE : FALSE;	break;
			case 0xC:	return ((POVFlags[DevNum] & (1 << 3)) && ((JoyState[DevNum].rgdwPOV[3] > 29250) || (JoyState[DevNum].rgdwPOV[3] <  6750)) && (JoyState[DevNum].rgdwPOV[3] != -1)) ? TRUE : FALSE;	break;
			case 0xD:	return ((POVFlags[DevNum] & (1 << 3)) && ((JoyState[DevNum].rgdwPOV[3] >  2250) && (JoyState[DevNum].rgdwPOV[3] < 15750))) ? TRUE : FALSE;	break;
			case 0xE:	return ((POVFlags[DevNum] & (1 << 3)) && ((JoyState[DevNum].rgdwPOV[3] > 11250) && (JoyState[DevNum].rgdwPOV[3] < 24750))) ? TRUE : FALSE;	break;
			case 0xF:	return ((POVFlags[DevNum] & (1 << 3)) && ((JoyState[DevNum].rgdwPOV[3] > 20250) && (JoyState[DevNum].rgdwPOV[3] < 33750))) ? TRUE : FALSE;	break;
			default:	return FALSE;
			}
		}
		else	return (JoyState[DevNum].rgbButtons[Button & 0x7F] & 0x80) ? TRUE : FALSE;
	}
}
void	ParseConfigMessages (HWND hDlg, int numItems, int *dlgDevices, int *dlgButtons, int *Buttons, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			for (j = 0; j < NumDevices; j++)
				SendDlgItemMessage(hDlg,dlgDevices[i],CB_ADDSTRING,0,(LPARAM)DeviceName[j]);	/* add each device */
			SendDlgItemMessage(hDlg,dlgDevices[i],CB_SETCURSEL,Buttons[i] >> 16,0);	/* select the one we want */
			ConfigButton(&Buttons[i],Buttons[i] >> 16,GetDlgItem(hDlg,dlgButtons[i]),FALSE);	/* and label the corresponding button */
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
			ConfigButton(&Buttons[i],(int)SendMessage((HWND)lParam,CB_GETCURSEL,0,0),GetDlgItem(hDlg,dlgButtons[i]),FALSE);
		}
		if (wmId == dlgButtons[i])
			ConfigButton(&Buttons[i],Buttons[i] >> 16,GetDlgItem(hDlg,dlgButtons[i]),TRUE);
	}
}
} // namespace Controllers