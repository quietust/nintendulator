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
struct StdPort_ArkanoidPaddle_State
{
	unsigned char Bits;
	unsigned short Pos;
	unsigned char BitPtr;
	unsigned char Strobe;
	unsigned char Button;
	unsigned char NewBits;
};
int	StdPort_ArkanoidPaddle::Save (FILE *out)
{
	int clen = 0;

	writeByte(State->Bits);
	writeWord(State->Pos);
	writeByte(State->BitPtr);
	writeByte(State->Strobe);
	writeByte(State->Button);
	writeByte(State->NewBits);

	return clen;
}
int	StdPort_ArkanoidPaddle::Load (FILE *in, int version_id)
{
	int clen = 0;

	// Skip length field from 0.975 and earlier
	if (version_id <= 1001)
	{
		unsigned short len;
		readWord(len);
		if (len != 7)
		{
			// State length was bad - discard all of it, then reset state
			fseek(in, len, SEEK_CUR); clen += len;
			State->Bits = 0;
			State->Pos = 340;
			State->BitPtr = 0;
			State->Strobe = 0;
			State->Button = 0;
			State->NewBits = 0;
			return clen;
		}
	}

	readByte(State->Bits);
	readWord(State->Pos);
	readByte(State->BitPtr);
	readByte(State->Strobe);
	readByte(State->Button);
	readByte(State->NewBits);

	return clen;
}
void	StdPort_ArkanoidPaddle::Frame (unsigned char mode)
{
	int x, i, bits;
	if (mode & MOV_PLAY)
	{
		State->Pos = MovData[0] | ((MovData[1] << 8) & 0x7F);
		State->Button = MovData[1] >> 7;
	}
	else
	{
		State->Button = IsPressed(Buttons[0]);
		State->Pos += GetDelta(Buttons[1]);
		// Arkanoid's expected range is 196-484 (code caps to 196-516)
		// Arkanoid 2 SP expected range is 156-452 (code caps to 156-372/420/452/484)
		// Arkanoid 2 VS expected range is 168-438
		if (State->Pos < 156)
			State->Pos = 156;
		if (State->Pos > 484)
			State->Pos = 484;
	}
	if (mode & MOV_RECORD)
	{
		MovData[0] = (unsigned char)(State->Pos & 0xFF);
		MovData[1] = (unsigned char)((State->Pos >> 8) | (State->Button << 7));
	}
	bits = 0;
	x = ~State->Pos;
	for (i = 0; i < 9; i++)
	{
		bits <<= 1;
		bits |= x & 1;
		x >>= 1;
	}
	State->NewBits = bits;
}
unsigned char	StdPort_ArkanoidPaddle::Read (void)
{
	unsigned char result;
	if (State->BitPtr < 8)
		result = (char)((State->Bits >> State->BitPtr++) & 1) << 4;
	else	result = 0x10;
	if (State->Button)
		result |= 0x08;
	return result;
}
void	StdPort_ArkanoidPaddle::Write (unsigned char Val)
{
	if ((!State->Strobe) && (Val & 1))
	{
		State->Bits = State->NewBits;
		State->BitPtr = 0;
	}
	State->Strobe = Val & 1;
}
INT_PTR	CALLBACK	StdPort_ArkanoidPaddle_ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static const int dlgLists[2] = {IDC_CONT_D0,IDC_CONT_D1};
	static const int dlgButtons[2] = {IDC_CONT_K0,IDC_CONT_K1};
	StdPort *Cont;
	if (uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		Cont = (StdPort *)lParam;
	}
	else	Cont = (StdPort *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	return ParseConfigMessages(hDlg, uMsg, wParam, lParam, 1, 1, dlgLists, dlgButtons, Cont ? Cont->Buttons : NULL);
}
void	StdPort_ArkanoidPaddle::Config (HWND hWnd)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_STDPORT_ARKANOIDPADDLE), hWnd, StdPort_ArkanoidPaddle_ConfigProc, (LPARAM)this);
}
void	StdPort_ArkanoidPaddle::SetMasks (void)
{
	MaskMouse = ((Buttons[1] >> 16) == 1);
}
StdPort_ArkanoidPaddle::~StdPort_ArkanoidPaddle (void)
{
	delete State;
	delete[] MovData;
}
StdPort_ArkanoidPaddle::StdPort_ArkanoidPaddle (DWORD *buttons)
{
	Type = STD_ARKANOIDPADDLE;
	NumButtons = 2;
	Buttons = buttons;
	State = new StdPort_ArkanoidPaddle_State;
	MovLen = 2;
	MovData = new unsigned char[MovLen];
	ZeroMemory(MovData, MovLen);
	State->Bits = 0;
	State->Pos = 340;
	State->BitPtr = 0;
	State->Strobe = 0;
	State->Button = 0;
	State->NewBits = 0;
}
} // namespace Controllers
