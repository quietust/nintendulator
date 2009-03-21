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
#include <pshpack1.h>
struct StdPort_FourScore_State
{
	unsigned char BitPtr;
	unsigned char Strobe;
};
#include <poppack.h>
#define State ((StdPort_FourScore_State *)Data)

static	void	AllocMov1 (StdPort *Cont)
{
	if (Cont->MovData)
		free(Cont->MovData);
	Cont->MovLen = FSPort1->MovLen + FSPort3->MovLen;
	Cont->MovData = (unsigned char *)malloc(Cont->MovLen);
	ZeroMemory(Cont->MovData, Cont->MovLen);
}
static	void	AllocMov2 (StdPort *Cont)
{
	if (Cont->MovData)
		free(Cont->MovData);
	Cont->MovLen = FSPort2->MovLen + FSPort4->MovLen;
	Cont->MovData = (unsigned char *)malloc(Cont->MovLen);
	ZeroMemory(Cont->MovData, Cont->MovLen);
}
void	StdPort_FourScore::Frame (unsigned char mode)
{
	int x = 0, y;
	if (mode & MOV_PLAY)
	{
		for (y = 0; x < FSPort1->MovLen; x++, y++)
			MovData[x] = FSPort1->MovData[y];
		for (y = 0; x < FSPort3->MovLen; x++, y++)
			MovData[x] = FSPort3->MovData[y];
	}
	FSPort1->Frame(mode);
	FSPort3->Frame(mode);
	if (mode & MOV_RECORD)
	{
		for (y = 0; x < FSPort1->MovLen; x++, y++)
			FSPort1->MovData[y] = MovData[x];
		for (y = 0; x < FSPort3->MovLen; x++, y++)
			FSPort3->MovData[y] = MovData[x];
	}
}
void	StdPort_FourScore2::Frame (unsigned char mode)
{
	int x = 0, y;
	if (mode & MOV_PLAY)
	{
		for (y = 0; x < FSPort2->MovLen; x++, y++)
			MovData[x] = FSPort2->MovData[y];
		for (y = 0; x < FSPort4->MovLen; x++, y++)
			MovData[x] = FSPort4->MovData[y];
	}
	FSPort2->Frame(mode);
	FSPort4->Frame(mode);
	if (mode & MOV_RECORD)
	{
		for (y = 0; x < FSPort2->MovLen; x++, y++)
			FSPort2->MovData[y] = MovData[x];
		for (y = 0; x < FSPort4->MovLen; x++, y++)
			FSPort4->MovData[y] = MovData[x];
	}
}

unsigned char	StdPort_FourScore::Read (void)
{
	unsigned char result = 0;
	if (State->Strobe)
		State->BitPtr = 0;
	switch (State->BitPtr)
	{
	case  0:case  1:case  2:case  3:case  4:case  5:case  6:case  7:
		result = FSPort1->Read();
		break;
	case  8:case  9:case 10:case 11:case 12:case 13:case 14:case 15:
		result = FSPort3->Read();
		break;
	case 19:
		result = 1;
		break;
	}
	if (State->BitPtr < 20)
		State->BitPtr++;
	return result;
}
unsigned char	StdPort_FourScore2::Read (void)
{
	unsigned char result = 0;
	if (State->Strobe)
		State->BitPtr = 0;
	switch (State->BitPtr)
	{
	case  0:case  1:case  2:case  3:case  4:case  5:case  6:case  7:
		result = FSPort2->Read();
		break;
	case  8:case  9:case 10:case 11:case 12:case 13:case 14:case 15:
		result = FSPort4->Read();
		break;
	case 18:
		result = 1;
		break;
	}
	if (State->BitPtr < 20)
		State->BitPtr++;
	return result;
}

void	StdPort_FourScore::Write (unsigned char Val)
{
	State->Strobe = Val & 1;
	if (State->Strobe)
		State->BitPtr = 0;
	FSPort1->Write(Val);
	FSPort3->Write(Val);
}
void	StdPort_FourScore2::Write (unsigned char Val)
{
	State->Strobe = Val & 1;
	if (State->Strobe)
		State->BitPtr = 0;
	FSPort2->Write(Val);
	FSPort4->Write(Val);
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
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_SETCURSEL,FSPort1->Type,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_SETCURSEL,FSPort2->Type,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_SETCURSEL,FSPort3->Type,0);
		SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_SETCURSEL,FSPort4->Type,0);
		if (Movie::Mode)
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
		case IDC_CONT_SPORT1:	if (wmEvent == CBN_SELCHANGE) { SET_STDCONT(FSPort1,(STDCONT_TYPE)SendDlgItemMessage(hDlg,IDC_CONT_SPORT1,CB_GETCURSEL,0,0)); AllocMov1(Port1); AllocMov2(Port2); }	break;
		case IDC_CONT_SPORT2:	if (wmEvent == CBN_SELCHANGE) { SET_STDCONT(FSPort2,(STDCONT_TYPE)SendDlgItemMessage(hDlg,IDC_CONT_SPORT2,CB_GETCURSEL,0,0)); AllocMov1(Port1); AllocMov2(Port2); }	break;
		case IDC_CONT_SPORT3:	if (wmEvent == CBN_SELCHANGE) { SET_STDCONT(FSPort3,(STDCONT_TYPE)SendDlgItemMessage(hDlg,IDC_CONT_SPORT3,CB_GETCURSEL,0,0)); AllocMov1(Port1); AllocMov2(Port2); }	break;
		case IDC_CONT_SPORT4:	if (wmEvent == CBN_SELCHANGE) { SET_STDCONT(FSPort4,(STDCONT_TYPE)SendDlgItemMessage(hDlg,IDC_CONT_SPORT4,CB_GETCURSEL,0,0)); AllocMov1(Port1); AllocMov2(Port2); }	break;
		case IDC_CONT_CPORT1:	FSPort1->Config(hDlg);	break;
		case IDC_CONT_CPORT2:	FSPort2->Config(hDlg);	break;
		case IDC_CONT_CPORT3:	FSPort3->Config(hDlg);	break;
		case IDC_CONT_CPORT4:	FSPort4->Config(hDlg);	break;
		}
		break;
	}

	return FALSE;
}
void	StdPort_FourScore::Config (HWND hWnd)
{
	DialogBox(hInst,(LPCTSTR)IDD_STDPORT_FOURSCORE,hWnd,ConfigProc);
}
void	StdPort_FourScore2::Config (HWND hWnd)
{
	DialogBox(hInst,(LPCTSTR)IDD_STDPORT_FOURSCORE,hWnd,ConfigProc);
}
StdPort_FourScore::~StdPort_FourScore (void)
{
	free(Data);
	free(MovData);
}
StdPort_FourScore2::~StdPort_FourScore2 (void)
{
	free(Data);
	free(MovData);
}
void	StdPort_FourScore::Init (int *buttons)
{
	Type = STD_FOURSCORE;
	NumButtons = 0;
	Buttons = buttons;
	DataLen = sizeof(*State);
	Data = malloc(DataLen);
	State->BitPtr = 0;
	State->Strobe = 0;
	MovData = NULL;
	AllocMov1(this);
}
void	StdPort_FourScore2::Init (int *buttons)
{
	Type = STD_FOURSCORE2;
	NumButtons = 0;
	Buttons = buttons;
	DataLen = sizeof(*State);
	Data = malloc(DataLen);
	State->BitPtr = 0;
	State->Strobe = 0;
	MovData = NULL;
	AllocMov2(this);
}
} // namespace Controllers