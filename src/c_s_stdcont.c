/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2008 QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "Movie.h"
#include "Controllers.h"

#define	Bits	Data[0]
#define	BitPtr	Data[1]
#define	Strobe	Data[2]
#define	NewBits	Data[3]

static	void	Frame (struct tStdPort *Cont, unsigned char mode)
{
	int i;
	if (mode & MOV_PLAY)
		Cont->NewBits = Cont->MovData[0];
	else
	{
		Cont->NewBits = 0;
		for (i = 0; i < 8; i++)
		{
			if (Controllers_IsPressed(Cont->Buttons[i]))
				Cont->NewBits |= 1 << i;
		}
		if (!Controllers.EnableOpposites)
		{	/* prevent simultaneously pressing left+right or up+down */
			if ((Cont->NewBits & 0xC0) == 0xC0)
				Cont->NewBits &= 0x3F;
			if ((Cont->NewBits & 0x30) == 0x30)
				Cont->NewBits &= 0xCF;
		}
	}
	if (mode & MOV_RECORD)
		Cont->MovData[0] = (unsigned char)Cont->NewBits;
}

static	unsigned char	Read (struct tStdPort *Cont)
{
	unsigned char result;
	if (Cont->Strobe)
	{
		Cont->Bits = Cont->NewBits;
		Cont->BitPtr = 0;
		result = (unsigned char)(Cont->Bits & 1);
	}
	else
	{
		if (Cont->BitPtr < 8)
			result = (unsigned char)(Cont->Bits >> Cont->BitPtr++) & 1;
		else	result = 1;
	}
	return result;
}
static	void	Write (struct tStdPort *Cont, unsigned char Val)
{
	if ((Cont->Strobe) || (Val & 1))
	{
		Cont->Strobe = Val & 1;
		Cont->Bits = Cont->NewBits;
		Cont->BitPtr = 0;
	}
}
static	LRESULT	CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int dlgLists[8] = {IDC_CONT_D0,IDC_CONT_D1,IDC_CONT_D2,IDC_CONT_D3,IDC_CONT_D4,IDC_CONT_D5,IDC_CONT_D6,IDC_CONT_D7};
	int dlgButtons[8] = {IDC_CONT_K0,IDC_CONT_K1,IDC_CONT_K2,IDC_CONT_K3,IDC_CONT_K4,IDC_CONT_K5,IDC_CONT_K6,IDC_CONT_K7};
	static struct tStdPort *Cont = NULL;
	if (uMsg == WM_INITDIALOG)
		Cont = (struct tStdPort *)lParam;
	Controllers_ParseConfigMessages(hDlg,8,dlgLists,dlgButtons,Cont->Buttons,uMsg,wParam,lParam);
	return FALSE;
}
static	void	Config (struct tStdPort *Cont, HWND hWnd)
{
	DialogBoxParam(hInst,(LPCTSTR)IDD_STDPORT_STDCONTROLLER,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tStdPort *Cont)
{
	free(Cont->Data);
	free(Cont->MovData);
}
void	StdPort_SetStdController (struct tStdPort *Cont)
{
	Cont->Read = Read;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->Frame = Frame;
	Cont->NumButtons = 8;
	Cont->DataLen = 4;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data[0]));
	Cont->MovLen = 1;
	Cont->MovData = malloc(Cont->MovLen * sizeof(Cont->MovData[0]));
	ZeroMemory(Cont->MovData,Cont->MovLen);
	Cont->Bits = 0;
	Cont->BitPtr = 0;
	Cont->Strobe = 0;
	Cont->NewBits = 0;
}
#undef	NewBits
#undef	Strobe
#undef	BitPtr
#undef	Bits