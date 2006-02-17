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
#define	Framed	Data[5]
static	void	Frame (struct tStdPort *Cont)
{
	Cont->Framed = 0;
}
static	void	UpdateCont (struct tStdPort *Cont)
{
	int x, y;
	int i;
	if (Controllers.MovieMode & MOV_PLAY)
	{
		x = Cont->MovData[0] | (Cont->MovData[1] << 8);
		y = Cont->MovData[2];
	}
	else
	{
		LockCursorPos(128,220);
		y = Controllers_IsPressed(Cont->Buttons[0]);
		x = Cont->Pos;
		if (!Cont->Framed)
			x += Controllers.MouseState.lX;
		if (x < 196)
			x = 196;
		if (x > 484)
			x = 484;
	}
	if (Controllers.MovieMode & MOV_RECORD)
	{
		Cont->MovData[0] = x & 0xFF;
		Cont->MovData[1] = x >> 8;
		Cont->MovData[2] = y;

	}
	Cont->Framed = 1;
	if (y)
		Cont->Button = 1;
	else	Cont->Button = 0;
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
static	unsigned char	Read (struct tStdPort *Cont)
{
	unsigned char result;
	if (Cont->Strobe)
		UpdateCont(Cont);
	result = (char)((Cont->Bits >> Cont->BitPtr++) & 1);
	if (Cont->BitPtr == 9)
		Cont->BitPtr--;
	result <<= 4;
	if (Cont->Button)
		result |= 0x08;
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
	int dlgLists[1] = {IDC_CONT_D0};
	int dlgButtons[1] = {IDC_CONT_K0};
	static struct tStdPort *Cont = NULL;
	if (uMsg == WM_INITDIALOG)
		Cont = (struct tStdPort *)lParam;
	Controllers_ParseConfigMessages(hDlg,1,dlgLists,dlgButtons,Cont->Buttons,uMsg,wParam,lParam);
	return FALSE;
}
static	void	Config (struct tStdPort *Cont, HWND hWnd)
{
	DialogBoxParam(hInst,(LPCTSTR)IDD_STDPORT_ARKANOIDPADDLE,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tStdPort *Cont)
{
	free(Cont->Data);
	free(Cont->MovData);
}
void	StdPort_SetArkanoidPaddle (struct tStdPort *Cont)
{
	Cont->Read = Read;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->Frame = Frame;
	Cont->NumButtons = 1;
	Cont->DataLen = 6;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data));
	Cont->MovLen = 3;
	Cont->MovData = malloc(Cont->MovLen * sizeof(Cont->MovData));
	Cont->Bits = 0;
	Cont->Pos = 340;
	Cont->BitPtr = 0;
	Cont->Strobe = 0;
	Cont->Button = 0;
	Cont->Framed = 0;
}
#undef	Bits
#undef	Pos
#undef	BitPtr
#undef	Strobe
#undef	Button
