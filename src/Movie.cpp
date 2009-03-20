/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2009 QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "MapperInterface.h"
#include "Movie.h"
#include "Controllers.h"
#include "States.h"
#include "Genie.h"
#include "NES.h"
#include "PPU.h"
#include <commdlg.h>

namespace Movie
{
	unsigned char	Mode;
	FILE *		Data;
	unsigned char	ControllerTypes[4];
	int		ReRecords;
	TCHAR		Filename[MAX_PATH];
	TCHAR *		Description;
	int		Len;
	int		Pos;
	int		FrameLen;

void	FindBlock (void)
{
	char buf[4];
	int len;

	rewind(Data);
	fseek(Data,16,SEEK_SET);
	fread(buf,4,1,Data);
	fread(&len,4,1,Data);
	while (memcmp(buf,"NMOV",4))
	{	/* find the NMOV block in the movie */
		fseek(Data,len,SEEK_CUR);
		fread(buf,4,1,Data);
		fread(&len,4,1,Data);
	}
	if (feof(Data))
		EI.DbgOut(_T("ERROR - Failed to locate movie data block!"));
}

void	ShowFrame (void)
{
	if (Mode)
		EI.StatusOut(_T("Frame %i"),Pos / FrameLen);
}

INT_PTR	CALLBACK	MoviePlayProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char buf[4];
	TCHAR str[64];
	OPENFILENAME ofn;
	TCHAR filename[MAX_PATH] = {0};
	int len;
	int wmId, wmEvent;
	BOOL resume;
	unsigned char tvmode;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		return TRUE;
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		switch (wmId)
		{
		case IDC_MOVIE_PLAY_BROWSE:
			ZeroMemory(&ofn,sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hDlg;
			ofn.hInstance = hInst;
			ofn.lpstrFilter = _T("Nintendulator Movie (*.NMV)\0") _T("*.NMV\0") _T("\0");
			ofn.lpstrCustomFilter = NULL;
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = filename;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = Path_NMV;
			ofn.Flags = OFN_FILEMUSTEXIST;
			ofn.lpstrDefExt = NULL;
			ofn.lCustData = 0;
			ofn.lpfnHook = NULL;
			ofn.lpTemplateName = NULL;

			if (!GetOpenFileName(&ofn))
				break;

			_tcscpy(Path_NMV,filename);
			Path_NMV[ofn.nFileOffset-1] = 0;

			Data = _tfopen(filename, _T("rb"));
			if (Data == NULL)
			{
				MessageBox(hDlg, _T("Unable to open movie file!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
				break;
			}

			fread(buf, 1, 4, Data);
			if (memcmp(buf, "NSS\x1a", 4))
			{
				MessageBox(hDlg, _T("Invalid movie file selected!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
				fclose(Data);
				break;
			}

			fread(buf, 1, 4, Data);
			if ((memcmp(buf, STATES_VERSION, 4)) && (memcmp(buf, STATES_BETA, 4)) && (memcmp(buf, STATES_PREV, 4)))
			{
				MessageBox(hDlg, _T("Incorrect movie version!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
				fclose(Data);
				break;
			}

			fseek(Data, 4, SEEK_CUR);
			fread(buf, 1, 4, Data);
			if (memcmp(buf, "NMOV", 4))
			{
				MessageBox(hDlg, _T("This is not a valid Nintendulator movie file!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
				fclose(Data);
				break;
			}
			SetDlgItemText(hDlg, IDC_MOVIE_PLAY_FILE, filename);

			FindBlock();
			fseek(Data, 3, SEEK_CUR);
			fread(&tvmode, 1, 1, Data);
			if (tvmode & 0x80)
				SetDlgItemText(hDlg, IDC_MOVIE_PLAY_TVMODE, _T("PAL"));
			else	SetDlgItemText(hDlg, IDC_MOVIE_PLAY_TVMODE, _T("NTSC"));
			fread(&ReRecords, 4, 1, Data);
			SetDlgItemInt(hDlg, IDC_MOVIE_PLAY_RERECORDS, ReRecords, FALSE);
			fread(&len, 4, 1, Data);
			if (len)
			{
				char *desc = (char *)malloc(len);
				int len2;
				fread(desc, len, 1, Data);
#ifdef	UNICODE
				// Windows 9x doesn't support this function natively
				len2 = MultiByteToWideChar(CP_UTF8, 0, desc, len, NULL, 0);
				Description = (TCHAR *)malloc(len2 * sizeof(TCHAR));
				if (Description)
				{
					MultiByteToWideChar(CP_UTF8, 0, desc, len, Description, len2);
					SetDlgItemText(hDlg, IDC_MOVIE_PLAY_DESCRIPTION, Description);
					free(Description);
					Description = NULL;
				}
#else	/* !UNICODE */
				len2 = 0;
#endif	/* UNICODE */
				free(desc);
			}
			fread(&Len, 4, 1, Data);
			SetDlgItemInt(hDlg, IDC_MOVIE_PLAY_FRAMES, Len, FALSE);
			_stprintf(str, _T("Length: %i:%02i.%02i"), Len / 3600, (Len % 3600) / 60, Len % 60);
			SetDlgItemText(hDlg, IDC_MOVIE_PLAY_LENGTH, str);

			fclose(Data);
			Data = NULL;
			break;
		case IDOK:
			GetDlgItemText(hDlg, IDC_MOVIE_PLAY_FILE, Filename, MAX_PATH);
			resume = IsDlgButtonChecked(hDlg, IDC_MOVIE_PLAY_RESUME);
			if (resume == BST_CHECKED)
				Data = _tfopen(Filename, _T("r+b"));
			else	Data = _tfopen(Filename, _T("rb"));
			if (Data)
			{
				if (resume)
					EndDialog(hDlg, 2);
				else	EndDialog(hDlg, 1);
			}
			else	MessageBox(hDlg, _T("Unable to open movie file!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
			break;
		case IDCANCEL:
			EndDialog(hDlg, 0);
			break;
		};
		break;
	}

	return FALSE;
}

void	Play (void)
{
	BOOL resume = FALSE, running = NES::Running;
	int ret, len;
	char buf[4];

	if (Mode)
	{
		MessageBox(hMainWnd,_T("A movie is already open!"),_T("Nintendulator"),MB_OK);
		return;
	}

	NES::Stop();

	ret = DialogBox(hInst, (LPCTSTR)IDD_MOVIE_PLAY, hMainWnd, MoviePlayProc);
	if (ret == 0)
	{
		if (running)
			NES::Start(FALSE);
		return;
	}
	if (ret == 2)
		resume = TRUE;

	fread(buf, 1, 4, Data);
	if (memcmp(buf, "NSS\x1a", 4))
	{
		MessageBox(hMainWnd, _T("Invalid movie file selected!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		fclose(Data);
		return;
	}

	fread(buf, 1, 4, Data);
	if ((memcmp(buf, STATES_VERSION, 4)) && (memcmp(buf, STATES_BETA, 4)) && (memcmp(buf, STATES_PREV, 4)))
	{
		MessageBox(hMainWnd, _T("Incorrect movie version!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		fclose(Data);
		return;
	}
	fread(&len, 4, 1, Data);
	fread(buf, 1, 4, Data);

	if (memcmp(buf, "NMOV", 4))
	{
		MessageBox(hMainWnd, _T("This is not a valid Nintendulator movie recording!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		fclose(Data);
		return;
	}

	ControllerTypes[0] = (unsigned char)Controllers::Port1.Type;
	ControllerTypes[1] = (unsigned char)Controllers::Port2.Type;
	ControllerTypes[2] = (unsigned char)Controllers::ExpPort.Type;

	if (Controllers::Port1.Type == Controllers::STD_FOURSCORE)
	{
		ControllerTypes[1] = 0;
		if (Controllers::FSPort1.Type)	ControllerTypes[1] |= 0x01;
		if (Controllers::FSPort2.Type)	ControllerTypes[1] |= 0x02;
		if (Controllers::FSPort3.Type)	ControllerTypes[1] |= 0x04;
		if (Controllers::FSPort4.Type)	ControllerTypes[1] |= 0x08;
	}

	ControllerTypes[3] = 0;	// Reset 'loaded controller state' flag

	Pos = Len = 0;

	NES::SRAM_Size = 0;		// when messing with movies, don't save SRAM

	if (RI.ROMType == ROM_FDS)	// when playing FDS movies, discard savedata; if there is savestate data, we'll be reloading it
		memcpy(NES::PRG_ROM[0x000], NES::PRG_ROM[0x400], RI.FDS_NumSides << 16);

	FindBlock();		// SPECIAL - seek to NMOV block
	fread(buf, 4, 1, Data);	// to see if Game Genie is used or not

	if (buf[3] & 0x40)		// we need to check this BEFORE reset so we can possibly swap in the menu
		NES::GameGenie = TRUE;	// this bit ONLY gets set for reset-based movies
	else	NES::GameGenie = FALSE;	// if it starts from a savestate, it's already in the game (and we load the GENI block)

	fseek(Data, 16, SEEK_SET);	// seek back to the beginning of the file (past the header)

	// cycle the mapper - special stuff might happen on load that we NEED to perform
	if (MI->Unload)
		MI->Unload();
	if (MI->Load)
		MI->Load();
	NES::Reset(RESET_HARD);
	// load savestate BEFORE enabling playback, so we don't try to load the NMOV block
	if (!States::LoadData(Data, len))
	{
		MessageBox(hMainWnd, _T("Failed to load movie!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		fclose(Data);
		return;
	}

	Mode = MOV_PLAY;
	if (resume)
		Mode |= MOV_REVIEW;
	
	FindBlock();

	fread(buf, 1, 4, Data);

	if (!ControllerTypes[3])	// ONLY parse this if we did NOT encounter a controller state block
	{				// otherwise, it would mess up the original states of the controllers
		if (buf[0] == Controllers::STD_FOURSCORE)
		{
			if (buf[1] & 0x01)
				StdPort_SetControllerType(&Controllers::FSPort1, Controllers::STD_STDCONTROLLER);
			else	StdPort_SetControllerType(&Controllers::FSPort1, Controllers::STD_UNCONNECTED);
			if (buf[1] & 0x02)
				StdPort_SetControllerType(&Controllers::FSPort2, Controllers::STD_STDCONTROLLER);
			else	StdPort_SetControllerType(&Controllers::FSPort2, Controllers::STD_UNCONNECTED);
			if (buf[1] & 0x04)
				StdPort_SetControllerType(&Controllers::FSPort3, Controllers::STD_STDCONTROLLER);
			else	StdPort_SetControllerType(&Controllers::FSPort3, Controllers::STD_UNCONNECTED);
			if (buf[1] & 0x08)
				StdPort_SetControllerType(&Controllers::FSPort4, Controllers::STD_STDCONTROLLER);
			else	StdPort_SetControllerType(&Controllers::FSPort4, Controllers::STD_UNCONNECTED);
			StdPort_SetControllerType(&Controllers::Port1, Controllers::STD_FOURSCORE);
			StdPort_SetControllerType(&Controllers::Port2, Controllers::STD_FOURSCORE);
		}
		else
		{
			StdPort_SetControllerType(&Controllers::Port1, buf[0]);
			StdPort_SetControllerType(&Controllers::Port2, buf[1]);
		}
		ExpPort_SetControllerType(&Controllers::ExpPort, buf[2]);
	}
	NES::SetCPUMode(buf[3] >> 7);	// Set to NTSC or PAL

	FrameLen = Controllers::Port1.MovLen + Controllers::Port2.MovLen + Controllers::ExpPort.MovLen;
	if (NES::HasMenu)
		FrameLen++;
	if (FrameLen != (buf[3] & 0x3F))
		MessageBox(hMainWnd, _T("The frame size specified in this movie is incorrect! This movie may not play properly!"), _T("Nintendulator"), MB_OK | MB_ICONWARNING);

	fread(&ReRecords, 4, 1, Data);
	fread(&len, 4, 1, Data);
	if (len)
	{
		char *desc = (char *)malloc(len);
		int len2;
		fread(desc, len, 1, Data);
#ifdef	UNICODE
		len2 = MultiByteToWideChar(CP_UTF8, 0, desc, len, NULL, 0);
		Description = (TCHAR *)malloc(len2 * sizeof(TCHAR));
		if (Description)
		{
			MultiByteToWideChar(CP_UTF8, 0, desc, len, Description, len2);
			EI.DbgOut(_T("Description: \"%s\""), Description);
			free(Description);
			Description = NULL;
		}
#else	/* !UNICODE */
		len2 = 0;
#endif	 /* UNICODE */
		free(desc);
	}
	EI.DbgOut(_T("Re-record count: %i"), ReRecords);
	Pos = 0;
	fread(&Len, 4, 1, Data);
	EI.DbgOut(_T("Length: %i:%02i.%02i"), Len / 3600, (Len % 3600) / 60, Len % 60);

	EnableMenuItem(hMenu, ID_MISC_PLAYMOVIE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MISC_RECORDMOVIE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MISC_STOPMOVIE, MF_ENABLED);
	EnableMenuItem(hMenu, ID_PPU_MODE_NTSC, MF_GRAYED);
	EnableMenuItem(hMenu, ID_PPU_MODE_PAL, MF_GRAYED);
	EnableMenuItem(hMenu, ID_GAME, MF_GRAYED);
	EnableMenuItem(hMenu, ID_CPU_GAMEGENIE, MF_GRAYED);
	if (Controllers::Port1.MovLen)
		memset(Controllers::Port1.MovData, 0, Controllers::Port1.MovLen);
	if (Controllers::Port2.MovLen)
		memset(Controllers::Port2.MovData, 0, Controllers::Port2.MovLen);
	if (Controllers::ExpPort.MovLen)
		memset(Controllers::ExpPort.MovData, 0, Controllers::ExpPort.MovLen);
}

INT_PTR	CALLBACK	MovieRecordProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	OPENFILENAME ofn;
	TCHAR filename[MAX_PATH] = {0};
	int wmId, wmEvent;
	int error;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_PORT1, Controllers::StdPort_Mappings[Controllers::Port1.Type]);
		if (Controllers::Port1.Type == Controllers::STD_FOURSCORE)
		{
			TCHAR conts[16] = {0};
			if (Controllers::FSPort1.Type)
				_tcscat(conts, _T(",1"));
			if (Controllers::FSPort2.Type)
				_tcscat(conts, _T(",2"));
			if (Controllers::FSPort3.Type)
				_tcscat(conts, _T(",3"));
			if (Controllers::FSPort4.Type)
				_tcscat(conts, _T(",4"));
			SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_PORT2, &conts[1]);
		}
		else	SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_PORT2, Controllers::StdPort_Mappings[Controllers::Port2.Type]);
		SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_EXPPORT, Controllers::ExpPort_Mappings[Controllers::ExpPort.Type]);
		CheckRadioButton(hDlg, IDC_MOVIE_RECORD_RESET, IDC_MOVIE_RECORD_CURRENT, IDC_MOVIE_RECORD_RESET);
#ifdef	UNICODE
		EnableWindow(GetDlgItem(hDlg, IDC_MOVIE_RECORD_DESCRIPTION), FALSE);
#endif	/* UNICODE*/
		return TRUE;
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		switch (wmId)
		{
		case IDC_MOVIE_RECORD_BROWSE:
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hDlg;
			ofn.hInstance = hInst;
			ofn.lpstrFilter = _T("Nintendulator Movie (*.NMV)\0") _T("*.NMV\0") _T("\0");
			ofn.lpstrCustomFilter = NULL;
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = filename;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = Path_NMV;
			ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
			ofn.lpstrDefExt = _T("NMV");
			ofn.lCustData = 0;
			ofn.lpfnHook = NULL;
			ofn.lpTemplateName = NULL;

			if (!GetSaveFileName(&ofn))
			{
				error = CommDlgExtendedError();
				break;
			}

			_tcscpy(Path_NMV, filename);
			Path_NMV[ofn.nFileOffset-1] = 0;
			SetDlgItemText(hDlg, IDC_MOVIE_RECORD_FILE, filename);
			break;
		case IDC_MOVIE_RECORD_CONT_CONFIG:
			Controllers::OpenConfig();
			SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_PORT1, Controllers::StdPort_Mappings[Controllers::Port1.Type]);
			if (Controllers::Port1.Type == Controllers::STD_FOURSCORE)
			{
				TCHAR conts[16] = {0};
				if (Controllers::FSPort1.Type)
					_tcscat(conts, _T(",1"));
				if (Controllers::FSPort2.Type)
					_tcscat(conts, _T(",2"));
				if (Controllers::FSPort3.Type)
					_tcscat(conts, _T(",3"));
				if (Controllers::FSPort4.Type)
					_tcscat(conts, _T(",4"));
				SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_PORT2, &conts[1]);
			}
			else	SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_PORT2, Controllers::StdPort_Mappings[Controllers::Port2.Type]);
			SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_EXPPORT, Controllers::ExpPort_Mappings[Controllers::ExpPort.Type]);
			break;
		case IDOK:
			GetDlgItemText(hDlg, IDC_MOVIE_RECORD_FILE, Filename, MAX_PATH);
			Data = _tfopen(Filename, _T("w+b"));
			if (Data)
			{
#ifdef	UNICODE
				int len = GetWindowTextLength(GetDlgItem(hDlg, IDC_MOVIE_RECORD_DESCRIPTION)) + 1;
				Description = (TCHAR *)malloc(len * sizeof(TCHAR));
				if (Description)
					GetDlgItemText(hDlg, IDC_MOVIE_RECORD_DESCRIPTION, Description, len);
#else	/* !UNICODE */
				// Non-Unicode version currently cannot display movie description
				Description = NULL;
#endif	/* UNICODE */
				if (IsDlgButtonChecked(hDlg, IDC_MOVIE_RECORD_CURRENT))
					EndDialog(hDlg, 2);
				else	EndDialog(hDlg, 1);
			}
			else	MessageBox(hDlg, _T("Unable to create movie file!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
			break;
		case IDCANCEL:
			EndDialog(hDlg, 0);
			break;
		};
		break;
	}

	return FALSE;
}

void	Record (void)
{
	BOOL fromState = FALSE, running = NES::Running;
	int ret, len, x;

	if (Mode)
	{
		MessageBox(hMainWnd, _T("A movie is already open!"), _T("Nintendulator"), MB_OK);
		return;
	}

	NES::Stop();

	ret = DialogBox(hInst, (LPCTSTR)IDD_MOVIE_RECORD, hMainWnd, MovieRecordProc);
	if (ret == 0)
	{
		if (running)
			NES::Start(FALSE);
		return;
	}
	if (ret == 2)
		fromState = TRUE;

	if ((MI) && (MI->Config) && (!NES::HasMenu))
		EnableMenuItem(hMenu, ID_GAME, MF_GRAYED);
	
	len = 0;
	ReRecords = 0;
	Pos = Len = 0;

	NES::SRAM_Size = 0;		// when messing with movies, don't save SRAM

	fwrite("NSS\x1A", 1, 4, Data);
	fwrite(STATES_VERSION, 1, 4, Data);
	fwrite(&len, 1, 4, Data);
	fwrite("NMOV", 1, 4, Data);

	if (fromState)
		States::SaveData(Data);
	else
	{
		// cycle the mapper - special stuff might happen on load that we NEED to perform
		if (MI->Unload)
			MI->Unload();
		if (MI->Load)
			MI->Load();
		if (RI.ROMType == ROM_FDS)	// if recording an FDS movie from reset, discard all savedata
			memcpy(NES::PRG_ROM[0x000], NES::PRG_ROM[0x400], RI.FDS_NumSides << 16);
		NES::Reset(RESET_HARD);
	}
	// save savestate BEFORE enabling recording, so we don't try to re-save the NMOV block
	Mode = MOV_RECORD;

	fwrite("NMOV", 1, 4, Data);
	fwrite(&len, 1, 4, Data);
	
	ControllerTypes[0] = (unsigned char)Controllers::Port1.Type;
	ControllerTypes[1] = (unsigned char)Controllers::Port2.Type;
	ControllerTypes[2] = (unsigned char)Controllers::ExpPort.Type;

	if (ControllerTypes[0] == Controllers::STD_FOURSCORE)
	{
		ControllerTypes[1] = 0;
		if (Controllers::FSPort1.Type)	ControllerTypes[1] |= 0x01;
		if (Controllers::FSPort2.Type)	ControllerTypes[1] |= 0x02;
		if (Controllers::FSPort3.Type)	ControllerTypes[1] |= 0x04;
		if (Controllers::FSPort4.Type)	ControllerTypes[1] |= 0x08;
	}
	fwrite(ControllerTypes, 1, 3, Data);
	x = PPU::IsPAL ? 0x80 : 0x00;
	FrameLen = 0;
	FrameLen += Controllers::Port1.MovLen;
	FrameLen += Controllers::Port2.MovLen;
	FrameLen += Controllers::ExpPort.MovLen;
	if (NES::HasMenu)
		FrameLen++;
	x |= FrameLen;
	if (NES::GameGenie && !fromState)	// set Game Genie bit only if recording from reset
		x |= 0x40;			// - otherwise we can just detect it from the GENI block
	fwrite(&x, 1, 1, Data);		// frame size, NTSC/PAL, Game Genie
	fwrite(&ReRecords, 4, 1, Data);// rerecord count
#ifdef	UNICODE
	if (Description)
	{
		char *desc;
		len = WideCharToMultiByte(CP_UTF8, 0, Description, -1, NULL, 0, NULL, NULL);
		desc = (char *)malloc(len);
		WideCharToMultiByte(CP_UTF8, 0, Description, -1, desc, len, NULL, NULL);
		fwrite(&len, 4, 1, Data);		// comment length
		fwrite(desc, len, 1, Data);	// comment data
		free(desc);
	}
	else
	{
		len = 0;
		fwrite(&len, 4, 1, Data);		// no comment
	}
#else	/* !UNICODE */
	len = 0;
	fwrite(&len, 4, 1, Data);		// no comment
#endif	/* UNICODE */
	fwrite(&Len, 4, 1, Data);	// movie data length (this gets filled in later)

	EnableMenuItem(hMenu, ID_MISC_PLAYMOVIE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MISC_RECORDMOVIE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MISC_STOPMOVIE, MF_ENABLED);
	EnableMenuItem(hMenu, ID_CPU_GAMEGENIE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_PPU_MODE_NTSC, MF_GRAYED);
	EnableMenuItem(hMenu, ID_PPU_MODE_PAL, MF_GRAYED);

	if (Description)
	{
		free(Description);	// don't need this anymore
		Description = NULL;
	}
}

static	void	EndMovie (void)
{
	if (!Mode)
	{
		MessageBox(hMainWnd,_T("No movie is currently active!"),_T("Nintendulator"),MB_OK);
		return;
	}
	if (Mode & MOV_RECORD)
	{
		int len = ftell(Data);
		int mlen;
		char tps[4];

		fseek(Data,8,SEEK_SET);		len -= 16;
		fwrite(&len,4,1,Data);		// 1: set the file length
		fseek(Data,16,SEEK_SET);
		fread(tps,4,1,Data);
		fread(&mlen,4,1,Data);		len -= 8;
		while (memcmp(tps,"NMOV",4))
		{	/* find the NMOV block in the movie */
			fseek(Data,mlen,SEEK_CUR);len -= mlen;
			fread(tps,4,1,Data);
			fread(&mlen,4,1,Data);	len -= 8;
		}
		fseek(Data,-4,SEEK_CUR);
		fwrite(&len,4,1,Data);		// 2: set the NMOV block length
		fseek(Data,4,SEEK_CUR);		len -= 4;	// skip controller data
		fwrite(&ReRecords,4,1,Data);len -= 4;	// write the final re-record count
		fseek(Data,ftell(Data),SEEK_SET);
		fread(&mlen,4,1,Data);		len -= 4;	// read description len
		if (mlen)
		{
			fseek(Data,mlen,SEEK_CUR);len -= mlen;	// skip the description
		}
		fread(tps,4,1,Data);		len -= 4;	// read movie data len
		fseek(Data,-4,SEEK_CUR);		// rewind
		if (len != Pos)
			EI.DbgOut(_T("Error - movie length mismatch!"));
		fwrite(&len,4,1,Data);		// 3: terminate the movie data
		// fseek(Data,len,SEEK_CUR);
		// TODO - truncate the file to this point
	}
	fclose(Data);

	if (Mode & MOV_PLAY)
		EI.DbgOut(_T("Movie playback stopped."));
	if (Mode & MOV_RECORD)
		EI.DbgOut(_T("Movie recording stopped."));

	Mode = 0;

	// Restore controller types to whatever was configured prior to movie playback
	if (ControllerTypes[0] == Controllers::STD_FOURSCORE)
	{
		if (ControllerTypes[1] & 0x01)
			StdPort_SetControllerType(&Controllers::FSPort1,Controllers::STD_STDCONTROLLER);
		else	StdPort_SetControllerType(&Controllers::FSPort1,Controllers::STD_UNCONNECTED);
		if (ControllerTypes[1] & 0x02)
			StdPort_SetControllerType(&Controllers::FSPort2,Controllers::STD_STDCONTROLLER);
		else	StdPort_SetControllerType(&Controllers::FSPort2,Controllers::STD_UNCONNECTED);
		if (ControllerTypes[1] & 0x04)
			StdPort_SetControllerType(&Controllers::FSPort3,Controllers::STD_STDCONTROLLER);
		else	StdPort_SetControllerType(&Controllers::FSPort3,Controllers::STD_UNCONNECTED);
		if (ControllerTypes[1] & 0x08)
			StdPort_SetControllerType(&Controllers::FSPort4,Controllers::STD_STDCONTROLLER);
		else	StdPort_SetControllerType(&Controllers::FSPort4,Controllers::STD_UNCONNECTED);
		StdPort_SetControllerType(&Controllers::Port1,Controllers::STD_FOURSCORE);
		StdPort_SetControllerType(&Controllers::Port2,Controllers::STD_FOURSCORE);
	}
	else
	{
		StdPort_SetControllerType(&Controllers::Port1,ControllerTypes[0]);
		StdPort_SetControllerType(&Controllers::Port2,ControllerTypes[1]);
	}
	ExpPort_SetControllerType(&Controllers::ExpPort,ControllerTypes[2]);
	if ((MI) && (MI->Config))
		EnableMenuItem(hMenu,ID_GAME,MF_ENABLED);
	EnableMenuItem(hMenu,ID_MISC_PLAYMOVIE,MF_ENABLED);
	EnableMenuItem(hMenu,ID_MISC_RECORDMOVIE,MF_ENABLED);
	EnableMenuItem(hMenu,ID_MISC_STOPMOVIE,MF_GRAYED);
	EnableMenuItem(hMenu,ID_CPU_GAMEGENIE,MF_ENABLED);
	EnableMenuItem(hMenu,ID_PPU_MODE_NTSC,MF_ENABLED);
	EnableMenuItem(hMenu,ID_PPU_MODE_PAL,MF_ENABLED);
}

void	Stop (void)
{
	BOOL running = NES::Running;
	NES::Stop();
	EndMovie();
	if (running)
		NES::Start(FALSE);
}

unsigned char	LoadInput (void)
{
	unsigned char Cmd = 0;
	if (Pos >= Len)
	{
		if (Pos == Len)
			PrintTitlebar(_T("Movie stopped."));
		else	PrintTitlebar(_T("Unexpected EOF in movie!"));
		EndMovie();
	}
	if (Controllers::Port1.MovLen)
		fread(Controllers::Port1.MovData,1,Controllers::Port1.MovLen,Data);		Pos += Controllers::Port1.MovLen;
	if (Controllers::Port2.MovLen)
		fread(Controllers::Port2.MovData,1,Controllers::Port2.MovLen,Data);		Pos += Controllers::Port2.MovLen;
	if (Controllers::ExpPort.MovLen)
		fread(Controllers::ExpPort.MovData,1,Controllers::ExpPort.MovLen,Data);	Pos += Controllers::ExpPort.MovLen;
	if (NES::HasMenu)
	{
		fread(&Cmd,1,1,Data);
		Pos++;
	}
	return Cmd;
}

void	SaveInput (unsigned char Cmd)
{
	int len = 0;
	if (Controllers::Port1.MovLen)
		fwrite(Controllers::Port1.MovData,1,Controllers::Port1.MovLen,Data);	len += Controllers::Port1.MovLen;
	if (Controllers::Port2.MovLen)
		fwrite(Controllers::Port2.MovData,1,Controllers::Port2.MovLen,Data);	len += Controllers::Port2.MovLen;
	if (Controllers::ExpPort.MovLen)
		fwrite(Controllers::ExpPort.MovData,1,Controllers::ExpPort.MovLen,Data);	len += Controllers::ExpPort.MovLen;
	if (NES::HasMenu)
	{
		fwrite(&Cmd,1,1,Data);
		len++;
	}
	Pos += len;
	Len += len;
}

int	Save (FILE *out)
{
	int clen = 0, offset;

	unsigned char tpc;
	unsigned long tpl;
	int tpi;

	offset = ftell(Data);	// save old offset in movie file

	FindBlock();

	fread(&tpc,1,1,Data);	fwrite(&tpc,1,1,out);	clen++;
	fread(&tpc,1,1,Data);	fwrite(&tpc,1,1,out);	clen++;
	fread(&tpc,1,1,Data);	fwrite(&tpc,1,1,out);	clen++;
	fread(&tpc,1,1,Data);	fwrite(&tpc,1,1,out);	clen++;
	fread(&tpl,4,1,Data);	fwrite(&ReRecords,4,1,out);	clen += 4;	// ignore rerecord count stored in movie

	fread(&tpl,4,1,Data);	fwrite(&tpl,4,1,out);	clen += 4;
	tpi = tpl;						clen += tpi;
	while (tpi > 0)
	{
		fread(&tpc,1,1,Data);
		fwrite(&tpc,1,1,out);
		tpi--;
	}

	fread(&tpl,4,1,Data);				clen += 4;	// the MLEN field, which is NOT yet accurate
	fwrite(&Pos,4,1,out);
	
	tpi = Pos;					clen += tpi;
	while (tpi > 0)
	{
		fread(&tpc,1,1,Data);
		fwrite(&tpc,1,1,out);
		tpi--;
	}
	rewind(Data);
	fseek(Data,offset,SEEK_SET);		// seek the movie to where it left off

	return clen;
}

int	Load (FILE *in)
{
	int clen = 0;

	int tpi;
	unsigned long tpl;
	unsigned char tpc;
	unsigned char Cmd;

	if (Mode & MOV_RECORD)
	{	// zoom to the correct position and update the necessary fields along the way
		FindBlock();
		fseek(Data,0,SEEK_CUR);
		fread(&tpl,4,1,in);	fwrite(&tpl,4,1,Data);	clen += 4;	// CTRL0, CTRL1, CTEXT, EXTR
		fread(&tpl,4,1,in);	fwrite(&tpl,4,1,Data);	clen += 4;	// RREC
		if (ReRecords < (int)tpl)
			ReRecords = tpl;
		ReRecords++;
		fread(&tpl,4,1,in);	fwrite(&tpl,4,1,Data);	clen += 4;	// ILEN
		tpi = tpl;						clen += tpi;	// INFO
		while (tpi > 0)
		{
			fread(&tpc,1,1,in);
			fwrite(&tpc,1,1,Data);
			tpi--;
		}
		fread(&Pos,4,1,in);	fwrite(&Pos,4,1,Data);	clen += 4;	// MLEN
		tpi = Pos;					clen += tpi;	// MDAT
		Cmd = 0;
		while (tpi > 0)
		{
			if (Controllers::Port1.MovLen)
			{
				fread(Controllers::Port1.MovData,1,Controllers::Port1.MovLen,in);
				fwrite(Controllers::Port1.MovData,1,Controllers::Port1.MovLen,Data);
				tpi -= Controllers::Port1.MovLen;
			}
			if (Controllers::Port2.MovLen)
			{
				fread(Controllers::Port2.MovData,1,Controllers::Port2.MovLen,in);
				fwrite(Controllers::Port2.MovData,1,Controllers::Port2.MovLen,Data);
				tpi -= Controllers::Port2.MovLen;
			}
			if (Controllers::ExpPort.MovLen)
			{
				fread(Controllers::ExpPort.MovData,1,Controllers::ExpPort.MovLen,in);
				fwrite(Controllers::ExpPort.MovData,1,Controllers::ExpPort.MovLen,Data);
				tpi -= Controllers::ExpPort.MovLen;
			}
			if (NES::HasMenu)
			{
				fread(&Cmd,1,1,in);
				fwrite(&Cmd,1,1,Data);
				tpi--;
			}
		}
		Controllers::Port1.Frame(&Controllers::Port1,MOV_PLAY);
		Controllers::Port2.Frame(&Controllers::Port2,MOV_PLAY);
		Controllers::ExpPort.Frame(&Controllers::ExpPort,MOV_PLAY);
		if ((Cmd) && (MI) && (MI->Config))
			MI->Config(CFG_CMD,Cmd);
	}
	else if (Mode & MOV_PLAY)
	{	// zoom the movie file to the current playback position
		FindBlock();
		fread(&tpl,4,1,in);	fread(&tpl,4,1,Data);		clen += 4;	// CTRL0, CTRL1, CTEXT, EXTR
		fread(&tpl,4,1,in);	fread(&tpl,4,1,Data);		clen += 4;	// RREC
		fread(&tpl,4,1,in);	fread(&tpi,4,1,Data);		clen += 4;	// ILEN
		fseek(in,tpl,SEEK_CUR);	fseek(Data,tpi,SEEK_CUR);	clen += tpl;	// INFO
		fread(&Pos,4,1,in);	fread(&Len,4,1,Data);		clen += 4;	// MLEN
		tpi = Pos;	fseek(Data,tpi,SEEK_CUR);		clen += tpi;	// MDAT
		Cmd = 0;
		while (tpi > 0)
		{
			if (Controllers::Port1.MovLen)
			{
				fread(Controllers::Port1.MovData,1,Controllers::Port1.MovLen,in);
				tpi -= Controllers::Port1.MovLen;
			}
			if (Controllers::Port2.MovLen)
			{
				fread(Controllers::Port2.MovData,1,Controllers::Port2.MovLen,in);
				tpi -= Controllers::Port2.MovLen;
			}
			if (Controllers::ExpPort.MovLen)
			{
				fread(Controllers::ExpPort.MovData,1,Controllers::ExpPort.MovLen,in);
				tpi -= Controllers::ExpPort.MovLen;
			}
			if (NES::HasMenu)
			{
				fread(&Cmd,1,1,in);
				tpi--;
			}
		}
		Controllers::Port1.Frame(&Controllers::Port1,MOV_PLAY);
		Controllers::Port2.Frame(&Controllers::Port2,MOV_PLAY);
		Controllers::ExpPort.Frame(&Controllers::ExpPort,MOV_PLAY);
		if ((Cmd) && (MI) && (MI->Config))
			MI->Config(CFG_CMD,Cmd);
	}
	else
	{
		fseek(in,8,SEEK_CUR);	clen += 8;
		fread(&tpl,4,1,in);	clen += 4;
		fseek(in,tpl,SEEK_CUR);	clen += tpl;
		fread(&tpl,4,1,in);	clen += 4;
		fseek(in,tpl,SEEK_CUR);	clen += tpl;
	}
	return clen;
}
} // namespace Movie