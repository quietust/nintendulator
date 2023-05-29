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
struct StdPort_SnesMouse_State
{
	unsigned long Bits;
	unsigned char BitPtr;
	unsigned char Strobe;
	signed char Xmov, Ymov;
	signed short Xdelta, Ydelta;
	unsigned char Buttons;
	unsigned char Sensitivity;
};
int	StdPort_SnesMouse::Save (FILE *out)
{
	int clen = 0;

	writeLong(State->Bits);
	writeByte(State->BitPtr);
	writeByte(State->Strobe);
	writeByte(State->Xmov);
	writeByte(State->Ymov);
	writeWord(State->Xdelta);
	writeWord(State->Ydelta);
	writeByte(State->Buttons);
	writeByte(State->Sensitivity);

	return clen;
}
int	StdPort_SnesMouse::Load (FILE *in, int version_id)
{
	int clen = 0;

	// Skip length field from 0.975
	if (version_id <= 1001)
	{
		unsigned short len;
		readWord(len);
		if (len != 14)
		{
			// State length was bad - discard all of it, then reset state
			fseek(in, len, SEEK_CUR); clen += len;
			State->Bits = 0;
			State->BitPtr = 0;
			State->Strobe = 0;
			State->Xmov = 0;
			State->Ymov = 0;
			State->Xdelta = 0;
			State->Ydelta = 0;
			State->Buttons = 0;
			State->Sensitivity = 0;
			return clen;
		}
	}

	readLong(State->Bits);
	readByte(State->BitPtr);
	readByte(State->Strobe);
	readByte(State->Xmov);
	readByte(State->Ymov);
	readWord(State->Xdelta);
	readWord(State->Ydelta);
	readByte(State->Buttons);
	readByte(State->Sensitivity);

	return clen;
}
void	StdPort_SnesMouse::Frame (unsigned char mode)
{
	if (mode & MOV_PLAY)
	{
		State->Xdelta = (signed char)MovData[0];
		State->Ydelta = (signed char)MovData[1];
		State->Buttons = MovData[2];
	}
	else
	{
		State->Buttons = 0;
		if (IsPressed(Buttons[0]))
			State->Buttons |= 0x1;
		if (IsPressed(Buttons[1]))
			State->Buttons |= 0x2;
		State->Xdelta = GetDelta(Buttons[2]);
		State->Ydelta = GetDelta(Buttons[3]);
	}
	if (mode & MOV_RECORD)
	{
		MovData[0] = (unsigned char)(State->Xdelta & 0xFF);
		MovData[1] = (unsigned char)(State->Ydelta & 0xFF);
		MovData[2] = (unsigned char)(State->Buttons & 0x03);
	}
	// need to do this to handle rounding properly
	if (State->Xdelta > 0)
		State->Xmov += State->Xdelta >> (2 - State->Sensitivity);
	else	State->Xmov -= (-State->Xdelta) >> (2 - State->Sensitivity);
	if (State->Ydelta > 0)
		State->Ymov += State->Ydelta >> (2 - State->Sensitivity);
	else	State->Ymov -= (-State->Ydelta) >> (2 - State->Sensitivity);
}
unsigned char	StdPort_SnesMouse::Read (void)
{
	unsigned char result;
	if (State->Strobe)
		State->Sensitivity = (State->Sensitivity + 1) % 3;
	if (State->BitPtr < 32)
		result = (unsigned char)(((State->Bits << State->BitPtr++) & 0x80000000) >> 31);
	else	result = 0x00;
	return result;
}
void	StdPort_SnesMouse::Write (unsigned char Val)
{
	if ((State->Strobe) && !(Val & 1))
	{
		State->Bits = 0x00010000;
		State->Bits |= State->Sensitivity << 20;
		State->Bits |= State->Buttons << 22;
		if (State->Ymov < 0)
		{
			State->Ymov = -State->Ymov;
			State->Bits |= 0x00008000;
		}
		if (State->Xmov < 0)
		{
			State->Xmov = -State->Xmov;
			State->Bits |= 0x00000080;
		}
		State->Bits |= (State->Ymov & 0x7F) << 8;
		State->Bits |= (State->Xmov & 0x7F) << 0;
		State->Xmov = State->Ymov = 0;
		State->BitPtr = 0;
	}
	State->Strobe = Val & 1;
}
INT_PTR	CALLBACK	StdPort_SnesMouse_ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static const int dlgLists[4] = {IDC_CONT_D0,IDC_CONT_D1,IDC_CONT_D2,IDC_CONT_D3};
	static const int dlgButtons[4] = {IDC_CONT_K0,IDC_CONT_K1,IDC_CONT_K2,IDC_CONT_K3};
	StdPort *Cont;
	if (uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		Cont = (StdPort *)lParam;
	}
	else	Cont = (StdPort *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	return ParseConfigMessages(hDlg, uMsg, wParam, lParam, 2, 2, dlgLists, dlgButtons, Cont ? Cont->Buttons : NULL);
}
void	StdPort_SnesMouse::Config (HWND hWnd)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_STDPORT_SNESMOUSE), hWnd, StdPort_SnesMouse_ConfigProc, (LPARAM)this);
}
void	StdPort_SnesMouse::SetMasks (void)
{
	MaskMouse = (((Buttons[2] >> 16) == 1) || ((Buttons[3] >> 16) == 1));
}
StdPort_SnesMouse::~StdPort_SnesMouse (void)
{
	delete State;
	delete[] MovData;
}
StdPort_SnesMouse::StdPort_SnesMouse (DWORD *buttons)
{
	Type = STD_SNESMOUSE;
	NumButtons = 4;
	Buttons = buttons;
	State = new StdPort_SnesMouse_State;
	MovLen = 3;
	MovData = new unsigned char[MovLen];
	ZeroMemory(MovData, MovLen);
	State->Bits = 0;
	State->BitPtr = 0;
	State->Strobe = 0;
	State->Xmov = 0;
	State->Ymov = 0;
	State->Xdelta = 0;
	State->Ydelta = 0;
	State->Buttons = 0;
	State->Sensitivity = 0;
}
} // namespace Controllers
