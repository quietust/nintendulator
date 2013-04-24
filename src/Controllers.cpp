/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
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

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace Controllers
{
HWND key;

StdPort *Port1, *Port2;
StdPort *FSPort1, *FSPort2, *FSPort3, *FSPort4;
ExpPort *PortExp;

int	Port1_Buttons[CONTROLLERS_MAXBUTTONS],
	Port2_Buttons[CONTROLLERS_MAXBUTTONS];
int	FSPort1_Buttons[CONTROLLERS_MAXBUTTONS],
	FSPort2_Buttons[CONTROLLERS_MAXBUTTONS],
	FSPort3_Buttons[CONTROLLERS_MAXBUTTONS],
	FSPort4_Buttons[CONTROLLERS_MAXBUTTONS];
int	PortExp_Buttons[CONTROLLERS_MAXBUTTONS];

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

LPDIRECTINPUT8		DirectInput;
LPDIRECTINPUTDEVICE8	DIDevices[MAX_CONTROLLERS];

BYTE		KeyState[256];
DIMOUSESTATE2	MouseState;
DIJOYSTATE2	JoyState[MAX_CONTROLLERS];	// first 2 entries are unused

void	StdPort_SetControllerType (StdPort *&Port, STDCONT_TYPE Type, int *buttons)
{
	if (Port != NULL)
	{
		delete Port;
		Port = NULL;
	}
	switch (Type)
	{
	case STD_UNCONNECTED:		Port = new StdPort_Unconnected(buttons);	break;
	case STD_STDCONTROLLER:		Port = new StdPort_StdController(buttons);	break;
	case STD_ZAPPER:		Port = new StdPort_Zapper(buttons);		break;
	case STD_ARKANOIDPADDLE:	Port = new StdPort_ArkanoidPaddle(buttons);	break;
	case STD_POWERPAD:		Port = new StdPort_PowerPad(buttons);		break;
	case STD_FOURSCORE:		Port = new StdPort_FourScore(buttons);		break;
	case STD_SNESCONTROLLER:	Port = new StdPort_SnesController(buttons);	break;
	case STD_VSZAPPER:		Port = new StdPort_VSZapper(buttons);		break;
	case STD_SNESMOUSE:		Port = new StdPort_SnesMouse(buttons);		break;
	case STD_FOURSCORE2:		Port = new StdPort_FourScore2(buttons);		break;
	default:MessageBox(hMainWnd, _T("Error: selected invalid controller type for standard port!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);	break;
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
	StdPort_Mappings[STD_FOURSCORE] = _T("Four Score (port 1 only)");
	StdPort_Mappings[STD_SNESCONTROLLER] = _T("SNES Controller");
	StdPort_Mappings[STD_VSZAPPER] = _T("VS Unisystem Zapper");
	StdPort_Mappings[STD_SNESMOUSE] = _T("SNES Mouse");
	StdPort_Mappings[STD_FOURSCORE2] = _T("Four Score (port 2 only)");
}

void    ExpPort_SetControllerType (ExpPort *&Port, EXPCONT_TYPE Type, int *buttons)
{
	if (Port != NULL)
	{
		delete Port;
		Port = NULL;
	}
        switch (Type)
        {
        case EXP_UNCONNECTED:		Port = new ExpPort_Unconnected(buttons);		break;
        case EXP_FAMI4PLAY:		Port = new ExpPort_Fami4Play(buttons);			break;
        case EXP_ARKANOIDPADDLE:	Port = new ExpPort_ArkanoidPaddle(buttons);		break;
        case EXP_FAMILYBASICKEYBOARD:	Port = new ExpPort_FamilyBasicKeyboard(buttons);	break;
        case EXP_SUBORKEYBOARD:		Port = new ExpPort_SuborKeyboard(buttons);		break;
        case EXP_FAMTRAINER:		Port = new ExpPort_FamTrainer(buttons);			break;
        case EXP_TABLET:		Port = new ExpPort_Tablet(buttons);			break;
        default:MessageBox(hMainWnd, _T("Error: selected invalid controller type for expansion port!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);	break;
        }
}
const TCHAR	*ExpPort_Mappings[EXP_MAX];
void	ExpPort_SetMappings (void)
{
	ExpPort_Mappings[EXP_UNCONNECTED] = _T("Unconnected");
	ExpPort_Mappings[EXP_FAMI4PLAY] = _T("Famicom 4-Player Adapter");
	ExpPort_Mappings[EXP_ARKANOIDPADDLE] = _T("Famicom Arkanoid Paddle");
	ExpPort_Mappings[EXP_FAMILYBASICKEYBOARD] = _T("Family Basic Keyboard");
	ExpPort_Mappings[EXP_SUBORKEYBOARD] = _T("Subor Keyboard");
	ExpPort_Mappings[EXP_FAMTRAINER] = _T("Family Trainer");
	ExpPort_Mappings[EXP_TABLET] = _T("Oeka Kids Tablet");
}

BOOL	POVAxis = FALSE;

INT_PTR	CALLBACK	ControllerProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	int i;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg, IDC_CONT_SPORT1, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hDlg, IDC_CONT_SPORT2, CB_RESETCONTENT, 0, 0);
		for (i = 0; i < STD_MAX; i++)
		{
			SendDlgItemMessage(hDlg, IDC_CONT_SPORT1, CB_ADDSTRING, 0, (LPARAM)StdPort_Mappings[i]);
			SendDlgItemMessage(hDlg, IDC_CONT_SPORT2, CB_ADDSTRING, 0, (LPARAM)StdPort_Mappings[i]);
		}
		SendDlgItemMessage(hDlg, IDC_CONT_SPORT1, CB_SETCURSEL, Port1->Type, 0);
		SendDlgItemMessage(hDlg, IDC_CONT_SPORT2, CB_SETCURSEL, Port2->Type, 0);

		SendDlgItemMessage(hDlg, IDC_CONT_SEXPPORT, CB_RESETCONTENT, 0, 0);
		for (i = 0; i < EXP_MAX; i++)
			SendDlgItemMessage(hDlg, IDC_CONT_SEXPPORT, CB_ADDSTRING, 0, (LPARAM)ExpPort_Mappings[i]);
		SendDlgItemMessage(hDlg, IDC_CONT_SEXPPORT, CB_SETCURSEL, PortExp->Type, 0);
		if (Movie::Mode)
		{
			EnableWindow(GetDlgItem(hDlg, IDC_CONT_SPORT1), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_CONT_SPORT2), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_CONT_SEXPPORT), FALSE);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg, IDC_CONT_SPORT1), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_CONT_SPORT2), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_CONT_SEXPPORT), TRUE);
		}

		if (EnableOpposites)
			CheckDlgButton(hDlg, IDC_CONT_UDLR, BST_CHECKED);
		else	CheckDlgButton(hDlg, IDC_CONT_UDLR, BST_UNCHECKED);

		return TRUE;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		switch (wmId)
		{
		case IDC_CONT_POV:
			POVAxis = (IsDlgButtonChecked(hDlg, IDC_CONT_POV) == BST_CHECKED);
			return TRUE;
		case IDOK:
			EnableOpposites = (IsDlgButtonChecked(hDlg, IDC_CONT_UDLR) == BST_CHECKED);
			POVAxis = FALSE;
			EndDialog(hDlg, 1);
			return TRUE;
		case IDC_CONT_SPORT1:
			if (wmEvent == CBN_SELCHANGE)
			{
				STDCONT_TYPE Type = (STDCONT_TYPE)SendDlgItemMessage(hDlg, IDC_CONT_SPORT1, CB_GETCURSEL, 0, 0);
				if (Type == STD_FOURSCORE2)
				{
					// undo selection - can NOT set port 1 to this!
					SendDlgItemMessage(hDlg, IDC_CONT_SPORT1, CB_SETCURSEL, Port1->Type, 0);
				}
				else if (Type == STD_FOURSCORE)
				{
					if (Port1->Type == STD_STDCONTROLLER)
						SET_STDCONT(FSPort1, STD_STDCONTROLLER);
					else	SET_STDCONT(FSPort1, STD_UNCONNECTED);
					if (Port2->Type == STD_STDCONTROLLER)
						SET_STDCONT(FSPort2, STD_STDCONTROLLER);
					else	SET_STDCONT(FSPort2, STD_UNCONNECTED);
					memcpy(FSPort1_Buttons, Port1_Buttons, sizeof(Port1_Buttons));
					memcpy(FSPort2_Buttons, Port2_Buttons, sizeof(Port2_Buttons));
					SET_STDCONT(Port1, STD_FOURSCORE);
					SET_STDCONT(Port2, STD_FOURSCORE2);
					SendDlgItemMessage(hDlg, IDC_CONT_SPORT2, CB_SETCURSEL, STD_FOURSCORE2, 0);
				}
				else if (Port1->Type == STD_FOURSCORE)
				{
					SET_STDCONT(Port1, Type);
					SET_STDCONT(Port2, FSPort2->Type);
					memcpy(Port2_Buttons, FSPort2_Buttons, sizeof(FSPort2_Buttons));
					SendDlgItemMessage(hDlg, IDC_CONT_SPORT2, CB_SETCURSEL, Port2->Type, 0);
				}
				else	SET_STDCONT(Port1, Type);
			}
			return TRUE;
		case IDC_CONT_SPORT2:
			if (wmEvent == CBN_SELCHANGE)
			{
				STDCONT_TYPE Type = (STDCONT_TYPE)SendDlgItemMessage(hDlg, IDC_CONT_SPORT2, CB_GETCURSEL, 0, 0);
				if (Type == STD_FOURSCORE)
				{
					// undo selection - can NOT set port 2 to this!
					SendDlgItemMessage(hDlg, IDC_CONT_SPORT2, CB_SETCURSEL, Port2->Type, 0);
				}
				else if (Type == STD_FOURSCORE2)
				{
					if (Port1->Type == STD_STDCONTROLLER)
						SET_STDCONT(FSPort1, STD_STDCONTROLLER);
					else	SET_STDCONT(FSPort1, STD_UNCONNECTED);
					if (Port2->Type == STD_STDCONTROLLER)
						SET_STDCONT(FSPort2, STD_STDCONTROLLER);
					else	SET_STDCONT(FSPort2, STD_UNCONNECTED);
					memcpy(FSPort1_Buttons, Port1_Buttons, sizeof(Port1_Buttons));
					memcpy(FSPort2_Buttons, Port2_Buttons, sizeof(Port2_Buttons));
					SET_STDCONT(Port1, STD_FOURSCORE);
					SET_STDCONT(Port2, STD_FOURSCORE2);
					SendDlgItemMessage(hDlg, IDC_CONT_SPORT1, CB_SETCURSEL, STD_FOURSCORE, 0);
				}
				else if (Port2->Type == STD_FOURSCORE2)
				{
					SET_STDCONT(Port1, FSPort1->Type);
					SET_STDCONT(Port2, Type);
					memcpy(Port1_Buttons, FSPort1_Buttons, sizeof(FSPort1_Buttons));
					SendDlgItemMessage(hDlg, IDC_CONT_SPORT1, CB_SETCURSEL, Port1->Type, 0);
				}
				else	SET_STDCONT(Port2, Type);
			}
			return TRUE;
		case IDC_CONT_SEXPPORT:
			if (wmEvent == CBN_SELCHANGE)
			{
				EXPCONT_TYPE Type = (EXPCONT_TYPE)SendDlgItemMessage(hDlg, IDC_CONT_SEXPPORT, CB_GETCURSEL, 0, 0);
				SET_EXPCONT(PortExp, Type);
			}
			return TRUE;
		case IDC_CONT_CPORT1:
			Port1->Config(hDlg);
			return TRUE;
		case IDC_CONT_CPORT2:
			Port2->Config(hDlg);
			return TRUE;
		case IDC_CONT_CEXPPORT:
			PortExp->Config(hDlg);
			return TRUE;
		}
		break;
	}
	return FALSE;
}
void	OpenConfig (void)
{
	DialogBox(hInst, (LPCTSTR)IDD_CONTROLLERS, hMainWnd, ControllerProc);
	SetDeviceUsed();
}

BOOL CALLBACK	EnumKeyboardObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	int ItemNum = 0;
	if (IsEqualGUID(lpddoi->guidType, GUID_Key))
	{
		ItemNum = lpddoi->dwOfs;
		if ((ItemNum >= 0) && (ItemNum < 256))
			KeyNames[ItemNum] = _tcsdup(lpddoi->tszName);
		else	MessageBox(hMainWnd, _T("Error - encountered invalid keyboard key ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}
	return DIENUM_CONTINUE;
}

BOOL CALLBACK	EnumMouseObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	int ItemNum = 0;
	if (IsEqualGUID(lpddoi->guidType, GUID_XAxis))
	{
		AxisFlags[1] |= 0x01;
		AxisNames[1][AXIS_X] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_YAxis))
	{
		AxisFlags[1] |= 0x02;
		AxisNames[1][AXIS_Y] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_ZAxis))
	{
		AxisFlags[1] |= 0x04;
		AxisNames[1][AXIS_Z] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_Button))
	{
		ItemNum = (lpddoi->dwOfs - ((BYTE *)&MouseState.rgbButtons - (BYTE *)&MouseState)) / sizeof(MouseState.rgbButtons[0]);
		if ((ItemNum >= 0) && (ItemNum < 8))
			ButtonNames[1][ItemNum] = _tcsdup(lpddoi->tszName);
		else	MessageBox(hMainWnd, _T("Error - encountered invalid mouse button ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}

	return DIENUM_CONTINUE;
}

BOOL CALLBACK	EnumJoystickObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	int DevNum = (int)pvRef;
	int ItemNum = 0;
	if (IsEqualGUID(lpddoi->guidType, GUID_XAxis))
	{
		AxisFlags[DevNum] |= 0x01;
		AxisNames[DevNum][AXIS_X] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_YAxis))
	{
		AxisFlags[DevNum] |= 0x02;
		AxisNames[DevNum][AXIS_Y] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_ZAxis))
	{
		AxisFlags[DevNum] |= 0x04;
		AxisNames[DevNum][AXIS_Z] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_RxAxis))
	{
		AxisFlags[DevNum] |= 0x08;
		AxisNames[DevNum][AXIS_RX] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_RyAxis))
	{
		AxisFlags[DevNum] |= 0x10;
		AxisNames[DevNum][AXIS_RY] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_RzAxis))
	{
		AxisFlags[DevNum] |= 0x20;
		AxisNames[DevNum][AXIS_RZ] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_Slider))
	{
		// ItemNum = DIDFT_GETINSTANCE(lpddoi->dwType);
		// Sliders are enumerated as axes, and this index
		// starts where the other axes left off
		// Thus, we need to ignore it and assign them incrementally
		// Hopefully, this actually works reliably
		if (!(AxisFlags[DevNum] & 0x40))
			ItemNum = 0;
		else if (!(AxisFlags[DevNum] & 0x80))
			ItemNum = 1;
		else	ItemNum = 2;
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
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_POV))
	{
		ItemNum = DIDFT_GETINSTANCE(lpddoi->dwType);
		if ((ItemNum >= 0) && (ItemNum < 4))
		{
			POVFlags[DevNum] |= 0x01 << ItemNum;
			POVNames[DevNum][ItemNum] = _tcsdup(lpddoi->tszName);
		}
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_Button))
	{
		ItemNum = DIDFT_GETINSTANCE(lpddoi->dwType);
		if ((ItemNum >= 0) && (ItemNum < 128))
			ButtonNames[DevNum][ItemNum] = _tcsdup(lpddoi->tszName);
	}
	return DIENUM_CONTINUE;
}

BOOL CALLBACK	EnumJoysticksCallback (LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	HRESULT hr;
	int DevNum = NumDevices;
	do
	{
		if (SUCCEEDED(DirectInput->CreateDevice(lpddi->guidInstance, &DIDevices[DevNum], NULL)))
		{
			DIDEVCAPS caps;
			if (FAILED(hr = DIDevices[DevNum]->SetDataFormat(&c_dfDIJoystick2)))
				break;
			if (FAILED(hr = DIDevices[DevNum]->SetCooperativeLevel(hMainWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
				break;
			caps.dwSize = sizeof(DIDEVCAPS);
			if (FAILED(hr = DIDevices[DevNum]->GetCapabilities(&caps)))
				break;

			NumButtons[DevNum] = caps.dwButtons;
			AxisFlags[DevNum] = 0;
			POVFlags[DevNum] = 0;
			DeviceName[DevNum] = _tcsdup(lpddi->tszProductName);

			DIDevices[DevNum]->EnumObjects(EnumJoystickObjectsCallback, (LPVOID)DevNum, DIDFT_ALL);
			EI.DbgOut(_T("Added input device '%s' with %i buttons, %i axes, %i POVs"), lpddi->tszProductName, caps.dwButtons, caps.dwAxes, caps.dwPOVs);
			NumDevices++;
		}
		return DIENUM_CONTINUE;
	} while (0);

	DIDevices[DevNum]->Release();
	DIDevices[DevNum] = NULL;
	return hr;
}

BOOL	InitKeyboard (void)
{
	DIDEVICEINSTANCE inst;
	DIDEVCAPS caps;
	do
	{
		if (FAILED(DirectInput->CreateDevice(GUID_SysKeyboard, &DIDevices[0], NULL)))
			return FALSE;
		if (FAILED(DIDevices[0]->SetDataFormat(&c_dfDIKeyboard)))
			break;
		if (FAILED(DIDevices[0]->SetCooperativeLevel(hMainWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
			break;

		caps.dwSize = sizeof(DIDEVCAPS);
		if (FAILED(DIDevices[0]->GetCapabilities(&caps)))
			break;

		inst.dwSize = sizeof(DIDEVICEINSTANCE);
		if (FAILED(DIDevices[0]->GetDeviceInfo(&inst)))
			break;

		NumButtons[0] = 256;	// normally, I would use caps.dwButtons
		AxisFlags[0] = 0;	// no axes
		POVFlags[0] = 0;	// no POV hats
		DeviceName[0] = _tcsdup(inst.tszProductName);

		DIDevices[0]->EnumObjects(EnumKeyboardObjectsCallback, NULL, DIDFT_ALL);
		EI.DbgOut(_T("Added input device '%s' with %i buttons, %i axes, %i POVs"), inst.tszProductName, caps.dwButtons, caps.dwAxes, caps.dwPOVs);
		return TRUE;
	} while (0);

	DIDevices[0]->Release();
	DIDevices[0] = NULL;
	return FALSE;
}

BOOL	InitMouse (void)
{
	DIDEVICEINSTANCE inst;
	DIDEVCAPS caps;
	do
	{
		if (FAILED(DirectInput->CreateDevice(GUID_SysMouse, &DIDevices[1], NULL)))
			return FALSE;
		if (FAILED(DIDevices[1]->SetDataFormat(&c_dfDIMouse2)))
			break;
		if (FAILED(DIDevices[1]->SetCooperativeLevel(hMainWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
			break;
		caps.dwSize = sizeof(DIDEVCAPS);
		if (FAILED(DIDevices[1]->GetCapabilities(&caps)))
			break;

		inst.dwSize = sizeof(DIDEVICEINSTANCE);
		if (FAILED(DIDevices[1]->GetDeviceInfo(&inst)))
			break;

		NumButtons[1] = caps.dwButtons;
		AxisFlags[1] = 0;
		POVFlags[1] = 0;
		DeviceName[1] = _tcsdup(inst.tszProductName);

		DIDevices[1]->EnumObjects(EnumMouseObjectsCallback, NULL, DIDFT_ALL);
		EI.DbgOut(_T("Added input device '%s' with %i buttons, %i axes, %i POVs"), inst.tszProductName, caps.dwButtons, caps.dwAxes, caps.dwPOVs);
		return TRUE;
	} while (0);

	DIDevices[1]->Release();
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
		ZeroMemory(AxisNames[i], sizeof(AxisNames[i]));
		ZeroMemory(ButtonNames[i], sizeof(ButtonNames[i]));
		ZeroMemory(POVNames[i], sizeof(POVNames[i]));
	}
	ZeroMemory(KeyNames, sizeof(KeyNames));

	StdPort_SetMappings();
	ExpPort_SetMappings();

	ZeroMemory(Port1_Buttons, sizeof(Port1_Buttons));
	ZeroMemory(Port2_Buttons, sizeof(Port2_Buttons));
	ZeroMemory(FSPort1_Buttons, sizeof(FSPort1_Buttons));
	ZeroMemory(FSPort2_Buttons, sizeof(FSPort2_Buttons));
	ZeroMemory(FSPort3_Buttons, sizeof(FSPort3_Buttons));
	ZeroMemory(FSPort4_Buttons, sizeof(FSPort4_Buttons));
	ZeroMemory(PortExp_Buttons, sizeof(PortExp_Buttons));

	Port1 = new StdPort_Unconnected(Port1_Buttons);
	Port2 = new StdPort_Unconnected(Port2_Buttons);
	FSPort1 = new StdPort_Unconnected(FSPort1_Buttons);
	FSPort2 = new StdPort_Unconnected(FSPort2_Buttons);
	FSPort3 = new StdPort_Unconnected(FSPort3_Buttons);
	FSPort4 = new StdPort_Unconnected(FSPort4_Buttons);
	PortExp = new ExpPort_Unconnected(PortExp_Buttons);
	
	if (FAILED(DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID *)&DirectInput, NULL)))
	{
		MessageBox(hMainWnd, _T("Unable to initialize DirectInput!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		return;
	}

	if (!InitKeyboard())
		MessageBox(hMainWnd, _T("Failed to initialize keyboard!"), _T("Nintendulator"), MB_OK | MB_ICONWARNING);
	if (!InitMouse())
		MessageBox(hMainWnd, _T("Failed to initialize mouse!"), _T("Nintendulator"), MB_OK | MB_ICONWARNING);

	NumDevices = 2;	// joysticks start at slot 2
	if (FAILED(DirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, NULL, DIEDFL_ALLDEVICES)))
		MessageBox(hMainWnd, _T("Failed to initialize joysticks!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);

	Movie::Mode = 0;
}

void	Destroy (void)
{
	int i, j;
	delete Port1;	Port1 = NULL;
	delete Port2;	Port2 = NULL;
	delete FSPort1;	FSPort1 = NULL;
	delete FSPort2;	FSPort2 = NULL;
	delete FSPort3;	FSPort3 = NULL;
	delete FSPort4;	FSPort4 = NULL;
	delete PortExp;	PortExp = NULL;
	for (i = 0; i < NumDevices; i++)
	{
		if (DIDevices[i])
		{
			DIDevices[i]->Release();
			DIDevices[i] = NULL;
		}
		// Allocated using _tcsdup()
		free(DeviceName[i]);
		DeviceName[i] = NULL;
		for (j = 0; j < 128; j++)
		{
			// Allocated using _tcsdup()
			free(ButtonNames[i][j]);
			ButtonNames[i][j] = NULL;
		}
		for (j = 0; j < 8; j++)
		{
			// Allocated using _tcsdup()
			free(AxisNames[i][j]);
			AxisNames[i][j] = NULL;
		}
		for (j = 0; j < 4; j++)
		{
			// Allocated using _tcsdup()
			free(POVNames[i][j]);
			POVNames[i][j] = NULL;
		}
	}
	for (j = 0; j < 256; j++)
	{
		// Allocated using _tcsdup()
		free(KeyNames[j]);
		KeyNames[j] = NULL;
	}

	DirectInput->Release();
	DirectInput = NULL;
}

void	Write (unsigned char Val)
{
	Port1->Write(Val & 0x1);
	Port2->Write(Val & 0x1);
	PortExp->Write(Val);
}

int	Save (FILE *out)
{
	int clen = 0;
	unsigned char type;

	type = (unsigned char)Port1->Type;	writeByte(type);
	clen += Port1->Save(out);

	type = (unsigned char)Port2->Type;	writeByte(type);
	clen += Port2->Save(out);

	type = (unsigned char)PortExp->Type;	writeByte(type);
	clen += PortExp->Save(out);

	type = (unsigned char)FSPort1->Type;	writeByte(type);
	clen += FSPort1->Save(out);

	type = (unsigned char)FSPort2->Type;	writeByte(type);
	clen += FSPort2->Save(out);

	type = (unsigned char)FSPort3->Type;	writeByte(type);
	clen += FSPort3->Save(out);

	type = (unsigned char)FSPort4->Type;	writeByte(type);
	clen += FSPort4->Save(out);

	return clen;
}

int	Load (FILE *in, int version_id)
{
	int clen = 0;
	unsigned char type;
	Movie::ControllerTypes[3] = 1;	// denotes that controller state has been loaded
					// if we're playing a movie, this means we should
					// SKIP the controller info in the movie block

	readByte(type);
	SET_STDCONT(Port1, (STDCONT_TYPE)type);
	clen += Port1->Load(in, version_id);

	readByte(type);
	// if there's a Four Score on the first port, then assume the same for port 2
	if (Port1->Type == STD_FOURSCORE)
		SET_STDCONT(Port2, STD_FOURSCORE2);
	// If not, however, make sure that the right half of a four-score doesn't end up here
	else if ((STDCONT_TYPE)type == STD_FOURSCORE2)
		SET_STDCONT(Port2, STD_UNCONNECTED);
	else	SET_STDCONT(Port2, (STDCONT_TYPE)type);
	clen += Port2->Load(in, version_id);

	readByte(type);
	SET_EXPCONT(PortExp, (EXPCONT_TYPE)type);
	clen += PortExp->Load(in, version_id);

	readByte(type);
	SET_STDCONT(FSPort1, (STDCONT_TYPE)type);
	clen += FSPort1->Load(in, version_id);

	readByte(type);
	SET_STDCONT(FSPort2, (STDCONT_TYPE)type);
	clen += FSPort2->Load(in, version_id);

	readByte(type);
	SET_STDCONT(FSPort3, (STDCONT_TYPE)type);
	clen += FSPort3->Load(in, version_id);

	readByte(type);
	SET_STDCONT(FSPort4, (STDCONT_TYPE)type);
	clen += FSPort4->Load(in, version_id);

	return clen;
}

void	SaveSettings (HKEY SettingsBase)
{
	RegSetValueEx(SettingsBase, _T("UDLR")    , 0, REG_DWORD, (LPBYTE)&EnableOpposites, sizeof(BOOL));

	RegSetValueEx(SettingsBase, _T("Port1T")  , 0, REG_DWORD, (LPBYTE)&Port1->Type  , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("Port2T")  , 0, REG_DWORD, (LPBYTE)&Port2->Type  , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort1T"), 0, REG_DWORD, (LPBYTE)&FSPort1->Type, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort2T"), 0, REG_DWORD, (LPBYTE)&FSPort2->Type, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort3T"), 0, REG_DWORD, (LPBYTE)&FSPort3->Type, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort4T"), 0, REG_DWORD, (LPBYTE)&FSPort4->Type, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("ExpPortT"), 0, REG_DWORD, (LPBYTE)&PortExp->Type, sizeof(DWORD));

	RegSetValueEx(SettingsBase, _T("Port1D")  , 0, REG_BINARY, (LPBYTE)Port1_Buttons  , sizeof(Port1_Buttons));
	RegSetValueEx(SettingsBase, _T("Port2D")  , 0, REG_BINARY, (LPBYTE)Port2_Buttons  , sizeof(Port2_Buttons));
	RegSetValueEx(SettingsBase, _T("FSPort1D"), 0, REG_BINARY, (LPBYTE)FSPort1_Buttons, sizeof(FSPort1_Buttons));
	RegSetValueEx(SettingsBase, _T("FSPort2D"), 0, REG_BINARY, (LPBYTE)FSPort2_Buttons, sizeof(FSPort2_Buttons));
	RegSetValueEx(SettingsBase, _T("FSPort3D"), 0, REG_BINARY, (LPBYTE)FSPort3_Buttons, sizeof(FSPort3_Buttons));
	RegSetValueEx(SettingsBase, _T("FSPort4D"), 0, REG_BINARY, (LPBYTE)FSPort4_Buttons, sizeof(FSPort4_Buttons));
	RegSetValueEx(SettingsBase, _T("ExpPortD"), 0, REG_BINARY, (LPBYTE)PortExp_Buttons, sizeof(PortExp_Buttons));
}

void	LoadSettings (HKEY SettingsBase)
{
	unsigned long Size;
	int Port1T = 0, Port2T = 0, FSPort1T = 0, FSPort2T = 0, FSPort3T = 0, FSPort4T = 0, ExpPortT = 0;
	Size = sizeof(BOOL);	RegQueryValueEx(SettingsBase, _T("UDLR")    , 0, NULL, (LPBYTE)&EnableOpposites, &Size);

	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("Port1T")  , 0, NULL, (LPBYTE)&Port1T  , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("Port2T")  , 0, NULL, (LPBYTE)&Port2T  , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSPort1T"), 0, NULL, (LPBYTE)&FSPort1T, &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSPort2T"), 0, NULL, (LPBYTE)&FSPort2T, &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSPort3T"), 0, NULL, (LPBYTE)&FSPort3T, &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSPort4T"), 0, NULL, (LPBYTE)&FSPort4T, &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("ExpPortT"), 0, NULL, (LPBYTE)&ExpPortT, &Size);

	if (Port1T == STD_FOURSCORE)
	{
		SET_STDCONT(Port1, STD_FOURSCORE);
		SET_STDCONT(Port1, STD_FOURSCORE2);
	}
	else
	{
		SET_STDCONT(Port1, (STDCONT_TYPE)Port1T);
		if ((Port2T == STD_FOURSCORE) || (Port2T == STD_FOURSCORE2))
			Port2T = STD_UNCONNECTED;
		SET_STDCONT(Port2, (STDCONT_TYPE)Port2T);
	}
	SET_STDCONT(FSPort1, (STDCONT_TYPE)FSPort1T);
	SET_STDCONT(FSPort2, (STDCONT_TYPE)FSPort2T);
	SET_STDCONT(FSPort3, (STDCONT_TYPE)FSPort3T);
	SET_STDCONT(FSPort4, (STDCONT_TYPE)FSPort4T);
	SET_EXPCONT(PortExp, (EXPCONT_TYPE)ExpPortT);

	Size = sizeof(Port1_Buttons);	RegQueryValueEx(SettingsBase, _T("Port1D")  , 0, NULL, (LPBYTE)Port1_Buttons  , &Size);
	Size = sizeof(Port2_Buttons);	RegQueryValueEx(SettingsBase, _T("Port2D")  , 0, NULL, (LPBYTE)Port2_Buttons  , &Size);
	Size = sizeof(FSPort1_Buttons);	RegQueryValueEx(SettingsBase, _T("FSPort1D"), 0, NULL, (LPBYTE)FSPort1_Buttons, &Size);
	Size = sizeof(FSPort2_Buttons);	RegQueryValueEx(SettingsBase, _T("FSPort2D"), 0, NULL, (LPBYTE)FSPort2_Buttons, &Size);
	Size = sizeof(FSPort3_Buttons);	RegQueryValueEx(SettingsBase, _T("FSPort3D"), 0, NULL, (LPBYTE)FSPort3_Buttons, &Size);
	Size = sizeof(FSPort4_Buttons);	RegQueryValueEx(SettingsBase, _T("FSPort4D"), 0, NULL, (LPBYTE)FSPort4_Buttons, &Size);
	Size = sizeof(PortExp_Buttons);	RegQueryValueEx(SettingsBase, _T("ExpPortD"), 0, NULL, (LPBYTE)PortExp_Buttons, &Size);
	SetDeviceUsed();
}

void	SetDeviceUsed (void)
{
	int i;

	for (i = 0; i < NumDevices; i++)
		DeviceUsed[i] = FALSE;

	for (i = 0; i < CONTROLLERS_MAXBUTTONS; i++)
	{
		if (Port1->Type == STD_FOURSCORE)
		{
			if (i < FSPort1->NumButtons)
				DeviceUsed[FSPort1->Buttons[i] >> 16] = TRUE;
			if (i < FSPort2->NumButtons)
				DeviceUsed[FSPort2->Buttons[i] >> 16] = TRUE;
			if (i < FSPort3->NumButtons)
				DeviceUsed[FSPort3->Buttons[i] >> 16] = TRUE;
			if (i < FSPort4->NumButtons)
				DeviceUsed[FSPort4->Buttons[i] >> 16] = TRUE;
		}
		else
		{
			if (i < Port1->NumButtons)
				DeviceUsed[Port1->Buttons[i] >> 16] = TRUE;
			if (i < Port2->NumButtons)
				DeviceUsed[Port2->Buttons[i] >> 16] = TRUE;
		}
		if (i < PortExp->NumButtons)
			DeviceUsed[PortExp->Buttons[i] >> 16] = TRUE;
	}
	if ((Port1->Type == STD_ARKANOIDPADDLE) || (Port2->Type == STD_ARKANOIDPADDLE) || (PortExp->Type == EXP_ARKANOIDPADDLE))
		DeviceUsed[1] = TRUE;

	if ((PortExp->Type == EXP_FAMILYBASICKEYBOARD) || (PortExp->Type == EXP_SUBORKEYBOARD))
		DeviceUsed[0] = TRUE;
}

void	Acquire (void)
{
	int i;
	for (i = 0; i < NumDevices; i++)
		if (DeviceUsed[i])
			DIDevices[i]->Acquire();
	Port1->SetMasks();
	Port2->SetMasks();
	PortExp->SetMasks();

	if (MaskMouse)
	{
		RECT rect;
		POINT point = {0, 0};

		GetClientRect(hMainWnd, &rect);
		ClientToScreen(hMainWnd, &point);

		rect.left += point.x;
		rect.right += point.x;
		rect.top += point.y;
		rect.bottom += point.y;

		ClipCursor(&rect);
		ShowCursor(FALSE);

		// do not allow both keyboard and mouse to be masked at the same time!
		MaskKeyboard = FALSE;
	}
}
void	UnAcquire (void)
{
	int i;
	for (i = 0; i < NumDevices; i++)
		if (DeviceUsed[i])
			DIDevices[i]->Unacquire();
	if (MaskMouse)
	{
		ClipCursor(NULL);
		ShowCursor(TRUE);
	}
	MaskKeyboard = FALSE;
	MaskMouse = FALSE;
}

void	ClearKeyState (void)
{
	ZeroMemory(KeyState, sizeof(KeyState));
}

void	ClearMouseState (void)
{
	ZeroMemory(&MouseState, sizeof(MouseState));
}

void	ClearJoyState (int dev)
{
	ZeroMemory(&JoyState[dev], sizeof(DIJOYSTATE2));
	// axes need to be initialized to 0x8000
	JoyState[dev].lX = JoyState[dev].lY = JoyState[dev].lZ = 0x8000;
	JoyState[dev].lRx = JoyState[dev].lRy = JoyState[dev].lRz = 0x8000;
	JoyState[dev].rglSlider[0] = JoyState[dev].rglSlider[1] = 0x8000;
	// and POV hats need to be initialized to -1
	JoyState[dev].rgdwPOV[0] = JoyState[dev].rgdwPOV[1] = JoyState[dev].rgdwPOV[2] = JoyState[dev].rgdwPOV[3] = (DWORD)-1;
}

void	UpdateInput (void)
{
	HRESULT hr;
	int i;
	unsigned char Cmd = 0;
	if (DeviceUsed[0])
	{
		hr = DIDevices[0]->GetDeviceState(256, KeyState);
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			ClearKeyState();
			DIDevices[0]->Acquire();
		}
	}
	if (DeviceUsed[1])
	{
		hr = DIDevices[1]->GetDeviceState(sizeof(DIMOUSESTATE2), &MouseState);
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			ClearMouseState();
			DIDevices[1]->Acquire();
		}
	}
	for (i = 2; i < NumDevices; i++)
		if (DeviceUsed[i])
		{
			hr = DIDevices[i]->Poll();
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
			{
				ClearJoyState(i);
				DIDevices[i]->Acquire();
				continue;
			}
			hr = DIDevices[i]->GetDeviceState(sizeof(DIJOYSTATE2), &JoyState[i]);
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
				ClearJoyState(i);
		}

	if (Movie::Mode & MOV_PLAY)
		Cmd = Movie::LoadInput();
	else
	{
		if ((MI) && (MI->Config))
			Cmd = MI->Config(CFG_QUERY, 0);
	}
	Port1->Frame(Movie::Mode);
	Port2->Frame(Movie::Mode);
	PortExp->Frame(Movie::Mode);
	if ((Cmd) && (MI) && (MI->Config))
		MI->Config(CFG_CMD, Cmd);
	if (Movie::Mode & MOV_RECORD)
		Movie::SaveInput(Cmd);
}

int	GetConfigButton (HWND hWnd, int DevNum)
{
	LPDIRECTINPUTDEVICE8 dev = DIDevices[DevNum];
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

	if (FAILED(dev->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(hMainWnd, _T("Unable to modify device input cooperative level!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		return Key;
	}

	dev->Acquire();
	DWORD ticks;
	ticks = GetTickCount();
	while (Key == -1)
	{
		// abort after 5 seconds
		if (GetTickCount() - ticks > 5000)
			break;
		if (DevNum == 0)
			hr = dev->GetDeviceState(256, KeyState);
		else if (DevNum == 1)
			hr = dev->GetDeviceState(sizeof(DIMOUSESTATE2), &MouseState);
		else
		{
			hr = dev->Poll();
			hr = dev->GetDeviceState(sizeof(DIJOYSTATE2), &JoyState[DevNum]);
		}
		for (i = 0; i < NumButtons[DevNum]; i++)
		{
			if (IsPressed((DevNum << 16) | i))
			{
				Key = i;
				break;
			}
		}
		if (Key != -1)	// if we got a button, don't check for an axis
			break;
		for (i = FirstAxis; i < LastAxis; i++)
		{
			if (IsPressed((DevNum << 16) | i))
			{
				Key = i;
				break;
			}
		}
		if (Key != -1)	// if we got an axis, don't check for a POV hat
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

	ticks = GetTickCount();
	while (1)
	{
		// if we timed out above, then skip this
		if (Key == -1)
			break;
		// wait 3 seconds to release key
		if (GetTickCount() - ticks > 3000)
			break;

		bool held = false;
		if (DevNum == 0)
			hr = dev->GetDeviceState(256, KeyState);
		else if (DevNum == 1)
			hr = dev->GetDeviceState(sizeof(DIMOUSESTATE2), &MouseState);
		else
		{
			hr = dev->Poll();
			hr = dev->GetDeviceState(sizeof(DIJOYSTATE2), &JoyState[DevNum]);
		}
		// don't need to reset FirstAxis/LastAxis or FirstPOV/LastPOV, since they were set in the previous loop
		for (i = 0; i < NumButtons[DevNum]; i++)
			if (IsPressed((DevNum << 16) | i))
			{
				held = true;
				break;
			}
		if (held)
			continue;
		for (i = FirstAxis; i < LastAxis; i++)
			if (IsPressed((DevNum << 16) | i))
			{
				held = true;
				break;
			}
		if (held)
			continue;
		for (i = FirstPOV; i < LastPOV; i++)
			if (IsPressed((DevNum << 16) | i))
			{
				held = true;
				break;
			}
		if (held)
			continue;
		break;
	}
	dev->Unacquire();
	if (FAILED(dev->SetCooperativeLevel(hMainWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(hMainWnd, _T("Unable to restore device input cooperative level!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		return Key;
	}
	return Key;
}

const TCHAR *	GetButtonLabel (int DevNum, int Button)
{
	static TCHAR str[256];
	_tcscpy(str, _T("???"));
	if (DevNum == 0)
	{
		if (KeyNames[Button])
			_tcscpy(str, KeyNames[Button]);
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
				_tcscpy(str, ButtonNames[1][Button]);
			return str;
		}
	}
	else
	{
		if ((Button & 0xE0) == 0x80)
		{
			Button &= 0x0F;
			if (AxisNames[DevNum][Button >> 1])
				_stprintf(str, _T("%s %s"), AxisNames[DevNum][Button >> 1], (Button & 1) ? _T("(+)") : _T("(-)"));
			return str;
		}
		else if ((Button & 0xE0) == 0xC0)
		{
			const TCHAR *POVs[8] = {_T("(N)"), _T("(NE)"), _T("(E)"), _T("(SE)"), _T("(S)"), _T("(SW)"), _T("(W)"), _T("(NW)") };
			Button &= 0x1F;
			if (POVNames[DevNum][Button >> 3])
				_stprintf(str, _T("%s %s"), POVNames[DevNum][Button >> 3], POVs[Button & 0x7]);
			return str;
		}
		else if ((Button & 0xE0) == 0xE0)
		{
			const TCHAR *POVs[4] = {_T("Y (-)"), _T("X (+)"), _T("Y (+)"), _T("X (-)") };
			Button &= 0xF;
			if (POVNames[DevNum][Button >> 2])
				_stprintf(str, _T("%s %s"), POVNames[DevNum][Button >> 2], POVs[Button & 0x3]);
			return str;
		}
		else
		{
			if (ButtonNames[DevNum][Button])
				_tcscpy(str, ButtonNames[DevNum][Button]);
			return str;
		}
	}
}

void	ConfigButton (int *Button, int Device, HWND hDlg, BOOL getKey)
{
	*Button &= 0xFFFF;
	if (getKey)	// this way, we can just re-label the button
	{
		key = CreateDialog(hInst, (LPCTSTR)IDD_KEYCONFIG, hDlg, NULL);
		ShowWindow(key, TRUE);	// FIXME - center this window properly
		ProcessMessages();	// let the "Press a key..." dialog display itself
		int newKey = GetConfigButton(key, Device);
		if (newKey != -1)
			*Button = newKey;
		ProcessMessages();	// flush all keypresses - don't want them going back to the parent dialog
		DestroyWindow(key);	// close the little window
		key = NULL;
	}
	SetWindowText(hDlg, GetButtonLabel(Device, *Button));
	*Button |= Device << 16;	// add the device ID
}

BOOL	IsPressed (int Button)
{
	int DevNum = (Button & 0xFFFF0000) >> 16;
	if (DevNum == 0)
		return (KeyState[Button & 0xFF] & 0x80) ? TRUE : FALSE;
	else if (DevNum == 1)
	{
		if (Button & 0x8)	// axis selected
		{
			switch (Button & 0x7)
			{
			case 0x0:	return ((AxisFlags[1] & (1 << AXIS_X)) && (MouseState.lX < -1)) ? TRUE : FALSE;	break;
			case 0x1:	return ((AxisFlags[1] & (1 << AXIS_X)) && (MouseState.lX > +1)) ? TRUE : FALSE;	break;
			case 0x2:	return ((AxisFlags[1] & (1 << AXIS_Y)) && (MouseState.lY < -1)) ? TRUE : FALSE;	break;
			case 0x3:	return ((AxisFlags[1] & (1 << AXIS_Y)) && (MouseState.lY > +1)) ? TRUE : FALSE;	break;
			case 0x4:	return ((AxisFlags[1] & (1 << AXIS_Z)) && (MouseState.lZ < -1)) ? TRUE : FALSE;	break;
			case 0x5:	return ((AxisFlags[1] & (1 << AXIS_Z)) && (MouseState.lZ > +1)) ? TRUE : FALSE;	break;
			}
		}
		else	return (MouseState.rgbButtons[Button & 0x7] & 0x80) ? TRUE : FALSE;
	}
	else
	{
		if ((Button & 0xE0) == 0x80)
		{	// axis
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
			}
		}
		else if ((Button & 0xE0) == 0xC0)
		{	// POV trigger (8-button mode)
			if (POVAxis)
				return FALSE;
			int povNum = (Button >> 3) & 0x3;
			switch (Button & 0x7)
			{
			case 0x00:	return ((POVFlags[DevNum] & (1 << povNum)) && ((JoyState[DevNum].rgdwPOV[povNum] > 33750) || (JoyState[DevNum].rgdwPOV[povNum] <  2250)) && (JoyState[DevNum].rgdwPOV[povNum] != -1)) ? TRUE : FALSE;	break;
			case 0x01:	return ((POVFlags[DevNum] & (1 << povNum)) && ((JoyState[DevNum].rgdwPOV[povNum] >  2250) && (JoyState[DevNum].rgdwPOV[povNum] <  6750))) ? TRUE : FALSE;	break;
			case 0x02:	return ((POVFlags[DevNum] & (1 << povNum)) && ((JoyState[DevNum].rgdwPOV[povNum] >  6750) && (JoyState[DevNum].rgdwPOV[povNum] < 11250))) ? TRUE : FALSE;	break;
			case 0x03:	return ((POVFlags[DevNum] & (1 << povNum)) && ((JoyState[DevNum].rgdwPOV[povNum] > 11250) && (JoyState[DevNum].rgdwPOV[povNum] < 15750))) ? TRUE : FALSE;	break;
			case 0x04:	return ((POVFlags[DevNum] & (1 << povNum)) && ((JoyState[DevNum].rgdwPOV[povNum] > 15750) && (JoyState[DevNum].rgdwPOV[povNum] < 20250))) ? TRUE : FALSE;	break;
			case 0x05:	return ((POVFlags[DevNum] & (1 << povNum)) && ((JoyState[DevNum].rgdwPOV[povNum] > 20250) && (JoyState[DevNum].rgdwPOV[povNum] < 24750))) ? TRUE : FALSE;	break;
			case 0x06:	return ((POVFlags[DevNum] & (1 << povNum)) && ((JoyState[DevNum].rgdwPOV[povNum] > 24750) && (JoyState[DevNum].rgdwPOV[povNum] < 29250))) ? TRUE : FALSE;	break;
			case 0x07:	return ((POVFlags[DevNum] & (1 << povNum)) && ((JoyState[DevNum].rgdwPOV[povNum] > 29250) && (JoyState[DevNum].rgdwPOV[povNum] < 33750))) ? TRUE : FALSE;	break;
			}
		}
		else if ((Button & 0xE0) == 0xE0)
		{	// POV trigger (axis mode)
			int povNum = (Button >> 2) & 0x3;
			switch (Button & 0x03)
			{
			case 0x0:	return ((POVFlags[DevNum] & (1 << povNum)) && ((JoyState[DevNum].rgdwPOV[povNum] > 29250) || (JoyState[DevNum].rgdwPOV[povNum] <  6750)) && (JoyState[DevNum].rgdwPOV[povNum] != -1)) ? TRUE : FALSE;	break;
			case 0x1:	return ((POVFlags[DevNum] & (1 << povNum)) && ((JoyState[DevNum].rgdwPOV[povNum] >  2250) && (JoyState[DevNum].rgdwPOV[povNum] < 15750))) ? TRUE : FALSE;	break;
			case 0x2:	return ((POVFlags[DevNum] & (1 << povNum)) && ((JoyState[DevNum].rgdwPOV[povNum] > 11250) && (JoyState[DevNum].rgdwPOV[povNum] < 24750))) ? TRUE : FALSE;	break;
			case 0x3:	return ((POVFlags[DevNum] & (1 << povNum)) && ((JoyState[DevNum].rgdwPOV[povNum] > 20250) && (JoyState[DevNum].rgdwPOV[povNum] < 33750))) ? TRUE : FALSE;	break;
			}
		}
		else	return (JoyState[DevNum].rgbButtons[Button & 0x7F] & 0x80) ? TRUE : FALSE;
	}
	// should never actually reach this point - this is just to make the compiler stop whining
	return FALSE;
}
INT_PTR	ParseConfigMessages (HWND hDlg, int numItems, int *dlgDevices, int *dlgButtons, int *Buttons, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);
	int i;
	if (key)
		return FALSE;
	if (uMsg == WM_INITDIALOG)
	{
		for (i = 0; i < numItems; i++)
		{
			int j;
			SendDlgItemMessage(hDlg, dlgDevices[i], CB_RESETCONTENT, 0, 0);		// clear the listbox
			for (j = 0; j < NumDevices; j++)
				SendDlgItemMessage(hDlg, dlgDevices[i], CB_ADDSTRING, 0, (LPARAM)DeviceName[j]);	// add each device
			SendDlgItemMessage(hDlg, dlgDevices[i], CB_SETCURSEL, Buttons[i] >> 16, 0);	// select the one we want
			ConfigButton(&Buttons[i], Buttons[i] >> 16, GetDlgItem(hDlg, dlgButtons[i]), FALSE);	// and label the corresponding button
		}
	}
	if (uMsg != WM_COMMAND)
		return FALSE;
	if (wmId == IDOK)
	{
		EndDialog(hDlg, 1);
		return TRUE;
	}
	for (i = 0; i < numItems; i++)
	{
		if (wmId == dlgDevices[i])
		{
			if (wmEvent != CBN_SELCHANGE)
				break;
			Buttons[i] = 0;
			ConfigButton(&Buttons[i], (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0), GetDlgItem(hDlg, dlgButtons[i]), FALSE);
			return TRUE;
		}
		if (wmId == dlgButtons[i])
		{
			ConfigButton(&Buttons[i], Buttons[i] >> 16, GetDlgItem(hDlg, dlgButtons[i]), TRUE);
			return TRUE;
		}
	}
	return FALSE;
}
} // namespace Controllers