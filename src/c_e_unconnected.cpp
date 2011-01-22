/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Controllers.h"

namespace Controllers
{
void	ExpPort_Unconnected::Frame (unsigned char mode)
{
}
unsigned char	ExpPort_Unconnected::Read1 (void)
{
	return 0;
}
unsigned char	ExpPort_Unconnected::Read2 (void)
{
	return 0;
}
void	ExpPort_Unconnected::Write (unsigned char Val)
{
}
void	ExpPort_Unconnected::Config (HWND hWnd)
{
	MessageBox(hWnd, _T("No configuration necessary!"), _T("Nintendulator"), MB_OK);
}
ExpPort_Unconnected::~ExpPort_Unconnected (void)
{
}
ExpPort_Unconnected::ExpPort_Unconnected (int *buttons)
{
	Type = EXP_UNCONNECTED;
	NumButtons = 0;
	Buttons = buttons;
	DataLen = 0;
	Data = NULL;
	MovLen = 0;
	MovData = NULL;
}
} // namespace Controllers