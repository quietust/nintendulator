/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "Controllers.h"

namespace Controllers
{
int	ExpPort_Unconnected::Save (FILE *out)
{
	int clen = 0;

	return clen;
}
int	ExpPort_Unconnected::Load (FILE *in, int version_id)
{
	int clen = 0;

	// Skip length field from 0.975 and earlier
	if (version_id <= 1001)
	{
		unsigned short len;
		readWord(len);
		if (len != 0)
		{
			// State length was bad - discard all of it, then reset state
			fseek(in, len, SEEK_CUR); clen += len;
			return clen;
		}
	}

	return clen;
}
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
void	ExpPort_Unconnected::SetMasks (void)
{
}
ExpPort_Unconnected::~ExpPort_Unconnected (void)
{
}
ExpPort_Unconnected::ExpPort_Unconnected (DWORD *buttons)
{
	Type = EXP_UNCONNECTED;
	NumButtons = 0;
	Buttons = buttons;
	State = NULL;
	MovLen = 0;
	MovData = NULL;
}
} // namespace Controllers
