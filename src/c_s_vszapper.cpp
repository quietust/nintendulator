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
#include "MapperInterface.h"
#include "PPU.h"
#include "GFX.h"

namespace Controllers
{
#include <pshpack1.h>
struct StdPort_VSZapper_State
{
	unsigned char Bits;
	unsigned char BitPtr;
	unsigned char Strobe;
	unsigned char PosX;
	unsigned char PosY;
	unsigned char Button;
};
#include <poppack.h>
#define State ((StdPort_VSZapper_State *)Data)

void	StdPort_VSZapper::Frame (unsigned char mode)
{
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
		else	State->PosX = State->PosY = 255;
		State->Button = IsPressed(Buttons[0]);
	}
	if (mode & MOV_RECORD)
	{
		MovData[0] = State->PosX;
		MovData[1] = State->PosY;
		MovData[2] = State->Button;
	}
}
unsigned char	StdPort_VSZapper::Read (void)
{
	unsigned char result;
	if (State->Strobe)
	{
		State->BitPtr = 0;
		result = (unsigned char)(State->Bits & 1);
	}
	else
	{
		if (State->BitPtr < 8)
			result = (unsigned char)(State->Bits >> State->BitPtr++) & 1;
		else	result = 0;
	}
	return result;
}
void	StdPort_VSZapper::Write (unsigned char Val)
{
	int x = State->PosX, y = State->PosY;

	State->Strobe = Val & 1;
	if (!State->Strobe)
		return;
		
	State->Bits = 0x10;
	State->BitPtr = 0;
	if (State->Button)
		State->Bits |= 0x80;

	if ((x < 0) || (x >= 256) || (y < 0) || (y >= 240))
		return;

	if (PPU::IsRendering && PPU::OnScreen)
	{
		int X, Y;
		int WhiteCount = 0;
		for (Y = y - 8; Y < y + 8; Y++)
		{
			if (Y < 0)
				Y = 0;
			if (Y < PPU::SLnum - 32)
				continue;
			if (Y > PPU::SLnum)
				break;
			for (X = x - 8; X < x + 8; X++)
			{
				if (X < 0)
					X = 0;
				if (X > 255)
					break;
				if ((Y == PPU::SLnum) && (X >= PPU::Clockticks))
					break;
				if (GFX::ZapperHit(PPU::DrawArray[Y * 256 + X]))
					WhiteCount++;
			}
		}
		if (WhiteCount >= 64)
			State->Bits |= 0x40;
	}
}
INT_PTR	CALLBACK	StdPort_VSZapper_ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int dlgLists[1] = {IDC_CONT_D0};
	int dlgButtons[1] = {IDC_CONT_K0};
	StdPort *Cont;
	if (uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		Cont = (StdPort *)lParam;
	}
	else	Cont = (StdPort *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	return ParseConfigMessages(hDlg, 1, dlgLists, dlgButtons, Cont ? Cont->Buttons : NULL, uMsg, wParam, lParam);
}
void	StdPort_VSZapper::Config (HWND hWnd)
{
	DialogBoxParam(hInst, (LPCTSTR)IDD_STDPORT_VSZAPPER, hWnd, StdPort_VSZapper_ConfigProc, (LPARAM)this);
}
void	StdPort_VSZapper::SetMasks (void)
{
}
StdPort_VSZapper::~StdPort_VSZapper (void)
{
	delete Data;
	delete[] MovData;
}
StdPort_VSZapper::StdPort_VSZapper (int *buttons)
{
	Type = STD_VSZAPPER;
	NumButtons = 1;
	Buttons = buttons;
	DataLen = sizeof(StdPort_VSZapper_State);
	Data = new StdPort_VSZapper_State;
	MovLen = 3;
	MovData = new unsigned char[MovLen];
	ZeroMemory(MovData, MovLen);
	State->PosX = 0;
	State->PosY = 0;
	State->Button = 0;
	State->Bits = 0x10;
	State->BitPtr = 0;
	State->Strobe = 0;
	GFX::SetFrameskip(-2);
}
} // namespace Controllers