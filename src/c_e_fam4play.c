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

#define	Bits1	Data[0]
#define	Bits2	Data[1]
#define	BitPtr1	Data[2]
#define	BitPtr2	Data[3]
#define	Strobe	Data[4]
#define	NewBit1	Data[5]
#define	NewBit2	Data[6]
static	void	Frame (struct tExpPort *Cont, unsigned char mode)
{
	int i;
	if (mode & MOV_PLAY)
	{
		Cont->NewBit1 = Cont->MovData[0];
		Cont->NewBit2 = Cont->MovData[1];
	}
	else
	{
		Cont->NewBit1 = 0;
		Cont->NewBit2 = 0;
		for (i = 0; i < 8; i++)
		{
			if (Controllers_IsPressed(Cont->Buttons[i]))
				Cont->NewBit1 |= 1 << i;
			if (Controllers_IsPressed(Cont->Buttons[i+8]))
				Cont->NewBit2 |= 1 << i;
		}
		if (!(Controllers.MovieMode & MOV_RECORD))
		{	/* prevent simultaneously pressing left+right or up+down, but not when recording a movie :) */
			if ((Cont->NewBit1 & 0xC0) == 0xC0)
				Cont->NewBit1 &= 0x3F;
			if ((Cont->NewBit2 & 0xC0) == 0xC0)
				Cont->NewBit2 &= 0x3F;

			if ((Cont->NewBit1 & 0x30) == 0x30)
				Cont->NewBit1 &= 0xCF;
			if ((Cont->NewBit2 & 0x30) == 0x30)
				Cont->NewBit2 &= 0xCF;
		}
	}
	if (Controllers.MovieMode & MOV_RECORD)
	{
		Cont->MovData[0] = (unsigned char)Cont->NewBit1;
		Cont->MovData[1] = (unsigned char)Cont->NewBit2;
	}
}

static	unsigned char	Read1 (struct tExpPort *Cont)
{
	unsigned char result = 1;
	if (Cont->Strobe)
	{
		Cont->Bits1 = Cont->NewBit1;
		Cont->BitPtr1 = 0;
		result = Cont->Bits1 & 1;
	}
	else
	{
		if (Cont->BitPtr1 < 8)
			result = (unsigned char)(Cont->Bits1 >> Cont->BitPtr1++) & 1;
	}
	return result << 1;
}
static	unsigned char	Read2 (struct tExpPort *Cont)
{
	unsigned char result = 1;
	if (Cont->Strobe)
	{
		Cont->Bits2 = Cont->NewBit2;
		Cont->BitPtr2 = 0;
		result = Cont->Bits2 & 1;
	}
	else
	{
		if (Cont->BitPtr2 < 8)
			result = (unsigned char)(Cont->Bits2 >> Cont->BitPtr2++) & 1;
	}
	return result << 1;
}
static	void	Write (struct tExpPort *Cont, unsigned char Val)
{
	if (Cont->Strobe = Val & 1)
	{
		Cont->Bits1 = Cont->NewBit1;
		Cont->Bits2 = Cont->NewBit2;
		Cont->BitPtr1 = 0;
		Cont->BitPtr2 = 0;
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
	Cont->DataLen = 7;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data));
	Cont->MovLen = 2;
	Cont->MovData = malloc(Cont->MovLen * sizeof(Cont->MovData));
	ZeroMemory(Cont->MovData,Cont->MovLen);
	Cont->Bits1 = 0;
	Cont->Bits2 = 0;
	Cont->BitPtr1 = 0;
	Cont->BitPtr2 = 0;
	Cont->Strobe = 0;
	Cont->NewBit1 = 0;
	Cont->NewBit2 = 0;
}
#undef	NewBit2
#undef	NewBit1
#undef	Strobe
#undef	BitPtr2
#undef	BitPtr1
#undef	Bits2
#undef	Bits1
