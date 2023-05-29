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
struct ExpPort_FamilyBasicKeyboard_State
{
	unsigned char Column;
	unsigned char Row;
	unsigned char Keys[9];
};
int	ExpPort_FamilyBasicKeyboard::Save (FILE *out)
{
	int clen = 0;

	writeByte(State->Column);
	writeByte(State->Row);
	for (int i = 0; i < 9; i++)
		writeByte(State->Keys[i]);

	return clen;
}
int	ExpPort_FamilyBasicKeyboard::Load (FILE *in, int version_id)
{
	int clen = 0;

	// Skip length field from 0.975 and earlier
	if (version_id <= 1001)
	{
		unsigned short len;
		readWord(len);
		if (len != 11)
		{
			// State length was bad - discard all of it, then reset state
			fseek(in, len, SEEK_CUR); clen += len;
			State->Column = 0;
			State->Row = 0;
			ZeroMemory(State->Keys, sizeof(State->Keys));
			return clen;
		}
	}

	readByte(State->Column);
	readByte(State->Row);
	for (int i = 0; i < 9; i++)
		readByte(State->Keys[i]);

	return clen;
}
void	ExpPort_FamilyBasicKeyboard::Frame (unsigned char mode)
{
	int row, col;
	if (mode & MOV_PLAY)
	{
		for (row = 0; row < 9; row++)
			State->Keys[row] = MovData[row];
	}
	else
	{
		static const int keymap[9][8] = {
			{DIK_F8, DIK_RETURN, DIK_LBRACKET, DIK_RBRACKET, DIK_CAPITAL /* KANA */, DIK_RSHIFT, DIK_BACKSLASH, DIK_END /* STOP */},
			{DIK_F7, DIK_GRAVE /* @ */, DIK_APOSTROPHE /* : */, DIK_SEMICOLON, DIK_HOME /* _ */, DIK_SLASH, DIK_MINUS, DIK_EQUALS /* ^ */},
			{DIK_F6, DIK_O, DIK_L, DIK_K, DIK_PERIOD, DIK_COMMA, DIK_P, DIK_0},
			{DIK_F5, DIK_I, DIK_U, DIK_J, DIK_M, DIK_N, DIK_9, DIK_8},
			{DIK_F4, DIK_Y, DIK_G, DIK_H, DIK_B, DIK_V, DIK_7, DIK_6},
			{DIK_F3, DIK_T, DIK_R, DIK_D, DIK_F, DIK_C, DIK_5, DIK_4},
			{DIK_F2, DIK_W, DIK_S, DIK_A, DIK_X, DIK_Z, DIK_E, DIK_3},
			{DIK_F1, DIK_ESCAPE, DIK_Q, DIK_LCONTROL, DIK_LSHIFT, DIK_TAB /* GRPH */, DIK_1, DIK_2},
			{DIK_DELETE /* CLR */, DIK_UP, DIK_RIGHT, DIK_LEFT, DIK_DOWN, DIK_SPACE, DIK_BACK /* DEL, actually backspace */, DIK_INSERT}
		};

		for (row = 0; row < 9; row++)
		{
			State->Keys[row] = 0;
			for (col = 0; col < 8; col++)
				if (IsPressed(keymap[row][col]))
					State->Keys[row] |= 1 << col;
		}
		// special case - allow either Ctrl key
		if (IsPressed(DIK_RCONTROL))
			State->Keys[7] |= 0x08;
	}
	if (mode & MOV_RECORD)
	{
		for (row = 0; row < 9; row++)
			MovData[row] = State->Keys[row];
	}
}
unsigned char	ExpPort_FamilyBasicKeyboard::Read1 (void)
{
	return 0;	// tape, not yet implemented
}
unsigned char	ExpPort_FamilyBasicKeyboard::Read2 (void)
{
	unsigned char result = 0;
	if (State->Column)
		result = (State->Keys[State->Row] & 0xF0) >> 3;
	else	result = (State->Keys[State->Row] & 0x0F) << 1;
	return result ^ 0x1E;
}
void	ExpPort_FamilyBasicKeyboard::Write (unsigned char Val)
{
	BOOL ResetKB = Val & 1;
	BOOL SelColumn = Val & 2;
	BOOL SelectKey = Val & 4;
	if (SelectKey)
	{
		if ((State->Column) && (!SelColumn))
			State->Row = (State->Row + 1) % 10;
		State->Column = SelColumn;
		if (ResetKB)
			State->Row = 0;
	}
	else
	{
		// tape, not yet implemented
	}
}
HWND	ExpPort_FamilyBasicKeyboard_ConfigWindow = NULL;
INT_PTR	CALLBACK	ExpPort_FamilyBasicKeyboard_ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	if (uMsg != WM_COMMAND)
		return FALSE;
	if (wmId == IDOK)
	{
		EndDialog(hDlg, 1);
		ExpPort_FamilyBasicKeyboard_ConfigWindow = NULL;
		return TRUE;
	}
	return FALSE;
}
void	ExpPort_FamilyBasicKeyboard::Config (HWND hWnd)
{
	if (!ExpPort_FamilyBasicKeyboard_ConfigWindow)
	{
		// use hMainWnd instead of hWnd, so it stays open after closing Controller Config
		ExpPort_FamilyBasicKeyboard_ConfigWindow = CreateDialog(hInst, MAKEINTRESOURCE(IDD_EXPPORT_FBKEY), hMainWnd, ExpPort_FamilyBasicKeyboard_ConfigProc);
		SetWindowPos(ExpPort_FamilyBasicKeyboard_ConfigWindow, hMainWnd, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
	}
}
void	ExpPort_FamilyBasicKeyboard::SetMasks (void)
{
	MaskKeyboard = TRUE;
}
ExpPort_FamilyBasicKeyboard::~ExpPort_FamilyBasicKeyboard (void)
{
	delete State;
	delete[] MovData;
	if (ExpPort_FamilyBasicKeyboard_ConfigWindow)
	{
		DestroyWindow(ExpPort_FamilyBasicKeyboard_ConfigWindow);
		ExpPort_FamilyBasicKeyboard_ConfigWindow = NULL;
	}
}
ExpPort_FamilyBasicKeyboard::ExpPort_FamilyBasicKeyboard (DWORD *buttons)
{
	Type = EXP_FAMILYBASICKEYBOARD;
	NumButtons = 0;
	Buttons = buttons;
	State = new ExpPort_FamilyBasicKeyboard_State;
	MovLen = 9;
	MovData = new unsigned char[MovLen];
	ZeroMemory(MovData, MovLen);
	State->Column = 0;
	State->Row = 0;
	ZeroMemory(State->Keys, sizeof(State->Keys));
}
} // namespace Controllers
