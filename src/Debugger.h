/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2009 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef DEBUGGER_H
#define DEBUGGER_H

#ifdef	ENABLE_DEBUGGER

#define DEBUG_MODE_CPU	1
#define DEBUG_MODE_PPU	2

#define DEBUG_TRACELINES 13
#define DEBUG_MEMLINES 8
#define DEBUG_BREAKPOINTS 256

#define	DEBUG_BREAK_EXEC	0x01
#define	DEBUG_BREAK_READ	0x02
#define	DEBUG_BREAK_WRITE	0x04
#define	DEBUG_BREAK_OPCODE	0x08
#define	DEBUG_BREAK_NMI		0x10
#define	DEBUG_BREAK_IRQ		0x20
#define	DEBUG_BREAK_BRK		0x40
/* #define	DEBUG_BREAK_RST		0x80	/* unused */

struct tBreakpoint
{
	TCHAR desc[32];
	unsigned short addr_start, addr_end;
	unsigned char opcode;
	unsigned char type;
	unsigned char enabled;
	struct tBreakpoint *prev, *next;
};

struct tDebugger
{
	BOOL	Enabled;
	int	Mode;

	BOOL	NTabChanged, PalChanged, PatChanged;

	BOOL	Logging, Step;

	int	Depth;

	int	Palette, Nametable;

	HDC	PaletteDC;	/* Palette */
	HBITMAP	PaletteBMP;

	HDC	PatternDC;	/* Pattern tables */
	HBITMAP	PatternBMP;

	HDC	NameDC;		/* Nametable */
	HBITMAP	NameBMP;

	HWND	CPUWnd;
	HWND	PPUWnd;

	int	TraceOffset;	/* -1 to center on PC, otherwise center on TraceOffset */
	int	MemOffset;

	FILE	*LogFile;
	unsigned char	BPcache[0x10101];
	struct tBreakpoint *Breakpoints;
};

extern struct tDebugger Debugger;

void	Debugger_Init (void);
void	Debugger_Release (void);
void	Debugger_SetMode(int NewMode);
void	Debugger_Update (void);
void	Debugger_AddInst (void);

#endif	/* !ENABLE_DEBUGGER */

#endif	/* !DEBUGGER_H */
