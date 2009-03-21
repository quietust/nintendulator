/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2009 QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Controllers.h"

namespace Controllers
{
void	StdPort_Unconnected::Frame (unsigned char mode)
{
}
unsigned char	StdPort_Unconnected::Read (void)
{
	return 0;
}
void	StdPort_Unconnected::Write (unsigned char Val)
{
}
void	StdPort_Unconnected::Config (HWND hWnd)
{
	MessageBox(hWnd,_T("No configuration necessary!"),_T("Nintendulator"),MB_OK);
}
StdPort_Unconnected::~StdPort_Unconnected (void)
{
	free(Data);
	free(MovData);
}
StdPort_Unconnected::StdPort_Unconnected (int *buttons)
{
	Type = STD_UNCONNECTED;
	NumButtons = 0;
	Buttons = buttons;
	DataLen = 0;
	Data = malloc(DataLen);
	MovLen = 0;
	MovData = (unsigned char *)malloc(MovLen);
	ZeroMemory(MovData,MovLen);
}
} // namespace Controllers