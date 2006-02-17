/*
Nintendulator - A Win32 NES emulator written in C.
Designed for maximum emulation accuracy.
Copyright (c) 2002  Quietust

Based on NinthStar, a portable Win32 NES Emulator written in C++
Copyright (C) 2000  David de Regt

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

For a copy of the GNU General Public License, go to:
http://www.gnu.org/copyleft/gpl.html#SEC1
*/

#include "Nintendulator.h"
#include "Controllers.h"

#define	BitPtr	Data[0]
#define	Strobe	Data[1]
#define	PortNum	Data[2]
static	unsigned char	Read (struct tStdPort *Cont)
{
	unsigned char result = 0;
	struct tStdPort *Port1 = NULL, *Port2 = NULL;
	switch (Cont->PortNum)
	{
	case 0:	Port1 = &Controllers.FSPort1;
		Port2 = &Controllers.FSPort3;
		break;
	case 1:	Port1 = &Controllers.FSPort2;
		Port2 = &Controllers.FSPort4;
		break;
	default:break;
	}
	switch (Cont->BitPtr)
	{
	case  0:case  1:case  2:case  3:case  4:case  5:case  6:case  7:
		result = Port1->Read(Port1);
		break;
	case  8:case  9:case 10:case 11:case 12:case 13:case 14:case 15:
		result = Port2->Read(Port2);
		break;
	case 18:if (Cont->PortNum == 1)
			result = 1;
		break;
	case 19:if (Cont->PortNum == 0)
			result = 1;
		break;
	}
	if (Cont->BitPtr < 20)
		Cont->BitPtr++;
	return result;
}
static	void	Write (struct tStdPort *Cont, unsigned char Val)
{
	struct tStdPort *Port1 = NULL, *Port2 = NULL;
	switch (Cont->PortNum)
	{
	case 0:	Port1 = &Controllers.FSPort1;
		Port2 = &Controllers.FSPort3;
		break;
	case 1:	Port1 = &Controllers.FSPort2;
		Port2 = &Controllers.FSPort4;
		break;
	default:break;
	}
	if ((Cont->Strobe == 1) && (!(Val & 1)))
		Cont->BitPtr = 0;
	Cont->Strobe = Val & 1;
	Port1->Write(Port1,Val);
	Port2->Write(Port2,Val);
}
static	BOOL CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)"Unconnected");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)"Unconnected");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_ADDSTRING,0,(LPARAM)"Unconnected");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_ADDSTRING,0,(LPARAM)"Unconnected");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)"Standard Controller");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)"Standard Controller");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_ADDSTRING,0,(LPARAM)"Standard Controller");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_ADDSTRING,0,(LPARAM)"Standard Controller");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)"Zapper");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)"Zapper");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_ADDSTRING,0,(LPARAM)"Zapper");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_ADDSTRING,0,(LPARAM)"Zapper");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)"Arkanoid Paddle");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)"Arkanoid Paddle");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_ADDSTRING,0,(LPARAM)"Arkanoid Paddle");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_ADDSTRING,0,(LPARAM)"Arkanoid Paddle");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)"Power Pad");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)"Power Pad");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_ADDSTRING,0,(LPARAM)"Power Pad");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_ADDSTRING,0,(LPARAM)"Power Pad");
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,Controllers.FSPort1.Type,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,Controllers.FSPort2.Type,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_SETCURSEL,Controllers.FSPort3.Type,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_SETCURSEL,Controllers.FSPort4.Type,0);
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case IDOK:
			EndDialog(hDlg,1);
			break;
		case IDC_CONT_SPORT1:	if (wmEvent == CBN_SELCHANGE) { StdPort_SetControllerType(&Controllers.FSPort1,SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_GETCURSEL,0,0)); }	break;
		case IDC_CONT_SPORT2:	if (wmEvent == CBN_SELCHANGE) { StdPort_SetControllerType(&Controllers.FSPort2,SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_GETCURSEL,0,0)); }	break;
		case IDC_CONT_SPORT3:	if (wmEvent == CBN_SELCHANGE) { StdPort_SetControllerType(&Controllers.FSPort3,SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_GETCURSEL,0,0)); }	break;
		case IDC_CONT_SPORT4:	if (wmEvent == CBN_SELCHANGE) { StdPort_SetControllerType(&Controllers.FSPort4,SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_GETCURSEL,0,0)); }	break;
		case IDC_CONT_CPORT1:	Controllers.FSPort1.Config(&Controllers.FSPort1,hDlg);	break;
		case IDC_CONT_CPORT2:	Controllers.FSPort2.Config(&Controllers.FSPort2,hDlg);	break;
		case IDC_CONT_CPORT3:	Controllers.FSPort3.Config(&Controllers.FSPort3,hDlg);	break;
		case IDC_CONT_CPORT4:	Controllers.FSPort4.Config(&Controllers.FSPort4,hDlg);	break;
		}
		break;
	}

	return FALSE;
}
static	void	Config (struct tStdPort *Cont, HWND hWnd)
{
	DialogBox(hInst,(LPCTSTR)IDD_STDPORT_FOURSCORE,hWnd,ConfigProc);
}
static	void	Unload (struct tStdPort *Cont)
{
	free(Cont->Data);
}
void	StdPort_SetFourScore (struct tStdPort *Cont)
{
	Controllers.Port1.Read = Read;
	Controllers.Port2.Read = Read;
	Controllers.Port1.Write = Write;
	Controllers.Port2.Write = Write;
	Controllers.Port1.Config = Config;
	Controllers.Port2.Config = Config;
	Controllers.Port1.Unload = Unload;
	Controllers.Port2.Unload = Unload;
	Controllers.Port1.Type = 5;
	Controllers.Port2.Type = 5;
	Controllers.Port1.DataLen = 3;
	Controllers.Port2.DataLen = 3;
	Controllers.Port1.Data = malloc(Controllers.Port1.DataLen * sizeof(Controllers.Port1.Data));
	Controllers.Port2.Data = malloc(Controllers.Port2.DataLen * sizeof(Controllers.Port2.Data));
	Controllers.Port1.BitPtr = 0;
	Controllers.Port2.BitPtr = 0;
	Controllers.Port1.Strobe = 0;
	Controllers.Port2.Strobe = 0;
	Controllers.Port1.PortNum = 0;
	Controllers.Port2.PortNum = 1;
}
#undef	BitPtr
#undef	Strobe
#undef	PortNum
