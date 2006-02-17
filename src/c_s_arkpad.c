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
#include "Movie.h"
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
#define	NewBits	Data[5]
static	void	Frame (struct tStdPort *Cont, unsigned char mode)
{
	int x, i;
	if (mode & MOV_PLAY)
	{
		Cont->Pos = Cont->MovData[0] | ((Cont->MovData[1] << 8) & 0x7F);
		Cont->Button = Cont->MovData[1] >> 7;
	}
	else
	{
		LockCursorPos(128,220);
		Cont->Button = Controllers_IsPressed(Cont->Buttons[0]);
		Cont->Pos += Controllers.MouseState.lX;
		if (Cont->Pos < 196)
			Cont->Pos = 196;
		if (Cont->Pos > 484)
			Cont->Pos = 484;
	}
	if (mode & MOV_RECORD)
	{
		Cont->MovData[0] = (unsigned char)(Cont->Pos & 0xFF);
		Cont->MovData[1] = (unsigned char)((Cont->Pos >> 8) | (Cont->Button << 7));
	}
	Cont->NewBits = 0;
	x = ~Cont->Pos;
	for (i = 0; i < 9; i++)
	{
		Cont->NewBits <<= 1;
		Cont->NewBits |= x & 1;
		x >>= 1;
	}
}
static	unsigned char	Read (struct tStdPort *Cont)
{
	unsigned char result;
	if (Cont->BitPtr < 8)
		result = (char)((Cont->Bits >> Cont->BitPtr++) & 1) << 4;
	else	result = 0x10;
	if (Cont->Button)
		result |= 0x08;
	return result;
}
static	void	Write (struct tStdPort *Cont, unsigned char Val)
{
	if ((!Cont->Strobe) && (Val & 1))
	{
		Cont->Bits = Cont->NewBits;
		Cont->BitPtr = 0;
	}
	Cont->Strobe = Val & 1;
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
	Cont->MovLen = 2;
	Cont->MovData = malloc(Cont->MovLen * sizeof(Cont->MovData));
	ZeroMemory(Cont->MovData,Cont->MovLen);
	Cont->Bits = 0;
	Cont->Pos = 340;
	Cont->BitPtr = 0;
	Cont->Strobe = 0;
	Cont->Button = 0;
	Cont->NewBits = 0;
}
#undef	NewBits
#undef	Button
#undef	Strobe
#undef	BitPtr
#undef	Pos
#undef	Bits
