/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) QMT Productions
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

namespace States
{
TCHAR BaseFilename[MAX_PATH];
int SelSlot;

void	Init (void)
{
	SelSlot = 0;
}

void	SetFilename (TCHAR *Filename)
{
	// all we need is the base filename
	_tsplitpath(Filename, NULL, NULL, BaseFilename, NULL);
}

void	SetSlot (int Slot)
{
	TCHAR tpchr[MAX_PATH];
	FILE *tmp;
	SelSlot = Slot;
	_stprintf(tpchr, _T("%s\\States\\%s.ns%i"), DataPath, BaseFilename, SelSlot);
	tmp = _tfopen(tpchr, _T("rb"));
	if (tmp)
	{
		fclose(tmp);
		PrintTitlebar(_T("State Selected: %i (occupied)"), Slot);
	}
	else	PrintTitlebar(_T("State Selected: %i (empty)"), Slot);
}

int	SaveData (FILE *out)
{
	int flen = 0, clen;
	{
		fwrite("CPUS", 1, 4, out);	flen += 4;
		fwrite(&clen, 4, 1, out);	flen += 4;
		clen = CPU::Save(out);
		fseek(out, -clen - 4, SEEK_CUR);
		fwrite(&clen, 4, 1, out);
		fseek(out, clen, SEEK_CUR);	flen += clen;
	}
	{
		fwrite("PPUS", 1, 4, out);	flen += 4;
		fwrite(&clen, 4, 1, out);	flen += 4;
		clen = PPU::Save(out);
		fseek(out, -clen - 4, SEEK_CUR);
		fwrite(&clen, 4, 1, out);
		fseek(out, clen, SEEK_CUR);	flen += clen;
	}
	{
		fwrite("APUS", 1, 4, out);	flen += 4;
		fwrite(&clen, 4, 1, out);	flen += 4;
		clen = APU::Save(out);
		fseek(out, -clen - 4, SEEK_CUR);
		fwrite(&clen, 4, 1, out);
		fseek(out, clen, SEEK_CUR);	flen += clen;
	}
	{
		fwrite("CONT", 1, 4, out);	flen += 4;
		fwrite(&clen, 4, 1, out);	flen += 4;
		clen = Controllers::Save(out);
		fseek(out, -clen - 4, SEEK_CUR);
		fwrite(&clen, 4, 1, out);
		fseek(out, clen, SEEK_CUR);	flen += clen;
	}
	if (Genie::CodeStat & 1)
	{
		fwrite("GENI", 1, 4, out);	flen += 4;
		fwrite(&clen, 4, 1, out);	flen += 4;
		clen = Genie::Save(out);
		fseek(out, -clen - 4, SEEK_CUR);
		fwrite(&clen, 4, 1, out);
		fseek(out, clen, SEEK_CUR);	flen += clen;
	}
	{
		for (clen = sizeof(NES::PRG_RAM) - 1; clen >= 0; clen--)
			if (NES::PRG_RAM[clen >> 12][clen & 0xFFF])
				break;
		if (clen >= 0)
		{
			clen++;
			fwrite("NPRA", 1, 4, out);		flen += 4;
			fwrite(&clen, 4, 1, out);		flen += 4;
			fwrite(NES::PRG_RAM, 1, clen, out);	flen += clen;	//	PRAM	uint8[...]	PRG_RAM data
		}
	}
	{
		for (clen = sizeof(NES::CHR_RAM) - 1; clen >= 0; clen--)
			if (NES::CHR_RAM[clen >> 10][clen & 0x3FF])
				break;
		if (clen >= 0)
		{
			clen++;
			fwrite("NCRA", 1, 4, out);		flen += 4;
			fwrite(&clen, 4, 1, out);		flen += 4;
			fwrite(NES::CHR_RAM, 1, clen, out);	flen += clen;	//	CRAM	uint8[...]	CHR_RAM data
		}
	}
	if (RI.ROMType == ROM_FDS)
	{
		fwrite("DISK", 1, 4, out);	flen += 4;
		fwrite(&clen, 4, 1, out);	flen += 4;
		clen = NES::FDSSave(out);
		fseek(out, -clen - 4, SEEK_CUR);
		fwrite(&clen, 4, 1, out);
		fseek(out, clen, SEEK_CUR);	flen += clen;
	}
	{
		if ((MI) && (MI->SaveLoad))
			clen = MI->SaveLoad(STATE_SIZE, 0, NULL);
		else	clen = 0;
		if (clen)
		{
			unsigned char *tpmi = new unsigned char[clen];
			MI->SaveLoad(STATE_SAVE, 0, tpmi);
			fwrite("MAPR", 1, 4, out);	flen += 4;
			fwrite(&clen, 4, 1, out);	flen += 4;
			fwrite(tpmi, 1, clen, out);	flen += clen;	//	CUST	uint8[...]	Custom mapper data
			delete[] tpmi;
		}
	}
	if (Movie::Mode)	// save state when recording, reviewing, OR playing
	{
		fwrite("NMOV", 1, 4, out);	flen += 4;
		fwrite(&clen, 4, 1, out);	flen += 4;
		clen = Movie::Save(out);
		fseek(out, -clen - 4, SEEK_CUR);
		fwrite(&clen, 4, 1, out);
		fseek(out, clen, SEEK_CUR);	flen += clen;
	}
	return flen;
}

void	SaveState (void)
{
	TCHAR tps[MAX_PATH];
	int flen;
	FILE *out;

	if (Genie::CodeStat & 0x80)
	{
		PrintTitlebar(_T("Cannot save state at the Game Genie code entry screen!"));
		return;
	}

	_stprintf(tps, _T("%s\\States\\%s.ns%i"), DataPath, BaseFilename, SelSlot);
	out = _tfopen(tps, _T("w+b"));
	flen = 0;

	fwrite("NSS\x1A", 1, 4, out);
	SaveVersion(out, STATES_CUR_VERSION);
	fwrite(&flen, 4, 1, out);
	if (Movie::Mode)	// save NREC during playback as well
		fwrite("NREC", 1, 4, out);
	else	fwrite("NSAV", 1, 4, out);

	flen = SaveData(out);

	// Write final filesize
	fseek(out, 8, SEEK_SET);
	fwrite(&flen, 4, 1, out);
	fclose(out);

	PrintTitlebar(_T("State saved: %i"), SelSlot);
	Movie::ShowFrame();
}

BOOL	LoadData (FILE *in, int flen, int version_id)
{
	char csig[4];
	int clen;
	BOOL SSOK = TRUE;
	fread(&csig, 1, 4, in);	flen -= 4;
	fread(&clen, 4, 1, in);	flen -= 4;
	while (flen > 0)
	{
		// If we encountered EOF while attempting to read data, bail out
		// This can happen if the length in the file header was wrong
		if (feof(in))
			return FALSE;
		flen -= clen;
		if (!memcmp(csig, "CPUS", 4))
			clen -= CPU::Load(in, version_id);
		else if (!memcmp(csig, "PPUS", 4))
			clen -= PPU::Load(in, version_id);
		else if (!memcmp(csig, "APUS", 4))
			clen -= APU::Load(in, version_id);
		else if (!memcmp(csig, "CONT", 4))
			clen -= Controllers::Load(in, version_id);
		else if (!memcmp(csig, "CTRL", 4))
		{
			// Skip old block without erroring out
			fseek(in, clen, SEEK_CUR);
			clen = 0;
		}
		else if (!memcmp(csig, "GENI", 4))
			clen -= Genie::Load(in, version_id);
		else if (!memcmp(csig, "NPRA", 4))
		{
			memset(NES::PRG_RAM, 0, sizeof(NES::PRG_RAM));
			fread(NES::PRG_RAM, 1, clen, in);	clen = 0;
		}
		else if (!memcmp(csig, "NCRA", 4))
		{
			memset(NES::CHR_RAM, 0, sizeof(NES::CHR_RAM));
			fread(NES::CHR_RAM, 1, clen, in);	clen = 0;
		}
		else if (!memcmp(csig, "DISK", 4))
		{
			if (RI.ROMType == ROM_FDS)
				clen -= NES::FDSLoad(in, version_id);
			else	EI.DbgOut(_T("Error - DISK save block found for non-FDS game!"));
		}
		else if (!memcmp(csig, "MAPR", 4))
		{
			if ((MI) && (MI->SaveLoad))
			{
				unsigned char *tpmi = new unsigned char[clen];
				fread(tpmi, 1, clen, in);		//	CUST	uint8[...]	Custom mapper data
				if (clen != MI->SaveLoad((version_id > 1002) ? STATE_LOAD_VER : STATE_LOAD, 0, tpmi))
					SSOK = FALSE;
				delete[] tpmi;
				clen = 0;
			}
		}
		else if (!memcmp(csig, "NMOV", 4))
			clen -= Movie::Load(in, version_id);
		else	EI.DbgOut(_T("Unknown savestate block '%c%c%c%c' encountered!"), csig[0], csig[1], csig[2], csig[3]);
		if (clen != 0)
		{
			EI.DbgOut(_T("Savestate block '%c%c%c%c' had wrong size, off by %i!"), csig[0], csig[1], csig[2], csig[3], clen);
			SSOK = FALSE;			// too much, or too little
			fseek(in, clen, SEEK_CUR);	// seek back to the block boundary
		}
		if (flen > 0)
		{
			fread(&csig, 1, 4, in);	flen -= 4;
			fread(&clen, 4, 1, in);	flen -= 4;
		}
	}
	return SSOK;
}

void	LoadState (void)
{
	TCHAR tps[MAX_PATH];
	char tpc[5];
	int version_id;
	FILE *in;
	int flen;

	_stprintf(tps, _T("%s\\States\\%s.ns%i"), DataPath, BaseFilename, SelSlot);
	in = _tfopen(tps, _T("rb"));
	if (!in)
	{
		PrintTitlebar(_T("No such save state: %i"), SelSlot);
		return;
	}

	fread(tpc, 1, 4, in);
	if (memcmp(tpc, "NSS\x1a", 4))
	{
		fclose(in);
		PrintTitlebar(_T("Not a valid savestate file: %i"), SelSlot);
		return;
	}
	version_id = LoadVersion(in);
	if ((version_id < STATES_MIN_VERSION) || (version_id > STATES_CUR_VERSION))
	{
		fclose(in);
		tpc[4] = 0;
		PrintTitlebar(_T("Invalid or unsupported savestate version (%i): %i"), version_id, SelSlot);
		return;
	}
	fread(&flen, 4, 1, in);
	fread(tpc, 1, 4, in);

	if (!memcmp(tpc, "NMOV", 4))
	{
		fclose(in);
		PrintTitlebar(_T("Selected savestate (%i) is a movie file - cannot load!"), SelSlot);
		return;
	}
	else if (!memcmp(tpc, "NREC", 4))
	{
		// Movie savestate, can always load these
	}
	else if (!memcmp(tpc, "NSAV", 4))
	{
		// Non-movie savestate, can NOT load these while a movie is open
		if (Movie::Mode)
		{
			fclose(in);
			PrintTitlebar(_T("Selected savestate (%i) does not contain movie data!"), SelSlot);
			return;
		}
	}
	else
	{
		fclose(in);
		tpc[4] = 0;
		PrintTitlebar(_T("Selected savestate (%i) has unknown type! (%hs)"), SelSlot, tpc);
		return;
	}

	fseek(in, 16, SEEK_SET);
	NES::Reset(RESET_SOFT);

	if (Movie::Mode & MOV_REVIEW)		// If the user is reviewing an existing movie
		Movie::Mode = MOV_RECORD;	// then resume recording once they LOAD state

	NES::GameGenie = FALSE;			// If the savestate uses it, it'll turn back on shortly
	CheckMenuItem(hMenu, ID_CPU_GAMEGENIE, MF_UNCHECKED);

	if (LoadData(in, flen, version_id))
		PrintTitlebar(_T("%s loaded: %i"), (version_id == STATES_CUR_VERSION) ? _T("State") : _T("Old state"), SelSlot);
	else	PrintTitlebar(_T("%s loaded with errors: %i"), (version_id == STATES_CUR_VERSION) ? _T("State") : _T("Old state"), SelSlot);
	Movie::ShowFrame();
	fclose(in);
}

void	SaveVersion (FILE *out, int version_id)
{
	BYTE buf[4];
	buf[0] = (version_id / 1000) % 10 + '0';
	buf[1] = (version_id /  100) % 10 + '0';
	buf[2] = (version_id /   10) % 10 + '0';
	buf[3] = (version_id /    1) % 10 + '0';
	fwrite(buf, 1, 4, out);
}

int	LoadVersion (FILE *in)
{
	BYTE buf[4];
	fread(buf, 1, 4, in);
	int version_id = (buf[0] - '0') * 1000 + (buf[1] - '0') * 100 + (buf[2] - '0') * 10 + (buf[3] - '0');
	return version_id;
}
} // namespace States
