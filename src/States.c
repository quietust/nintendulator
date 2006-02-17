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
#include "NES.h"
#include "States.h"
#include "CPU.h"
#include "PPU.h"
#include "APU.h"
#include "Movie.h"
#include "Controllers.h"
#include "GFX.h"
#include "Genie.h"
#include "MapperInterface.h"

struct	tStates	States;

extern FILE *movie;

void	States_Init (void)
{
	char tmp[MAX_PATH];
	sprintf(tmp,"%sSaves",ProgPath);
	States.SelSlot = 0;
	States.NeedSave = FALSE;
	States.NeedLoad = FALSE;
	CreateDirectory(tmp,NULL);	// attempt to create Saves dir (if it exists, it fails silently)
}

void	States_SetFilename (char *Filename)
{
	char Tmp[MAX_PATH];
	size_t i;
	for (i = strlen(Filename); Filename[i] != '\\'; i--);
	strcpy(Tmp,&Filename[i+1]);
	for (i = strlen(Tmp); i >= 0; i--)
		if (Tmp[i] == '.')
		{
			Tmp[i] = 0;
			break;
		}
	sprintf(States.BaseFilename,"%sSaves\\%s",ProgPath,Tmp);
}

void	States_SetSlot (int Slot)
{
	char tpchr[256];
	FILE *tmp;
	States.SelSlot = Slot;
	sprintf(tpchr,"%s.ns%i",States.BaseFilename,Slot);
	if (tmp = fopen(tpchr,"rb"))
	{
		fclose(tmp);
		GFX_ShowText("State Selected: %i (occupied)", Slot);
	}
	else	GFX_ShowText("State Selected: %i (empty)", Slot);
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
		clen = 0;
		fwrite(&Controllers.FSPort1.Type,4,1,out);	clen += 4;
		fwrite(&Controllers.FSPort2.Type,4,1,out);	clen += 4;
		fwrite(&Controllers.FSPort3.Type,4,1,out);	clen += 4;
		fwrite(&Controllers.FSPort4.Type,4,1,out);	clen += 4;
		fwrite(&Controllers.Port1.Type,4,1,out);	clen += 4;
		fwrite(&Controllers.Port2.Type,4,1,out);	clen += 4;
		fwrite(&Controllers.ExpPort.Type,4,1,out);	clen += 4;
		fwrite(Controllers.Port1.Data,Controllers.Port1.DataLen,1,out);		clen += Controllers.Port1.DataLen;
		fwrite(Controllers.Port2.Data,Controllers.Port2.DataLen,1,out);		clen += Controllers.Port2.DataLen;
		fwrite(Controllers.FSPort1.Data,Controllers.FSPort1.DataLen,1,out);	clen += Controllers.FSPort1.DataLen;
		fwrite(Controllers.FSPort2.Data,Controllers.FSPort2.DataLen,1,out);	clen += Controllers.FSPort2.DataLen;
		fwrite(Controllers.FSPort3.Data,Controllers.FSPort3.DataLen,1,out);	clen += Controllers.FSPort3.DataLen;
		fwrite(Controllers.FSPort4.Data,Controllers.FSPort4.DataLen,1,out);	clen += Controllers.FSPort4.DataLen;
		fwrite(Controllers.ExpPort.Data,Controllers.ExpPort.DataLen,1,out);	clen += Controllers.ExpPort.DataLen;
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
	char tpchr[256];
	int flen;
	FILE *out;

	States.NeedSave = FALSE;

	if (Genie.CodeStat & 0x80)
	{
		GFX_ShowText("Cannot save state at the Game Genie code entry screen!");
		return;
	}

	sprintf(tpchr,"%s.ns%i",States.BaseFilename,States.SelSlot);
	out = fopen(tpchr,"w+b");
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

	GFX_ShowText("State saved: %i", States.SelSlot);
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
		{
			int tpi;
			Movie.ControllerTypes[3] = 1;

			fread(&tpi,4,1,in);	StdPort_SetControllerType(&Controllers.FSPort1,tpi);	clen -= 4;
			fread(&tpi,4,1,in);	StdPort_SetControllerType(&Controllers.FSPort2,tpi);	clen -= 4;
			fread(&tpi,4,1,in);	StdPort_SetControllerType(&Controllers.FSPort3,tpi);	clen -= 4;
			fread(&tpi,4,1,in);	StdPort_SetControllerType(&Controllers.FSPort4,tpi);	clen -= 4;
			fread(&tpi,4,1,in);	StdPort_SetControllerType(&Controllers.Port1,tpi);	clen -= 4;
			fread(&tpi,4,1,in);	StdPort_SetControllerType(&Controllers.Port2,tpi);	clen -= 4;
			fread(&tpi,4,1,in);	ExpPort_SetControllerType(&Controllers.ExpPort,tpi);	clen -= 4;

			fread(Controllers.Port1.Data,Controllers.Port1.DataLen,1,in);		clen -= Controllers.Port1.DataLen;
			fread(Controllers.Port2.Data,Controllers.Port2.DataLen,1,in);		clen -= Controllers.Port2.DataLen;
			fread(Controllers.FSPort1.Data,Controllers.FSPort1.DataLen,1,in);	clen -= Controllers.FSPort1.DataLen;
			fread(Controllers.FSPort2.Data,Controllers.FSPort2.DataLen,1,in);	clen -= Controllers.FSPort2.DataLen;
			fread(Controllers.FSPort3.Data,Controllers.FSPort3.DataLen,1,in);	clen -= Controllers.FSPort3.DataLen;
			fread(Controllers.FSPort4.Data,Controllers.FSPort4.DataLen,1,in);	clen -= Controllers.FSPort4.DataLen;
			fread(Controllers.ExpPort.Data,Controllers.ExpPort.DataLen,1,in);	clen -= Controllers.ExpPort.DataLen;
		}
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
			else	EI.DbgOut("Error - DISK save block found for non-FDS game!");
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
	char tpchr[256];
	FILE *in;
	int flen;

	States.NeedLoad = FALSE;

	sprintf(tpchr,"%s.ns%i",States.BaseFilename,States.SelSlot);
	in = fopen(tpchr,"rb");
	if (!in)
	{
		GFX_ShowText("No such save state: %i", States.SelSlot);
		return;
	}

	fread(tpchr,1,4,in);
	if (memcmp(tpchr,"NSS\x1a",4))
	{
		fclose(in);
		GFX_ShowText("Not a valid savestate file: %i", States.SelSlot);
		return;
	}
	fread(tpchr,1,4,in);
	if (memcmp(tpchr,STATES_VERSION,4))
	{
		fclose(in);
		tpchr[4] = 0;
		GFX_ShowText("Incorrect savestate version (%s): %i", tpchr, States.SelSlot);
		return;
	}
	fread(&flen,4,1,in);
	fread(tpchr,1,4,in);

	if (!memcmp(tpchr,"NMOV",4))
	{
		fclose(in);
		GFX_ShowText("Selected savestate (%i) is a movie file - cannot load!", States.SelSlot);
		return;
	}
	else if (!memcmp(tpchr,"NREC",4))
	{
		/* Movie savestate, can always load these */
	}
	else if (!memcmp(tpchr,"NSAV",4))
	{
		/* Non-movie savestate, can NOT load these while a movie is open */
		if (Movie.Mode)
		{
			fclose(in);
			GFX_ShowText("Selected savestate (%i) does not contain movie data!", States.SelSlot);
			return;
		}
	}
	else
	{
		fclose(in);
		tpchr[4] = 0;
		GFX_ShowText("Selected savestate (%i) has unknown type! (%s)", States.SelSlot, tpchr);
		return;
	}

	fseek(in,16,SEEK_SET);
	NES_Reset(RESET_SOFT);

	if (Movie.Mode & MOV_REVIEW)		/* If the user is reviewing an existing movie */
		Movie.Mode = MOV_RECORD;	/* then resume recording once they LOAD state */

	NES.GameGenie = FALSE;	/* If the savestate uses it, it'll turn back on shortly */
	CheckMenuItem(GetMenu(mWnd),ID_CPU_GAMEGENIE,MF_UNCHECKED);

	if (States_LoadData(in, flen))
		GFX_ShowText("State loaded: %i", States.SelSlot);
	else	GFX_ShowText("State loaded with errors: %i", States.SelSlot);
	Movie_ShowFrame();
	fclose(in);
}
