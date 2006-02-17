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
#include "PPU.h"

#define	PosX	Data[0]
#define	PosY	Data[1]
#define	Button	Data[2]
static	void	Frame (struct tStdPort *Cont, unsigned char mode)
{
	static POINT pos;
	if (mode & MOV_PLAY)
	{
		Cont->PosX = Cont->MovData[0];
		Cont->PosY = Cont->MovData[1];
		Cont->Button = Cont->MovData[2];
		pos.x = Cont->PosX * SizeMult;
		pos.y = Cont->PosY * SizeMult;
		ClientToScreen(mWnd,&pos);
		SetCursorPos(pos.x,pos.y);
	}
	else
	{
		GetCursorPos(&pos);
		ScreenToClient(mWnd,&pos);
		Cont->PosX = pos.x / SizeMult;
		Cont->PosY = pos.y / SizeMult;
		if ((Cont->PosX < 0) || (Cont->PosX > 255) || (Cont->PosY < 0) || (Cont->PosY > 239))
			Cont->PosX = Cont->PosY = 255;	// if it's off-screen, push it to the bottom
		Cont->Button = Controllers_IsPressed(Cont->Buttons[0]);
	}
	if (mode & MOV_RECORD)
	{
		Cont->MovData[0] = (unsigned char)Cont->PosX;
		Cont->MovData[1] = (unsigned char)Cont->PosY;
		Cont->MovData[2] = (unsigned char)Cont->Button;
	}
}

static	unsigned char	Read (struct tStdPort *Cont)
{
	int x = Cont->PosX, y = Cont->PosY, z = Cont->Button;
	int CurPix = 0xF;
	unsigned char Bits = 0x00;

	if ((x < 0) || (x >= 256) || (y < 0) || (y >= 240))
		return Bits | 0x08;
	if (z)
		Bits |= 0x10;
	if (y < 240)
		CurPix = DrawArray[y*256 + x] & 0x3F;
	if ((CurPix != 0x20) && (CurPix != 0x30) && (CurPix != 0x3D))
		Bits |= 0x08;
	return Bits;
}
static	void	Write (struct tStdPort *Cont, unsigned char Val)
{
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
	DialogBoxParam(hInst,(LPCTSTR)IDD_STDPORT_ZAPPER,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tStdPort *Cont)
{
	free(Cont->Data);
	free(Cont->MovData);
}
void	StdPort_SetZapper (struct tStdPort *Cont)
{
	Cont->Read = Read;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->Frame = Frame;
	Cont->NumButtons = 1;
	Cont->DataLen = 3;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data));
	Cont->MovLen = 3;
	Cont->MovData = malloc(Cont->MovLen * sizeof(Cont->MovData));
	ZeroMemory(Cont->MovData,Cont->MovLen);
	Cont->PosX = 0;
	Cont->PosY = 0;
	Cont->Button = 0;
}
#undef	Button
#undef	PosY
#undef	PosX
#undef	LastPix
