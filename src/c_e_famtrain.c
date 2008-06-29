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
#define	Sel	Data[1]
#define	NewBits	Data[2]
static	void	Frame (struct tExpPort *Cont, unsigned char mode)
{
	int i;
	if (mode & MOV_PLAY)
		Cont->NewBits = Cont->MovData[0] | (Cont->MovData[1] << 8);
	else
	{
		Cont->NewBits = 0;
		for (i = 0; i < 12; i++)
		{
			if (Controllers_IsPressed(Cont->Buttons[i]))
				Cont->NewBits |= 1 << i;
		}
	}
	if (mode & MOV_RECORD)
	{
		Cont->MovData[0] = (unsigned char)(Cont->NewBits & 0xFF);
		Cont->MovData[1] = (unsigned char)(Cont->NewBits >> 8);
	}
}
static	unsigned char	Read1 (struct tExpPort *Cont)
{
	return 0;
}
static	unsigned char	Read2 (struct tExpPort *Cont)
{
	unsigned char result = 0;
	if (Cont->Sel & 0x1)
		result = (unsigned char)(Cont->Bits >> 8) & 0xF;
	else if (Cont->Sel & 0x2)
		result = (unsigned char)(Cont->Bits >> 4) & 0xF;
	else if (Cont->Sel & 0x4)
		result = (unsigned char)(Cont->Bits >> 0) & 0xF;
	return (result ^ 0xF) << 1;
}
static	void	Write (struct tExpPort *Cont, unsigned char Val)
{
	Cont->Bits = Cont->NewBits;
	Cont->Sel = ~Val & 7;
}
static	LRESULT	CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int dlgLists[12] = {IDC_CONT_D0,IDC_CONT_D1,IDC_CONT_D2,IDC_CONT_D3,IDC_CONT_D4,IDC_CONT_D5,IDC_CONT_D6,IDC_CONT_D7,IDC_CONT_D8,IDC_CONT_D9,IDC_CONT_D10,IDC_CONT_D11};
	int dlgButtons[12] = {IDC_CONT_K0,IDC_CONT_K1,IDC_CONT_K2,IDC_CONT_K3,IDC_CONT_K4,IDC_CONT_K5,IDC_CONT_K6,IDC_CONT_K7,IDC_CONT_K8,IDC_CONT_K9,IDC_CONT_K10,IDC_CONT_K11};
	static struct tExpPort *Cont = NULL;
	if (uMsg == WM_INITDIALOG)
		Cont = (struct tExpPort *)lParam;
	if ((uMsg == WM_COMMAND) && (LOWORD(wParam) == IDC_CONT_FLIP))
	{
		int i;
		int Buttons[12];
		for (i = 0; i < 12; i++)
			Buttons[i] = Cont->Buttons[i];
		Cont->Buttons[0] = Buttons[3];
		Cont->Buttons[1] = Buttons[2];
		Cont->Buttons[2] = Buttons[1];
		Cont->Buttons[3] = Buttons[0];
		Cont->Buttons[4] = Buttons[7];
		Cont->Buttons[5] = Buttons[6];
		Cont->Buttons[6] = Buttons[5];
		Cont->Buttons[7] = Buttons[4];
		Cont->Buttons[8] = Buttons[11];
		Cont->Buttons[9] = Buttons[10];
		Cont->Buttons[10] = Buttons[9];
		Cont->Buttons[11] = Buttons[8];
		for (i = 0; i < 12; i++)
			Controllers_ConfigButton(&Cont->Buttons[i],Cont->Buttons[i] >> 16,GetDlgItem(hDlg,dlgButtons[i]),FALSE);
	}
	else	Controllers_ParseConfigMessages(hDlg,12,dlgLists,dlgButtons,Cont->Buttons,uMsg,wParam,lParam);
	return FALSE;
}
static	void	Config (struct tExpPort *Cont, HWND hWnd)
{
	DialogBoxParam(hInst,(LPCTSTR)IDD_EXPPORT_FAMTRAIN,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tExpPort *Cont)
{
	free(Cont->Data);
	free(Cont->MovData);
}
void	ExpPort_SetFamTrainer (struct tExpPort *Cont)
{
	Cont->Read1 = Read1;
	Cont->Read2 = Read2;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->Frame = Frame;
	Cont->NumButtons = 12;
	Cont->DataLen = 3;
	Cont->Data = malloc(Cont->DataLen * sizeof(Cont->Data[0]));
	Cont->MovLen = 2;
	Cont->MovData = malloc(Cont->MovLen * sizeof(Cont->MovData[0]));
	ZeroMemory(Cont->MovData,Cont->MovLen);
	Cont->Bits = 0;
	Cont->Sel = 0;
	Cont->NewBits = 0;
}
#undef	NewBits
#undef	Sel
#undef	Bits
