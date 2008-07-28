/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2008 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef	CPU_H
#define CPU_H

union SplitReg { unsigned long Full; unsigned char Segment[4]; };

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

struct tCPU
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
	unsigned char FC, FZ, FI, FD, FV, FN;
	unsigned char LastRead;
	union SplitReg rPC;
};
extern	struct tCPU CPU;
extern	unsigned char CPU_RAM[0x800];

#define PC rPC.Full
#define PCL rPC.Segment[0]
#define PCH rPC.Segment[1]

unsigned char	__fastcall	CPU_MemGet (unsigned int);
void	__fastcall	CPU_MemSet (unsigned int,unsigned char);

void	CPU_JoinFlags (void);
void	CPU_SplitFlags (void);

void	CPU_GetHandlers (void);
void	CPU_Reset (void);
void	CPU_PowerOn (void);
#ifndef	NSFPLAYER
int	CPU_Save (FILE *);
int	CPU_Load (FILE *);
#endif	/* !NSFPLAYER */
void	CPU_ExecOp (void);
int	MAPINT	CPU_ReadRAM (int,int);
void	MAPINT	CPU_WriteRAM (int,int,int);
int	MAPINT	CPU_Read4k (int,int);
void	MAPINT	CPU_Write4k (int,int,int);
int	MAPINT	CPU_ReadPRG (int,int);
void	MAPINT	CPU_WritePRG (int,int,int);

#endif	/* !CPU_H */
