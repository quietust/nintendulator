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

#define	Bits	Data[0]
#define	BitPtr	Data[1]
#define	Strobe	Data[2]

static	void	Frame (struct tStdPort *Cont)
{
	if ((Controllers.MovieMode & MOV_PLAY) && (Controllers.MovieMode & MOV_FMV))
	{	// shuffle FMV data 
		unsigned char x = Cont->MovData[0], y = 0;
		if (x & 0x01)	y |= 0x80;	// right
		if (x & 0x02)	y |= 0x40;	// left
		if (x & 0x04)	y |= 0x10;	// up
		if (x & 0x08)	y |= 0x20;	// down
		if (x & 0x10)	y |= 0x02;	// A
		if (x & 0x20)	y |= 0x01;	// B
		if (x & 0x40)	y |= 0x04;	// sel
		if (x & 0x80)	y |= 0x08;	// start
		Cont->MovData[0] = y;
	}
}

static	void	UpdateCont (struct tStdPort *Cont)
{
	int i;

	if (Controllers.MovieMode & MOV_PLAY)
		Cont->Bits = Cont->MovData[0];
	else
	{
		Cont->Bits = 0;
		for (i = 0; i < 8; i++)
		{
			if (Controllers_IsPressed(Cont->Buttons[i]))
				Cont->Bits |= 1 << i;
		}
		if (!Controllers.MovieMode & MOV_RECORD)
		{	/* prevent simultaneously pressing left+right or up+down, but not when recording a movie :) */
			if ((Cont->Bits & 0xC0) == 0xC0)
				Cont->Bits &= 0x3F;
			if ((Cont->Bits & 0x30) == 0x30)
				Cont->Bits &= 0xCF;
		}
	}
	if (Controllers.MovieMode & MOV_RECORD)
		Cont->MovData[0] = Cont->Bits;

	Cont->BitPtr = 0;
}

static	unsigned char	Read (struct tStdPort *Cont)
{
	unsigned char result;
	if (Cont->Strobe)
		UpdateCont(Cont);
	if (Cont->BitPtr < 8)
		result = (unsigned char)(Cont->Bits >> Cont->BitPtr++) & 1;
	else	result = 1;
	return result;
}
static	void	Write (struct tStdPort *Cont, unsigned char Val)
{
	Cont->Strobe = Val & 1;
	if (Cont->Strobe)
		UpdateCont(Cont);
}
static	LRESULT	CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int dlgLists[8] = {IDC_CONT_D0,IDC_CONT_D1,IDC_CONT_D2,IDC_CONT_D3,IDC_CONT_D4,IDC_CONT_D5,IDC_CONT_D6,IDC_CONT_D7};
	int dlgButtons[8] = {IDC_CONT_K0,IDC_CONT_K1,IDC_CONT_K2,IDC_CONT_K3,IDC_CONT_K4,IDC_CONT_K5,IDC_CONT_K6,IDC_CONT_K7};
	static struct tStdPort *Cont = NULL;
	if (uMsg == WM_INITDIALOG)
		Cont = (struct tStdPort *)lParam;
	Controllers_ParseConfigMessages(hDlg,8,dlgLists,dlgButtons,Cont->Buttons,uMsg,wParam,lParam);
	return FALSE;
}
static	void	Config (struct tStdPort *Cont, HWND hWnd)
{
	DialogBoxParam(hInst,(LPCTSTR)IDD_STDPORT_STDCONTROLLER,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tStdPort *Cont)
{
	free(Cont->Data);
	free(Cont->MovData);
}
void	StdPort_SetStdController (struct tStdPort *Cont)
{
	Cont->Read = Read;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->Frame = Frame;
	Cont->NumButtons = 8;
	Cont->DataLen = 3;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data));
	Cont->MovLen = 1;
	Cont->MovData = malloc(Cont->MovLen * sizeof(Cont->MovData));
	ZeroMemory(Cont->MovData,Cont->MovLen);
	Cont->Bits = 0;
	Cont->BitPtr = 0;
	Cont->Strobe = 0;
}
#undef	Bits
#undef	BitPtr
#undef	Strobe