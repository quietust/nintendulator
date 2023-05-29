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
struct ExpPort_FamTrainer_State
{
	unsigned short Bits;
	unsigned char Sel;
	unsigned short NewBits;
};
int	ExpPort_FamTrainer::Save (FILE *out)
{
	int clen = 0;

	writeWord(State->Bits);
	writeByte(State->Sel);
	writeWord(State->NewBits);

	return clen;
}
int	ExpPort_FamTrainer::Load (FILE *in, int version_id)
{
	int clen = 0;

	// Skip length field from 0.975 and earlier
	if (version_id <= 1001)
	{
		unsigned short len;
		readWord(len);
		if (len != 5)
		{
			// State length was bad - discard all of it, then reset state
			fseek(in, len, SEEK_CUR); clen += len;
			State->Bits = 0;
			State->Sel = 0;
			State->NewBits = 0;
			return clen;
		}
	}

	readWord(State->Bits);
	readByte(State->Sel);
	readWord(State->NewBits);

	return clen;
}
void	ExpPort_FamTrainer::Frame (unsigned char mode)
{
	int i;
	if (mode & MOV_PLAY)
		State->NewBits = MovData[0] | (MovData[1] << 8);
	else
	{
		State->NewBits = 0;
		for (i = 0; i < 12; i++)
		{
			if (IsPressed(Buttons[i]))
				State->NewBits |= 1 << i;
		}
	}
	if (mode & MOV_RECORD)
	{
		MovData[0] = (unsigned char)(State->NewBits & 0xFF);
		MovData[1] = (unsigned char)(State->NewBits >> 8);
	}
}
unsigned char	ExpPort_FamTrainer::Read1 (void)
{
	return 0;
}
unsigned char	ExpPort_FamTrainer::Read2 (void)
{
	unsigned char result = 0;
	if (State->Sel & 0x1)
		result = (unsigned char)(State->Bits >> 8) & 0xF;
	else if (State->Sel & 0x2)
		result = (unsigned char)(State->Bits >> 4) & 0xF;
	else if (State->Sel & 0x4)
		result = (unsigned char)(State->Bits >> 0) & 0xF;
	return (result ^ 0xF) << 1;
}
void	ExpPort_FamTrainer::Write (unsigned char Val)
{
	State->Bits = State->NewBits;
	State->Sel = ~Val & 7;
}
INT_PTR	CALLBACK	ExpPort_FamTrainer_ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static const int dlgLists[12] = {IDC_CONT_D0,IDC_CONT_D1,IDC_CONT_D2,IDC_CONT_D3,IDC_CONT_D4,IDC_CONT_D5,IDC_CONT_D6,IDC_CONT_D7,IDC_CONT_D8,IDC_CONT_D9,IDC_CONT_D10,IDC_CONT_D11};
	static const int dlgButtons[12] = {IDC_CONT_K0,IDC_CONT_K1,IDC_CONT_K2,IDC_CONT_K3,IDC_CONT_K4,IDC_CONT_K5,IDC_CONT_K6,IDC_CONT_K7,IDC_CONT_K8,IDC_CONT_K9,IDC_CONT_K10,IDC_CONT_K11};
	ExpPort *Cont;
	if (uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		Cont = (ExpPort *)lParam;
	}
	else	Cont = (ExpPort *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if ((uMsg == WM_COMMAND) && (LOWORD(wParam) == IDC_CONT_FLIP))
	{
		int i;
		int Buttons[12];
		for (i = 0; i < 12; i++)
			Buttons[i] = Cont->Buttons[i];
		Cont->Buttons[0] = Buttons[3];
		Cont->Buttons[1] = Buttons[2];
		Cont->Buttons[2] = Buttons[1];
		Cont->Buttons[3] = Buttons[0];
		Cont->Buttons[4] = Buttons[7];
		Cont->Buttons[5] = Buttons[6];
		Cont->Buttons[6] = Buttons[5];
		Cont->Buttons[7] = Buttons[4];
		Cont->Buttons[8] = Buttons[11];
		Cont->Buttons[9] = Buttons[10];
		Cont->Buttons[10] = Buttons[9];
		Cont->Buttons[11] = Buttons[8];
		for (i = 0; i < 12; i++)
			ConfigButton(&Cont->Buttons[i], Cont->Buttons[i] >> 16, GetDlgItem(hDlg, dlgButtons[i]), FALSE);
		return TRUE;
	}
	return ParseConfigMessages(hDlg, uMsg, wParam, lParam, 12, 0, dlgLists, dlgButtons, Cont ? Cont->Buttons : NULL);
}
void	ExpPort_FamTrainer::Config (HWND hWnd)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_EXPPORT_FAMTRAIN), hWnd, ExpPort_FamTrainer_ConfigProc, (LPARAM)this);
}
void	ExpPort_FamTrainer::SetMasks (void)
{
}
ExpPort_FamTrainer::~ExpPort_FamTrainer (void)
{
	delete State;
	delete[] MovData;
}
ExpPort_FamTrainer::ExpPort_FamTrainer (DWORD *buttons)
{
	Type = EXP_FAMTRAINER;
	NumButtons = 12;
	Buttons = buttons;
	State = new ExpPort_FamTrainer_State;
	MovLen = 2;
	MovData = new unsigned char[MovLen];
	ZeroMemory(MovData, MovLen);
	State->Bits = 0;
	State->Sel = 0;
	State->NewBits = 0;
}
} // namespace Controllers
