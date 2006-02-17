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

#ifdef	FMV_PLAYBACK
#define	Bits	Data[0]
#define	BitPtr	Data[1]
#define	Strobe	Data[2]
#define	File	Data[3]
static	void	Movie_UpdateCont (struct tStdPort *Cont)
{
	unsigned char x;
	Cont->Bits = 0;
	if (feof((FILE *)Cont->File))
		return;
	fread(&x,1,1,(FILE *)Cont->File);
	if (x & 0x01)	Cont->Bits |= 0x80;	// right
	if (x & 0x02)	Cont->Bits |= 0x40;	// left
	if (x & 0x04)	Cont->Bits |= 0x10;	// up
	if (x & 0x08)	Cont->Bits |= 0x20;	// down
	if (x & 0x10)	Cont->Bits |= 0x02;	// A
	if (x & 0x20)	Cont->Bits |= 0x01;	// B
	if (x & 0x40)	Cont->Bits |= 0x04;	// sel
	if (x & 0x80)	Cont->Bits |= 0x08;	// start
}

static	unsigned char	MovieRead (struct tStdPort *Cont)
{
	unsigned char result;
	if (Cont->Strobe)
		Cont->BitPtr = 0;
	if (Cont->BitPtr < 8)
		result = (unsigned char)(Cont->Bits >> Cont->BitPtr++) & 1;
	else	result = 1;
	return result;
}
static	void	MovieWrite (struct tStdPort *Cont, unsigned char Val)
{
	Cont->Strobe = Val & 1;
	if (Cont->Strobe)
		Cont->BitPtr = 0;
}
static	void	MovieConfig (struct tStdPort *Cont, HWND hWnd)
{
}
static	void	MovieUnload (struct tStdPort *Cont)
{
	fclose((FILE *)Cont->File);
	free(Cont->Data);
}
void	StdPort_SetMovie (struct tStdPort *Cont)
{
	Cont->Read = MovieRead;
	Cont->Write = MovieWrite;
	Cont->Config = MovieConfig;
	Cont->Unload = MovieUnload;
	Cont->NumButtons = 0;
	Cont->DataLen = 3;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data));
	Cont->Bits = 0;
	Cont->BitPtr = 0;
	Cont->Strobe = 0;
	if (Cont == &Controllers.Port2 && Controllers.Port1.Type == 0x80)
		Cont->File = Controllers.Port1.File;
	else
	{
//		(FILE *)Cont->File = fopen("m:\\nes\\famtasia\\movies\\gimmick!.fmv","rb");
		(FILE *)Cont->File = fopen("m:\\nes\\famtasia\\movies\\gradius.fmv","rb");
		fseek((FILE *)Cont->File,0x90,SEEK_SET);
	}
}
#undef	File
#undef	Bits
#undef	BitPtr
#undef	Strobe
#endif

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
	case STD_UNCONNECTED:		StdPort_SetUnconnected(Cont);		break;
	case STD_STDCONTROLLER:		StdPort_SetStdController(Cont);		break;
	case STD_ZAPPER:		StdPort_SetZapper(Cont);		break;
	case STD_ARKANOIDPADDLE:	StdPort_SetArkanoidPaddle(Cont);	break;
	case STD_POWERPAD:		StdPort_SetPowerPad(Cont);		break;
	case STD_FOURSCORE:		StdPort_SetFourScore(Cont);		break;
#ifdef	FMV_PLAYBACK
	case 0x80:			StdPort_SetMovie(Cont);			break;
#endif
	default:MessageBox(mWnd,"Error: selected invalid controller type for standard port!","Nintendulator",MB_OK | MB_ICONERROR);	break;
	}
}
char	*StdPort_Mappings[STD_MAX];
void	StdPort_SetMappings (void)
{
	StdPort_Mappings[STD_UNCONNECTED] = "Unconnected";
	StdPort_Mappings[STD_STDCONTROLLER] = "Standard Controller";
	StdPort_Mappings[STD_ZAPPER] = "Zapper";
	StdPort_Mappings[STD_ARKANOIDPADDLE] = "Arkanoid Paddle";
	StdPort_Mappings[STD_POWERPAD] = "Power Pad";
	StdPort_Mappings[STD_FOURSCORE] = "Four Score";
}

void	ExpPort_SetUnconnected		(struct tExpPort *);
void	ExpPort_SetFami4Play		(struct tExpPort *);
void	ExpPort_SetArkanoidPaddle	(struct tExpPort *);
void	ExpPort_SetFamilyBasicKeyboard	(struct tExpPort *);
void	ExpPort_SetAltKeyboard		(struct tExpPort *);
void	ExpPort_SetFamTrainer		(struct tExpPort *);

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
	default:MessageBox(mWnd,"Error: selected invalid controller type for expansion port!","Nintendulator",MB_OK | MB_ICONERROR);	break;
	}
}
char	*ExpPort_Mappings[EXP_MAX];
void	ExpPort_SetMappings (void)
{
	ExpPort_Mappings[EXP_UNCONNECTED] = "Unconnected";
	ExpPort_Mappings[EXP_FAMI4PLAY] = "Famicom 4-Player Adapter";
	ExpPort_Mappings[EXP_ARKANOIDPADDLE] = "Famicom Arkanoid Paddle";
	ExpPort_Mappings[EXP_FAMILYBASICKEYBOARD] = "Family Basic Keyboard";
	ExpPort_Mappings[EXP_ALTKEYBOARD] = "Alternate Keyboard";
	ExpPort_Mappings[EXP_FAMTRAINER] = "Family Trainer";
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
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		switch (wmId)
		{
		case IDOK:
			EndDialog(hDlg,1);
			break;
		case IDC_CONT_SPORT1:	if (wmEvent == CBN_SELCHANGE)
					{
						int Type = (int)SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_GETCURSEL,0,0);
						if (Type == STD_FOURSCORE)
						{
							StdPort_SetControllerType(&Controllers.FSPort1,Controllers.Port1.Type);
							StdPort_SetControllerType(&Controllers.FSPort2,Controllers.Port2.Type);
							memcpy(Controllers.FSPort1.Buttons,Controllers.Port1.Buttons,sizeof(Controllers.Port1.Buttons));
							memcpy(Controllers.FSPort2.Buttons,Controllers.Port2.Buttons,sizeof(Controllers.Port2.Buttons));
							StdPort_SetControllerType(&Controllers.Port2,STD_FOURSCORE);
							SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,STD_FOURSCORE,0);
						}
						else if (Controllers.Port1.Type	== STD_FOURSCORE)
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
						int Type = (int)SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_GETCURSEL,0,0);
						if (Type == STD_FOURSCORE)
						{
							StdPort_SetControllerType(&Controllers.FSPort1,Controllers.Port1.Type);
							StdPort_SetControllerType(&Controllers.FSPort2,Controllers.Port2.Type);
							memcpy(Controllers.FSPort1.Buttons,Controllers.Port1.Buttons,sizeof(Controllers.Port1.Buttons));
							memcpy(Controllers.FSPort2.Buttons,Controllers.Port2.Buttons,sizeof(Controllers.Port2.Buttons));
							StdPort_SetControllerType(&Controllers.Port1,STD_FOURSCORE);
							SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,STD_FOURSCORE,0);
						}
						else if (Controllers.Port2.Type	== STD_FOURSCORE)
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
		case IDC_CONT_SEXPPORT:	if (wmEvent == CBN_SELCHANGE) { ExpPort_SetControllerType(&Controllers.ExpPort,(int)SendDlgItemMessage(hDlg,IDC_CONT_SEXPPORT,CB_GETCURSEL,0,0)); }	break;

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
		Controllers.NumAxes[Controllers.NumDevices] = tpc.dwAxes;
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
	Controllers.NumAxes[0] = 0;
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
	Controllers.NumAxes[1] = tpc.dwAxes;
	
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
#ifdef	FMV_PLAYBACK
	if (Controllers.Port1.Type == 0x80)
		Movie_UpdateCont(&Controllers.Port1);
	if (Controllers.Port2.Type == 0x80)
		Movie_UpdateCont(&Controllers.Port2);
#endif
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
		}
		for (i = 0; i < Controllers.NumButtons[0]; i++)
		{
			if (Controllers_IsPressed(i))
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
		}
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
		for (i = 0x08; i < (0x08 | Controllers.NumAxes[1]); i++)
		{
			if (Controllers_IsPressed((1 << 16) | i))
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
			if (Controllers_IsPressed(((Dev+2) << 16) | i))
			{
				Key = i;
				break;
			}
		}
		if (Key != -1)	/* if we got a button, don't check for an axis */
			break;
		for (i = 0x80; i < (0x80 | Controllers.NumAxes[Dev+2]); i++)
		{
			if (Controllers_IsPressed(((Dev+2) << 16) | i))
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

char *	Controllers_GetButtonLabel (int DevNum, int Button)
{
	static char str[256];
	char *axes[16] = {"X-Axis","Y-Axis","Z-Axis","rX-Axis","rY-Axis","rZ-Axis","Slider 1","Slider 2"};
	if (DevNum == 0)
		return KeyLookup[Button];
	else if (DevNum == 1)
	{
		if (Button & 0x08)
		{
			Button &= 0x07;
			if (Button & 1)
				sprintf(str,"%s (+)",axes[Button >> 1]);
			else	sprintf(str,"%s (-)",axes[Button >> 1]);
			return str;
		}
		else
		{
			sprintf(str,"Button %i",Button + 1);
			return str;
		}
	}
	else
	{
		if (Button & 0x80)
		{
			Button &= 0x7F;
			if (Button & 1)
				sprintf(str,"%s (+)",axes[Button >> 1]);
			else	sprintf(str,"%s (-)",axes[Button >> 1]);
			return str;
		}
		else
		{
			sprintf(str,"Button %i",Button + 1);
			return str;
		}
	}
}

void	Controllers_ConfigButton (int *Button, int Device, HWND hDlg, BOOL getKey)
{
	*Button &= 0xFFFF;
	if (getKey)	/* this way, we can just re-label the button */
	{
		HWND key = CreateDialog(hInst,(LPCTSTR)IDD_KEYCONFIG,hDlg,NULL);
		ShowWindow(key,TRUE);	/* FIXME - center this window properly */
		ProcessMessages();	/* let the "Press a key..." dialog display itself */
		if (Device == 0)
			*Button = Controllers_GetConfigKey(key);
		else if (Device == 1)
			*Button = Controllers_GetConfigMouse(key);
		else	*Button = Controllers_GetConfigJoy(key,Device-2);
		ProcessMessages();	/* flush all keypresses - don't want them going back to the parent dialog */
		DestroyWindow(key);	/* close the little window */
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
			LockCursorPos(128,120);	/* if we're detecting mouse movement, lock the cursor in place */
			switch (Button & 0x7)
			{
			case 0x0:	return (Controllers.MouseState.lX < 0) ? TRUE : FALSE;	break;
			case 0x1:	return (Controllers.MouseState.lX > 0) ? TRUE : FALSE;	break;
			case 0x2:	return (Controllers.MouseState.lY < 0) ? TRUE : FALSE;	break;
			case 0x3:	return (Controllers.MouseState.lY > 0) ? TRUE : FALSE;	break;
			case 0x4:	return (Controllers.MouseState.lZ < 0) ? TRUE : FALSE;	break;
			case 0x5:	return (Controllers.MouseState.lZ > 0) ? TRUE : FALSE;	break;
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
			case 0x0:	return (Controllers.JoyState[DevNum-2].lX < 0x4000 && Controllers.JoyState[DevNum-2].lX != 0x0000) ? TRUE : FALSE;	break;
			case 0x1:	return (Controllers.JoyState[DevNum-2].lX > 0xC000 && Controllers.JoyState[DevNum-2].lX != 0xFFFF) ? TRUE : FALSE;	break;
			case 0x2:	return (Controllers.JoyState[DevNum-2].lY < 0x4000 && Controllers.JoyState[DevNum-2].lY != 0x0000) ? TRUE : FALSE;	break;
			case 0x3:	return (Controllers.JoyState[DevNum-2].lY > 0xC000 && Controllers.JoyState[DevNum-2].lY != 0xFFFF) ? TRUE : FALSE;	break;
			case 0x4:	return (Controllers.JoyState[DevNum-2].lZ < 0x4000 && Controllers.JoyState[DevNum-2].lZ != 0x0000) ? TRUE : FALSE;	break;
			case 0x5:	return (Controllers.JoyState[DevNum-2].lZ > 0xC000 && Controllers.JoyState[DevNum-2].lZ != 0xFFFF) ? TRUE : FALSE;	break;
			case 0x6:	return (Controllers.JoyState[DevNum-2].lRx < 0x4000 && Controllers.JoyState[DevNum-2].lRx != 0x0000) ? TRUE : FALSE;	break;
			case 0x7:	return (Controllers.JoyState[DevNum-2].lRx > 0xC000 && Controllers.JoyState[DevNum-2].lRx != 0xFFFF) ? TRUE : FALSE;	break;
			case 0x8:	return (Controllers.JoyState[DevNum-2].lRy < 0x4000 && Controllers.JoyState[DevNum-2].lRy != 0x0000) ? TRUE : FALSE;	break;
			case 0x9:	return (Controllers.JoyState[DevNum-2].lRy > 0xC000 && Controllers.JoyState[DevNum-2].lRy != 0xFFFF) ? TRUE : FALSE;	break;
			case 0xA:	return (Controllers.JoyState[DevNum-2].lRz < 0x4000 && Controllers.JoyState[DevNum-2].lRz != 0x0000) ? TRUE : FALSE;	break;
			case 0xB:	return (Controllers.JoyState[DevNum-2].lRz > 0xC000 && Controllers.JoyState[DevNum-2].lRz != 0xFFFF) ? TRUE : FALSE;	break;
			case 0xC:	return (Controllers.JoyState[DevNum-2].rglSlider[0] < 0x4000 && Controllers.JoyState[DevNum-2].rglSlider[0] != 0x0000) ? TRUE : FALSE;	break;
			case 0xD:	return (Controllers.JoyState[DevNum-2].rglSlider[0] > 0xC000 && Controllers.JoyState[DevNum-2].rglSlider[0] != 0xFFFF) ? TRUE : FALSE;	break;
			case 0xE:	return (Controllers.JoyState[DevNum-2].rglSlider[1] < 0x4000 && Controllers.JoyState[DevNum-2].rglSlider[1] != 0x0000) ? TRUE : FALSE;	break;
			case 0xF:	return (Controllers.JoyState[DevNum-2].rglSlider[1] > 0xC000 && Controllers.JoyState[DevNum-2].rglSlider[1] != 0xFFFF) ? TRUE : FALSE;	break;
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