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

#define	STATES_VERSION	"0910"

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

void	States_SaveState (void)
{
	unsigned short tps;
	char tpchr[256];
	int clen, flen;
	FILE *out;

	States.NeedSave = FALSE;
	sprintf(tpchr,"%s.ns%i",States.BaseFilename,States.SelSlot);
	out = fopen(tpchr,"w+b");
	flen = 0;

	fwrite("NSS\x1A",1,4,out);
	fwrite(STATES_VERSION,1,4,out);
	fwrite(&flen,1,4,out);
	fwrite(&flen,1,4,out);
	{
		clen = 0;
		fwrite("CPUS",1,4,out);		flen += 4;
		fwrite(&clen,1,4,out);		flen += 4;
			//Data
		fwrite(&CPU.PCH,1,1,out);	clen++;		//	PCL	uint8		Program Counter, low byte
		fwrite(&CPU.PCL,1,1,out);	clen++;		//	PCH	uint8		Program Counter, high byte
		fwrite(&CPU.A,1,1,out);		clen++;		//	A	uint8		Accumulator
		fwrite(&CPU.X,1,1,out);		clen++;		//	X	uint8		X register
		fwrite(&CPU.Y,1,1,out);		clen++;		//	Y	uint8		Y register
		fwrite(&CPU.SP,1,1,out);	clen++;		//	SP	uint8		Stack pointer
		CPU_JoinFlags();
		fwrite(&CPU.P,1,1,out);		clen++;		//	P	uint8		Processor status register
		fwrite(&CPU.LastRead,1,1,out);	clen++;		//	BUS	uint8		Last contents of data bus
		fwrite(&CPU.WantNMI,1,1,out);	clen++;		//	NMI	uint8		TRUE if falling edge detected on /NMI
		fwrite(&CPU.WantIRQ,1,1,out);	clen++;		//	IRQ	uint8		Flags 1=FRAME, 2=DPCM, 4=EXTERNAL
		fwrite(CPU_RAM,1,0x800,out);	clen += 0x800;	//	RAM	uint8[0x800]	2KB work RAM

		fseek(out,-clen - 4,SEEK_CUR);
		fwrite(&clen,1,4,out);
		fseek(out,clen,SEEK_CUR);	flen += clen;
	}
	{
		clen = 0;
		fwrite("PPUS",1,4,out);		flen += 4;
		fwrite(&clen,1,4,out);		flen += 4;
			//Data
		fwrite(PPU_VRAM,1,0x1000,out);	clen += 0x1000;	//	NTAR	uint8[0x1000]	4 KB of name/attribute table RAM
		fwrite(PPU.Sprite,1,0x100,out);	clen += 0x100;	//	SPRA	uint8[0x100]	256 bytes of sprite RAM
		fwrite(PPU.Palette,1,0x20,out);	clen += 0x20;	//	PRAM	uint8[0x20]	32 bytes of palette index RAM
		fwrite(&PPU.Reg2000,1,1,out);	clen++;		//	R2000	uint8		Last value written to $2000
		fwrite(&PPU.Reg2001,1,1,out);	clen++;		//	R2001	uint8		Last value written to $2001
		fwrite(&PPU.Reg2002,1,1,out);	clen++;		//	R2002	uint8		Current contents of $2002
		fwrite(&PPU.SprAddr,1,1,out);	clen++;		//	SPADR	uint8		SPR-RAM Address ($2003)

		fwrite(&PPU.IntX,1,1,out);	clen++;		//	XOFF	uint8		Tile X-offset.

		fwrite(&PPU.HVTog,1,1,out);	clen++;		//	VTOG	uint8		Toggle used by $2005 and $2006.
		tps = (unsigned short)PPU.VRAMAddr;
		fwrite(&tps,2,1,out);		clen += 2;	//	RADD	uint16		VRAM Address
		tps = (unsigned short)PPU.IntReg;
		fwrite(&tps,2,1,out);		clen += 2;	//	TADD	uint16		VRAM Address Latch
		fwrite(&PPU.ppuLatch,1,1,out);	clen++;		//	VBUF	uint8		VRAM Read Buffer
		fwrite(&PPU.ppuLatch,1,1,out);	clen++;		//	PGEN	uint8		PPU "general" latch.  See Ki's document. ***CHECKME***

		tps = (unsigned short)PPU.Clockticks;
		fwrite(&tps,2,1,out);		clen += 2;	//	TICKS	uint16		Clock Ticks (0..340)
		tps = (unsigned short)PPU.SLnum;
		fwrite(&tps,2,1,out);		clen += 2;	//	SLNUM	uint16		Scanline number
		fwrite(&PPU.ShortSL,1,1,out);	clen++;		//	SHORT	uint8		Short frame (last scanline 1 clock tick shorter)

		tps = (unsigned short)PPU.IOAddr;
		fwrite(&tps,2,1,out);		clen += 2;	//	IOADD	uint16		External I/O Address
		fwrite(&PPU.IOVal,1,1,out);	clen++;		//	IOVAL	uint8		External I/O Value
		fwrite(&PPU.IOMode,1,1,out);	clen++;		//	IOMOD	uint8		External I/O Mode (0=none, 1=renderer, 2=r2007, 3=w2007)

		fwrite(&PPU.IsPAL,1,1,out);	clen++;		//	NTSCP	uint8		0 for NTSC, 1 for PAL

		fseek(out,-clen - 4,SEEK_CUR);
		fwrite(&clen,1,4,out);
		fseek(out,clen,SEEK_CUR);	flen += clen;
	}
	{
		clen = 0;
		fwrite("APUS",1,4,out);		flen += 4;
		fwrite(&clen,1,4,out);		flen += 4;
			//Data
		fwrite(APU.Regs,1,0x17,out);	clen += 0x17;	//	REGS	uint8[0x17]	All of the APU registers
		fseek(out,-clen - 4,SEEK_CUR);
		fwrite(&clen,1,4,out);
		fseek(out,clen,SEEK_CUR);	flen += clen;
	}
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

	// Write final filesize
	fseek(out,8,SEEK_SET);
	fwrite(&flen,1,4,out);
	fclose(out);

	GFX_ShowText("State saved: %i", States.SelSlot);
}

void	States_LoadState (void)
{
	char tpchr[256];
	FILE *in;
	unsigned short tps;
	char csig[4];
	int clen, flen;
	BOOL SSOK = TRUE;

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
		GFX_ShowText("Corrupted save state: %i", States.SelSlot);
		return;
	}
	fread(tpchr,1,4,in);
	if (memcmp(tpchr,STATES_VERSION,4))
	{
		fclose(in);
		tpchr[4] = '0';
		GFX_ShowText("Incorrect savestate version (%s): %i", tpchr, States.SelSlot);
		return;
	}
	fread(&flen,4,1,in);
	fseek(in,4,SEEK_CUR);

	NES_Reset(RESET_SOFT);

	fread(&csig,1,4,in);	flen -= 4;
	fread(&clen,4,1,in);	flen -= 4;
	while (flen > 0)
	{
		flen -= clen;
		if (!strncmp(csig,"CPUS",4))
		{
			fread(&CPU.PCH,1,1,in);		clen--;		//	PCL	uint8		Program Counter, low byte
			fread(&CPU.PCL,1,1,in);		clen--;		//	PCH	uint8		Program Counter, high byte
			fread(&CPU.A,1,1,in);		clen--;		//	A	uint8		Accumulator
			fread(&CPU.X,1,1,in);		clen--;		//	X	uint8		X register
			fread(&CPU.Y,1,1,in);		clen--;		//	Y	uint8		Y register
			fread(&CPU.SP,1,1,in);		clen--;		//	SP	uint8		Stack pointer
			fread(&CPU.P,1,1,in);		clen--;		//	P	uint8		Processor status register
			CPU_SplitFlags();
			fread(&CPU.LastRead,1,1,in);	clen--;		//	BUS	uint8		Last contents of data bus
			fread(&CPU.WantNMI,1,1,in);	clen--;		//	NMI	uint8		TRUE if falling edge detected on /NMI
			fread(&CPU.WantIRQ,1,1,in);	clen--;		//	IRQ	uint8		Flags 1=FRAME, 2=DPCM, 4=EXTERNAL
			fread(CPU_RAM,1,0x800,in);	clen -= 0x800;	//	RAM	uint8[0x800]	2KB work RAM
		}
		else if (!strncmp(csig,"PPUS",4))
		{
			fread(PPU_VRAM,1,0x1000,in);	clen -= 0x1000;	//	NTAR	uint8[0x1000]	4 KB of name/attribute table RAM
			fread(PPU.Sprite,1,0x100,in);	clen -= 0x100;	//	SPRA	uint8[0x100]	256 bytes of sprite RAM
			fread(PPU.Palette,1,0x20,in);	clen -= 0x20;	//	PRAM	uint8[0x20]	32 bytes of palette index RAM
			fread(&PPU.Reg2000,1,1,in);	clen--;		//	R2000	uint8		Last value written to $2000
			fread(&PPU.Reg2001,1,1,in);	clen--;		//	R2001	uint8		Last value written to $2001
			fread(&PPU.Reg2002,1,1,in);	clen--;		//	R2002	uint8		Current contents of $2002
			fread(&PPU.SprAddr,1,1,in);	clen--;		//	SPADR	uint8		SPR-RAM Address ($2003)

			fread(&PPU.IntX,1,1,in);	clen--;		//	XOFF	uint8		Tile X-offset.

			fread(&PPU.HVTog,1,1,in);	clen--;		//	VTOG	uint8		Toggle used by $2005 and $2006.
			fread(&tps,2,1,in);		clen -= 2;	//	RADD	uint16		VRAM Address
			PPU.VRAMAddr = tps;
			fread(&tps,2,1,in);		clen -= 2;	//	TADD	uint16		VRAM Address Latch
			PPU.IntReg = tps;
			fread(&PPU.ppuLatch,1,1,in);	clen--;		//	VBUF	uint8		VRAM Read Buffer
			fread(&PPU.ppuLatch,1,1,in);	clen--;		//	PGEN	uint8		PPU "general" latch.  See Ki's document. ***CHECKME***

			fread(&tps,2,1,in);		clen -= 2;	//	TICKS	uint16		Clock Ticks (0..340)
			PPU.Clockticks = tps;
			fread(&tps,2,1,in);		clen -= 2;	//	SLNUM	uint16		Scanline number
			PPU.SLnum = tps;
			fread(&PPU.ShortSL,1,1,in);	clen--;		//	SHORT	uint8		Short frame (last scanline 1 clock tick shorter)

			fread(&tps,2,1,in);		clen -= 2;	//	IOADD	uint16		External I/O Address
			PPU.IOAddr = tps;
			fread(&PPU.IOVal,1,1,in);	clen--;		//	IOVAL	uint8		External I/O Value
			fread(&PPU.IOMode,1,1,in);	clen--;		//	IOMOD	uint8		External I/O Mode (0=none, 1=renderer, 2=r2007, 3=w2007)

			fread(&PPU.IsPAL,1,1,in);	clen--;		//	NTSCP	uint8		0 for NTSC, 1 for PAL

			PPU.IsRendering = PPU.OnScreen = FALSE;
			PPU.ColorEmphasis = (PPU.Reg2001 & 0xE0) << 1;
			NES_SetCPUMode(PPU.IsPAL);
		}
		else if (!strncmp(csig,"APUS",4))
		{
			int i;
			fread(APU.Regs,1,0x17,in);	clen -= 0x17;
			APU_WriteReg(0x15,APU.Regs[0x15]);
			APU_WriteReg(0x17,APU.Regs[0x14]);
			for (i = 0; i < 0x14; i++)
				APU_WriteReg(i,APU.Regs[i]);
		}
		else if (!strncmp(csig,"NPRA",4))
		{
			memset(PRG_RAM,0,sizeof(PRG_RAM));
			fread(PRG_RAM,1,clen,in);	clen = 0;
		}
		else if (!strncmp(csig,"NCRA",4))
		{
			memset(CHR_RAM,0,sizeof(CHR_RAM));
			fread(CHR_RAM,1,clen,in);	clen = 0;
		}
		else if (!strncmp(csig,"MAPR",4))
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
		}
		if (clen != 0)	SSOK = FALSE;
		if (flen > 0)
		{
			fread(&csig,1,4,in);	flen -= 4;
			fread(&clen,4,1,in);	flen -= 4;
		}
	}
	APU_WriteReg(0x015,0x0F);
	fclose(in);
	if (SSOK)
		GFX_ShowText("State loaded: %i", States.SelSlot);
	else	GFX_ShowText("State loaded with errors: %i", States.SelSlot);
}
