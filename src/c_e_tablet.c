/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2008 QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "Movie.h"
#include "Controllers.h"
#include "GFX.h"

#define	Bits	Data[0]
#define	Strobe	Data[1]
#define	BitPtr	Data[2]
#define	PosX	Data[3]
#define	PosY	Data[4]
#define	Button	Data[5]
#define	NewBits	Data[6]
static	void	Frame (struct tExpPort *Cont, unsigned char mode)
{
	int x, y;
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
			Cont->PosX = Cont->PosY = 0;
		Cont->Button = Controllers_IsPressed(Cont->Buttons[0]);
	}
	if (mode & MOV_RECORD)
	{
		Cont->MovData[0] = (unsigned char)Cont->PosX;
		Cont->MovData[1] = (unsigned char)Cont->PosY;
		Cont->MovData[2] = (unsigned char)Cont->Button;
	}
	
	Cont->NewBits = 0;
	if (Cont->Button)
		Cont->NewBits |= 0x0001;
	if (Cont->PosY >= 48)
		Cont->NewBits |= 0x0002;
	else if (Cont->Button)
		Cont->NewBits |= 0x0003;
	x = (Cont->PosX + 8) * 240 / 256;
	y = (Cont->PosY - 14) * 256 / 240;
	if (y < 0) y = 0;
	if (y > 255) y = 255;
	Cont->NewBits |= (x << 10) | (y << 2);
}
static	unsigned char	Read1 (struct tExpPort *Cont)
{
	return 0;
}
static	unsigned char	Read2 (struct tExpPort *Cont)
{
	if (Cont->Strobe & 1)
	{
		if (Cont->Strobe & 2)
		{
			if ((Cont->Bits << Cont->BitPtr) & 0x40000)
				return 0x00;
			else	return 0x08;
		}
		else	return 0x04;
	}
	else	return 0x00;
}
static	void	Write (struct tExpPort *Cont, unsigned char Val)
{
	if (Val & 1)
	{
		if ((~Cont->Strobe) & Val & 2)
			Cont->BitPtr++;
	}
	else
	{
		Cont->Bits = Cont->NewBits;
		Cont->BitPtr = 0;
	}
	Cont->Strobe = Val & 3;
}
static	LRESULT	CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int dlgLists[1] = {IDC_CONT_D0};
	int dlgButtons[1] = {IDC_CONT_K0};
	static struct tExpPort *Cont = NULL;
	if (uMsg == WM_INITDIALOG)
		Cont = (struct tExpPort *)lParam;
	Controllers_ParseConfigMessages(hDlg,1,dlgLists,dlgButtons,Cont->Buttons,uMsg,wParam,lParam);
	return FALSE;
}
static	void	Config (struct tExpPort *Cont, HWND hWnd)
{
	DialogBoxParam(hInst,(LPCTSTR)IDD_EXPPORT_TABLET,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tExpPort *Cont)
{
	free(Cont->Data);
	free(Cont->MovData);
}
void	ExpPort_SetTablet (struct tExpPort *Cont)
{
	Cont->Read1 = Read1;
	Cont->Read2 = Read2;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->Frame = Frame;
	Cont->NumButtons = 1;
	Cont->DataLen = 7;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data[0]));
	Cont->MovLen = 3;
	Cont->MovData = malloc(Cont->MovLen * sizeof(Cont->MovData[0]));
	ZeroMemory(Cont->MovData,Cont->MovLen);
	Cont->Bits = 0;
	Cont->Strobe = 0;
	Cont->BitPtr = 0;
	Cont->PosX = 0;
	Cont->PosY = 0;
	Cont->Button = 0;
	Cont->NewBits = 0;
}
#undef	NewBits
#undef	Button
#undef	PosY
#undef	PosX
#undef	BitPtr
#undef	Strobe
#undef	Bits