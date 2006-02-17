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
#include "Controllers.h"
#include "GFX.h"
#include "MapperInterface.h"

struct	tStates	States;

extern FILE *movie;

void	States_Init (void)
{
	States.SelSlot = 0;
	States.NeedSave = FALSE;
	States.NeedLoad = FALSE;
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
	States.SelSlot = Slot;
	GFX_ShowText("State Selected: %i", Slot);
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
		/* TODO - Add CTRL state */
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
	if (Controllers.MovieMode & (MOV_RECORD | MOV_PLAY))
	{
		int moviepos = ftell(movie);
		int mlen, mpos;
		char tps[4];
		unsigned char tpc;
		unsigned long tpl;
		int tpi;
		extern int ReRecords;

		fwrite("NMOV",1,4,out);		flen += 4;
		fwrite(&clen,1,4,out);		flen += 4;
		clen = 0;
		
		rewind(movie);
		fseek(movie,16,SEEK_SET);
		fread(tps,4,1,movie);
		fread(&mlen,4,1,movie);
		mpos = ftell(movie);
		while (memcmp(tps,"NMOV",4))
		{	/* find the NMOV block in the movie */
			fseek(movie,mlen,SEEK_CUR);
			fread(tps,4,1,movie);
			fread(&mlen,4,1,movie);
			mpos = ftell(movie);
		}

		fread(&tpc,1,1,movie);	fwrite(&tpc,1,1,out);	clen++;
		fread(&tpc,1,1,movie);	fwrite(&tpc,1,1,out);	clen++;
		fread(&tpc,1,1,movie);	fwrite(&tpc,1,1,out);	clen++;
		fread(&tpc,1,1,movie);	fwrite(&tpc,1,1,out);	clen++;
		fseek(movie,4,SEEK_CUR);fwrite(&ReRecords,4,1,out);	clen += 4;	// ignore rerecord count stored in movie

		fread(&tpl,4,1,movie);	fwrite(&tpl,4,1,out);	clen += 4;
		while (tpl > 0)
		{
			fread(&tpc,1,1,movie);
			fwrite(&tpc,1,1,out);	clen++;
		}

		fread(&tpl,4,1,movie);	clen += 4;	// the MLEN field, which is NOT yet accurate
		tpl = (moviepos - mpos) - clen;
		fwrite(&tpl,4,1,out);
		
		tpi = tpl;
		while (tpi > 0)
		{
			fread(&tpc,1,1,movie);
			fwrite(&tpc,1,1,out);	clen++;
			tpi--;
		}
		rewind(movie);
		fseek(movie,moviepos,SEEK_SET);

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
	sprintf(tpchr,"%s.ns%i",States.BaseFilename,States.SelSlot);
	out = fopen(tpchr,"w+b");
	flen = 0;

	fwrite("NSS\x1A",1,4,out);
	fwrite(STATES_VERSION,1,4,out);
	fwrite(&flen,1,4,out);
	if (Controllers.MovieMode & (MOV_RECORD | MOV_PLAY))
		fwrite("NREC",1,4,out);
	else	fwrite("NSAV",1,4,out);

	flen = States_SaveData(out);

	// Write final filesize
	fseek(out,8,SEEK_SET);
	fwrite(&flen,1,4,out);
	fclose(out);

	GFX_ShowText("State saved: %i", States.SelSlot);
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
		{
			if (Controllers.MovieMode & MOV_RECORD)
			{	// are we recording?
				extern char MovieName[256];
				extern int ReRecords;
				char tps[5];
				int mlen;
				int tpi;
				unsigned long tpl;
				unsigned char tpc;

				rewind(movie);
				fseek(movie,16,SEEK_SET);
				fread(tps,4,1,movie);
				fread(&mlen,4,1,movie);
				while (memcmp(tps,"NMOV",4))
				{	/* find the NMOV block in the movie */
					fseek(movie,mlen,SEEK_CUR);
					fread(tps,4,1,movie);
					fread(&mlen,4,1,movie);
				}
				fseek(movie,-4,SEEK_CUR);
				fwrite(&clen,4,1,movie);	// overwrite NMOV block length

				fread(&tpl,4,1,in);	fwrite(&tpl,4,1,movie);	clen -= 4;	// CTRL0, CTRL1, CTEXT, EXTR
				fread(&tpl,4,1,in);				clen -= 4;	// RREC
				if (ReRecords < (int)tpl)
					ReRecords = tpl;
				ReRecords++;
				fwrite(&tpl,4,1,movie);
				fread(&tpl,4,1,in);	fwrite(&tpl,4,1,movie);	clen -= 4;	// ILEN
				tpi = tpl;					clen -= tpl;	// INFO
				while (tpi > 0)
				{
					fread(&tpc,1,1,in);
					fwrite(&tpc,1,1,movie);
					tpi--;
				}
				fread(&tpl,4,1,in);	fwrite(&tpl,4,1,movie);	clen -= 4;	// MLEN
				tpi = tpl;					clen -= tpl;	// MDAT
				while (tpi > 0)
				{
					if (Controllers.Port1.MovLen)
					{
						fread(Controllers.Port1.MovData,1,Controllers.Port1.MovLen,in);
						fwrite(Controllers.Port1.MovData,1,Controllers.Port1.MovLen,movie);
						tpi -= Controllers.Port1.MovLen;
					}
					if (Controllers.Port2.MovLen)
					{
						fread(Controllers.Port2.MovData,1,Controllers.Port2.MovLen,in);
						fwrite(Controllers.Port2.MovData,1,Controllers.Port2.MovLen,movie);
						tpi -= Controllers.Port2.MovLen;
					}
					if (Controllers.ExpPort.MovLen)
					{
						fread(Controllers.ExpPort.MovData,1,Controllers.ExpPort.MovLen,in);
						fwrite(Controllers.ExpPort.MovData,1,Controllers.ExpPort.MovLen,movie);
						tpi -= Controllers.ExpPort.MovLen;
					}
				}
				Controllers.Port1.Frame(&Controllers.Port1,MOV_PLAY);
				Controllers.Port2.Frame(&Controllers.Port2,MOV_PLAY);
				Controllers.ExpPort.Frame(&Controllers.ExpPort,MOV_PLAY);
			}
			else
			{	// nope, skip it
				fseek(in,clen,SEEK_CUR);
				clen = 0;
			}
		}
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
	if (Controllers.MovieMode & MOV_PLAY)
	{
		GFX_ShowText("Cannot load state while playing a movie!");
		return;
	}

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
		GFX_ShowText("Corrupted save state: %i", States.SelSlot);
		return;
	}
	fread(tpchr,1,4,in);
	if (0 && memcmp(tpchr,STATES_VERSION,4))
	{	/* For now, allow savestates with wrong version */
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
		/* Non-movie savestate, can NOT load these while recording */
		if (Controllers.MovieMode & MOV_RECORD)
		{
			fclose(in);
			GFX_ShowText("Selected savestate (%i) does not contain movie data!", States.SelSlot);
			return;
		}
	}
	else if (!memcmp(tpchr,"\0\0\0\0",4))
	{
		/* For now, allow movies with a null type */
	}
	else
	{
		fclose(in);
		tpchr[4] = 0;
		GFX_ShowText("Selected savestate (%i) has unknown type! (%s)", States.SelSlot, tpchr);
		return;
	}

	fseek(in,16,SEEK_SET);
	NES_Reset(RESET_HARD);

	if (States_LoadData(in, flen))
		GFX_ShowText("State loaded: %i", States.SelSlot);
	else	GFX_ShowText("State loaded with errors: %i", States.SelSlot);
	fclose(in);
}
