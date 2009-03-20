/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2009 QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Controllers.h"

static	void	Frame (struct Controllers::tExpPort *Cont, unsigned char mode)
{
}
static	unsigned char	Read1 (struct Controllers::tExpPort *Cont)
{
	return 0;
}
static	unsigned char	Read2 (struct Controllers::tExpPort *Cont)
{
	return 0;
}
static	void	Write (struct Controllers::tExpPort *Cont, unsigned char Val)
{
}
static	void	Config (struct Controllers::tExpPort *Cont, HWND hWnd)
{
	MessageBox(hWnd,_T("No configuration necessary!"),_T("Nintendulator"),MB_OK);
}
static	void	Unload (struct Controllers::tExpPort *Cont)
{
	free(Cont->Data);
	free(Cont->MovData);
}
void	ExpPort_SetUnconnected (struct Controllers::tExpPort *Cont)
{
	Cont->Read1 = Read1;
	Cont->Read2 = Read2;
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
