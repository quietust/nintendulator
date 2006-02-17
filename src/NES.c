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
#include "CPU.h"
#include "PPU.h"
#include "APU.h"
#include "GFX.h"
#include "AVI.h"
#include "Debugger.h"
#include "States.h"
#include "Controllers.h"
#include "Genie.h"

struct tNES NES;
unsigned char PRG_ROM[0x800][0x1000];	/* 8192 KB */
unsigned char CHR_ROM[0x1000][0x400];	/* 4096 KB */
unsigned char PRG_RAM[0x10][0x1000];	/*   64 KB */
unsigned char CHR_RAM[0x20][0x400];	/*   32 KB */

static	char *CompatLevel[COMPAT_NONE] = {"Fully supported!","Mostly supported","Partially supported"};

void	NES_Init (void)
{
	SetWindowPos(mWnd,mWnd,0,0,256,240,SWP_NOZORDER);
	MapperInterface_Init();
	APU_Init();
	GFX_Init();
#ifdef ENABLE_DEBUGGER
	Debugger_Init();
#endif	/* ENABLE_DEBUGGER */
	States_Init();
	Controllers_Init();
#ifdef ENABLE_DEBUGGER
	Debugger_SetMode(0);
#endif	/* ENABLE_DEBUGGER */
	NES_LoadSettings();
	NES_CloseFile();

	NES.Stopped = TRUE;
	NES.Stop = FALSE;
	NES.NeedQuit = FALSE;
	NES.GameGenie = FALSE;
	NES.ROMLoaded = FALSE;
	memset(&RI,0,sizeof(RI));

	GFX_UpdateTitlebar();
}

void	NES_Release (void)
{
	if (NES.ROMLoaded)
		NES_CloseFile();
	APU_Release();
	if (APU.buffer)
		free(APU.buffer);
	GFX_Release();
	Controllers_Release();
	MapperInterface_Release();

	NES_SaveSettings();
	DestroyWindow(mWnd);
}

void	NES_OpenFile (char *filename)
{
	size_t len = strlen(filename);
	const char *LoadRet = NULL;
	if (NES.ROMLoaded)
		NES_CloseFile();

	EI.DbgOut("Loading file '%s'...",filename);
	if (!stricmp(filename + len - 4, ".NES"))
		LoadRet = NES_OpenFileiNES(filename);
	else if (!stricmp(filename + len - 4, ".NSF"))
		LoadRet = NES_OpenFileNSF(filename);
	else if (!stricmp(filename + len - 4, ".UNF"))
		LoadRet = NES_OpenFileUNIF(filename);
	else if (!stricmp(filename + len - 5, ".UNIF"))
		LoadRet = NES_OpenFileUNIF(filename);
	else if (!stricmp(filename + len - 4, ".FDS"))
		LoadRet = NES_OpenFileFDS(filename);
	else	LoadRet = "File type not recognized!";

	if (LoadRet)
	{
		MessageBox(mWnd,LoadRet,"Nintendulator",MB_OK | MB_ICONERROR);
		NES_CloseFile();
		return;
	}
	NES.ROMLoaded = TRUE;
	EI.DbgOut("Loaded successfully!");
	States_SetFilename(filename);

	if (MI->Load)
		MI->Load();

	NES.HasMenu = FALSE;
	if (MI->Config)
	{
		if (MI->Config(CFG_WINDOW,FALSE))
			NES.HasMenu = TRUE;
		EnableMenuItem(GetMenu(mWnd),ID_GAME,MF_ENABLED);
	}
	else	EnableMenuItem(GetMenu(mWnd),ID_GAME,MF_GRAYED);
	NES_LoadSRAM();

	if (RI.ROMType == ROM_NSF)
	{
		NES.GameGenie = FALSE;
		CheckMenuItem(GetMenu(mWnd),ID_CPU_GAMEGENIE,MF_UNCHECKED);
		EnableMenuItem(GetMenu(mWnd),ID_CPU_GAMEGENIE,MF_GRAYED);
	}
	else
	{
		EnableMenuItem(GetMenu(mWnd),ID_CPU_SAVESTATE,MF_ENABLED);
		EnableMenuItem(GetMenu(mWnd),ID_CPU_LOADSTATE,MF_ENABLED);
		EnableMenuItem(GetMenu(mWnd),ID_CPU_PREVSTATE,MF_ENABLED);
		EnableMenuItem(GetMenu(mWnd),ID_CPU_NEXTSTATE,MF_ENABLED);

		EnableMenuItem(GetMenu(mWnd),ID_MISC_PLAYMOVIE,MF_ENABLED);
		EnableMenuItem(GetMenu(mWnd),ID_MISC_RESUMEMOVIE,MF_ENABLED);
		EnableMenuItem(GetMenu(mWnd),ID_MISC_RECORDMOVIE,MF_ENABLED);
		EnableMenuItem(GetMenu(mWnd),ID_MISC_RECORDSTATE,MF_ENABLED);

		EnableMenuItem(GetMenu(mWnd),ID_MISC_STARTAVICAPTURE,MF_ENABLED);
	}

	EnableMenuItem(GetMenu(mWnd),ID_FILE_CLOSE,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_RUN,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_STEP,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_STOP,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_SOFTRESET,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_HARDRESET,MF_ENABLED);

	DrawMenuBar(mWnd);

#ifdef ENABLE_DEBUGGER
	Debugger.NTabChanged = TRUE;
	Debugger.PalChanged = TRUE;
	Debugger.PatChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */

	NES_Reset(RESET_FULL);
	if ((NES.AutoRun) || (RI.ROMType == ROM_NSF))
	{
		if (NES.Stopped)
		{
			NES.Stop = FALSE;
			NES_Run();
		}
		else
		{
			NES.Stop = TRUE;
			NES.NeedReset = RESET_SOFT;
		}
	}
}

int	NES_FDSSave (FILE *out)
{
	int clen = 0;
	int x, y, n = 0;
	unsigned long data = 0;
	fwrite(&data,4,1,out); clen += 4;
	for (x = 0; x < RI.FDS_NumSides << 4; x++)
	{
		for (y = 0; y < 4096; y++)
		{
			if (PRG_ROM[x][y] != PRG_ROM[0x400 | x][y])
			{
				data = y | (x << 12) | (PRG_ROM[x][y] << 24);
				fwrite(&data,4,1,out);	clen += 4;
				n++;
			}
		}
	}
	fseek(out,-clen,SEEK_CUR);
	fwrite(&n,4,1,out);
	fseek(out,clen-4,SEEK_CUR);
	return clen;
}

int	NES_FDSLoad (FILE *in)
{
	int clen = 0;
	int n;
	unsigned long data;
	fread(&n,4,1,in);
	for (; n >= 0; n--)
	{
		fread(&data,4,1,in);
		if (feof(in))
			break;
		PRG_ROM[(data >> 12) & 0x3FF][data & 0xFFF] = (unsigned char)(data >> 24);
		clen += 4;
	}
	return clen;
}

void	NES_SaveSRAM (void)
{
	char Filename[MAX_PATH];
	FILE *SRAMFile;
	if (!NES.SRAM_Size)
		return;
	if (RI.ROMType == ROM_FDS)
		sprintf(Filename,"%s.fsv",States.BaseFilename);
	else	sprintf(Filename,"%s.sav",States.BaseFilename);
	SRAMFile = fopen(Filename,"wb");
	if (RI.ROMType == ROM_FDS)
	{
		NES_FDSSave(SRAMFile);
		EI.DbgOut("Saved disk changes");
	}
	else
	{
		fwrite(PRG_RAM,1,NES.SRAM_Size,SRAMFile);
		EI.DbgOut("Saved SRAM.");
	}
	fclose(SRAMFile);
}
void	NES_LoadSRAM (void)
{
	char Filename[MAX_PATH];
	FILE *SRAMFile;
	int len;
	if (!NES.SRAM_Size)
		return;
	if (RI.ROMType == ROM_FDS)
		sprintf(Filename,"%s.fsv",States.BaseFilename);
	else	sprintf(Filename,"%s.sav",States.BaseFilename);
	SRAMFile = fopen(Filename,"rb");
	if (!SRAMFile)
		return;
	fseek(SRAMFile,0,SEEK_END);
	len = ftell(SRAMFile);
	fseek(SRAMFile,0,SEEK_SET);
	if (RI.ROMType == ROM_FDS)
	{
		if (len == NES_FDSLoad(SRAMFile))
			EI.DbgOut("Loaded disk changes");
		else	EI.DbgOut("File length mismatch while loading disk data!");
	}
	else
	{
		fread(PRG_RAM,1,NES.SRAM_Size,SRAMFile);
		if (len == NES.SRAM_Size)
			EI.DbgOut("Loaded SRAM.");
		else	EI.DbgOut("File length mismatch while loading SRAM!");
	}
	fclose(SRAMFile);
}

void	NES_CloseFile (void)
{
	int i;
	NES_SaveSRAM();
	if (NES.ROMLoaded)
	{
		MapperInterface_UnloadMapper();
		NES.ROMLoaded = FALSE;
		EI.DbgOut("ROM unloaded.");
	}

	if (RI.ROMType)
	{
		if (RI.ROMType == ROM_UNIF)
			free(RI.UNIF_BoardName);
		else if (RI.ROMType == ROM_NSF)
		{
			free(RI.NSF_Title);
			free(RI.NSF_Artist);
			free(RI.NSF_Copyright);
		}
		free(RI.Filename);
		memset(&RI,0,sizeof(RI));
	}

	if (aviout)
		AVI_End();
	if (Controllers.MovieMode)
		Controllers_StopMovie();

	EnableMenuItem(GetMenu(mWnd),ID_GAME,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_GAMEGENIE,MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_SAVESTATE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_LOADSTATE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_PREVSTATE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_NEXTSTATE,MF_GRAYED);

	EnableMenuItem(GetMenu(mWnd),ID_FILE_CLOSE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_RUN,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_STEP,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_STOP,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_SOFTRESET,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_CPU_HARDRESET,MF_GRAYED);

	EnableMenuItem(GetMenu(mWnd),ID_MISC_PLAYMOVIE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_RESUMEMOVIE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_RECORDMOVIE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_RECORDSTATE,MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_STARTAVICAPTURE,MF_GRAYED);

	NES.SRAM_Size = 0;
	for (i = 0; i < 16; i++)
	{
		CPU.PRGPointer[i] = PRG_RAM[0];
		CPU.ReadHandler[i] = CPU_ReadPRG;
		CPU.WriteHandler[i] = CPU_WritePRG;
		CPU.Readable[i] = FALSE;
		CPU.Writable[i] = FALSE;
		PPU.CHRPointer[i] = CHR_RAM[0];
		PPU.ReadHandler[i] = PPU_BusRead;
		PPU.WriteHandler[i] = PPU_BusWriteCHR;
		PPU.Writable[i] = FALSE;
	}
}

#define MKID(a) ((unsigned long) \
		(((a) >> 24) & 0x000000FF) | \
		(((a) >>  8) & 0x0000FF00) | \
		(((a) <<  8) & 0x00FF0000) | \
		(((a) << 24) & 0xFF000000))

const char *	NES_OpenFileiNES (char *filename)
{
	FILE *in;
	unsigned char Header[4];
	unsigned long tmp;
	BOOL p2Found;

	in = fopen(filename,"rb");
	fread(&tmp,4,1,in);
	if (tmp != MKID('NES\x1A'))
	{
		fclose(in);
		return "iNES header signature not found!";
	}
	RI.Filename = strdup(filename);
	RI.ROMType = ROM_INES;
	fread(Header,1,4,in);
	for (tmp = 8; tmp < 0x10; tmp++)
	{
		unsigned char x;
		fread(&x,1,1,in);
		if (x)
		{
			fclose(in);
			return "Invalid data found in header!";
		}
	}
	
	RI.INES_PRGSize = Header[0];
	RI.INES_CHRSize = Header[1];
	RI.INES_MapperNum = ((Header[2] & 0xF0) >> 4) | (Header[3] & 0xF0);
	RI.INES_Flags = (Header[2] & 0xF) | ((Header[3] & 0xF) << 4);

	if (RI.INES_Flags & 0x04)
	{
		fclose(in);
		return "Trained ROMs are unsupported!";
	}

	fread(PRG_ROM,1,RI.INES_PRGSize * 0x4000,in);
	fread(CHR_ROM,1,RI.INES_CHRSize * 0x2000,in);

	fclose(in);

	NES.PRGMask = ((RI.INES_PRGSize << 2) - 1) & MAX_PRGROM_MASK;
	NES.CHRMask = ((RI.INES_CHRSize << 3) - 1) & MAX_CHRROM_MASK;

	p2Found = FALSE;
	for (tmp = 1; tmp < 0x40000000; tmp <<= 1)
		if (tmp == RI.INES_PRGSize)
			p2Found = TRUE;
	if (!p2Found)
		NES.PRGMask = MAX_PRGROM_MASK;

	p2Found = FALSE;
	for (tmp = 1; tmp < 0x40000000; tmp <<= 1)
		if (tmp == RI.INES_CHRSize)
			p2Found = TRUE;
	if (!p2Found)
		NES.CHRMask = MAX_CHRROM_MASK;
	
	if (!MapperInterface_LoadMapper(&RI))
	{
		static char err[256];
		sprintf(err,"Mapper %i not supported!",RI.INES_MapperNum);
		return err;
	}
	EI.DbgOut("iNES ROM image loaded: mapper %i (%s) - %s",RI.INES_MapperNum,MI->Description,CompatLevel[MI->Compatibility]);
	EI.DbgOut("PRG: %iKB; CHR: %iKB",RI.INES_PRGSize << 4,RI.INES_CHRSize << 3);
	EI.DbgOut("Flags: %s%s",RI.INES_Flags & 0x02 ? "Battery-backed SRAM, " : "", RI.INES_Flags & 0x08 ? "Four-screen VRAM" : (RI.INES_Flags & 0x01 ? "Vertical mirroring" : "Horizontal mirroring"));
	return NULL;
}

const char *	NES_OpenFileUNIF (char *filename)
{
	unsigned long Signature, BlockLen;
	unsigned char *tPRG[0x10], *tCHR[0x10];
	unsigned char *PRGPoint, *CHRPoint;
	unsigned long PRGsize, CHRsize;
	int i;
	unsigned char tp;
	BOOL p2Found;
	DWORD p2;

	FILE *in = fopen(filename,"rb");
	if (!in)
		return FALSE;
	fread(&Signature,4,1,in);
	if (Signature != MKID('UNIF'))
	{
		fclose(in);
		return "UNIF header signature not found!";
	}

	fseek(in,28,SEEK_CUR);	/* skip "expansion area" */

	RI.Filename = strdup(filename);
	RI.ROMType = ROM_UNIF;

	for (i = 0; i < 0x10; i++)
	{
		tPRG[i] = tCHR[i] = 0;
	}

	while (!feof(in))
	{
		int id = 0;
		fread(&Signature,4,1,in);
		fread(&BlockLen,4,1,in);
		if (feof(in))
			continue;
		switch (Signature)
		{
		case MKID('MAPR'):
			RI.UNIF_BoardName = (char *)malloc(BlockLen);
			fread(RI.UNIF_BoardName,1,BlockLen,in);
			break;
		case MKID('TVCI'):
			fread(&tp,1,1,in);
			RI.UNIF_NTSCPAL = tp;
			if (tp == 0) NES_SetCPUMode(0);
			if (tp == 1) NES_SetCPUMode(1);
			break;
		case MKID('BATR'):
			fread(&tp,1,1,in);
			RI.UNIF_Battery = TRUE;
			break;
		case MKID('MIRR'):
			fread(&tp,1,1,in);
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
			tPRG[id] = (unsigned char *)malloc(BlockLen);
			fread(tPRG[id],1,BlockLen,in);
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
			tCHR[id] = (unsigned char *)malloc(BlockLen);
			fread(tCHR[id],1,BlockLen,in);
			break;
		default:
			fseek(in,BlockLen,SEEK_CUR);
		}
	}

	PRGPoint = PRG_ROM[0];
	CHRPoint = CHR_ROM[0];
	
	for (i = 0; i < 0x10; i++)
	{
		if (tPRG[i])
		{
			memcpy(PRGPoint, tPRG[i], RI.UNIF_PRGSize[i]);
			PRGPoint += RI.UNIF_PRGSize[i];
			free(tPRG[i]);
		}
		if (tCHR[i])
		{
			memcpy(CHRPoint, tCHR[i], RI.UNIF_CHRSize[i]);
			CHRPoint += RI.UNIF_CHRSize[i];
			free(tCHR[i]);
		}
	}

	fclose(in);

	PRGsize = (unsigned int)(PRGPoint - PRG_ROM[0]);
	NES.PRGMask = ((PRGsize / 0x1000) - 1) & MAX_PRGROM_MASK;
	CHRsize = (unsigned int)(CHRPoint - CHR_ROM[0]);
	NES.CHRMask = ((CHRsize / 0x400) - 1) & MAX_CHRROM_MASK;

	p2Found = FALSE;
	for (p2 = 1; p2 < 0x40000000; p2 <<= 1)
		if (p2 == PRGsize)
			p2Found = TRUE;
	if (!p2Found)
		NES.PRGMask = MAX_PRGROM_MASK;

	p2Found = FALSE;
	for (p2 = 1; p2 < 0x40000000; p2 <<= 1)
		if (p2 == CHRsize)
			p2Found = TRUE;
	if (!p2Found)
		NES.CHRMask = MAX_CHRROM_MASK;

	if (!MapperInterface_LoadMapper(&RI))
	{
		static char err[256];
		sprintf(err,"UNIF boardset \"%s\" not supported!",RI.UNIF_BoardName);
		return err;
	}
	EI.DbgOut("UNIF file loaded: %s (%s) - %s",RI.UNIF_BoardName,MI->Description,CompatLevel[MI->Compatibility]);
	EI.DbgOut("PRG: %iKB; CHR: %iKB",PRGsize >> 10,CHRsize >> 10);
	EI.DbgOut("Battery status: %s",RI.UNIF_Battery ? "present" : "not present");
	{
		const char *mir[6] = {"Horizontal","Vertical","Single-screen L","Single-screen H","Four-screen","Dynamic"};
		EI.DbgOut("Mirroring mode: %i (%s)",RI.UNIF_Mirroring,mir[RI.UNIF_Mirroring]);
	}
	{
		const char *ntscpal[3] = {"NTSC","PAL","Dual"};
		EI.DbgOut("Television standard: %s",ntscpal[RI.UNIF_NTSCPAL]);
	}
	return NULL;
}

const char *	NES_OpenFileFDS (char *filename)
{
	FILE *in;
	unsigned long Header;
	unsigned char numSides;
	int i;

	in = fopen(filename,"rb");
	fread(&Header,4,1,in);
	if (Header != MKID('FDS\x1a'))
	{
		fclose(in);
		return "FDS header signature not found!";
	}
	fread(&numSides,1,1,in);
	fseek(in,11,SEEK_CUR);
	RI.Filename = strdup(filename);
	RI.ROMType = ROM_FDS;

	for (i = 0; i < numSides; i++)
		fread(PRG_ROM[i << 4],1,65500,in);
	fclose(in);

	memcpy(&PRG_ROM[0x400],&PRG_ROM[0x000],numSides << 16);

	RI.FDS_NumSides = numSides;

	NES.PRGMask = ((RI.FDS_NumSides << 4) - 1) & MAX_PRGROM_MASK;

	if (!MapperInterface_LoadMapper(&RI))
		return "Famicom Disk System support not found!";

	EI.DbgOut("FDS file loaded: %s - %s",MI->Description,CompatLevel[MI->Compatibility]);
	EI.DbgOut("Data length: %i disk side(s)",RI.FDS_NumSides);
	NES.SRAM_Size = 1;	// special, so FDS always saves changes
	return NULL;
}

const char *	NES_OpenFileNSF (char *filename)
{
	FILE *in;
	unsigned char Header[128];	/* Header Bytes */
	int ROMlen;

	in = fopen(filename,"rb");
	fseek(in,0,SEEK_END);
	ROMlen = ftell(in) - 128;
	fseek(in,0,SEEK_SET);
	fread(Header,1,128,in);
	if (memcmp(Header,"NESM\x1a\x01",6))
	{
		fclose(in);
		return "NSF header signature not found!";
	}

	RI.Filename = strdup(filename);
	RI.ROMType = ROM_NSF;
	RI.NSF_DataSize = ROMlen;
	RI.NSF_NumSongs = Header[0x06];
	RI.NSF_SoundChips = Header[0x7B];
	RI.NSF_NTSCPAL = Header[0x7A];
	RI.NSF_NTSCSpeed = Header[0x6E] | (Header[0x6F] << 8);
	RI.NSF_PALSpeed = Header[0x78] | (Header[0x79] << 8);
	memcpy(RI.NSF_InitBanks,&Header[0x70],8);
	RI.NSF_InitSong = Header[0x07];
	RI.NSF_InitAddr = Header[0x0A] | (Header[0x0B] << 8);
	RI.NSF_PlayAddr = Header[0x0C] | (Header[0x0D] << 8);
	RI.NSF_Title = (char *)malloc(32);
	RI.NSF_Artist = (char *)malloc(32);
	RI.NSF_Copyright = (char *)malloc(32);
	memcpy(RI.NSF_Title,&Header[0x0E],32);
	memcpy(RI.NSF_Artist,&Header[0x2E],32);
	memcpy(RI.NSF_Copyright,&Header[0x4E],32);
	if (memcmp(RI.NSF_InitBanks,"\0\0\0\0\0\0\0\0",8))
		fread(&PRG_ROM[0][0] + ((Header[0x8] | (Header[0x9] << 8)) & 0x0FFF),1,ROMlen,in);
	else
	{
		memcpy(RI.NSF_InitBanks,"\x00\x01\x02\x03\x04\x05\x06\x07",8);
		fread(&PRG_ROM[0][0] + ((Header[0x8] | (Header[0x9] << 8)) & 0x7FFF),1,ROMlen,in);
	}
	fclose(in);

	if ((RI.NSF_NTSCSpeed == 16666) || (RI.NSF_NTSCSpeed == 16667))
	{
		EI.DbgOut("Adjusting NSF playback speed for NTSC...");
		RI.NSF_NTSCSpeed = 16639;
	}
	if (RI.NSF_PALSpeed == 20000)
	{
		EI.DbgOut("Adjusting NSF playback speed for PAL...");
		RI.NSF_PALSpeed = 19997;
	}

	NES.PRGMask = MAX_PRGROM_MASK;

	if (!MapperInterface_LoadMapper(&RI))
		return "NSF support not found!";
	EI.DbgOut("NSF loaded: %s - %s",MI->Description,CompatLevel[MI->Compatibility]);
	EI.DbgOut("Data length: %iKB",RI.NSF_DataSize >> 10);
	return NULL;
}
/*
const char *	NES_OpenFileNFRF (char *filename)
{
	unsigned long BlockSig, tp, i;
	int BlockHeader;
	char *BoardName;
	unsigned char *tbuf, *PRGPoint = PRG_ROM[0], *CHRPoint = CHR_ROM[0];
	BOOL p2Found = FALSE;
	int p2;
	FILE *in = fopen(filename,"rb");

	if (!in)
		return FALSE;
	fread(&BlockSig,1,4,in);
	if (BlockSig != MKID('NFRF'))
	{
		fclose(in);
		return "Header signature not found!";
	}
	fseek(in,4,SEEK_CUR);
	EI.Flags = 0;
	while (!feof(in))
	{
		fread(&BlockSig,1,4,in);
		fread(&BlockHeader,1,4,in);
		if (feof(in))
			break;
		switch (BlockSig)
		{
		case MKID('BORD'):
			BoardName = (char *)malloc(BlockHeader);
			fread(BoardName,1,BlockHeader,in);
			break;
		case MKID('TVCI'):
			fread(&tp,1,4,in);
			if (BlockHeader-4)
				fseek(in,BlockHeader-4,SEEK_CUR);
			if (tp == 0) NES_SetCPUMode(0);
			if (tp == 1) NES_SetCPUMode(1);	
			break;
		case MKID('BATT'):
			EI.Flags |= 0x02;
			break;
		case MKID('MIRR'):
			fread(&tp,1,4,in);
			if (BlockHeader-4)
				fseek(in,BlockHeader-4,SEEK_CUR);
			if (tp == 3)
				tp = 5;
			EI.Flags |= (tp & 0x0F) << 8;
			break;
		case MKID('PRGM'):
			fread(&tp,1,4,in);
			fseek(in,4,SEEK_CUR);
			BlockHeader -= 8;
			if (BlockHeader < 0)
			{
				fclose(in);
				return "Invalid NRFF - PRGM block corrupt!";
			}
			if (BlockHeader)
			{
				tbuf = (unsigned char *)malloc(BlockHeader);
				fread(tbuf, 1, BlockHeader, in);
				for (i = 0; i < tp; i += BlockHeader)
					memcpy(PRGPoint+i,tbuf,BlockHeader);
			}
			else	memset(PRGPoint,0,tp);	// open bus
			PRGPoint += tp;
			break;
		case MKID('CHAR'):
			fread(&tp,1,4,in);
			fseek(in,4,SEEK_CUR);
			BlockHeader -= 8;
			if (BlockHeader < 0)
			{
				fclose(in);
				return "Invalid NRFF - CHAR block corrupt!";
			}
			if (BlockHeader)
			{
				tbuf = (unsigned char *)malloc(BlockHeader);
				fread(tbuf, 1, BlockHeader, in);
				for (i = 0; i < tp; i += BlockHeader)
					memcpy(CHRPoint+i,tbuf,BlockHeader);
			}
			else	memset(CHRPoint,0,tp);	// open bus
			CHRPoint += tp;
			break;
		default:
			fseek(in,BlockHeader,SEEK_CUR);
			break;
		}
	}
	fclose(in);
	EI.PRG_ROM_Size = (int)(PRGPoint - PRG_ROM[0]);
	EI.CHR_ROM_Size = (int)(CHRPoint - CHR_ROM[0]);
	NES.PRGMask = ((EI.PRG_ROM_Size >> 12) - 1) & MAX_PRGROM_MASK;
	NES.CHRMask = ((EI.CHR_ROM_Size >> 10) - 1) & MAX_CHRROM_MASK;
	for (p2 = 1; p2 < 0x10000000; p2 <<= 1)
		if (p2 == EI.PRG_ROM_Size) p2Found = FALSE;
	if (!p2Found) NES.PRGMask = 0xFFFFFFFF;
	if (!MapperInterface_LoadByBoard(BoardName))
	{
		static char err[256];
		sprintf(err,"Boardset \"%s\" not supported!",RI.NRFF_BoardName);
		return err;
	}
	GFX_ShowText("NFRF image loaded (%s, %i PRG/%i CHR)",BoardName,EI.PRG_ROM_Size >> 10,EI.CHR_ROM_Size >> 10);
	free(BoardName);
	return NULL;
}
*/
void	NES_SetCPUMode (int NewMode)
{
	HMENU hMenu = GetMenu(mWnd);
	if (NewMode == 0)
	{
		CheckMenuRadioItem(hMenu,ID_PPU_MODE_NTSC,ID_PPU_MODE_PAL,ID_PPU_MODE_NTSC,MF_BYCOMMAND);
		PPU.SLEndFrame = 262;
		if (PPU.SLnum >= PPU.SLEndFrame - 1)	/* if we switched from PAL, scanline number could be invalid */
			PPU.SLnum = PPU.SLEndFrame - 2;
		GFX.WantFPS = 60;
		GFX_LoadPalette(GFX.PaletteNTSC);
		APU_SetFPS(60);
		PPU.IsPAL = FALSE;
		EI.DbgOut("Emulation switched to NTSC");
	}
	else
	{
		CheckMenuRadioItem(hMenu,ID_PPU_MODE_NTSC,ID_PPU_MODE_PAL,ID_PPU_MODE_PAL,MF_BYCOMMAND);
		PPU.SLEndFrame = 312;
		GFX.WantFPS = 50;
		GFX_LoadPalette(GFX.PalettePAL);
		APU_SetFPS(50);
		PPU.IsPAL = TRUE;
		EI.DbgOut("Emulation switched to PAL");
	}
}

void	NES_Reset (int ResetType)
{
	int i;
	for (i = 0x0; i < 0x10; i++)
	{
		CPU.ReadHandler[i] = CPU_ReadPRG;
		CPU.WriteHandler[i] = CPU_WritePRG;
		CPU.Readable[i] = FALSE;
		CPU.Writable[i] = FALSE;
		CPU.PRGPointer[i] = NULL;
	}
	CPU.ReadHandler[0] = CPU_ReadRAM;	CPU.WriteHandler[0] = CPU_WriteRAM;
	CPU.ReadHandler[1] = CPU_ReadRAM;	CPU.WriteHandler[1] = CPU_WriteRAM;
	CPU.ReadHandler[2] = PPU_IntRead;	CPU.WriteHandler[2] = PPU_IntWrite;
	CPU.ReadHandler[3] = PPU_IntRead;	CPU.WriteHandler[3] = PPU_IntWrite;
	CPU.ReadHandler[4] = CPU_Read4k;	CPU.WriteHandler[4] = CPU_Write4k;
	if (!NES.GameGenie)
		Genie.CodeStat = 0;
	for (i = 0x0; i < 0x8; i++)
	{
		PPU.ReadHandler[i] = PPU_BusRead;
		PPU.WriteHandler[i] = PPU_BusWriteCHR;
	}
	for (i = 0x8; i < 0x10; i++)
	{
		PPU.ReadHandler[i] = PPU_BusRead;
		PPU.WriteHandler[i] = PPU_BusWriteNT;
	}
	switch (ResetType)
	{
	case RESET_FULL:
		ZeroMemory(PRG_RAM,sizeof(PRG_RAM));
		ZeroMemory(CHR_RAM,sizeof(CHR_RAM));
		EI.DbgOut("Performing initial hard reset...");
		PPU_PowerOn();
		CPU_PowerOn();
		EI.SetCHR_RAM8(0,0);
		EI.Mirror_4();
		if ((MI) && (MI->Reset))
			MI->Reset(RESET_HARD);
		break;
	case RESET_HARD:
		EI.DbgOut("Performing hard reset...");
		ZeroMemory(PRG_RAM,sizeof(PRG_RAM));
		ZeroMemory(CHR_RAM,sizeof(CHR_RAM));
		CPU_PowerOn();
		if (MI2)
		{
			MI = MI2;
			MI2 = NULL;
		}
		PPU_PowerOn();
		if (NES.GameGenie)
			Genie_Reset();
		else
		{
			EI.SetCHR_RAM8(0,0);
			EI.Mirror_4();
			if ((MI) && (MI->Reset))
				MI->Reset(RESET_HARD);
		}
		break;
	case RESET_SOFT:
		EI.DbgOut("Performing soft reset...");
		if (MI2)
		{
			MI = MI2;
			MI2 = NULL;
		}
		EI.SetCHR_RAM8(0,0);
		EI.Mirror_4();
		if (NES.GameGenie)
		{
			if (Genie.CodeStat & 1)
				Genie_Init();	// Set up the PRG read handlers BEFORE resetting the mapper
			else	Genie_Reset();	// map the Game Genie back in its place
		}
		if ((MI) && (MI->Reset))
			MI->Reset(RESET_SOFT);
		break;
	}
#ifdef ENABLE_DEBUGGER
	Debugger.TraceOffset = -1;
#endif	/* ENABLE_DEBUGGER */
	APU_Reset();
	CPU_Reset();
	PPU_Reset();
#ifdef ENABLE_DEBUGGER
	if (Debugger.Enabled)
		Debugger_Update();
#endif	/* ENABLE_DEBUGGER */
	NES.Scanline = FALSE;
	EI.DbgOut("Reset complete.");
}

void	NES_Run (void)
{
#ifdef	CPU_BENCHMARK
	/* Run with cyctest.nes */
	int i;
	LARGE_INTEGER ClockFreq;
	LARGE_INTEGER ClockVal1, ClockVal2;
	QueryPerformanceFrequency(&ClockFreq);
	QueryPerformanceCounter(&ClockVal1);
	for (i = 0; i < 454211; i++)
	{
		CPU_ExecOp();
		CPU_ExecOp();
		CPU_ExecOp();
		CPU_ExecOp();
		CPU_ExecOp();
		CPU_ExecOp();
		CPU_ExecOp();
		CPU_ExecOp();
		CPU_ExecOp();
		CPU_ExecOp();
	}
	QueryPerformanceCounter(&ClockVal2);

	EI.DbgOut("10 seconds emulated in %lu milliseconds",(unsigned long)((ClockVal2.QuadPart - ClockVal1.QuadPart) * 1000 / ClockFreq.QuadPart));
#else
rerun:
	NES.Stopped = FALSE;

#ifdef ENABLE_DEBUGGER
	Debugger.TraceOffset = -1;
#endif	/* ENABLE_DEBUGGER */

	Controllers_Acquire();
	if ((!NES.Stop) && (NES.SoundEnabled))
		APU_SoundON();

	NES.NeedReset = 0;
	do
	{
#ifdef ENABLE_DEBUGGER
		if (Debugger.Enabled)
			Debugger_AddInst();
#endif	/* ENABLE_DEBUGGER */
		CPU_ExecOp();
#ifdef ENABLE_DEBUGGER
		if (Debugger.Enabled)
			Debugger_Update();
#endif	/* ENABLE_DEBUGGER */
		if (NES.Scanline)
		{
			int SLnum = PPU.SLnum;
			NES.Scanline = FALSE;
			ProcessMessages();
			if (States.NeedLoad)	// load state anywhere
			{
				States_LoadState();
				continue;
			}
			else if (SLnum == 240)
			{
#ifdef ENABLE_DEBUGGER
				if (Debugger.Enabled)
					Debugger_UpdateGraphics();
#endif	/* ENABLE_DEBUGGER */
				if (States.NeedSave)	// but only save on scanline 240
					States_SaveState();
				if (NES.FrameStep)	// and if we need to stop, do it here
				{	// so if we save, it won't advance the CPU/PPU at all
					NES.GotStep = FALSE;
					Controllers_ShowFrame();
					while (NES.FrameStep && !NES.GotStep && !NES.Stop)
						ProcessMessages();
				}
			}
			else if (SLnum == 241)
				Controllers_UpdateInput();
		}
	}	while (!NES.Stop);

	APU_SoundOFF();
	Controllers_UnAcquire();

	NES.Stopped = TRUE;
	GFX_UpdateTitlebar();

	if (!NES.ROMLoaded)
	{
		NES.ROMLoaded = TRUE;	/* so CloseFile() flushes the mapper */
		NES_CloseFile();
		return;
	}
	if (NES.NeedQuit)
	{
		NES_Release();
		return;
	}
	if (NES.NeedReset == RESET_HARD)
	{
		NES.NeedReset = 0;
		NES_Reset(RESET_HARD);
	}
	else if (NES.NeedReset == RESET_SOFT)
	{
		NES.NeedReset = 0;
		NES_Reset(RESET_SOFT);
		NES.Stop = FALSE;
		goto rerun;
	}
#endif	/* CPU_BENCHMARK */
}

void	NES_MapperConfig (void)
{
	MI->Config(CFG_WINDOW,TRUE);
}

void	NES_UpdateInterface (void)
{
#ifdef ENABLE_DEBUGGER
	Debugger_SetMode(Debugger.Mode);
#endif	/* ENABLE_DEBUGGER */
	SetWindowClientArea(mWnd,256 * SizeMult,240 * SizeMult);
}

void	NES_LoadSettings (void)
{
	HMENU hMenu = GetMenu(mWnd);
	HKEY SettingsBase;
	unsigned long Size;	unsigned long Type;
	int Port1T = 0, Port2T = 0, FSPort1T = 0, FSPort2T = 0, FSPort3T = 0, FSPort4T = 0, ExpPortT = 0;
	int PosX, PosY;

	/* Load Defaults */
	SizeMult = 1;
	NES.SoundEnabled = 1;
	GFX.aFSkip = 1;
	GFX.FSkip = 0;
	GFX.PaletteNTSC = 0;
	GFX.PalettePAL = 1;
	GFX.NTSChue = 340;
	GFX.NTSCsat = 30;
	GFX.CustPaletteNTSC[0] = GFX.CustPalettePAL[0] = 0;
	Controllers.Port1.Type = 0;
	ZeroMemory(Controllers.Port1.Buttons,sizeof(Controllers.Port1.Buttons));
	Controllers.Port2.Type = 0;
	ZeroMemory(Controllers.Port2.Buttons,sizeof(Controllers.Port2.Buttons));
	Controllers.ExpPort.Type = 0;
	ZeroMemory(Controllers.ExpPort.Buttons,sizeof(Controllers.ExpPort.Buttons));

	GFX.SlowDown = FALSE;
	GFX.SlowRate = 2;
	CheckMenuRadioItem(hMenu,ID_PPU_SLOWDOWN_2,ID_PPU_SLOWDOWN_20,ID_PPU_SLOWDOWN_2,MF_BYCOMMAND);

	NES.FrameStep = FALSE;
	
	/* End Defaults */

	RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Nintendulator\\", 0, KEY_ALL_ACCESS, &SettingsBase);
	Size = 4;
	RegQueryValueEx(SettingsBase,"SoundEnabled",0,&Type,(unsigned char *)&NES.SoundEnabled,&Size);
	RegQueryValueEx(SettingsBase,"SizeMult"    ,0,&Type,(unsigned char *)&SizeMult   ,&Size);
	RegQueryValueEx(SettingsBase,"FSkip"       ,0,&Type,(unsigned char *)&GFX.FSkip  ,&Size);
	RegQueryValueEx(SettingsBase,"aFSkip"      ,0,&Type,(unsigned char *)&GFX.aFSkip ,&Size);
	RegQueryValueEx(SettingsBase,"PPUMode"     ,0,&Type,(unsigned char *)&PPU.IsPAL  ,&Size);
	RegQueryValueEx(SettingsBase,"AutoRun"     ,0,&Type,(unsigned char *)&NES.AutoRun,&Size);
	RegQueryValueEx(SettingsBase,"PosX"        ,0,&Type,(unsigned char *)&PosX       ,&Size);
	RegQueryValueEx(SettingsBase,"PosY"        ,0,&Type,(unsigned char *)&PosY       ,&Size);

	RegQueryValueEx(SettingsBase,"PaletteNTSC" ,0,&Type,(unsigned char *)&GFX.PaletteNTSC,&Size);
	RegQueryValueEx(SettingsBase,"PalettePAL"  ,0,&Type,(unsigned char *)&GFX.PalettePAL ,&Size);
	RegQueryValueEx(SettingsBase,"NTSChue"     ,0,&Type,(unsigned char *)&GFX.NTSChue    ,&Size);
	RegQueryValueEx(SettingsBase,"NTSCsat"     ,0,&Type,(unsigned char *)&GFX.NTSCsat    ,&Size);

	Size = 256;
	RegQueryValueEx(SettingsBase,"CustPaletteNTSC",0,&Type,(unsigned char *)&GFX.CustPaletteNTSC,&Size);
	RegQueryValueEx(SettingsBase,"CustPalettePAL" ,0,&Type,(unsigned char *)&GFX.CustPalettePAL ,&Size);

	Size = 4;
	RegQueryValueEx(SettingsBase,"Port1T"  ,0,&Type,(unsigned char *)&Port1T  ,&Size);
	RegQueryValueEx(SettingsBase,"Port2T"  ,0,&Type,(unsigned char *)&Port2T  ,&Size);
	RegQueryValueEx(SettingsBase,"FSPort1T",0,&Type,(unsigned char *)&FSPort1T,&Size);
	RegQueryValueEx(SettingsBase,"FSPort2T",0,&Type,(unsigned char *)&FSPort2T,&Size);
	RegQueryValueEx(SettingsBase,"FSPort3T",0,&Type,(unsigned char *)&FSPort3T,&Size);
	RegQueryValueEx(SettingsBase,"FSPort4T",0,&Type,(unsigned char *)&FSPort4T,&Size);
	RegQueryValueEx(SettingsBase,"ExpPortT",0,&Type,(unsigned char *)&ExpPortT,&Size);

	if ((Port1T == STD_POWERPAD) || (Port2T == STD_POWERPAD))
		StdPort_SetControllerType(&Controllers.Port1,STD_POWERPAD);
	else
	{
		StdPort_SetControllerType(&Controllers.Port1,Port1T);
		StdPort_SetControllerType(&Controllers.Port2,Port2T);
	}
	StdPort_SetControllerType(&Controllers.FSPort1,FSPort1T);
	StdPort_SetControllerType(&Controllers.FSPort2,FSPort2T);
	StdPort_SetControllerType(&Controllers.FSPort3,FSPort3T);
	StdPort_SetControllerType(&Controllers.FSPort4,FSPort4T);
	ExpPort_SetControllerType(&Controllers.ExpPort,ExpPortT);

	Size = sizeof(Controllers.Port1.Buttons);
	RegQueryValueEx(SettingsBase,"Port1D"  ,0,&Type,(unsigned char *)Controllers.Port1.Buttons  ,&Size);
	RegQueryValueEx(SettingsBase,"Port2D"  ,0,&Type,(unsigned char *)Controllers.Port2.Buttons  ,&Size);
	RegQueryValueEx(SettingsBase,"FSPort1D",0,&Type,(unsigned char *)Controllers.FSPort1.Buttons,&Size);
	RegQueryValueEx(SettingsBase,"FSPort2D",0,&Type,(unsigned char *)Controllers.FSPort2.Buttons,&Size);
	RegQueryValueEx(SettingsBase,"FSPort3D",0,&Type,(unsigned char *)Controllers.FSPort3.Buttons,&Size);
	RegQueryValueEx(SettingsBase,"FSPort4D",0,&Type,(unsigned char *)Controllers.FSPort4.Buttons,&Size);
	Size = sizeof(Controllers.ExpPort.Buttons);
	RegQueryValueEx(SettingsBase,"ExpPortD",0,&Type,(unsigned char *)Controllers.ExpPort.Buttons,&Size);
	Controllers_SetDeviceUsed();
	
	RegCloseKey(SettingsBase);
	
	if (NES.SoundEnabled)
		CheckMenuItem(hMenu, ID_SOUND_ENABLED, MF_CHECKED);
	
	if (GFX.aFSkip)
		CheckMenuItem(hMenu, ID_PPU_FRAMESKIP_AUTO, MF_CHECKED);

	if (NES.AutoRun)
		CheckMenuItem(hMenu, ID_CPU_AUTORUN, MF_CHECKED);

	GFX_SetFrameskip();

	switch (SizeMult)
	{
	case 1:	CheckMenuRadioItem(hMenu,ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_1X,MF_BYCOMMAND);	break;
	default:SizeMult = 2;
	case 2:	CheckMenuRadioItem(hMenu,ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_2X,MF_BYCOMMAND);	break;
	case 3:	CheckMenuRadioItem(hMenu,ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_3X,MF_BYCOMMAND);	break;
	case 4:	CheckMenuRadioItem(hMenu,ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_4X,MF_BYCOMMAND);	break;
	}

	if (PPU.IsPAL)
		NES_SetCPUMode(1);
	else	NES_SetCPUMode(0);
	
	CheckMenuRadioItem(hMenu,ID_DEBUG_LEVEL1,ID_DEBUG_LEVEL4,ID_DEBUG_LEVEL1,MF_BYCOMMAND);

	SetWindowPos(mWnd,mWnd,PosX,PosY,0,0,SWP_NOSIZE | SWP_NOZORDER);

	NES_UpdateInterface();
}

void	NES_SaveSettings (void)
{
	HKEY SettingsBase;
	RECT wRect;
	GetWindowRect(mWnd,&wRect);
	if (RegOpenKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\Nintendulator\\",0,KEY_ALL_ACCESS,&SettingsBase))
		RegCreateKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\Nintendulator\\",0,"NintendulatorClass",REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&SettingsBase,NULL);
	RegSetValueEx(SettingsBase,"SoundEnabled",0,REG_DWORD,(unsigned char *)&NES.SoundEnabled,4);
	RegSetValueEx(SettingsBase,"SizeMult"    ,0,REG_DWORD,(unsigned char *)&SizeMult        ,4);
	RegSetValueEx(SettingsBase,"FSkip"       ,0,REG_DWORD,(unsigned char *)&GFX.FSkip       ,4);
	RegSetValueEx(SettingsBase,"aFSkip"      ,0,REG_DWORD,(unsigned char *)&GFX.aFSkip      ,4);
	RegSetValueEx(SettingsBase,"PPUMode"     ,0,REG_DWORD,(unsigned char *)&PPU.IsPAL       ,4);
	RegSetValueEx(SettingsBase,"AutoRun"     ,0,REG_DWORD,(unsigned char *)&NES.AutoRun     ,4);
	RegSetValueEx(SettingsBase,"PosX"        ,0,REG_DWORD,(unsigned char *)&wRect.left      ,4);
	RegSetValueEx(SettingsBase,"PosY"        ,0,REG_DWORD,(unsigned char *)&wRect.top       ,4);

	RegSetValueEx(SettingsBase,"PaletteNTSC" ,0,REG_DWORD,(unsigned char *)&GFX.PaletteNTSC ,4);
	RegSetValueEx(SettingsBase,"PalettePAL"  ,0,REG_DWORD,(unsigned char *)&GFX.PalettePAL  ,4);
	RegSetValueEx(SettingsBase,"NTSChue"     ,0,REG_DWORD,(unsigned char *)&GFX.NTSChue     ,4);
	RegSetValueEx(SettingsBase,"NTSCsat"     ,0,REG_DWORD,(unsigned char *)&GFX.NTSCsat     ,4);

	RegSetValueEx(SettingsBase,"CustPaletteNTSC",0,REG_SZ,(unsigned char *)GFX.CustPaletteNTSC,sizeof(GFX.CustPaletteNTSC));
	RegSetValueEx(SettingsBase,"CustPalettePAL" ,0,REG_SZ,(unsigned char *)GFX.CustPalettePAL ,sizeof(GFX.CustPalettePAL));


	RegSetValueEx(SettingsBase,"Port1T"  ,0,REG_DWORD,(unsigned char *)&Controllers.Port1.Type  ,sizeof(Controllers.Port1.Type));
	RegSetValueEx(SettingsBase,"Port2T"  ,0,REG_DWORD,(unsigned char *)&Controllers.Port2.Type  ,sizeof(Controllers.Port2.Type));
	RegSetValueEx(SettingsBase,"FSPort1T",0,REG_DWORD,(unsigned char *)&Controllers.FSPort1.Type,sizeof(Controllers.FSPort1.Type));
	RegSetValueEx(SettingsBase,"FSPort2T",0,REG_DWORD,(unsigned char *)&Controllers.FSPort2.Type,sizeof(Controllers.FSPort2.Type));
	RegSetValueEx(SettingsBase,"FSPort3T",0,REG_DWORD,(unsigned char *)&Controllers.FSPort3.Type,sizeof(Controllers.FSPort3.Type));
	RegSetValueEx(SettingsBase,"FSPort4T",0,REG_DWORD,(unsigned char *)&Controllers.FSPort4.Type,sizeof(Controllers.FSPort4.Type));
	RegSetValueEx(SettingsBase,"ExpPortT",0,REG_DWORD,(unsigned char *)&Controllers.ExpPort.Type,sizeof(Controllers.ExpPort.Type));
	
	RegSetValueEx(SettingsBase,"Port1D"  ,0,REG_BINARY,(unsigned char *)Controllers.Port1.Buttons  ,sizeof(Controllers.Port1.Buttons));
	RegSetValueEx(SettingsBase,"Port2D"  ,0,REG_BINARY,(unsigned char *)Controllers.Port2.Buttons  ,sizeof(Controllers.Port2.Buttons));
	RegSetValueEx(SettingsBase,"FSPort1D",0,REG_BINARY,(unsigned char *)Controllers.FSPort1.Buttons,sizeof(Controllers.FSPort1.Buttons));
	RegSetValueEx(SettingsBase,"FSPort2D",0,REG_BINARY,(unsigned char *)Controllers.FSPort2.Buttons,sizeof(Controllers.FSPort2.Buttons));
	RegSetValueEx(SettingsBase,"FSPort3D",0,REG_BINARY,(unsigned char *)Controllers.FSPort3.Buttons,sizeof(Controllers.FSPort3.Buttons));
	RegSetValueEx(SettingsBase,"FSPort4D",0,REG_BINARY,(unsigned char *)Controllers.FSPort4.Buttons,sizeof(Controllers.FSPort4.Buttons));
	RegSetValueEx(SettingsBase,"ExpPortD",0,REG_BINARY,(unsigned char *)Controllers.ExpPort.Buttons,sizeof(Controllers.ExpPort.Buttons));
	
	RegCloseKey(SettingsBase);
}

void	NES_Repaint (void)
{
	GFX_Repaint();
}
