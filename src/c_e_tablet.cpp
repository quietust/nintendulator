/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) QMT Productions
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "Movie.h"
#include "Controllers.h"
#include "GFX.h"

namespace Controllers
{
struct ExpPort_Tablet_State
{
	unsigned long Bits;
	unsigned char Strobe;
	unsigned char BitPtr;
	unsigned char PosX;
	unsigned char PosY;
	unsigned char Button;
	unsigned long NewBits;
};
int	ExpPort_Tablet::Save (FILE *out)
{
	int clen = 0;

	writeLong(State->Bits);
	writeByte(State->Strobe);
	writeByte(State->BitPtr);
	writeByte(State->PosX);
	writeByte(State->PosY);
	writeByte(State->Button);
	writeLong(State->NewBits);

	return clen;
}
int	ExpPort_Tablet::Load (FILE *in, int version_id)
{
	int clen = 0;

	// Skip length field from 0.975 and earlier
	if (version_id <= 1001)
	{
		unsigned short len;
		readWord(len);
		if (len != 13)
		{
			// State length was bad - discard all of it, then reset state
			fseek(in, len, SEEK_CUR); clen += len;
			State->Bits = 0;
			State->Strobe = 0;
			State->BitPtr = 0;
			State->PosX = 0;
			State->PosY = 0;
			State->Button = 0;
			State->NewBits = 0;
			return clen;
		}
	}

	readLong(State->Bits);
	readByte(State->Strobe);
	readByte(State->BitPtr);
	readByte(State->PosX);
	readByte(State->PosY);
	readByte(State->Button);
	readLong(State->NewBits);

	return clen;
}
void	ExpPort_Tablet::Frame (unsigned char mode)
{
	int x, y;
	POINT pos;
	if (mode & MOV_PLAY)
	{
		State->PosX = MovData[0];
		State->PosY = MovData[1];
		State->Button = MovData[2];
		GFX::SetCursorPos(State->PosX, State->PosY);
	}
	else
	{
		GFX::GetCursorPos(&pos);
		if ((pos.x >= 0) && (pos.x <= 255) && (pos.y >= 0) && (pos.y <= 239))
		{
			State->PosX = pos.x;
			State->PosY = pos.y;
		}
		else	State->PosX = State->PosY = 0;
		State->Button = IsPressed(Buttons[0]);
	}
	if (mode & MOV_RECORD)
	{
		MovData[0] = State->PosX;
		MovData[1] = State->PosY;
		MovData[2] = State->Button;
	}
	
	State->NewBits = 0;
	if (State->Button)
		State->NewBits |= 0x0001;
	if (State->PosY >= 48)
		State->NewBits |= 0x0002;
	else if (State->Button)
		State->NewBits |= 0x0003;
	x = (State->PosX + 8) * 240 / 256;
	y = (State->PosY - 14) * 256 / 240;
	if (y < 0) y = 0;
	if (y > 255) y = 255;
	State->NewBits |= (x << 10) | (y << 2);
}
unsigned char	ExpPort_Tablet::Read1 (void)
{
	return 0;
}
unsigned char	ExpPort_Tablet::Read2 (void)
{
	if (State->Strobe & 1)
	{
		if (State->Strobe & 2)
		{
			if ((State->Bits << State->BitPtr) & 0x40000)
				return 0x00;
			else	return 0x08;
		}
		else	return 0x04;
	}
	else	return 0x00;
}
void	ExpPort_Tablet::Write (unsigned char Val)
{
	if (Val & 1)
	{
		if ((~State->Strobe) & Val & 2)
			State->BitPtr++;
	}
	else
	{
		State->Bits = State->NewBits;
		State->BitPtr = 0;
	}
	State->Strobe = Val & 3;
}
INT_PTR	CALLBACK	ExpPort_Tablet_ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const int dlgLists[1] = {IDC_CONT_D0};
	const int dlgButtons[1] = {IDC_CONT_K0};
	ExpPort *Cont;
	if (uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		Cont = (ExpPort *)lParam;
	}
	else	Cont = (ExpPort *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	return ParseConfigMessages(hDlg, uMsg, wParam, lParam, 1, 0, dlgLists, dlgButtons, Cont ? Cont->Buttons : NULL);
}
void	ExpPort_Tablet::Config (HWND hWnd)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_EXPPORT_TABLET), hWnd, ExpPort_Tablet_ConfigProc, (LPARAM)this);
}
void	ExpPort_Tablet::SetMasks (void)
{
}
ExpPort_Tablet::~ExpPort_Tablet (void)
{
	delete State;
	delete[] MovData;
}
ExpPort_Tablet::ExpPort_Tablet (DWORD *buttons)
{
	Type = EXP_TABLET;
	NumButtons = 1;
	Buttons = buttons;
	State = new ExpPort_Tablet_State;
	MovLen = 3;
	MovData = new unsigned char[MovLen];
	ZeroMemory(MovData, MovLen);
	State->Bits = 0;
	State->Strobe = 0;
	State->BitPtr = 0;
	State->PosX = 0;
	State->PosY = 0;
	State->Button = 0;
	State->NewBits = 0;
}
} // namespace Controllers
