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

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "MapperInterface.h"
#include "Movie.h"
#include "Controllers.h"
#include "States.h"
#include "NES.h"
#include "PPU.h"
#include <commdlg.h>

struct	tMovie	Movie;

void	Movie_ShowFrame (void)
{
	if (Movie.Mode)
		EI.StatusOut(_T("Frame %i"),Movie.Pos / Movie.FrameLen);
}

void	Movie_Play (BOOL Review)
{
	unsigned char buf[5];
	OPENFILENAME ofn;
	int len;

	if (Movie.Mode)
	{
		MessageBox(mWnd,_T("A movie is already open!"),_T("Nintendulator"),MB_OK);
		return;
	}

	NES_Stop();

	ZeroMemory(&ofn,sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = mWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFilter = _T("Nintendulator Movie (*.NMV)\0") _T("*.NMV\0") _T("\0");
	ofn.lpstrCustomFilter = NULL;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = Movie.Filename;
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
		return;

	_tcscpy(Path_NMV,Movie.Filename);
	Path_NMV[ofn.nFileOffset-1] = 0;

	if (Review)
		Movie.Data = _tfopen(Movie.Filename,_T("r+b"));
	else	Movie.Data = _tfopen(Movie.Filename,_T("rb"));
	fread(buf,1,4,Movie.Data);
	if (memcmp(buf,"NSS\x1a",4))
	{
		MessageBox(mWnd,_T("Invalid movie file selected!"),_T("Nintendulator"),MB_OK);
		fclose(Movie.Data);
		return;
	}

	fread(buf,1,4,Movie.Data);
	if (memcmp(buf,STATES_VERSION,4))
	{
		MessageBox(mWnd,_T("Incorrect movie version!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		fclose(Movie.Data);
		return;
	}
	fread(&len,4,1,Movie.Data);
	fread(buf,1,4,Movie.Data);

	if (memcmp(buf,"NMOV",4))
	{
		MessageBox(mWnd,_T("This is not a valid Nintendulator movie recording!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		fclose(Movie.Data);
		return;
	}
	fseek(Movie.Data,16,SEEK_SET);

	Movie.ControllerTypes[0] = Controllers.Port1.Type;
	Movie.ControllerTypes[1] = Controllers.Port2.Type;
	Movie.ControllerTypes[2] = Controllers.ExpPort.Type;

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
		memcpy(PRG_ROM[0x000],PRG_ROM[0x400],RI.FDS_NumSides << 16);
	NES_Reset(RESET_HARD);
	// load savestate BEFORE enabling playback, so we don't try to load the NMOV block
	if (!States_LoadData(Movie.Data,len))
	{
		MessageBox(mWnd,_T("Failed to load movie!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		fclose(Movie.Data);
		return;
	}

	Movie.Mode = MOV_PLAY;
	if (Review)
		Movie.Mode |= MOV_REVIEW;
	
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

	fread(buf,1,4,Movie.Data);

	if (!Movie.ControllerTypes[3])	// ONLY parse this if we did NOT encounter a controller state block
	{				// otherwise, it would mess up the original states of the controllers
		if (buf[0] == STD_FOURSCORE)
		{
			if (buf[1] & 0x01)
				StdPort_SetControllerType(&Controllers.FSPort1,STD_STDCONTROLLER);
			else	StdPort_SetControllerType(&Controllers.FSPort1,STD_UNCONNECTED);
			if (buf[1] & 0x02)
				StdPort_SetControllerType(&Controllers.FSPort2,STD_STDCONTROLLER);
			else	StdPort_SetControllerType(&Controllers.FSPort2,STD_UNCONNECTED);
			if (buf[1] & 0x04)
				StdPort_SetControllerType(&Controllers.FSPort3,STD_STDCONTROLLER);
			else	StdPort_SetControllerType(&Controllers.FSPort3,STD_UNCONNECTED);
			if (buf[1] & 0x08)
				StdPort_SetControllerType(&Controllers.FSPort4,STD_STDCONTROLLER);
			else	StdPort_SetControllerType(&Controllers.FSPort4,STD_UNCONNECTED);
			StdPort_SetControllerType(&Controllers.Port1,STD_FOURSCORE);
			StdPort_SetControllerType(&Controllers.Port2,STD_FOURSCORE);
		}
		else
		{
			StdPort_SetControllerType(&Controllers.Port1,buf[0]);
			StdPort_SetControllerType(&Controllers.Port2,buf[1]);
		}
		ExpPort_SetControllerType(&Controllers.ExpPort,buf[2]);
	}
	NES_SetCPUMode(buf[3] >> 7);	// Set to NTSC or PAL

	Movie.FrameLen = Controllers.Port1.MovLen + Controllers.Port2.MovLen + Controllers.ExpPort.MovLen;
	if (NES.HasMenu)
		Movie.FrameLen++;
	if (Movie.FrameLen != (buf[3] & 0x7F))
		MessageBox(mWnd,_T("The frame size specified in this movie is incorrect! This movie may not play properly!"),_T("Nintendulator"),MB_OK | MB_ICONWARNING);

	fread(&Movie.ReRecords,4,1,Movie.Data);
	fread(&len,4,1,Movie.Data);
	if (len)
	{
		char *desc = malloc(len);
		fread(desc,len,1,Movie.Data);
		EI.DbgOut(_T("Description: \"") PRINTF_CHAR8 _T("\""),desc);
		free(desc);
	}
	EI.DbgOut(_T("Re-record count: %i"),Movie.ReRecords);
	Movie.Pos = 0;
	fread(&Movie.Len,4,1,Movie.Data);

	EnableMenuItem(GetMenu(mWnd),ID_MISC_PLAYMOVIE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_RESUMEMOVIE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_RECORDMOVIE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_RECORDSTATE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_STOPMOVIE,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_PPU_MODE_NTSC,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_PPU_MODE_PAL,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_GAME,MF_GRAYED);
	if (Controllers.Port1.MovLen)
		memset(Controllers.Port1.MovData,0,Controllers.Port1.MovLen);
	if (Controllers.Port2.MovLen)
		memset(Controllers.Port2.MovData,0,Controllers.Port2.MovLen);
	if (Controllers.ExpPort.MovLen)
		memset(Controllers.ExpPort.MovData,0,Controllers.ExpPort.MovLen);
}

void	Movie_Record (BOOL fromState)
{
	OPENFILENAME ofn;
	int len;
	unsigned char x;

	if (Movie.Mode)
	{
		MessageBox(mWnd,_T("A movie is already open!"),_T("Nintendulator"),MB_OK);
		return;
	}

	NES_Stop();
	if ((MI) && (MI->Config) && (!NES.HasMenu))
	{
		MessageBox(mWnd,_T("This game does not support using the 'Game' menu while recording!"),_T("Nintendulator"),MB_OK);
		EnableMenuItem(GetMenu(mWnd),ID_GAME,MF_GRAYED);
	}

	ZeroMemory(&ofn,sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = mWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFilter = _T("Nintendulator Movie (*.NMV)\0") _T("*.NMV\0") _T("\0");
	ofn.lpstrCustomFilter = NULL;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = Movie.Filename;
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
		return;

	_tcscpy(Path_NMV,Movie.Filename);
	Path_NMV[ofn.nFileOffset-1] = 0;

	Controllers_OpenConfig();	// Allow user to choose the desired controllers
	Movie.Mode = MOV_RECORD;	// ...and lock it out until the movie ends

	Movie.Data = _tfopen(Movie.Filename,_T("w+b"));
	len = 0;
	Movie.ReRecords = 0;
	Movie.Pos = Movie.Len = 0;

	NES.SRAM_Size = 0;		// when messing with movies, don't save SRAM

	fwrite("NSS\x1A",1,4,Movie.Data);
	fwrite(STATES_VERSION,1,4,Movie.Data);
	fwrite(&len,1,4,Movie.Data);
	fwrite("NMOV",1,4,Movie.Data);

	if (fromState)
		States_SaveData(Movie.Data);
	else
	{
		if (RI.ROMType == ROM_FDS)	// if recording an FDS movie from reset, discard all savedata
			memcpy(PRG_ROM[0x000],PRG_ROM[0x400],RI.FDS_NumSides << 16);
		NES_Reset(RESET_HARD);
	}

	fwrite("NMOV",1,4,Movie.Data);
	fwrite(&len,1,4,Movie.Data);
	
	Movie.ControllerTypes[0] = Controllers.Port1.Type;
	Movie.ControllerTypes[1] = Controllers.Port2.Type;
	Movie.ControllerTypes[2] = Controllers.ExpPort.Type;

	if (Movie.ControllerTypes[0] == STD_FOURSCORE)
	{
		Movie.ControllerTypes[1] = 0;
		if (Controllers.FSPort1.Type)	Movie.ControllerTypes[1] |= 0x01;
		if (Controllers.FSPort2.Type)	Movie.ControllerTypes[1] |= 0x02;
		if (Controllers.FSPort3.Type)	Movie.ControllerTypes[1] |= 0x04;
		if (Controllers.FSPort4.Type)	Movie.ControllerTypes[1] |= 0x08;
	}
	fwrite(Movie.ControllerTypes,1,3,Movie.Data);
	x = PPU.IsPAL ? 0x80 : 0x00;
	Movie.FrameLen = 0;
	Movie.FrameLen += Controllers.Port1.MovLen;
	Movie.FrameLen += Controllers.Port2.MovLen;
	Movie.FrameLen += Controllers.ExpPort.MovLen;
	if (NES.HasMenu)
		Movie.FrameLen++;
	x |= Movie.FrameLen;
	fwrite(&x,1,1,Movie.Data);		// frame size, NTSC/PAL
	fwrite(&Movie.ReRecords,4,1,Movie.Data);// rerecord count
	fwrite(&len,4,1,Movie.Data);		// comment length
	// TODO - Actually store comment text
	fwrite(&Movie.Len,4,1,Movie.Data);	// movie data length (this gets filled in later)

	EnableMenuItem(GetMenu(mWnd),ID_MISC_PLAYMOVIE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_RESUMEMOVIE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_RECORDMOVIE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_RECORDSTATE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_STOPMOVIE,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_PPU_MODE_NTSC,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_PPU_MODE_PAL,MF_GRAYED);
}

static	void	EndMovie (void)
{
	if (!(Movie.Mode))
	{
		MessageBox(mWnd,_T("No movie is currently active!"),_T("Nintendulator"),MB_OK);
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
	Movie.Mode = 0;

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
		EnableMenuItem(GetMenu(mWnd),ID_GAME,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_PLAYMOVIE,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_RESUMEMOVIE,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_RECORDMOVIE,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_RECORDSTATE,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_STOPMOVIE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_PPU_MODE_NTSC,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_PPU_MODE_PAL,MF_ENABLED);
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
	if (Controllers.Port1.MovLen)
		fread(Controllers.Port1.MovData,1,Controllers.Port1.MovLen,Movie.Data);		Movie.Pos += Controllers.Port1.MovLen;
	if (Controllers.Port2.MovLen)
		fread(Controllers.Port2.MovData,1,Controllers.Port2.MovLen,Movie.Data);		Movie.Pos += Controllers.Port2.MovLen;
	if (Controllers.ExpPort.MovLen)
		fread(Controllers.ExpPort.MovData,1,Controllers.ExpPort.MovLen,Movie.Data);	Movie.Pos += Controllers.ExpPort.MovLen;
	if (Movie.Pos >= Movie.Len)
	{
		if (Movie.Pos == Movie.Len)
			PrintTitlebar(_T("Movie stopped."));
		else	PrintTitlebar(_T("Unexpected EOF in movie!"));
		EndMovie();
	}
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
	int clen = 0;

	int mlen;
	char tps[4];
	unsigned char tpc;
	unsigned long tpl;
	int tpi;

	rewind(Movie.Data);
	fseek(Movie.Data,16,SEEK_SET);
	fread(tps,4,1,Movie.Data);
	fread(&mlen,4,1,Movie.Data);
	while (memcmp(tps,"NMOV",4))
	{	/* find the NMOV block in the movie file */
		fseek(Movie.Data,mlen,SEEK_CUR);
		fread(tps,4,1,Movie.Data);
		fread(&mlen,4,1,Movie.Data);
	}

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
	fseek(Movie.Data,clen+24,SEEK_SET);		// seek the movie to where it left off

	return clen;
}

int	Movie_Load (FILE *in)
{
	int clen = 0;

	char tps[4];
	int mlen;
	int tpi;
	unsigned long tpl;
	unsigned char tpc;
	unsigned char Cmd;

	if (Movie.Mode & MOV_RECORD)
	{	// zoom to the correct position and update the necessary fields along the way
		rewind(Movie.Data);
		fseek(Movie.Data,16,SEEK_SET);
		fread(tps,4,1,Movie.Data);
		fread(&mlen,4,1,Movie.Data);
		while (memcmp(tps,"NMOV",4))
		{	/* find the NMOV block in the movie file */
			fseek(Movie.Data,mlen,SEEK_CUR);
			fread(tps,4,1,Movie.Data);
			fread(&mlen,4,1,Movie.Data);
		}
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
		rewind(Movie.Data);
		fseek(Movie.Data,16,SEEK_SET);
		fread(tps,4,1,Movie.Data);
		fread(&mlen,4,1,Movie.Data);
		while (memcmp(tps,"NMOV",4))
		{	/* find the NMOV block in the movie file */
			fseek(Movie.Data,mlen,SEEK_CUR);
			fread(tps,4,1,Movie.Data);
			fread(&mlen,4,1,Movie.Data);
		}
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
