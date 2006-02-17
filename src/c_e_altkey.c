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

#define	Out	Data[0]
#define	Scan	Data[1]
static	void	Frame (struct tExpPort *Cont, unsigned char mode)
{
	if (mode & MOV_RECORD)
	{
		MessageBox(mWnd,_T("Alternate Famicom Keyboard does not support recording movies!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		Movie_Stop();
	}
}
static	unsigned char	Read1 (struct tExpPort *Cont)
{
	return 0;	/* tape, not yet implemented */
}
static	unsigned char	Read2 (struct tExpPort *Cont)
{
	unsigned char result = 0x1E;
	if (Cont->Out == 0)
	{
		switch (Cont->Scan)
		{
		case 0:	if (Controllers.KeyState[DIK_4] & 0x80)		result ^= 0x02;
			if (Controllers.KeyState[DIK_G] & 0x80)		result ^= 0x04;
			if (Controllers.KeyState[DIK_F] & 0x80)		result ^= 0x08;
			if (Controllers.KeyState[DIK_C] & 0x80)		result ^= 0x10;
			break;
		case 1:	if (Controllers.KeyState[DIK_2] & 0x80)		result ^= 0x02;
			if (Controllers.KeyState[DIK_D] & 0x80)		result ^= 0x04;
			if (Controllers.KeyState[DIK_S] & 0x80)		result ^= 0x08;
			if (Controllers.KeyState[DIK_END] & 0x80)	result ^= 0x10;
			break;
		case 2:	if (Controllers.KeyState[DIK_INSERT] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_BACKSPACE] & 0x80)	result ^= 0x04;
			if (Controllers.KeyState[DIK_PGDN] & 0x80)	result ^= 0x08;
			if (Controllers.KeyState[DIK_RIGHT] & 0x80)	result ^= 0x10;
			break;
		case 3:	if (Controllers.KeyState[DIK_9] & 0x80)		result ^= 0x02;
			if (Controllers.KeyState[DIK_I] & 0x80)		result ^= 0x04;
			if (Controllers.KeyState[DIK_L] & 0x80)		result ^= 0x08;
			if (Controllers.KeyState[DIK_COMMA] & 0x80)	result ^= 0x10;
			break;
		case 4:	if (Controllers.KeyState[DIK_RBRACKET] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_RETURN] & 0x80)	result ^= 0x04;
			if (Controllers.KeyState[DIK_UP] & 0x80)	result ^= 0x08;
			if (Controllers.KeyState[DIK_LEFT] & 0x80)	result ^= 0x10;
			break;
		case 5:	if (Controllers.KeyState[DIK_Q] & 0x80)		result ^= 0x02;
			if (Controllers.KeyState[DIK_CAPITAL] & 0x80)	result ^= 0x04;
			if (Controllers.KeyState[DIK_Z] & 0x80)		result ^= 0x08;
			if (Controllers.KeyState[DIK_SYSRQ] & 0x80)	result ^= 0x10;
			break;
		case 6:	if (Controllers.KeyState[DIK_7] & 0x80)		result ^= 0x02;
			if (Controllers.KeyState[DIK_Y] & 0x80)		result ^= 0x04;
			if (Controllers.KeyState[DIK_K] & 0x80)		result ^= 0x08;
			if (Controllers.KeyState[DIK_M] & 0x80)		result ^= 0x10;
			break;
		case 7:	if (Controllers.KeyState[DIK_MINUS] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_SEMICOLON] & 0x80)	result ^= 0x04;
			if (Controllers.KeyState[DIK_APOSTROPHE] & 0x80)result ^= 0x08;
			if (Controllers.KeyState[DIK_CAPITAL] & 0x80)	result ^= 0x10;
			break;
		case 8:	if (Controllers.KeyState[DIK_T] & 0x80)		result ^= 0x02;
			if (Controllers.KeyState[DIK_H] & 0x80)		result ^= 0x04;
			if (Controllers.KeyState[DIK_N] & 0x80)		result ^= 0x08;
			if (Controllers.KeyState[DIK_SPACE] & 0x80)	result ^= 0x10;
			break;
		case 9: if (Controllers.KeyState[DIK_GRAVE] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_F10] & 0x80)	result |= 0x04;
			if (Controllers.KeyState[DIK_F11] & 0x80)	result |= 0x08;
			if (Controllers.KeyState[DIK_F12] & 0x80)	result |= 0x10;
			break;
		case 10:if (Controllers.KeyState[DIK_SCROLL] & 0x80)	result |= 0x02;
			if (Controllers.KeyState[DIK_PAUSE] & 0x80)	result |= 0x04;
			if (Controllers.KeyState[DIK_GRAVE] & 0x80)	result |= 0x08;
			if (Controllers.KeyState[DIK_TAB] & 0x80)	result |= 0x10;
			break;
		case 11:if (Controllers.KeyState[DIK_CAPITAL] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_SLASH] & 0x80)	result |= 0x04;
			if (Controllers.KeyState[DIK_RSHIFT] & 0x80)	result |= 0x08;
			if (Controllers.KeyState[DIK_LMENU] & 0x80)	result |= 0x10;
			break;
		case 12:if (Controllers.KeyState[DIK_RMENU] & 0x80)	result |= 0x02;
			if (Controllers.KeyState[DIK_APPS] & 0x80)	result |= 0x04;
			if (Controllers.KeyState[DIK_RCONTROL] & 0x80)	result |= 0x08;
			if (Controllers.KeyState[DIK_NUMPAD1] & 0x80)	result |= 0x10;
		}
	}
	else
	{
		switch (Cont->Scan)
		{
		case 0:	if (Controllers.KeyState[DIK_F2] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_E] & 0x80)		result ^= 0x04;
			if (Controllers.KeyState[DIK_5] & 0x80)		result ^= 0x08;
			if (Controllers.KeyState[DIK_V] & 0x80)		result ^= 0x10;
			break;
		case 1:	if (Controllers.KeyState[DIK_F1] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_W] & 0x80)		result ^= 0x04;
			if (Controllers.KeyState[DIK_3] & 0x80)		result ^= 0x08;
			if (Controllers.KeyState[DIK_X] & 0x80)		result ^= 0x10;
			break;
		case 2:	if (Controllers.KeyState[DIK_F8] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_PGUP] & 0x80)	result ^= 0x04;
			if (Controllers.KeyState[DIK_DELETE] & 0x80)	result ^= 0x08;
			if (Controllers.KeyState[DIK_HOME] & 0x80)	result ^= 0x10;
			break;
		case 3:	if (Controllers.KeyState[DIK_F5] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_O] & 0x80)		result ^= 0x04;
			if (Controllers.KeyState[DIK_0] & 0x80)		result ^= 0x08;
			if (Controllers.KeyState[DIK_PERIOD] & 0x80)	result ^= 0x10;
			break;
		case 4:	if (Controllers.KeyState[DIK_F7] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_LBRACKET] & 0x80)	result ^= 0x04;
			if (Controllers.KeyState[DIK_BACKSLASH] & 0x80)	result ^= 0x08;
			if (Controllers.KeyState[DIK_DOWN] & 0x80)	result ^= 0x10;
			break;
		case 5:	if (Controllers.KeyState[DIK_ESCAPE] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_A] & 0x80)		result ^= 0x04;
			if (Controllers.KeyState[DIK_1] & 0x80)		result ^= 0x08;
			if (Controllers.KeyState[DIK_LCONTROL] & 0x80)	result ^= 0x10;
			break;
		case 6:	if (Controllers.KeyState[DIK_F4] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_U] & 0x80)		result ^= 0x04;
			if (Controllers.KeyState[DIK_8] & 0x80)		result ^= 0x08;
			if (Controllers.KeyState[DIK_J] & 0x80)		result ^= 0x10;
			break;
		case 7:	if (Controllers.KeyState[DIK_F6] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_P] & 0x80)		result ^= 0x04;
			if (Controllers.KeyState[DIK_EQUALS] & 0x80)	result ^= 0x08;
			if ((Controllers.KeyState[DIK_LSHIFT] & 0x80) || (Controllers.KeyState[DIK_RSHIFT] & 0x80))
									result ^= 0x10;
			break;
		case 8:	if (Controllers.KeyState[DIK_F3] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_R] & 0x80)		result ^= 0x04;
			if (Controllers.KeyState[DIK_6] & 0x80)		result ^= 0x08;
			if (Controllers.KeyState[DIK_B] & 0x80)		result ^= 0x10;
			break;
		case 9:	if (Controllers.KeyState[DIK_ADD] & 0x80)	result ^= 0x02;
			if (Controllers.KeyState[DIK_SUBTRACT] & 0x80)	result ^= 0x04;
			if (Controllers.KeyState[DIK_MULTIPLY] & 0x80)	result ^= 0x08;
			if (Controllers.KeyState[DIK_DIVIDE] & 0x80)	result ^= 0x10;
			break;
		case 10:if (Controllers.KeyState[DIK_NUMPAD6] & 0x80)	result |= 0x02;
			if (Controllers.KeyState[DIK_NUMPAD7] & 0x80)	result |= 0x04;
			if (Controllers.KeyState[DIK_NUMPAD8] & 0x80)	result |= 0x08;
			if (Controllers.KeyState[DIK_NUMPAD9] & 0x80)	result |= 0x10;
			break;
		case 11:if (Controllers.KeyState[DIK_NUMPAD0] & 0x80)	result |= 0x02;
			if (Controllers.KeyState[DIK_ADD] & 0x80)	result |= 0x04;
			if (Controllers.KeyState[DIK_SUBTRACT] & 0x80)	result |= 0x08;
			if (Controllers.KeyState[DIK_MULTIPLY] & 0x80)	result |= 0x10;
			break;
		case 12:if (Controllers.KeyState[DIK_DIVIDE] & 0x80)	result |= 0x02;
			if (Controllers.KeyState[DIK_DECIMAL] & 0x80)	result |= 0x04;
			if (Controllers.KeyState[DIK_NUMLOCK] & 0x80)	result |= 0x08;
			if (Controllers.KeyState[DIK_NUMPADENTER] & 0x80)	result |= 0x10;
			break;
		}
	}
	return result;
}

static	void	Write (struct tExpPort *Cont, unsigned char Val)
{
	BOOL ResetKB = Val & 1;
	BOOL WhichScan = Val & 2;
	BOOL SelectKey = Val & 4;
	if (SelectKey)
	{
		if ((Cont->Out) && (!WhichScan))
			Cont->Scan++;
		Cont->Out = WhichScan;
		if (ResetKB)
			Cont->Scan = 0;
	}
	else
	{
		/* tape, not yet implemented */
	}
}
static	void	Config (struct tExpPort *Cont, HWND hWnd)
{
	MessageBox(hWnd,_T("No configuration necessary!"),_T("Nintendulator"),MB_OK);
}
static	void	Unload (struct tExpPort *Cont)
{
	free(Cont->Data);
	free(Cont->MovData);
}
void	ExpPort_SetAltKeyboard (struct tExpPort *Cont)
{
	Cont->Read1 = Read1;
	Cont->Read2 = Read2;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->Frame = Frame;
	Cont->NumButtons = 0;
	Cont->DataLen = 2;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data));
	Cont->MovLen = 0;
	Cont->MovData = malloc(Cont->MovLen * sizeof(Cont->MovData));
	ZeroMemory(Cont->MovData,Cont->MovLen);
	Cont->Out = 0;
	Cont->Scan = 0;
}
#undef	Out
#undef	Scan