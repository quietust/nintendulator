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
#define	Pos	Data[1]
#define	BitPtr	Data[2]
#define	Strobe	Data[3]
#define	Button	Data[4]
static	unsigned char	Read1 (struct tExpPort *Cont)
{
	if (Cont->Button)
		return 0x02;
	else	return 0;
}

static	unsigned char	Read2 (struct tExpPort *Cont)
{
	unsigned char result = (char)(((Cont->Bits >> Cont->BitPtr++) & 1) << 1);
	if (Cont->BitPtr == 9)
		Cont->BitPtr--;
	return result;
}

static	void	Write (struct tExpPort *Cont, unsigned char Val)
{
	if ((Cont->Strobe == 1) && (!(Val & 1)))
	{
		int DevNum;
		unsigned long x;
		int i;
		LockCursorPos(128,220);
		DevNum = Cont->Buttons[0] >> 16;
		if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[0] & 0xFF] & 0x80)) ||
		    ((DevNum == 1) && (Controllers.MouseState.rgbButtons[Cont->Buttons[0] & 0x7] & 0x80)) ||
		    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].rgbButtons[Cont->Buttons[0] & 0x7F] & 0x80)))
			Cont->Button = 1;
		else	Cont->Button = 0;
		x = Cont->Pos;
		x += Controllers.MouseState.lX;
		if (x < 196)
			x = 196;
		if (x > 484)
			x = 484;
		Cont->Pos = x;
		Cont->Bits = 0;
		x = ~x;
		for (i = 0; i < 9; i++)
		{
			Cont->Bits <<= 1;
			Cont->Bits |= x & 1;
			x >>= 1;
		}
		Cont->BitPtr = 0;
	}
	Cont->Strobe = Val & 1;
}
static	void	UpdateConfigKeys (HWND hDlg, struct tExpPort *Cont)
{
	char tps[64];
	int bn;
	int DevNum = SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_GETCURSEL,0,0);

	SendDlgItemMessage(hDlg,IDC_CONT_K0,CB_RESETCONTENT,0,0);
	if (DevNum == 0)
	{
		for (bn = 0; bn < 256; bn++)
			SendDlgItemMessage(hDlg,IDC_CONT_K0,CB_ADDSTRING,0,(LPARAM)KeyLookup[bn]);
		SendDlgItemMessage(hDlg,IDC_CONT_K0,CB_SETCURSEL,Cont->Buttons[0] & 0xFF,0);
	}
	else
	{
		for (bn = 0; bn < Controllers.NumButtons[DevNum]; bn++)
		{
			sprintf(tps,"Button %i",bn+1);
			SendDlgItemMessage(hDlg,IDC_CONT_K0,CB_ADDSTRING,0,(LPARAM)tps);
		}
		SendDlgItemMessage(hDlg,IDC_CONT_K0,CB_SETCURSEL,Cont->Buttons[0] & 0xFFFF,0);
	}
}
static	void	UpdateConfigDevices (HWND hDlg, struct tExpPort *Cont)
{
	int i;
	
	SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_RESETCONTENT,0,0);

	for (i = 0; i < Controllers.NumDevices; i++)
		SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_ADDSTRING,0,(LPARAM)Controllers.DeviceName[i]);
	SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_SETCURSEL,Cont->Buttons[0] >> 16,0);
	
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
		case IDC_CONT_K0:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[0] = SendDlgItemMessage(hDlg,IDC_CONT_K0,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_GETCURSEL,0,0) << 16); }	break;
		};
		break;
	}
	return FALSE;
}
static	void	Config (struct tExpPort *Cont, HWND hWnd)
{
	DialogBoxParam(hInst,(LPCTSTR)IDD_EXPPORT_ARKANOIDPADDLE,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tExpPort *Cont)
{
	free(Cont->Data);
}
void	ExpPort_SetArkanoidPaddle (struct tExpPort *Cont)
{
	Cont->Read1 = Read1;
	Cont->Read2 = Read2;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->NumButtons = 1;
	Cont->DataLen = 5;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data));
	Cont->Bits = 0;
	Cont->Pos = 340;
	Cont->BitPtr = 0;
	Cont->Strobe = 0;
	Cont->Button = 0;
}
#undef	Bits
#undef	Pos
#undef	BitPtr
#undef	Strobe
#undef	Button
