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

#define	LastPix	Data[0]
static	unsigned char	Read (struct tStdPort *Cont)
{
	int DevNum;
	int x, y;
	long CurPix = 0;
	int Delta;
	POINT pos;
	unsigned char Bits = 0x00;

	GetCursorPos(&pos);
	ScreenToClient(mWnd,&pos);
	x = pos.x / SizeMult;
	y = pos.y / SizeMult;
	if ((x < 0) || (x >= 256) || (y < 0) || (y >= 240))
		return Bits;

	DevNum = Cont->Buttons[0] >> 16;
	if (((DevNum == 0) && (Controllers.KeyState[Cont->Buttons[0] & 0xFF] & 0x80)) ||
	    ((DevNum == 1) && (Controllers.MouseState.rgbButtons[Cont->Buttons[0] & 0x7] & 0x80)) ||
	    ((DevNum >= 2) && (Controllers.JoyState[DevNum-2].rgbButtons[Cont->Buttons[0] & 0x7F] & 0x80)))
		Bits |= 0x10;

	switch (GFX.Depth)
	{
	case 15:CurPix = ((unsigned short *)((char *)GFX.DrawArray + y*GFX.Pitch))[x];
		CurPix = ((CurPix & 0x1F) << 3) | ((CurPix & 0x3E0) << 6) | ((CurPix & 0x7C00) << 9) | 0x070707;break;
	case 16:CurPix = ((unsigned short *)((char *)GFX.DrawArray + y*GFX.Pitch))[x];
		CurPix = ((CurPix & 0x1F) << 3) | ((CurPix & 0x7E0) << 5) | ((CurPix & 0xF800) << 8) | 0x070307;break;
	case 32:CurPix = ((long *)((char *)GFX.DrawArray + y*GFX.Pitch))[x];					break;
	}
/*	Delta = (int)((((signed)(CurPix & 0xFF) - (signed)(Cont->LastPix & 0xFF)) / 3.36) +
		(((signed)((CurPix >> 8) & 0xFF) - (signed)((Cont->LastPix >> 8) & 0xFF)) / 1.7) +
		(((signed)((CurPix >> 16) & 0xFF) - (signed)((Cont->LastPix >> 16) & 0xFF)) / 9.1));*/
	Delta = (int)(((signed)(CurPix & 0xFF) / 3.36) +
		(((signed)(CurPix >> 8) & 0xFF) / 1.7) +
		(((signed)(CurPix >> 16) & 0xFF) / 9.1));
/*	if (Delta < 0xF0)*/
	if (CurPix != 0xFFFFFF)
		Bits |= 0x08;
	Cont->LastPix = CurPix;
	return Bits;
}
static	void	Write (struct tStdPort *Cont, unsigned char Val)
{
}
static	void	UpdateConfigKeys (HWND hDlg, struct tStdPort *Cont)
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
static	void	UpdateConfigDevices (HWND hDlg, struct tStdPort *Cont)
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
		case IDC_CONT_K0:	if (wmEvent == CBN_SELCHANGE) { Cont->Buttons[0] = SendDlgItemMessage(hDlg,IDC_CONT_K0,CB_GETCURSEL,0,0) | (SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_GETCURSEL,0,0) << 16); }	break;
//		case IDC_CONT_K0:	Controllers_ConfigButton(&Cont->Buttons[0],SendDlgItemMessage(hDlg,IDC_CONT_D0,CB_GETCURSEL,0,0),hDlg,IDC_CONT_K0);	break;
		};
		break;
	}
	return FALSE;
}
static	void	Config (struct tStdPort *Cont, HWND hWnd)
{
	DialogBoxParam(hInst,(LPCTSTR)IDD_STDPORT_ZAPPER,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tStdPort *Cont)
{
	free(Cont->Data);
}
void	StdPort_SetZapper (struct tStdPort *Cont)
{
	Cont->Read = Read;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->NumButtons = 1;
	Cont->DataLen = 1;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data));
	Cont->LastPix = 0;
}
#undef	LastPix
