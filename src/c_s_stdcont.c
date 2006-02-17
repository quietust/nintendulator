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

#define	Bits	Data[0]
#define	BitPtr	Data[1]
#define	Strobe	Data[2]
static	unsigned char	Read (struct tStdPort *Cont)
{
	unsigned char result = 1;
	if (Cont->BitPtr < 8)
		result = (unsigned char)(Cont->Bits >> Cont->BitPtr++) & 1;
	return result;
}
static	void	Write (struct tStdPort *Cont, unsigned char Val)
{
	if ((Cont->Strobe == 1) && (!(Val & 1)))
	{
		int i, DevNum;

		Cont->BitPtr = 0;
		Cont->Bits = 0;
		for (i = 0; i < 4; i++)
		{
			DevNum = Cont->Buttons[i] >> 16;
			if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[i] & 0xFF] & 0x80)) ||
			    ((DevNum == 1) && (Controllers.MouseState.rgbButtons[Cont->Buttons[i] & 0x7] & 0x80)) ||
			    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].rgbButtons[Cont->Buttons[i] & 0x7F] & 0x80)))
				Cont->Bits |= 1 << i;
		}
		DevNum = Cont->Buttons[4] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[4] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (LockCursorPos(128,120)) && (Controllers.MouseState.lY < 0)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].lY < 0x4000)))
			Cont->Bits |= 0x10;
		DevNum = Cont->Buttons[5] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[5] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (LockCursorPos(128,120)) && (Controllers.MouseState.lY > 0)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].lY > 0xC000)))
			Cont->Bits |= 0x20;
		DevNum = Cont->Buttons[6] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[6] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (LockCursorPos(128,120)) && (Controllers.MouseState.lX < 0)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].lX < 0x4000)))
			Cont->Bits |= 0x40;
		DevNum = Cont->Buttons[7] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[7] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (LockCursorPos(128,120)) && (Controllers.MouseState.lX > 0)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].lX > 0xC000)))
			Cont->Bits |= 0x80;

		if ((Cont->Bits & 0xC0) == 0xC0)
			Cont->Bits &= 0x3F;
		if ((Cont->Bits & 0x30) == 0x30)
			Cont->Bits &= 0xCF;
	}
	Cont->Strobe = Val & 1;
}
static	void	UpdateConfigKeys (HWND hDlg, struct tStdPort *Cont)
{
	HWND TPWND[8] =
	{
		GetDlgItem(hDlg,IDC_CONT_K0),
		GetDlgItem(hDlg,IDC_CONT_K1),
		GetDlgItem(hDlg,IDC_CONT_K2),
		GetDlgItem(hDlg,IDC_CONT_K3),
		GetDlgItem(hDlg,IDC_CONT_K4),
		GetDlgItem(hDlg,IDC_CONT_K5),
		GetDlgItem(hDlg,IDC_CONT_K6),
		GetDlgItem(hDlg,IDC_CONT_K7)
	};
	HWND DevWnd[8] =
	{
		GetDlgItem(hDlg,IDC_CONT_D0),
		GetDlgItem(hDlg,IDC_CONT_D1),
		GetDlgItem(hDlg,IDC_CONT_D2),
		GetDlgItem(hDlg,IDC_CONT_D3),
		GetDlgItem(hDlg,IDC_CONT_D4),
		GetDlgItem(hDlg,IDC_CONT_D5),
		GetDlgItem(hDlg,IDC_CONT_D6),
		GetDlgItem(hDlg,IDC_CONT_D7)
	};
	int DevNum[8] =
	{
		SendMessage(DevWnd[0],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[1],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[2],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[3],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[4],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[5],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[6],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[7],CB_GETCURSEL,0,0)
	};
	char tps[64];
	int bn, cur;

	for (cur = 0; cur < 8; cur++)
	{
		SendMessage(TPWND[cur],CB_RESETCONTENT,0,0);
		if (DevNum[cur] == 0)
		{
			for (bn = 0; bn < 256; bn++)
				SendMessage(TPWND[cur],CB_ADDSTRING,0,(LPARAM)KeyLookup[bn]);
			SendMessage(TPWND[cur],CB_SETCURSEL,Cont->Buttons[cur] & 0xFF,0);
		}
		else
		{
			if (cur < 4)
			{
				for (bn = 0; bn < Controllers.NumButtons[DevNum[cur]]; bn++)
				{
					sprintf(tps,"Button %i",bn+1);
					SendMessage(TPWND[cur],CB_ADDSTRING,0,(LPARAM)tps);
				}
			}
			else	SendMessage(TPWND[cur],CB_ADDSTRING,0,(LPARAM)"(Main Axis)");
			SendMessage(TPWND[cur],CB_SETCURSEL,Cont->Buttons[cur] & 0xFFFF,0);
		}
	}
}
static	void	UpdateConfigDevices (HWND hDlg, struct tStdPort *Cont)
{
	int i;
	
	SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D1,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D2,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D3,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D4,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D5,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D6,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D7,CB_RESETCONTENT,0,0);

	for (i = 0; i < Controllers.NumDevices; i++)
	{
		SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D1,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D2,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D3,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D4,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D5,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D6,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D7,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
	}
	SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_SETCURSEL,Cont->Buttons[0] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D1,CB_SETCURSEL,Cont->Buttons[1] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D2,CB_SETCURSEL,Cont->Buttons[2] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D3,CB_SETCURSEL,Cont->Buttons[3] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D4,CB_SETCURSEL,Cont->Buttons[4] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D5,CB_SETCURSEL,Cont->Buttons[5] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D6,CB_SETCURSEL,Cont->Buttons[6] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D7,CB_SETCURSEL,Cont->Buttons[7] >> 16,0);
	
	UpdateConfigKeys(hDlg,Cont);
}
static	BOOL CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static struct tStdPort *Cont;
	int wmId, wmEvent;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		Cont = (struct tStdPort *)lParam;
		UpdateConfigDevices(hDlg,Cont);
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
		case IDC_CONT_D0:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[0] = SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D1:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[1] = SendDlgItemMessage(hDlg,IDC_CONT_D1,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D2:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[2] = SendDlgItemMessage(hDlg,IDC_CONT_D2,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D3:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[3] = SendDlgItemMessage(hDlg,IDC_CONT_D3,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D4:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[4] = SendDlgItemMessage(hDlg,IDC_CONT_D4,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D5:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[5] = SendDlgItemMessage(hDlg,IDC_CONT_D5,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D6:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[6] = SendDlgItemMessage(hDlg,IDC_CONT_D6,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D7:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[7] = SendDlgItemMessage(hDlg,IDC_CONT_D7,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_K0:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[0] = SendDlgItemMessage(hDlg,IDC_CONT_K0,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K1:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[1] = SendDlgItemMessage(hDlg,IDC_CONT_K1,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D1,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K2:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[2] = SendDlgItemMessage(hDlg,IDC_CONT_K2,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D2,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K3:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[3] = SendDlgItemMessage(hDlg,IDC_CONT_K3,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D3,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K4:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[4] = SendDlgItemMessage(hDlg,IDC_CONT_K4,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D4,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K5:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[5] = SendDlgItemMessage(hDlg,IDC_CONT_K5,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D5,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K6:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[6] = SendDlgItemMessage(hDlg,IDC_CONT_K6,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D6,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K7:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[7] = SendDlgItemMessage(hDlg,IDC_CONT_K7,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D7,CB_GETCURSEL,0,0) << 16); }	break;
		};
		break;
	}
	return FALSE;
}
static	void	Config (struct tStdPort *Cont, HWND hWnd)
{
	DialogBoxParam(hInst,(LPCTSTR)IDD_STDPORT_STDCONTROLLER,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tStdPort *Cont)
{
	free(Cont->Data);
}
void	StdPort_SetStdController (struct tStdPort *Cont)
{
	Cont->Read = Read;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->NumButtons = 8;
	Cont->DataLen = 3;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data));
	Cont->Bits = 0;
	Cont->BitPtr = 0;
	Cont->Strobe = 0;
}
#undef	Bits
#undef	BitPtr
#undef	Strobe