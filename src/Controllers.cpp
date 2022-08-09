/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) QMT Productions
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

DWORD	Port1_Buttons[CONTROLLERS_MAXBUTTONS],
	Port2_Buttons[CONTROLLERS_MAXBUTTONS];
DWORD	FSPort1_Buttons[CONTROLLERS_MAXBUTTONS],
	FSPort2_Buttons[CONTROLLERS_MAXBUTTONS],
	FSPort3_Buttons[CONTROLLERS_MAXBUTTONS],
	FSPort4_Buttons[CONTROLLERS_MAXBUTTONS];
DWORD	PortExp_Buttons[CONTROLLERS_MAXBUTTONS];

BOOL	EnableOpposites;

int	NumDevices;

struct TDeviceInfo
{
	LPDIRECTINPUTDEVICE8 DIDevice;
	GUID GUID;

	BOOL Used;
	TCHAR *Name;

	int NumButtons;
	BYTE AxisFlags;
	BYTE POVFlags;

	TCHAR *ButtonNames[256];
	TCHAR *AxisNames[8];
	TCHAR *POVNames[4];

	union
	{
		BYTE KeyState[256];
		DIMOUSESTATE2 MouseState;
		DIJOYSTATE2 JoyState;
	};
};
TDeviceInfo Devices[MAX_CONTROLLERS];

LPDIRECTINPUT8		DirectInput;

void	StdPort_SetControllerType (StdPort *&Port, STDCONT_TYPE Type, DWORD *buttons)
{
	if (Port != NULL)
	{
		delete Port;
		Port = NULL;
	}
	switch (Type)
	{
	default:			MessageBox(hMainWnd, _T("Error: selected invalid controller type for standard port!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
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

void    ExpPort_SetControllerType (ExpPort *&Port, EXPCONT_TYPE Type, DWORD *buttons)
{
	if (Port != NULL)
	{
		delete Port;
		Port = NULL;
	}
        switch (Type)
        {
        default:			MessageBox(hMainWnd, _T("Error: selected invalid controller type for expansion port!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
        case EXP_UNCONNECTED:		Port = new ExpPort_Unconnected(buttons);		break;
        case EXP_FAMI4PLAY:		Port = new ExpPort_Fami4Play(buttons);			break;
        case EXP_ARKANOIDPADDLE:	Port = new ExpPort_ArkanoidPaddle(buttons);		break;
        case EXP_FAMILYBASICKEYBOARD:	Port = new ExpPort_FamilyBasicKeyboard(buttons);	break;
        case EXP_SUBORKEYBOARD:		Port = new ExpPort_SuborKeyboard(buttons);		break;
        case EXP_FAMTRAINER:		Port = new ExpPort_FamTrainer(buttons);			break;
        case EXP_TABLET:		Port = new ExpPort_Tablet(buttons);			break;
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

BOOL	POVAxis = TRUE;

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
			POVAxis = !(IsDlgButtonChecked(hDlg, IDC_CONT_POV) == BST_CHECKED);
			return TRUE;
		case IDOK:
			EnableOpposites = (IsDlgButtonChecked(hDlg, IDC_CONT_UDLR) == BST_CHECKED);
			POVAxis = TRUE;
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
	DialogBox(hInst, MAKEINTRESOURCE(IDD_CONTROLLERS), hMainWnd, ControllerProc);
	SetDeviceUsed();
}

BOOL CALLBACK	EnumKeyboardObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	int ItemNum = 0;
	TDeviceInfo &dev = Devices[0];

	if (IsEqualGUID(lpddoi->guidType, GUID_Key))
	{
		ItemNum = lpddoi->dwOfs;
		if ((ItemNum >= 0) && (ItemNum < 256))
			dev.ButtonNames[ItemNum] = _tcsdup(lpddoi->tszName);
		else	MessageBox(hMainWnd, _T("Error - encountered invalid keyboard key ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}
	return DIENUM_CONTINUE;
}

BOOL CALLBACK	EnumMouseObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	size_t ItemNum = 0;
	TDeviceInfo &dev = Devices[1];
	if (IsEqualGUID(lpddoi->guidType, GUID_XAxis))
	{
		dev.AxisFlags |= 0x01;
		dev.AxisNames[AXIS_X] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_YAxis))
	{
		dev.AxisFlags |= 0x02;
		dev.AxisNames[AXIS_Y] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_ZAxis))
	{
		dev.AxisFlags |= 0x04;
		dev.AxisNames[AXIS_Z] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_Button))
	{
		ItemNum = (lpddoi->dwOfs - FIELD_OFFSET(DIMOUSESTATE2, rgbButtons)) / sizeof(dev.MouseState.rgbButtons[0]);
		if ((ItemNum >= 0) && (ItemNum < 8))
			dev.ButtonNames[ItemNum] = _tcsdup(lpddoi->tszName);
		else	MessageBox(hMainWnd, _T("Error - encountered invalid mouse button ID!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
	}

	return DIENUM_CONTINUE;
}

BOOL CALLBACK	EnumJoystickObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	int DevNum = (int)(size_t)pvRef;
	TDeviceInfo &dev = Devices[DevNum];
	int ItemNum = 0;
	if (IsEqualGUID(lpddoi->guidType, GUID_XAxis))
	{
		dev.AxisFlags |= 0x01;
		dev.AxisNames[AXIS_X] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_YAxis))
	{
		dev.AxisFlags |= 0x02;
		dev.AxisNames[AXIS_Y] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_ZAxis))
	{
		dev.AxisFlags |= 0x04;
		dev.AxisNames[AXIS_Z] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_RxAxis))
	{
		dev.AxisFlags |= 0x08;
		dev.AxisNames[AXIS_RX] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_RyAxis))
	{
		dev.AxisFlags |= 0x10;
		dev.AxisNames[AXIS_RY] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_RzAxis))
	{
		dev.AxisFlags |= 0x20;
		dev.AxisNames[AXIS_RZ] = _tcsdup(lpddoi->tszName);
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_Slider))
	{
		// ItemNum = DIDFT_GETINSTANCE(lpddoi->dwType);
		// Sliders are enumerated as axes, and this index
		// starts where the other axes left off
		// Thus, we need to ignore it and assign them incrementally
		// Hopefully, this actually works reliably
		if (!(dev.AxisFlags & 0x40))
			ItemNum = 0;
		else if (!(dev.AxisFlags & 0x80))
			ItemNum = 1;
		else	ItemNum = 2;
		if (ItemNum == 0)
		{
			dev.AxisFlags |= 0x40;
			dev.AxisNames[AXIS_S0] = _tcsdup(lpddoi->tszName);
		}
		else if (ItemNum == 1)
		{
			dev.AxisFlags |= 0x80;
			dev.AxisNames[AXIS_S1] = _tcsdup(lpddoi->tszName);
		}
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_POV))
	{
		ItemNum = DIDFT_GETINSTANCE(lpddoi->dwType);
		if ((ItemNum >= 0) && (ItemNum < 4))
		{
			dev.POVFlags |= 0x01 << ItemNum;
			dev.POVNames[ItemNum] = _tcsdup(lpddoi->tszName);
		}
	}
	if (IsEqualGUID(lpddoi->guidType, GUID_Button))
	{
		ItemNum = DIDFT_GETINSTANCE(lpddoi->dwType);
		if ((ItemNum >= 0) && (ItemNum < 128))
			dev.ButtonNames[ItemNum] = _tcsdup(lpddoi->tszName);
	}
	return DIENUM_CONTINUE;
}

BOOL CALLBACK	EnumJoysticksCallback (LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	HRESULT hr;
	int DevNum = NumDevices;
	TDeviceInfo &dev = Devices[DevNum];
	do
	{
		if (SUCCEEDED(DirectInput->CreateDevice(lpddi->guidInstance, &dev.DIDevice, NULL)))
		{
			DIDEVCAPS caps;
			if (FAILED(hr = dev.DIDevice->SetDataFormat(&c_dfDIJoystick2)))
				break;
			if (FAILED(hr = dev.DIDevice->SetCooperativeLevel(hMainWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
				break;
			caps.dwSize = sizeof(DIDEVCAPS);
			if (FAILED(hr = dev.DIDevice->GetCapabilities(&caps)))
				break;

			dev.NumButtons = caps.dwButtons;
			dev.AxisFlags = 0;
			dev.POVFlags = 0;
			dev.Name = _tcsdup(lpddi->tszProductName);
			dev.GUID = lpddi->guidInstance;

			dev.DIDevice->EnumObjects(EnumJoystickObjectsCallback, (LPVOID)(size_t)DevNum, DIDFT_ALL);
			EI.DbgOut(_T("Added input device '%s' with %i buttons, %i axes, %i POVs"), lpddi->tszProductName, caps.dwButtons, caps.dwAxes, caps.dwPOVs);
			NumDevices++;
		}
		return DIENUM_CONTINUE;
	} while (0);

	dev.DIDevice->Release();
	dev.DIDevice = NULL;
	return hr;
}

BOOL	InitKeyboard (void)
{
	DIDEVICEINSTANCE inst;
	DIDEVCAPS caps;
	TDeviceInfo &dev = Devices[0];
	do
	{
		if (FAILED(DirectInput->CreateDevice(GUID_SysKeyboard, &dev.DIDevice, NULL)))
			return FALSE;
		if (FAILED(dev.DIDevice->SetDataFormat(&c_dfDIKeyboard)))
			break;
		if (FAILED(dev.DIDevice->SetCooperativeLevel(hMainWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
			break;

		caps.dwSize = sizeof(DIDEVCAPS);
		if (FAILED(dev.DIDevice->GetCapabilities(&caps)))
			break;

		inst.dwSize = sizeof(DIDEVICEINSTANCE);
		if (FAILED(dev.DIDevice->GetDeviceInfo(&inst)))
			break;

		dev.NumButtons = 256;	// normally, I would use caps.dwButtons
		dev.AxisFlags = 0;	// no axes
		dev.POVFlags = 0;	// no POV hats
		dev.Name = _tcsdup(inst.tszProductName);
		dev.GUID = GUID_SysKeyboard;

		dev.DIDevice->EnumObjects(EnumKeyboardObjectsCallback, NULL, DIDFT_ALL);
		EI.DbgOut(_T("Added input device '%s' with %i buttons, %i axes, %i POVs"), inst.tszProductName, caps.dwButtons, caps.dwAxes, caps.dwPOVs);
		return TRUE;
	} while (0);

	dev.DIDevice->Release();
	dev.DIDevice = NULL;
	return FALSE;
}

BOOL	InitMouse (void)
{
	DIDEVICEINSTANCE inst;
	DIDEVCAPS caps;
	TDeviceInfo &dev = Devices[1];
	do
	{
		if (FAILED(DirectInput->CreateDevice(GUID_SysMouse, &dev.DIDevice, NULL)))
			return FALSE;
		if (FAILED(dev.DIDevice->SetDataFormat(&c_dfDIMouse2)))
			break;
		if (FAILED(dev.DIDevice->SetCooperativeLevel(hMainWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
			break;
		caps.dwSize = sizeof(DIDEVCAPS);
		if (FAILED(dev.DIDevice->GetCapabilities(&caps)))
			break;

		inst.dwSize = sizeof(DIDEVICEINSTANCE);
		if (FAILED(dev.DIDevice->GetDeviceInfo(&inst)))
			break;

		dev.NumButtons = caps.dwButtons;
		dev.AxisFlags = 0;
		dev.POVFlags = 0;
		dev.Name = _tcsdup(inst.tszProductName);
		dev.GUID = GUID_SysMouse;

		dev.DIDevice->EnumObjects(EnumMouseObjectsCallback, NULL, DIDFT_ALL);
		EI.DbgOut(_T("Added input device '%s' with %i buttons, %i axes, %i POVs"), inst.tszProductName, caps.dwButtons, caps.dwAxes, caps.dwPOVs);
		return TRUE;
	} while (0);

	dev.DIDevice->Release();
	dev.DIDevice = NULL;
	return FALSE;
}

void	Init (void)
{
	int i;
	
	for (i = 0; i < MAX_CONTROLLERS; i++)
	{
		TDeviceInfo &dev = Devices[i];

		dev.DIDevice = NULL;
		ZeroMemory(&dev.GUID, sizeof(dev.GUID));

		dev.Used = FALSE;
		dev.Name = NULL;

		dev.NumButtons = 0;
		dev.AxisFlags = 0;
		dev.POVFlags = 0;
		ZeroMemory(dev.ButtonNames, sizeof(dev.ButtonNames));
		ZeroMemory(dev.AxisNames, sizeof(dev.AxisNames));
		ZeroMemory(dev.POVNames, sizeof(dev.POVNames));
	}

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
		TDeviceInfo &dev = Devices[i];

		if (dev.DIDevice)
		{
			dev.DIDevice->Release();
			dev.DIDevice = NULL;
		}
		// Allocated using _tcsdup()
		free(dev.Name);
		dev.Name = NULL;
		for (j = 0; j < 256; j++)
		{
			// Allocated using _tcsdup()
			free(dev.ButtonNames[j]);
			dev.ButtonNames[j] = NULL;
		}
		for (j = 0; j < 8; j++)
		{
			// Allocated using _tcsdup()
			free(dev.AxisNames[j]);
			dev.AxisNames[j] = NULL;
		}
		for (j = 0; j < 4; j++)
		{
			// Allocated using _tcsdup()
			free(dev.POVNames[j]);
			dev.POVNames[j] = NULL;
		}
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
	{
		EI.DbgOut(_T("WARNING: Four-score configured in port 2 but not port 1 - savestate may be corrupted."));
		SET_STDCONT(Port2, STD_UNCONNECTED);
	}
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
	DWORD numDevsUsed = 0;
	for (int i = 0; i < MAX_CONTROLLERS; i++)
		if (Devices[i].Used)
			numDevsUsed++;

	int buflen = sizeof(DWORD) + numDevsUsed * (sizeof(GUID) + sizeof(DWORD));
	BYTE *buf = new BYTE[buflen], *b = buf;

	memcpy(b, &numDevsUsed, sizeof(DWORD)); b += sizeof(DWORD);
	for (DWORD i = 0; i < MAX_CONTROLLERS; i++)
	{
		if (!Devices[i].Used)
			continue;
		memcpy(b, &i, sizeof(DWORD)); b += sizeof(DWORD);
		memcpy(b, &Devices[i].GUID, sizeof(GUID)); b += sizeof(GUID);
	}

	RegSetValueEx(SettingsBase, _T("UDLR")    , 0, REG_DWORD, (LPBYTE)&EnableOpposites, sizeof(BOOL));

	RegSetValueEx(SettingsBase, _T("Port1T")  , 0, REG_DWORD, (LPBYTE)&Port1->Type  , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("Port2T")  , 0, REG_DWORD, (LPBYTE)&Port2->Type  , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort1T"), 0, REG_DWORD, (LPBYTE)&FSPort1->Type, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort2T"), 0, REG_DWORD, (LPBYTE)&FSPort2->Type, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort3T"), 0, REG_DWORD, (LPBYTE)&FSPort3->Type, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort4T"), 0, REG_DWORD, (LPBYTE)&FSPort4->Type, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("ExpPortT"), 0, REG_DWORD, (LPBYTE)&PortExp->Type, sizeof(DWORD));

	RegSetValueEx(SettingsBase, _T("Port1D")  , 0, REG_BINARY, (LPBYTE)Port1_Buttons  , Port1->NumButtons * sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("Port2D")  , 0, REG_BINARY, (LPBYTE)Port2_Buttons  , Port2->NumButtons * sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort1D"), 0, REG_BINARY, (LPBYTE)FSPort1_Buttons, FSPort1->NumButtons * sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort2D"), 0, REG_BINARY, (LPBYTE)FSPort2_Buttons, FSPort2->NumButtons * sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort3D"), 0, REG_BINARY, (LPBYTE)FSPort3_Buttons, FSPort3->NumButtons * sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort4D"), 0, REG_BINARY, (LPBYTE)FSPort4_Buttons, FSPort4->NumButtons * sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("ExpPortD"), 0, REG_BINARY, (LPBYTE)PortExp_Buttons, PortExp->NumButtons * sizeof(DWORD));

	RegSetValueEx(SettingsBase, _T("PortM")   , 0, REG_BINARY, buf, buflen);

	delete[] buf;
}

void	LoadSettings (HKEY SettingsBase)
{
	unsigned long Size;
	DWORD Port1T = 0, Port2T = 0, FSPort1T = 0, FSPort2T = 0, FSPort3T = 0, FSPort4T = 0, ExpPortT = 0;
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

	Size = Port1->NumButtons * sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("Port1D")  , 0, NULL, (LPBYTE)Port1_Buttons  , &Size);
	Size = Port2->NumButtons * sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("Port2D")  , 0, NULL, (LPBYTE)Port2_Buttons  , &Size);
	Size = FSPort1->NumButtons * sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSPort1D"), 0, NULL, (LPBYTE)FSPort1_Buttons, &Size);
	Size = FSPort2->NumButtons * sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSPort2D"), 0, NULL, (LPBYTE)FSPort2_Buttons, &Size);
	Size = FSPort3->NumButtons * sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSPort3D"), 0, NULL, (LPBYTE)FSPort3_Buttons, &Size);
	Size = FSPort4->NumButtons * sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSPort4D"), 0, NULL, (LPBYTE)FSPort4_Buttons, &Size);
	Size = PortExp->NumButtons * sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("ExpPortD"), 0, NULL, (LPBYTE)PortExp_Buttons, &Size);

	if (RegQueryValueEx(SettingsBase, _T("PortM"), 0, NULL, NULL, &Size) == ERROR_SUCCESS)
	{
		byte* buf = new byte[Size], *b = buf;
		RegQueryValueEx(SettingsBase, _T("PortM"), 0, NULL, buf, &Size);

		DWORD mapLen;
		memcpy(&mapLen, buf, sizeof(DWORD)); b += sizeof(DWORD);
		DWORD *map_nums = new DWORD[mapLen];
		GUID *map_guids = new GUID[mapLen];
		for (size_t i = 0; i < mapLen; i++)
		{
			memcpy(&map_nums[i], b, sizeof(DWORD)); b += sizeof(DWORD);
			memcpy(&map_guids[i], b, sizeof(GUID)); b += sizeof(GUID);
		}
		
		int Lens[7] = { Port1->NumButtons, Port2->NumButtons, FSPort1->NumButtons, FSPort2->NumButtons, FSPort3->NumButtons, FSPort4->NumButtons, PortExp->NumButtons };
		DWORD *Datas[7] = { Port1_Buttons, Port2_Buttons, FSPort1_Buttons, FSPort2_Buttons, FSPort3_Buttons, FSPort4_Buttons, PortExp_Buttons };
		TCHAR *Descs[7] = { _T("Port 1"), _T("Port 2"), _T("Four-score Port 1"), _T("Four-score Port 2"), _T("Four-score Port 3"), _T("Four-score Port 4"), _T("Expansion Port") };

		// Resolve stored device IDs into current device index values via GUID matching
		// This isn't the most efficient way of doing it, but it's still fast so nobody cares
		for (int i = 0; i < 7; i++)
		{
			for (int j = 0; j < Lens[i]; j++)
			{
				DWORD &Button = Datas[i][j];
				DWORD DevNum = (Button & 0xFFFF0000) >> 16;
				int found = 0;
				for (size_t k = 0; k < mapLen; k++)
				{
					if (DevNum == map_nums[k])
					{
						found = 1;
						for (int m = 0; m < NumDevices; m++)
						{
							if (IsEqualGUID(map_guids[k], Devices[m].GUID))
							{
								found = 2;
								Button = (m << 16) | (Button & 0xFFFF);
							}
						}
					}
				}
				if (found < 2)
				{
					EI.DbgOut(_T("%s controller button %i failed to find device, unmapping."), Descs[i], j);
					Button = 0;
				}
			}
		}

		delete[] map_nums;
		delete[] map_guids;
		delete[] buf;
	}

	SetDeviceUsed();
}

void	SetDeviceUsed (void)
{
	int i;

	for (i = 0; i < NumDevices; i++)
		Devices[i].Used = FALSE;

	for (i = 0; i < CONTROLLERS_MAXBUTTONS; i++)
	{
		if (Port1->Type == STD_FOURSCORE)
		{
			if (i < FSPort1->NumButtons)
				Devices[FSPort1->Buttons[i] >> 16].Used = TRUE;
			if (i < FSPort2->NumButtons)
				Devices[FSPort2->Buttons[i] >> 16].Used = TRUE;
			if (i < FSPort3->NumButtons)
				Devices[FSPort3->Buttons[i] >> 16].Used = TRUE;
			if (i < FSPort4->NumButtons)
				Devices[FSPort4->Buttons[i] >> 16].Used = TRUE;
		}
		else
		{
			if (i < Port1->NumButtons)
				Devices[Port1->Buttons[i] >> 16].Used = TRUE;
			if (i < Port2->NumButtons)
				Devices[Port2->Buttons[i] >> 16].Used = TRUE;
		}
		if (i < PortExp->NumButtons)
			Devices[PortExp->Buttons[i] >> 16].Used = TRUE;
	}
	if ((Port1->Type == STD_ARKANOIDPADDLE) || (Port2->Type == STD_ARKANOIDPADDLE) || (PortExp->Type == EXP_ARKANOIDPADDLE))
		Devices[1].Used = TRUE;

	if ((PortExp->Type == EXP_FAMILYBASICKEYBOARD) || (PortExp->Type == EXP_SUBORKEYBOARD))
		Devices[0].Used = TRUE;
}

void	Acquire (void)
{
	int i;
	for (i = 0; i < NumDevices; i++)
		if (Devices[i].Used)
			Devices[i].DIDevice->Acquire();
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
		if (Devices[i].Used)
			Devices[i].DIDevice->Unacquire();
	if (MaskMouse)
	{
		ClipCursor(NULL);
		ShowCursor(TRUE);
	}
	MaskKeyboard = FALSE;
	MaskMouse = FALSE;
}

void	ClearDevState (int DevNum)
{
	TDeviceInfo &dev = Devices[DevNum];
	if (DevNum == 0)
		ZeroMemory(&dev.KeyState, sizeof(dev.KeyState));
	else if (DevNum == 1)
		ZeroMemory(&dev.MouseState, sizeof(dev.MouseState));
	else
	{
		ZeroMemory(&dev.JoyState, sizeof(dev.JoyState));
		// axes need to be initialized to 0x8000
		dev.JoyState.lX = dev.JoyState.lY = dev.JoyState.lZ = 0x8000;
		dev.JoyState.lRx = dev.JoyState.lRy = dev.JoyState.lRz = 0x8000;
		dev.JoyState.rglSlider[0] = dev.JoyState.rglSlider[1] = 0x8000;
		// and POV hats need to be initialized to -1
		dev.JoyState.rgdwPOV[0] = dev.JoyState.rgdwPOV[1] = dev.JoyState.rgdwPOV[2] = dev.JoyState.rgdwPOV[3] = (DWORD)-1;
	}
}

void	UpdateInput (void)
{
	HRESULT hr;
	int i;
	unsigned char Cmd = 0;
	for (i = 0; i < NumDevices; i++)
	{
		TDeviceInfo &dev = Devices[i];
		if (!dev.Used)
			continue;
		if (i == 0)
		{
			hr = dev.DIDevice->GetDeviceState(sizeof(dev.KeyState), &dev.KeyState);
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
			{
				ClearDevState(0);
				dev.DIDevice->Acquire();
			}
		}
		else if (i == 1)
		{
			hr = dev.DIDevice->GetDeviceState(sizeof(dev.MouseState), &dev.MouseState);
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
			{
				ClearDevState(1);
				dev.DIDevice->Acquire();
			}
		}
		else
		{
			hr = dev.DIDevice->Poll();
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
			{
				ClearDevState(i);
				dev.DIDevice->Acquire();
				continue;
			}
			hr = dev.DIDevice->GetDeviceState(sizeof(dev.JoyState), &dev.JoyState);
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
				ClearDevState(i);
		}
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

int	GetConfigButton (HWND hWnd, int DevNum, BOOL AxesOnly = FALSE)
{
	TDeviceInfo &dev = Devices[DevNum];
	HRESULT hr;
	int i;
	int Key = -1;
	int FirstAxis, LastAxis;
	int FirstPOV, LastPOV;
	if (DevNum == 0)
	{
		if (AxesOnly)
			return 0;
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
		if (POVAxis)
		{
			FirstPOV = 0xE0;
			LastPOV = 0xF0;
		}
		else
		{
			FirstPOV = 0xC0;
			LastPOV = 0xE0;
		}
	}

	if (FAILED(dev.DIDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(hMainWnd, _T("Unable to modify device input cooperative level!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		return Key;
	}

	dev.DIDevice->Acquire();
	DWORD ticks;
	ticks = GetTickCount();
	while (Key == -1)
	{
		ProcessMessages();	// Wine workaround
		// abort after 5 seconds
		if (GetTickCount() - ticks > 5000)
			break;
		if (DevNum == 0)
			hr = dev.DIDevice->GetDeviceState(sizeof(dev.KeyState), &dev.KeyState);
		else if (DevNum == 1)
			hr = dev.DIDevice->GetDeviceState(sizeof(dev.MouseState), &dev.MouseState);
		else
		{
			hr = dev.DIDevice->Poll();
			hr = dev.DIDevice->GetDeviceState(sizeof(dev.JoyState), &dev.JoyState);
		}
		if (!AxesOnly)
		{
			for (i = 0; i < dev.NumButtons; i++)
			{
				if (IsPressed((DevNum << 16) | i))
				{
					Key = i;
					break;
				}
			}
			if (Key != -1)	// if we got a button, don't check for an axis
				break;
		}
		for (i = FirstAxis; i < LastAxis; i++)
		{
			// For axes, require sufficient magnitude,
			// otherwise Mouse controls are nigh impossible to configure
			if (GetDelta((DevNum << 16) | i) >= 8)
			{
				Key = i;
				break;
			}
		}
		if (!AxesOnly)
		{
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
	}

	ticks = GetTickCount();
	while (1)
	{
		// if we timed out above, then skip this
		if (Key == -1)
			break;
		ProcessMessages();	// Wine workaround
		// wait 3 seconds to release key
		if (GetTickCount() - ticks > 3000)
			break;

		bool held = false;
		if (DevNum == 0)
			hr = dev.DIDevice->GetDeviceState(sizeof(dev.KeyState), &dev.KeyState);
		else if (DevNum == 1)
			hr = dev.DIDevice->GetDeviceState(sizeof(dev.MouseState), &dev.MouseState);
		else
		{
			hr = dev.DIDevice->Poll();
			hr = dev.DIDevice->GetDeviceState(sizeof(dev.JoyState), &dev.JoyState);
		}
		// don't need to reset FirstAxis/LastAxis or FirstPOV/LastPOV, since they were set in the previous loop
		if (!AxesOnly)
		{
			for (i = 0; i < dev.NumButtons; i++)
				if (IsPressed((DevNum << 16) | i))
				{
					held = true;
					break;
				}
			if (held)
				continue;
		}
		for (i = FirstAxis; i < LastAxis; i++)
			if (GetDelta((DevNum << 16) | i) >= 8)
			{
				held = true;
				break;
			}
		if (held)
			continue;
		if (!AxesOnly)
		{
			for (i = FirstPOV; i < LastPOV; i++)
				if (IsPressed((DevNum << 16) | i))
				{
					held = true;
					break;
				}
			if (held)
				continue;
		}
		break;
	}
	dev.DIDevice->Unacquire();
	if (FAILED(dev.DIDevice->SetCooperativeLevel(hMainWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		MessageBox(hMainWnd, _T("Unable to restore device input cooperative level!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		return Key;
	}
	return Key;
}

const TCHAR *	GetButtonLabel (int DevNum, int Button, BOOL AxesOnly = FALSE)
{
	TDeviceInfo &dev = Devices[DevNum];

	static TCHAR str[256];
	_tcscpy(str, _T("???"));

	if (AxesOnly && (!DevNum || !Button))
		return str;
	if (DevNum == 0)
	{
		if (dev.ButtonNames[Button])
			_tcscpy(str, dev.ButtonNames[Button]);
		return str;
	}
	else if (DevNum == 1)
	{
		if (Button & 0x08)
		{
			Button &= 0x07;
			if (dev.AxisNames[Button >> 1])
				_stprintf(str, _T("%s %s"), dev.AxisNames[Button >> 1], (Button & 1) ? _T("(+)") : _T("(-)"));
			return str;
		}
		else
		{
			if (dev.ButtonNames[Button])
				_tcscpy(str, dev.ButtonNames[Button]);
			return str;
		}
	}
	else
	{
		if ((Button & 0xE0) == 0x80)
		{
			Button &= 0x0F;
			if (dev.AxisNames[Button >> 1])
				_stprintf(str, _T("%s %s"), dev.AxisNames[Button >> 1], (Button & 1) ? _T("(+)") : _T("(-)"));
			return str;
		}
		else if ((Button & 0xE0) == 0xC0)
		{
			const TCHAR *POVs[8] = {_T("(N)"), _T("(NE)"), _T("(E)"), _T("(SE)"), _T("(S)"), _T("(SW)"), _T("(W)"), _T("(NW)") };
			Button &= 0x1F;
			if (dev.POVNames[Button >> 3])
				_stprintf(str, _T("%s %s"), dev.POVNames[Button >> 3], POVs[Button & 0x7]);
			return str;
		}
		else if ((Button & 0xE0) == 0xE0)
		{
			const TCHAR *POVs[4] = {_T("Y (-)"), _T("X (+)"), _T("Y (+)"), _T("X (-)") };
			Button &= 0xF;
			if (dev.POVNames[Button >> 2])
				_stprintf(str, _T("%s %s"), dev.POVNames[Button >> 2], POVs[Button & 0x3]);
			return str;
		}
		else
		{
			if (dev.ButtonNames[Button])
				_tcscpy(str, dev.ButtonNames[Button]);
			return str;
		}
	}
}

void	ConfigButton (DWORD *Button, int Device, HWND hDlg, BOOL getKey, BOOL AxesOnly)
{
	*Button &= 0xFFFF;
	if (getKey)	// this way, we can just re-label the button
	{
		key = CreateDialog(hInst, AxesOnly ? MAKEINTRESOURCE(IDD_AXISCONFIG) : MAKEINTRESOURCE(IDD_KEYCONFIG), hDlg, NULL);
		ShowWindow(key, TRUE);	// FIXME - center this window properly
		ProcessMessages();	// let the "Press a key..." dialog display itself
		int newKey = GetConfigButton(key, Device, AxesOnly);
		if (newKey != -1)
			*Button = newKey;
		ProcessMessages();	// flush all keypresses - don't want them going back to the parent dialog
		DestroyWindow(key);	// close the little window
		key = NULL;
	}
	SetWindowText(hDlg, GetButtonLabel(Device, *Button, AxesOnly));
	*Button |= Device << 16;	// add the device ID
}

BOOL	IsPressed (int Button)
{
	int DevNum = (Button & 0xFFFF0000) >> 16;
	TDeviceInfo &dev = Devices[DevNum];
	if (DevNum == 0)
		return (dev.KeyState[Button & 0xFF] & 0x80) ? TRUE : FALSE;
	else if (DevNum == 1)
	{
		if (Button & 0x8)	// axis selected
		{
			switch (Button & 0x7)
			{
			case 0x0:	return ((dev.AxisFlags & (1 << AXIS_X)) && (dev.MouseState.lX < -1)) ? TRUE : FALSE;	break;
			case 0x1:	return ((dev.AxisFlags & (1 << AXIS_X)) && (dev.MouseState.lX > +1)) ? TRUE : FALSE;	break;
			case 0x2:	return ((dev.AxisFlags & (1 << AXIS_Y)) && (dev.MouseState.lY < -1)) ? TRUE : FALSE;	break;
			case 0x3:	return ((dev.AxisFlags & (1 << AXIS_Y)) && (dev.MouseState.lY > +1)) ? TRUE : FALSE;	break;
			case 0x4:	return ((dev.AxisFlags & (1 << AXIS_Z)) && (dev.MouseState.lZ < -1)) ? TRUE : FALSE;	break;
			case 0x5:	return ((dev.AxisFlags & (1 << AXIS_Z)) && (dev.MouseState.lZ > +1)) ? TRUE : FALSE;	break;
			}
		}
		else	return (dev.MouseState.rgbButtons[Button & 0x7] & 0x80) ? TRUE : FALSE;
	}
	else
	{
		if ((Button & 0xE0) == 0x80)
		{	// axis
			switch (Button & 0xF)
			{
			case 0x0:	return ((dev.AxisFlags & (1 << AXIS_X)) && (dev.JoyState.lX < 0x4000)) ? TRUE : FALSE;	break;
			case 0x1:	return ((dev.AxisFlags & (1 << AXIS_X)) && (dev.JoyState.lX > 0xC000)) ? TRUE : FALSE;	break;
			case 0x2:	return ((dev.AxisFlags & (1 << AXIS_Y)) && (dev.JoyState.lY < 0x4000)) ? TRUE : FALSE;	break;
			case 0x3:	return ((dev.AxisFlags & (1 << AXIS_Y)) && (dev.JoyState.lY > 0xC000)) ? TRUE : FALSE;	break;
			case 0x4:	return ((dev.AxisFlags & (1 << AXIS_Z)) && (dev.JoyState.lZ < 0x4000)) ? TRUE : FALSE;	break;
			case 0x5:	return ((dev.AxisFlags & (1 << AXIS_Z)) && (dev.JoyState.lZ > 0xC000)) ? TRUE : FALSE;	break;
			case 0x6:	return ((dev.AxisFlags & (1 << AXIS_RX)) && (dev.JoyState.lRx < 0x4000)) ? TRUE : FALSE;	break;
			case 0x7:	return ((dev.AxisFlags & (1 << AXIS_RX)) && (dev.JoyState.lRx > 0xC000)) ? TRUE : FALSE;	break;
			case 0x8:	return ((dev.AxisFlags & (1 << AXIS_RY)) && (dev.JoyState.lRy < 0x4000)) ? TRUE : FALSE;	break;
			case 0x9:	return ((dev.AxisFlags & (1 << AXIS_RY)) && (dev.JoyState.lRy > 0xC000)) ? TRUE : FALSE;	break;
			case 0xA:	return ((dev.AxisFlags & (1 << AXIS_RZ)) && (dev.JoyState.lRz < 0x4000)) ? TRUE : FALSE;	break;
			case 0xB:	return ((dev.AxisFlags & (1 << AXIS_RZ)) && (dev.JoyState.lRz > 0xC000)) ? TRUE : FALSE;	break;
			case 0xC:	return ((dev.AxisFlags & (1 << AXIS_S0)) && (dev.JoyState.rglSlider[0] < 0x4000)) ? TRUE : FALSE;	break;
			case 0xD:	return ((dev.AxisFlags & (1 << AXIS_S0)) && (dev.JoyState.rglSlider[0] > 0xC000)) ? TRUE : FALSE;	break;
			case 0xE:	return ((dev.AxisFlags & (1 << AXIS_S1)) && (dev.JoyState.rglSlider[1] < 0x4000)) ? TRUE : FALSE;	break;
			case 0xF:	return ((dev.AxisFlags & (1 << AXIS_S1)) && (dev.JoyState.rglSlider[1] > 0xC000)) ? TRUE : FALSE;	break;
			}
		}
		else if ((Button & 0xE0) == 0xC0)
		{	// POV trigger (8-button mode)
			int povNum = (Button >> 3) & 0x3;
			if (dev.JoyState.rgdwPOV[povNum] == -1)
				return FALSE;
			switch (Button & 0x7)
			{
			case 0x00:	return ((dev.POVFlags & (1 << povNum)) && ((dev.JoyState.rgdwPOV[povNum] > 33750) || (dev.JoyState.rgdwPOV[povNum] <  2250))) ? TRUE : FALSE;	break;
			case 0x01:	return ((dev.POVFlags & (1 << povNum)) && ((dev.JoyState.rgdwPOV[povNum] >  2250) && (dev.JoyState.rgdwPOV[povNum] <  6750))) ? TRUE : FALSE;	break;
			case 0x02:	return ((dev.POVFlags & (1 << povNum)) && ((dev.JoyState.rgdwPOV[povNum] >  6750) && (dev.JoyState.rgdwPOV[povNum] < 11250))) ? TRUE : FALSE;	break;
			case 0x03:	return ((dev.POVFlags & (1 << povNum)) && ((dev.JoyState.rgdwPOV[povNum] > 11250) && (dev.JoyState.rgdwPOV[povNum] < 15750))) ? TRUE : FALSE;	break;
			case 0x04:	return ((dev.POVFlags & (1 << povNum)) && ((dev.JoyState.rgdwPOV[povNum] > 15750) && (dev.JoyState.rgdwPOV[povNum] < 20250))) ? TRUE : FALSE;	break;
			case 0x05:	return ((dev.POVFlags & (1 << povNum)) && ((dev.JoyState.rgdwPOV[povNum] > 20250) && (dev.JoyState.rgdwPOV[povNum] < 24750))) ? TRUE : FALSE;	break;
			case 0x06:	return ((dev.POVFlags & (1 << povNum)) && ((dev.JoyState.rgdwPOV[povNum] > 24750) && (dev.JoyState.rgdwPOV[povNum] < 29250))) ? TRUE : FALSE;	break;
			case 0x07:	return ((dev.POVFlags & (1 << povNum)) && ((dev.JoyState.rgdwPOV[povNum] > 29250) && (dev.JoyState.rgdwPOV[povNum] < 33750))) ? TRUE : FALSE;	break;
			}
		}
		else if ((Button & 0xE0) == 0xE0)
		{	// POV trigger (axis mode)
			int povNum = (Button >> 2) & 0x3;
			if (dev.JoyState.rgdwPOV[povNum] == -1)
				return FALSE;
			switch (Button & 0x03)
			{
			case 0x0:	return ((dev.POVFlags & (1 << povNum)) && ((dev.JoyState.rgdwPOV[povNum] > 29250) || (dev.JoyState.rgdwPOV[povNum] <  6750))) ? TRUE : FALSE;	break;
			case 0x1:	return ((dev.POVFlags & (1 << povNum)) && ((dev.JoyState.rgdwPOV[povNum] >  2250) && (dev.JoyState.rgdwPOV[povNum] < 15750))) ? TRUE : FALSE;	break;
			case 0x2:	return ((dev.POVFlags & (1 << povNum)) && ((dev.JoyState.rgdwPOV[povNum] > 11250) && (dev.JoyState.rgdwPOV[povNum] < 24750))) ? TRUE : FALSE;	break;
			case 0x3:	return ((dev.POVFlags & (1 << povNum)) && ((dev.JoyState.rgdwPOV[povNum] > 20250) && (dev.JoyState.rgdwPOV[povNum] < 33750))) ? TRUE : FALSE;	break;
			}
		}
		else	return (dev.JoyState.rgbButtons[Button & 0x7F] & 0x80) ? TRUE : FALSE;
	}
	// should never actually reach this point - this is just to make the compiler stop whining
	return FALSE;
}

int	GetDelta (int Button)
{
	int DevNum = (Button & 0xFFFF0000) >> 16;
	TDeviceInfo &dev = Devices[DevNum];
	if (DevNum == 0)
		return 0;	// Keyboard not supported
	else if (DevNum == 1)
	{
		if (Button & 0x8)	// axis selected
		{
			switch (Button & 0x7)
			{
			case 0x0:	return (dev.AxisFlags & (1 << AXIS_X)) ? -dev.MouseState.lX : 0;	break;
			case 0x1:	return (dev.AxisFlags & (1 << AXIS_X)) ?  dev.MouseState.lX : 0;	break;
			case 0x2:	return (dev.AxisFlags & (1 << AXIS_Y)) ? -dev.MouseState.lY : 0;	break;
			case 0x3:	return (dev.AxisFlags & (1 << AXIS_Y)) ?  dev.MouseState.lY : 0;	break;
			case 0x4:	return (dev.AxisFlags & (1 << AXIS_Z)) ? -dev.MouseState.lZ : 0;	break;
			case 0x5:	return (dev.AxisFlags & (1 << AXIS_Z)) ?  dev.MouseState.lZ : 0;	break;
			}
		}
		else	return 0;
	}
	else
	{
		if ((Button & 0xE0) == 0x80)
		{	// axis
			switch (Button & 0xF)
			{
			case 0x0:	return (dev.AxisFlags & (1 << AXIS_X)) ? -(0x8000 - dev.JoyState.lX) / 0x400 : 0;	break;
			case 0x1:	return (dev.AxisFlags & (1 << AXIS_X)) ?  (0x8000 - dev.JoyState.lX) / 0x400 : 0;	break;
			case 0x2:	return (dev.AxisFlags & (1 << AXIS_Y)) ? -(0x8000 - dev.JoyState.lY) / 0x400 : 0;	break;
			case 0x3:	return (dev.AxisFlags & (1 << AXIS_Y)) ?  (0x8000 - dev.JoyState.lY) / 0x400 : 0;	break;
			case 0x4:	return (dev.AxisFlags & (1 << AXIS_Z)) ? -(0x8000 - dev.JoyState.lZ) / 0x400 : 0;	break;
			case 0x5:	return (dev.AxisFlags & (1 << AXIS_Z)) ?  (0x8000 - dev.JoyState.lZ) / 0x400 : 0;	break;
			case 0x6:	return (dev.AxisFlags & (1 << AXIS_RX)) ? -(0x8000 - dev.JoyState.lRx) / 0x400 : 0;	break;
			case 0x7:	return (dev.AxisFlags & (1 << AXIS_RX)) ?  (0x8000 - dev.JoyState.lRx) / 0x400 : 0;	break;
			case 0x8:	return (dev.AxisFlags & (1 << AXIS_RY)) ? -(0x8000 - dev.JoyState.lRy) / 0x400 : 0;	break;
			case 0x9:	return (dev.AxisFlags & (1 << AXIS_RY)) ?  (0x8000 - dev.JoyState.lRy) / 0x400 : 0;	break;
			case 0xA:	return (dev.AxisFlags & (1 << AXIS_RZ)) ? -(0x8000 - dev.JoyState.lRz) / 0x400 : 0;	break;
			case 0xB:	return (dev.AxisFlags & (1 << AXIS_RZ)) ?  (0x8000 - dev.JoyState.lRz) / 0x400 : 0;	break;
			case 0xC:	return (dev.AxisFlags & (1 << AXIS_S0)) ? -(0x8000 - dev.JoyState.rglSlider[0]) / 0x400 : 0;	break;
			case 0xD:	return (dev.AxisFlags & (1 << AXIS_S0)) ?  (0x8000 - dev.JoyState.rglSlider[0]) / 0x400 : 0;	break;
			case 0xE:	return (dev.AxisFlags & (1 << AXIS_S1)) ? -(0x8000 - dev.JoyState.rglSlider[1]) / 0x400 : 0;	break;
			case 0xF:	return (dev.AxisFlags & (1 << AXIS_S1)) ?  (0x8000 - dev.JoyState.rglSlider[1]) / 0x400 : 0;	break;
			}
		}
		else	return 0;	// buttons and POV axes not supported
	}
	// should never actually reach this point - this is just to make the compiler stop whining
	return 0;
}

INT_PTR	ParseConfigMessages (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, int numButtons, int numAxes, const int *dlgDevices, const int *dlgButtons, DWORD *Buttons)
{
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);
	int i;
	if (key)
		return FALSE;
	if (uMsg == WM_INITDIALOG)
	{
		for (i = 0; i < numButtons + numAxes; i++)
		{
			int j;
			SendDlgItemMessage(hDlg, dlgDevices[i], CB_RESETCONTENT, 0, 0);		// clear the listbox
			// for configurable Axes, replace Keyboard with "Select a device..."
			SendDlgItemMessage(hDlg, dlgDevices[i], CB_ADDSTRING, 0, i < numButtons ? (LPARAM)Devices[0].Name : (LPARAM)_T("Select a device..."));
			for (j = 1; j < NumDevices; j++)
				SendDlgItemMessage(hDlg, dlgDevices[i], CB_ADDSTRING, 0, (LPARAM)Devices[j].Name);	// add each device
			SendDlgItemMessage(hDlg, dlgDevices[i], CB_SETCURSEL, Buttons[i] >> 16, 0);	// select the one we want
			ConfigButton(&Buttons[i], Buttons[i] >> 16, GetDlgItem(hDlg, dlgButtons[i]), FALSE, i >= numButtons);	// and label the corresponding button
		}
	}
	if (uMsg != WM_COMMAND)
		return FALSE;
	if (wmId == IDOK)
	{
		EndDialog(hDlg, 1);
		return TRUE;
	}
	for (i = 0; i < numButtons + numAxes; i++)
	{
		if (wmId == dlgDevices[i])
		{
			if (wmEvent != CBN_SELCHANGE)
				break;
			Buttons[i] = 0;
			ConfigButton(&Buttons[i], (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0), GetDlgItem(hDlg, dlgButtons[i]), FALSE, i >= numButtons);
			return TRUE;
		}
		if (wmId == dlgButtons[i])
		{
			ConfigButton(&Buttons[i], Buttons[i] >> 16, GetDlgItem(hDlg, dlgButtons[i]), TRUE, i >= numButtons);
			return TRUE;
		}
	}
	return FALSE;
}
} // namespace Controllers
