/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "MapperInterface.h"
#include "NES.h"
#include "CPU.h"
#include "PPU.h"
#include "APU.h"
#include "GFX.h"
#include "AVI.h"
#include "Debugger.h"
#include "States.h"
#include "Movie.h"
#include "Controllers.h"
#include "Genie.h"
#include <shellapi.h>

#pragma comment(lib, "shlwapi.lib")

namespace NES
{
int SRAM_Size;

int PRGMask, CHRMask;

BOOL ROMLoaded;
BOOL DoStop, Running, Scanline;
BOOL GameGenie;
BOOL SoundEnabled;
BOOL AutoRun;
BOOL FrameStep, GotStep;
BOOL HasMenu;

unsigned char PRG_ROM[MAX_PRGROM_SIZE][0x1000];
unsigned char PRG_RAM[MAX_PRGRAM_SIZE][0x1000];
unsigned char CHR_ROM[MAX_CHRROM_SIZE][0x400];
unsigned char CHR_RAM[MAX_CHRRAM_SIZE][0x400];

const TCHAR *CompatLevel[COMPAT_NUMTYPES] = {_T("Unsupported"), _T("Partially supported"), _T("Mostly supported"), _T("Fully supported!")};

void	Init (void)
{
	MapperInterface::Init();
	Controllers::Init();
	APU::Init();
	GFX::Init();
	AVI::Init();
#ifdef	ENABLE_DEBUGGER
	Debugger::Init();
#endif	/* ENABLE_DEBUGGER */
	States::Init();
#ifdef	ENABLE_DEBUGGER
	Debugger::SetMode(0);
#endif	/* ENABLE_DEBUGGER */
	LoadSettings();
	SetupDataPath();

	GFX::Create();

	CloseFile();

	Running = FALSE;
	DoStop = FALSE;
	GameGenie = FALSE;
	ROMLoaded = FALSE;
	ZeroMemory(&RI, sizeof(RI));

	UpdateTitlebar();
}

void	Release (void)
{
	if (ROMLoaded)
		CloseFile();
	SaveSettings();
	APU::Release();
	GFX::Release();
	Controllers::Release();
	MapperInterface::Release();

	DestroyWindow(hMainWnd);
}

void	OpenFile (TCHAR *filename)
{
	size_t len = _tcslen(filename);
	const TCHAR *LoadRet = NULL;
	FILE *data;
	if (ROMLoaded)
		CloseFile();

	EI.DbgOut(_T("Loading file '%s'..."), filename);
	data = _tfopen(filename, _T("rb"));

	if (!data)
	{
		MessageBox(hMainWnd, _T("Unable to open file!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
		CloseFile();
		return;
	}

	if (!_tcsicmp(filename + len - 4, _T(".NES")))
		LoadRet = OpenFileiNES(data);
	else if (!_tcsicmp(filename + len - 4, _T(".NSF")))
		LoadRet = OpenFileNSF(data);
	else if (!_tcsicmp(filename + len - 4, _T(".UNF")))
		LoadRet = OpenFileUNIF(data);
	else if (!_tcsicmp(filename + len - 5, _T(".UNIF")))
		LoadRet = OpenFileUNIF(data);
	else if (!_tcsicmp(filename + len - 4, _T(".FDS")))
		LoadRet = OpenFileFDS(data);
	else	LoadRet = _T("File type not recognized!");
	fclose(data);

	if (LoadRet)
	{
		MessageBox(hMainWnd, LoadRet, _T("Nintendulator"), MB_OK | MB_ICONERROR);
		CloseFile();
		return;
	}
	// if the ROM loaded without errors, drop the filename into ROMInfo
	RI.Filename = _tcsdup(filename);
	ROMLoaded = TRUE;
	EI.DbgOut(_T("Loaded successfully!"));
	States::SetFilename(filename);

	HasMenu = FALSE;
	if (MI->Config)
	{
		if (MI->Config(CFG_WINDOW, FALSE))
			HasMenu = TRUE;
		EnableMenuItem(hMenu, ID_GAME, MF_ENABLED);
	}
	else	EnableMenuItem(hMenu, ID_GAME, MF_GRAYED);
	LoadSRAM();

	if (RI.ROMType == ROM_NSF)
	{
		GameGenie = FALSE;
		CheckMenuItem(hMenu, ID_CPU_GAMEGENIE, MF_UNCHECKED);
		EnableMenuItem(hMenu, ID_CPU_GAMEGENIE, MF_GRAYED);
	}
	else
	{
		EnableMenuItem(hMenu, ID_CPU_SAVESTATE, MF_ENABLED);
		EnableMenuItem(hMenu, ID_CPU_LOADSTATE, MF_ENABLED);
		EnableMenuItem(hMenu, ID_CPU_PREVSTATE, MF_ENABLED);
		EnableMenuItem(hMenu, ID_CPU_NEXTSTATE, MF_ENABLED);

		EnableMenuItem(hMenu, ID_MISC_PLAYMOVIE, MF_ENABLED);
		EnableMenuItem(hMenu, ID_MISC_RECORDMOVIE, MF_ENABLED);

		EnableMenuItem(hMenu, ID_MISC_STARTAVICAPTURE, MF_ENABLED);
	}

	EnableMenuItem(hMenu, ID_FILE_CLOSE, MF_ENABLED);
	EnableMenuItem(hMenu, ID_CPU_RUN, MF_ENABLED);
	EnableMenuItem(hMenu, ID_CPU_STEP, MF_ENABLED);
	EnableMenuItem(hMenu, ID_CPU_STOP, MF_ENABLED);
	EnableMenuItem(hMenu, ID_CPU_SOFTRESET, MF_ENABLED);
	EnableMenuItem(hMenu, ID_CPU_HARDRESET, MF_ENABLED);

	DrawMenuBar(hMainWnd);

#ifdef	ENABLE_DEBUGGER
	Debugger::NTabChanged = TRUE;
	Debugger::PalChanged = TRUE;
	Debugger::PatChanged = TRUE;
	Debugger::SprChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */

	Reset(RESET_HARD);
	if ((AutoRun) || (RI.ROMType == ROM_NSF))
		Start(FALSE);
}

int	FDSSave (FILE *out)
{
	int clen = 0;
	int x, y, n = 0;
	unsigned long data = 0;
	writeLong(data);
	for (x = 0; x < RI.FDS_NumSides << 4; x++)
	{
		for (y = 0; y < 4096; y++)
		{
			if (PRG_ROM[x][y] != PRG_ROM[(MAX_PRGROM_SIZE >> 1) | x][y])
			{
				data = y | (x << 12) | (PRG_ROM[x][y] << 24);
				writeLong(data);
				n++;
			}
		}
	}
	fseek(out, -clen, SEEK_CUR);
	fwrite(&n, 4, 1, out);
	fseek(out, clen - 4, SEEK_CUR);
	return clen;
}

int	FDSLoad (FILE *in)
{
	int clen = 0;
	int n;
	unsigned long data;
	readLong(n);
	for (; n >= 0; n--)
	{
		readLong(data);
		if (feof(in))
		{
			clen -= 4;
			break;
		}
		PRG_ROM[(data >> 12) & 0x3FF][data & 0xFFF] = (unsigned char)(data >> 24);
	}
	return clen;
}

void	SaveSRAM (void)
{
	TCHAR Filename[MAX_PATH];
	FILE *SRAMFile;
	if (!SRAM_Size)
		return;
	if (RI.ROMType == ROM_FDS)
		_stprintf(Filename, _T("%s\\FDS\\%s.fsv"), DataPath, States::BaseFilename);
	else	_stprintf(Filename, _T("%s\\SRAM\\%s.sav"), DataPath, States::BaseFilename);
	SRAMFile = _tfopen(Filename, _T("wb"));
	if (RI.ROMType == ROM_FDS)
	{
		FDSSave(SRAMFile);
		EI.DbgOut(_T("Saved disk changes"));
	}
	else
	{
		fwrite(PRG_RAM, 1, SRAM_Size, SRAMFile);
		EI.DbgOut(_T("Saved SRAM."));
	}
	fclose(SRAMFile);
}
void	LoadSRAM (void)
{
	TCHAR Filename[MAX_PATH];
	FILE *SRAMFile;
	int len;
	if (!SRAM_Size)
		return;
	if (RI.ROMType == ROM_FDS)
		_stprintf(Filename, _T("%s\\FDS\\%s.fsv"), DataPath, States::BaseFilename);
	else	_stprintf(Filename, _T("%s\\SRAM\\%s.sav"), DataPath, States::BaseFilename);
	SRAMFile = _tfopen(Filename, _T("rb"));
	if (!SRAMFile)
		return;
	fseek(SRAMFile, 0, SEEK_END);
	len = ftell(SRAMFile);
	fseek(SRAMFile, 0, SEEK_SET);
	if (RI.ROMType == ROM_FDS)
	{
		if (len == FDSLoad(SRAMFile))
			EI.DbgOut(_T("Loaded disk changes"));
		else	EI.DbgOut(_T("File length mismatch while loading disk data!"));
	}
	else
	{
		ZeroMemory(PRG_RAM, SRAM_Size);
		fread(PRG_RAM, 1, SRAM_Size, SRAMFile);
		if (len == SRAM_Size)
			EI.DbgOut(_T("Loaded SRAM (%i bytes)."), SRAM_Size);
		else	EI.DbgOut(_T("File length mismatch while loading SRAM!"));
	}
	fclose(SRAMFile);
}

void	CloseFile (void)
{
	int i;
	SaveSRAM();
	if (ROMLoaded)
	{
		MapperInterface::UnloadMapper();
		ROMLoaded = FALSE;
		EI.DbgOut(_T("ROM unloaded."));
	}

	if (RI.ROMType)
	{
		if (RI.ROMType == ROM_UNIF)
			delete[] RI.UNIF_BoardName;
		else if (RI.ROMType == ROM_NSF)
		{
			delete[] RI.NSF_Title;
			delete[] RI.NSF_Artist;
			delete[] RI.NSF_Copyright;
		}
		// Allocated using _tcsdup()
		free(RI.Filename);
		ZeroMemory(&RI, sizeof(RI));
	}

	if (AVI::handle)
		AVI::End();
	if (Movie::Mode)
		Movie::Stop();

	EnableMenuItem(hMenu, ID_GAME, MF_GRAYED);
	EnableMenuItem(hMenu, ID_CPU_GAMEGENIE, MF_ENABLED);
	EnableMenuItem(hMenu, ID_CPU_SAVESTATE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_CPU_LOADSTATE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_CPU_PREVSTATE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_CPU_NEXTSTATE, MF_GRAYED);

	EnableMenuItem(hMenu, ID_FILE_CLOSE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_CPU_RUN, MF_GRAYED);
	EnableMenuItem(hMenu, ID_CPU_STEP, MF_GRAYED);
	EnableMenuItem(hMenu, ID_CPU_STOP, MF_GRAYED);
	EnableMenuItem(hMenu, ID_CPU_SOFTRESET, MF_GRAYED);
	EnableMenuItem(hMenu, ID_CPU_HARDRESET, MF_GRAYED);

	EnableMenuItem(hMenu, ID_MISC_PLAYMOVIE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MISC_RECORDMOVIE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MISC_STARTAVICAPTURE, MF_GRAYED);

	SRAM_Size = 0;
	for (i = 0; i < 16; i++)
	{
		CPU::PRGPointer[i] = PRG_RAM[0];
		CPU::ReadHandler[i] = CPU::ReadPRG;
		CPU::WriteHandler[i] = CPU::WritePRG;
		CPU::Readable[i] = FALSE;
		CPU::Writable[i] = FALSE;
		PPU::CHRPointer[i] = CHR_RAM[0];
		PPU::ReadHandler[i] = PPU::BusRead;
		PPU::WriteHandler[i] = PPU::BusWriteCHR;
		PPU::Writable[i] = FALSE;
	}
}

#define MKID(a) ((unsigned long) \
		(((a) >> 24) & 0x000000FF) | \
		(((a) >>  8) & 0x0000FF00) | \
		(((a) <<  8) & 0x00FF0000) | \
		(((a) << 24) & 0xFF000000))

const TCHAR *	OpenFileiNES (FILE *in)
{
	int i;
	unsigned char Header[16];
	BOOL p2Found;
	DWORD p2;

	fread(Header, 1, 16, in);
	if (*(unsigned long *)Header != MKID('NES\x1A'))
		return _T("iNES header signature not found!");

	if ((Header[7] & 0x0C) == 0x04)
		return _T("Header is corrupted by \"DiskDude!\" - please repair it and try again.");

	if ((Header[7] & 0x0C) == 0x0C)
		return _T("Header format not recognized - please repair it and try again.");

	RI.ROMType = ROM_INES;

	RI.INES_Version = 1;	// start off by parsing as iNES v1

	RI.INES_PRGSize = Header[4];
	RI.INES_CHRSize = Header[5];
	RI.INES_MapperNum = ((Header[6] & 0xF0) >> 4) | (Header[7] & 0xF0);
	RI.INES_Flags = (Header[6] & 0x0F) | ((Header[7] & 0x0F) << 4);

	if ((Header[7] & 0x0C) == 0x08)
	{
		EI.DbgOut(_T("NES 2.0 ROM image detected!"));
		RI.INES_Version = 2;

		RI.INES_MapperNum |= (Header[8] & 0x0F) << 8;
		RI.INES2_SubMapper = (Header[8] & 0xF0) >> 4;
		RI.INES_PRGSize |= (Header[9] & 0x0F) << 8;
		RI.INES_CHRSize |= (Header[9] & 0xF0) << 4;
		RI.INES2_PRGRAM = Header[10];
		RI.INES2_CHRRAM = Header[11];
		RI.INES2_TVMode = Header[12];
		RI.INES2_VSDATA = Header[13];
	}
	else
	{
		for (i = 8; i < 0x10; i++)
			if (Header[i] != 0)
			{
				EI.DbgOut(_T("Unrecognized data found at header offset %i - you are recommended to clean this ROM and reload it."), i);
				break;
			}
	}

	if (RI.INES_Flags & 0x04)
		return _T("Trained ROMs are unsupported!");

	if (RI.INES_PRGSize > MAX_PRGROM_SIZE / 4)
		return _T("PRG ROM is too large! Increase MAX_PRGROM_SIZE and recompile!");

	if (RI.INES_CHRSize > MAX_CHRROM_SIZE / 8)
		return _T("CHR ROM is too large! Increase MAX_CHRROM_SIZE and recompile!");

	fread(PRG_ROM, 1, RI.INES_PRGSize * 0x4000, in);
	fread(CHR_ROM, 1, RI.INES_CHRSize * 0x2000, in);

	PRGMask = ((RI.INES_PRGSize << 2) - 1) & MAX_PRGROM_MASK;
	CHRMask = ((RI.INES_CHRSize << 3) - 1) & MAX_CHRROM_MASK;

	p2Found = FALSE;
	for (p2 = 1; p2 < 0x40000000; p2 <<= 1)
		if (p2 == RI.INES_PRGSize)
			p2Found = TRUE;
	if (!p2Found)
		PRGMask = MAX_PRGROM_MASK;

	p2Found = FALSE;
	for (p2 = 1; p2 < 0x40000000; p2 <<= 1)
		if (p2 == RI.INES_CHRSize)
			p2Found = TRUE;
	if (!p2Found)
		CHRMask = MAX_CHRROM_MASK;

	
	if (!MapperInterface::LoadMapper(&RI))
	{
		static TCHAR err[256];
		_stprintf(err, _T("Mapper %i not supported!"), RI.INES_MapperNum);
		return err;
	}
	EI.DbgOut(_T("iNES ROM image loaded: mapper %i (%s) - %s"), RI.INES_MapperNum, MI->Description, CompatLevel[MI->Compatibility]);
	EI.DbgOut(_T("PRG: %iKB; CHR: %iKB"), RI.INES_PRGSize << 4, RI.INES_CHRSize << 3);
	EI.DbgOut(_T("Flags: %s%s"), RI.INES_Flags & 0x02 ? _T("Battery-backed SRAM, ") : _T(""), RI.INES_Flags & 0x08 ? _T("Four-screen VRAM") : (RI.INES_Flags & 0x01 ? _T("Vertical mirroring") : _T("Horizontal mirroring")));

	// Special checks for Playchoice-10 and Vs. Unisystem ROMs to auto-select palette
	if ((RI.INES_Flags & 0x10) && (RI.INES_Version == 2))
	{
		switch (RI.INES2_VSDATA & 0x0F)
		{
		case 2:	GFX::LoadPalette(GFX::PALETTE_VS1);	break;
		case 3:	GFX::LoadPalette(GFX::PALETTE_VS2);	break;
		case 4:	GFX::LoadPalette(GFX::PALETTE_VS3);	break;
		case 5:	GFX::LoadPalette(GFX::PALETTE_VS4);	break;
		default:
			GFX::LoadPalette(GFX::PALETTE_PC10);	break;
		}
	}
	else if (RI.INES_Flags & 0x20)
		GFX::LoadPalette(GFX::PALETTE_PC10);
	// Need to do this, in case the last loaded ROM was one of the above
	else	GFX::LoadPalette(PPU::IsPAL ? GFX::PalettePAL : GFX::PaletteNTSC);

	if ((RI.INES_Version == 2) && !(RI.INES2_TVMode & 0x02))
		SetCPUMode(RI.INES2_TVMode & 0x01);
	return NULL;
}

const TCHAR *	OpenFileUNIF (FILE *in)
{
	unsigned long Signature, BlockLen;
	unsigned char *tPRG[0x10], *tCHR[0x10];
	unsigned char *PRGPoint, *CHRPoint;
	unsigned long PRGsize = 0, CHRsize = 0;
	const TCHAR *error = NULL;
	int i;
	unsigned char tp;
	BOOL p2Found;
	DWORD p2;

	fread(&Signature, 4, 1, in);
	if (Signature != MKID('UNIF'))
		return _T("UNIF header signature not found!");

	fseek(in, 28, SEEK_CUR);	// skip "expansion area"

	RI.ROMType = ROM_UNIF;

	RI.UNIF_BoardName = NULL;
	RI.UNIF_Mirroring = 0;
	RI.UNIF_NTSCPAL = 0;
	RI.UNIF_Battery = FALSE;
	RI.UNIF_NumPRG = 0;
	RI.UNIF_NumCHR = 0;
	for (i = 0; i < 0x10; i++)
	{
		RI.UNIF_PRGSize[i] = 0;
		RI.UNIF_CHRSize[i] = 0;
		tPRG[i] = NULL;
		tCHR[i] = NULL;
	}

	while (!feof(in))
	{
		int id = 0;
		fread(&Signature, 4, 1, in);
		fread(&BlockLen, 4, 1, in);
		if (feof(in))
			continue;
		switch (Signature)
		{
		case MKID('MAPR'):
			RI.UNIF_BoardName = new char[BlockLen];
			fread(RI.UNIF_BoardName, 1, BlockLen, in);
			break;
		case MKID('TVCI'):
			fread(&tp, 1, 1, in);
			RI.UNIF_NTSCPAL = tp;
			break;
		case MKID('BATR'):
			fread(&tp, 1, 1, in);
			RI.UNIF_Battery = TRUE;
			break;
		case MKID('MIRR'):
			fread(&tp, 1, 1, in);
			RI.UNIF_Mirroring = tp;
			break;
		case MKID('PRGF'):	id++;
		case MKID('PRGE'):	id++;
		case MKID('PRGD'):	id++;
		case MKID('PRGC'):	id++;
		case MKID('PRGB'):	id++;
		case MKID('PRGA'):	id++;
		case MKID('PRG9'):	id++;
		case MKID('PRG8'):	id++;
		case MKID('PRG7'):	id++;
		case MKID('PRG6'):	id++;
		case MKID('PRG5'):	id++;
		case MKID('PRG4'):	id++;
		case MKID('PRG3'):	id++;
		case MKID('PRG2'):	id++;
		case MKID('PRG1'):	id++;
		case MKID('PRG0'):
			RI.UNIF_NumPRG++;
			RI.UNIF_PRGSize[id] = BlockLen;
			PRGsize += BlockLen;
			tPRG[id] = new unsigned char[BlockLen];
			fread(tPRG[id], 1, BlockLen, in);
			break;

		case MKID('CHRF'):	id++;
		case MKID('CHRE'):	id++;
		case MKID('CHRD'):	id++;
		case MKID('CHRC'):	id++;
		case MKID('CHRB'):	id++;
		case MKID('CHRA'):	id++;
		case MKID('CHR9'):	id++;
		case MKID('CHR8'):	id++;
		case MKID('CHR7'):	id++;
		case MKID('CHR6'):	id++;
		case MKID('CHR5'):	id++;
		case MKID('CHR4'):	id++;
		case MKID('CHR3'):	id++;
		case MKID('CHR2'):	id++;
		case MKID('CHR1'):	id++;
		case MKID('CHR0'):
			RI.UNIF_NumCHR++;
			RI.UNIF_CHRSize[id] = BlockLen;
			CHRsize += BlockLen;
			tCHR[id] = new unsigned char[BlockLen];
			fread(tCHR[id], 1, BlockLen, in);
			break;
		default:
			fseek(in, BlockLen, SEEK_CUR);
		}
	}

	PRGPoint = PRG_ROM[0];
	CHRPoint = CHR_ROM[0];

	// check for size overflow, but don't immediately return
	// since we need to clean up after this other stuff too
	if (PRGsize > MAX_PRGROM_SIZE * 0x1000)
		error = _T("PRG ROM is too large! Increase MAX_PRGROM_SIZE and recompile!");

	if (CHRsize > MAX_CHRROM_SIZE * 0x400)
		error = _T("CHR ROM is too large! Increase MAX_CHRROM_SIZE and recompile!");

	PRGMask = ((PRGsize / 0x1000) - 1) & MAX_PRGROM_MASK;
	CHRMask = ((CHRsize / 0x400) - 1) & MAX_CHRROM_MASK;

	for (i = 0; i < 0x10; i++)
	{
		if (tPRG[i])
		{
			if (!error)
			{
				memcpy(PRGPoint, tPRG[i], RI.UNIF_PRGSize[i]);
				PRGPoint += RI.UNIF_PRGSize[i];
			}
			delete tPRG[i];
		}
		if (tCHR[i])
		{
			if (!error)
			{
				memcpy(CHRPoint, tCHR[i], RI.UNIF_CHRSize[i]);
				CHRPoint += RI.UNIF_CHRSize[i];
			}
			delete tCHR[i];
		}
	}
	if (error)
		return error;

	p2Found = FALSE;
	for (p2 = 1; p2 < 0x40000000; p2 <<= 1)
		if (p2 == PRGsize)
			p2Found = TRUE;
	if (!p2Found)
		PRGMask = MAX_PRGROM_MASK;

	p2Found = FALSE;
	for (p2 = 1; p2 < 0x40000000; p2 <<= 1)
		if (p2 == CHRsize)
			p2Found = TRUE;
	if (!p2Found)
		CHRMask = MAX_CHRROM_MASK;

	if (!MapperInterface::LoadMapper(&RI))
	{
		static TCHAR err[1024];
		_stprintf(err, _T("UNIF boardset \"%hs\" not supported!"), RI.UNIF_BoardName);
		return err;
	}

	if ((RI.UNIF_NTSCPAL == 0) || (RI.UNIF_NTSCPAL == 1))
		SetCPUMode(RI.UNIF_NTSCPAL);

	EI.DbgOut(_T("UNIF file loaded: %hs (%s) - %s"), RI.UNIF_BoardName, MI->Description, CompatLevel[MI->Compatibility]);
	EI.DbgOut(_T("PRG: %iKB; CHR: %iKB"), PRGsize >> 10, CHRsize >> 10);
	EI.DbgOut(_T("Battery status: %s"), RI.UNIF_Battery ? _T("present") : _T("not present"));

	const TCHAR *mir[6] = {_T("Horizontal"), _T("Vertical"), _T("Single-screen L"), _T("Single-screen H"), _T("Four-screen"), _T("Dynamic")};
	EI.DbgOut(_T("Mirroring mode: %i (%s)"), RI.UNIF_Mirroring, mir[RI.UNIF_Mirroring]);

	const TCHAR *ntscpal[3] = {_T("NTSC"), _T("PAL"), _T("Dual")};
	EI.DbgOut(_T("Television standard: %s"), ntscpal[RI.UNIF_NTSCPAL]);
	return NULL;
}

const TCHAR *	OpenFileFDS (FILE *in)
{
	unsigned long Header;
	unsigned char numSides;
	int i;

	fread(&Header, 4, 1, in);
	if (Header != MKID('FDS\x1a'))
		return _T("FDS header signature not found!");
	fread(&numSides, 1, 1, in);
	fseek(in, 11, SEEK_CUR);

	RI.ROMType = ROM_FDS;
	RI.FDS_NumSides = numSides;

	if (RI.FDS_NumSides > (MAX_PRGROM_SIZE >> 1) / 16)
		return _T("FDS image is too large! Increase MAX_PRGROM_SIZE and recompile!");

	for (i = 0; i < numSides; i++)
		fread(PRG_ROM[i << 4], 1, 65500, in);

	memcpy(PRG_ROM[MAX_PRGROM_SIZE >> 1], PRG_ROM[0x000], numSides << 16);

	PRGMask = ((RI.FDS_NumSides << 4) - 1) & MAX_PRGROM_MASK;

	if (!MapperInterface::LoadMapper(&RI))
		return _T("Famicom Disk System support not found!");

	EI.DbgOut(_T("FDS file loaded: %s - %s"), MI->Description, CompatLevel[MI->Compatibility]);
	EI.DbgOut(_T("Data length: %i disk side(s)"), RI.FDS_NumSides);
	SRAM_Size = 1;	// special, so FDS always saves changes
	return NULL;
}

const TCHAR *	OpenFileNSF (FILE *in)
{
	unsigned char Header[128];	// Header Bytes
	int ROMlen;

	fseek(in, 0, SEEK_END);
	ROMlen = ftell(in) - 128;
	fseek(in, 0, SEEK_SET);
	fread(Header, 1, 128, in);
	if (memcmp(Header, "NESM\x1a", 5))
		return _T("NSF header signature not found!");
	if (Header[5] != 1)
		return _T("This NSF version is not supported!");

	RI.ROMType = ROM_NSF;
	RI.NSF_DataSize = ROMlen;
	RI.NSF_NumSongs = Header[0x06];
	RI.NSF_SoundChips = Header[0x7B];
	RI.NSF_NTSCPAL = Header[0x7A];
	RI.NSF_NTSCSpeed = Header[0x6E] | (Header[0x6F] << 8);
	RI.NSF_PALSpeed = Header[0x78] | (Header[0x79] << 8);
	memcpy(RI.NSF_InitBanks, &Header[0x70], 8);
	RI.NSF_InitSong = Header[0x07];
	RI.NSF_InitAddr = Header[0x0A] | (Header[0x0B] << 8);
	RI.NSF_PlayAddr = Header[0x0C] | (Header[0x0D] << 8);
	RI.NSF_Title = new char[32];
	RI.NSF_Artist = new char[32];
	RI.NSF_Copyright = new char[32];
	memcpy(RI.NSF_Title, &Header[0x0E], 32);
	RI.NSF_Title[31] = 0;
	memcpy(RI.NSF_Artist, &Header[0x2E], 32);
	RI.NSF_Artist[31] = 0;
	memcpy(RI.NSF_Copyright, &Header[0x4E], 32);
	RI.NSF_Copyright[31] = 0;

	if (ROMlen > MAX_PRGROM_SIZE * 0x1000)
		return _T("NSF is too large! Increase MAX_PRGROM_SIZE and recompile!");

	if (memcmp(RI.NSF_InitBanks, "\0\0\0\0\0\0\0\0", 8))
		fread(&PRG_ROM[0][0] + ((Header[0x8] | (Header[0x9] << 8)) & 0x0FFF), 1, ROMlen, in);
	else
	{
		memcpy(RI.NSF_InitBanks, "\x00\x01\x02\x03\x04\x05\x06\x07", 8);
		fread(&PRG_ROM[0][0] + ((Header[0x8] | (Header[0x9] << 8)) & 0x7FFF), 1, ROMlen, in);
	}

	// don't bother trying to figure out the maximum value
	PRGMask = MAX_PRGROM_MASK;

	if ((RI.NSF_NTSCSpeed == 16666) || (RI.NSF_NTSCSpeed == 16667))
	{
		EI.DbgOut(_T("Adjusting NSF playback speed for NTSC..."));
		RI.NSF_NTSCSpeed = 16639;
	}
	if (RI.NSF_PALSpeed == 20000)
	{
		EI.DbgOut(_T("Adjusting NSF playback speed for PAL..."));
		RI.NSF_PALSpeed = 19997;
	}

	if (!MapperInterface::LoadMapper(&RI))
		return _T("NSF support not found!");
	EI.DbgOut(_T("NSF loaded: %s - %s"), MI->Description, CompatLevel[MI->Compatibility]);
	EI.DbgOut(_T("Data length: %iKB"), RI.NSF_DataSize >> 10);
	return NULL;
}

void	SetCPUMode (int NewMode)
{
	if (NewMode == 0)
	{
		PPU::IsPAL = FALSE;
		CheckMenuRadioItem(hMenu, ID_PPU_MODE_NTSC, ID_PPU_MODE_PAL, ID_PPU_MODE_NTSC, MF_BYCOMMAND);
		PPU::SLEndFrame = 262;
		if (PPU::SLnum >= PPU::SLEndFrame - 1)	// if we switched from PAL, scanline number could be invalid
			PPU::SLnum = PPU::SLEndFrame - 2;
		GFX::WantFPS = 60;
		GFX::LoadPalette(GFX::PaletteNTSC);
		APU::SetFPS(60);
		EI.DbgOut(_T("Emulation switched to NTSC"));
	}
	else
	{
		PPU::IsPAL = TRUE;
		CheckMenuRadioItem(hMenu, ID_PPU_MODE_NTSC, ID_PPU_MODE_PAL, ID_PPU_MODE_PAL, MF_BYCOMMAND);
		PPU::SLEndFrame = 312;
		GFX::WantFPS = 50;
		GFX::LoadPalette(GFX::PalettePAL);
		APU::SetFPS(50);
		EI.DbgOut(_T("Emulation switched to PAL"));
	}
}

void	Reset (RESET_TYPE ResetType)
{
	int i;
	for (i = 0x0; i < 0x10; i++)
	{
		CPU::ReadHandler[i] = CPU::ReadPRG;
		CPU::WriteHandler[i] = CPU::WritePRG;
		CPU::Readable[i] = FALSE;
		CPU::Writable[i] = FALSE;
		CPU::PRGPointer[i] = NULL;
	}
	CPU::ReadHandler[0] = CPU::ReadRAM;	CPU::WriteHandler[0] = CPU::WriteRAM;
	CPU::ReadHandler[1] = CPU::ReadRAM;	CPU::WriteHandler[1] = CPU::WriteRAM;
	CPU::ReadHandler[2] = PPU::IntRead;	CPU::WriteHandler[2] = PPU::IntWrite;
	CPU::ReadHandler[3] = PPU::IntRead;	CPU::WriteHandler[3] = PPU::IntWrite;
	// special check for Vs. Unisystem ROMs with NES 2.0 headers to apply a special PPU
	if ((RI.ROMType == ROM_INES) && (RI.INES_Flags & 0x10) && (RI.INES_Version == 2))
	{
		int VsPPU = RI.INES2_VSDATA & 0x0F;
		// only 5 of the special PPUs need this behavior
		if ((VsPPU >= 0x8) && (VsPPU <= 0xC))
		{
			CPU::ReadHandler[2] = PPU::IntReadVs;	CPU::WriteHandler[2] = PPU::IntWriteVs;
			CPU::ReadHandler[3] = PPU::IntReadVs;	CPU::WriteHandler[3] = PPU::IntWriteVs;
			switch (VsPPU)
			{
			case 0x8:	PPU::VsSecurity = 0x1B;	break;
			case 0x9:	PPU::VsSecurity = 0x3D;	break;
			case 0xA:	PPU::VsSecurity = 0x1C;	break;
			case 0xB:	PPU::VsSecurity = 0x1B;	break;
			case 0xC:	PPU::VsSecurity = 0x00;	break;	// don't know what value the 2C05-05 uses
			}
		}
	}

	CPU::ReadHandler[4] = CPU::Read4k;	CPU::WriteHandler[4] = CPU::Write4k;
	if (!GameGenie)
		Genie::CodeStat = 0;
	for (i = 0x0; i < 0x8; i++)
	{
		PPU::ReadHandler[i] = PPU::BusRead;
		PPU::WriteHandler[i] = PPU::BusWriteCHR;
		PPU::CHRPointer[i] = PPU::OpenBus;
		PPU::Writable[i] = FALSE;
	}
	for (i = 0x8; i < 0x10; i++)
	{
		PPU::ReadHandler[i] = PPU::BusRead;
		PPU::WriteHandler[i] = PPU::BusWriteNT;
		PPU::CHRPointer[i] = PPU::OpenBus;
		PPU::Writable[i] = FALSE;
	}
	switch (ResetType)
	{
	case RESET_HARD:
		EI.DbgOut(_T("Performing hard reset..."));
		ZeroMemory((unsigned char *)PRG_RAM+SRAM_Size, sizeof(PRG_RAM)-SRAM_Size);
		ZeroMemory(CHR_RAM, sizeof(CHR_RAM));
		if (MI2)
		{
			MI = MI2;
			MI2 = NULL;
		}
		CPU::PowerOn();
		PPU::PowerOn();
		APU::PowerOn();
		if (GameGenie)
			Genie::Reset();
		else	if ((MI) && (MI->Reset))
			MI->Reset(RESET_HARD);
		break;
	case RESET_SOFT:
		EI.DbgOut(_T("Performing soft reset..."));
		if (MI2)
		{
			MI = MI2;
			MI2 = NULL;
		}
		if (GameGenie)
		{
			if (Genie::CodeStat & 1)
				Genie::Init();	// Set up the PRG read handlers BEFORE resetting the mapper
			else	Genie::Reset();	// map the Game Genie back in its place
		}
		if ((MI) && (MI->Reset))
			MI->Reset(RESET_SOFT);
		break;
	}
	APU::Reset();
	CPU::Reset();
	PPU::Reset();
	CPU::WantNMI = FALSE;
#ifdef	ENABLE_DEBUGGER
	if (Debugger::Enabled)
		Debugger::Update(DEBUG_MODE_CPU | DEBUG_MODE_PPU);
#endif	/* ENABLE_DEBUGGER */
	Scanline = FALSE;
	EI.DbgOut(_T("Reset complete."));
}

DWORD	WINAPI	Thread (void *param)
{
#ifdef	CPU_BENCHMARK
	// Run with cyctest.nes
	int i;
	LARGE_INTEGER ClockFreq;
	LARGE_INTEGER ClockVal1, ClockVal2;
	QueryPerformanceFrequency(&ClockFreq);
	QueryPerformanceCounter(&ClockVal1);
	for (i = 0; i < 454211; i++)
	{
		CPU::ExecOp();
		CPU::ExecOp();
		CPU::ExecOp();
		CPU::ExecOp();
		CPU::ExecOp();
		CPU::ExecOp();
		CPU::ExecOp();
		CPU::ExecOp();
		CPU::ExecOp();
		CPU::ExecOp();
	}
	QueryPerformanceCounter(&ClockVal2);

	EI.DbgOut(_T("10 seconds emulated in %lu milliseconds"), (unsigned long)((ClockVal2.QuadPart - ClockVal1.QuadPart) * 1000 / ClockFreq.QuadPart));
#else	/* !CPU_BENCHMARK */

	Controllers::Acquire();

	if ((!DoStop) && (SoundEnabled))
		APU::SoundON();	// don't turn on sound if we're only stepping 1 instruction

	if ((PPU::SLnum == 240) && (FrameStep))
	{	// if we save or load while paused, we want to end up here
		// so we don't end up advancing another frame
		GotStep = FALSE;
		Movie::ShowFrame();
		while (FrameStep && !GotStep && !DoStop)
			Sleep(1);
	}
	
	while (!DoStop)
	{
#ifdef	ENABLE_DEBUGGER
		if (Debugger::Enabled)
			Debugger::AddInst();
#endif	/* ENABLE_DEBUGGER */
		CPU::ExecOp();
#ifdef	ENABLE_DEBUGGER
		if (Debugger::Enabled)
			Debugger::Update(DEBUG_MODE_CPU);
#endif	/* ENABLE_DEBUGGER */
		if (Scanline)
		{
			Scanline = FALSE;
			if (PPU::SLnum == 240)
			{
#ifdef	ENABLE_DEBUGGER
				if (Debugger::Enabled)
					Debugger::Update(DEBUG_MODE_PPU);
#endif	/* ENABLE_DEBUGGER */
				if (FrameStep)	// if we pause during emulation
				{	// it'll get caught down here at scanline 240
					GotStep = FALSE;
					Movie::ShowFrame();
					while (FrameStep && !GotStep && !DoStop)
						Sleep(1);
				}
			}
			else if (PPU::SLnum == 241)
				Controllers::UpdateInput();
		}
	}

	APU::SoundOFF();
	Controllers::UnAcquire();
	Movie::ShowFrame();

#endif	/* CPU_BENCHMARK */
	UpdateTitlebar();
	Running = FALSE;
	return 0;
}
void	Start (BOOL step)
{
	DWORD ThreadID;
	if (Running)
		return;
	Running = TRUE;
#ifdef	ENABLE_DEBUGGER
	Debugger::Step = step;
#endif	/* ENABLE_DEBUGGER */
	DoStop = FALSE;
	CloseHandle(CreateThread(NULL, 0, Thread, NULL, 0, &ThreadID));
}
void	Stop (void)
{
	if (!Running)
		return;
	DoStop = TRUE;
	while (Running)
	{
		ProcessMessages();
		Sleep(1);
	}
}

void	MapperConfig (void)
{
	if ((MI) && (MI->Config))
		MI->Config(CFG_WINDOW, TRUE);
}

void	UpdateInterface (void)
{
	HWND hWnd = hMainWnd;
	int w = 256 * SizeMult;
	int h = 240 * SizeMult;

	RECT client, window, desktop;
	SetWindowPos(hWnd, hWnd, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
	GetClientRect(hWnd, &client);
	SetWindowPos(hWnd, hWnd, 0, 0, 2 * w - client.right, 2 * h - client.bottom, SWP_NOMOVE | SWP_NOZORDER);

	// try to make sure the window is completely visible on the desktop
	desktop.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
	desktop.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
	desktop.right = desktop.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
	desktop.bottom = desktop.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
	
	bool moved = false;
	GetWindowRect(hWnd, &window);
	// check right/bottom first, then left/top
	if (window.right > desktop.right)
	{
		window.left -= window.right - desktop.right;
		window.right = desktop.right;
		moved = true;
	}
	if (window.bottom > desktop.bottom)
	{
		window.top -= window.bottom - desktop.bottom;
		window.bottom = desktop.bottom;
		moved = true;
	}
	// that way, if the window is bigger than the desktop, the bottom/right will be cut off instead of the top/left
	if (window.left < desktop.left)
	{
		window.right += desktop.left - window.left;
		window.left = desktop.left;
		moved = true;
	}
	if (window.top < desktop.top)
	{
		window.bottom += desktop.top - window.top;
		window.top = desktop.top;
		moved = true;
	}
	// it's still technically possible for the window to get lost if the desktop is not rectangular (e.g. one monitor is rotated)
	// but this should take care of most of the complaints about the window getting lost
	if (moved)
		SetWindowPos(hWnd, hWnd, window.left, window.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void	LoadSettings (void)
{
	HKEY SettingsBase;
	unsigned long Size;
	int Port1T = 0, Port2T = 0, FSPort1T = 0, FSPort2T = 0, FSPort3T = 0, FSPort4T = 0, ExpPortT = 0;
	int PosX, PosY;

	// set default window position to just right of the debug status window
	RECT dbg;
	GetWindowRect(hDebug, &dbg);
	PosX = dbg.right;
	PosY = 0;

	// Load Defaults
	SizeMult = 2;
	SoundEnabled = TRUE;
	dbgVisible = TRUE;
	GFX::aFSkip = 1;
	GFX::FSkip = 0;
	GFX::PaletteNTSC = GFX::PALETTE_NTSC;
	GFX::PalettePAL = GFX::PALETTE_PAL;
	GFX::NTSChue = 0;
	GFX::NTSCsat = 50;
	GFX::PALsat = 50;
	GFX::CustPaletteNTSC[0] = GFX::CustPalettePAL[0] = 0;

	GFX::SlowDown = FALSE;
	GFX::SlowRate = 2;
	CheckMenuRadioItem(hMenu, ID_PPU_SLOWDOWN_2, ID_PPU_SLOWDOWN_20, ID_PPU_SLOWDOWN_2, MF_BYCOMMAND);

	FrameStep = FALSE;
	Path_ROM[0] = Path_NMV[0] = Path_AVI[0] = Path_PAL[0] = 0;

	// End Defaults

	RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Nintendulator\\"), 0, KEY_ALL_ACCESS, &SettingsBase);
	Size = sizeof(BOOL);	RegQueryValueEx(SettingsBase, _T("SoundEnabled"), 0, NULL, (LPBYTE)&SoundEnabled                , &Size);
	Size = sizeof(BOOL);	RegQueryValueEx(SettingsBase, _T("aFSkip")      , 0, NULL, (LPBYTE)&GFX::aFSkip                 , &Size);
	Size = sizeof(BOOL);	RegQueryValueEx(SettingsBase, _T("PPUMode")     , 0, NULL, (LPBYTE)&PPU::IsPAL                  , &Size);
	Size = sizeof(BOOL);	RegQueryValueEx(SettingsBase, _T("AutoRun")     , 0, NULL, (LPBYTE)&AutoRun                     , &Size);
	Size = sizeof(BOOL);	RegQueryValueEx(SettingsBase, _T("Scanlines")   , 0, NULL, (LPBYTE)&GFX::Scanlines              , &Size);
	Size = sizeof(BOOL);	RegQueryValueEx(SettingsBase, _T("UDLR")        , 0, NULL, (LPBYTE)&Controllers::EnableOpposites, &Size);
	Size = sizeof(BOOL);	RegQueryValueEx(SettingsBase, _T("DebugWindow") , 0, NULL, (LPBYTE)&dbgVisible                  , &Size);

	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("SizeMult")   , 0, NULL, (LPBYTE)&SizeMult        , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSkip")      , 0, NULL, (LPBYTE)&GFX::FSkip      , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("PosX")       , 0, NULL, (LPBYTE)&PosX            , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("PosY")       , 0, NULL, (LPBYTE)&PosY            , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("PaletteNTSC"), 0, NULL, (LPBYTE)&GFX::PaletteNTSC, &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("PalettePAL") , 0, NULL, (LPBYTE)&GFX::PalettePAL , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("NTSChue")    , 0, NULL, (LPBYTE)&GFX::NTSChue    , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("NTSCsat")    , 0, NULL, (LPBYTE)&GFX::NTSCsat    , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("PALsat")     , 0, NULL, (LPBYTE)&GFX::PALsat     , &Size);

	Size = MAX_PATH * sizeof(TCHAR);	RegQueryValueEx(SettingsBase, _T("CustPaletteNTSC"), 0,NULL, (LPBYTE)&GFX::CustPaletteNTSC, &Size);
	Size = MAX_PATH * sizeof(TCHAR);	RegQueryValueEx(SettingsBase, _T("CustPalettePAL") , 0,NULL, (LPBYTE)&GFX::CustPalettePAL , &Size);
	Size = MAX_PATH * sizeof(TCHAR);	RegQueryValueEx(SettingsBase, _T("Path_ROM"), 0, NULL, (LPBYTE)&Path_ROM, &Size);
	Size = MAX_PATH * sizeof(TCHAR);	RegQueryValueEx(SettingsBase, _T("Path_NMV"), 0, NULL, (LPBYTE)&Path_NMV, &Size);
	Size = MAX_PATH * sizeof(TCHAR);	RegQueryValueEx(SettingsBase, _T("Path_AVI"), 0, NULL, (LPBYTE)&Path_AVI, &Size);
	Size = MAX_PATH * sizeof(TCHAR);	RegQueryValueEx(SettingsBase, _T("Path_PAL"), 0, NULL, (LPBYTE)&Path_PAL, &Size);

	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("Port1T")  , 0, NULL, (LPBYTE)&Port1T  , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("Port2T")  , 0, NULL, (LPBYTE)&Port2T  , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSPort1T"), 0, NULL, (LPBYTE)&FSPort1T, &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSPort2T"), 0, NULL, (LPBYTE)&FSPort2T, &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSPort3T"), 0, NULL, (LPBYTE)&FSPort3T, &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("FSPort4T"), 0, NULL, (LPBYTE)&FSPort4T, &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("ExpPortT"), 0, NULL, (LPBYTE)&ExpPortT, &Size);

	if (Port1T == Controllers::STD_FOURSCORE)
	{
		SET_STDCONT(Controllers::Port1, Controllers::STD_FOURSCORE);
		SET_STDCONT(Controllers::Port1, Controllers::STD_FOURSCORE2);
	}
	else
	{
		SET_STDCONT(Controllers::Port1, (Controllers::STDCONT_TYPE)Port1T);
		SET_STDCONT(Controllers::Port2, (Controllers::STDCONT_TYPE)Port2T);
	}
	SET_STDCONT(Controllers::FSPort1, (Controllers::STDCONT_TYPE)FSPort1T);
	SET_STDCONT(Controllers::FSPort2, (Controllers::STDCONT_TYPE)FSPort2T);
	SET_STDCONT(Controllers::FSPort3, (Controllers::STDCONT_TYPE)FSPort3T);
	SET_STDCONT(Controllers::FSPort4, (Controllers::STDCONT_TYPE)FSPort4T);
	SET_EXPCONT(Controllers::PortExp, (Controllers::EXPCONT_TYPE)ExpPortT);

	Size = sizeof(Controllers::Port1_Buttons);	RegQueryValueEx(SettingsBase, _T("Port1D")  , 0, NULL, (LPBYTE)Controllers::Port1_Buttons  , &Size);
	Size = sizeof(Controllers::Port2_Buttons);	RegQueryValueEx(SettingsBase, _T("Port2D")  , 0, NULL, (LPBYTE)Controllers::Port2_Buttons  , &Size);
	Size = sizeof(Controllers::FSPort1_Buttons);	RegQueryValueEx(SettingsBase, _T("FSPort1D"), 0, NULL, (LPBYTE)Controllers::FSPort1_Buttons, &Size);
	Size = sizeof(Controllers::FSPort2_Buttons);	RegQueryValueEx(SettingsBase, _T("FSPort2D"), 0, NULL, (LPBYTE)Controllers::FSPort2_Buttons, &Size);
	Size = sizeof(Controllers::FSPort3_Buttons);	RegQueryValueEx(SettingsBase, _T("FSPort3D"), 0, NULL, (LPBYTE)Controllers::FSPort3_Buttons, &Size);
	Size = sizeof(Controllers::FSPort4_Buttons);	RegQueryValueEx(SettingsBase, _T("FSPort4D"), 0, NULL, (LPBYTE)Controllers::FSPort4_Buttons, &Size);
	Size = sizeof(Controllers::PortExp_Buttons);	RegQueryValueEx(SettingsBase, _T("ExpPortD"), 0, NULL, (LPBYTE)Controllers::PortExp_Buttons, &Size);
	Controllers::SetDeviceUsed();

	RegCloseKey(SettingsBase);

	if (SoundEnabled)
		CheckMenuItem(hMenu, ID_SOUND_ENABLED, MF_CHECKED);

	if (AutoRun)
		CheckMenuItem(hMenu, ID_FILE_AUTORUN, MF_CHECKED);

	if (GFX::NTSChue >= 300)	// Old versions of Nintendulator used hue settings between 300 and 360
		GFX::NTSChue -= 330;	// Current versions use values between -30 and +30, so convert them if necessary

	GFX::SetFrameskip(-1);

	switch (SizeMult)
	{
	case 1:	CheckMenuRadioItem(hMenu, ID_PPU_SIZE_1X, ID_PPU_SIZE_4X, ID_PPU_SIZE_1X, MF_BYCOMMAND);	break;
	default:SizeMult = 2;
	case 2:	CheckMenuRadioItem(hMenu, ID_PPU_SIZE_1X, ID_PPU_SIZE_4X, ID_PPU_SIZE_2X, MF_BYCOMMAND);	break;
	case 3:	CheckMenuRadioItem(hMenu, ID_PPU_SIZE_1X, ID_PPU_SIZE_4X, ID_PPU_SIZE_3X, MF_BYCOMMAND);	break;
	case 4:	CheckMenuRadioItem(hMenu, ID_PPU_SIZE_1X, ID_PPU_SIZE_4X, ID_PPU_SIZE_4X, MF_BYCOMMAND);	break;
	}

	if (GFX::Scanlines)
		CheckMenuItem(hMenu, ID_PPU_SCANLINES, MF_CHECKED);

	if (PPU::IsPAL)
		SetCPUMode(1);
	else	SetCPUMode(0);

	SetWindowPos(hMainWnd, HWND_TOP, PosX, PosY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	if (dbgVisible)
	{
		CheckMenuItem(hMenu, ID_DEBUG_STATWND, MF_CHECKED);
		ShowWindow(hDebug, SW_SHOW);
	}

	UpdateInterface();
}

void	SaveSettings (void)
{
	HKEY SettingsBase;
	RECT wRect;
	GetWindowRect(hMainWnd, &wRect);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Nintendulator\\"), 0, KEY_ALL_ACCESS, &SettingsBase))
		RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Nintendulator\\"), 0, _T("NintendulatorClass"), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &SettingsBase, NULL);
	RegSetValueEx(SettingsBase, _T("SoundEnabled"), 0, REG_DWORD, (LPBYTE)&SoundEnabled  , sizeof(BOOL));
	RegSetValueEx(SettingsBase, _T("SizeMult")    , 0, REG_DWORD, (LPBYTE)&SizeMult      , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSkip")       , 0, REG_DWORD, (LPBYTE)&GFX::FSkip    , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("aFSkip")      , 0, REG_DWORD, (LPBYTE)&GFX::aFSkip   , sizeof(BOOL));
	RegSetValueEx(SettingsBase, _T("PPUMode")     , 0, REG_DWORD, (LPBYTE)&PPU::IsPAL    , sizeof(BOOL));
	RegSetValueEx(SettingsBase, _T("AutoRun")     , 0, REG_DWORD, (LPBYTE)&AutoRun       , sizeof(BOOL));
	RegSetValueEx(SettingsBase, _T("PosX")        , 0, REG_DWORD, (LPBYTE)&wRect.left    , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("PosY")        , 0, REG_DWORD, (LPBYTE)&wRect.top     , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("Scanlines")   , 0, REG_DWORD, (LPBYTE)&GFX::Scanlines, sizeof(BOOL));
	RegSetValueEx(SettingsBase, _T("DebugWindow") , 0, REG_DWORD, (LPBYTE)&dbgVisible    , sizeof(BOOL));

	RegSetValueEx(SettingsBase, _T("PaletteNTSC") , 0, REG_DWORD, (LPBYTE)&GFX::PaletteNTSC, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("PalettePAL")  , 0, REG_DWORD, (LPBYTE)&GFX::PalettePAL , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("NTSChue")     , 0, REG_DWORD, (LPBYTE)&GFX::NTSChue    , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("NTSCsat")     , 0, REG_DWORD, (LPBYTE)&GFX::NTSCsat    , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("PALsat")      , 0, REG_DWORD, (LPBYTE)&GFX::PALsat     , sizeof(DWORD));

	RegSetValueEx(SettingsBase, _T("UDLR")        , 0, REG_DWORD, (LPBYTE)&Controllers::EnableOpposites, sizeof(BOOL));

	RegSetValueEx(SettingsBase, _T("CustPaletteNTSC"), 0, REG_SZ, (LPBYTE)GFX::CustPaletteNTSC, (DWORD)(sizeof(TCHAR) * _tcslen(GFX::CustPaletteNTSC)));
	RegSetValueEx(SettingsBase, _T("CustPalettePAL") , 0, REG_SZ, (LPBYTE)GFX::CustPalettePAL , (DWORD)(sizeof(TCHAR) * _tcslen(GFX::CustPalettePAL)));
	RegSetValueEx(SettingsBase, _T("Path_ROM")       , 0, REG_SZ, (LPBYTE)Path_ROM            , (DWORD)(sizeof(TCHAR) * _tcslen(Path_ROM)));
	RegSetValueEx(SettingsBase, _T("Path_NMV")       , 0, REG_SZ, (LPBYTE)Path_NMV            , (DWORD)(sizeof(TCHAR) * _tcslen(Path_NMV)));
	RegSetValueEx(SettingsBase, _T("Path_AVI")       , 0, REG_SZ, (LPBYTE)Path_AVI            , (DWORD)(sizeof(TCHAR) * _tcslen(Path_AVI)));
	RegSetValueEx(SettingsBase, _T("Path_PAL")       , 0, REG_SZ, (LPBYTE)Path_PAL            , (DWORD)(sizeof(TCHAR) * _tcslen(Path_PAL)));

	RegSetValueEx(SettingsBase, _T("Port1T")  , 0, REG_DWORD, (LPBYTE)&Controllers::Port1->Type  , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("Port2T")  , 0, REG_DWORD, (LPBYTE)&Controllers::Port2->Type  , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort1T"), 0, REG_DWORD, (LPBYTE)&Controllers::FSPort1->Type, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort2T"), 0, REG_DWORD, (LPBYTE)&Controllers::FSPort2->Type, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort3T"), 0, REG_DWORD, (LPBYTE)&Controllers::FSPort3->Type, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("FSPort4T"), 0, REG_DWORD, (LPBYTE)&Controllers::FSPort4->Type, sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("ExpPortT"), 0, REG_DWORD, (LPBYTE)&Controllers::PortExp->Type, sizeof(DWORD));

	RegSetValueEx(SettingsBase, _T("Port1D")  , 0, REG_BINARY, (LPBYTE)Controllers::Port1_Buttons  , sizeof(Controllers::Port1_Buttons));
	RegSetValueEx(SettingsBase, _T("Port2D")  , 0, REG_BINARY, (LPBYTE)Controllers::Port2_Buttons  , sizeof(Controllers::Port2_Buttons));
	RegSetValueEx(SettingsBase, _T("FSPort1D"), 0, REG_BINARY, (LPBYTE)Controllers::FSPort1_Buttons, sizeof(Controllers::FSPort1_Buttons));
	RegSetValueEx(SettingsBase, _T("FSPort2D"), 0, REG_BINARY, (LPBYTE)Controllers::FSPort2_Buttons, sizeof(Controllers::FSPort2_Buttons));
	RegSetValueEx(SettingsBase, _T("FSPort3D"), 0, REG_BINARY, (LPBYTE)Controllers::FSPort3_Buttons, sizeof(Controllers::FSPort3_Buttons));
	RegSetValueEx(SettingsBase, _T("FSPort4D"), 0, REG_BINARY, (LPBYTE)Controllers::FSPort4_Buttons, sizeof(Controllers::FSPort4_Buttons));
	RegSetValueEx(SettingsBase, _T("ExpPortD"), 0, REG_BINARY, (LPBYTE)Controllers::PortExp_Buttons, sizeof(Controllers::PortExp_Buttons));

	RegCloseKey(SettingsBase);
}

void	RelocateSaveData_Progdir (void)
{
	WIN32_FIND_DATA Data;
	HANDLE Handle;

	TCHAR filename[MAX_PATH];

	// check if the program's builtin Saves directory is present
	_tcscpy(filename, ProgPath);
	PathAppend(filename, _T("Saves"));
	if (GetFileAttributes(filename) == INVALID_FILE_ATTRIBUTES)
		return;

	// is it actually a directory?
	if (!(GetFileAttributes(filename) & FILE_ATTRIBUTE_DIRECTORY))
		return;

	// Relocate FDS disk changes
	_stprintf(filename, _T("%sSaves\\*.fsv"), ProgPath);
	Handle = FindFirstFile(filename, &Data);
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR oldfile[MAX_PATH], newfile[MAX_PATH];
			_stprintf(oldfile, _T("%sSaves\\%s"), ProgPath, Data.cFileName);
			_stprintf(newfile, _T("%s\\FDS\\%s"), DataPath, Data.cFileName);
			MoveFile(oldfile, newfile);
		}	while (FindNextFile(Handle,&Data));
		FindClose(Handle);
	}

	// Relocate SRAM data
	_stprintf(filename, _T("%sSaves\\*.sav"), ProgPath);
	Handle = FindFirstFile(filename, &Data);
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR oldfile[MAX_PATH], newfile[MAX_PATH];
			_stprintf(oldfile, _T("%sSaves\\%s"), ProgPath, Data.cFileName);
			_stprintf(newfile, _T("%s\\SRAM\\%s"), DataPath, Data.cFileName);
			MoveFile(oldfile, newfile);
		}	while (FindNextFile(Handle,&Data));
		FindClose(Handle);
	}

	// Relocate Savestates
	_stprintf(filename, _T("%sSaves\\*.ns?"), ProgPath);
	Handle = FindFirstFile(filename, &Data);
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR oldfile[MAX_PATH], newfile[MAX_PATH];
			_stprintf(oldfile, _T("%sSaves\\%s"), ProgPath, Data.cFileName);
			_stprintf(newfile, _T("%s\\States\\%s"), DataPath, Data.cFileName);
			MoveFile(oldfile, newfile);
		}	while (FindNextFile(Handle,&Data));
		FindClose(Handle);
	}

	// Relocate Debug dumps - Logfiles
	_stprintf(filename, _T("%sSaves\\*.debug"), ProgPath);
	Handle = FindFirstFile(filename, &Data);
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR oldfile[MAX_PATH], newfile[MAX_PATH];
			_stprintf(oldfile, _T("%sSaves\\%s"), ProgPath, Data.cFileName);
			_stprintf(newfile, _T("%s\\Dumps\\%s"), DataPath, Data.cFileName);
			MoveFile(oldfile, newfile);
		}	while (FindNextFile(Handle,&Data));
		FindClose(Handle);
	}

	// Relocate Debug dumps - CPU dumps
	_stprintf(filename, _T("%sSaves\\*.cpumem"), ProgPath);
	Handle = FindFirstFile(filename, &Data);
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR oldfile[MAX_PATH], newfile[MAX_PATH];
			_stprintf(oldfile, _T("%sSaves\\%s"), ProgPath, Data.cFileName);
			_stprintf(newfile, _T("%s\\Dumps\\%s"), DataPath, Data.cFileName);
			MoveFile(oldfile, newfile);
		}	while (FindNextFile(Handle,&Data));
		FindClose(Handle);
	}

	// Relocate Debug dumps - PPU dumps
	_stprintf(filename, _T("%sSaves\\*.ppumem"), ProgPath);
	Handle = FindFirstFile(filename, &Data);
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR oldfile[MAX_PATH], newfile[MAX_PATH];
			_stprintf(oldfile, _T("%sSaves\\%s"), ProgPath, Data.cFileName);
			_stprintf(newfile, _T("%s\\Dumps\\%s"), DataPath, Data.cFileName);
			MoveFile(oldfile, newfile);
		}	while (FindNextFile(Handle,&Data));
		FindClose(Handle);
	}

	// Finally, try to delete the old Saves directory entirely
	_tcscpy(filename, ProgPath);
	PathAppend(filename, _T("Saves"));
	if (RemoveDirectory(filename))
		EI.DbgOut(_T("Savestate directory successfully relocated"));
	else
	{
		TCHAR str[MAX_PATH * 2 + 256];
		_stprintf(str, _T("Nintendulator was unable to fully relocate its data files to \"%s\".\nPlease delete the folder \"%s\" after relocating its contents."), DataPath, filename);
		MessageBox(NULL, str, _T("Nintendulator"), MB_OK | MB_ICONERROR);
		BrowseFolder(filename);
	}
}

// Nintendulator 0.975 builds between October 29, 2010 and January 18, 2011 relocated files from Application Data to My Documents
// As it turns out, this was the wrong thing to do - Application Data was the correct location all along
// In order to properly handle users who used builds during that time period, we need to be able to move the stuff back to Application Data
void	RelocateSaveData_Mydocs (void)
{
	WIN32_FIND_DATA Data;
	HANDLE Handle;

	TCHAR oldPath[MAX_PATH];
	TCHAR filename[MAX_PATH];

	// look for our folder in My Documents, if it exists
	if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, oldPath)))
		return;
	PathAppend(oldPath, _T("Nintendulator"));

	// check if the Nintendulator folder exists in My Documents
	if (GetFileAttributes(oldPath) == INVALID_FILE_ATTRIBUTES)
		return;
	if (!(GetFileAttributes(oldPath) & FILE_ATTRIBUTE_DIRECTORY))
		return;

	// Relocate FDS disk changes
	_stprintf(filename, _T("%s\\FDS\\*"), oldPath);
	Handle = FindFirstFile(filename, &Data);
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR oldfile[MAX_PATH], newfile[MAX_PATH];
			_stprintf(oldfile, _T("%s\\FDS\\%s"), oldPath, Data.cFileName);
			_stprintf(newfile, _T("%s\\FDS\\%s"), DataPath, Data.cFileName);
			MoveFile(oldfile, newfile);
		}	while (FindNextFile(Handle,&Data));
		FindClose(Handle);
	}
	_tcscpy(filename, oldPath);
	PathAppend(filename, _T("FDS"));
	if ((GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES) && (GetFileAttributes(filename) & FILE_ATTRIBUTE_DIRECTORY))
		RemoveDirectory(filename);

	// Relocate SRAM data
	_stprintf(filename, _T("%s\\SRAM\\*"), oldPath);
	Handle = FindFirstFile(filename, &Data);
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR oldfile[MAX_PATH], newfile[MAX_PATH];
			_stprintf(oldfile, _T("%s\\SRAM\\%s"), oldPath, Data.cFileName);
			_stprintf(newfile, _T("%s\\SRAM\\%s"), DataPath, Data.cFileName);
			MoveFile(oldfile, newfile);
		}	while (FindNextFile(Handle,&Data));
		FindClose(Handle);
	}
	_tcscpy(filename, oldPath);
	PathAppend(filename, _T("SRAM"));
	if ((GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES) && (GetFileAttributes(filename) & FILE_ATTRIBUTE_DIRECTORY))
		RemoveDirectory(filename);

	// Relocate Savestates
	_stprintf(filename, _T("%s\\States\\*"), oldPath);
	Handle = FindFirstFile(filename, &Data);
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR oldfile[MAX_PATH], newfile[MAX_PATH];
			_stprintf(oldfile, _T("%s\\States\\%s"), oldPath, Data.cFileName);
			_stprintf(newfile, _T("%s\\States\\%s"), DataPath, Data.cFileName);
			MoveFile(oldfile, newfile);
		}	while (FindNextFile(Handle,&Data));
		FindClose(Handle);
	}
	_tcscpy(filename, oldPath);
	PathAppend(filename, _T("States"));
	if ((GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES) && (GetFileAttributes(filename) & FILE_ATTRIBUTE_DIRECTORY))
		RemoveDirectory(filename);

	// Relocate Debug dumps
	_stprintf(filename, _T("%s\\Dumps\\*"), oldPath);
	Handle = FindFirstFile(filename, &Data);
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR oldfile[MAX_PATH], newfile[MAX_PATH];
			_stprintf(oldfile, _T("%s\\Dumps\\%s"), oldPath, Data.cFileName);
			_stprintf(newfile, _T("%s\\Dumps\\%s"), DataPath, Data.cFileName);
			MoveFile(oldfile, newfile);
		}	while (FindNextFile(Handle,&Data));
		FindClose(Handle);
	}
	_tcscpy(filename, oldPath);
	PathAppend(filename, _T("Dumps"));
	if ((GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES) && (GetFileAttributes(filename) & FILE_ATTRIBUTE_DIRECTORY))
		RemoveDirectory(filename);

	// Finally, try to delete the old directory from My Documents
	if (RemoveDirectory(oldPath))
		EI.DbgOut(_T("Savestate directory successfully relocated"));
	else
	{
		TCHAR str[MAX_PATH * 2 + 256];
		_stprintf(str, _T("Nintendulator was unable to fully relocate its data files to \"%s\".\nPlease delete the folder \"%s\" after relocating its contents."), DataPath, oldPath);
		MessageBox(NULL, str, _T("Nintendulator"), MB_OK | MB_ICONERROR);
		BrowseFolder(oldPath);
	}
}

void	SetupDataPath (void)
{
	TCHAR path[MAX_PATH];

	// Create data subdirectories
	// Savestates
	_tcscpy(path, DataPath);
	PathAppend(path, _T("States"));
	if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory(path, NULL);

	// SRAM data
	_tcscpy(path, DataPath);
	PathAppend(path, _T("SRAM"));
	if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory(path, NULL);

	// FDS disk changes
	_tcscpy(path, DataPath);
	PathAppend(path, _T("FDS"));
	if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory(path, NULL);

	// Debug dumps
	_tcscpy(path, DataPath);
	PathAppend(path, _T("Dumps"));
	if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory(path, NULL);

	// check if we need to relocate save data from the program's "Saves" subfolder
	RelocateSaveData_Progdir();
	// and the same for My Documents, for people who used 0.975 builds between October 29, 2010 and January 18, 2011
	RelocateSaveData_Mydocs();
}
} // namespace NES