/*
Nintendulator - A Win32 NES emulator written in C.
Designed for maximum emulation accuracy.
Copyright (c) 2002-2006  QMT Productions

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
#include "NES.h"
#include "States.h"
#include "CPU.h"
#include "PPU.h"
#include "APU.h"
#include "Movie.h"
#include "Controllers.h"
#include "GFX.h"
#include "Genie.h"

struct	tStates	States;

extern FILE *movie;

void	States_Init (void)
{
	TCHAR tmp[MAX_PATH];
	_stprintf(tmp,_T("%sSaves"),ProgPath);
	States.SelSlot = 0;
	States.NeedSave = FALSE;
	States.NeedLoad = FALSE;
	CreateDirectory(tmp,NULL);	// attempt to create Saves dir (if it exists, it fails silently)
}

void	States_SetFilename (TCHAR *Filename)
{
	TCHAR Tmp[MAX_PATH];
	size_t i;
	for (i = _tcslen(Filename); Filename[i] != '\\'; i--);
	_tcscpy(Tmp,&Filename[i+1]);
	for (i = _tcslen(Tmp); i >= 0; i--)
		if (Tmp[i] == '.')
		{
			Tmp[i] = 0;
			break;
		}
	_stprintf(States.BaseFilename,_T("%sSaves\\%s"),ProgPath,Tmp);
}

void	States_SetSlot (int Slot)
{
	TCHAR tpchr[MAX_PATH];
	FILE *tmp;
	States.SelSlot = Slot;
	_stprintf(tpchr,_T("%s.ns%i"),States.BaseFilename,Slot);
	if (tmp = _tfopen(tpchr,_T("rb")))
	{
		fclose(tmp);
		PrintTitlebar(_T("State Selected: %i (occupied)"), Slot);
	}
	else	PrintTitlebar(_T("State Selected: %i (empty)"), Slot);
}

int	States_SaveData (FILE *out)
{
	int flen = 0, clen;
	{
		fwrite("CPUS",1,4,out);		flen += 4;
		fwrite(&clen,1,4,out);		flen += 4;
		clen = CPU_Save(out);
		fseek(out,-clen - 4,SEEK_CUR);
		fwrite(&clen,1,4,out);
		fseek(out,clen,SEEK_CUR);	flen += clen;
	}
	{
		fwrite("PPUS",1,4,out);		flen += 4;
		fwrite(&clen,1,4,out);		flen += 4;
		clen = PPU_Save(out);
		fseek(out,-clen - 4,SEEK_CUR);
		fwrite(&clen,1,4,out);
		fseek(out,clen,SEEK_CUR);	flen += clen;
	}
	{
		fwrite("APUS",1,4,out);		flen += 4;
		fwrite(&clen,1,4,out);		flen += 4;
		clen = APU_Save(out);
		fseek(out,-clen - 4,SEEK_CUR);
		fwrite(&clen,1,4,out);
		fseek(out,clen,SEEK_CUR);	flen += clen;
	}
	{
		fwrite("CTRL",1,4,out);		flen += 4;
		fwrite(&clen,1,4,out);		flen += 4;
		clen = Controllers_Save(out);
		fseek(out,-clen - 4,SEEK_CUR);
		fwrite(&clen,1,4,out);
		fseek(out,clen,SEEK_CUR);	flen += clen;
	}
	if (Genie.CodeStat & 1)
	{
		fwrite("GENI",1,4,out);		flen += 4;
		fwrite(&clen,1,4,out);		flen += 4;
		clen = Genie_Save(out);
		fseek(out,-clen - 4,SEEK_CUR);
		fwrite(&clen,1,4,out);
		fseek(out,clen,SEEK_CUR);	flen += clen;
	}
	{
		for (clen = sizeof(PRG_RAM) - 1; clen >= 0; clen--)
			if (PRG_RAM[clen >> 12][clen & 0xFFF])
				break;
		if (clen >= 0)
		{
			clen++;
			fwrite("NPRA",1,4,out);		flen += 4;
			fwrite(&clen,1,4,out);		flen += 4;
				//Data
			fwrite(PRG_RAM,1,clen,out);	flen += clen;	//	PRAM	uint8[...]	PRG_RAM data
		}
	}
	{
		for (clen = sizeof(CHR_RAM) - 1; clen >= 0; clen--)
			if (CHR_RAM[clen >> 10][clen & 0x3FF])
				break;
		if (clen >= 0)
		{
			clen++;
			fwrite("NCRA",1,4,out);		flen += 4;
			fwrite(&clen,1,4,out);		flen += 4;
				//Data
			fwrite(CHR_RAM,1,clen,out);	flen += clen;	//	CRAM	uint8[...]	CHR_RAM data
		}
	}
	if (RI.ROMType == ROM_FDS)
	{
		fwrite("DISK",1,4,out);		flen += 4;
		fwrite(&clen,1,4,out);		flen += 4;
		clen = NES_FDSSave(out);
		fseek(out,-clen - 4,SEEK_CUR);
		fwrite(&clen,1,4,out);
		fseek(out,clen,SEEK_CUR);	flen += clen;
	}
	{
		if ((MI) && (MI->SaveLoad))
			clen = MI->SaveLoad(STATE_SIZE,0,NULL);
		else	clen = 0;
		if (clen)
		{
			char *tpmi = malloc(clen);
			MI->SaveLoad(STATE_SAVE,0,tpmi);
			fwrite("MAPR",1,4,out);		flen += 4;
			fwrite(&clen,1,4,out);		flen += 4;
				//Data
			fwrite(tpmi,1,clen,out);	flen += clen;	//	CUST	uint8[...]	Custom mapper data
			free(tpmi);
		}
	}
	if (Movie.Mode)	// save state when recording, reviewing, OR playing
	{
		fwrite("NMOV",1,4,out);		flen += 4;
		fwrite(&clen,1,4,out);		flen += 4;
		clen = Movie_Save(out);
		fseek(out,-clen - 4,SEEK_CUR);
		fwrite(&clen,1,4,out);
		fseek(out,clen,SEEK_CUR);	flen += clen;
	}
	return flen;
}

void	States_SaveState (void)
{
	TCHAR tps[MAX_PATH];
	int flen;
	FILE *out;

	States.NeedSave = FALSE;

	if (Genie.CodeStat & 0x80)
	{
		PrintTitlebar(_T("Cannot save state at the Game Genie code entry screen!"));
		return;
	}

	_stprintf(tps,_T("%s.ns%i"),States.BaseFilename,States.SelSlot);
	out = _tfopen(tps,_T("w+b"));
	flen = 0;

	fwrite("NSS\x1A",1,4,out);
	fwrite(STATES_VERSION,1,4,out);
	fwrite(&flen,1,4,out);
	if (Movie.Mode)	// save NREC during playback as well
		fwrite("NREC",1,4,out);
	else	fwrite("NSAV",1,4,out);

	flen = States_SaveData(out);

	// Write final filesize
	fseek(out,8,SEEK_SET);
	fwrite(&flen,1,4,out);
	fclose(out);

	PrintTitlebar(_T("State saved: %i"), States.SelSlot);
	Movie_ShowFrame();
}

BOOL	States_LoadData (FILE *in, int flen)
{
	char csig[4];
	int clen;
	BOOL SSOK = TRUE;
	fread(&csig,1,4,in);	flen -= 4;
	fread(&clen,4,1,in);	flen -= 4;
	while (flen > 0)
	{
		flen -= clen;
		if (!memcmp(csig,"CPUS",4))
			clen -= CPU_Load(in);
		else if (!memcmp(csig,"PPUS",4))
			clen -= PPU_Load(in);
		else if (!memcmp(csig,"APUS",4))
			clen -= APU_Load(in);
		else if (!memcmp(csig,"CTRL",4))
			clen -= Controllers_Load(in);
		else if (!memcmp(csig,"GENI",4))
			clen -= Genie_Load(in);
		else if (!memcmp(csig,"NPRA",4))
		{
			memset(PRG_RAM,0,sizeof(PRG_RAM));
			fread(PRG_RAM,1,clen,in);	clen = 0;
		}
		else if (!memcmp(csig,"NCRA",4))
		{
			memset(CHR_RAM,0,sizeof(CHR_RAM));
			fread(CHR_RAM,1,clen,in);	clen = 0;
		}
		else if (!memcmp(csig,"DISK",4))
		{
			if (RI.ROMType == ROM_FDS)
				clen -= NES_FDSLoad(in);
			else	EI.DbgOut(_T("Error - DISK save block found for non-FDS game!"));
		}
		else if (!memcmp(csig,"MAPR",4))
		{
			if ((MI) && (MI->SaveLoad))
			{
				int len = MI->SaveLoad(STATE_SIZE,0,NULL);
				char *tpmi = malloc(len);
				fread(tpmi,1,len,in);			//	CUST	uint8[...]	Custom mapper data
				MI->SaveLoad(STATE_LOAD,0,tpmi);
				free(tpmi);
				clen -= len;
			}
			else	clen = 1;	// ack! we shouldn't have a MAPR block here!
			if (clen != 0)
			{
				clen = 0;	// need to handle this HERE and not below
				SSOK = FALSE;	// since we didn't read past the end of the chunk
			}
		}
		else if (!memcmp(csig,"NMOV",4))
			clen -= Movie_Load(in);
		if (clen != 0)
		{
			SSOK = FALSE;		// too much, or too little
			fseek(in,clen,SEEK_CUR);// seek back to the block boundary
		}
		if (flen > 0)
		{
			fread(&csig,1,4,in);	flen -= 4;
			fread(&clen,4,1,in);	flen -= 4;
		}
	}
	return SSOK;
}

void	States_LoadState (void)
{
	TCHAR tps[MAX_PATH];
	char tpc[5];
	FILE *in;
	int flen;

	States.NeedLoad = FALSE;

	_stprintf(tps,_T("%s.ns%i"),States.BaseFilename,States.SelSlot);
	in = _tfopen(tps,_T("rb"));
	if (!in)
	{
		PrintTitlebar(_T("No such save state: %i"), States.SelSlot);
		return;
	}

	fread(tpc,1,4,in);
	if (memcmp(tpc,"NSS\x1a",4))
	{
		fclose(in);
		PrintTitlebar(_T("Not a valid savestate file: %i"), States.SelSlot);
		return;
	}
	fread(tpc,1,4,in);
	if ((memcmp(tpc,STATES_VERSION,4)) && (memcmp(tpc,STATES_BETA,4)) && (memcmp(tpc,STATES_PREV,4)))
	{
		fclose(in);
		tpc[4] = 0;
		PrintTitlebar(_T("Incorrect savestate version (") PRINTF_CHAR8 _T("): %i"), tpc, States.SelSlot);
		return;
	}
	fread(&flen,4,1,in);
	fread(tpc,1,4,in);

	if (!memcmp(tpc,"NMOV",4))
	{
		fclose(in);
		PrintTitlebar(_T("Selected savestate (%i) is a movie file - cannot load!"), States.SelSlot);
		return;
	}
	else if (!memcmp(tpc,"NREC",4))
	{
		/* Movie savestate, can always load these */
	}
	else if (!memcmp(tpc,"NSAV",4))
	{
		/* Non-movie savestate, can NOT load these while a movie is open */
		if (Movie.Mode)
		{
			fclose(in);
			PrintTitlebar(_T("Selected savestate (%i) does not contain movie data!"), States.SelSlot);
			return;
		}
	}
	else
	{
		fclose(in);
		tpc[4] = 0;
		PrintTitlebar(_T("Selected savestate (%i) has unknown type! (") PRINTF_CHAR8 _T(")"), States.SelSlot, tpc);
		return;
	}

	fseek(in,16,SEEK_SET);
	NES_Reset(RESET_SOFT);

	if (Movie.Mode & MOV_REVIEW)		/* If the user is reviewing an existing movie */
		Movie.Mode = MOV_RECORD;	/* then resume recording once they LOAD state */

	NES.GameGenie = FALSE;	/* If the savestate uses it, it'll turn back on shortly */
	CheckMenuItem(hMenu,ID_CPU_GAMEGENIE,MF_UNCHECKED);

	if (States_LoadData(in, flen))
		PrintTitlebar(_T("State loaded: %i"), States.SelSlot);
	else	PrintTitlebar(_T("State loaded with errors: %i"), States.SelSlot);
	Movie_ShowFrame();
	fclose(in);
}
