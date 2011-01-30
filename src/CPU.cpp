/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifdef	NSFPLAYER
# include "in_nintendulator.h"
# include "MapperInterface.h"
# include "CPU.h"
# include "APU.h"
#else	/* !NSFPLAYER */
# include "stdafx.h"
# include "Nintendulator.h"
# include "MapperInterface.h"
# include "NES.h"
# include "CPU.h"
# include "PPU.h"
# include "APU.h"
# include "Controllers.h"
#endif	/* NSFPLAYER */

namespace CPU
{
FCPURead	ReadHandler[0x10];
FCPUWrite	WriteHandler[0x10];
unsigned char *	PRGPointer[0x10];
BOOL	Readable[0x10], Writable[0x10];

#ifndef	NSFPLAYER
unsigned char WantNMI;
#endif	/* !NSFPLAYER */
unsigned char WantIRQ;
unsigned char PCMCycles;
#ifdef	ENABLE_DEBUGGER
unsigned char GotInterrupt;
#endif	/* ENABLE_DEBUGGER */

unsigned char A, X, Y, SP, P;
bool FC, FZ, FI, FD, FV, FN;
unsigned char LastRead;
SplitReg rPC;
unsigned char RAM[0x800];

BOOL LogBadOps;

SplitReg rCalcAddr;
SplitReg rTmpAddr;
unsigned char BranchOffset;

#define CalcAddr rCalcAddr.Full
#define CalcAddrL rCalcAddr.Segment[0]
#define CalcAddrH rCalcAddr.Segment[1]

#define TmpAddr rTmpAddr.Full
#define TmpAddrL rTmpAddr.Segment[0]
#define TmpAddrH rTmpAddr.Segment[1]

unsigned char Opcode;
unsigned short OpAddr;

void	(MAPINT *CPUCycle)		(void);
void	MAPINT	NoCPUCycle (void) { }

BOOL LastNMI;
BOOL LastIRQ;

inline	void	RunCycle (void)
{
#ifndef	NSFPLAYER
	LastNMI = WantNMI;
#endif	/* !NSFPLAYER */
	LastIRQ = WantIRQ && !FI;
	CPUCycle();
#ifndef	CPU_BENCHMARK
#ifndef	NSFPLAYER
	PPU::Run();
#endif	/* !NSFPLAYER */
	APU::Run();
#endif	/* !CPU_BENCHMARK */
}
#define	MemGetCode	MemGet
unsigned char	MemGet (unsigned int Addr)
{
	int buf;

	if (PCMCycles)
	{
		// zero out PCMCycles so we don't recurse back into this case during the following fetches
		int _PCMCycles = PCMCycles - 1;
		PCMCycles = 0;
		if (((Addr == 0x4016) || (Addr == 0x4017)))
		{
			// Consecutive controller port reads from this are treated as one
			if (_PCMCycles--)
				MemGet(Addr);
			while (_PCMCycles--)
				RunCycle();
		}
		else
		{
			// but other addresses see multiple reads as expected
			while (_PCMCycles--)
				MemGet(Addr);
		}
		APU::DPCM::Fetch();
	}

	RunCycle();
	// avoid a function call if possible - if it's ReadPRG, then run it inline
	if (ReadHandler[(Addr >> 12) & 0xF] == ReadPRG)
	{
		if (Readable[(Addr >> 12) & 0xF])
			buf = PRGPointer[(Addr >> 12) & 0xF][Addr & 0xFFF];
		else	buf = -1;
	}
	else	buf = ReadHandler[(Addr >> 12) & 0xF]((Addr >> 12) & 0xF, Addr & 0xFFF);
	if (buf != -1)
		LastRead = (unsigned char)buf;
	return LastRead;
}
void	MemSet (unsigned int Addr, unsigned char Val)
{
	if (PCMCycles)
		PCMCycles--;
	RunCycle();
	WriteHandler[(Addr >> 12) & 0xF]((Addr >> 12) & 0xF, Addr & 0xFFF, Val);
}
__forceinline	void	Push (unsigned char Val)
{
	MemSet(0x100 | SP--, Val);
}
__forceinline	unsigned char	Pull (void)
{
	return MemGet(0x100 | ++SP);
}
void	JoinFlags (void)
{
	P = 0x20;
	if (FC) P |= 0x01;
	if (FZ) P |= 0x02;
	if (FI) P |= 0x04;
	if (FD) P |= 0x08;
	if (FV) P |= 0x40;
	if (FN) P |= 0x80;
}
void	SplitFlags (void)
{
	FC = (P >> 0) & 1;
	FZ = (P >> 1) & 1;
	FI = (P >> 2) & 1;
	FD = (P >> 3) & 1;
	FV = (P >> 6) & 1;
	FN = (P >> 7) & 1;
}
#ifndef	NSFPLAYER
void	DoNMI (void)
{
	MemGetCode(PC);
	MemGetCode(PC);
	Push(PCH);
	Push(PCL);
	JoinFlags();
	Push(P);
	FI = 1;

	PCL = MemGet(0xFFFA);
	PCH = MemGet(0xFFFB);
#ifdef	ENABLE_DEBUGGER
	GotInterrupt = INTERRUPT_NMI;
#endif	/* ENABLE_DEBUGGER */
}
#endif	/* !NSFPLAYER */
void	DoIRQ (void)
{
	MemGetCode(PC);
	MemGetCode(PC);
	Push(PCH);
	Push(PCL);
	JoinFlags();
	Push(P);
	FI = 1;
#ifndef	NSFPLAYER
	if (WantNMI)
	{
		WantNMI = FALSE;
		PCL = MemGet(0xFFFA);
		PCH = MemGet(0xFFFB);
	}
	else
	{
		PCL = MemGet(0xFFFE);
		PCH = MemGet(0xFFFF);
	}
#else	/* NSFPLAYER */
	PCL = MemGet(0xFFFE);
	PCH = MemGet(0xFFFF);
#endif	/* !NSFPLAYER */
#ifdef	ENABLE_DEBUGGER
	GotInterrupt = INTERRUPT_IRQ;
#endif	/* ENABLE_DEBUGGER */
}

void	GetHandlers (void)
{
	if ((MI) && (MI->CPUCycle))
		CPUCycle = MI->CPUCycle;
	else	CPUCycle = NoCPUCycle;
}

void	PowerOn (void)
{
	ZeroMemory(RAM, sizeof(RAM));
	A = 0;
	X = 0;
	Y = 0;
	PC = 0;
	SP = 0;
	P = 0x20;
	CalcAddr = 0;
	SplitFlags();
	GetHandlers();
}

void	Reset (void)
{
	MemGetCode(PC);
	MemGetCode(PC);
	MemGet(0x100 | SP--);
	MemGet(0x100 | SP--);
	MemGet(0x100 | SP--);
	FI = 1;

	PCL = MemGet(0xFFFC);
	PCH = MemGet(0xFFFD);
#ifdef	ENABLE_DEBUGGER
	GotInterrupt = INTERRUPT_RST;
#endif	/* ENABLE_DEBUGGER */
}

#ifndef	NSFPLAYER
int	Save (FILE *out)
{
	int clen = 0;
		//Data
	writeByte(PCH);		//	PCL	uint8		Program Counter, low byte
	writeByte(PCL);		//	PCH	uint8		Program Counter, high byte
	writeByte(A);		//	A	uint8		Accumulator
	writeByte(X);		//	X	uint8		X register
	writeByte(Y);		//	Y	uint8		Y register
	writeByte(SP);		//	SP	uint8		Stack pointer
	JoinFlags();
	writeByte(P);		//	P	uint8		Processor status register
	writeByte(LastRead);	//	BUS	uint8		Last contents of data bus
	writeByte(WantNMI);	//	NMI	uint8		TRUE if falling edge detected on /NMI
	writeByte(WantIRQ);	//	IRQ	uint8		Flags 1=FRAME, 2=DPCM, 4=EXTERNAL
	writeArray(RAM, 0x800);	//	RAM	uint8[0x800]	2KB work RAM
	return clen;
}

int	Load (FILE *in)
{
	int clen = 0;
	readByte(PCH);		//	PCL	uint8		Program Counter, low byte
	readByte(PCL);		//	PCH	uint8		Program Counter, high byte
	readByte(A);		//	A	uint8		Accumulator
	readByte(X);		//	X	uint8		X register
	readByte(Y);		//	Y	uint8		Y register
	readByte(SP);		//	SP	uint8		Stack pointer
	readByte(P);		//	P	uint8		Processor status register
	SplitFlags();
	readByte(LastRead);	//	BUS	uint8		Last contents of data bus
	readByte(WantNMI);	//	NMI	uint8		TRUE if falling edge detected on /NMI
	readByte(WantIRQ);	//	IRQ	uint8		Flags 1=FRAME, 2=DPCM, 4=EXTERNAL
	readArray(RAM, 0x800);	//	RAM	uint8[0x800]	2KB work RAM
	return clen;
}
#endif	/* !NSFPLAYER */

int	MAPINT	ReadRAM (int Bank, int Addr)
{
	return RAM[Addr & 0x07FF];
}

void	MAPINT	WriteRAM (int Bank, int Addr, int Val)
{
	RAM[Addr & 0x07FF] = (unsigned char)Val;
}

int	MAPINT	Read4k (int Bank, int Addr)
{
	switch (Addr)
	{
	case 0x015:	return APU::Read4015();			break;
#ifndef	NSFPLAYER
	case 0x016:	return (LastRead & 0xC0) |
			(Controllers::Port1->Read() & 0x19) |
			(Controllers::PortExp->Read1() & 0x1F);	break;
	case 0x017:	return (LastRead & 0xC0) |
			(Controllers::Port2->Read() & 0x19) |
			(Controllers::PortExp->Read2() & 0x1F);	break;
#else	/* NSFPLAYER */
	case 0x016:
	case 0x017:	return (LastRead & 0xC0);		break;
#endif	/* !NSFPLAYER */
	default:	return LastRead;			break;
	}
}

void	MAPINT	Write4k (int Bank, int Addr, int Val)
{
	int i;
	switch (Addr)
	{
	case 0x000:case 0x001:case 0x002:case 0x003:
	case 0x004:case 0x005:case 0x006:case 0x007:
	case 0x008:case 0x009:case 0x00A:case 0x00B:
	case 0x00C:case 0x00D:case 0x00E:case 0x00F:
	case 0x010:case 0x011:case 0x012:case 0x013:
	case 0x015:case 0x017:
			APU::WriteReg(Addr, (unsigned char)Val);	break;
	case 0x014:	for (i = 0; i < 0x100; i++)
				MemSet(0x2004, MemGet((Val << 8) | i));
			MemGet(PC);	break;
#ifndef	NSFPLAYER
	case 0x016:	Controllers::Write((unsigned char)Val);		break;
#endif	/* !NSFPLAYER */
	}
}

int	MAPINT	ReadPRG (int Bank, int Addr)
{
	if (Readable[Bank])
		return PRGPointer[Bank][Addr];
	else	return -1;
}

void	MAPINT	WritePRG (int Bank, int Addr, int Val)
{
	if (Writable[Bank])
		PRGPointer[Bank][Addr] = (unsigned char)Val;
}

__forceinline	void	AM_IMP (void)
{
	MemGetCode(PC);
}
__forceinline	void	AM_IMM (void)
{
	CalcAddr = PC++;
}
__forceinline	void	AM_ABS (void)
{
	CalcAddrL = MemGetCode(PC++);
	CalcAddrH = MemGetCode(PC++);
}
__forceinline	void	AM_REL (void)
{
	BranchOffset = MemGetCode(PC++);
}
__forceinline	void	AM_ABX (void)
{
	CalcAddrL = MemGetCode(PC++);
	CalcAddrH = MemGetCode(PC++);
	bool inc = (CalcAddrL + X) >= 0x100;
	CalcAddrL += X;
	if (inc)
	{
		MemGet(CalcAddr);
		CalcAddrH++;
	}
}
__forceinline	void	AM_ABXW (void)
{
	CalcAddrL = MemGetCode(PC++);
	CalcAddrH = MemGetCode(PC++);
	bool inc = (CalcAddrL + X) >= 0x100;
	CalcAddrL += X;
	MemGet(CalcAddr);
	if (inc)
		CalcAddrH++;
}
__forceinline	void	AM_ABY (void)
{
	CalcAddrL = MemGetCode(PC++);
	CalcAddrH = MemGetCode(PC++);
	bool inc = (CalcAddrL + Y) >= 0x100;
	CalcAddrL += Y;
	if (inc)
	{
		MemGet(CalcAddr);
		CalcAddrH++;
	}
}
__forceinline	void	AM_ABYW (void)
{
	CalcAddrL = MemGetCode(PC++);
	CalcAddrH = MemGetCode(PC++);
	bool inc = (CalcAddrL + Y) >= 0x100;
	CalcAddrL += Y;
	MemGet(CalcAddr);
	if (inc)
		CalcAddrH++;
}
__forceinline	void	AM_ZPG (void)
{
	CalcAddr = MemGetCode(PC++);
}
__forceinline	void	AM_ZPX (void)
{
	CalcAddr = MemGetCode(PC++);
	MemGet(CalcAddr);
	CalcAddrL = CalcAddrL + X;
}
__forceinline	void	AM_ZPY (void)
{
	CalcAddr = MemGetCode(PC++);
	MemGet(CalcAddr);
	CalcAddrL = CalcAddrL + Y;
}
__forceinline	void	AM_INX (void)
{
	TmpAddr = MemGetCode(PC++);
	MemGet(TmpAddr);
	TmpAddrL = TmpAddrL + X;
	CalcAddrL = MemGet(TmpAddr);
	TmpAddrL++;
	CalcAddrH = MemGet(TmpAddr);
}
__forceinline	void	AM_INY (void)
{
	TmpAddr = MemGetCode(PC++);
	CalcAddrL = MemGet(TmpAddr);
	TmpAddrL++;
	CalcAddrH = MemGet(TmpAddr);
	bool inc = (CalcAddrL + Y) >= 0x100;
	CalcAddrL += Y;
	if (inc)
	{
		MemGet(CalcAddr);
		CalcAddrH++;
	}
}
__forceinline	void	AM_INYW (void)
{
	TmpAddr = MemGetCode(PC++);
	CalcAddrL = MemGet(TmpAddr);
	TmpAddrL++;
	CalcAddrH = MemGet(TmpAddr);
	bool inc = (CalcAddrL + Y) >= 0x100;
	CalcAddrL += Y;
	MemGet(CalcAddr);
	if (inc)
		CalcAddrH++;
}
__forceinline	void	IN_ADC (void)
{
	unsigned char Val = MemGet(CalcAddr);
	int result = A + Val + FC;
	FV = !!(~(A ^ Val) & (A ^ result) & 0x80);
	FC = !!(result & 0x100);
	A = result & 0xFF;
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}
__forceinline	void	IN_AND (void)
{
	A &= MemGet(CalcAddr);
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}
__forceinline	void	IN_ASL (void)
{
	unsigned char TmpData = MemGet(CalcAddr);
	MemSet(CalcAddr, TmpData);
	FC = (TmpData >> 7) & 1;
	TmpData <<= 1;
	FZ = (TmpData == 0);
	FN = (TmpData >> 7) & 1;
	MemSet(CalcAddr, TmpData);
}
__forceinline	void	IN_ASL_A (void)
{
	FC = (A >> 7) & 1;
	A <<= 1;
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}
void	IN_BRANCH (bool Condition)
{
	if (Condition)
	{
		// If an interrupt occurs during the final cycle of a non-pagecrossing branch
		// then it will be ignored until the next instruction completes
#ifndef	NSFPLAYER
		BOOL SkipNMI = FALSE;
#endif	/* !NSFPLAYER */
		BOOL SkipIRQ = FALSE;
#ifndef	NSFPLAYER
		if (WantNMI && !LastNMI)
			SkipNMI = TRUE;
#endif	/* !NSFPLAYER */
		if (WantIRQ && !LastIRQ)
			SkipIRQ = TRUE;
		MemGet(PC);
#ifndef	NSFPLAYER
		if (SkipNMI)
			LastNMI = FALSE;
#endif	/* !NSFPLAYER */
		if (SkipIRQ)
			LastIRQ = FALSE;
		bool inc = (PCL + BranchOffset) >= 0x100;
		PCL += BranchOffset;
		if (BranchOffset & 0x80)
		{
			if (!inc)
			{
				MemGet(PC);
				PCH--;
			}
		}
		else
		{
			if (inc)
			{
				MemGet(PC);
				PCH++;
			}
		}
	}
}
__forceinline	void	IN_BCS (void) {	IN_BRANCH( FC);	}
__forceinline	void	IN_BCC (void) {	IN_BRANCH(!FC);	}
__forceinline	void	IN_BEQ (void) {	IN_BRANCH( FZ);	}
__forceinline	void	IN_BNE (void) {	IN_BRANCH(!FZ);	}
__forceinline	void	IN_BMI (void) {	IN_BRANCH( FN);	}
__forceinline	void	IN_BPL (void) {	IN_BRANCH(!FN);	}
__forceinline	void	IN_BVS (void) {	IN_BRANCH( FV);	}
__forceinline	void	IN_BVC (void) {	IN_BRANCH(!FV);	}
__forceinline	void	IN_BIT (void)
{
	unsigned char Val = MemGet(CalcAddr);
	FV = (Val >> 6) & 1;
	FN = (Val >> 7) & 1;
	FZ = (Val & A) == 0;
}
void	IN_BRK (void)
{
	MemGet(PC);
	Push(PCH);
	Push(PCL);
	JoinFlags();
	Push(P | 0x10);
	FI = 1;
#ifndef	NSFPLAYER
	if (WantNMI)
	{
		WantNMI = FALSE;
		PCL = MemGet(0xFFFA);
		PCH = MemGet(0xFFFB);
	}
	else
	{
		PCL = MemGet(0xFFFE);
		PCH = MemGet(0xFFFF);
	}
#else	/* NSFPLAYER */
	PCL = MemGet(0xFFFE);
	PCH = MemGet(0xFFFF);
#endif	/* !NSFPLAYER */
#ifdef	ENABLE_DEBUGGER
	GotInterrupt = INTERRUPT_BRK;
#endif	/* ENABLE_DEBUGGER */
}
__forceinline	void	IN_CLC (void) { FC = 0; }
__forceinline	void	IN_CLD (void) { FD = 0; }
__forceinline	void	IN_CLI (void) { FI = 0; }
__forceinline	void	IN_CLV (void) { FV = 0; }
__forceinline	void	IN_CMP (void)
{
	int result = A - MemGet(CalcAddr);
	FC = (result >= 0);
	FZ = (result == 0);
	FN = (result >> 7) & 1;
}
__forceinline	void	IN_CPX (void)
{
	int result = X - MemGet(CalcAddr);
	FC = (result >= 0);
	FZ = (result == 0);
	FN = (result >> 7) & 1;
}
__forceinline	void	IN_CPY (void)
{
	int result = Y - MemGet(CalcAddr);
	FC = (result >= 0);
	FZ = (result == 0);
	FN = (result >> 7) & 1;
}
__forceinline	void	IN_DEC (void)
{
	unsigned char TmpData = MemGet(CalcAddr);
	MemSet(CalcAddr, TmpData);
	TmpData--;
	FZ = (TmpData == 0);
	FN = (TmpData >> 7) & 1;
	MemSet(CalcAddr, TmpData);
}
__forceinline	void	IN_DEX (void)
{
	X--;
	FZ = (X == 0);
	FN = (X >> 7) & 1;
}
__forceinline	void	IN_DEY (void)
{
	Y--;
	FZ = (Y == 0);
	FN = (Y >> 7) & 1;
}
__forceinline	void	IN_EOR (void)
{
	A ^= MemGet(CalcAddr);
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}
__forceinline	void	IN_INC (void)
{
	unsigned char TmpData = MemGet(CalcAddr);
	MemSet(CalcAddr, TmpData);
	TmpData++;
	FZ = (TmpData == 0);
	FN = (TmpData >> 7) & 1;
	MemSet(CalcAddr, TmpData);
}
__forceinline	void	IN_INX (void)
{
	X++;
	FZ = (X == 0);
	FN = (X >> 7) & 1;
}
__forceinline	void	IN_INY (void)
{
	Y++;
	FZ = (Y == 0);
	FN = (Y >> 7) & 1;
}
__forceinline	void	IN_JMP (void)
{
	PC = CalcAddr;
}
__forceinline	void	IN_JMP_I (void)
{
	PCL = MemGet(CalcAddr);
	CalcAddrL++;
	PCH = MemGet(CalcAddr);
}
__forceinline	void	IN_JSR (void)
{
	TmpAddrL = MemGetCode(PC++);
	MemGet(0x100 | SP);
	Push(PCH);
	Push(PCL);
	PCH = MemGetCode(PC);
	PCL = TmpAddrL;
}
__forceinline	void	IN_LDA (void)
{
	A = MemGet(CalcAddr);
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}
__forceinline	void	IN_LDX (void)
{
	X = MemGet(CalcAddr);
	FZ = (X == 0);
	FN = (X >> 7) & 1;
}
__forceinline	void	IN_LDY (void)
{
	Y = MemGet(CalcAddr);
	FZ = (Y == 0);
	FN = (Y >> 7) & 1;
}
__forceinline	void	IN_LSR (void)
{
	unsigned char TmpData = MemGet(CalcAddr);
	MemSet(CalcAddr, TmpData);
	FC = (TmpData & 0x01);
	TmpData >>= 1;
	FZ = (TmpData == 0);
	FN = 0;
	MemSet(CalcAddr, TmpData);
}
__forceinline	void	IN_LSR_A (void)
{
	FC = (A & 0x01);
	A >>= 1;
	FZ = (A == 0);
	FN = 0;
}
__forceinline	void	IN_NOP (void)
{
}
__forceinline	void	IN_ORA (void)
{
	A |= MemGet(CalcAddr);
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}
__forceinline	void	IN_PHA (void)
{
	Push(A);
}
__forceinline	void	IN_PHP (void)
{
	JoinFlags();
	Push(P | 0x10);
}
__forceinline	void	IN_PLA (void)
{
	MemGet(0x100 | SP);
	A = Pull();
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}
__forceinline	void	IN_PLP (void)
{
	MemGet(0x100 | SP);
	P = Pull();
	SplitFlags();
}
__forceinline	void	IN_ROL (void)
{
	unsigned char TmpData = MemGet(CalcAddr);
	MemSet(CalcAddr, TmpData);
	unsigned char carry = FC;
	FC = (TmpData >> 7) & 1;
	TmpData = (TmpData << 1) | carry;
	FZ = (TmpData == 0);
	FN = (TmpData >> 7) & 1;
	MemSet(CalcAddr, TmpData);
}
__forceinline	void	IN_ROL_A (void)
{
	unsigned char carry = FC;
	FC = (A >> 7) & 1;
	A = (A << 1) | carry;
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}
__forceinline	void	IN_ROR (void)
{
	unsigned char TmpData = MemGet(CalcAddr);
	MemSet(CalcAddr, TmpData);
	unsigned char carry = FC;
	FC = (TmpData & 0x01);
	TmpData = (TmpData >> 1) | (carry << 7);
	FZ = (TmpData == 0);
	FN = !!carry;
	MemSet(CalcAddr, TmpData);
}
__forceinline	void	IN_ROR_A (void)
{
	unsigned char carry = FC;
	FC = (A & 0x01);
	A = (A >> 1) | (carry << 7);
	FZ = (A == 0);
	FN = !!carry;
}
__forceinline	void	IN_RTI (void)
{
	MemGet(0x100 | SP);
	P = Pull();
	SplitFlags();
	PCL = Pull();
	PCH = Pull();
}
__forceinline	void	IN_RTS (void)
{
	MemGet(0x100 | SP);
	PCL = Pull();
	PCH = Pull();
	MemGet(PC++);
}
__forceinline	void	IN_SBC (void)
{
	unsigned char Val = MemGet(CalcAddr);
	int result = A + ~Val + FC;
	FV = !!((A ^ Val) & (A ^ result) & 0x80);
	FC = !(result & 0x100);
	A = result & 0xFF;
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}
__forceinline	void	IN_SEC (void) { FC = 1; }
__forceinline	void	IN_SED (void) { FD = 1; }
__forceinline	void	IN_SEI (void) { FI = 1; }
__forceinline	void	IN_STA (void)
{
	MemSet(CalcAddr, A);
}
__forceinline	void	IN_STX (void)
{
	MemSet(CalcAddr, X);
}
__forceinline	void	IN_STY (void)
{
	MemSet(CalcAddr, Y);
}
__forceinline	void	IN_TAX (void)
{
	X = A;
	FZ = (X == 0);
	FN = (X >> 7) & 1;
}
__forceinline	void	IN_TAY (void)
{
	Y = A;
	FZ = (Y == 0);
	FN = (Y >> 7) & 1;
}
__forceinline	void	IN_TSX (void)
{
	X = SP;
	FZ = (X == 0);
	FN = (X >> 7) & 1;
}
__forceinline	void	IN_TXA (void)
{
	A = X;
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}
__forceinline	void	IN_TXS (void)
{
	SP = X;
}
__forceinline	void	IN_TYA (void)
{
	A = Y;
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}

__forceinline	void	IV_UNK (void)
{
	// This isn't affected by the "Log Invalid Ops" toggle
	EI.DbgOut(_T("Unsupported opcode $%02X (???) encountered at $%04X"), Opcode, OpAddr);
}

__forceinline	void	IV_HLT (void)
{
	// And neither is this, considering it's going to be followed immediately by a message box
	EI.DbgOut(_T("HLT opcode $%02X encountered at $%04X; CPU locked"), Opcode, OpAddr);
#ifndef	NSFPLAYER
	MessageBox(hMainWnd, _T("Bad opcode, CPU locked"), _T("Nintendulator"), MB_OK);
	NES::DoStop = TRUE;
#endif	/* !NSFPLAYER */
}
__forceinline	void	IV_NOP (void)
{
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (NOP) encountered at $%04X"), Opcode, OpAddr);
	MemGet(CalcAddr);
}
__forceinline	void	IV_NOP2 (void)
{
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (NOP) encountered at $%04X"), Opcode, OpAddr);
}
__forceinline	void	IV_SLO (void)
{	// ASL + ORA
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (SLO) encountered at $%04X"), Opcode, OpAddr);
	unsigned char TmpData = MemGet(CalcAddr);
	MemSet(CalcAddr, TmpData);
	FC = (TmpData >> 7) & 1;
	TmpData <<= 1;
	A |= TmpData;
	FZ = (A == 0);
	FN = (A >> 7) & 1;
	MemSet(CalcAddr, TmpData);
}
__forceinline	void	IV_RLA (void)
{	// ROL + AND
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (RLA) encountered at $%04X"), Opcode, OpAddr);
	unsigned char TmpData = MemGet(CalcAddr);
	MemSet(CalcAddr, TmpData);
	unsigned char carry = FC;
	FC = (TmpData >> 7) & 1;
	TmpData = (TmpData << 1) | carry;
	A &= TmpData;
	FZ = (A == 0);
	FN = (A >> 7) & 1;
	MemSet(CalcAddr, TmpData);
}
__forceinline	void	IV_SRE (void)
{	// LSR + EOR
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (SRE) encountered at $%04X"), Opcode, OpAddr);
	unsigned char TmpData = MemGet(CalcAddr);
	MemSet(CalcAddr, TmpData);
	FC = (TmpData & 0x01);
	TmpData >>= 1;
	A ^= TmpData;
	FZ = (A == 0);
	FN = (A >> 7) & 1;
	MemSet(CalcAddr, TmpData);
}
__forceinline	void	IV_RRA (void)
{	// ROR + ADC
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (RRA) encountered at $%04X"), Opcode, OpAddr);
	unsigned char TmpData = MemGet(CalcAddr);
	MemSet(CalcAddr, TmpData);
	unsigned char carry = FC << 7;
	FC = (TmpData & 0x01);
	TmpData = (TmpData >> 1) | carry;
	int result = A + TmpData + FC;
	FV = !!(~(A ^ TmpData) & (A ^ result) & 0x80);
	FC = !!(result & 0x100);
	A = result & 0xFF;
	FZ = (A == 0);
	FN = (A >> 7) & 1;
	MemSet(CalcAddr, TmpData);
}
__forceinline	void	IV_SAX (void)
{	// STA + STX
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (SAX) encountered at $%04X"), Opcode, OpAddr);
	MemSet(CalcAddr, A & X);
}
__forceinline	void	IV_LAX (void)
{	// LDA + LDX
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (LAX) encountered at $%04X"), Opcode, OpAddr);
	X = A = MemGet(CalcAddr);
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}
__forceinline	void	IV_DCP (void)
{	// DEC + CMP
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (DCP) encountered at $%04X"), Opcode, OpAddr);
	unsigned char TmpData = MemGet(CalcAddr);
	MemSet(CalcAddr, TmpData);
	TmpData--;
	int result = A - TmpData;
	FC = (result >= 0);
	FZ = (result == 0);
	FN = (result >> 7) & 1;
	MemSet(CalcAddr, TmpData);
}
__forceinline	void	IV_ISB (void)
{	// INC + SBC
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (ISB) encountered at $%04X"), Opcode, OpAddr);
	unsigned char TmpData = MemGet(CalcAddr);
	MemSet(CalcAddr, TmpData);
	TmpData++;
	int result = A + ~TmpData + FC;
	FV = !!((A ^ TmpData) & (A ^ result) & 0x80);
	FC = !(result & 0x100);
	A = result & 0xFF;
	FZ = (A == 0);
	FN = (A >> 7) & 1;
	MemSet(CalcAddr, TmpData);
}
__forceinline	void	IV_SBC (void)
{	// NOP + SBC
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (SBC) encountered at $%04X"), Opcode, OpAddr);
	unsigned char Val = MemGet(CalcAddr);
	int result = A + ~Val + FC;
	FV = !!((A ^ Val) & (A ^ result) & 0x80);
	FC = !(result & 0x100);
	A = result & 0xFF;
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}

__forceinline	void	IV_AAC (void)
{	// ASL A+ORA and ROL A+AND, behaves strangely
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (AAC) encountered at $%04X"), Opcode, OpAddr);
	A &= MemGet(CalcAddr);
	FZ = (A == 0);
	FC = FN = (A >> 7) & 1;
}
__forceinline	void	IV_ASR (void)
{	// LSR A+EOR, behaves strangely
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (ASR) encountered at $%04X"), Opcode, OpAddr);
	A &= MemGet(CalcAddr);
	FC = (A & 0x01);
	A >>= 1;
	FZ = (A == 0);
	FN = 0;
}
__forceinline	void	IV_ARR (void)
{	// ROR A+AND, behaves strangely
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (ARR) encountered at $%04X"), Opcode, OpAddr);
	A = ((A & MemGet(CalcAddr)) >> 1) | (FC << 7);
	FZ = (A == 0);
	FN = (A >> 7) & 1;
	FC = (A >> 6) & 1;
	FV = FC ^ ((A >> 5) & 1);
}
__forceinline	void	IV_ATX (void)
{	// LDA+TAX, behaves strangely
	// documented as ANDing accumulator with data, but seemingly behaves exactly the same as LAX
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (ATX) encountered at $%04X"), Opcode, OpAddr);
	X = A = MemGet(CalcAddr);
	FZ = (A == 0);
	FN = (A >> 7) & 1;
}
__forceinline	void	IV_AXS (void)
{	// CMP+DEX, behaves strangely
	if (LogBadOps)
		EI.DbgOut(_T("Invalid opcode $%02X (AXS) encountered at $%04X"), Opcode, OpAddr);
	int result = (A & X) - MemGet(CalcAddr);
	FC = (result >= 0);
	FZ = (result == 0);
	X = result & 0xFF;
	FN = (result >> 7) & 1;
}
void	ExecOp (void)
{
	Opcode = MemGetCode(OpAddr = PC++);
	switch (Opcode)
	{
case 0x00:AM_IMM();  IN_BRK();break;case 0x10:AM_REL();  IN_BPL();break;case 0x08:AM_IMP();  IN_PHP();break;case 0x18:AM_IMP();  IN_CLC();break;case 0x04:AM_ZPG();  IV_NOP();break;case 0x14:AM_ZPX();  IV_NOP();break;case 0x0C:AM_ABS();  IV_NOP();break;case 0x1C:AM_ABX();  IV_NOP();break;
case 0x20:           IN_JSR();break;case 0x30:AM_REL();  IN_BMI();break;case 0x28:AM_IMP();  IN_PLP();break;case 0x38:AM_IMP();  IN_SEC();break;case 0x24:AM_ZPG();  IN_BIT();break;case 0x34:AM_ZPX();  IV_NOP();break;case 0x2C:AM_ABS();  IN_BIT();break;case 0x3C:AM_ABX();  IV_NOP();break;
case 0x40:AM_IMP();  IN_RTI();break;case 0x50:AM_REL();  IN_BVC();break;case 0x48:AM_IMP();  IN_PHA();break;case 0x58:AM_IMP();  IN_CLI();break;case 0x44:AM_ZPG();  IV_NOP();break;case 0x54:AM_ZPX();  IV_NOP();break;case 0x4C:AM_ABS();  IN_JMP();break;case 0x5C:AM_ABX();  IV_NOP();break;
case 0x60:AM_IMP();  IN_RTS();break;case 0x70:AM_REL();  IN_BVS();break;case 0x68:AM_IMP();  IN_PLA();break;case 0x78:AM_IMP();  IN_SEI();break;case 0x64:AM_ZPG();  IV_NOP();break;case 0x74:AM_ZPX();  IV_NOP();break;case 0x6C:AM_ABS();IN_JMP_I();break;case 0x7C:AM_ABX();  IV_NOP();break;
case 0x80:AM_IMM();  IV_NOP();break;case 0x90:AM_REL();  IN_BCC();break;case 0x88:AM_IMP();  IN_DEY();break;case 0x98:AM_IMP();  IN_TYA();break;case 0x84:AM_ZPG();  IN_STY();break;case 0x94:AM_ZPX();  IN_STY();break;case 0x8C:AM_ABS();  IN_STY();break;case 0x9C:AM_ABXW(); IV_NOP();break;
case 0xA0:AM_IMM();  IN_LDY();break;case 0xB0:AM_REL();  IN_BCS();break;case 0xA8:AM_IMP();  IN_TAY();break;case 0xB8:AM_IMP();  IN_CLV();break;case 0xA4:AM_ZPG();  IN_LDY();break;case 0xB4:AM_ZPX();  IN_LDY();break;case 0xAC:AM_ABS();  IN_LDY();break;case 0xBC:AM_ABX();  IN_LDY();break;
case 0xC0:AM_IMM();  IN_CPY();break;case 0xD0:AM_REL();  IN_BNE();break;case 0xC8:AM_IMP();  IN_INY();break;case 0xD8:AM_IMP();  IN_CLD();break;case 0xC4:AM_ZPG();  IN_CPY();break;case 0xD4:AM_ZPX();  IV_NOP();break;case 0xCC:AM_ABS();  IN_CPY();break;case 0xDC:AM_ABX();  IV_NOP();break;
case 0xE0:AM_IMM();  IN_CPX();break;case 0xF0:AM_REL();  IN_BEQ();break;case 0xE8:AM_IMP();  IN_INX();break;case 0xF8:AM_IMP();  IN_SED();break;case 0xE4:AM_ZPG();  IN_CPX();break;case 0xF4:AM_ZPX();  IV_NOP();break;case 0xEC:AM_ABS();  IN_CPX();break;case 0xFC:AM_ABX();  IV_NOP();break;

case 0x02:           IV_HLT();break;case 0x12:           IV_HLT();break;case 0x0A:AM_IMP();IN_ASL_A();break;case 0x1A:AM_IMP(); IV_NOP2();break;case 0x06:AM_ZPG();  IN_ASL();break;case 0x16:AM_ZPX();  IN_ASL();break;case 0x0E:AM_ABS();  IN_ASL();break;case 0x1E:AM_ABXW(); IN_ASL();break;
case 0x22:           IV_HLT();break;case 0x32:           IV_HLT();break;case 0x2A:AM_IMP();IN_ROL_A();break;case 0x3A:AM_IMP(); IV_NOP2();break;case 0x26:AM_ZPG();  IN_ROL();break;case 0x36:AM_ZPX();  IN_ROL();break;case 0x2E:AM_ABS();  IN_ROL();break;case 0x3E:AM_ABXW(); IN_ROL();break;
case 0x42:           IV_HLT();break;case 0x52:           IV_HLT();break;case 0x4A:AM_IMP();IN_LSR_A();break;case 0x5A:AM_IMP(); IV_NOP2();break;case 0x46:AM_ZPG();  IN_LSR();break;case 0x56:AM_ZPX();  IN_LSR();break;case 0x4E:AM_ABS();  IN_LSR();break;case 0x5E:AM_ABXW(); IN_LSR();break;
case 0x62:           IV_HLT();break;case 0x72:           IV_HLT();break;case 0x6A:AM_IMP();IN_ROR_A();break;case 0x7A:AM_IMP(); IV_NOP2();break;case 0x66:AM_ZPG();  IN_ROR();break;case 0x76:AM_ZPX();  IN_ROR();break;case 0x6E:AM_ABS();  IN_ROR();break;case 0x7E:AM_ABXW(); IN_ROR();break;
case 0x82:AM_IMM();  IV_NOP();break;case 0x92:           IV_HLT();break;case 0x8A:AM_IMP();  IN_TXA();break;case 0x9A:AM_IMP();  IN_TXS();break;case 0x86:AM_ZPG();  IN_STX();break;case 0x96:AM_ZPY();  IN_STX();break;case 0x8E:AM_ABS();  IN_STX();break;case 0x9E:AM_ABYW(); IV_NOP();break;
case 0xA2:AM_IMM();  IN_LDX();break;case 0xB2:           IV_HLT();break;case 0xAA:AM_IMP();  IN_TAX();break;case 0xBA:AM_IMP();  IN_TSX();break;case 0xA6:AM_ZPG();  IN_LDX();break;case 0xB6:AM_ZPY();  IN_LDX();break;case 0xAE:AM_ABS();  IN_LDX();break;case 0xBE:AM_ABY();  IN_LDX();break;
case 0xC2:AM_IMM();  IV_NOP();break;case 0xD2:           IV_HLT();break;case 0xCA:AM_IMP();  IN_DEX();break;case 0xDA:AM_IMP(); IV_NOP2();break;case 0xC6:AM_ZPG();  IN_DEC();break;case 0xD6:AM_ZPX();  IN_DEC();break;case 0xCE:AM_ABS();  IN_DEC();break;case 0xDE:AM_ABXW(); IN_DEC();break;
case 0xE2:AM_IMM();  IV_NOP();break;case 0xF2:           IV_HLT();break;case 0xEA:AM_IMP();  IN_NOP();break;case 0xFA:AM_IMP(); IV_NOP2();break;case 0xE6:AM_ZPG();  IN_INC();break;case 0xF6:AM_ZPX();  IN_INC();break;case 0xEE:AM_ABS();  IN_INC();break;case 0xFE:AM_ABXW(); IN_INC();break;

case 0x01:AM_INX();  IN_ORA();break;case 0x11:AM_INY();  IN_ORA();break;case 0x09:AM_IMM();  IN_ORA();break;case 0x19:AM_ABY();  IN_ORA();break;case 0x05:AM_ZPG();  IN_ORA();break;case 0x15:AM_ZPX();  IN_ORA();break;case 0x0D:AM_ABS();  IN_ORA();break;case 0x1D:AM_ABX();  IN_ORA();break;
case 0x21:AM_INX();  IN_AND();break;case 0x31:AM_INY();  IN_AND();break;case 0x29:AM_IMM();  IN_AND();break;case 0x39:AM_ABY();  IN_AND();break;case 0x25:AM_ZPG();  IN_AND();break;case 0x35:AM_ZPX();  IN_AND();break;case 0x2D:AM_ABS();  IN_AND();break;case 0x3D:AM_ABX();  IN_AND();break;
case 0x41:AM_INX();  IN_EOR();break;case 0x51:AM_INY();  IN_EOR();break;case 0x49:AM_IMM();  IN_EOR();break;case 0x59:AM_ABY();  IN_EOR();break;case 0x45:AM_ZPG();  IN_EOR();break;case 0x55:AM_ZPX();  IN_EOR();break;case 0x4D:AM_ABS();  IN_EOR();break;case 0x5D:AM_ABX();  IN_EOR();break;
case 0x61:AM_INX();  IN_ADC();break;case 0x71:AM_INY();  IN_ADC();break;case 0x69:AM_IMM();  IN_ADC();break;case 0x79:AM_ABY();  IN_ADC();break;case 0x65:AM_ZPG();  IN_ADC();break;case 0x75:AM_ZPX();  IN_ADC();break;case 0x6D:AM_ABS();  IN_ADC();break;case 0x7D:AM_ABX();  IN_ADC();break;
case 0x81:AM_INX();  IN_STA();break;case 0x91:AM_INYW(); IN_STA();break;case 0x89:AM_IMM();  IV_NOP();break;case 0x99:AM_ABYW(); IN_STA();break;case 0x85:AM_ZPG();  IN_STA();break;case 0x95:AM_ZPX();  IN_STA();break;case 0x8D:AM_ABS();  IN_STA();break;case 0x9D:AM_ABXW(); IN_STA();break;
case 0xA1:AM_INX();  IN_LDA();break;case 0xB1:AM_INY();  IN_LDA();break;case 0xA9:AM_IMM();  IN_LDA();break;case 0xB9:AM_ABY();  IN_LDA();break;case 0xA5:AM_ZPG();  IN_LDA();break;case 0xB5:AM_ZPX();  IN_LDA();break;case 0xAD:AM_ABS();  IN_LDA();break;case 0xBD:AM_ABX();  IN_LDA();break;
case 0xC1:AM_INX();  IN_CMP();break;case 0xD1:AM_INY();  IN_CMP();break;case 0xC9:AM_IMM();  IN_CMP();break;case 0xD9:AM_ABY();  IN_CMP();break;case 0xC5:AM_ZPG();  IN_CMP();break;case 0xD5:AM_ZPX();  IN_CMP();break;case 0xCD:AM_ABS();  IN_CMP();break;case 0xDD:AM_ABX();  IN_CMP();break;
case 0xE1:AM_INX();  IN_SBC();break;case 0xF1:AM_INY();  IN_SBC();break;case 0xE9:AM_IMM();  IN_SBC();break;case 0xF9:AM_ABY();  IN_SBC();break;case 0xE5:AM_ZPG();  IN_SBC();break;case 0xF5:AM_ZPX();  IN_SBC();break;case 0xED:AM_ABS();  IN_SBC();break;case 0xFD:AM_ABX();  IN_SBC();break;

case 0x03:AM_INX();  IV_SLO();break;case 0x13:AM_INYW(); IV_SLO();break;case 0x0B:AM_IMM();  IV_AAC();break;case 0x1B:AM_ABYW(); IV_SLO();break;case 0x07:AM_ZPG();  IV_SLO();break;case 0x17:AM_ZPX();  IV_SLO();break;case 0x0F:AM_ABS();  IV_SLO();break;case 0x1F:AM_ABXW(); IV_SLO();break;
case 0x23:AM_INX();  IV_RLA();break;case 0x33:AM_INYW(); IV_RLA();break;case 0x2B:AM_IMM();  IV_AAC();break;case 0x3B:AM_ABYW(); IV_RLA();break;case 0x27:AM_ZPG();  IV_RLA();break;case 0x37:AM_ZPX();  IV_RLA();break;case 0x2F:AM_ABS();  IV_RLA();break;case 0x3F:AM_ABXW(); IV_RLA();break;
case 0x43:AM_INX();  IV_SRE();break;case 0x53:AM_INYW(); IV_SRE();break;case 0x4B:AM_IMM();  IV_ASR();break;case 0x5B:AM_ABYW(); IV_SRE();break;case 0x47:AM_ZPG();  IV_SRE();break;case 0x57:AM_ZPX();  IV_SRE();break;case 0x4F:AM_ABS();  IV_SRE();break;case 0x5F:AM_ABXW(); IV_SRE();break;
case 0x63:AM_INX();  IV_RRA();break;case 0x73:AM_INYW(); IV_RRA();break;case 0x6B:AM_IMM();  IV_ARR();break;case 0x7B:AM_ABYW(); IV_RRA();break;case 0x67:AM_ZPG();  IV_RRA();break;case 0x77:AM_ZPX();  IV_RRA();break;case 0x6F:AM_ABS();  IV_RRA();break;case 0x7F:AM_ABXW(); IV_RRA();break;
case 0x83:AM_INX();  IV_SAX();break;case 0x93:AM_INY();  IV_UNK();break;case 0x8B:AM_IMM();  IV_UNK();break;case 0x9B:AM_ABY();  IV_UNK();break;case 0x87:AM_ZPG();  IV_SAX();break;case 0x97:AM_ZPY();  IV_SAX();break;case 0x8F:AM_ABS();  IV_SAX();break;case 0x9F:AM_ABY();  IV_UNK();break;
case 0xA3:AM_INX();  IV_LAX();break;case 0xB3:AM_INY();  IV_LAX();break;case 0xAB:AM_IMM();  IV_ATX();break;case 0xBB:AM_ABY();  IV_UNK();break;case 0xA7:AM_ZPG();  IV_LAX();break;case 0xB7:AM_ZPY();  IV_LAX();break;case 0xAF:AM_ABS();  IV_LAX();break;case 0xBF:AM_ABY();  IV_LAX();break;
case 0xC3:AM_INX();  IV_DCP();break;case 0xD3:AM_INYW(); IV_DCP();break;case 0xCB:AM_IMM();  IV_AXS();break;case 0xDB:AM_ABYW(); IV_DCP();break;case 0xC7:AM_ZPG();  IV_DCP();break;case 0xD7:AM_ZPX();  IV_DCP();break;case 0xCF:AM_ABS();  IV_DCP();break;case 0xDF:AM_ABXW(); IV_DCP();break;
case 0xE3:AM_INX();  IV_ISB();break;case 0xF3:AM_INYW(); IV_ISB();break;case 0xEB:AM_IMM();  IV_SBC();break;case 0xFB:AM_ABYW(); IV_ISB();break;case 0xE7:AM_ZPG();  IV_ISB();break;case 0xF7:AM_ZPX();  IV_ISB();break;case 0xEF:AM_ABS();  IV_ISB();break;case 0xFF:AM_ABXW(); IV_ISB();break;
	}

#ifndef	NSFPLAYER
	if (LastNMI)
	{
		DoNMI();
		WantNMI = FALSE;
	}
	else
#endif	/* !NSFPLAYER */
	if (LastIRQ)
		DoIRQ();
#ifdef	ENABLE_DEBUGGER
	else	GotInterrupt = 0;
#endif	/* ENABLE_DEBUGGER */
}
} // namespace CPU