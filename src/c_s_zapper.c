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
	int x, y;
	long CurPix = 0;
//	int Delta;
	POINT pos;
	unsigned char Bits = 0x00;

	GetCursorPos(&pos);
	ScreenToClient(mWnd,&pos);
	x = pos.x / SizeMult;
	y = pos.y / SizeMult;
	if ((x < 0) || (x >= 256) || (y < 0) || (y >= 240))
		return Bits | 0x08;

	if (Controllers_IsPressed(Cont->Buttons[0]))
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
/*	Delta = (int)(((signed)(CurPix & 0xFF) / 3.36) +
		(((signed)(CurPix >> 8) & 0xFF) / 1.7) +
		(((signed)(CurPix >> 16) & 0xFF) / 9.1));*/
	if ((CurPix != 0xFFFFFF) && (CurPix != 0xC0C0C0))
		Bits |= 0x08;
	Cont->LastPix = CurPix;
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
