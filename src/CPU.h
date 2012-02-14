/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * $URL$
 * $Id$
 */

#pragma once

#define	IRQ_FRAME	0x01
#define	IRQ_DPCM	0x02
#define	IRQ_EXTERNAL	0x04
#define	IRQ_DEBUG	0x08

#ifdef	ENABLE_DEBUGGER
#define	INTERRUPT_NMI	1
#define	INTERRUPT_RST	2
#define	INTERRUPT_IRQ	3
#define	INTERRUPT_BRK	4
#endif	/* ENABLE_DEBUGGER */

namespace CPU
{
union SplitReg { unsigned short Full; unsigned char Segment[2]; };

extern FCPURead ReadHandler[0x10];
extern FCPUWrite WriteHandler[0x10];
extern unsigned char *PRGPointer[0x10];
extern BOOL Readable[0x10], Writable[0x10];

#ifndef	NSFPLAYER
extern unsigned char WantNMI;
#endif	/* !NSFPLAYER */
extern unsigned char WantIRQ;
extern unsigned char PCMCycles;
#ifdef	ENABLE_DEBUGGER
extern unsigned char GotInterrupt;
#endif	/* ENABLE_DEBUGGER */

extern unsigned char A, X, Y, SP, P;
extern bool FC, FZ, FI, FD, FV, FN;
extern unsigned char LastRead;
extern SplitReg rPC;

extern unsigned char RAM[0x800];

extern BOOL LogBadOps;

#define PC rPC.Full
#define PCL rPC.Segment[0]
#define PCH rPC.Segment[1]

unsigned char	MemGet (unsigned int);
void	MemSet (unsigned int, unsigned char);

void	JoinFlags (void);
void	SplitFlags (void);

void	GetHandlers (void);
void	Reset (void);
void	PowerOn (void);
#ifndef	NSFPLAYER
int	Save (FILE *);
int	Load (FILE *);
#endif	/* !NSFPLAYER */
void	ExecOp (void);
int	MAPINT	ReadRAM (int, int);
void	MAPINT	WriteRAM (int, int, int);
int	MAPINT	ReadPRG (int, int);
void	MAPINT	WritePRG (int, int, int);
} // namespace CPU
