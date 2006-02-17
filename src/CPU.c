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

#ifdef NSFPLAYER
# include "in_nintendulator.h"
# include "CPU.h"
# include "APU.h"
#else
# include "Nintendulator.h"
# include "NES.h"
# include "CPU.h"
# include "PPU.h"
# include "APU.h"
# include "GFX.h"
# include "Controllers.h"
#endif

struct tCPU CPU;
unsigned char CPU_RAM[0x800];

union SplitReg _CalcAddr;
union SplitReg _TmpAddr;
signed char BranchOffset;

#define CalcAddr _CalcAddr.Full
#define CalcAddrL _CalcAddr.Segment[0]
#define CalcAddrH _CalcAddr.Segment[1]

#define TmpAddr _TmpAddr.Full
#define TmpAddrL _TmpAddr.Segment[0]
#define TmpAddrH _TmpAddr.Segment[1]

unsigned char TmpData;

unsigned char Opcode;
unsigned long OpAddr;

static	void	(_MAPINT *CPU_CPUCycle)		(void);
void	_MAPINT	CPU_NoCPUCycle (void) { }

static	BOOL LastNMI;
static	BOOL LastIRQ;

static	__forceinline void	RunCycle (void)
{
#ifndef NSFPLAYER
	LastNMI = CPU.WantNMI;
#endif
	LastIRQ = CPU.WantIRQ && !CPU.FI;
	CPU_CPUCycle();
#ifndef	CPU_BENCHMARK
#ifndef NSFPLAYER
	PPU_Run();
#endif
	APU_Run();
#endif
}
#define	CPU_MemGetCode	CPU_MemGet
unsigned char __fastcall	CPU_MemGet (unsigned int Addy)
{
	static int buf;
	RunCycle();
	if (CPU.ReadHandler[(Addy >> 12) & 0xF] == CPU_ReadPRG)
	{
		if (CPU.Readable[(Addy >> 12) & 0xF])
			buf = CPU.PRGPointer[(Addy >> 12) & 0xF][Addy & 0xFFF];
		else	buf = -1;
		if (buf != -1)
			CPU.LastRead = buf;
		return CPU.LastRead;
	}
	buf = CPU.ReadHandler[(Addy >> 12) & 0xF]((Addy >> 12) & 0xF,Addy & 0xFFF);
	if (buf != -1)
		CPU.LastRead = buf;
	return CPU.LastRead;
}
void __fastcall	CPU_MemSet (unsigned int Addy, unsigned char Val)
{
	RunCycle();
	CPU.WriteHandler[(Addy >> 12) & 0xF]((Addy >> 12) & 0xF,Addy & 0xFFF,Val);
}
static	__forceinline void	Push (unsigned char Val)
{
	CPU_MemSet(0x100 | CPU.SP--,Val);
}
static	__forceinline unsigned char	Pull (void)
{
	return CPU_MemGet(0x100 | ++CPU.SP);
}
void	CPU_JoinFlags (void)
{
	CPU.P = 0x20;
	if (CPU.FC) CPU.P |= 0x01;
	if (CPU.FZ) CPU.P |= 0x02;
	if (CPU.FI) CPU.P |= 0x04;
	if (CPU.FD) CPU.P |= 0x08;
	if (CPU.FV) CPU.P |= 0x40;
	if (CPU.FN) CPU.P |= 0x80;
}
void	CPU_SplitFlags (void)
{
	__asm
	{
		mov	al,CPU.P
		shr	al,1
		setc	CPU.FC
		shr	al,1
		setc	CPU.FZ
		shr	al,1
		setc	CPU.FI
		shr	al,1
		setc	CPU.FD
		shr	al,3
		setc	CPU.FV
		shr	al,1
		setc	CPU.FN
	}
}
#ifndef NSFPLAYER
static	__forceinline void	DoNMI (void)
{
	CPU_MemGetCode(CPU.PC);
	CPU_MemGetCode(CPU.PC);
	Push(CPU.PCH);
	Push(CPU.PCL);
	CPU_JoinFlags();
	Push(CPU.P & 0xEF);
	CPU.FI = 1;

	CPU.PCL = CPU_MemGet(0xFFFA);
	CPU.PCH = CPU_MemGet(0xFFFB);
}
#endif
static	__forceinline void	DoIRQ (void)
{
	CPU_MemGetCode(CPU.PC);
	CPU_MemGetCode(CPU.PC);
	Push(CPU.PCH);
	Push(CPU.PCL);
	CPU_JoinFlags();
	Push(CPU.P & 0xEF);
	CPU.FI = 1;

	CPU.PCL = CPU_MemGet(0xFFFE);
	CPU.PCH = CPU_MemGet(0xFFFF);
}

void	CPU_GetHandlers (void)
{
	if ((MI) && (MI->CPUCycle))
		CPU_CPUCycle = MI->CPUCycle;
	else	CPU_CPUCycle = CPU_NoCPUCycle;
}

void	CPU_PowerOn (void)
{
	ZeroMemory(CPU_RAM,sizeof(CPU_RAM));
	CPU.A = 0;
	CPU.X = 0;
	CPU.Y = 0;
	CPU.PC = 0;
	CPU.SP = 0;
	CPU.P = 0x20;
	CalcAddr = 0;
	TmpAddr = 0;
	CPU_SplitFlags();
	CPU_GetHandlers();
}

void	CPU_Reset (void)
{
	CPU_MemGetCode(CPU.PC);
	CPU_MemGetCode(CPU.PC);
	CPU_MemGet(0x100 | CPU.SP--);
	CPU_MemGet(0x100 | CPU.SP--);
	CPU_MemGet(0x100 | CPU.SP--);
	CPU.FI = 1;

	CPU.PCL = CPU_MemGet(0xFFFC);
	CPU.PCH = CPU_MemGet(0xFFFD);
}

int	CPU_Save (FILE *out)
{
	int clen = 0;
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
	return clen;
}

int	CPU_Load (FILE *in)
{
	int clen = 0;
	fread(&CPU.PCH,1,1,in);		clen++;		//	PCL	uint8		Program Counter, low byte
	fread(&CPU.PCL,1,1,in);		clen++;		//	PCH	uint8		Program Counter, high byte
	fread(&CPU.A,1,1,in);		clen++;		//	A	uint8		Accumulator
	fread(&CPU.X,1,1,in);		clen++;		//	X	uint8		X register
	fread(&CPU.Y,1,1,in);		clen++;		//	Y	uint8		Y register
	fread(&CPU.SP,1,1,in);		clen++;		//	SP	uint8		Stack pointer
	fread(&CPU.P,1,1,in);		clen++;		//	P	uint8		Processor status register
	CPU_SplitFlags();
	fread(&CPU.LastRead,1,1,in);	clen++;		//	BUS	uint8		Last contents of data bus
	fread(&CPU.WantNMI,1,1,in);	clen++;		//	NMI	uint8		TRUE if falling edge detected on /NMI
	fread(&CPU.WantIRQ,1,1,in);	clen++;		//	IRQ	uint8		Flags 1=FRAME, 2=DPCM, 4=EXTERNAL
	fread(CPU_RAM,1,0x800,in);	clen += 0x800;	//	RAM	uint8[0x800]	2KB work RAM
	return clen;
}

int	_MAPINT	CPU_ReadRAM (int Bank, int Addy)
{
	return CPU_RAM[Addy & 0x07FF];
}

void	_MAPINT	CPU_WriteRAM (int Bank, int Addy, int Val)
{
	CPU_RAM[Addy & 0x07FF] = Val;
}

int	_MAPINT	CPU_Read4k (int Bank, int Addy)
{
	switch (Addy)
	{
	case 0x015:	return APU_Read4015();						break;
#ifndef NSFPLAYER
	case 0x016:	return (CPU.LastRead & 0xC0) |
			(Controllers.Port1.Read(&Controllers.Port1) & 0x19) |
			(Controllers.ExpPort.Read1(&Controllers.ExpPort) & 0x1F);	break;
	case 0x017:	return (CPU.LastRead & 0xC0) |
			(Controllers.Port2.Read(&Controllers.Port2) & 0x19) |
			(Controllers.ExpPort.Read2(&Controllers.ExpPort) & 0x1F);	break;
#else
	case 0x016:
	case 0x017:	return (CPU.LastRead & 0xC0);					break;
#endif
	default:	return CPU.LastRead;						break;
	}
}

void	_MAPINT	CPU_Write4k (int Bank, int Addy, int Val)
{
	int i;
	switch (Addy)
	{
	case 0x000:case 0x001:case 0x002:case 0x003:
	case 0x004:case 0x005:case 0x006:case 0x007:
	case 0x008:case 0x009:case 0x00A:case 0x00B:
	case 0x00C:case 0x00D:case 0x00E:case 0x00F:
	case 0x010:case 0x011:case 0x012:case 0x013:
	case 0x015:case 0x017:
			APU_WriteReg(Addy,Val);	break;
	case 0x014:	for (i = 0; i < 0x100; i++)
				CPU_MemSet(0x2004,CPU_MemGet((Val << 8) | i));
						break;
#ifndef NSFPLAYER
	case 0x016:	Controllers_Write(Val);	break;
#endif
	}
}

int	_MAPINT	CPU_ReadPRG (int Bank, int Addy)
{
	if (CPU.Readable[Bank])
		return CPU.PRGPointer[Bank][Addy];
	else	return -1;
}

void	_MAPINT	CPU_WritePRG (int Bank, int Addy, int Val)
{
	if (CPU.Writable[Bank])
		CPU.PRGPointer[Bank][Addy] = Val;
}

static	__forceinline void	AM_IMP (void)
{
	CPU_MemGetCode(CPU.PC);
}
static	__forceinline void	AM_IMM (void)
{
	CalcAddr = CPU.PC++;
}
static	__forceinline void	AM_ABS (void)
{
	CalcAddrL = CPU_MemGetCode(CPU.PC++);
	CalcAddrH = CPU_MemGetCode(CPU.PC++);
}

static	__forceinline void	AM_REL (void)
{
	BranchOffset = CPU_MemGetCode(CPU.PC++);
}
static	__forceinline void	AM_ABX (void)
{
	CalcAddrL = CPU_MemGetCode(CPU.PC++);
	CalcAddrH = CPU_MemGetCode(CPU.PC++);
	__asm
	{
		mov	al,CPU.X
		add	CalcAddrL,al
		jnc	noinc
		mov	ecx,CalcAddr
		call	CPU_MemGet
		inc	CalcAddrH
noinc:
	}
}
static	__forceinline void	AM_ABXW (void)
{
	CalcAddrL = CPU_MemGetCode(CPU.PC++);
	CalcAddrH = CPU_MemGetCode(CPU.PC++);
	__asm
	{
		mov	al,CPU.X
		add	CalcAddrL,al
		jc	doinc
		mov	ecx,CalcAddr
		call	CPU_MemGet
		jmp	end
doinc:		mov	ecx,CalcAddr
		call	CPU_MemGet
		inc	CalcAddrH
end:
	}
}
static	__forceinline void	AM_ABY (void)
{
	CalcAddrL = CPU_MemGetCode(CPU.PC++);
	CalcAddrH = CPU_MemGetCode(CPU.PC++);
	__asm
	{
		mov	al,CPU.Y
		add	CalcAddrL,al
		jnc	noinc
		mov	ecx,CalcAddr
		call	CPU_MemGet
		inc	CalcAddrH
noinc:
	}
}
static	__forceinline void	AM_ABYW (void)
{
	CalcAddrL = CPU_MemGetCode(CPU.PC++);
	CalcAddrH = CPU_MemGetCode(CPU.PC++);
	__asm
	{
		mov	al,CPU.Y
		add	CalcAddrL,al
		jc	doinc
		mov	ecx,CalcAddr
		call	CPU_MemGet
		jmp	end
doinc:		mov	ecx,CalcAddr
		call	CPU_MemGet
		inc	CalcAddrH
end:
	}
}
static	__forceinline void	AM_ZPG (void)
{
	CalcAddr = CPU_MemGetCode(CPU.PC++);
}
static	__forceinline void	AM_ZPX (void)
{
	CalcAddr = CPU_MemGetCode(CPU.PC++);
	CPU_MemGet(CalcAddr);
	CalcAddrL += CPU.X;
}
static	__forceinline void	AM_ZPY (void)
{
	CalcAddr = CPU_MemGetCode(CPU.PC++);
	CPU_MemGet(CalcAddr);
	CalcAddrL += CPU.Y;
}
static	__forceinline void	AM_INX (void)
{
	TmpAddr = CPU_MemGetCode(CPU.PC++);
	CPU_MemGet(TmpAddr);
	TmpAddrL += CPU.X;
	CalcAddrL = CPU_MemGet(TmpAddr);
	TmpAddrL++;
	CalcAddrH = CPU_MemGet(TmpAddr);
}
static	__forceinline void	AM_INY (void)
{
	TmpAddr = CPU_MemGetCode(CPU.PC++);
	CalcAddrL = CPU_MemGet(TmpAddr);
	TmpAddrL++;
	CalcAddrH = CPU_MemGet(TmpAddr);
	__asm
	{
		mov	al,CPU.Y
		add	CalcAddrL,al
		jnc	noinc
		mov	ecx,CalcAddr
		call	CPU_MemGet
		inc	CalcAddrH
noinc:
	}
}
static	__forceinline void	AM_INYW (void)
{
	TmpAddr = CPU_MemGetCode(CPU.PC++);
	CalcAddrL = CPU_MemGet(TmpAddr);
	TmpAddrL++;
	CalcAddrH = CPU_MemGet(TmpAddr);
	__asm
	{
		mov	al,CPU.Y
		add	CalcAddrL,al
		jc	doinc
		mov	ecx,CalcAddr
		call	CPU_MemGet
		jmp	end
doinc:		mov	ecx,CalcAddr
		call	CPU_MemGet
		inc	CalcAddrH
end:
	}
}

static	__forceinline void	IN_UNK (void)
{
#ifndef NSFPLAYER
	EI.DbgOut("Invalid opcode $%02X encountered at $%04X",Opcode,OpAddr);
#endif
}
static	__forceinline void	IN_ADC (void)
{
	CPU_MemGet(CalcAddr);
	__asm
	{
		mov	ah,CPU.FC
		add	ah,0xFF
		mov	al,CPU.LastRead
		adc	CPU.A,al
		setc	CPU.FC
		setz	CPU.FZ
		sets	CPU.FN
		seto	CPU.FV
	}
}
static	__forceinline void	IN_AND (void)
{
	CPU_MemGet(CalcAddr);
	__asm
	{
		mov	al,CPU.LastRead
		and	CPU.A,al
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_ASL (void)
{
	TmpData = CPU_MemGet(CalcAddr);
	CPU_MemSet(CalcAddr,TmpData);
	__asm
	{
		shl	TmpData,1
		setc	CPU.FC
		setz	CPU.FZ
		sets	CPU.FN
	}
	CPU_MemSet(CalcAddr,TmpData);
}
static	__forceinline void	IN_ASL_A (void)
{
	__asm
	{
		shl	CPU.A,1
		setc	CPU.FC
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_BRANCH (BOOL Condition)
{
	if (Condition)
	{
		CPU_MemGet(CPU.PC);
		__asm
		{
			mov	al,BranchOffset
			test	al,0x80
			je	pos
			neg	al
			sub	CPU.PCL,al
			jnc	end
			mov	ecx,CPU.PC
			call	CPU_MemGet
			dec	CPU.PCH
			jmp	end
pos:			add	CPU.PCL,al
			jnc	end
			mov	ecx,CPU.PC
			call	CPU_MemGet
			inc	CPU.PCH
end:
		}
	}
}
static	__forceinline void	IN_BCS (void) {	IN_BRANCH( CPU.FC);	}
static	__forceinline void	IN_BCC (void) {	IN_BRANCH(!CPU.FC);	}
static	__forceinline void	IN_BEQ (void) {	IN_BRANCH( CPU.FZ);	}
static	__forceinline void	IN_BNE (void) {	IN_BRANCH(!CPU.FZ);	}
static	__forceinline void	IN_BMI (void) {	IN_BRANCH( CPU.FN);	}
static	__forceinline void	IN_BPL (void) {	IN_BRANCH(!CPU.FN);	}
static	__forceinline void	IN_BVS (void) {	IN_BRANCH( CPU.FV);	}
static	__forceinline void	IN_BVC (void) {	IN_BRANCH(!CPU.FV);	}
static	__forceinline void	IN_BIT (void)
{
	CPU_MemGet(CalcAddr);
	__asm
	{
		mov	al,CPU.LastRead
		mov	dl,al
		shr	al,7
		setc	CPU.FV
		shr	al,1
		setc	CPU.FN
		and	dl,CPU.A
		setz	CPU.FZ
	}
}
static	__forceinline void	IN_BRK (void)
{
	CPU_MemGet(CPU.PC);
	Push(CPU.PCH);
	Push(CPU.PCL);
	CPU_JoinFlags();
	Push(CPU.P | 0x10);
#ifndef NSFPLAYER
	if (CPU.WantNMI)
	{
		CPU.WantNMI = FALSE;
		CPU.PCL = CPU_MemGet(0xFFFC);
		CPU.PCH = CPU_MemGet(0xFFFD);
	}
	else
	{
		CPU.FI = 1;
		CPU.PCL = CPU_MemGet(0xFFFE);
		CPU.PCH = CPU_MemGet(0xFFFF);
	}
#else
	CPU.FI = 1;
	CPU.PCL = CPU_MemGet(0xFFFE);
	CPU.PCH = CPU_MemGet(0xFFFF);
#endif
}
static	__forceinline void	IN_CLC (void) {	__asm	mov CPU.FC,0	}
static	__forceinline void	IN_CLD (void) {	__asm	mov CPU.FD,0	}
static	__forceinline void	IN_CLI (void) {	__asm	mov CPU.FI,0	}
static	__forceinline void	IN_CLV (void) {	__asm	mov CPU.FV,0	}
static	__forceinline void	IN_CMP (void)
{
	CPU_MemGet(CalcAddr);
	__asm
	{
		mov	al,CPU.A
		sub	al,CPU.LastRead
		setnc	CPU.FC
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_CPX (void)
{
	CPU_MemGet(CalcAddr);
	__asm
	{
		mov	al,CPU.X
		sub	al,CPU.LastRead
		setnc	CPU.FC
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_CPY (void)
{
	CPU_MemGet(CalcAddr);
	__asm
	{
		mov	al,CPU.Y
		sub	al,CPU.LastRead
		setnc	CPU.FC
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_DEC (void)
{
	TmpData = CPU_MemGet(CalcAddr);
	CPU_MemSet(CalcAddr,TmpData);
	__asm
	{
		dec	TmpData
		setz	CPU.FZ
		sets	CPU.FN
	}
	CPU_MemSet(CalcAddr,TmpData);
}
static	__forceinline void	IN_DEX (void)
{
	__asm
	{
		dec	CPU.X
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_DEY (void)
{
	__asm
	{
		dec	CPU.Y
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_EOR (void)
{
	CPU_MemGet(CalcAddr);
	__asm
	{
		mov	al,CPU.LastRead
		xor	CPU.A,al
		setz	CPU.FZ
		sets	CPU.FN
 	}
}
static	__forceinline void	IN_HLT (void)
{
#ifndef NSFPLAYER
	EI.DbgOut("Invalid opcode $%02X encountered at $%04X; CPU locked",Opcode,OpAddr);
	MessageBox(mWnd, "Bad opcode, CPU locked", "Nintendulator", MB_OK);
	NES.Stop = TRUE;
#endif
}
static	__forceinline void	IN_INC (void)
{
	TmpData = CPU_MemGet(CalcAddr);
	CPU_MemSet(CalcAddr,TmpData);
	__asm
	{
		inc	TmpData
		setz	CPU.FZ
		sets	CPU.FN
	}
	CPU_MemSet(CalcAddr,TmpData);
}
static	__forceinline void	IN_INX (void)
{
	__asm
	{
		inc	CPU.X
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_INY (void)
{
	__asm
	{
		inc	CPU.Y
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_JMP (void)
{
	CPU.PC = CalcAddr;
}
static	__forceinline void	IN_JMP_I (void)
{
	CPU.PCL = CPU_MemGet(CalcAddr);
	CalcAddrL++;
	CPU.PCH = CPU_MemGet(CalcAddr);
}
static	__forceinline void	IN_JSR (void)
{
	TmpAddrL = CPU_MemGetCode(CPU.PC++);
	CPU_MemGet(0x100 | CPU.SP);
	Push(CPU.PCH);
	Push(CPU.PCL);
	CPU.PCH = CPU_MemGetCode(CPU.PC);
	CPU.PCL = TmpAddrL;
}
static	__forceinline void	IN_LDA (void)
{
	CPU.A = CPU_MemGet(CalcAddr);
	__asm
	{
		test	CPU.A,0xFF
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_LDX (void)
{
	CPU.X = CPU_MemGet(CalcAddr);
	__asm
	{
		test	CPU.X,0xFF
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_LDY (void)
{
	CPU.Y = CPU_MemGet(CalcAddr);
	__asm
	{
		test	CPU.Y,0xFF
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_LSR (void)
{
	TmpData = CPU_MemGet(CalcAddr);
	CPU_MemSet(CalcAddr,TmpData);
	__asm
	{
		shr	TmpData,1
		setc	CPU.FC
		mov	CPU.FN,0
		setz	CPU.FZ
	}
	CPU_MemSet(CalcAddr,TmpData);
}
static	__forceinline void	IN_LSR_A (void)
{
	__asm
	{
		shr	CPU.A,1
		setc	CPU.FC
		mov	CPU.FN,0
		setz	CPU.FZ
	}
}
static	__forceinline void	IN_NOP (void)
{
}
static	__forceinline void	IN_ORA (void)
{
	CPU_MemGet(CalcAddr);
	__asm
	{
		mov	al,CPU.LastRead
		or	CPU.A,al
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_PHA (void)
{
	Push(CPU.A);
}
static	__forceinline void	IN_PHP (void)
{
	CPU_JoinFlags();
	Push(CPU.P | 0x10);
}
static	__forceinline void	IN_PLA (void)
{
	CPU_MemGet(0x100 | CPU.SP);
	CPU.A = Pull();
	__asm
	{
		test	CPU.A,0xFF
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_PLP (void)
{
	CPU_MemGet(0x100 | CPU.SP);
	CPU.P = Pull();
	CPU_SplitFlags();
}
static	__forceinline void	IN_ROL (void)
{
	TmpData = CPU_MemGet(CalcAddr);
	CPU_MemSet(CalcAddr,TmpData);
	__asm
	{
		mov	al,CPU.FC
		add	al,0xFF
		rcl	TmpData,1
		setc	CPU.FC
		test	TmpData,0xFF
		setz	CPU.FZ
		sets	CPU.FN
	}
	CPU_MemSet(CalcAddr,TmpData);
}
static	__forceinline void	IN_ROL_A (void)
{
	__asm
	{
		mov	al,CPU.FC
		add	al,0xFF
		rcl	CPU.A,1
		setc	CPU.FC
		test	CPU.A,0xFF
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_ROR (void)
{
	TmpData = CPU_MemGet(CalcAddr);
	CPU_MemSet(CalcAddr,TmpData);
	__asm
	{
		mov	al,CPU.FC
		mov	CPU.FN,al
		add	al,0xFF
		rcr	TmpData,1
		setc	CPU.FC
		test	TmpData,0xFF
		setz	CPU.FZ
	}
	CPU_MemSet(CalcAddr,TmpData);
}
static	__forceinline void	IN_ROR_A (void)
{
	__asm
	{
		mov	al,CPU.FC
		mov	CPU.FN,al
		add	al,0xFF
		rcr	CPU.A,1
		setc	CPU.FC
		test	CPU.A,0xFF
		setz	CPU.FZ
	}
}
static	__forceinline void	IN_RTI (void)
{
	CPU_MemGet(0x100 | CPU.SP);
	CPU.P = Pull();
	CPU_SplitFlags();
	CPU.PCL = Pull();
	CPU.PCH = Pull();
}
static	__forceinline void	IN_RTS (void)
{
	CPU_MemGet(0x100 | CPU.SP);
	CPU.PCL = Pull();
	CPU.PCH = Pull();
	CPU_MemGet(CPU.PC++);
}
static	__forceinline void	IN_SBC (void)
{
	CPU_MemGet(CalcAddr);
	__asm
	{
		mov	ah,CPU.FC
		sub	ah,1
		mov	al,CPU.LastRead
		sbb	CPU.A,al
		setnc	CPU.FC
		setz	CPU.FZ
		sets	CPU.FN
		seto	CPU.FV
	}
}
static	__forceinline void	IN_SEC (void) {	__asm	mov CPU.FC,1	}
static	__forceinline void	IN_SED (void) {	__asm	mov CPU.FD,1	}
static	__forceinline void	IN_SEI (void) {	__asm	mov CPU.FI,1	}
static	__forceinline void	IN_STA (void)
{
	CPU_MemSet(CalcAddr,CPU.A);
}
static	__forceinline void	IN_STX (void)
{
	CPU_MemSet(CalcAddr,CPU.X);
}
static	__forceinline void	IN_STY (void)
{
	CPU_MemSet(CalcAddr,CPU.Y);
}
static	__forceinline void	IN_TAX (void)
{
	__asm
	{
		mov	al,CPU.A
		mov	CPU.X,al
		test	al,al
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_TAY (void)
{
	__asm
	{
		mov	al,CPU.A
		mov	CPU.Y,al
		test	al,al
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_TSX (void)
{
	__asm
	{
		mov	al,CPU.SP
		mov	CPU.X,al
		test	al,al
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_TXA (void)
{
	__asm
	{
		mov	al,CPU.X
		mov	CPU.A,al
		test	al,al
		setz	CPU.FZ
		sets	CPU.FN
	}
}
static	__forceinline void	IN_TXS (void)
{
	__asm
	{
		mov	al,CPU.X
		mov	CPU.SP,al
	}
}
static	__forceinline void	IN_TYA (void)
{
	__asm
	{
		mov	al,CPU.Y
		mov	CPU.A,al
		test	al,al
		setz	CPU.FZ
		sets	CPU.FN
	}
}
extern	void	DPCM_Fetch (void);
void	CPU_ExecOp (void)
{
	Opcode = CPU_MemGetCode(OpAddr = CPU.PC++);
	switch (Opcode)
	{
case 0x00:AM_IMM();  IN_BRK();break;case 0x10:AM_REL();  IN_BPL();break;case 0x08:AM_IMP();  IN_PHP();break;case 0x18:AM_IMP();  IN_CLC();break;case 0x04:AM_ZPG();  IN_UNK();break;case 0x14:AM_ZPX();  IN_UNK();break;case 0x0C:AM_ABS();  IN_UNK();break;case 0x1C:AM_ABX();  IN_UNK();break;
case 0x20:           IN_JSR();break;case 0x30:AM_REL();  IN_BMI();break;case 0x28:AM_IMP();  IN_PLP();break;case 0x38:AM_IMP();  IN_SEC();break;case 0x24:AM_ZPG();  IN_BIT();break;case 0x34:AM_ZPX();  IN_UNK();break;case 0x2C:AM_ABS();  IN_BIT();break;case 0x3C:AM_ABX();  IN_UNK();break;
case 0x40:AM_IMP();  IN_RTI();break;case 0x50:AM_REL();  IN_BVC();break;case 0x48:AM_IMP();  IN_PHA();break;case 0x58:AM_IMP();  IN_CLI();break;case 0x44:AM_ZPG();  IN_UNK();break;case 0x54:AM_ZPX();  IN_UNK();break;case 0x4C:AM_ABS();  IN_JMP();break;case 0x5C:AM_ABX();  IN_UNK();break;
case 0x60:AM_IMP();  IN_RTS();break;case 0x70:AM_REL();  IN_BVS();break;case 0x68:AM_IMP();  IN_PLA();break;case 0x78:AM_IMP();  IN_SEI();break;case 0x64:AM_ZPG();  IN_UNK();break;case 0x74:AM_ZPX();  IN_UNK();break;case 0x6C:AM_ABS();IN_JMP_I();break;case 0x7C:AM_ABX();  IN_UNK();break;
case 0x80:AM_IMM();  IN_UNK();break;case 0x90:AM_REL();  IN_BCC();break;case 0x88:AM_IMP();  IN_DEY();break;case 0x98:AM_IMP();  IN_TYA();break;case 0x84:AM_ZPG();  IN_STY();break;case 0x94:AM_ZPX();  IN_STY();break;case 0x8C:AM_ABS();  IN_STY();break;case 0x9C:AM_ABX();  IN_UNK();break;
case 0xA0:AM_IMM();  IN_LDY();break;case 0xB0:AM_REL();  IN_BCS();break;case 0xA8:AM_IMP();  IN_TAY();break;case 0xB8:AM_IMP();  IN_CLV();break;case 0xA4:AM_ZPG();  IN_LDY();break;case 0xB4:AM_ZPX();  IN_LDY();break;case 0xAC:AM_ABS();  IN_LDY();break;case 0xBC:AM_ABX();  IN_LDY();break;
case 0xC0:AM_IMM();  IN_CPY();break;case 0xD0:AM_REL();  IN_BNE();break;case 0xC8:AM_IMP();  IN_INY();break;case 0xD8:AM_IMP();  IN_CLD();break;case 0xC4:AM_ZPG();  IN_CPY();break;case 0xD4:AM_ZPX();  IN_UNK();break;case 0xCC:AM_ABS();  IN_CPY();break;case 0xDC:AM_ABX();  IN_UNK();break;
case 0xE0:AM_IMM();  IN_CPX();break;case 0xF0:AM_REL();  IN_BEQ();break;case 0xE8:AM_IMP();  IN_INX();break;case 0xF8:AM_IMP();  IN_SED();break;case 0xE4:AM_ZPG();  IN_CPX();break;case 0xF4:AM_ZPX();  IN_UNK();break;case 0xEC:AM_ABS();  IN_CPX();break;case 0xFC:AM_ABX();  IN_UNK();break;

case 0x02:           IN_HLT();break;case 0x12:           IN_HLT();break;case 0x0A:AM_IMP();IN_ASL_A();break;case 0x1A:AM_IMP();  IN_UNK();break;case 0x06:AM_ZPG();  IN_ASL();break;case 0x16:AM_ZPX();  IN_ASL();break;case 0x0E:AM_ABS();  IN_ASL();break;case 0x1E:AM_ABXW(); IN_ASL();break;
case 0x22:           IN_HLT();break;case 0x32:           IN_HLT();break;case 0x2A:AM_IMP();IN_ROL_A();break;case 0x3A:AM_IMP();  IN_UNK();break;case 0x26:AM_ZPG();  IN_ROL();break;case 0x36:AM_ZPX();  IN_ROL();break;case 0x2E:AM_ABS();  IN_ROL();break;case 0x3E:AM_ABXW(); IN_ROL();break;
case 0x42:           IN_HLT();break;case 0x52:           IN_HLT();break;case 0x4A:AM_IMP();IN_LSR_A();break;case 0x5A:AM_IMP();  IN_UNK();break;case 0x46:AM_ZPG();  IN_LSR();break;case 0x56:AM_ZPX();  IN_LSR();break;case 0x4E:AM_ABS();  IN_LSR();break;case 0x5E:AM_ABXW(); IN_LSR();break;
case 0x62:           IN_HLT();break;case 0x72:           IN_HLT();break;case 0x6A:AM_IMP();IN_ROR_A();break;case 0x7A:AM_IMP();  IN_UNK();break;case 0x66:AM_ZPG();  IN_ROR();break;case 0x76:AM_ZPX();  IN_ROR();break;case 0x6E:AM_ABS();  IN_ROR();break;case 0x7E:AM_ABXW(); IN_ROR();break;
case 0x82:AM_IMM();  IN_UNK();break;case 0x92:           IN_HLT();break;case 0x8A:AM_IMP();  IN_TXA();break;case 0x9A:AM_IMP();  IN_TXS();break;case 0x86:AM_ZPG();  IN_STX();break;case 0x96:AM_ZPY();  IN_STX();break;case 0x8E:AM_ABS();  IN_STX();break;case 0x9E:AM_ABY();  IN_UNK();break;
case 0xA2:AM_IMM();  IN_LDX();break;case 0xB2:           IN_HLT();break;case 0xAA:AM_IMP();  IN_TAX();break;case 0xBA:AM_IMP();  IN_TSX();break;case 0xA6:AM_ZPG();  IN_LDX();break;case 0xB6:AM_ZPY();  IN_LDX();break;case 0xAE:AM_ABS();  IN_LDX();break;case 0xBE:AM_ABY();  IN_LDX();break;
case 0xC2:AM_IMM();  IN_UNK();break;case 0xD2:           IN_HLT();break;case 0xCA:AM_IMP();  IN_DEX();break;case 0xDA:AM_IMP();  IN_UNK();break;case 0xC6:AM_ZPG();  IN_DEC();break;case 0xD6:AM_ZPX();  IN_DEC();break;case 0xCE:AM_ABS();  IN_DEC();break;case 0xDE:AM_ABXW(); IN_DEC();break;
case 0xE2:AM_IMM();  IN_UNK();break;case 0xF2:           IN_HLT();break;case 0xEA:AM_IMP();  IN_NOP();break;case 0xFA:AM_IMP();  IN_UNK();break;case 0xE6:AM_ZPG();  IN_INC();break;case 0xF6:AM_ZPX();  IN_INC();break;case 0xEE:AM_ABS();  IN_INC();break;case 0xFE:AM_ABXW(); IN_INC();break;

case 0x01:AM_INX();  IN_ORA();break;case 0x11:AM_INY();  IN_ORA();break;case 0x09:AM_IMM();  IN_ORA();break;case 0x19:AM_ABY();  IN_ORA();break;case 0x05:AM_ZPG();  IN_ORA();break;case 0x15:AM_ZPX();  IN_ORA();break;case 0x0D:AM_ABS();  IN_ORA();break;case 0x1D:AM_ABX();  IN_ORA();break;
case 0x21:AM_INX();  IN_AND();break;case 0x31:AM_INY();  IN_AND();break;case 0x29:AM_IMM();  IN_AND();break;case 0x39:AM_ABY();  IN_AND();break;case 0x25:AM_ZPG();  IN_AND();break;case 0x35:AM_ZPX();  IN_AND();break;case 0x2D:AM_ABS();  IN_AND();break;case 0x3D:AM_ABX();  IN_AND();break;
case 0x41:AM_INX();  IN_EOR();break;case 0x51:AM_INY();  IN_EOR();break;case 0x49:AM_IMM();  IN_EOR();break;case 0x59:AM_ABY();  IN_EOR();break;case 0x45:AM_ZPG();  IN_EOR();break;case 0x55:AM_ZPX();  IN_EOR();break;case 0x4D:AM_ABS();  IN_EOR();break;case 0x5D:AM_ABX();  IN_EOR();break;
case 0x61:AM_INX();  IN_ADC();break;case 0x71:AM_INY();  IN_ADC();break;case 0x69:AM_IMM();  IN_ADC();break;case 0x79:AM_ABY();  IN_ADC();break;case 0x65:AM_ZPG();  IN_ADC();break;case 0x75:AM_ZPX();  IN_ADC();break;case 0x6D:AM_ABS();  IN_ADC();break;case 0x7D:AM_ABX();  IN_ADC();break;
case 0x81:AM_INX();  IN_STA();break;case 0x91:AM_INYW(); IN_STA();break;case 0x89:AM_IMM();  IN_UNK();break;case 0x99:AM_ABYW(); IN_STA();break;case 0x85:AM_ZPG();  IN_STA();break;case 0x95:AM_ZPX();  IN_STA();break;case 0x8D:AM_ABS();  IN_STA();break;case 0x9D:AM_ABXW(); IN_STA();break;
case 0xA1:AM_INX();  IN_LDA();break;case 0xB1:AM_INY();  IN_LDA();break;case 0xA9:AM_IMM();  IN_LDA();break;case 0xB9:AM_ABY();  IN_LDA();break;case 0xA5:AM_ZPG();  IN_LDA();break;case 0xB5:AM_ZPX();  IN_LDA();break;case 0xAD:AM_ABS();  IN_LDA();break;case 0xBD:AM_ABX();  IN_LDA();break;
case 0xC1:AM_INX();  IN_CMP();break;case 0xD1:AM_INY();  IN_CMP();break;case 0xC9:AM_IMM();  IN_CMP();break;case 0xD9:AM_ABY();  IN_CMP();break;case 0xC5:AM_ZPG();  IN_CMP();break;case 0xD5:AM_ZPX();  IN_CMP();break;case 0xCD:AM_ABS();  IN_CMP();break;case 0xDD:AM_ABX();  IN_CMP();break;
case 0xE1:AM_INX();  IN_SBC();break;case 0xF1:AM_INY();  IN_SBC();break;case 0xE9:AM_IMM();  IN_SBC();break;case 0xF9:AM_ABY();  IN_SBC();break;case 0xE5:AM_ZPG();  IN_SBC();break;case 0xF5:AM_ZPX();  IN_SBC();break;case 0xED:AM_ABS();  IN_SBC();break;case 0xFD:AM_ABX();  IN_SBC();break;

case 0x03:AM_INX();  IN_UNK();break;case 0x13:AM_INY();  IN_UNK();break;case 0x0B:AM_IMM();  IN_UNK();break;case 0x1B:AM_ABY();  IN_UNK();break;case 0x07:AM_ZPG();  IN_UNK();break;case 0x17:AM_ZPX();  IN_UNK();break;case 0x0F:AM_ABS();  IN_UNK();break;case 0x1F:AM_ABX();  IN_UNK();break;
case 0x23:AM_INX();  IN_UNK();break;case 0x33:AM_INY();  IN_UNK();break;case 0x2B:AM_IMM();  IN_UNK();break;case 0x3B:AM_ABY();  IN_UNK();break;case 0x27:AM_ZPG();  IN_UNK();break;case 0x37:AM_ZPX();  IN_UNK();break;case 0x2F:AM_ABS();  IN_UNK();break;case 0x3F:AM_ABX();  IN_UNK();break;
case 0x43:AM_INX();  IN_UNK();break;case 0x53:AM_INY();  IN_UNK();break;case 0x4B:AM_IMM();  IN_UNK();break;case 0x5B:AM_ABY();  IN_UNK();break;case 0x47:AM_ZPG();  IN_UNK();break;case 0x57:AM_ZPX();  IN_UNK();break;case 0x4F:AM_ABS();  IN_UNK();break;case 0x5F:AM_ABX();  IN_UNK();break;
case 0x63:AM_INX();  IN_UNK();break;case 0x73:AM_INY();  IN_UNK();break;case 0x6B:AM_IMM();  IN_UNK();break;case 0x7B:AM_ABY();  IN_UNK();break;case 0x67:AM_ZPG();  IN_UNK();break;case 0x77:AM_ZPX();  IN_UNK();break;case 0x6F:AM_ABS();  IN_UNK();break;case 0x7F:AM_ABX();  IN_UNK();break;
case 0x83:AM_INX();  IN_UNK();break;case 0x93:AM_INY();  IN_UNK();break;case 0x8B:AM_IMM();  IN_UNK();break;case 0x9B:AM_ABY();  IN_UNK();break;case 0x87:AM_ZPG();  IN_UNK();break;case 0x97:AM_ZPY();  IN_UNK();break;case 0x8F:AM_ABS();  IN_UNK();break;case 0x9F:AM_ABY();  IN_UNK();break;
case 0xA3:AM_INX();  IN_UNK();break;case 0xB3:AM_INY();  IN_UNK();break;case 0xAB:AM_IMM();  IN_UNK();break;case 0xBB:AM_ABY();  IN_UNK();break;case 0xA7:AM_ZPG();  IN_UNK();break;case 0xB7:AM_ZPY();  IN_UNK();break;case 0xAF:AM_ABS();  IN_UNK();break;case 0xBF:AM_ABY();  IN_UNK();break;
case 0xC3:AM_INX();  IN_UNK();break;case 0xD3:AM_INY();  IN_UNK();break;case 0xCB:AM_IMM();  IN_UNK();break;case 0xDB:AM_ABY();  IN_UNK();break;case 0xC7:AM_ZPG();  IN_UNK();break;case 0xD7:AM_ZPX();  IN_UNK();break;case 0xCF:AM_ABS();  IN_UNK();break;case 0xDF:AM_ABX();  IN_UNK();break;
case 0xE3:AM_INX();  IN_UNK();break;case 0xF3:AM_INY();  IN_UNK();break;case 0xEB:AM_IMM();  IN_UNK();break;case 0xFB:AM_ABY();  IN_UNK();break;case 0xE7:AM_ZPG();  IN_UNK();break;case 0xF7:AM_ZPX();  IN_UNK();break;case 0xEF:AM_ABS();  IN_UNK();break;case 0xFF:AM_ABX();  IN_UNK();break;
	}
	DPCM_Fetch();

#ifndef NSFPLAYER
	if (LastNMI)
	{
		DoNMI();
		CPU.WantNMI = FALSE;
	}
	else
#endif
	if (LastIRQ)
		DoIRQ();
}