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

#define	Bits1	Data[0]
#define	Bits2	Data[1]
#define	BitPtr1	Data[2]
#define	BitPtr2	Data[3]
#define	Strobe	Data[4]
static	unsigned char	Read1 (struct tExpPort *Cont)
{
	unsigned char result = 1;
	if (Cont->BitPtr1 < 8)
		result = (unsigned char)(Cont->Bits1 >> Cont->BitPtr1++) & 1;
	return result << 1;
}
static	unsigned char	Read2 (struct tExpPort *Cont)
{
	unsigned char result = 1;
	if (Cont->BitPtr2 < 8)
		result = (unsigned char)(Cont->Bits2 >> Cont->BitPtr2++) & 1;
	return result << 1;
}
static	void	Write (struct tExpPort *Cont, unsigned char Val)
{
	if ((Cont->Strobe == 1) && (!(Val & 1)))
	{
		int i, DevNum;

		Cont->BitPtr1 = 0;
		Cont->BitPtr2 = 0;
		Cont->Bits1 = 0;
		Cont->Bits2 = 0;
		for (i = 0; i < 4; i++)
		{
			DevNum = Cont->Buttons[i] >> 16;
			if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[i] & 0xFF] & 0x80)) ||
			    ((DevNum == 1) && (Controllers.MouseState.rgbButtons[Cont->Buttons[i] & 0x7] & 0x80)) ||
			    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].rgbButtons[Cont->Buttons[i] & 0x7F] & 0x80)))
				Cont->Bits1 |= 1 << i;
		}
		DevNum = Cont->Buttons[4] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[4] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (LockCursorPos(128,120)) && (Controllers.MouseState.lY < 0)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].lY < 0x4000)))
			Cont->Bits1 |= 0x10;
		DevNum = Cont->Buttons[5] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[5] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (LockCursorPos(128,120)) && (Controllers.MouseState.lY > 0)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].lY > 0xC000)))
			Cont->Bits1 |= 0x20;
		DevNum = Cont->Buttons[6] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[6] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (LockCursorPos(128,120)) && (Controllers.MouseState.lX < 0)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].lX < 0x4000)))
			Cont->Bits1 |= 0x40;
		DevNum = Cont->Buttons[7] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[7] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (LockCursorPos(128,120)) && (Controllers.MouseState.lX > 0)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].lX > 0xC000)))
			Cont->Bits1 |= 0x80;

		if ((Cont->Bits1 & 0xC0) == 0xC0)
			Cont->Bits1 &= 0x3F;
		if ((Cont->Bits1 & 0x30) == 0x30)
			Cont->Bits1 &= 0xCF;

		for (i = 8; i < 12; i++)
		{
			DevNum = Cont->Buttons[i] >> 16;
			if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[i] & 0xFF] & 0x80)) ||
			    ((DevNum == 1) && (Controllers.MouseState.rgbButtons[Cont->Buttons[i] & 0x7] & 0x80)) ||
			    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].rgbButtons[Cont->Buttons[i] & 0x7F] & 0x80)))
				Cont->Bits2 |= 1 << (i - 8);
		}
		DevNum = Cont->Buttons[12] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[12] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (LockCursorPos(128,120)) && (Controllers.MouseState.lY < 0)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].lY < 0x4000)))
			Cont->Bits2 |= 0x10;
		DevNum = Cont->Buttons[13] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[13] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (LockCursorPos(128,120)) && (Controllers.MouseState.lY > 0)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].lY > 0xC000)))
			Cont->Bits2 |= 0x20;
		DevNum = Cont->Buttons[14] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[14] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (LockCursorPos(128,120)) && (Controllers.MouseState.lX < 0)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].lX < 0x4000)))
			Cont->Bits2 |= 0x40;
		DevNum = Cont->Buttons[15] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[15] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (LockCursorPos(128,120)) && (Controllers.MouseState.lX > 0)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].lX > 0xC000)))
			Cont->Bits2 |= 0x80;

		if ((Cont->Bits2 & 0xC0) == 0xC0)
			Cont->Bits2 &= 0x3F;
		if ((Cont->Bits2 & 0x30) == 0x30)
			Cont->Bits2 &= 0xCF;
	}
	Cont->Strobe = Val & 1;
}
static	void	UpdateConfigKeys (HWND hDlg, struct tExpPort *Cont)
{
	HWND TPWND[16] =
	{
		GetDlgItem(hDlg,IDC_CONT_K0),
		GetDlgItem(hDlg,IDC_CONT_K1),
		GetDlgItem(hDlg,IDC_CONT_K2),
		GetDlgItem(hDlg,IDC_CONT_K3),
		GetDlgItem(hDlg,IDC_CONT_K4),
		GetDlgItem(hDlg,IDC_CONT_K5),
		GetDlgItem(hDlg,IDC_CONT_K6),
		GetDlgItem(hDlg,IDC_CONT_K7),
		GetDlgItem(hDlg,IDC_CONT_K8),
		GetDlgItem(hDlg,IDC_CONT_K9),
		GetDlgItem(hDlg,IDC_CONT_K10),
		GetDlgItem(hDlg,IDC_CONT_K11),
		GetDlgItem(hDlg,IDC_CONT_K12),
		GetDlgItem(hDlg,IDC_CONT_K13),
		GetDlgItem(hDlg,IDC_CONT_K14),
		GetDlgItem(hDlg,IDC_CONT_K15)
	};
	HWND DevWnd[16] =
	{
		GetDlgItem(hDlg,IDC_CONT_D0),
		GetDlgItem(hDlg,IDC_CONT_D1),
		GetDlgItem(hDlg,IDC_CONT_D2),
		GetDlgItem(hDlg,IDC_CONT_D3),
		GetDlgItem(hDlg,IDC_CONT_D4),
		GetDlgItem(hDlg,IDC_CONT_D5),
		GetDlgItem(hDlg,IDC_CONT_D6),
		GetDlgItem(hDlg,IDC_CONT_D7),
		GetDlgItem(hDlg,IDC_CONT_D8),
		GetDlgItem(hDlg,IDC_CONT_D9),
		GetDlgItem(hDlg,IDC_CONT_D10),
		GetDlgItem(hDlg,IDC_CONT_D11),
		GetDlgItem(hDlg,IDC_CONT_D12),
		GetDlgItem(hDlg,IDC_CONT_D13),
		GetDlgItem(hDlg,IDC_CONT_D14),
		GetDlgItem(hDlg,IDC_CONT_D15)
	};
	int DevNum[16] =
	{
		SendMessage(DevWnd[0],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[1],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[2],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[3],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[4],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[5],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[6],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[7],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[8],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[9],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[10],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[11],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[12],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[13],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[14],CB_GETCURSEL,0,0),
		SendMessage(DevWnd[15],CB_GETCURSEL,0,0)
	};
	char tps[64];
	int bn, cur;

	for (cur = 0; cur < 16; cur++)
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
			if ((cur < 4) || ((cur >= 8) && (cur < 12)))
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
static	void	UpdateConfigDevices (HWND hDlg, struct tExpPort *Cont)
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
	SendDlgItemMessage(hDlg,IDC_CONT_D8,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D9,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D10,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D11,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D12,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D13,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D14,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D15,CB_RESETCONTENT,0,0);

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
		SendDlgItemMessage(hDlg,IDC_CONT_D8,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D9,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D10,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D11,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D12,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D13,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D14,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
		SendDlgItemMessage(hDlg,IDC_CONT_D15,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
	}
	SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_SETCURSEL,Cont->Buttons[0] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D1,CB_SETCURSEL,Cont->Buttons[1] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D2,CB_SETCURSEL,Cont->Buttons[2] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D3,CB_SETCURSEL,Cont->Buttons[3] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D4,CB_SETCURSEL,Cont->Buttons[4] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D5,CB_SETCURSEL,Cont->Buttons[5] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D6,CB_SETCURSEL,Cont->Buttons[6] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D7,CB_SETCURSEL,Cont->Buttons[7] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D8,CB_SETCURSEL,Cont->Buttons[8] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D9,CB_SETCURSEL,Cont->Buttons[9] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D10,CB_SETCURSEL,Cont->Buttons[10] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D11,CB_SETCURSEL,Cont->Buttons[11] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D12,CB_SETCURSEL,Cont->Buttons[12] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D13,CB_SETCURSEL,Cont->Buttons[13] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D14,CB_SETCURSEL,Cont->Buttons[14] >> 16,0);
	SendDlgItemMessage(hDlg,IDC_CONT_D15,CB_SETCURSEL,Cont->Buttons[15] >> 16,0);
	
	UpdateConfigKeys(hDlg,Cont);
}
static	BOOL CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static struct tExpPort *Cont;
	int wmId, wmEvent;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		Cont = (struct tExpPort *)lParam;
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
		case IDC_CONT_D8:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[8] = SendDlgItemMessage(hDlg,IDC_CONT_D8,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D9:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[9] = SendDlgItemMessage(hDlg,IDC_CONT_D9,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D10:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[10] = SendDlgItemMessage(hDlg,IDC_CONT_D10,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D11:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[11] = SendDlgItemMessage(hDlg,IDC_CONT_D11,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D12:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[12] = SendDlgItemMessage(hDlg,IDC_CONT_D12,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D13:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[13] = SendDlgItemMessage(hDlg,IDC_CONT_D13,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D14:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[14] = SendDlgItemMessage(hDlg,IDC_CONT_D14,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;
		case IDC_CONT_D15:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[15] = SendDlgItemMessage(hDlg,IDC_CONT_D15,CB_GETCURSEL,0,0) << 16; UpdateConfigKeys(hDlg,Cont); }	break;

		case IDC_CONT_K0:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[0] = SendDlgItemMessage(hDlg,IDC_CONT_K0,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K1:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[1] = SendDlgItemMessage(hDlg,IDC_CONT_K1,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D1,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K2:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[2] = SendDlgItemMessage(hDlg,IDC_CONT_K2,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D2,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K3:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[3] = SendDlgItemMessage(hDlg,IDC_CONT_K3,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D3,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K4:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[4] = SendDlgItemMessage(hDlg,IDC_CONT_K4,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D4,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K5:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[5] = SendDlgItemMessage(hDlg,IDC_CONT_K5,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D5,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K6:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[6] = SendDlgItemMessage(hDlg,IDC_CONT_K6,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D6,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K7:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[7] = SendDlgItemMessage(hDlg,IDC_CONT_K7,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D7,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K8:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[8] = SendDlgItemMessage(hDlg,IDC_CONT_K8,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D8,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K9:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[9] = SendDlgItemMessage(hDlg,IDC_CONT_K9,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D9,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K10:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[10] = SendDlgItemMessage(hDlg,IDC_CONT_K10,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D10,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K11:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[11] = SendDlgItemMessage(hDlg,IDC_CONT_K11,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D11,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K12:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[12] = SendDlgItemMessage(hDlg,IDC_CONT_K12,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D12,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K13:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[13] = SendDlgItemMessage(hDlg,IDC_CONT_K13,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D13,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K14:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[14] = SendDlgItemMessage(hDlg,IDC_CONT_K14,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D14,CB_GETCURSEL,0,0) << 16); }	break;
		case IDC_CONT_K15:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[15] = SendDlgItemMessage(hDlg,IDC_CONT_K15,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D15,CB_GETCURSEL,0,0) << 16); }	break;
		};
		break;
	}
	return FALSE;
}
static	void	Config (struct tExpPort *Cont, HWND hWnd)
{
	DialogBoxParam(hInst,(LPCTSTR)IDD_EXPPORT_FAMI4PLAY,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tExpPort *Cont)
{
	free(Cont->Data);
}
void	ExpPort_SetFami4Play (struct tExpPort *Cont)
{
	Cont->Read1 = Read1;
	Cont->Read2 = Read2;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->NumButtons = 16;
	Cont->DataLen = 6;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data));
	Cont->Bits1 = 0;
	Cont->Bits2 = 0;
	Cont->BitPtr1 = 0;
	Cont->BitPtr2 = 0;
	Cont->Strobe = 0;
}
#undef	Bits1
#undef	Bits2
#undef	BitPtr1
#undef	BitPtr2
#undef	Strobe
