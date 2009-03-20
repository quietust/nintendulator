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
#define	BitPtr	Data[0]
#define	Strobe	Data[1]
#define	PortNum	Data[2]

static	void	AllocMov (struct tStdPort *Cont)
{
	free(Cont->MovData);
	if (Cont->PortNum == 0)
		Cont->MovLen = FSPort1.MovLen + FSPort3.MovLen;
	if (Cont->PortNum == 1)
		Cont->MovLen = FSPort2.MovLen + FSPort4.MovLen;
	Cont->MovData = (unsigned char *)malloc(Cont->MovLen * sizeof(Cont->MovData[0]));
	ZeroMemory(Cont->MovData,Cont->MovLen);
}

static	void	Frame (struct tStdPort *Cont, unsigned char mode)
{
	int x = 0, y;
	if (Cont->PortNum == 0)
	{
		if (mode & MOV_PLAY)
		{
			for (y = 0; x < FSPort1.MovLen; x++, y++)
				Cont->MovData[x] = FSPort1.MovData[y];
			for (y = 0; x < FSPort3.MovLen; x++, y++)
				Cont->MovData[x] = FSPort3.MovData[y];
		}
		FSPort1.Frame(&FSPort1,mode);
		FSPort3.Frame(&FSPort3,mode);
		if (mode & MOV_RECORD)
		{
			for (y = 0; x < FSPort1.MovLen; x++, y++)
				FSPort1.MovData[y] = Cont->MovData[x];
			for (y = 0; x < FSPort3.MovLen; x++, y++)
				FSPort3.MovData[y] = Cont->MovData[x];
		}
	}
	if (Cont->PortNum == 1)
	{
		if (mode & MOV_PLAY)
		{
			for (y = 0; x < FSPort2.MovLen; x++, y++)
				Cont->MovData[x] = FSPort2.MovData[y];
			for (y = 0; x < FSPort4.MovLen; x++, y++)
				Cont->MovData[x] = FSPort4.MovData[y];
		}
		FSPort2.Frame(&FSPort2,mode);
		FSPort4.Frame(&FSPort4,mode);
		if (mode & MOV_RECORD)
		{
			for (y = 0; x < FSPort2.MovLen; x++, y++)
				FSPort2.MovData[y] = Cont->MovData[x];
			for (y = 0; x < FSPort4.MovLen; x++, y++)
				FSPort4.MovData[y] = Cont->MovData[x];
		}
	}
}

static	unsigned char	Read (struct tStdPort *Cont)
{
	unsigned char result = 0;
	struct tStdPort *Port1 = NULL, *Port2 = NULL;
	switch (Cont->PortNum)
	{
	case 0:	Port1 = &FSPort1;
		Port2 = &FSPort3;
		break;
	case 1:	Port1 = &FSPort2;
		Port2 = &FSPort4;
		break;
	default:break;
	}
	if (Cont->Strobe)
		Cont->BitPtr = 0;
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
	case 0:	Port1 = &FSPort1;
		Port2 = &FSPort3;
		break;
	case 1:	Port1 = &FSPort2;
		Port2 = &FSPort4;
		break;
	default:break;
	}
	Cont->Strobe = Val & 1;
	if (Cont->Strobe)
		Cont->BitPtr = 0;
	Port1->Write(Port1,Val);
	Port2->Write(Port2,Val);
}
static	INT_PTR	CALLBACK	ConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_RESETCONTENT,0,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)StdPort_Mappings[STD_UNCONNECTED]);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)StdPort_Mappings[STD_UNCONNECTED]);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_ADDSTRING,0,(LPARAM)StdPort_Mappings[STD_UNCONNECTED]);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_ADDSTRING,0,(LPARAM)StdPort_Mappings[STD_UNCONNECTED]);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_ADDSTRING,0,(LPARAM)StdPort_Mappings[STD_STDCONTROLLER]);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_ADDSTRING,0,(LPARAM)StdPort_Mappings[STD_STDCONTROLLER]);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_ADDSTRING,0,(LPARAM)StdPort_Mappings[STD_STDCONTROLLER]);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_ADDSTRING,0,(LPARAM)StdPort_Mappings[STD_STDCONTROLLER]);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,FSPort1.Type,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,FSPort2.Type,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_SETCURSEL,FSPort3.Type,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_SETCURSEL,FSPort4.Type,0);
		if (Movie.Mode)
		{
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SPORT1),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SPORT2),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SPORT3),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SPORT4),FALSE);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SPORT1),TRUE);
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SPORT2),TRUE);
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SPORT3),TRUE);
			EnableWindow(GetDlgItem(hDlg,IDC_CONT_SPORT4),TRUE);
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		switch (wmId)
		{
		case IDOK:
			EndDialog(hDlg,1);
			break;
		case IDC_CONT_SPORT1:	if (wmEvent == CBN_SELCHANGE) { StdPort_SetControllerType(&FSPort1,(int)SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_GETCURSEL,0,0)); AllocMov(&Port1); AllocMov(&Port2); }	break;
		case IDC_CONT_SPORT2:	if (wmEvent == CBN_SELCHANGE) { StdPort_SetControllerType(&FSPort2,(int)SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_GETCURSEL,0,0)); AllocMov(&Port1); AllocMov(&Port2); }	break;
		case IDC_CONT_SPORT3:	if (wmEvent == CBN_SELCHANGE) { StdPort_SetControllerType(&FSPort3,(int)SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_GETCURSEL,0,0)); AllocMov(&Port1); AllocMov(&Port2); }	break;
		case IDC_CONT_SPORT4:	if (wmEvent == CBN_SELCHANGE) { StdPort_SetControllerType(&FSPort4,(int)SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_GETCURSEL,0,0)); AllocMov(&Port1); AllocMov(&Port2); }	break;
		case IDC_CONT_CPORT1:	FSPort1.Config(&FSPort1,hDlg);	break;
		case IDC_CONT_CPORT2:	FSPort2.Config(&FSPort2,hDlg);	break;
		case IDC_CONT_CPORT3:	FSPort3.Config(&FSPort3,hDlg);	break;
		case IDC_CONT_CPORT4:	FSPort4.Config(&FSPort4,hDlg);	break;
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
	free(Cont->MovData);
}
void	StdPort_SetFourScore (struct tStdPort *Cont)
{
	Cont->Read = Read;
	Cont->Write = Write;
	Cont->Config = Config;
	Cont->Unload = Unload;
	Cont->Frame = Frame;
	Cont->Type = STD_FOURSCORE;
	Cont->DataLen = 3;
	Cont->Data = (unsigned long *)malloc(Cont->DataLen * sizeof(Cont->Data[0]));
	Cont->BitPtr = 0;
	Cont->Strobe = 0;
	if (Cont == &Port1)
		Cont->PortNum = 0;
	if (Cont == &Port2)
		Cont->PortNum = 1;
	Cont->MovData = NULL;
	AllocMov(Cont);
}
#undef	BitPtr
#undef	Strobe
#undef	PortNum
} // namespace Controllers