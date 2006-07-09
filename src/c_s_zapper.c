/*
Nintendulator - A Win32 NES emulator written in C.
Designed for maximum emulation accuracy.
Copyright (c) 2002-2006  QMT Productions

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

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "Movie.h"
#include "Controllers.h"
#include "MapperInterface.h"
#include "PPU.h"
#include "GFX.h"

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
		GFX_SetCursorPos(Cont->PosX, Cont->PosY);
	}
	else
	{
		GFX_GetCursorPos(&pos);
		Cont->PosX = pos.x;
		Cont->PosY = pos.y;
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
	int WhiteCount = 0;
	unsigned char Bits = 0x00;
	int X, Y;

	if (z)
		Bits |= 0x10;

	if ((x < 0) || (x >= 256) || (y < 0) || (y >= 240))
		return Bits | 0x08;

	if (PPU.IsRendering && PPU.OnScreen)
		for (Y = y - 8; Y < y + 8; Y++)
		{
			if (Y < 0)
				Y = 0;
			if (Y < PPU.SLnum - 32)
				continue;
			if (Y >= PPU.SLnum)
				break;
			for (X = x - 8; X < x + 8; X++)
			{
				if (X < 0)
					X = 0;
				if (X > 255)
					break;
				if (GFX_ZapperHit(DrawArray[Y * 256 + X]))
					WhiteCount++;
			}
		}
	if (WhiteCount < 64)
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
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data[0]));
	Cont->MovLen = 3;
	Cont->MovData = malloc(Cont->MovLen * sizeof(Cont->MovData[0]));
	ZeroMemory(Cont->MovData,Cont->MovLen);
	Cont->PosX = 0;
	Cont->PosY = 0;
	Cont->Button = 0;
	GFX_SetFrameskip(-1);
}
#undef	Button
#undef	PosY
#undef	PosX
