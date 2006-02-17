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

#ifndef __CPU_H__
#define __CPU_H__

#include "MapperInterface.h"

union SplitReg { unsigned long Full; unsigned char Segment[4]; };

struct tCPU
{
	PReadFunc	ReadHandler[0x10];
	PWriteFunc	WriteHandler[0x10];
	unsigned char *	PRGPointer[0x10];
	BOOL	Readable[0x10], Writable[0x10];

	BOOL WantNMI, WantReset, WantIRQ;
	unsigned char A, X, Y, SP, P;
	unsigned char FC, FZ, FI, FD, FB, FV, FN;
	unsigned char LastRead;
	union SplitReg _PC;
};
extern	struct tCPU CPU;
extern	unsigned char CPU_RAM[0x800];

#define PC _PC.Full
#define PCL _PC.Segment[0]
#define PCH _PC.Segment[1]

unsigned char	__fastcall	CPU_MemGet (unsigned int);
void	__fastcall	CPU_MemSet (unsigned int,unsigned char);

void	CPU_JoinFlags (void);
void	CPU_SplitFlags (void);

void	CPU_Reset (void);
void	CPU_PowerOn (void);
void	CPU_ExecOp (void);
int	__cdecl	CPU_ReadRAM (int,int);
void	__cdecl	CPU_WriteRAM (int,int,int);
int	__cdecl	CPU_Read4k (int,int);
void	__cdecl	CPU_Write4k (int,int,int);
int	__cdecl	CPU_ReadPRG (int,int);
void	__cdecl	CPU_WritePRG (int,int,int);

#endif

