/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) QMT Productions
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "Movie.h"
#include "Controllers.h"

namespace Controllers
{
struct StdPort_SnesController_State
{
	unsigned char Bits1;
	unsigned char Bits2;
	unsigned char BitPtr;
	unsigned char Strobe;
	unsigned char NewBit1;
	unsigned char NewBit2;
};
int	StdPort_SnesController::Save (FILE *out)
{
	int clen = 0;

	writeByte(State->Bits1);
	writeByte(State->Bits2);
	writeByte(State->BitPtr);
	writeByte(State->Strobe);
	writeByte(State->NewBit1);
	writeByte(State->NewBit2);

	return clen;
}
int	StdPort_SnesController::Load (FILE *in, int version_id)
{
	int clen = 0;

	// Skip length field from 0.975 and earlier
	if (version_id <= 1001)
	{
		unsigned short len;
		readWord(len);
		if (len != 6)
		{
			// State length was bad - discard all of it, then reset state
			fseek(in, len, SEEK_CUR); clen += len;
			State->Bits1 = 0;
			State->Bits2 = 0;
			State->BitPtr = 0;
			State->Strobe = 0;
			State->NewBit1 = 0;
			State->NewBit2 = 0;
			return clen;
		}
	}

	readByte(State->Bits1);
	readByte(State->Bits2);
	readByte(State->BitPtr);
	readByte(State->Strobe);
	readByte(State->NewBit1);
	readByte(State->NewBit2);

	return clen;
}
void	StdPort_SnesController::Frame (unsigned char mode)
{
	int i;
	if (mode & MOV_PLAY)
	{
		State->NewBit1 = MovData[0];
		State->NewBit2 = MovData[1];
	}
	else
	{
		State->NewBit1 = 0;
		State->NewBit2 = 0;
		for (i = 0; i < 8; i++)
		{
			if (IsPressed(Buttons[i]))
				State->NewBit1 |= 1 << i;
			if ((i < 4) && (IsPressed(Buttons[i+8])))
				State->NewBit2 |= 1 << i;
		}
		if (!EnableOpposites)
		{	// prevent simultaneously pressing left+right or up+down
			if ((State->NewBit1 & 0xC0) == 0xC0)
				State->NewBit1 &= 0x3F;
			if ((State->NewBit1 & 0x30) == 0x30)
				State->NewBit1 &= 0xCF;
		}
	}
	if (mode & MOV_RECORD)
	{
		MovData[0] = State->NewBit1;
		MovData[1] = State->NewBit2;
	}
}
unsigned char	StdPort_SnesController::Read (void)
{
	unsigned char result = 1;
	if (State->Strobe)
	{
		State->Bits1 = State->NewBit1;
		State->Bits2 = State->NewBit2;
		State->BitPtr = 0;
		result = (unsigned char)(State->Bits1 & 1);
	}
	else
	{
		if (State->BitPtr < 8)
			result = (unsigned char)(State->Bits1 >> State->BitPtr++) & 1;
		else if (State->BitPtr < 16)
			result = (unsigned char)(State->Bits2 >> (State->BitPtr++ - 8)) & 1;
	}
	return result;
}
void	StdPort_SnesController::Write (unsigned char Val)
{
	if ((State->Strobe) || (Val & 1))
	{
		State->Strobe = Val & 1;
		State->Bits1 = State->NewBit1;
		State->Bits2 = State->NewBit2;
		State->BitPtr = 0;
	}
}
INT_PTR	CALLBACK	StdPort_SnesController_ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const int dlgLists[12] = {IDC_CONT_D0,IDC_CONT_D1,IDC_CONT_D2,IDC_CONT_D3,IDC_CONT_D4,IDC_CONT_D5,IDC_CONT_D6,IDC_CONT_D7,IDC_CONT_D8,IDC_CONT_D9,IDC_CONT_D10,IDC_CONT_D11};
	const int dlgButtons[12] = {IDC_CONT_K0,IDC_CONT_K1,IDC_CONT_K2,IDC_CONT_K3,IDC_CONT_K4,IDC_CONT_K5,IDC_CONT_K6,IDC_CONT_K7,IDC_CONT_K8,IDC_CONT_K9,IDC_CONT_K10,IDC_CONT_K11};
	StdPort *Cont;
	if (uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		Cont = (StdPort *)lParam;
	}
	else	Cont = (StdPort *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	return ParseConfigMessages(hDlg, uMsg, wParam, lParam, 12, 0, dlgLists, dlgButtons, Cont ? Cont->Buttons : NULL);
}
void	StdPort_SnesController::Config (HWND hWnd)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_STDPORT_SNESCONTROLLER), hWnd, StdPort_SnesController_ConfigProc, (LPARAM)this);
}
void	StdPort_SnesController::SetMasks (void)
{
}
StdPort_SnesController::~StdPort_SnesController (void)
{
	delete State;
	delete[] MovData;
}
StdPort_SnesController::StdPort_SnesController (DWORD *buttons)
{
	Type = STD_SNESCONTROLLER;
	NumButtons = 12;
	Buttons = buttons;
	State = new StdPort_SnesController_State;
	MovLen = 2;
	MovData = new unsigned char[MovLen];
	ZeroMemory(MovData, MovLen);
	State->Bits1 = 0;
	State->Bits2 = 0;
	State->BitPtr = 0;
	State->Strobe = 0;
	State->NewBit1 = 0;
	State->NewBit2 = 0;
}
} // namespace Controllers
