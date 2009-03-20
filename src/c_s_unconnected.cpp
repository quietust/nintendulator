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
static	void	Frame (struct tStdPort *Cont, unsigned char mode)
{
}
static	unsigned char	Read (struct tStdPort *Cont)
{
	return 0;
}
static	void	Write (struct tStdPort *Cont, unsigned char Val)
{
}
static	void	Config (struct tStdPort *Cont, HWND hWnd)
{
	MessageBox(hWnd,_T("No configuration necessary!"),_T("Nintendulator"),MB_OK);
}
static	void	Unload (struct tStdPort *Cont)
{
}
void	StdPort_SetUnconnected (struct tStdPort *Cont)
{
	Cont->Read = Read;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->Frame = Frame;
	Cont->NumButtons = 0;
	Cont->DataLen = 0;
	Cont->Data = (unsigned long *)malloc(Cont->DataLen * sizeof(Cont->Data[0]));
	Cont->MovLen = 0;
	Cont->MovData = (unsigned char *)malloc(Cont->MovLen * sizeof(Cont->MovData[0]));
	ZeroMemory(Cont->MovData,Cont->MovLen);
}
} // namespace Controllers