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
struct StdPort_StdController_State
{
	unsigned char Bits;
	unsigned char BitPtr;
	unsigned char Strobe;
	unsigned char NewBits;
};
#include <poppack.h>
#define State ((StdPort_StdController_State *)Data)

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
static	INT_PTR	CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int dlgLists[8] = {IDC_CONT_D0,IDC_CONT_D1,IDC_CONT_D2,IDC_CONT_D3,IDC_CONT_D4,IDC_CONT_D5,IDC_CONT_D6,IDC_CONT_D7};
	int dlgButtons[8] = {IDC_CONT_K0,IDC_CONT_K1,IDC_CONT_K2,IDC_CONT_K3,IDC_CONT_K4,IDC_CONT_K5,IDC_CONT_K6,IDC_CONT_K7};
	StdPort *Cont;
	if (uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		Cont = (StdPort *)lParam;
	}
	else	Cont = (StdPort *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	ParseConfigMessages(hDlg, 8, dlgLists, dlgButtons, Cont ? Cont->Buttons : NULL, uMsg, wParam, lParam);
	return FALSE;
}
void	StdPort_StdController::Config (HWND hWnd)
{
	DialogBoxParam(hInst, (LPCTSTR)IDD_STDPORT_STDCONTROLLER, hWnd, ConfigProc, (LPARAM)this);
}
StdPort_StdController::~StdPort_StdController (void)
{
	delete Data;
	delete[] MovData;
}
StdPort_StdController::StdPort_StdController (int *buttons)
{
	Type = STD_STDCONTROLLER;
	NumButtons = 8;
	Buttons = buttons;
	DataLen = sizeof(StdPort_StdController_State);
	Data = new StdPort_StdController_State;
	MovLen = 1;
	MovData = new unsigned char[MovLen];
	ZeroMemory(MovData, MovLen);
	State->Bits = 0;
	State->BitPtr = 0;
	State->Strobe = 0;
	State->NewBits = 0;
}
} // namespace Controllers