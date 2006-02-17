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

#define MSBSwapShort(ShortVar)	ShortVar = (((ShortVar & 0xFF00) >> 8) | ((ShortVar & 0x00FF) << 8))
#define MSBSwapLong(LongVar)	LongVar = (((LongVar & 0xFF000000) >> 24) | ((LongVar & 0x00FF0000) >> 8) | ((LongVar & 0x0000FF00) << 8) | ((LongVar & 0x000000FF) << 24))

struct	tStates	States;

void	States_Init (void)
{
	States.SelSlot = 0;
	States.NeedSave = FALSE;
	States.NeedLoad = FALSE;
}

void	States_SetFilename (char *Filename)
{
	char Tmp[MAX_PATH];
	int i;
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

void	States_SaveState (void)
{
	int i;
	char tpchr[256];
	unsigned long tpl;
	unsigned short tps;
	unsigned char tpb;
	int NumBlocks = 0;
	FILE *out;

	States.NeedSave = FALSE;
	sprintf(tpchr,"%s.ss%i",States.BaseFilename,States.SelSlot);
	out = fopen(tpchr,"w+b");

	fwrite("SNSS",1,4,out);
	tpl = MSBSwapLong(NumBlocks);
	fwrite(&tpl,1,4,out);
	{
		//Write BASR Block
		NumBlocks++;
			//Header
		fwrite("BASR",1,4,out);
		tpl = 1;
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
		tpl = 0x1931;
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
			//Data
		fwrite(&CPU.A,1,1,out);
		fwrite(&CPU.X,1,1,out);
		fwrite(&CPU.Y,1,1,out);
		CPU_JoinFlags();
		fwrite(&CPU.P,1,1,out);
		fwrite(&CPU.SP,1,1,out);
		fwrite(&CPU.PCH,1,1,out);
		fwrite(&CPU.PCL,1,1,out);
		fwrite(&PPU.Reg2000,1,1,out);
		fwrite(&PPU.Reg2001,1,1,out);
		fwrite(CPU_RAM,1,0x800,out);
		fwrite(PPU.Sprite,1,0x100,out);
		fwrite(PPU_VRAM,1,0x1000,out);
		fwrite(PPU.Palette,1,0x20,out);
		for (i = 8; i < 12; i++)
		{
			tpb = (PPU.CHRPointer[i] - PPU_VRAM[0]) >> 10;
			fwrite(&tpb,1,1,out);
		}
		tps = (unsigned short)PPU.VRAMAddr;
		MSBSwapShort(tps);
		fwrite(&tps,1,2,out);
		fwrite(&PPU.SprAddr,1,1,out);
		fwrite(&PPU.IntX,1,1,out);
	}
	for (i = sizeof(CHR_RAM) - 1; i >= 0; i--)
		if (CHR_RAM[i >> 10][i & 0x3FF])
			break;
	if (i >= 0)
	{	//Write VRAM Block
		NumBlocks++;
		fwrite("VRAM",1,4,out);
		tpl = 1;		// Version
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
		tpl = i & 0x7000;
		if (i & 0xFFF)
			tpl += 0x1000;	// Length
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
			//Data
		MSBSwapLong(tpl);
		fwrite(CHR_RAM[0],1,tpl,out);
	}

	for (i = sizeof(PRG_RAM) - 1; i >= 0; i--)
		if (PRG_RAM[i >> 12][i & 0xFFF])
			break;
	if (i >= 0)
	{	//Write SRAM Block
		NumBlocks++;
		fwrite("SRAM",1,4,out);
		tpl = 1;		// Version
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
		tpl = i & 0xF000;
		if (i & 0xFFF)
			tpl += 0x1000;	// Length
		i = tpl;
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
			//Data
		tpl = 1;
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
			// Is SRAM writeable? who cares?
		fwrite(PRG_RAM[0],1,i,out);
	}
	{	// Always write MPRD block
		Ar128 tpmi;
		NumBlocks++;
		fwrite("MPRD",1,4,out);
		tpl = 1;				//Version
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
		tpl = 0x98;				//Length
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
			//Data
		for (i = 0; i < 4; i++)
		{
			tps = MP.GetPRG_ROM4(8 + (i << 1)) >> 1;
			if ((unsigned)tps == -1)
				tps = 0x8000 | (MP.GetPRG_RAM4(8 + (i << 1)) >> 1);
			MSBSwapShort(tps);
			fwrite(&tps,2,1,out);
		}
		for (i = 0; i < 8; i++)
		{
			tps = MP.GetCHR_ROM1(i);
			if ((unsigned)tps == -1)
				tps = 0x8000 | MP.GetCHR_RAM1(i);
			MSBSwapShort(tps);
			fwrite(&tps,2,1,out);
		}
		if ((MI) && (MI->SaveMI))
			MI->SaveMI(tpmi);
		fwrite(tpmi,1,0x80,out);
	}
/*	{
		//Write CNTR Block
		NumBlocks++;
			//Header
		fwrite("CNTR",1,4,out);
		tpl = 1;				//Version
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
		tpl = 0x11;				//Length
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
			//Data
		tpb = 0;
		fwrite(&tpb,1,1,out);
		fwrite(&tpb,1,1,out);
		fwrite(&Controllers.Port1.BitPtr,1,1,out);
		fwrite(&Controllers.Port2.BitPtr,1,1,out);
		fwrite(&Controllers.Port1.Strobe,1,1,out);
		tps = 0;
		MSBSwapShort(tps);
		fwrite(&tps,1,2,out);
		tpl = Controllers.Port1.Bits;
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
		tpb = 1;
		fwrite(&tpb,1,1,out);
		tpl = Controllers.Port2.Bits;
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
		fwrite(&tpb,1,1,out);
	}*/
	{
		unsigned char tpk[0x16];
		//Write SOUN Block
		NumBlocks++;
			//Header
		fwrite("SOUN",1,4,out);
		tpl = 1;				//Version
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
		tpl = 0x16;				//Length
		MSBSwapLong(tpl);
		fwrite(&tpl,1,4,out);
			//Data
		APU_GetSoundRegisters(tpk);
		fwrite(tpk,1,0x16,out);
	}

	//Write final NumBlocks
	fseek(out, 4, SEEK_SET);
	tpl = MSBSwapLong(NumBlocks);
	fwrite(&tpl,1,4,out);
	fclose(out);

	GFX_ShowText("State saved: %i", States.SelSlot);
}

void	States_LoadState (void)
{
	int i, j;
	char tpchr[256];
	FILE *in;
	char SNSSSig[4];
//	unsigned long tpl;
	unsigned short tps;
	unsigned char tpc;
	int NumBlocks;
	char BlockSig[4];
	int BlockHeader[2];

	States.NeedLoad = FALSE;
	sprintf(tpchr,"%s.ss%i",States.BaseFilename,States.SelSlot);
	in = fopen(tpchr,"rb");
	if (!in)
	{
		GFX_ShowText("No such save state: %i", States.SelSlot);
		return;
	}

	fread(SNSSSig,1,4,in);
	if (strncmp(SNSSSig,"SNSS",4))
	{
		fclose(in);
		GFX_ShowText("Corrupted save state: %i", States.SelSlot);
		return;
	}

	NES_Reset(RESET_SOFT);
	PPU.Clockticks = 0;
	PPU.SLnum = 0;

	fread(&NumBlocks,1,4,in);
	MSBSwapLong(NumBlocks);

	for (i = 0; i < NumBlocks; i++)
	{
		fread(BlockSig,1,4,in);
		fread(BlockHeader,4,2,in);
		MSBSwapLong(BlockHeader[0]);
		MSBSwapLong(BlockHeader[1]);
		
		if (!strncmp(BlockSig,"BASR",4))
		{
			if (BlockHeader[0] == 1)
			{
				fread(&CPU.A,1,1,in);
				fread(&CPU.X,1,1,in);
				fread(&CPU.Y,1,1,in);
				fread(&CPU.P,1,1,in);
				CPU_SplitFlags();
				fread(&CPU.SP,1,1,in);
				fread(&CPU.PCH,1,1,in);
				fread(&CPU.PCL,1,1,in);
				fread(&PPU.Reg2000,1,1,in);
				fread(&PPU.Reg2001,1,1,in);
				fread(CPU_RAM,1,0x800,in);
				fread(PPU.Sprite,1,0x100,in);
				fread(PPU_VRAM,1,0x1000,in);
				fread(PPU.Palette,1,0x20,in);
				for (j = 8; j < 12; j++)
				{
					fread(&tpc,1,1,in);
					PPU.CHRPointer[j] = PPU_VRAM[tpc];
				}
				fread(&tps,1,2,in);
				MSBSwapShort(tps);
				PPU.VRAMAddr = tps;
				fread(&PPU.SprAddr,1,1,in);
				fread(&PPU.IntX,1,1,in);
			}
		}
		else if (!strncmp(BlockSig,"VRAM",4))
		{
			if (BlockHeader[0] == 1)
				fread(CHR_RAM[0],1,BlockHeader[1],in);
		}
		else if (!strncmp(BlockSig,"SRAM",4))
		{
			if (BlockHeader[0] == 1)
			{
				fseek(in,4,SEEK_CUR);
				fread(PRG_RAM[0],1,BlockHeader[1],in);
			}
		}
/*		else if (!strncmp(BlockSig,"CNTR",4))
		{
			if (BlockHeader[0] == 1)
			{
				fseek(in,2,SEEK_CUR);
				fread(&Controllers.Port1.BitPtr,1,1,in);
				fread(&Controllers.Port2.BitPtr,1,1,in);
				fread(&Controllers.Port1.Strobe,1,1,in);		//Strobe bit
				fseek(in,2,SEEK_CUR);
				fread(&tpl,1,4,in);
				MSBSwapLong(tpl);
				Controllers.Port1.Bits = tpl;
				fseek(in,1,SEEK_CUR);
				fread(&tpl,1,4,in);
				MSBSwapLong(tpl);
				Controllers.Port2.Bits = tpl;
				fseek(in,1,SEEK_CUR);
				fseek(in,BlockHeader[1] - 0x11,SEEK_CUR);	// skip movie data
			}
		}*/
		else if (!strncmp(BlockSig,"SOUN",4))
		{
			if (BlockHeader[0] == 1)
			{
				unsigned char tpk[0x16];
				fread(tpk,1,0x16,in);
				APU_SetSoundRegisters(tpk);
			}
		}
		else if (!strncmp(BlockSig,"MPRD",4))
		{
			if (BlockHeader[0] == 1)
			{
				Ar128 tpmi;
				for (j = 0; j < 4; j++)
				{	
					fread(&tps,2,1,in);
					MSBSwapShort(tps);
					if (tps & 0x8000)
						MP.SetPRG_RAM8(8 | (j << 1),tps);
					else	MP.SetPRG_ROM8(8 | (j << 1),tps);
				}
				for (j = 0; j < 8; j++)
				{	
					fread(&tps,2,1,in);
					MSBSwapShort(tps);
					if (tps & 0x8000)
						MP.SetCHR_RAM1(j,tps);
					else	MP.SetCHR_ROM1(j,tps);
				}
				fread(tpmi,1,0x80,in);
				if (MI->LoadMI)
					MI->LoadMI(tpmi);
			}
		}
	}
	fclose(in);
	GFX_ShowText("State loaded: %i", States.SelSlot);
}
