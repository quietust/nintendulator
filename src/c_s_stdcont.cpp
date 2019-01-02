/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "Movie.h"
#include "Controllers.h"

namespace Controllers
{
struct StdPort_StdController_State
{
	unsigned char Bits;
	unsigned char BitPtr;
	unsigned char Strobe;
	unsigned char NewBits;
};
int	StdPort_StdController::Save (FILE *out)
{
	int clen = 0;

	writeByte(State->Bits);
	writeByte(State->BitPtr);
	writeByte(State->Strobe);
	writeByte(State->NewBits);

	return clen;
}
int	StdPort_StdController::Load (FILE *in, int version_id)
{
	int clen = 0;

	// Skip length field from 0.975 and earlier
	if (version_id <= 1001)
	{
		unsigned short len;
		readWord(len);
		if (len != 4)
		{
			// State length was bad - discard all of it, then reset state
			fseek(in, len, SEEK_CUR); clen += len;
			State->Bits = 0;
			State->BitPtr = 0;
			State->Strobe = 0;
			State->NewBits = 0;
			return clen;
		}
	}

	readByte(State->Bits);
	readByte(State->BitPtr);
	readByte(State->Strobe);
	readByte(State->NewBits);

	return clen;
}
void	StdPort_StdController::Frame (unsigned char mode)
{
	int i;
	if (mode & MOV_PLAY)
		State->NewBits = MovData[0];
	else
	{
		State->NewBits = 0;
		for (i = 0; i < 8; i++)
		{
			if (IsPressed(Buttons[i]))
				State->NewBits |= 1 << i;
		}
		if (!EnableOpposites)
		{	// prevent simultaneously pressing left+right or up+down
			if ((State->NewBits & 0xC0) == 0xC0)
				State->NewBits &= 0x3F;
			if ((State->NewBits & 0x30) == 0x30)
				State->NewBits &= 0xCF;
		}
	}
	if (mode & MOV_RECORD)
		MovData[0] = State->NewBits;
}
unsigned char	StdPort_StdController::Read (void)
{
	unsigned char result;
	if (State->Strobe)
	{
		State->Bits = State->NewBits;
		State->BitPtr = 0;
		result = (unsigned char)(State->Bits & 1);
	}
	else
	{
		if (State->BitPtr < 8)
			result = (unsigned char)(State->Bits >> State->BitPtr++) & 1;
		else	result = 1;
	}
	return result;
}
void	StdPort_StdController::Write (unsigned char Val)
{
	if ((State->Strobe) || (Val & 1))
	{
		State->Strobe = Val & 1;
		State->Bits = State->NewBits;
		State->BitPtr = 0;
	}
}
INT_PTR	CALLBACK	StdPort_StdController_ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const int dlgLists[8] = {IDC_CONT_D0,IDC_CONT_D1,IDC_CONT_D2,IDC_CONT_D3,IDC_CONT_D4,IDC_CONT_D5,IDC_CONT_D6,IDC_CONT_D7};
	const int dlgButtons[8] = {IDC_CONT_K0,IDC_CONT_K1,IDC_CONT_K2,IDC_CONT_K3,IDC_CONT_K4,IDC_CONT_K5,IDC_CONT_K6,IDC_CONT_K7};
	StdPort *Cont;
	if (uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		Cont = (StdPort *)lParam;
	}
	else	Cont = (StdPort *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	return ParseConfigMessages(hDlg, uMsg, wParam, lParam, 8, 0, dlgLists, dlgButtons, Cont ? Cont->Buttons : NULL);
}
void	StdPort_StdController::Config (HWND hWnd)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_STDPORT_STDCONTROLLER), hWnd, StdPort_StdController_ConfigProc, (LPARAM)this);
}
void	StdPort_StdController::SetMasks (void)
{
}
StdPort_StdController::~StdPort_StdController (void)
{
	delete State;
	delete[] MovData;
}
StdPort_StdController::StdPort_StdController (DWORD *buttons)
{
	Type = STD_STDCONTROLLER;
	NumButtons = 8;
	Buttons = buttons;
	State = new StdPort_StdController_State;
	MovLen = 1;
	MovData = new unsigned char[MovLen];
	ZeroMemory(MovData, MovLen);
	State->Bits = 0;
	State->BitPtr = 0;
	State->Strobe = 0;
	State->NewBits = 0;
}
} // namespace Controllers
