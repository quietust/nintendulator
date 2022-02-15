/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) QMT Productions
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
#endif	/* NSFPLAYER */

namespace CPU
{
FCPURead	ReadHandler[0x10];
FCPURead	ReadHandlerDebug[0x10];
FCPUWrite	WriteHandler[0x10];
unsigned char *	PRGPointer[0x10];
BOOL	Readable[0x10], Writable[0x10];

#ifndef	NSFPLAYER
unsigned char WantNMI;
#endif	/* !NSFPLAYER */
unsigned char WantIRQ;
BOOL EnableDMA;
unsigned char DMAPage;
#ifdef	ENABLE_DEBUGGER
unsigned char GotInterrupt;
unsigned long Cycles;
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
#ifdef	ENABLE_DEBUGGER
	Cycles++;
#endif
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
	if (EnableDMA)
	{
		if ((EnableDMA & (DMA_PCM | DMA_SPR)) == DMA_PCM)
		{
			BOOL isController = (Addr == 0x4016) || (Addr == 0x4017);

			// Only run through this section if PCM triggered by itself
			// if it interrupts Sprite DMA, the timing comes out differently
			EnableDMA &= ~DMA_PCM;

			// Halt - read address and discard
			MemGet(Addr);

			// Dummy read
			isController ? RunCycle() : MemGet(Addr);

			// Optional alignment read
			if (!(APU::InternalClock & 1))
				isController ? RunCycle() : MemGet(Addr);

			// DPCM read
			APU::DPCM::Fetch();
		}
		if (EnableDMA & DMA_SPR)
		{
			// Check if PCM and Sprite DMAs managed to coincide
			BOOL WasPCM = (EnableDMA & DMA_PCM);	EnableDMA = 0;

			// Halt
			MemGet(Addr);

			WasPCM |= (EnableDMA & DMA_PCM);	EnableDMA = 0;

			if (!(APU::InternalClock & 1))
			{
				MemGet(Addr);
				// PCM DMA can't trigger here, wrong alignment
			}

			for (int i = 0; i < 0x100; i++)
			{
				if (WasPCM)
				{
					// process PCM DMA
					APU::DPCM::Fetch();
					// Realign clock
					MemGet(Addr);
					WasPCM = FALSE;
				}
				else
				{
					// check for PCM DMA and schedule it for the next loop
					WasPCM = (EnableDMA & DMA_PCM);	EnableDMA = 0;
				}
				buf = MemGet((DMAPage << 8) | i);
				MemSet(0x2004, buf);
			}
			// if it triggered at the very end, handle it here
			if (WasPCM)
			{
				// process PCM DMA
				APU::DPCM::Fetch();
				// Realign clock
				MemGet(Addr);
			}
		}
	}

	RunCycle();
	// avoid an indirect function call if possible - if it's ReadPRG, then call it directly (ideally inline)
	if (ReadHandler[(Addr >> 12) & 0xF] == ReadPRG)
		buf = ReadPRG((Addr >> 12) & 0xF, Addr & 0xFFF);
	else	buf = ReadHandler[(Addr >> 12) & 0xF]((Addr >> 12) & 0xF, Addr & 0xFFF);
	if (buf != -1)
		LastRead = (unsigned char)buf;
	return LastRead;
}
void	MemSet (unsigned int Addr, unsigned char Val)
{
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
	if (LastNMI)
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
	// Prevent NMI from triggering immediately after IRQ
	if (LastNMI)
		LastNMI = FALSE;
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
#ifdef	ENABLE_DEBUGGER
	Cycles = 0;
#endif	/* ENABLE_DEBUGGER */
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

int	Load (FILE *in, int version_id)
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

int	MAPINT	ReadUnsafe (int Bank, int Addr)
{
	return 0xFF;
}

int	MAPINT	ReadRAM (int Bank, int Addr)
{
	return RAM[Addr & 0x07FF];
}

void	MAPINT	WriteRAM (int Bank, int Addr, int Val)
{
	RAM[Addr & 0x07FF] = (unsigned char)Val;
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
__forceinline	void	AM_NON (void)
{
	// placeholder for instructions that don't use any of the above addressing modes
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
__forceinline	void	IN_ASLA (void)
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
	MemGet(CalcAddr);
	Push(PCH);
	Push(PCL);
	JoinFlags();
	Push(P | 0x10);
	FI = 1;
#ifndef	NSFPLAYER
	if (LastNMI)
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
	// Prevent NMI from triggering immediately after BRK
	if (LastNMI)
		LastNMI = FALSE;
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
__forceinline	void	IN_JMPI (void)
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
__forceinline	void	IN_LSRA (void)
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
__forceinline	void	IN_ROLA (void)
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
__forceinline	void	IN_RORA (void)
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
	// Perform an extra memory access
	MemGet(CalcAddr);
}

__forceinline	void	IV_HLT (void)
{
	// And neither is this, considering it's going to be followed immediately by a message box
	EI.DbgOut(_T("HLT opcode $%02X encountered at $%04X; CPU locked"), Opcode, OpAddr);
#ifndef	NSFPLAYER
	MessageBox(hMainWnd, _T("Bad opcode, CPU locked"), _T("Nintendulator"), MB_OK);
	NES::DoStop = STOPMODE_NOW | STOPMODE_BREAK;
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
#ifndef	NSFPLAYER
	if (LastNMI)
	{
		WantNMI = FALSE;
		DoNMI();
		return;
	}
	else
#endif	/* !NSFPLAYER */
	if (LastIRQ)
	{
		DoIRQ();
		return;
	}
#ifdef	ENABLE_DEBUGGER
	else	GotInterrupt = 0;
#endif	/* ENABLE_DEBUGGER */

	Opcode = MemGetCode(OpAddr = PC++);
	switch (Opcode)
	{
#define OP(op,adr,ins) case 0x##op: AM_##adr(); ins(); break;

OP(00, IMM, IN_BRK) OP(10, REL, IN_BPL) OP(08, IMP, IN_PHP) OP(18, IMP, IN_CLC) OP(04, ZPG, IV_NOP) OP(14, ZPX, IV_NOP) OP(0C, ABS, IV_NOP) OP(1C, ABX, IV_NOP)
OP(20, NON, IN_JSR) OP(30, REL, IN_BMI) OP(28, IMP, IN_PLP) OP(38, IMP, IN_SEC) OP(24, ZPG, IN_BIT) OP(34, ZPX, IV_NOP) OP(2C, ABS, IN_BIT) OP(3C, ABX, IV_NOP)
OP(40, IMP, IN_RTI) OP(50, REL, IN_BVC) OP(48, IMP, IN_PHA) OP(58, IMP, IN_CLI) OP(44, ZPG, IV_NOP) OP(54, ZPX, IV_NOP) OP(4C, ABS, IN_JMP) OP(5C, ABX, IV_NOP)
OP(60, IMP, IN_RTS) OP(70, REL, IN_BVS) OP(68, IMP, IN_PLA) OP(78, IMP, IN_SEI) OP(64, ZPG, IV_NOP) OP(74, ZPX, IV_NOP) OP(6C, ABS,IN_JMPI) OP(7C, ABX, IV_NOP)
OP(80, IMM, IV_NOP) OP(90, REL, IN_BCC) OP(88, IMP, IN_DEY) OP(98, IMP, IN_TYA) OP(84, ZPG, IN_STY) OP(94, ZPX, IN_STY) OP(8C, ABS, IN_STY) OP(9C,ABXW, IV_NOP)
OP(A0, IMM, IN_LDY) OP(B0, REL, IN_BCS) OP(A8, IMP, IN_TAY) OP(B8, IMP, IN_CLV) OP(A4, ZPG, IN_LDY) OP(B4, ZPX, IN_LDY) OP(AC, ABS, IN_LDY) OP(BC, ABX, IN_LDY)
OP(C0, IMM, IN_CPY) OP(D0, REL, IN_BNE) OP(C8, IMP, IN_INY) OP(D8, IMP, IN_CLD) OP(C4, ZPG, IN_CPY) OP(D4, ZPX, IV_NOP) OP(CC, ABS, IN_CPY) OP(DC, ABX, IV_NOP)
OP(E0, IMM, IN_CPX) OP(F0, REL, IN_BEQ) OP(E8, IMP, IN_INX) OP(F8, IMP, IN_SED) OP(E4, ZPG, IN_CPX) OP(F4, ZPX, IV_NOP) OP(EC, ABS, IN_CPX) OP(FC, ABX, IV_NOP)

OP(02, NON, IV_HLT) OP(12, NON, IV_HLT) OP(0A, IMP,IN_ASLA) OP(1A, IMP,IV_NOP2) OP(06, ZPG, IN_ASL) OP(16, ZPX, IN_ASL) OP(0E, ABS, IN_ASL) OP(1E,ABXW, IN_ASL)
OP(22, NON, IV_HLT) OP(32, NON, IV_HLT) OP(2A, IMP,IN_ROLA) OP(3A, IMP,IV_NOP2) OP(26, ZPG, IN_ROL) OP(36, ZPX, IN_ROL) OP(2E, ABS, IN_ROL) OP(3E,ABXW, IN_ROL)
OP(42, NON, IV_HLT) OP(52, NON, IV_HLT) OP(4A, IMP,IN_LSRA) OP(5A, IMP,IV_NOP2) OP(46, ZPG, IN_LSR) OP(56, ZPX, IN_LSR) OP(4E, ABS, IN_LSR) OP(5E,ABXW, IN_LSR)
OP(62, NON, IV_HLT) OP(72, NON, IV_HLT) OP(6A, IMP,IN_RORA) OP(7A, IMP,IV_NOP2) OP(66, ZPG, IN_ROR) OP(76, ZPX, IN_ROR) OP(6E, ABS, IN_ROR) OP(7E,ABXW, IN_ROR)
OP(82, IMM, IV_NOP) OP(92, NON, IV_HLT) OP(8A, IMP, IN_TXA) OP(9A, IMP, IN_TXS) OP(86, ZPG, IN_STX) OP(96, ZPY, IN_STX) OP(8E, ABS, IN_STX) OP(9E,ABYW, IV_NOP)
OP(A2, IMM, IN_LDX) OP(B2, NON, IV_HLT) OP(AA, IMP, IN_TAX) OP(BA, IMP, IN_TSX) OP(A6, ZPG, IN_LDX) OP(B6, ZPY, IN_LDX) OP(AE, ABS, IN_LDX) OP(BE, ABY, IN_LDX)
OP(C2, IMM, IV_NOP) OP(D2, NON, IV_HLT) OP(CA, IMP, IN_DEX) OP(DA, IMP,IV_NOP2) OP(C6, ZPG, IN_DEC) OP(D6, ZPX, IN_DEC) OP(CE, ABS, IN_DEC) OP(DE,ABXW, IN_DEC)
OP(E2, IMM, IV_NOP) OP(F2, NON, IV_HLT) OP(EA, IMP, IN_NOP) OP(FA, IMP,IV_NOP2) OP(E6, ZPG, IN_INC) OP(F6, ZPX, IN_INC) OP(EE, ABS, IN_INC) OP(FE,ABXW, IN_INC)

OP(01, INX, IN_ORA) OP(11, INY, IN_ORA) OP(09, IMM, IN_ORA) OP(19, ABY, IN_ORA) OP(05, ZPG, IN_ORA) OP(15, ZPX, IN_ORA) OP(0D, ABS, IN_ORA) OP(1D, ABX, IN_ORA)
OP(21, INX, IN_AND) OP(31, INY, IN_AND) OP(29, IMM, IN_AND) OP(39, ABY, IN_AND) OP(25, ZPG, IN_AND) OP(35, ZPX, IN_AND) OP(2D, ABS, IN_AND) OP(3D, ABX, IN_AND)
OP(41, INX, IN_EOR) OP(51, INY, IN_EOR) OP(49, IMM, IN_EOR) OP(59, ABY, IN_EOR) OP(45, ZPG, IN_EOR) OP(55, ZPX, IN_EOR) OP(4D, ABS, IN_EOR) OP(5D, ABX, IN_EOR)
OP(61, INX, IN_ADC) OP(71, INY, IN_ADC) OP(69, IMM, IN_ADC) OP(79, ABY, IN_ADC) OP(65, ZPG, IN_ADC) OP(75, ZPX, IN_ADC) OP(6D, ABS, IN_ADC) OP(7D, ABX, IN_ADC)
OP(81, INX, IN_STA) OP(91,INYW, IN_STA) OP(89, IMM, IV_NOP) OP(99,ABYW, IN_STA) OP(85, ZPG, IN_STA) OP(95, ZPX, IN_STA) OP(8D, ABS, IN_STA) OP(9D,ABXW, IN_STA)
OP(A1, INX, IN_LDA) OP(B1, INY, IN_LDA) OP(A9, IMM, IN_LDA) OP(B9, ABY, IN_LDA) OP(A5, ZPG, IN_LDA) OP(B5, ZPX, IN_LDA) OP(AD, ABS, IN_LDA) OP(BD, ABX, IN_LDA)
OP(C1, INX, IN_CMP) OP(D1, INY, IN_CMP) OP(C9, IMM, IN_CMP) OP(D9, ABY, IN_CMP) OP(C5, ZPG, IN_CMP) OP(D5, ZPX, IN_CMP) OP(CD, ABS, IN_CMP) OP(DD, ABX, IN_CMP)
OP(E1, INX, IN_SBC) OP(F1, INY, IN_SBC) OP(E9, IMM, IN_SBC) OP(F9, ABY, IN_SBC) OP(E5, ZPG, IN_SBC) OP(F5, ZPX, IN_SBC) OP(ED, ABS, IN_SBC) OP(FD, ABX, IN_SBC)

OP(03, INX, IV_SLO) OP(13,INYW, IV_SLO) OP(0B, IMM, IV_AAC) OP(1B,ABYW, IV_SLO) OP(07, ZPG, IV_SLO) OP(17, ZPX, IV_SLO) OP(0F, ABS, IV_SLO) OP(1F,ABXW, IV_SLO)
OP(23, INX, IV_RLA) OP(33,INYW, IV_RLA) OP(2B, IMM, IV_AAC) OP(3B,ABYW, IV_RLA) OP(27, ZPG, IV_RLA) OP(37, ZPX, IV_RLA) OP(2F, ABS, IV_RLA) OP(3F,ABXW, IV_RLA)
OP(43, INX, IV_SRE) OP(53,INYW, IV_SRE) OP(4B, IMM, IV_ASR) OP(5B,ABYW, IV_SRE) OP(47, ZPG, IV_SRE) OP(57, ZPX, IV_SRE) OP(4F, ABS, IV_SRE) OP(5F,ABXW, IV_SRE)
OP(63, INX, IV_RRA) OP(73,INYW, IV_RRA) OP(6B, IMM, IV_ARR) OP(7B,ABYW, IV_RRA) OP(67, ZPG, IV_RRA) OP(77, ZPX, IV_RRA) OP(6F, ABS, IV_RRA) OP(7F,ABXW, IV_RRA)
OP(83, INX, IV_SAX) OP(93,INYW, IV_UNK) OP(8B, IMM, IV_UNK) OP(9B,ABYW, IV_UNK) OP(87, ZPG, IV_SAX) OP(97, ZPY, IV_SAX) OP(8F, ABS, IV_SAX) OP(9F,ABYW, IV_UNK)
OP(A3, INX, IV_LAX) OP(B3, INY, IV_LAX) OP(AB, IMM, IV_ATX) OP(BB, ABY, IV_UNK) OP(A7, ZPG, IV_LAX) OP(B7, ZPY, IV_LAX) OP(AF, ABS, IV_LAX) OP(BF, ABY, IV_LAX)
OP(C3, INX, IV_DCP) OP(D3,INYW, IV_DCP) OP(CB, IMM, IV_AXS) OP(DB,ABYW, IV_DCP) OP(C7, ZPG, IV_DCP) OP(D7, ZPX, IV_DCP) OP(CF, ABS, IV_DCP) OP(DF,ABXW, IV_DCP)
OP(E3, INX, IV_ISB) OP(F3,INYW, IV_ISB) OP(EB, IMM, IV_SBC) OP(FB,ABYW, IV_ISB) OP(E7, ZPG, IV_ISB) OP(F7, ZPX, IV_ISB) OP(EF, ABS, IV_ISB) OP(FF,ABXW, IV_ISB)

#undef OP
	}
}
} // namespace CPU
