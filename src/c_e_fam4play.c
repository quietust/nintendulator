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
static	void	Frame (struct tExpPort *Cont)
{
}

static	void	UpdateCont1 (struct tExpPort *Cont)
{
	int i;

	Cont->Bits1 = 0;
	for (i = 0; i < 8; i++)
	{
		if (Controllers_IsPressed(Cont->Buttons[i]))
			Cont->Bits1 |= 1 << i;
	}

	if ((Cont->Bits1 & 0xC0) == 0xC0)
		Cont->Bits1 &= 0x3F;
	if ((Cont->Bits1 & 0x30) == 0x30)
		Cont->Bits1 &= 0xCF;
	Cont->BitPtr1 = 0;
}
static	void	UpdateCont2 (struct tExpPort *Cont)
{
	int i;

	Cont->Bits2 = 0;

	for (i = 8; i < 16; i++)
	{
		if (Controllers_IsPressed(Cont->Buttons[i]))
			Cont->Bits2 |= 1 << (i - 8);
	}

	if ((Cont->Bits2 & 0xC0) == 0xC0)
		Cont->Bits2 &= 0x3F;
	if ((Cont->Bits2 & 0x30) == 0x30)
		Cont->Bits2 &= 0xCF;
	Cont->BitPtr2 = 0;
}
static	unsigned char	Read1 (struct tExpPort *Cont)
{
	unsigned char result = 1;
	if (Cont->Strobe)
		UpdateCont1(Cont);
	if (Cont->BitPtr1 < 8)
		result = (unsigned char)(Cont->Bits1 >> Cont->BitPtr1++) & 1;
	return result << 1;
}
static	unsigned char	Read2 (struct tExpPort *Cont)
{
	unsigned char result = 1;
	if (Cont->Strobe)
		UpdateCont2(Cont);
	if (Cont->BitPtr2 < 8)
		result = (unsigned char)(Cont->Bits2 >> Cont->BitPtr2++) & 1;
	return result << 1;
}
static	void	Write (struct tExpPort *Cont, unsigned char Val)
{
	Cont->Strobe = Val & 1;
	if (Cont->Strobe)
	{
		UpdateCont1(Cont);
		UpdateCont2(Cont);
	}
}
static	LRESULT	CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int dlgLists[16] = {IDC_CONT_D0,IDC_CONT_D1,IDC_CONT_D2,IDC_CONT_D3,IDC_CONT_D4,IDC_CONT_D5,IDC_CONT_D6,IDC_CONT_D7,IDC_CONT_D8,IDC_CONT_D9,IDC_CONT_D10,IDC_CONT_D11,IDC_CONT_D12,IDC_CONT_D13,IDC_CONT_D14,IDC_CONT_D15};
	int dlgButtons[16] = {IDC_CONT_K0,IDC_CONT_K1,IDC_CONT_K2,IDC_CONT_K3,IDC_CONT_K4,IDC_CONT_K5,IDC_CONT_K6,IDC_CONT_K7,IDC_CONT_K8,IDC_CONT_K9,IDC_CONT_K10,IDC_CONT_K11,IDC_CONT_K12,IDC_CONT_K13,IDC_CONT_K14,IDC_CONT_K15};
	static struct tExpPort *Cont = NULL;
	if (uMsg == WM_INITDIALOG)
		Cont = (struct tExpPort *)lParam;
	Controllers_ParseConfigMessages(hDlg,16,dlgLists,dlgButtons,Cont->Buttons,uMsg,wParam,lParam);
	return FALSE;
}
static	void	Config (struct tExpPort *Cont, HWND hWnd)
{
	DialogBoxParam(hInst,(LPCTSTR)IDD_EXPPORT_FAMI4PLAY,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tExpPort *Cont)
{
	free(Cont->Data);
	free(Cont->MovData);
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
	Cont->MovLen = 0;
	Cont->MovData = malloc(Cont->MovLen * sizeof(Cont->MovData));
	ZeroMemory(Cont->MovData,Cont->MovLen);
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
