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

struct	tMovie	Movie;

void	Movie_FindBlock (void)
{
	char buf[4];
	int len;

	rewind(Movie.Data);
	fseek(Movie.Data,16,SEEK_SET);
	fread(buf,4,1,Movie.Data);
	fread(&len,4,1,Movie.Data);
	while (memcmp(buf,"NMOV",4))
	{	/* find the NMOV block in the movie */
		fseek(Movie.Data,len,SEEK_CUR);
		fread(buf,4,1,Movie.Data);
		fread(&len,4,1,Movie.Data);
	}
	if (feof(Movie.Data))
		EI.DbgOut(_T("ERROR - Failed to locate movie data block!"));
}

void	Movie_ShowFrame (void)
{
	if (Movie.Mode)
		EI.StatusOut(_T("Frame %i"),Movie.Pos / Movie.FrameLen);
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

			Movie.Data = _tfopen(filename, _T("rb"));
			if (Movie.Data == NULL)
			{
				MessageBox(hDlg, _T("Unable to open movie file!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
				break;
			}

			fread(buf, 1, 4, Movie.Data);
			if (memcmp(buf, "NSS\x1a", 4))
			{
				MessageBox(hDlg, _T("Invalid movie file selected!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
				fclose(Movie.Data);
				break;
			}

			fread(buf, 1, 4, Movie.Data);
			if ((memcmp(buf, STATES_VERSION, 4)) && (memcmp(buf, STATES_BETA, 4)) && (memcmp(buf, STATES_PREV, 4)))
			{
				MessageBox(hDlg, _T("Incorrect movie version!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
				fclose(Movie.Data);
				break;
			}

			fseek(Movie.Data, 4, SEEK_CUR);
			fread(buf, 1, 4, Movie.Data);
			if (memcmp(buf, "NMOV", 4))
			{
				MessageBox(hDlg, _T("This is not a valid Nintendulator movie file!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
				fclose(Movie.Data);
				break;
			}
			SetDlgItemText(hDlg, IDC_MOVIE_PLAY_FILE, filename);

			Movie_FindBlock();
			fseek(Movie.Data, 3, SEEK_CUR);
			fread(&tvmode, 1, 1, Movie.Data);
			if (tvmode & 0x80)
				SetDlgItemText(hDlg, IDC_MOVIE_PLAY_TVMODE, _T("PAL"));
			else	SetDlgItemText(hDlg, IDC_MOVIE_PLAY_TVMODE, _T("NTSC"));
			fread(&Movie.ReRecords, 4, 1, Movie.Data);
			SetDlgItemInt(hDlg, IDC_MOVIE_PLAY_RERECORDS, Movie.ReRecords, FALSE);
			fread(&len, 4, 1, Movie.Data);
			if (len)
			{
				char *desc = malloc(len);
				int len2;
				fread(desc, len, 1, Movie.Data);
#ifdef	UNICODE
				// Windows 9x doesn't support this function natively
				len2 = MultiByteToWideChar(CP_UTF8, 0, desc, len, NULL, 0);
				Movie.Description = malloc(len2 * sizeof(TCHAR));
				if (Movie.Description)
				{
					MultiByteToWideChar(CP_UTF8, 0, desc, len, Movie.Description, len2);
					SetDlgItemText(hDlg, IDC_MOVIE_PLAY_DESCRIPTION, Movie.Description);
					free(Movie.Description);
					Movie.Description = NULL;
				}
#else	/* !UNICODE */
				len2 = 0;
#endif	/* UNICODE */
				free(desc);
			}
			fread(&Movie.Len, 4, 1, Movie.Data);
			SetDlgItemInt(hDlg, IDC_MOVIE_PLAY_FRAMES, Movie.Len, FALSE);
			_stprintf(str, _T("Length: %i:%02i.%02i"), Movie.Len / 3600, (Movie.Len % 3600) / 60, Movie.Len % 60);
			SetDlgItemText(hDlg, IDC_MOVIE_PLAY_LENGTH, str);

			fclose(Movie.Data);
			Movie.Data = NULL;
			break;
		case IDOK:
			GetDlgItemText(hDlg, IDC_MOVIE_PLAY_FILE, Movie.Filename, MAX_PATH);
			resume = IsDlgButtonChecked(hDlg, IDC_MOVIE_PLAY_RESUME);
			if (resume == BST_CHECKED)
				Movie.Data = _tfopen(Movie.Filename, _T("r+b"));
			else	Movie.Data = _tfopen(Movie.Filename, _T("rb"));
			if (Movie.Data)
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

void	Movie_Play (void)
{
	BOOL resume = FALSE, running = NES.Running;
	int ret, len;
	char buf[4];

	if (Movie.Mode)
	{
		MessageBox(hMainWnd,_T("A movie is already open!"),_T("Nintendulator"),MB_OK);
		return;
	}

	NES_Stop();

	ret = DialogBox(hInst, (LPCTSTR)IDD_MOVIE_PLAY, hMainWnd, MoviePlayProc);
	if (ret == 0)
	{
		if (running)
			NES_Start(FALSE);
		return;
	}
	if (ret == 2)
		resume = TRUE;

	fread(buf, 1, 4, Movie.Data);
	if (memcmp(buf, "NSS\x1a", 4))
	{
		MessageBox(hMainWnd, _T("Invalid movie file selected!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		fclose(Movie.Data);
		return;
	}

	fread(buf, 1, 4, Movie.Data);
	if ((memcmp(buf, STATES_VERSION, 4)) && (memcmp(buf, STATES_BETA, 4)) && (memcmp(buf, STATES_PREV, 4)))
	{
		MessageBox(hMainWnd, _T("Incorrect movie version!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		fclose(Movie.Data);
		return;
	}
	fread(&len, 4, 1, Movie.Data);
	fread(buf, 1, 4, Movie.Data);

	if (memcmp(buf, "NMOV", 4))
	{
		MessageBox(hMainWnd, _T("This is not a valid Nintendulator movie recording!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		fclose(Movie.Data);
		return;
	}

	Movie.ControllerTypes[0] = (unsigned char)Controllers.Port1.Type;
	Movie.ControllerTypes[1] = (unsigned char)Controllers.Port2.Type;
	Movie.ControllerTypes[2] = (unsigned char)Controllers.ExpPort.Type;

	if (Controllers.Port1.Type == STD_FOURSCORE)
	{
		Movie.ControllerTypes[1] = 0;
		if (Controllers.FSPort1.Type)	Movie.ControllerTypes[1] |= 0x01;
		if (Controllers.FSPort2.Type)	Movie.ControllerTypes[1] |= 0x02;
		if (Controllers.FSPort3.Type)	Movie.ControllerTypes[1] |= 0x04;
		if (Controllers.FSPort4.Type)	Movie.ControllerTypes[1] |= 0x08;
	}

	Movie.ControllerTypes[3] = 0;	// Reset 'loaded controller state' flag

	Movie.Pos = Movie.Len = 0;

	NES.SRAM_Size = 0;		// when messing with movies, don't save SRAM

	if (RI.ROMType == ROM_FDS)	// when playing FDS movies, discard savedata; if there is savestate data, we'll be reloading it
		memcpy(PRG_ROM[0x000], PRG_ROM[0x400], RI.FDS_NumSides << 16);

	Movie_FindBlock();		// SPECIAL - seek to NMOV block
	fread(buf, 4, 1, Movie.Data);	// to see if Game Genie is used or not

	if (buf[3] & 0x40)		// we need to check this BEFORE reset so we can possibly swap in the menu
		NES.GameGenie = TRUE;	// this bit ONLY gets set for reset-based movies
	else	NES.GameGenie = FALSE;	// if it starts from a savestate, it's already in the game (and we load the GENI block)

	fseek(Movie.Data, 16, SEEK_SET);	// seek back to the beginning of the file (past the header)

	// cycle the mapper - special stuff might happen on load that we NEED to perform
	if (MI->Unload)
		MI->Unload();
	if (MI->Load)
		MI->Load();
	NES_Reset(RESET_HARD);
	// load savestate BEFORE enabling playback, so we don't try to load the NMOV block
	if (!States_LoadData(Movie.Data, len))
	{
		MessageBox(hMainWnd, _T("Failed to load movie!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		fclose(Movie.Data);
		return;
	}

	Movie.Mode = MOV_PLAY;
	if (resume)
		Movie.Mode |= MOV_REVIEW;
	
	Movie_FindBlock();

	fread(buf, 1, 4, Movie.Data);

	if (!Movie.ControllerTypes[3])	// ONLY parse this if we did NOT encounter a controller state block
	{				// otherwise, it would mess up the original states of the controllers
		if (buf[0] == STD_FOURSCORE)
		{
			if (buf[1] & 0x01)
				StdPort_SetControllerType(&Controllers.FSPort1, STD_STDCONTROLLER);
			else	StdPort_SetControllerType(&Controllers.FSPort1, STD_UNCONNECTED);
			if (buf[1] & 0x02)
				StdPort_SetControllerType(&Controllers.FSPort2, STD_STDCONTROLLER);
			else	StdPort_SetControllerType(&Controllers.FSPort2, STD_UNCONNECTED);
			if (buf[1] & 0x04)
				StdPort_SetControllerType(&Controllers.FSPort3, STD_STDCONTROLLER);
			else	StdPort_SetControllerType(&Controllers.FSPort3, STD_UNCONNECTED);
			if (buf[1] & 0x08)
				StdPort_SetControllerType(&Controllers.FSPort4, STD_STDCONTROLLER);
			else	StdPort_SetControllerType(&Controllers.FSPort4, STD_UNCONNECTED);
			StdPort_SetControllerType(&Controllers.Port1, STD_FOURSCORE);
			StdPort_SetControllerType(&Controllers.Port2, STD_FOURSCORE);
		}
		else
		{
			StdPort_SetControllerType(&Controllers.Port1, buf[0]);
			StdPort_SetControllerType(&Controllers.Port2, buf[1]);
		}
		ExpPort_SetControllerType(&Controllers.ExpPort, buf[2]);
	}
	NES_SetCPUMode(buf[3] >> 7);	// Set to NTSC or PAL

	Movie.FrameLen = Controllers.Port1.MovLen + Controllers.Port2.MovLen + Controllers.ExpPort.MovLen;
	if (NES.HasMenu)
		Movie.FrameLen++;
	if (Movie.FrameLen != (buf[3] & 0x3F))
		MessageBox(hMainWnd, _T("The frame size specified in this movie is incorrect! This movie may not play properly!"), _T("Nintendulator"), MB_OK | MB_ICONWARNING);

	fread(&Movie.ReRecords, 4, 1, Movie.Data);
	fread(&len, 4, 1, Movie.Data);
	if (len)
	{
		char *desc = malloc(len);
		int len2;
		fread(desc, len, 1, Movie.Data);
#ifdef	UNICODE
		len2 = MultiByteToWideChar(CP_UTF8, 0, desc, len, NULL, 0);
		Movie.Description = malloc(len2 * sizeof(TCHAR));
		if (Movie.Description)
		{
			MultiByteToWideChar(CP_UTF8, 0, desc, len, Movie.Description, len2);
			EI.DbgOut(_T("Description: \"%s\""), Movie.Description);
			free(Movie.Description);
			Movie.Description = NULL;
		}
#else	/* !UNICODE */
		len2 = 0;
#endif	 /* UNICODE */
		free(desc);
	}
	EI.DbgOut(_T("Re-record count: %i"), Movie.ReRecords);
	Movie.Pos = 0;
	fread(&Movie.Len, 4, 1, Movie.Data);
	EI.DbgOut(_T("Length: %i:%02i.%02i"), Movie.Len / 3600, (Movie.Len % 3600) / 60, Movie.Len % 60);

	EnableMenuItem(hMenu, ID_MISC_PLAYMOVIE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MISC_RECORDMOVIE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MISC_STOPMOVIE, MF_ENABLED);
	EnableMenuItem(hMenu, ID_PPU_MODE_NTSC, MF_GRAYED);
	EnableMenuItem(hMenu, ID_PPU_MODE_PAL, MF_GRAYED);
	EnableMenuItem(hMenu, ID_GAME, MF_GRAYED);
	EnableMenuItem(hMenu, ID_CPU_GAMEGENIE, MF_GRAYED);
	if (Controllers.Port1.MovLen)
		memset(Controllers.Port1.MovData, 0, Controllers.Port1.MovLen);
	if (Controllers.Port2.MovLen)
		memset(Controllers.Port2.MovData, 0, Controllers.Port2.MovLen);
	if (Controllers.ExpPort.MovLen)
		memset(Controllers.ExpPort.MovData, 0, Controllers.ExpPort.MovLen);
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
		SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_PORT1, StdPort_Mappings[Controllers.Port1.Type]);
		if (Controllers.Port1.Type == STD_FOURSCORE)
		{
			TCHAR conts[16] = {0};
			if (Controllers.FSPort1.Type)
				_tcscat(conts, _T(",1"));
			if (Controllers.FSPort2.Type)
				_tcscat(conts, _T(",2"));
			if (Controllers.FSPort3.Type)
				_tcscat(conts, _T(",3"));
			if (Controllers.FSPort4.Type)
				_tcscat(conts, _T(",4"));
			SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_PORT2, &conts[1]);
		}
		else	SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_PORT2, StdPort_Mappings[Controllers.Port2.Type]);
		SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_EXPPORT, ExpPort_Mappings[Controllers.ExpPort.Type]);
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
			Controllers_OpenConfig();
			SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_PORT1, StdPort_Mappings[Controllers.Port1.Type]);
			if (Controllers.Port1.Type == STD_FOURSCORE)
			{
				TCHAR conts[16] = {0};
				if (Controllers.FSPort1.Type)
					_tcscat(conts, _T(",1"));
				if (Controllers.FSPort2.Type)
					_tcscat(conts, _T(",2"));
				if (Controllers.FSPort3.Type)
					_tcscat(conts, _T(",3"));
				if (Controllers.FSPort4.Type)
					_tcscat(conts, _T(",4"));
				SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_PORT2, &conts[1]);
			}
			else	SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_PORT2, StdPort_Mappings[Controllers.Port2.Type]);
			SetDlgItemText(hDlg, IDC_MOVIE_RECORD_CONT_EXPPORT, ExpPort_Mappings[Controllers.ExpPort.Type]);
			break;
		case IDOK:
			GetDlgItemText(hDlg, IDC_MOVIE_RECORD_FILE, Movie.Filename, MAX_PATH);
			Movie.Data = _tfopen(Movie.Filename, _T("w+b"));
			if (Movie.Data)
			{
#ifdef	UNICODE
				int len = GetWindowTextLength(GetDlgItem(hDlg, IDC_MOVIE_RECORD_DESCRIPTION)) + 1;
				Movie.Description = malloc(len * sizeof(TCHAR));
				if (Movie.Description)
					GetDlgItemText(hDlg, IDC_MOVIE_RECORD_DESCRIPTION, Movie.Description, len);
#else	/* !UNICODE */
				// Non-Unicode version currently cannot display movie description
				Movie.Description = NULL;
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

void	Movie_Record (void)
{
	BOOL fromState = FALSE, running = NES.Running;
	int ret, len, x;

	if (Movie.Mode)
	{
		MessageBox(hMainWnd, _T("A movie is already open!"), _T("Nintendulator"), MB_OK);
		return;
	}

	NES_Stop();

	ret = DialogBox(hInst, (LPCTSTR)IDD_MOVIE_RECORD, hMainWnd, MovieRecordProc);
	if (ret == 0)
	{
		if (running)
			NES_Start(FALSE);
		return;
	}
	if (ret == 2)
		fromState = TRUE;

	if ((MI) && (MI->Config) && (!NES.HasMenu))
		EnableMenuItem(hMenu, ID_GAME, MF_GRAYED);
	
	len = 0;
	Movie.ReRecords = 0;
	Movie.Pos = Movie.Len = 0;

	NES.SRAM_Size = 0;		// when messing with movies, don't save SRAM

	fwrite("NSS\x1A", 1, 4, Movie.Data);
	fwrite(STATES_VERSION, 1, 4, Movie.Data);
	fwrite(&len, 1, 4, Movie.Data);
	fwrite("NMOV", 1, 4, Movie.Data);

	if (fromState)
		States_SaveData(Movie.Data);
	else
	{
		// cycle the mapper - special stuff might happen on load that we NEED to perform
		if (MI->Unload)
			MI->Unload();
		if (MI->Load)
			MI->Load();
		if (RI.ROMType == ROM_FDS)	// if recording an FDS movie from reset, discard all savedata
			memcpy(PRG_ROM[0x000], PRG_ROM[0x400], RI.FDS_NumSides << 16);
		NES_Reset(RESET_HARD);
	}
	// save savestate BEFORE enabling recording, so we don't try to re-save the NMOV block
	Movie.Mode = MOV_RECORD;

	fwrite("NMOV", 1, 4, Movie.Data);
	fwrite(&len, 1, 4, Movie.Data);
	
	Movie.ControllerTypes[0] = (unsigned char)Controllers.Port1.Type;
	Movie.ControllerTypes[1] = (unsigned char)Controllers.Port2.Type;
	Movie.ControllerTypes[2] = (unsigned char)Controllers.ExpPort.Type;

	if (Movie.ControllerTypes[0] == STD_FOURSCORE)
	{
		Movie.ControllerTypes[1] = 0;
		if (Controllers.FSPort1.Type)	Movie.ControllerTypes[1] |= 0x01;
		if (Controllers.FSPort2.Type)	Movie.ControllerTypes[1] |= 0x02;
		if (Controllers.FSPort3.Type)	Movie.ControllerTypes[1] |= 0x04;
		if (Controllers.FSPort4.Type)	Movie.ControllerTypes[1] |= 0x08;
	}
	fwrite(Movie.ControllerTypes, 1, 3, Movie.Data);
	x = PPU.IsPAL ? 0x80 : 0x00;
	Movie.FrameLen = 0;
	Movie.FrameLen += Controllers.Port1.MovLen;
	Movie.FrameLen += Controllers.Port2.MovLen;
	Movie.FrameLen += Controllers.ExpPort.MovLen;
	if (NES.HasMenu)
		Movie.FrameLen++;
	x |= Movie.FrameLen;
	if (NES.GameGenie && !fromState)	// set Game Genie bit only if recording from reset
		x |= 0x40;			// - otherwise we can just detect it from the GENI block
	fwrite(&x, 1, 1, Movie.Data);		// frame size, NTSC/PAL, Game Genie
	fwrite(&Movie.ReRecords, 4, 1, Movie.Data);// rerecord count
#ifdef	UNICODE
	if (Movie.Description)
	{
		char *desc;
		len = WideCharToMultiByte(CP_UTF8, 0, Movie.Description, -1, NULL, 0, NULL, NULL);
		desc = malloc(len);
		WideCharToMultiByte(CP_UTF8, 0, Movie.Description, -1, desc, len, NULL, NULL);
		fwrite(&len, 4, 1, Movie.Data);		// comment length
		fwrite(desc, len, 1, Movie.Data);	// comment data
		free(desc);
	}
	else
	{
		len = 0;
		fwrite(&len, 4, 1, Movie.Data);		// no comment
	}
#else	/* !UNICODE */
	len = 0;
	fwrite(&len, 4, 1, Movie.Data);		// no comment
#endif	/* UNICODE */
	fwrite(&Movie.Len, 4, 1, Movie.Data);	// movie data length (this gets filled in later)

	EnableMenuItem(hMenu, ID_MISC_PLAYMOVIE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MISC_RECORDMOVIE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MISC_STOPMOVIE, MF_ENABLED);
	EnableMenuItem(hMenu, ID_CPU_GAMEGENIE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_PPU_MODE_NTSC, MF_GRAYED);
	EnableMenuItem(hMenu, ID_PPU_MODE_PAL, MF_GRAYED);

	if (Movie.Description)
	{
		free(Movie.Description);	// don't need this anymore
		Movie.Description = NULL;
	}
}

static	void	EndMovie (void)
{
	if (!(Movie.Mode))
	{
		MessageBox(hMainWnd,_T("No movie is currently active!"),_T("Nintendulator"),MB_OK);
		return;
	}
	if (Movie.Mode & MOV_RECORD)
	{
		int len = ftell(Movie.Data);
		int mlen;
		char tps[4];

		fseek(Movie.Data,8,SEEK_SET);		len -= 16;
		fwrite(&len,4,1,Movie.Data);		// 1: set the file length
		fseek(Movie.Data,16,SEEK_SET);
		fread(tps,4,1,Movie.Data);
		fread(&mlen,4,1,Movie.Data);		len -= 8;
		while (memcmp(tps,"NMOV",4))
		{	/* find the NMOV block in the movie */
			fseek(Movie.Data,mlen,SEEK_CUR);len -= mlen;
			fread(tps,4,1,Movie.Data);
			fread(&mlen,4,1,Movie.Data);	len -= 8;
		}
		fseek(Movie.Data,-4,SEEK_CUR);
		fwrite(&len,4,1,Movie.Data);		// 2: set the NMOV block length
		fseek(Movie.Data,4,SEEK_CUR);		len -= 4;	// skip controller data
		fwrite(&Movie.ReRecords,4,1,Movie.Data);len -= 4;	// write the final re-record count
		fseek(Movie.Data,ftell(Movie.Data),SEEK_SET);
		fread(&mlen,4,1,Movie.Data);		len -= 4;	// read description len
		if (mlen)
		{
			fseek(Movie.Data,mlen,SEEK_CUR);len -= mlen;	// skip the description
		}
		fread(tps,4,1,Movie.Data);		len -= 4;	// read movie data len
		fseek(Movie.Data,-4,SEEK_CUR);		// rewind
		if (len != Movie.Pos)
			EI.DbgOut(_T("Error - movie length mismatch!"));
		fwrite(&len,4,1,Movie.Data);		// 3: terminate the movie data
		// fseek(Movie.Data,len,SEEK_CUR);
		// TODO - truncate the file to this point
	}
	fclose(Movie.Data);

	if (Movie.Mode & MOV_PLAY)
		EI.DbgOut(_T("Movie playback stopped."));
	if (Movie.Mode & MOV_RECORD)
		EI.DbgOut(_T("Movie recording stopped."));

	Movie.Mode = 0;

	// Restore controller types to whatever was configured prior to movie playback
	if (Movie.ControllerTypes[0] == STD_FOURSCORE)
	{
		if (Movie.ControllerTypes[1] & 0x01)
			StdPort_SetControllerType(&Controllers.FSPort1,STD_STDCONTROLLER);
		else	StdPort_SetControllerType(&Controllers.FSPort1,STD_UNCONNECTED);
		if (Movie.ControllerTypes[1] & 0x02)
			StdPort_SetControllerType(&Controllers.FSPort2,STD_STDCONTROLLER);
		else	StdPort_SetControllerType(&Controllers.FSPort2,STD_UNCONNECTED);
		if (Movie.ControllerTypes[1] & 0x04)
			StdPort_SetControllerType(&Controllers.FSPort3,STD_STDCONTROLLER);
		else	StdPort_SetControllerType(&Controllers.FSPort3,STD_UNCONNECTED);
		if (Movie.ControllerTypes[1] & 0x08)
			StdPort_SetControllerType(&Controllers.FSPort4,STD_STDCONTROLLER);
		else	StdPort_SetControllerType(&Controllers.FSPort4,STD_UNCONNECTED);
		StdPort_SetControllerType(&Controllers.Port1,STD_FOURSCORE);
		StdPort_SetControllerType(&Controllers.Port2,STD_FOURSCORE);
	}
	else
	{
		StdPort_SetControllerType(&Controllers.Port1,Movie.ControllerTypes[0]);
		StdPort_SetControllerType(&Controllers.Port2,Movie.ControllerTypes[1]);
	}
	ExpPort_SetControllerType(&Controllers.ExpPort,Movie.ControllerTypes[2]);
	if ((MI) && (MI->Config))
		EnableMenuItem(hMenu,ID_GAME,MF_ENABLED);
	EnableMenuItem(hMenu,ID_MISC_PLAYMOVIE,MF_ENABLED);
	EnableMenuItem(hMenu,ID_MISC_RECORDMOVIE,MF_ENABLED);
	EnableMenuItem(hMenu,ID_MISC_STOPMOVIE,MF_GRAYED);
	EnableMenuItem(hMenu,ID_CPU_GAMEGENIE,MF_ENABLED);
	EnableMenuItem(hMenu,ID_PPU_MODE_NTSC,MF_ENABLED);
	EnableMenuItem(hMenu,ID_PPU_MODE_PAL,MF_ENABLED);
}

void	Movie_Stop (void)
{
	BOOL running = NES.Running;
	NES_Stop();
	EndMovie();
	if (running)
		NES_Start(FALSE);
}

unsigned char	Movie_LoadInput (void)
{
	unsigned char Cmd = 0;
	if (Movie.Pos >= Movie.Len)
	{
		if (Movie.Pos == Movie.Len)
			PrintTitlebar(_T("Movie stopped."));
		else	PrintTitlebar(_T("Unexpected EOF in movie!"));
		EndMovie();
	}
	if (Controllers.Port1.MovLen)
		fread(Controllers.Port1.MovData,1,Controllers.Port1.MovLen,Movie.Data);		Movie.Pos += Controllers.Port1.MovLen;
	if (Controllers.Port2.MovLen)
		fread(Controllers.Port2.MovData,1,Controllers.Port2.MovLen,Movie.Data);		Movie.Pos += Controllers.Port2.MovLen;
	if (Controllers.ExpPort.MovLen)
		fread(Controllers.ExpPort.MovData,1,Controllers.ExpPort.MovLen,Movie.Data);	Movie.Pos += Controllers.ExpPort.MovLen;
	if (NES.HasMenu)
	{
		fread(&Cmd,1,1,Movie.Data);
		Movie.Pos++;
	}
	return Cmd;
}

void	Movie_SaveInput (unsigned char Cmd)
{
	int len = 0;
	if (Controllers.Port1.MovLen)
		fwrite(Controllers.Port1.MovData,1,Controllers.Port1.MovLen,Movie.Data);	len += Controllers.Port1.MovLen;
	if (Controllers.Port2.MovLen)
		fwrite(Controllers.Port2.MovData,1,Controllers.Port2.MovLen,Movie.Data);	len += Controllers.Port2.MovLen;
	if (Controllers.ExpPort.MovLen)
		fwrite(Controllers.ExpPort.MovData,1,Controllers.ExpPort.MovLen,Movie.Data);	len += Controllers.ExpPort.MovLen;
	if (NES.HasMenu)
	{
		fwrite(&Cmd,1,1,Movie.Data);
		len++;
	}
	Movie.Pos += len;
	Movie.Len += len;
}

int	Movie_Save (FILE *out)
{
	int clen = 0, offset;

	unsigned char tpc;
	unsigned long tpl;
	int tpi;

	offset = ftell(Movie.Data);	// save old offset in movie file

	Movie_FindBlock();

	fread(&tpc,1,1,Movie.Data);	fwrite(&tpc,1,1,out);	clen++;
	fread(&tpc,1,1,Movie.Data);	fwrite(&tpc,1,1,out);	clen++;
	fread(&tpc,1,1,Movie.Data);	fwrite(&tpc,1,1,out);	clen++;
	fread(&tpc,1,1,Movie.Data);	fwrite(&tpc,1,1,out);	clen++;
	fread(&tpl,4,1,Movie.Data);	fwrite(&Movie.ReRecords,4,1,out);	clen += 4;	// ignore rerecord count stored in movie

	fread(&tpl,4,1,Movie.Data);	fwrite(&tpl,4,1,out);	clen += 4;
	tpi = tpl;						clen += tpi;
	while (tpi > 0)
	{
		fread(&tpc,1,1,Movie.Data);
		fwrite(&tpc,1,1,out);
		tpi--;
	}

	fread(&tpl,4,1,Movie.Data);				clen += 4;	// the MLEN field, which is NOT yet accurate
	fwrite(&Movie.Pos,4,1,out);
	
	tpi = Movie.Pos;					clen += tpi;
	while (tpi > 0)
	{
		fread(&tpc,1,1,Movie.Data);
		fwrite(&tpc,1,1,out);
		tpi--;
	}
	rewind(Movie.Data);
	fseek(Movie.Data,offset,SEEK_SET);		// seek the movie to where it left off

	return clen;
}

int	Movie_Load (FILE *in)
{
	int clen = 0;

	int tpi;
	unsigned long tpl;
	unsigned char tpc;
	unsigned char Cmd;

	if (Movie.Mode & MOV_RECORD)
	{	// zoom to the correct position and update the necessary fields along the way
		Movie_FindBlock();
		fseek(Movie.Data,0,SEEK_CUR);
		fread(&tpl,4,1,in);	fwrite(&tpl,4,1,Movie.Data);	clen += 4;	// CTRL0, CTRL1, CTEXT, EXTR
		fread(&tpl,4,1,in);	fwrite(&tpl,4,1,Movie.Data);	clen += 4;	// RREC
		if (Movie.ReRecords < (int)tpl)
			Movie.ReRecords = tpl;
		Movie.ReRecords++;
		fread(&tpl,4,1,in);	fwrite(&tpl,4,1,Movie.Data);	clen += 4;	// ILEN
		tpi = tpl;						clen += tpi;	// INFO
		while (tpi > 0)
		{
			fread(&tpc,1,1,in);
			fwrite(&tpc,1,1,Movie.Data);
			tpi--;
		}
		fread(&Movie.Pos,4,1,in);	fwrite(&Movie.Pos,4,1,Movie.Data);	clen += 4;	// MLEN
		tpi = Movie.Pos;					clen += tpi;	// MDAT
		Cmd = 0;
		while (tpi > 0)
		{
			if (Controllers.Port1.MovLen)
			{
				fread(Controllers.Port1.MovData,1,Controllers.Port1.MovLen,in);
				fwrite(Controllers.Port1.MovData,1,Controllers.Port1.MovLen,Movie.Data);
				tpi -= Controllers.Port1.MovLen;
			}
			if (Controllers.Port2.MovLen)
			{
				fread(Controllers.Port2.MovData,1,Controllers.Port2.MovLen,in);
				fwrite(Controllers.Port2.MovData,1,Controllers.Port2.MovLen,Movie.Data);
				tpi -= Controllers.Port2.MovLen;
			}
			if (Controllers.ExpPort.MovLen)
			{
				fread(Controllers.ExpPort.MovData,1,Controllers.ExpPort.MovLen,in);
				fwrite(Controllers.ExpPort.MovData,1,Controllers.ExpPort.MovLen,Movie.Data);
				tpi -= Controllers.ExpPort.MovLen;
			}
			if (NES.HasMenu)
			{
				fread(&Cmd,1,1,in);
				fwrite(&Cmd,1,1,Movie.Data);
				tpi--;
			}
		}
		Controllers.Port1.Frame(&Controllers.Port1,MOV_PLAY);
		Controllers.Port2.Frame(&Controllers.Port2,MOV_PLAY);
		Controllers.ExpPort.Frame(&Controllers.ExpPort,MOV_PLAY);
		if ((Cmd) && (MI) && (MI->Config))
			MI->Config(CFG_CMD,Cmd);
	}
	else if (Movie.Mode & MOV_PLAY)
	{	// zoom the movie file to the current playback position
		Movie_FindBlock();
		fread(&tpl,4,1,in);	fread(&tpl,4,1,Movie.Data);		clen += 4;	// CTRL0, CTRL1, CTEXT, EXTR
		fread(&tpl,4,1,in);	fread(&tpl,4,1,Movie.Data);		clen += 4;	// RREC
		fread(&tpl,4,1,in);	fread(&tpi,4,1,Movie.Data);		clen += 4;	// ILEN
		fseek(in,tpl,SEEK_CUR);	fseek(Movie.Data,tpi,SEEK_CUR);		clen += tpl;	// INFO
		fread(&Movie.Pos,4,1,in);	fread(&Movie.Len,4,1,Movie.Data);	clen += 4;	// MLEN
		tpi = Movie.Pos;	fseek(Movie.Data,tpi,SEEK_CUR);		clen += tpi;	// MDAT
		Cmd = 0;
		while (tpi > 0)
		{
			if (Controllers.Port1.MovLen)
			{
				fread(Controllers.Port1.MovData,1,Controllers.Port1.MovLen,in);
				tpi -= Controllers.Port1.MovLen;
			}
			if (Controllers.Port2.MovLen)
			{
				fread(Controllers.Port2.MovData,1,Controllers.Port2.MovLen,in);
				tpi -= Controllers.Port2.MovLen;
			}
			if (Controllers.ExpPort.MovLen)
			{
				fread(Controllers.ExpPort.MovData,1,Controllers.ExpPort.MovLen,in);
				tpi -= Controllers.ExpPort.MovLen;
			}
			if (NES.HasMenu)
			{
				fread(&Cmd,1,1,in);
				tpi--;
			}
		}
		Controllers.Port1.Frame(&Controllers.Port1,MOV_PLAY);
		Controllers.Port2.Frame(&Controllers.Port2,MOV_PLAY);
		Controllers.ExpPort.Frame(&Controllers.ExpPort,MOV_PLAY);
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

