/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
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
#include <pshpack1.h>
struct StdPort_SnesController_State
{
	unsigned char Bits1;
	unsigned char Bits2;
	unsigned char BitPtr;
	unsigned char Strobe;
	unsigned char NewBit1;
	unsigned char NewBit2;
};
#include <poppack.h>
int	StdPort_SnesController::Save (FILE *out)
{
	int clen = 0;
	unsigned short len = sizeof(*State);

	writeWord(len);
	writeArray(State, len);

	return clen;
}
int	StdPort_SnesController::Load (FILE *in, int version_id)
{
	int clen = 0;
	unsigned short len;

	readWord(len);
	readArraySkip(State, len, sizeof(*State));

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
	int dlgLists[12] = {IDC_CONT_D0,IDC_CONT_D1,IDC_CONT_D2,IDC_CONT_D3,IDC_CONT_D4,IDC_CONT_D5,IDC_CONT_D6,IDC_CONT_D7,IDC_CONT_D8,IDC_CONT_D9,IDC_CONT_D10,IDC_CONT_D11};
	int dlgButtons[12] = {IDC_CONT_K0,IDC_CONT_K1,IDC_CONT_K2,IDC_CONT_K3,IDC_CONT_K4,IDC_CONT_K5,IDC_CONT_K6,IDC_CONT_K7,IDC_CONT_K8,IDC_CONT_K9,IDC_CONT_K10,IDC_CONT_K11};
	StdPort *Cont;
	if (uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		Cont = (StdPort *)lParam;
	}
	else	Cont = (StdPort *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	return ParseConfigMessages(hDlg, 12, dlgLists, dlgButtons, Cont ? Cont->Buttons : NULL, uMsg, wParam, lParam);
}
void	StdPort_SnesController::Config (HWND hWnd)
{
	DialogBoxParam(hInst, (LPCTSTR)IDD_STDPORT_SNESCONTROLLER, hWnd, StdPort_SnesController_ConfigProc, (LPARAM)this);
}
void	StdPort_SnesController::SetMasks (void)
{
}
StdPort_SnesController::~StdPort_SnesController (void)
{
	delete State;
	delete[] MovData;
}
StdPort_SnesController::StdPort_SnesController (int *buttons)
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