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

int PRGSizeROM, PRGSizeRAM, CHRSizeROM, CHRSizeRAM;
int PRGMaskROM, PRGMaskRAM, CHRMaskROM, CHRMaskRAM;

BOOL ROMLoaded;
BOOL DoStop, Running, Scanline;
BOOL GameGenie;
BOOL SoundEnabled;
BOOL AutoRun;
BOOL FrameStep, GotStep;
BOOL HasMenu;
Region CurRegion = REGION_NONE;

unsigned char PRG_ROM[MAX_PRGROM_SIZE][0x1000];
unsigned char PRG_RAM[MAX_PRGRAM_SIZE][0x1000];
unsigned char CHR_ROM[MAX_CHRROM_SIZE][0x400];
unsigned char CHR_RAM[MAX_CHRRAM_SIZE][0x400];

void	Init (void)
{
	SetupDataPath();
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

	GFX::Start();

	CloseFile();

	Running = FALSE;
	DoStop = FALSE;
	GameGenie = FALSE;
	ROMLoaded = FALSE;
	ZeroMemory(&RI, sizeof(RI));

	UpdateTitlebar();
}

void	Destroy (void)
{
	if (ROMLoaded)
		CloseFile();
	SaveSettings();
#ifdef	ENABLE_DEBUGGER
	Debugger::Destroy();
#endif	/* ENABLE_DEBUGGER */
	GFX::Destroy();
	APU::Destroy();
	Controllers::Destroy();
	MapperInterface::Destroy();

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

	// initialize bank masks based on returned bank sizes
	PRGMaskROM = getMask(PRGSizeROM - 1) & MAX_PRGROM_MASK;
	PRGMaskRAM = getMask(PRGSizeRAM - 1) & MAX_PRGRAM_MASK;
	CHRMaskROM = getMask(CHRSizeROM - 1) & MAX_CHRROM_MASK;
	CHRMaskRAM = getMask(CHRSizeRAM - 1) & MAX_CHRRAM_MASK;

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

	Reset(RESET_HARD);
	if ((AutoRun) || (RI.ROMType == ROM_NSF))
		Start(FALSE);
}

int	FDSSave (FILE *out)
{
	int clen = 0;
	int x, y, n = 0;
	unsigned long data;
	writeLong(n);
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

int	FDSLoad (FILE *in, int version_id)
{
	int clen = 0;
	int n;
	unsigned long data;
	readLong(n);
	while (n > 0)
	{
		readLong(data);
		PRG_ROM[(data >> 12) & (MAX_PRGROM_MASK >> 1)][data & 0xFFF] = (unsigned char)(data >> 24);
		n--;
	}
	return clen;
}

void	SaveSRAM (void)
{
	TCHAR Filename[MAX_PATH];
	if (!SRAM_Size)
		return;
	if (RI.ROMType == ROM_FDS)
	{
		_stprintf(Filename, _T("%s\\FDS\\%s.fsav"), DataPath, States::BaseFilename);
		FILE *FSAVFile = _tfopen(Filename, _T("wb"));
		if (!FSAVFile)
		{
			EI.DbgOut(_T("Failed to save disk changes!"));
			return;
		}
		States::SaveVersion(FSAVFile, STATES_CUR_VERSION);
		FDSSave(FSAVFile);
		EI.DbgOut(_T("Saved disk changes"));
		fclose(FSAVFile);

		// check if there's an old-format FSV present
		_stprintf(Filename, _T("%s\\FDS\\%s.fsv"), DataPath, States::BaseFilename);
		FSAVFile = _tfopen(Filename, _T("rb"));
		if (FSAVFile)
		{
			// if so, delete it - we're not going to use it
			fclose(FSAVFile);
			_tunlink(Filename);
		}
	}
	else
	{
		_stprintf(Filename, _T("%s\\SRAM\\%s.sav"), DataPath, States::BaseFilename);
		FILE *SRAMFile = _tfopen(Filename, _T("wb"));
		if (!SRAMFile)
		{
			EI.DbgOut(_T("Failed to save SRAM!"));
			return;
		}
		fwrite(PRG_RAM, 1, SRAM_Size, SRAMFile);
		EI.DbgOut(_T("Saved SRAM."));
		fclose(SRAMFile);
	}
}
void	LoadSRAM (void)
{
	TCHAR Filename[MAX_PATH];
	int len;
	if (!SRAM_Size)
		return;
	if (RI.ROMType == ROM_FDS)
	{
		_stprintf(Filename, _T("%s\\FDS\\%s.fsav"), DataPath, States::BaseFilename);
		FILE *FSAVFile = _tfopen(Filename, _T("rb"));
		int version_id;
		if (FSAVFile)
		{
			version_id = States::LoadVersion(FSAVFile);
			fseek(FSAVFile, 0, SEEK_END);
			len = ftell(FSAVFile) - 4;
			fseek(FSAVFile, 4, SEEK_SET);
		}
		else
		{
			_stprintf(Filename, _T("%s\\FDS\\%s.fsv"), DataPath, States::BaseFilename);
			FSAVFile = _tfopen(Filename, _T("rb"));
			if (!FSAVFile)
				return;
			version_id = 975;
			fseek(FSAVFile, 0, SEEK_END);
			len = ftell(FSAVFile);
			fseek(FSAVFile, 0, SEEK_SET);
		}
		if (len == FDSLoad(FSAVFile, version_id))
			EI.DbgOut(_T("Loaded disk changes"));
		else	EI.DbgOut(_T("File length mismatch while loading disk data!"));
		fclose(FSAVFile);
	}
	else
	{
		_stprintf(Filename, _T("%s\\SRAM\\%s.sav"), DataPath, States::BaseFilename);
		FILE *SRAMFile = _tfopen(Filename, _T("rb"));
		if (!SRAMFile)
			return;
		fseek(SRAMFile, 0, SEEK_END);
		len = ftell(SRAMFile);
		fseek(SRAMFile, 0, SEEK_SET);
		ZeroMemory(PRG_RAM, SRAM_Size);
		fread(PRG_RAM, 1, SRAM_Size, SRAMFile);
		if (len == SRAM_Size)
			EI.DbgOut(_T("Loaded SRAM (%i bytes)."), SRAM_Size);
		else	EI.DbgOut(_T("File length mismatch while loading SRAM!"));
		fclose(SRAMFile);
	}
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

// Generates a bit mask sufficient to fit the specified value
DWORD	getMask (unsigned int maxval)
{
	DWORD result = 0;
	while (maxval > 0)
	{
		result = (result << 1) | 1;
		maxval >>= 1;
	}
	return result;
}

const TCHAR *	OpenFileiNES (FILE *in)
{
	int i;
	unsigned char Header[16];

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

		if (((RI.INES2_PRGRAM & 0xF) == 0xF) || ((RI.INES2_PRGRAM & 0xF0) == 0xF0))
			return _T("Invalid PRG RAM size specified!");
		if (((RI.INES2_CHRRAM & 0xF) == 0xF) || ((RI.INES2_CHRRAM & 0xF0) == 0xF0))
			return _T("Invalid CHR RAM size specified!");
		if (RI.INES2_CHRRAM & 0xF0)
			EI.DbgOut(_T("This ROM uses battery-backed CHR RAM, which is not yet supported!"));
		if (Header[14])
			return _T("Unrecognized data found at header offset 14 - this ROM may make use of features not supported by this emulator!");
		if (Header[15])
			return _T("Unrecognized data found at header offset 15 - this ROM may make use of features not supported by this emulator!");
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

	PRGSizeROM = RI.INES_PRGSize * 0x4;
	CHRSizeROM = RI.INES_CHRSize * 0x8;

	if (PRGSizeROM > MAX_PRGROM_SIZE)
		return _T("PRG ROM is too large! Increase MAX_PRGROM_SIZE and recompile!");

	if (CHRSizeROM > MAX_CHRROM_SIZE)
		return _T("CHR ROM is too large! Increase MAX_CHRROM_SIZE and recompile!");

	fread(PRG_ROM, 1, RI.INES_PRGSize * 0x4000, in);
	fread(CHR_ROM, 1, RI.INES_CHRSize * 0x2000, in);

	if (RI.INES_Version == 2)
	{
		PRGSizeRAM = 0;
		if (RI.INES2_PRGRAM & 0xF)
			PRGSizeRAM += 64 << (RI.INES2_PRGRAM & 0xF);
		if (RI.INES2_PRGRAM & 0xF0)
			PRGSizeRAM += 64 << ((RI.INES2_PRGRAM & 0xF0) >> 4);
		if (PRGSizeRAM > MAX_PRGRAM_SIZE * 0x1000)
			return _T("PRG RAM is too large! Increase MAX_PRGRAM_SIZE and recompile!");
		PRGSizeRAM = (PRGSizeRAM / 0x1000) + ((PRGSizeRAM % 0x1000) ? 1 : 0);

		CHRSizeRAM = 0;
		if (RI.INES2_CHRRAM & 0xF)
			CHRSizeRAM += 64 << (RI.INES2_CHRRAM & 0xF);
		if (RI.INES2_CHRRAM & 0xF0)
			CHRSizeRAM += 64 << ((RI.INES2_CHRRAM & 0xF0) >> 4);
		if (CHRSizeRAM > MAX_CHRRAM_SIZE * 0x400)
			return _T("CHR RAM is too large! Increase MAX_CHRRAM_SIZE and recompile!");
		CHRSizeRAM = (CHRSizeRAM / 0x400) + ((CHRSizeRAM % 0x400) ? 1 : 0);
		if ((CHRSizeROM == 0) && (CHRSizeRAM == 0))
			return _T("Selected ROM has no CHR ROM or CHR RAM!");
	}
	else
	{
		PRGSizeRAM = 0x10;	// default to 64KB (for MMC5)
		CHRSizeRAM = 0x20;	// default to 32KB (for Namcot 106)
	}

	if (!MapperInterface::LoadMapper(&RI))
	{
		static TCHAR err[256];
		_stprintf(err, _T("Mapper %i not supported!"), RI.INES_MapperNum);
		return err;
	}
	EI.DbgOut(_T("iNES ROM image loaded: mapper %i (%s) - %s"), RI.INES_MapperNum, MI->Description, MapperInterface::CompatLevel[MI->Compatibility]);
	EI.DbgOut(_T("PRG: %iKB; CHR: %iKB"), RI.INES_PRGSize << 4, RI.INES_CHRSize << 3);
	EI.DbgOut(_T("Flags: %s%s"), RI.INES_Flags & 0x02 ? _T("Battery-backed SRAM, ") : _T(""), RI.INES_Flags & 0x08 ? _T("Four-screen VRAM") : (RI.INES_Flags & 0x01 ? _T("Vertical mirroring") : _T("Horizontal mirroring")));

	// Special checks for Playchoice-10 and Vs. Unisystem ROMs to auto-select palette
	if ((RI.INES_Flags & 0x10) && (RI.INES_Version == 2))
	{
		GFX::PALETTE pals[16] = {
			GFX::PALETTE_PC10, // RP2C03B
			GFX::PALETTE_PC10, // RP2C03G - no dump available, assuming identical to RP2C03B
			GFX::PALETTE_VS1, // RP2C04-0001
			GFX::PALETTE_VS2, // RP2C04-0002
			GFX::PALETTE_VS3, // RP2C04-0003
			GFX::PALETTE_VS4, // RP2C04-0004
			GFX::PALETTE_PC10_ALT, // RC2C03B
			GFX::PALETTE_PC10, // RC2C03C
			GFX::PALETTE_PC10, // RC2C05C-01 - no dump available, assuming identical to RC2C05C-03
			GFX::PALETTE_PC10, // RC2C05C-02 - no dump available, assuming identical to RC2C05C-03
			GFX::PALETTE_PC10, // RC2C05C-03
			GFX::PALETTE_PC10, // RC2C05C-04 - no dump available, assuming identical to RC2C05C-03
			GFX::PALETTE_PC10, // RC2C05C-05 - no dump available, assuming identical to RC2C05C-03
			GFX::PALETTE_NTSC, // unused
			GFX::PALETTE_NTSC, // unused
			GFX::PALETTE_NTSC // unused
		};
		GFX::LoadPalette(pals[RI.INES2_VSDATA & 0x0F]);
	}
	else if (RI.INES_Flags & 0x20)
		GFX::LoadPalette(GFX::PALETTE_PC10);
	// Need to do this, in case the last loaded ROM was one of the above
	else	GFX::LoadPalette(PPU::IsPAL ? GFX::PalettePAL : GFX::PaletteNTSC);

	if ((RI.INES_Version == 2) && !(RI.INES2_TVMode & 0x02))
		SetRegion((RI.INES2_TVMode & 0x01) ? REGION_PAL : REGION_NTSC);
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

	PRGSizeROM = (PRGsize / 0x1000) + ((PRGsize % 0x1000) ? 1 : 0);
	CHRSizeROM = (CHRsize / 0x400) + ((CHRsize % 0x400) ? 1 : 0);
	// allow unlimited RAM sizes for UNIF
	PRGSizeRAM = MAX_PRGRAM_SIZE;
	CHRSizeRAM = MAX_CHRRAM_SIZE;

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

	if (!MapperInterface::LoadMapper(&RI))
	{
		static TCHAR err[1024];
		_stprintf(err, _T("UNIF boardset \"%hs\" not supported!"), RI.UNIF_BoardName);
		return err;
	}

	if (RI.UNIF_NTSCPAL == 0)
		SetRegion(REGION_NTSC);
	if (RI.UNIF_NTSCPAL == 1)
		SetRegion(REGION_PAL);

	EI.DbgOut(_T("UNIF file loaded: %hs (%s) - %s"), RI.UNIF_BoardName, MI->Description, MapperInterface::CompatLevel[MI->Compatibility]);
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

	// pre-divide by 2, because the upper half of PRG ROM is used to store the backup copy for saving changes
	if (RI.FDS_NumSides > (MAX_PRGROM_SIZE >> 1) / 16)
		return _T("FDS image is too large! Increase MAX_PRGROM_SIZE and recompile!");

	for (i = 0; i < numSides; i++)
		fread(PRG_ROM[i << 4], 1, 65500, in);

	memcpy(PRG_ROM[MAX_PRGROM_SIZE >> 1], PRG_ROM[0x000], numSides << 16);

	PRGSizeROM = RI.FDS_NumSides * 16;
	CHRSizeROM = 0;
	PRGSizeRAM = 0x8; // FDS only has 32KB of PRG RAM
	CHRSizeRAM = 0x8; // and 8KB of CHR RAM

	if (!MapperInterface::LoadMapper(&RI))
		return _T("Famicom Disk System support not found!");

	EI.DbgOut(_T("FDS file loaded: %s - %s"), MI->Description, MapperInterface::CompatLevel[MI->Compatibility]);
	EI.DbgOut(_T("Data length: %i disk side(s)"), RI.FDS_NumSides);
	SRAM_Size = 1;	// special, so FDS always saves changes
	return NULL;
}

const TCHAR *	OpenFileNSF (FILE *in)
{
	unsigned char Header[128];	// Header Bytes
	int ROMlen;
	int LoadAddr;

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
	LoadAddr = Header[0x08] | (Header[0x09] << 8);
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
	{
		fread(&PRG_ROM[0][0] + (LoadAddr & 0x0FFF), 1, ROMlen, in);
		PRGSizeROM = ROMlen + (LoadAddr & 0xFFF);
	}
	else
	{
		memcpy(RI.NSF_InitBanks, "\x00\x01\x02\x03\x04\x05\x06\x07", 8);
		fread(&PRG_ROM[0][0] + (LoadAddr & 0x7FFF), 1, ROMlen, in);
		PRGSizeROM = ROMlen + (LoadAddr & 0x7FFF);
	}

	PRGSizeROM = (PRGSizeROM / 0x1000) + ((PRGSizeROM % 0x1000) ? 1 : 0);
	PRGSizeRAM = 0x2;	// 8KB at $6000-$7FFF
	// no CHR at all
	CHRSizeROM = 0;
	CHRSizeRAM = 0;

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
	EI.DbgOut(_T("NSF loaded: %s - %s"), MI->Description, MapperInterface::CompatLevel[MI->Compatibility]);
	EI.DbgOut(_T("Data length: %iKB"), RI.NSF_DataSize >> 10);
	return NULL;
}

void	SetRegion (Region NewRegion)
{
	if ((CurRegion == NewRegion) && (NewRegion != REGION_NONE))
		return;
	CurRegion = NewRegion;
	switch (CurRegion)
	{
	default:
		EI.DbgOut(_T("Invalid region selected!"));
	case REGION_NTSC:
		CheckMenuRadioItem(hMenu, ID_PPU_MODE_NTSC, ID_PPU_MODE_DENDY, ID_PPU_MODE_NTSC, MF_BYCOMMAND);
		PPU::SetRegion();
		APU::SetRegion();
		GFX::SetRegion();
		EI.DbgOut(_T("Emulation switched to NTSC"));
		break;
	case REGION_PAL:
		CheckMenuRadioItem(hMenu, ID_PPU_MODE_NTSC, ID_PPU_MODE_DENDY, ID_PPU_MODE_PAL, MF_BYCOMMAND);
		PPU::SetRegion();
		APU::SetRegion();
		GFX::SetRegion();
		EI.DbgOut(_T("Emulation switched to PAL"));
		break;
	case REGION_DENDY:
		CheckMenuRadioItem(hMenu, ID_PPU_MODE_NTSC, ID_PPU_MODE_DENDY, ID_PPU_MODE_DENDY, MF_BYCOMMAND);
		PPU::SetRegion();
		APU::SetRegion();
		GFX::SetRegion();
		EI.DbgOut(_T("Emulation switched to Hybrid"));
		break;
	}
}

void	InitHandlers (void)
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

	CPU::ReadHandler[4] = APU::IntRead;	CPU::WriteHandler[4] = APU::IntWrite;
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
}

void	Reset (RESET_TYPE ResetType)
{
	InitHandlers();
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
	Debugger::PalChanged = TRUE;
	Debugger::PatChanged = TRUE;
	Debugger::NTabChanged = TRUE;
	Debugger::SprChanged = TRUE;
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
	for (i = 0; i < 2725266; i++)
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

	EI.DbgOut(_T("1 minute emulated in %lu milliseconds"), (unsigned long)((ClockVal2.QuadPart - ClockVal1.QuadPart) * 1000 / ClockFreq.QuadPart));
#else	/* !CPU_BENCHMARK */

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
	Controllers::Acquire();
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
	Controllers::UnAcquire();
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

void	SaveSettings (void)
{
	HKEY SettingsBase;
	RECT wRect;
	GetWindowRect(hMainWnd, &wRect);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Nintendulator\\"), 0, KEY_ALL_ACCESS, &SettingsBase))
		RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Nintendulator\\"), 0, _T("NintendulatorClass"), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &SettingsBase, NULL);
	RegSetValueEx(SettingsBase, _T("SoundEnabled"), 0, REG_DWORD, (LPBYTE)&SoundEnabled    , sizeof(BOOL));
	RegSetValueEx(SettingsBase, _T("AutoRun")     , 0, REG_DWORD, (LPBYTE)&AutoRun         , sizeof(BOOL));
	RegSetValueEx(SettingsBase, _T("DebugWindow") , 0, REG_DWORD, (LPBYTE)&dbgVisible      , sizeof(BOOL));
	RegSetValueEx(SettingsBase, _T("BadOpcodes")  , 0, REG_DWORD, (LPBYTE)&CPU::LogBadOps  , sizeof(BOOL));

	RegSetValueEx(SettingsBase, _T("SizeMult")    , 0, REG_DWORD, (LPBYTE)&SizeMult        , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("PosX")        , 0, REG_DWORD, (LPBYTE)&wRect.left      , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("PosY")        , 0, REG_DWORD, (LPBYTE)&wRect.top       , sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("Region")      , 0, REG_DWORD, (LPBYTE)&CurRegion       , sizeof(DWORD));

	RegSetValueEx(SettingsBase, _T("Path_ROM"), 0, REG_SZ, (LPBYTE)Path_ROM, (DWORD)(sizeof(TCHAR) * _tcslen(Path_ROM)));
	RegSetValueEx(SettingsBase, _T("Path_NMV"), 0, REG_SZ, (LPBYTE)Path_NMV, (DWORD)(sizeof(TCHAR) * _tcslen(Path_NMV)));
	RegSetValueEx(SettingsBase, _T("Path_AVI"), 0, REG_SZ, (LPBYTE)Path_AVI, (DWORD)(sizeof(TCHAR) * _tcslen(Path_AVI)));
	RegSetValueEx(SettingsBase, _T("Path_PAL"), 0, REG_SZ, (LPBYTE)Path_PAL, (DWORD)(sizeof(TCHAR) * _tcslen(Path_PAL)));

	RegSetValueEx(SettingsBase, _T("ConfigVersion"), 0, REG_DWORD, (LPBYTE)&ConfigVersion, sizeof(DWORD));

	Controllers::SaveSettings(SettingsBase);
	GFX::SaveSettings(SettingsBase);

	RegCloseKey(SettingsBase);
}

void	RelocateSaveData_Mydocs (void);
void	RelocateSaveData_Progdir (void);

void	LoadSettings (void)
{
	HKEY SettingsBase;
	unsigned long Size;
	int PosX, PosY;
	Region MyRegion = REGION_NTSC;

	// set default window position to just right of the debug status window
	RECT dbg;
	GetWindowRect(hDebug, &dbg);
	PosX = dbg.right;
	PosY = 0;

	// Load Defaults
	SizeMult = 2;
	SoundEnabled = TRUE;
	dbgVisible = TRUE;
	CPU::LogBadOps = FALSE;

	FrameStep = FALSE;
	Path_ROM[0] = Path_NMV[0] = Path_AVI[0] = Path_PAL[0] = 0;

	// End Defaults

	RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Nintendulator\\"), 0, KEY_ALL_ACCESS, &SettingsBase);
	Size = sizeof(BOOL);	RegQueryValueEx(SettingsBase, _T("SoundEnabled"), 0, NULL, (LPBYTE)&SoundEnabled   , &Size);
	Size = sizeof(BOOL);	RegQueryValueEx(SettingsBase, _T("AutoRun")     , 0, NULL, (LPBYTE)&AutoRun        , &Size);
	Size = sizeof(BOOL);	RegQueryValueEx(SettingsBase, _T("DebugWindow") , 0, NULL, (LPBYTE)&dbgVisible     , &Size);
	Size = sizeof(BOOL);	RegQueryValueEx(SettingsBase, _T("BadOpcodes")  , 0, NULL, (LPBYTE)&CPU::LogBadOps , &Size);

	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("SizeMult")    , 0, NULL, (LPBYTE)&SizeMult       , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("PosX")        , 0, NULL, (LPBYTE)&PosX           , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("PosY")        , 0, NULL, (LPBYTE)&PosY           , &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("Region")      , 0, NULL, (LPBYTE)&MyRegion       , &Size);

	Size = MAX_PATH * sizeof(TCHAR);	RegQueryValueEx(SettingsBase, _T("Path_ROM"), 0, NULL, (LPBYTE)&Path_ROM, &Size);
	Size = MAX_PATH * sizeof(TCHAR);	RegQueryValueEx(SettingsBase, _T("Path_NMV"), 0, NULL, (LPBYTE)&Path_NMV, &Size);
	Size = MAX_PATH * sizeof(TCHAR);	RegQueryValueEx(SettingsBase, _T("Path_AVI"), 0, NULL, (LPBYTE)&Path_AVI, &Size);
	Size = MAX_PATH * sizeof(TCHAR);	RegQueryValueEx(SettingsBase, _T("Path_PAL"), 0, NULL, (LPBYTE)&Path_PAL, &Size);

	Controllers::LoadSettings(SettingsBase);
	GFX::LoadSettings(SettingsBase);

	ConfigVersion = 0;
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("ConfigVersion"), 0, NULL, (LPBYTE)&ConfigVersion, &Size);
	switch (ConfigVersion)
	{
	case 0:
		// Old versions used hue settings 300 to 360; current version uses -30 to +30
		if (GFX::NTSChue >= 300)
			GFX::NTSChue -= 330;

		// check if we need to relocate save data from the program's "Saves" subfolder
		RelocateSaveData_Progdir();
		// and the same for My Documents, for people who used 0.975 builds between October 29, 2010 and January 18, 2011
		RelocateSaveData_Mydocs();
		ConfigVersion++;
		break;
	}

	RegCloseKey(SettingsBase);

	if (SoundEnabled)
		CheckMenuItem(hMenu, ID_SOUND_ENABLED, MF_CHECKED);

	if (AutoRun)
		CheckMenuItem(hMenu, ID_FILE_AUTORUN, MF_CHECKED);

	if (CPU::LogBadOps)
		CheckMenuItem(hMenu, ID_CPU_BADOPS, MF_CHECKED);

	switch (SizeMult)
	{
	case 1:	CheckMenuRadioItem(hMenu, ID_PPU_SIZE_1X, ID_PPU_SIZE_4X, ID_PPU_SIZE_1X, MF_BYCOMMAND);	break;
	default:SizeMult = 2;
	case 2:	CheckMenuRadioItem(hMenu, ID_PPU_SIZE_1X, ID_PPU_SIZE_4X, ID_PPU_SIZE_2X, MF_BYCOMMAND);	break;
	case 3:	CheckMenuRadioItem(hMenu, ID_PPU_SIZE_1X, ID_PPU_SIZE_4X, ID_PPU_SIZE_3X, MF_BYCOMMAND);	break;
	case 4:	CheckMenuRadioItem(hMenu, ID_PPU_SIZE_1X, ID_PPU_SIZE_4X, ID_PPU_SIZE_4X, MF_BYCOMMAND);	break;
	}

	SetRegion(MyRegion);

	SetWindowPos(hMainWnd, HWND_TOP, PosX, PosY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	if (dbgVisible)
	{
		CheckMenuItem(hMenu, ID_DEBUG_STATWND, MF_CHECKED);
		ShowWindow(hDebug, SW_SHOW);
	}

	UpdateInterface();
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
}
} // namespace NES