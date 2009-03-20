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
#define	Bits1	Data[0]
#define	Bits2	Data[1]
#define	BitPtr	Data[2]
#define	Strobe	Data[3]
#define	NewBit1	Data[4]
#define	NewBit2	Data[5]

static	void	Frame (struct tStdPort *Cont, unsigned char mode)
{
	int i;
	unsigned char CBits1[12] = {0x02,0x01,0x00,0x00,0x04,0x10,0x80,0x00,0x08,0x20,0x40,0x00};
	unsigned char CBits2[12] = {0x00,0x00,0x02,0x01,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x04};
	if (mode & MOV_PLAY)
	{
		Cont->NewBit1 = Cont->MovData[0];
		Cont->NewBit2 = Cont->MovData[1];
	}
	else
	{
		Cont->NewBit1 = 0;
		Cont->NewBit2 = 0;
		for (i = 0; i < 12; i++)
		{
			if (IsPressed(Cont->Buttons[i]))
			{
				Cont->NewBit1 |= CBits1[i];
				Cont->NewBit2 |= CBits2[i];
			}
		}
	}
	if (mode & MOV_RECORD)
	{
		Cont->MovData[0] = (unsigned char)Cont->NewBit1;
		Cont->MovData[1] = (unsigned char)Cont->NewBit2;
	}
}

static	unsigned char	Read (struct tStdPort *Cont)
{
	unsigned char result = 0;
	if (Cont->Strobe)
	{
		Cont->Bits1 = Cont->NewBit1;
		Cont->Bits2 = Cont->NewBit2;
		Cont->BitPtr = 0;

	}
	if (Cont->BitPtr < 8)
		result |= ((unsigned char)(Cont->Bits1 >> Cont->BitPtr) & 1) << 3;
	else	result |= 0x08;
	if (Cont->BitPtr < 4)
		result |= ((unsigned char)(Cont->Bits2 >> Cont->BitPtr) & 1) << 4;
	else	result |= 0x10;
	if ((!Cont->Strobe) && (Cont->BitPtr < 8))
		Cont->BitPtr++;
	return result;
}
static	void	Write (struct tStdPort *Cont, unsigned char Val)
{
	if ((Cont->Strobe) || (Val & 1))
	{
		Cont->Strobe = Val & 1;
		Cont->Bits1 = Cont->NewBit1;
		Cont->Bits2 = Cont->NewBit2;
		Cont->BitPtr = 0;
	}
}
static	INT_PTR	CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int dlgLists[12] = {IDC_CONT_D0,IDC_CONT_D1,IDC_CONT_D2,IDC_CONT_D3,IDC_CONT_D4,IDC_CONT_D5,IDC_CONT_D6,IDC_CONT_D7,IDC_CONT_D8,IDC_CONT_D9,IDC_CONT_D10,IDC_CONT_D11};
	int dlgButtons[12] = {IDC_CONT_K0,IDC_CONT_K1,IDC_CONT_K2,IDC_CONT_K3,IDC_CONT_K4,IDC_CONT_K5,IDC_CONT_K6,IDC_CONT_K7,IDC_CONT_K8,IDC_CONT_K9,IDC_CONT_K10,IDC_CONT_K11};
	static struct tStdPort *Cont = NULL;
	if (uMsg == WM_INITDIALOG)
		Cont = (struct tStdPort *)lParam;
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
			ConfigButton(&Cont->Buttons[i],Cont->Buttons[i] >> 16,GetDlgItem(hDlg,dlgButtons[i]),FALSE);
	}
	else	ParseConfigMessages(hDlg,12,dlgLists,dlgButtons,Cont->Buttons,uMsg,wParam,lParam);
	return FALSE;
}
static	void	Config (struct tStdPort *Cont, HWND hWnd)
{
	DialogBoxParam(hInst,(LPCTSTR)IDD_STDPORT_POWERPAD,hWnd,ConfigProc,(LPARAM)Cont);
}
static	void	Unload (struct tStdPort *Cont)
{
	free(Cont->Data);
	free(Cont->MovData);
}
void	StdPort_SetPowerPad (struct tStdPort *Cont)
{
	Cont->Read = Read;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->Frame = Frame;
	Cont->NumButtons = 12;
	Cont->DataLen = 6;
	Cont->Data = (unsigned long *)malloc(Cont->DataLen * sizeof(Cont->Data[0]));
	Cont->MovLen = 2;
	Cont->MovData = (unsigned char *)malloc(Cont->MovLen * sizeof(Cont->MovData[0]));
	ZeroMemory(Cont->MovData,Cont->MovLen);
	Cont->Bits1 = 0;
	Cont->Bits2 = 0;
	Cont->BitPtr = 0;
	Cont->Strobe = 0;
	Cont->NewBit1 = 0;
	Cont->NewBit2 = 0;
}
#undef	NewBit2
#undef	NewBit1
#undef	Strobe
#undef	BitPtr
#undef	Bits2
#undef	Bits1
} // namespace Controllers