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
#include "MapperInterface.h"
#include "PPU.h"
#include "GFX.h"

#define	Bits	Data[0]
#define	BitPtr	Data[1]
#define	Strobe	Data[2]
#define	PosX	Data[3]
#define	PosY	Data[4]
#define	Button	Data[5]

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
	unsigned char result;
	if (Cont->Strobe)
	{
		Cont->BitPtr = 0;
		result = (unsigned char)(Cont->Bits & 1);
	}
	else
	{
		if (Cont->BitPtr < 8)
			result = (unsigned char)(Cont->Bits >> Cont->BitPtr++) & 1;
		else	result = 0;
	}
	return result;
}
static	void	Write (struct tStdPort *Cont, unsigned char Val)
{
	int x = Cont->PosX, y = Cont->PosY;

	Cont->Strobe = Val & 1;
	if (!Cont->Strobe)
		return;
		
	Cont->Bits = 0x10;
	Cont->BitPtr = 0;
	if (Cont->Button)
		Cont->Bits |= 0x80;

	if ((x < 0) || (x >= 256) || (y < 0) || (y >= 240))
		return;

	if (PPU.IsRendering && PPU.OnScreen)
	{
		int X, Y;
		int WhiteCount = 0;
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
		if (WhiteCount >= 64)
			Cont->Bits |= 0x40;
	}
}
static	INT_PTR	CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	DialogBoxParam(hInst,(LPCTSTR)IDD_STDPORT_VSZAPPER,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tStdPort *Cont)
{
	free(Cont->Data);
	free(Cont->MovData);
}
void	StdPort_SetVSZapper (struct tStdPort *Cont)
{
	Cont->Read = Read;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->Frame = Frame;
	Cont->NumButtons = 1;
	Cont->DataLen = 6;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data[0]));
	Cont->MovLen = 3;
	Cont->MovData = malloc(Cont->MovLen * sizeof(Cont->MovData[0]));
	ZeroMemory(Cont->MovData,Cont->MovLen);
	Cont->PosX = 0;
	Cont->PosY = 0;
	Cont->Button = 0;
	Cont->Bits = 0x10;
	Cont->BitPtr = 0;
	Cont->Strobe = 0;
	GFX_SetFrameskip(-1);
}
#undef	Button
#undef	PosY
#undef	PosX
#undef	Strobe
#undef	BitPtr
#undef	Bits
