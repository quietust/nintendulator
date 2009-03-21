/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2009 QMT Productions
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
struct ExpPort_FamilyBasicKeyboard_State
{
	unsigned char Out;
	unsigned char Scan;
};
#include <poppack.h>
#define State ((ExpPort_FamilyBasicKeyboard_State *)Data)

void	ExpPort_FamilyBasicKeyboard::Frame (unsigned char mode)
{
	if (mode & MOV_RECORD)
	{
		MessageBox(hMainWnd,_T("Family Basic Keyboard does not support recording movies!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		Movie::Stop();
	}
}
unsigned char	ExpPort_FamilyBasicKeyboard::Read1 (void)
{
	return 0;	/* tape, not yet implemented */
}
unsigned char	ExpPort_FamilyBasicKeyboard::Read2 (void)
{
	unsigned char result = 0;
	if (State->Out == 0)
	{
		switch (State->Scan)
		{
		case 0:	if (KeyState[DIK_F8] & 0x80)		result |= 0x02;
			if (KeyState[DIK_RETURN] & 0x80)	result |= 0x04;
			if (KeyState[DIK_LBRACKET] & 0x80)	result |= 0x08;
			if (KeyState[DIK_RBRACKET] & 0x80)	result |= 0x10;
			break;
		case 1:	if (KeyState[DIK_F7] & 0x80)		result |= 0x02;
			if (KeyState[DIK_GRAVE] & 0x80)		result |= 0x04;	/* @ */
			if (KeyState[DIK_APOSTROPHE] & 0x80)	result |= 0x08;	/* : */
			if (KeyState[DIK_SEMICOLON] & 0x80)	result |= 0x10;
			break;
		case 2:	if (KeyState[DIK_F6] & 0x80)	result |= 0x02;
			if (KeyState[DIK_O] & 0x80)	result |= 0x04;
			if (KeyState[DIK_L] & 0x80)	result |= 0x08;
			if (KeyState[DIK_K] & 0x80)	result |= 0x10;
			break;
		case 3:	if (KeyState[DIK_F5] & 0x80)	result |= 0x02;
			if (KeyState[DIK_I] & 0x80)	result |= 0x04;
			if (KeyState[DIK_U] & 0x80)	result |= 0x08;
			if (KeyState[DIK_J] & 0x80)	result |= 0x10;
			break;
		case 4:	if (KeyState[DIK_F4] & 0x80)	result |= 0x02;
			if (KeyState[DIK_Y] & 0x80)	result |= 0x04;
			if (KeyState[DIK_G] & 0x80)	result |= 0x08;
			if (KeyState[DIK_H] & 0x80)	result |= 0x10;
			break;
		case 5:	if (KeyState[DIK_F3] & 0x80)	result |= 0x02;
			if (KeyState[DIK_T] & 0x80)	result |= 0x04;
			if (KeyState[DIK_R] & 0x80)	result |= 0x08;
			if (KeyState[DIK_D] & 0x80)	result |= 0x10;
			break;
		case 6:	if (KeyState[DIK_F2] & 0x80)	result |= 0x02;
			if (KeyState[DIK_W] & 0x80)	result |= 0x04;
			if (KeyState[DIK_S] & 0x80)	result |= 0x08;
			if (KeyState[DIK_A] & 0x80)	result |= 0x10;
			break;
		case 7:	if (KeyState[DIK_F1] & 0x80)		result |= 0x02;
			if (KeyState[DIK_ESCAPE] & 0x80)	result |= 0x04;
			if (KeyState[DIK_Q] & 0x80)		result |= 0x08;
			if ((KeyState[DIK_LCONTROL] & 0x80) || (KeyState[DIK_RCONTROL] & 0x80))
								result |= 0x10;
			break;
		case 8:	if (KeyState[DIK_BACK] & 0x80)	result |= 0x02;	/* CLR */
			if (KeyState[DIK_UP] & 0x80)	result |= 0x04;
			if (KeyState[DIK_RIGHT] & 0x80)	result |= 0x08;
			if (KeyState[DIK_LEFT] & 0x80)	result |= 0x10;
			break;
		}
	}
	else
	{
		switch (State->Scan)
		{
		case 0:	if (KeyState[DIK_CAPITAL] & 0x80)	result |= 0x02;	/* KANA */
			if (KeyState[DIK_RSHIFT] & 0x80)	result |= 0x04;
			if (KeyState[DIK_BACKSLASH] & 0x80)	result |= 0x08;
			if (KeyState[DIK_END] & 0x80)		result |= 0x10;	/* STOP */
			break;
		case 1:	if (KeyState[DIK_HOME] & 0x80)		result |= 0x02;	/* _ */
			if (KeyState[DIK_SLASH] & 0x80)		result |= 0x04;
			if (KeyState[DIK_MINUS] & 0x80)		result |= 0x08;
			if (KeyState[DIK_EQUALS] & 0x80)	result |= 0x10; /* ^ */
			break;
		case 2:	if (KeyState[DIK_PERIOD] & 0x80)	result |= 0x02;
			if (KeyState[DIK_COMMA] & 0x80)		result |= 0x04;
			if (KeyState[DIK_P] & 0x80)		result |= 0x08;
			if (KeyState[DIK_0] & 0x80)		result |= 0x10;
			break;
		case 3:	if (KeyState[DIK_M] & 0x80)	result |= 0x02;
			if (KeyState[DIK_N] & 0x80)	result |= 0x04;
			if (KeyState[DIK_9] & 0x80)	result |= 0x08;
			if (KeyState[DIK_8] & 0x80)	result |= 0x10;
			break;
		case 4:	if (KeyState[DIK_B] & 0x80)	result |= 0x02;
			if (KeyState[DIK_V] & 0x80)	result |= 0x04;
			if (KeyState[DIK_7] & 0x80)	result |= 0x08;
			if (KeyState[DIK_6] & 0x80)	result |= 0x10;
			break;
		case 5:	if (KeyState[DIK_F] & 0x80)	result |= 0x02;
			if (KeyState[DIK_C] & 0x80)	result |= 0x04;
			if (KeyState[DIK_5] & 0x80)	result |= 0x08;
			if (KeyState[DIK_4] & 0x80)	result |= 0x10;
			break;
		case 6:	if (KeyState[DIK_X] & 0x80)	result |= 0x02;
			if (KeyState[DIK_Z] & 0x80)	result |= 0x04;
			if (KeyState[DIK_E] & 0x80)	result |= 0x08;
			if (KeyState[DIK_3] & 0x80)	result |= 0x10;
			break;
		case 7:	if (KeyState[DIK_LSHIFT] & 0x80)	result |= 0x02;
			if (KeyState[DIK_TAB] & 0x80)		result |= 0x04;	/* GRPH */
			if (KeyState[DIK_1] & 0x80)		result |= 0x08;
			if (KeyState[DIK_2] & 0x80)		result |= 0x10;
			break;
		case 8:	if (KeyState[DIK_DOWN] & 0x80)		result |= 0x02;
			if (KeyState[DIK_SPACE] & 0x80)		result |= 0x04;
			if (KeyState[DIK_DELETE] & 0x80)	result |= 0x08;
			if (KeyState[DIK_INSERT] & 0x80)	result |= 0x10;
			break;
		}
	}
	return result ^ 0x1E;
}
void	ExpPort_FamilyBasicKeyboard::Write (unsigned char Val)
{
	BOOL ResetKB = Val & 1;
	BOOL WhichScan = Val & 2;
	BOOL SelectKey = Val & 4;
	if (SelectKey)
	{
		if ((State->Out) && (!WhichScan))
		{
			State->Scan++;
			State->Scan %= 10;
		}
		State->Out = WhichScan;
		if (ResetKB)
			State->Scan = 0;
	}
	else
	{
		/* tape, not yet implemented */
	}
}
static	HWND	ConfigWindow = NULL;
static	INT_PTR	CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	if (uMsg != WM_COMMAND)
		return FALSE;
	if (wmId == IDOK)
	{
		EndDialog(hDlg,1);
		ConfigWindow = NULL;
		return FALSE;
	}
	return FALSE;
}

void	ExpPort_FamilyBasicKeyboard::Config (HWND hWnd)
{
	if (!ConfigWindow)
	{
		// use hMainWnd instead of hWnd, so it stays open after closing Controller Config
		ConfigWindow = CreateDialog(hInst,(LPCTSTR)IDD_EXPPORT_FBKEY,hMainWnd,ConfigProc);
		SetWindowPos(ConfigWindow,hMainWnd,0,0,0,0,SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
	}
}

ExpPort_FamilyBasicKeyboard::~ExpPort_FamilyBasicKeyboard (void)
{
	free(Data);
	free(MovData);
	if (ConfigWindow)
	{
		DestroyWindow(ConfigWindow);
		ConfigWindow = NULL;
	}
}
ExpPort_FamilyBasicKeyboard::ExpPort_FamilyBasicKeyboard (int *buttons)
{
	Type = EXP_FAMILYBASICKEYBOARD;
	NumButtons = 0;
	Buttons = buttons;
	DataLen = sizeof(*State);
	Data = malloc(DataLen);
	MovLen = 0;
	MovData = (unsigned char *)malloc(MovLen);
	ZeroMemory(MovData, MovLen);
	State->Out = 0;
	State->Scan = 0;
}
} // namespace Controllers