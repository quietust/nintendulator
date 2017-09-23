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
#include <pshpack1.h>
struct ExpPort_Fami4Play_State
{
	unsigned char Bits1;
	unsigned char Bits2;
	unsigned char BitPtr1;
	unsigned char BitPtr2;
	unsigned char Strobe;
	unsigned char NewBit1;
	unsigned char NewBit2;
};
#include <poppack.h>
int	ExpPort_Fami4Play::Save (FILE *out)
{
	int clen = 0;
	unsigned short len = sizeof(*State);

	writeWord(len);
	writeArray(State, len);

	return clen;
}
int	ExpPort_Fami4Play::Load (FILE *in, int version_id)
{
	int clen = 0;
	unsigned short len;

	readWord(len);
	readArraySkip(State, len, sizeof(*State));

	return clen;
}
void	ExpPort_Fami4Play::Frame (unsigned char mode)
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
			if (IsPressed(Buttons[i+8]))
				State->NewBit2 |= 1 << i;
		}
		if (!EnableOpposites)
		{	// prevent simultaneously pressing left+right or up+down
			if ((State->NewBit1 & 0xC0) == 0xC0)
				State->NewBit1 &= 0x3F;
			if ((State->NewBit2 & 0xC0) == 0xC0)
				State->NewBit2 &= 0x3F;

			if ((State->NewBit1 & 0x30) == 0x30)
				State->NewBit1 &= 0xCF;
			if ((State->NewBit2 & 0x30) == 0x30)
				State->NewBit2 &= 0xCF;
		}
	}
	if (mode & MOV_RECORD)
	{
		MovData[0] = State->NewBit1;
		MovData[1] = State->NewBit2;
	}
}
unsigned char	ExpPort_Fami4Play::Read1 (void)
{
	unsigned char result = 1;
	if (State->Strobe)
	{
		State->Bits1 = State->NewBit1;
		State->BitPtr1 = 0;
		result = (unsigned char)(State->Bits1 & 1);
	}
	else
	{
		if (State->BitPtr1 < 8)
			result = (unsigned char)(State->Bits1 >> State->BitPtr1++) & 1;
	}
	return result << 1;
}
unsigned char	ExpPort_Fami4Play::Read2 (void)
{
	unsigned char result = 1;
	if (State->Strobe)
	{
		State->Bits2 = State->NewBit2;
		State->BitPtr2 = 0;
		result = (unsigned char)(State->Bits2 & 1);
	}
	else
	{
		if (State->BitPtr2 < 8)
			result = (unsigned char)(State->Bits2 >> State->BitPtr2++) & 1;
	}
	return result << 1;
}
void	ExpPort_Fami4Play::Write (unsigned char Val)
{
	if ((State->Strobe) || (Val & 1))
	{
		State->Strobe = Val & 1;
		State->Bits1 = State->NewBit1;
		State->Bits2 = State->NewBit2;
		State->BitPtr1 = 0;
		State->BitPtr2 = 0;
	}
}
INT_PTR	CALLBACK	ExpPort_Fami4Play_ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const int dlgLists[16] = {IDC_CONT_D0,IDC_CONT_D1,IDC_CONT_D2,IDC_CONT_D3,IDC_CONT_D4,IDC_CONT_D5,IDC_CONT_D6,IDC_CONT_D7,IDC_CONT_D8,IDC_CONT_D9,IDC_CONT_D10,IDC_CONT_D11,IDC_CONT_D12,IDC_CONT_D13,IDC_CONT_D14,IDC_CONT_D15};
	const int dlgButtons[16] = {IDC_CONT_K0,IDC_CONT_K1,IDC_CONT_K2,IDC_CONT_K3,IDC_CONT_K4,IDC_CONT_K5,IDC_CONT_K6,IDC_CONT_K7,IDC_CONT_K8,IDC_CONT_K9,IDC_CONT_K10,IDC_CONT_K11,IDC_CONT_K12,IDC_CONT_K13,IDC_CONT_K14,IDC_CONT_K15};
	ExpPort *Cont;
	if (uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		Cont = (ExpPort *)lParam;
	}
	else	Cont = (ExpPort *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	return ParseConfigMessages(hDlg, uMsg, wParam, lParam, 16, 0, dlgLists, dlgButtons, Cont ? Cont->Buttons : NULL);
}
void	ExpPort_Fami4Play::Config (HWND hWnd)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_EXPPORT_FAMI4PLAY), hWnd, ExpPort_Fami4Play_ConfigProc, (LPARAM)this);
}
void	ExpPort_Fami4Play::SetMasks (void)
{
}
ExpPort_Fami4Play::~ExpPort_Fami4Play (void)
{
	delete State;
	delete[] MovData;
}
ExpPort_Fami4Play::ExpPort_Fami4Play (DWORD *buttons)
{
	Type = EXP_FAMI4PLAY;
	NumButtons = 16;
	Buttons = buttons;
	State = new ExpPort_Fami4Play_State;
	MovLen = 2;
	MovData = new unsigned char[MovLen];
	ZeroMemory(MovData, MovLen);
	State->Bits1 = 0;
	State->Bits2 = 0;
	State->BitPtr1 = 0;
	State->BitPtr2 = 0;
	State->Strobe = 0;
	State->NewBit1 = 0;
	State->NewBit2 = 0;
}
} // namespace Controllers
