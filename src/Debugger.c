/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2009 QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include <time.h>
#include <windowsx.h>
#include "Nintendulator.h"
#include "resource.h"
#include "MapperInterface.h"
#include "NES.h"
#include "APU.h"
#include "Debugger.h"
#include "CPU.h"
#include "PPU.h"
#include "GFX.h"
#include "Genie.h"
#include "States.h"

#ifdef	ENABLE_DEBUGGER

struct tDebugger Debugger;

static BOOL inUpdate = FALSE;

enum ADDRMODE { IMP, ACC, IMM, ADR, ABS, IND, REL, ABX, ABY, ZPG, ZPX, ZPY, INX, INY, ERR, NUM_ADDR_MODES };

enum ADDRMODE TraceAddrMode[256] =
{
	IMM, INX, ERR, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
	ADR, INX, ERR, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
	IMP, INX, ERR, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, ACC, IMM, ADR, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
	IMP, INX, ERR, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, ACC, IMM, IND, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
	IMM, INX, IMM, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPY, ZPY, IMP, ABY, IMP, ABY, ABX, ABX, ABY, ABY,
	IMM, INX, IMM, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPY, ZPY, IMP, ABY, IMP, ABY, ABX, ABX, ABY, ABY,
	IMM, INX, IMM, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
	IMM, INX, IMM, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX
};

unsigned char AddrBytes[NUM_ADDR_MODES] = {1, 1, 2, 3, 3, 3, 2, 3, 3, 2, 2, 2, 2, 2, 1};

char TraceArr[256][5] =
{
	" BRK", " ORA", "*HLT", "*SLO", "*NOP", " ORA", " ASL", "*SLO", " PHP", " ORA", " ASL", " ???", "*NOP", " ORA", " ASL", "*SLO",
	" BPL", " ORA", "*HLT", "*SLO", "*NOP", " ORA", " ASL", "*SLO", " CLC", " ORA", "*NOP", "*SLO", "*NOP", " ORA", " ASL", "*SLO",
	" JSR", " AND", "*HLT", "*RLA", " BIT", " AND", " ROL", "*RLA", " PLP", " AND", " ROL", " ???", " BIT", " AND", " ROL", "*RLA",
	" BMI", " AND", "*HLT", "*RLA", "*NOP", " AND", " ROL", "*RLA", " SEC", " AND", "*NOP", "*RLA", "*NOP", " AND", " ROL", "*RLA",
	" RTI", " EOR", "*HLT", "*SRE", "*NOP", " EOR", " LSR", "*SRE", " PHA", " EOR", " LSR", " ???", " JMP", " EOR", " LSR", "*SRE",
	" BVC", " EOR", "*HLT", "*SRE", "*NOP", " EOR", " LSR", "*SRE", " CLI", " EOR", "*NOP", "*SRE", "*NOP", " EOR", " LSR", "*SRE",
	" RTS", " ADC", "*HLT", "*RRA", "*NOP", " ADC", " ROR", "*RRA", " PLA", " ADC", " ROR", " ???", " JMP", " ADC", " ROR", "*RRA",
	" BVS", " ADC", "*HLT", "*RRA", "*NOP", " ADC", " ROR", "*RRA", " SEI", " ADC", "*NOP", "*RRA", "*NOP", " ADC", " ROR", "*RRA",
	"*NOP", " STA", "*NOP", "*SAX", " STY", " STA", " STX", "*SAX", " DEY", "*NOP", " TXA", " ???", " STY", " STA", " STX", "*SAX",
	" BCC", " STA", "*HLT", " ???", " STY", " STA", " STX", "*SAX", " TYA", " STA", " TXS", " ???", " ???", " STA", " ???", " ???",
	" LDY", " LDA", " LDX", "*LAX", " LDY", " LDA", " LDX", "*LAX", " TAY", " LDA", " TAX", " ???", " LDY", " LDA", " LDX", "*LAX",
	" BCS", " LDA", "*HLT", "*LAX", " LDY", " LDA", " LDX", "*LAX", " CLV", " LDA", " TSX", " ???", " LDY", " LDA", " LDX", "*LAX",
	" CPY", " CMP", "*NOP", "*DCP", " CPY", " CMP", " DEC", "*DCP", " INY", " CMP", " DEX", " ???", " CPY", " CMP", " DEC", "*DCP",
	" BNE", " CMP", "*HLT", "*DCP", "*NOP", " CMP", " DEC", "*DCP", " CLD", " CMP", "*NOP", "*DCP", "*NOP", " CMP", " DEC", "*DCP",
	" CPX", " SBC", "*NOP", "*ISB", " CPX", " SBC", " INC", "*ISB", " INX", " SBC", " NOP", "*SBC", " CPX", " SBC", " INC", "*ISB",
	" BEQ", " SBC", "*HLT", "*ISB", "*NOP", " SBC", " INC", "*ISB", " SED", " SBC", "*NOP", "*ISB", "*NOP", " SBC", " INC", "*ISB"
};

#define	BP_NA (0)
#define	BP_RD (DEBUG_BREAK_READ)
#define	BP_WR (DEBUG_BREAK_WRITE)
#define	BP_RW (DEBUG_BREAK_READ | DEBUG_BREAK_WRITE)

unsigned char TraceIO[256] =
{
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RD, BP_NA, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_RD, BP_WR, BP_RD, BP_WR, BP_WR, BP_WR, BP_WR, BP_WR, BP_NA, BP_RD, BP_NA, BP_RD, BP_WR, BP_WR, BP_WR, BP_WR,
	BP_NA, BP_WR, BP_NA, BP_RD, BP_WR, BP_WR, BP_WR, BP_WR, BP_NA, BP_WR, BP_NA, BP_RD, BP_RD, BP_WR, BP_RD, BP_RD,
	BP_RD, BP_RD, BP_RD, BP_RD, BP_RD, BP_RD, BP_RD, BP_RD, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RD, BP_RD,
	BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RD, BP_RD, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RD, BP_RD,
	BP_RD, BP_RD, BP_RD, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_RD, BP_RD, BP_RD, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW
};

enum {
	D_PAL_W = 256,
	D_PAL_H = 32,

	D_PAT_W = 256,
	D_PAT_H = 128,

	D_NAM_W = 256,
	D_NAM_H = 240,

	D_SPR_W = 256,
	D_SPR_H = 88,

	D_TIL_W = 64,
	D_TIL_H = 64,
};

INT_PTR CALLBACK CPUProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PPUProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void	Debugger_StartLogging (void);
void	Debugger_StopLogging (void);
void	Debugger_CacheBreakpoints (void);

void	Debugger_Init (void)
{
	HDC TpHDC = GetDC(hMainWnd);
	Debugger.Depth = GetDeviceCaps(TpHDC, BITSPIXEL);
	ReleaseDC(hMainWnd, TpHDC);

	Debugger.Enabled = FALSE;
	Debugger.Mode = 0;

	Debugger.NTabChanged = Debugger.PalChanged = Debugger.PatChanged = Debugger.SprChanged = Debugger.DetChanged = FALSE;
	Debugger.DetailType = Debugger.DetailNum = 0;
	Debugger.DetailTypeSave = Debugger.DetailNumSave = 0;
	
	Debugger.Logging = FALSE;
	Debugger.Step = FALSE;

	Debugger.Palette = 0;
	Debugger.Nametable = 0;

	TpHDC = GetWindowDC(GetDesktopWindow());

	Debugger.PaletteDC = CreateCompatibleDC(TpHDC);
	Debugger.PaletteBMP = CreateCompatibleBitmap(TpHDC, D_PAL_W, D_PAL_H);
	SelectObject(Debugger.PaletteDC, Debugger.PaletteBMP);
	BitBlt(Debugger.PaletteDC, 0, 0, D_PAL_W, D_PAL_H, NULL, 0, 0, BLACKNESS);

	Debugger.PatternDC = CreateCompatibleDC(TpHDC);
	Debugger.PatternBMP = CreateCompatibleBitmap(TpHDC, D_PAT_W, D_PAT_H);
	SelectObject(Debugger.PatternDC, Debugger.PatternBMP);
	BitBlt(Debugger.PatternDC, 0, 0, D_PAT_W, D_PAT_H, NULL, 0, 0, BLACKNESS);

	Debugger.NameDC = CreateCompatibleDC(TpHDC);
	Debugger.NameBMP = CreateCompatibleBitmap(TpHDC, D_NAM_W, D_NAM_H);
	SelectObject(Debugger.NameDC, Debugger.NameBMP);
	BitBlt(Debugger.NameDC, 0, 0, D_NAM_W, D_NAM_H, NULL, 0, 0, BLACKNESS);

	Debugger.SpriteDC = CreateCompatibleDC(TpHDC);
	Debugger.SpriteBMP = CreateCompatibleBitmap(TpHDC, D_SPR_W, D_SPR_H);
	SelectObject(Debugger.SpriteDC, Debugger.SpriteBMP);
	BitBlt(Debugger.SpriteDC, 0, 0, D_SPR_W, D_SPR_H, NULL, 0, 0, BLACKNESS);

	Debugger.TileDC = CreateCompatibleDC(TpHDC);
	Debugger.TileBMP = CreateCompatibleBitmap(TpHDC, D_TIL_W, D_TIL_H);
	SelectObject(Debugger.TileDC, Debugger.TileBMP);
	BitBlt(Debugger.TileDC, 0, 0, D_TIL_W, D_TIL_H, NULL, 0, 0, BLACKNESS);

	Debugger.CPUWnd = NULL;
	Debugger.PPUWnd = NULL;

	Debugger.Breakpoints = NULL;
	Debugger.TraceOffset = -1;
	Debugger.MemOffset = 0;

	Debugger.LogFile = NULL;
	Debugger_CacheBreakpoints();
}

void	Debugger_Release (void)
{
	Debugger_StopLogging();
	Debugger_SetMode(0);
	while (Debugger.Breakpoints != NULL)
	{
		Debugger.Breakpoints = Debugger.Breakpoints->next;
		free(Debugger.Breakpoints->prev);
	}
}

void	Debugger_SetMode (int NewMode)
{
	RECT wRect;
	GetWindowRect(hMainWnd,&wRect);

	Debugger.Mode = NewMode;
	Debugger.Enabled = (Debugger.Mode > 0) ? TRUE : FALSE;

	if ((Debugger.Mode & DEBUG_MODE_CPU) && !Debugger.CPUWnd)
	{
		Debugger.CPUWnd = CreateDialog(hInst, (LPCTSTR)IDD_DEBUGGER_CPU, hMainWnd, CPUProc);
		SetWindowPos(Debugger.CPUWnd, hMainWnd, wRect.right, wRect.top, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOOWNERZORDER);
	}
	else if (!(Debugger.Mode & DEBUG_MODE_CPU) && Debugger.CPUWnd)
	{
		DestroyWindow(Debugger.CPUWnd);
		Debugger.CPUWnd = NULL;
	}
	if ((Debugger.Mode & DEBUG_MODE_PPU) && !Debugger.PPUWnd)
	{
		Debugger.PPUWnd = CreateDialog(hInst, (LPCTSTR)IDD_DEBUGGER_PPU, hMainWnd, PPUProc);
		SetWindowPos(Debugger.PPUWnd, hMainWnd, wRect.left, wRect.bottom, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOOWNERZORDER);
		Debugger.NTabChanged = TRUE;
		Debugger.PalChanged = TRUE;
		Debugger.PatChanged = TRUE;
		Debugger.SprChanged = TRUE;
	}
	else if (!(Debugger.Mode & DEBUG_MODE_PPU) && Debugger.PPUWnd)
	{
		DestroyWindow(Debugger.PPUWnd);
		Debugger.PPUWnd = NULL;
	}

	if (Debugger.Mode & DEBUG_MODE_CPU)
		CheckMenuItem(hMenu,ID_DEBUG_CPU,MF_CHECKED);
	else	CheckMenuItem(hMenu,ID_DEBUG_CPU,MF_UNCHECKED);
	if (Debugger.Mode & DEBUG_MODE_PPU)
		CheckMenuItem(hMenu,ID_DEBUG_PPU,MF_CHECKED);
	else	CheckMenuItem(hMenu,ID_DEBUG_PPU,MF_UNCHECKED);

	Debugger_Update();
	SetFocus(hMainWnd);
}

void	Debugger_StartLogging (void)
{
	TCHAR filename[MAX_PATH];
	struct tm *newtime;
	time_t aclock;

	if (!NES.ROMLoaded)
		return;

	time(&aclock);
	newtime = localtime(&aclock);

	_stprintf(filename, _T("%s\\Dumps\\%s.%04i%02i%02i_%02i%02i%02i.debug"), DataPath, States.BaseFilename,
		newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);

	Debugger.Logging = TRUE;
	Debugger.LogFile = _tfopen(filename, _T("a+"));
}

void	Debugger_StopLogging (void)
{
	if (Debugger.Logging)
		fclose(Debugger.LogFile);
	Debugger.Logging = FALSE;
}

unsigned char DebugMemCPU (unsigned short Addr)
{
	int Bank = (Addr >> 12) & 0xF;
	FCPURead Read = CPU.ReadHandler[Bank];
	if ((Read == CPU_ReadRAM) || (Read == CPU_ReadPRG) || (Read == GenieRead) || (Read == GenieRead1) || (Read == GenieRead2) || (Read == GenieRead3))
		return (unsigned char)Read(Bank, Addr & 0xFFF);
	else	return 0xFF;
}

unsigned char DebugMemPPU (unsigned short Addr)
{
	int Bank = (Addr >> 10) & 0xF;
	FPPURead Read = PPU.ReadHandler[Bank];
	if (Read == PPU_BusRead)
		return (unsigned char)Read(Bank, Addr & 0x3FF);
	else	return 0xFF;
}

/* Decodes an instruction into plain text, suitable for displaying in the debugger or writing to a logfile
 * Returns the effective address for usage with breakpoints */
unsigned short	DecodeInstruction (unsigned short Addr, char *str1, TCHAR *str2)
{
	unsigned char OpData[3] = {0, 0, 0};
	unsigned short Operand = 0, MidAddr = 0, EffectiveAddr = 0;
	OpData[0] = DebugMemCPU(Addr);
	switch (TraceAddrMode[OpData[0]])
	{
	case IND:
		OpData[1] = DebugMemCPU(Addr+1);
		OpData[2] = DebugMemCPU(Addr+2);
		Operand = OpData[1] | (OpData[2] << 8);
		EffectiveAddr = DebugMemCPU(Operand) | (DebugMemCPU(Operand+1) << 8);
		break;
	case ADR:
		OpData[1] = DebugMemCPU(Addr+1);
		OpData[2] = DebugMemCPU(Addr+2);Operand = OpData[1] | (OpData[2] << 8);
		break;
	case ABS:
		OpData[1] = DebugMemCPU(Addr+1);
		OpData[2] = DebugMemCPU(Addr+2);Operand = OpData[1] | (OpData[2] << 8);
		break;
	case ABX:
		OpData[1] = DebugMemCPU(Addr+1);
		OpData[2] = DebugMemCPU(Addr+2);Operand = OpData[1] | (OpData[2] << 8);
		EffectiveAddr = Operand + CPU.X;
		break;
	case ABY:
		OpData[1] = DebugMemCPU(Addr+1);
		OpData[2] = DebugMemCPU(Addr+2);Operand = OpData[1] | (OpData[2] << 8);
		EffectiveAddr = Operand + CPU.Y;
		break;
	case IMM:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = OpData[1];
		break;
	case ZPG:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = OpData[1];
		break;
	case ZPX:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = OpData[1];
		EffectiveAddr = (Operand + CPU.X) & 0xFF;
		break;
	case ZPY:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = OpData[1];
		EffectiveAddr = (Operand + CPU.Y) & 0xFF;
		break;
	case INX:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = OpData[1];
		MidAddr = (Operand + CPU.X) & 0xFF;
		EffectiveAddr = DebugMemCPU(MidAddr) | (DebugMemCPU((MidAddr+1) & 0xFF) << 8);
		break;
	case INY:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = OpData[1];
		MidAddr = DebugMemCPU(Operand) | (DebugMemCPU((Operand+1) & 0xFF) << 8);
		EffectiveAddr = MidAddr + CPU.Y;
		break;
	case IMP:
		break;
	case ACC:
		break;
	case ERR:
		break;
	case REL:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = Addr+2+(signed char)OpData[1];
		break;
	}
	/* Use this for outputting to debug logfile */
	if (str1)
	{
		switch (TraceAddrMode[DebugMemCPU(Addr)])
		{
		case IMP:
		case ERR:	sprintf(str1, "%04X  %02X       %s                           ",			Addr, OpData[0],			TraceArr[OpData[0]]);									break;
		case ACC:	sprintf(str1, "%04X  %02X       %s A                         ",			Addr, OpData[0],			TraceArr[OpData[0]]);									break;
		case IMM:	sprintf(str1, "%04X  %02X %02X    %s #$%02X                      ",		Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand);								break;
		case REL:	sprintf(str1, "%04X  %02X %02X    %s $%04X                     ",		Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand);								break;
		case ZPG:	sprintf(str1, "%04X  %02X %02X    %s $%02X = %02X                  ",		Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand, DebugMemCPU(Operand));					break;
		case ZPX:	sprintf(str1, "%04X  %02X %02X    %s $%02X,X @ %02X = %02X           ",		Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand, EffectiveAddr, DebugMemCPU(EffectiveAddr));		break;
		case ZPY:	sprintf(str1, "%04X  %02X %02X    %s $%02X,Y @ %02X = %02X           ",		Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand, EffectiveAddr, DebugMemCPU(EffectiveAddr));		break;
		case INX:	sprintf(str1, "%04X  %02X %02X    %s ($%02X,X) @ %02X = %04X = %02X  ",		Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand, MidAddr, EffectiveAddr, DebugMemCPU(EffectiveAddr));	break;
		case INY:	sprintf(str1, "%04X  %02X %02X    %s ($%02X),Y = %04X @ %04X = %02X",		Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand, MidAddr, EffectiveAddr, DebugMemCPU(EffectiveAddr));	break;
		case ADR:	sprintf(str1, "%04X  %02X %02X %02X %s $%04X                     ",		Addr, OpData[0], OpData[1], OpData[2],	TraceArr[OpData[0]], Operand);								break;
		case ABS:	sprintf(str1, "%04X  %02X %02X %02X %s $%04X = %02X                ",		Addr, OpData[0], OpData[1], OpData[2],	TraceArr[OpData[0]], Operand, DebugMemCPU(Operand));					break;
		case IND:	sprintf(str1, "%04X  %02X %02X %02X %s ($%04X) = %04X            ",		Addr, OpData[0], OpData[1], OpData[2],	TraceArr[OpData[0]], Operand, EffectiveAddr);						break;
		case ABX:	sprintf(str1, "%04X  %02X %02X %02X %s $%04X,X @ %04X = %02X       ",		Addr, OpData[0], OpData[1], OpData[2],	TraceArr[OpData[0]], Operand, EffectiveAddr, DebugMemCPU(EffectiveAddr));		break;
		case ABY:	sprintf(str1, "%04X  %02X %02X %02X %s $%04X,Y @ %04X = %02X       ",		Addr, OpData[0], OpData[1], OpData[2],	TraceArr[OpData[0]], Operand, EffectiveAddr, DebugMemCPU(EffectiveAddr));		break;
		default:	strcpy(str1, "                                             ");																			break;
		}
	}
	/* Use this for outputting to the debugger's Trace pane */
	if (str2)
	{
		switch (TraceAddrMode[DebugMemCPU(Addr)])
		{
		case IMP:
		case ERR:	_stprintf(str2, _T("%04X\t%02X\t%hs"),						Addr, OpData[0],			TraceArr[OpData[0]]);									break;
		case ACC:	_stprintf(str2, _T("%04X\t%02X\t%hs\tA"),					Addr, OpData[0],			TraceArr[OpData[0]]);									break;
		case IMM:	_stprintf(str2, _T("%04X\t%02X %02X\t%hs\t#$%02X"),				Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand);								break;
		case REL:	_stprintf(str2, _T("%04X\t%02X %02X\t%hs\t$%04X"),				Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand);								break;
		case ZPG:	_stprintf(str2, _T("%04X\t%02X %02X\t%hs\t$%02X = %02X"),			Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand, DebugMemCPU(Operand));					break;
		case ZPX:	_stprintf(str2, _T("%04X\t%02X %02X\t%hs\t$%02X,X @ %02X = %02X"),		Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand, EffectiveAddr, DebugMemCPU(EffectiveAddr));		break;
		case ZPY:	_stprintf(str2, _T("%04X\t%02X %02X\t%hs\t$%02X,Y @ %02X = %02X"),		Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand, EffectiveAddr, DebugMemCPU(EffectiveAddr));		break;
		case INX:	_stprintf(str2, _T("%04X\t%02X %02X\t%hs\t($%02X,X) @ %02X = %04X = %02X"),	Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand, MidAddr, EffectiveAddr, DebugMemCPU(EffectiveAddr));	break;
		case INY:	_stprintf(str2, _T("%04X\t%02X %02X\t%hs\t($%02X),Y = %04X @ %04X = %02X"),	Addr, OpData[0], OpData[1],		TraceArr[OpData[0]], Operand, MidAddr, EffectiveAddr, DebugMemCPU(EffectiveAddr));	break;
		case ADR:	_stprintf(str2, _T("%04X\t%02X %02X %02X\t%hs\t$%04X"),				Addr, OpData[0], OpData[1], OpData[2],	TraceArr[OpData[0]], Operand);								break;
		case ABS:	_stprintf(str2, _T("%04X\t%02X %02X %02X\t%hs\t$%04X = %02X"),			Addr, OpData[0], OpData[1], OpData[2],	TraceArr[OpData[0]], Operand, DebugMemCPU(Operand));					break;
		case IND:	_stprintf(str2, _T("%04X\t%02X %02X %02X\t%hs\t($%04X) = %04X"),		Addr, OpData[0], OpData[1], OpData[2],	TraceArr[OpData[0]], Operand, EffectiveAddr);						break;
		case ABX:	_stprintf(str2, _T("%04X\t%02X %02X %02X\t%hs\t$%04X,X @ %04X = %02X"),		Addr, OpData[0], OpData[1], OpData[2],	TraceArr[OpData[0]], Operand, EffectiveAddr, DebugMemCPU(EffectiveAddr));		break;
		case ABY:	_stprintf(str2, _T("%04X\t%02X %02X %02X\t%hs\t$%04X,Y @ %04X = %02X"),		Addr, OpData[0], OpData[1], OpData[2],	TraceArr[OpData[0]], Operand, EffectiveAddr, DebugMemCPU(EffectiveAddr));		break;
		default :	_tcscpy(str2, _T(""));																								break;
		}
	}
	return EffectiveAddr;
}

void	Debugger_UpdateCPU (void)
{
	TCHAR tps[64];
	int i, TpVal;
	unsigned short Addr;
	SCROLLINFO sinfo;

	/* if we chose "Step", stop emulation */
	if (Debugger.Step)
		NES.Stop = TRUE;
	/* check for breakpoints */
	{
		/* PC has execution breakpoint */
		if (Debugger.BPcache[CPU.PC] & DEBUG_BREAK_EXEC)
			NES.Stop = TRUE;
		/* I/O break */
		Addr = DecodeInstruction((unsigned short)CPU.PC, NULL, NULL);
		TpVal = DebugMemCPU((unsigned short)CPU.PC);
		/* read opcode, effective address has read breakpoint */
		if ((TraceIO[TpVal] & DEBUG_BREAK_READ) && (Debugger.BPcache[Addr] & DEBUG_BREAK_READ))
			NES.Stop = TRUE;
		/* write opcode, effective address has write breakpoint */
		if ((TraceIO[TpVal] & DEBUG_BREAK_WRITE) && (Debugger.BPcache[Addr] & DEBUG_BREAK_WRITE))
			NES.Stop = TRUE;
		/* opcode breakpoint */
		if (Debugger.BPcache[0x10000 | TpVal] & DEBUG_BREAK_OPCODE)
			NES.Stop = TRUE;
		/* interrupt breakpoints */
		if ((CPU.GotInterrupt == INTERRUPT_NMI) && (Debugger.BPcache[0x10100] & DEBUG_BREAK_NMI))
			NES.Stop = TRUE;
/*		if ((CPU.GotInterrupt == INTERRUPT_RST) && (Debugger.BPcache[0x10100] & DEBUG_BREAK_RST))
			NES.Stop = TRUE;*/
		if ((CPU.GotInterrupt == INTERRUPT_IRQ) && (Debugger.BPcache[0x10100] & DEBUG_BREAK_IRQ))
			NES.Stop = TRUE;
		if ((CPU.GotInterrupt == INTERRUPT_BRK) && (Debugger.BPcache[0x10100] & DEBUG_BREAK_BRK))
			NES.Stop = TRUE;

	}
	/* if emulation wasn't stopped, don't bother updating the dialog */
	if (!NES.Stop)
		return;

	inUpdate = TRUE;

	_stprintf(tps, _T("%04X"), CPU.PC);
	SetDlgItemText(Debugger.CPUWnd, IDC_DEBUG_REG_PC, tps);

	_stprintf(tps, _T("%02X"), CPU.A);
	SetDlgItemText(Debugger.CPUWnd, IDC_DEBUG_REG_A, tps);

	_stprintf(tps, _T("%02X"), CPU.X);
	SetDlgItemText(Debugger.CPUWnd, IDC_DEBUG_REG_X, tps);

	_stprintf(tps, _T("%02X"), CPU.Y);
	SetDlgItemText(Debugger.CPUWnd, IDC_DEBUG_REG_Y, tps);

	_stprintf(tps, _T("%02X"), CPU.SP);
	SetDlgItemText(Debugger.CPUWnd, IDC_DEBUG_REG_SP, tps);

	CPU_JoinFlags();
	_stprintf(tps, _T("%02X"), CPU.P);
	SetDlgItemText(Debugger.CPUWnd, IDC_DEBUG_REG_P, tps);

	CheckDlgButton(Debugger.CPUWnd, IDC_DEBUG_FLAG_N, CPU.FN ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(Debugger.CPUWnd, IDC_DEBUG_FLAG_V, CPU.FV ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(Debugger.CPUWnd, IDC_DEBUG_FLAG_D, CPU.FD ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(Debugger.CPUWnd, IDC_DEBUG_FLAG_I, CPU.FI ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(Debugger.CPUWnd, IDC_DEBUG_FLAG_Z, CPU.FZ ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(Debugger.CPUWnd, IDC_DEBUG_FLAG_C, CPU.FC ? BST_CHECKED : BST_UNCHECKED);

	CheckDlgButton(Debugger.CPUWnd, IDC_DEBUG_IRQ_EXT, (CPU.WantIRQ & IRQ_EXTERNAL) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(Debugger.CPUWnd, IDC_DEBUG_IRQ_PCM, (CPU.WantIRQ & IRQ_DPCM) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(Debugger.CPUWnd, IDC_DEBUG_IRQ_FRAME, (CPU.WantIRQ & IRQ_FRAME) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(Debugger.CPUWnd, IDC_DEBUG_IRQ_DEBUG, (CPU.WantIRQ & IRQ_DEBUG) ? BST_CHECKED : BST_UNCHECKED);

	SetDlgItemInt(Debugger.CPUWnd, IDC_DEBUG_TIMING_SCANLINE, PPU.SLnum, TRUE);

	_stprintf(tps, _T("%04X"), PPU.VRAMAddr);
	SetDlgItemText(Debugger.CPUWnd, IDC_DEBUG_TIMING_VRAM, tps);

	_stprintf(tps, _T("%i/%.3f"), PPU.Clockticks, PPU.Clockticks / (PPU.IsPAL ? 3.2 : 3.0));
	SetDlgItemText(Debugger.CPUWnd, IDC_DEBUG_TIMING_CPU, tps);

	for (i = 0; i < 16; i++)
	{
		const int CPUPages[16] = {
			IDC_DEBUG_BANK_CPU0, IDC_DEBUG_BANK_CPU1, IDC_DEBUG_BANK_CPU2, IDC_DEBUG_BANK_CPU3, 
			IDC_DEBUG_BANK_CPU4, IDC_DEBUG_BANK_CPU5, IDC_DEBUG_BANK_CPU6, IDC_DEBUG_BANK_CPU7, 
			IDC_DEBUG_BANK_CPU8, IDC_DEBUG_BANK_CPU9, IDC_DEBUG_BANK_CPUA, IDC_DEBUG_BANK_CPUB, 
			IDC_DEBUG_BANK_CPUC, IDC_DEBUG_BANK_CPUD, IDC_DEBUG_BANK_CPUE, IDC_DEBUG_BANK_CPUF
		};

		if (EI.GetPRG_ROM4(i) >= 0)
			_stprintf(tps, _T("%03X"), EI.GetPRG_ROM4(i));
		else if (EI.GetPRG_RAM4(i) >= 0)
			_stprintf(tps, _T("A%02X"), EI.GetPRG_RAM4(i));
		else	_stprintf(tps, _T("???"));
		SetDlgItemText(Debugger.CPUWnd, CPUPages[i], tps);
	}
	for (i = 0; i < 12; i++)
	{
		const int PPUPages[16] = {
			IDC_DEBUG_BANK_PPU00, IDC_DEBUG_BANK_PPU04, IDC_DEBUG_BANK_PPU08, IDC_DEBUG_BANK_PPU0C, 
			IDC_DEBUG_BANK_PPU10, IDC_DEBUG_BANK_PPU14, IDC_DEBUG_BANK_PPU18, IDC_DEBUG_BANK_PPU1C, 
			IDC_DEBUG_BANK_PPU20, IDC_DEBUG_BANK_PPU24, IDC_DEBUG_BANK_PPU28, IDC_DEBUG_BANK_PPU2C
		};

		if (EI.GetCHR_ROM1(i) >= 0)
			_stprintf(tps, _T("%03X"), EI.GetCHR_ROM1(i));
		else if (EI.GetCHR_RAM1(i) >= 0)
			_stprintf(tps, _T("A%02X"), EI.GetCHR_RAM1(i));
		else if (EI.GetCHR_NT1(i) >= 0)
			_stprintf(tps, _T("N%02X"), EI.GetCHR_NT1(i));
		else	_stprintf(tps, _T("???"));
		SetDlgItemText(Debugger.CPUWnd, PPUPages[i], tps);
	}

	/* update trace window - turn off redraw */
	SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_TRACE_LIST, WM_SETREDRAW, FALSE, 0);
	SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_TRACE_LIST, LB_RESETCONTENT, 0, 0);

	Addr = (unsigned short)((Debugger.TraceOffset == -1) ? CPU.PC : Debugger.TraceOffset);
	TpVal = -1;

	sinfo.cbSize = sizeof(SCROLLINFO);
	sinfo.fMask = SIF_POS;
	sinfo.nPos = Addr;
	SetScrollInfo(GetDlgItem(Debugger.CPUWnd, IDC_DEBUG_TRACE_SCROLL), SB_CTL, &sinfo, TRUE);

	for (i = 0; i < DEBUG_TRACELINES; i++)
	{
		if (Addr == CPU.PC)
			TpVal = i;
		DecodeInstruction(Addr, NULL, tps);
		SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_TRACE_LIST, LB_ADDSTRING, 0, (LPARAM)tps);
		Addr = Addr + AddrBytes[TraceAddrMode[DebugMemCPU(Addr)]];
	}
	/* re-enable redraw just before we set the selection */
	SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_TRACE_LIST, WM_SETREDRAW, TRUE, 0);
	SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_TRACE_LIST, LB_SETCURSEL, TpVal, 0);

	/* update memory window - turn off redraw */
	SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_MEM_LIST, WM_SETREDRAW, FALSE, 0);
	SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_MEM_LIST, LB_RESETCONTENT, 0, 0);

	if (IsDlgButtonChecked(Debugger.CPUWnd, IDC_DEBUG_MEM_CPU))
	{
		sinfo.cbSize = sizeof(SCROLLINFO);
		sinfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
		sinfo.nPos = Debugger.MemOffset;
		sinfo.nPage = 0x080;
		sinfo.nMin = 0;
		sinfo.nMax = 0xFFF;
		SetScrollInfo(GetDlgItem(Debugger.CPUWnd, IDC_DEBUG_MEM_SCROLL), SB_CTL, &sinfo, TRUE);
		for (i = 0; i < DEBUG_MEMLINES; i++)
		{
			/* past the end? */
			if ((Debugger.MemOffset + i) * 0x10 >= 0x10000)
				break;
			_stprintf(tps, _T("%04X:\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X"),
				(Debugger.MemOffset + i) * 0x10,
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x0)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x1)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x2)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x3)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x4)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x5)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x6)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x7)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x8)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x9)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0xA)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0xB)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0xC)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0xD)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0xE)),
				DebugMemCPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0xF)));
			SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_MEM_LIST, LB_ADDSTRING, 0, (LPARAM)tps);
		}
	}
	else if (IsDlgButtonChecked(Debugger.CPUWnd, IDC_DEBUG_MEM_PPU))
	{
		sinfo.cbSize = sizeof(SCROLLINFO);
		sinfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
		sinfo.nPos = Debugger.MemOffset;
		sinfo.nPage = 0x080;
		sinfo.nMin = 0;
		sinfo.nMax = 0x3FF;
		SetScrollInfo(GetDlgItem(Debugger.CPUWnd, IDC_DEBUG_MEM_SCROLL), SB_CTL, &sinfo, TRUE);
		for (i = 0; i < DEBUG_MEMLINES; i++)
		{
			/* past the end? */
			if ((Debugger.MemOffset + i) * 0x10 >= 0x4000)
				break;
			_stprintf(tps, _T("%04X:\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X"),
				(Debugger.MemOffset + i) * 0x10,
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x0)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x1)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x2)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x3)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x4)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x5)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x6)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x7)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x8)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0x9)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0xA)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0xB)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0xC)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0xD)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0xE)),
				DebugMemPPU((unsigned short)((Debugger.MemOffset + i) * 0x10 + 0xF)));
			SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_MEM_LIST, LB_ADDSTRING, 0, (LPARAM)tps);
		}
	}
	else if (IsDlgButtonChecked(Debugger.CPUWnd, IDC_DEBUG_MEM_SPR))
	{
		sinfo.cbSize = sizeof(SCROLLINFO);
		sinfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
		sinfo.nPos = Debugger.MemOffset;
		sinfo.nPage = 0x8;
		sinfo.nMin = 0;
		sinfo.nMax = 0xF;
		SetScrollInfo(GetDlgItem(Debugger.CPUWnd, IDC_DEBUG_MEM_SCROLL), SB_CTL, &sinfo, TRUE);
		for (i = 0; i < DEBUG_MEMLINES; i++)
		{
			/* past the end? */
			if ((Debugger.MemOffset + i) * 0x10 >= 0x100)
				break;
			_stprintf(tps, _T("%02X:\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X"),
				(Debugger.MemOffset + i) * 0x10,
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0x0],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0x1],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0x2],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0x3],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0x4],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0x5],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0x6],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0x7],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0x8],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0x9],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0xA],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0xB],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0xC],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0xD],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0xE],
				PPU.Sprite[(Debugger.MemOffset + i) * 0x10 + 0xF]);
			SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_MEM_LIST, LB_ADDSTRING, 0, (LPARAM)tps);
		}
	}
	else if (IsDlgButtonChecked(Debugger.CPUWnd, IDC_DEBUG_MEM_PAL))
	{
		sinfo.cbSize = sizeof(SCROLLINFO);
		sinfo.fMask = SIF_DISABLENOSCROLL | SIF_RANGE;
		sinfo.nPage = 0;
		sinfo.nMin = 0;
		sinfo.nMax = 0;
		SetScrollInfo(GetDlgItem(Debugger.CPUWnd, IDC_DEBUG_MEM_SCROLL), SB_CTL, &sinfo, TRUE);
		for (i = 0; i < DEBUG_MEMLINES; i++)
		{
			/* past the end? */
			if ((Debugger.MemOffset + i) * 0x10 >= 0x20)
				break;
			_stprintf(tps, _T("%02X:\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X"),
				(Debugger.MemOffset + i) * 0x10,
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0x0],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0x1],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0x2],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0x3],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0x4],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0x5],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0x6],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0x7],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0x8],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0x9],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0xA],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0xB],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0xC],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0xD],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0xE],
				PPU.Palette[(Debugger.MemOffset + i) * 0x10 + 0xF]);
			SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_MEM_LIST, LB_ADDSTRING, 0, (LPARAM)tps);
		}
	}

	/* re-enable redraw now that we're done drawing it */
	SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_MEM_LIST, WM_SETREDRAW, TRUE, 0);
	inUpdate = FALSE;
}

void	Debugger_AddInst (void)
{
	if (Debugger.Logging)
	{
		unsigned short Addr = (unsigned short)CPU.PC;
		char tps[64];
		DecodeInstruction(Addr, tps, NULL);
		fwrite(tps, 1, strlen(tps), Debugger.LogFile);
		CPU_JoinFlags();
		sprintf(tps, "  A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3i SL:%i\n", CPU.A, CPU.X, CPU.Y, CPU.P, CPU.SP, PPU.Clockticks, PPU.SLnum);
		fwrite(tps, 1, strlen(tps), Debugger.LogFile);
	}
}

void	Debug_DrawTile (unsigned long *dest, int PPUaddr, int palette, int width, int height, int pitch)
{
	int sy, sx, py, px;
	int byte0, byte1, color;
	for (sy = 0; sy < 8; sy++)
	{
		byte0 = DebugMemPPU((unsigned short)(PPUaddr + sy));
		byte1 = DebugMemPPU((unsigned short)(PPUaddr + sy + 8));
		for (sx = 0; sx < 8; sx++)
		{
			color = ((byte0 & 0x80) >> 7) | ((byte1 & 0x80) >> 6);
			byte0 <<= 1;
			byte1 <<= 1;
			if (color)
				color |= palette << 2;
			color = GFX.Palette32[PPU.Palette[color]];
			if ((width == 1) && (height == 1))
				dest[sx] = color;
			else
			{
				for (py = 0; py < height; py++)
					for (px = 0; px < width; px++)
						dest[px + sx * width + py * pitch] = color;
			}
		}
		dest += pitch * height;
	}
}

void	Debugger_UpdatePPU (void)
{
	int MemAddr;
	int x, y;

	if (Debugger.PalChanged)
	{
		unsigned char color;
		/* updating palette also invalidates pattern tables, nametable, and sprites */
		Debugger.PatChanged = TRUE;
		Debugger.NTabChanged = TRUE;
		Debugger.SprChanged = TRUE;
		if (Debugger.DetailType == DEBUG_DETAIL_PALETTE)
			Debugger.DetChanged = TRUE;

		for (x = 0; x < 16; x++)
		{
			for (y = 0; y < 2; y++)
			{
				HBRUSH brush;
				RECT rect;
				color = PPU.Palette[y * 16 + x];
				rect.top = y * (D_PAL_H / 2);
				rect.bottom = rect.top + (D_PAL_H / 2);
				rect.left = x * (D_PAL_W / 16);
				rect.right = rect.left + (D_PAL_W / 16);
				brush = CreateSolidBrush(RGB(GFX.RawPalette[0][color][0], GFX.RawPalette[0][color][1], GFX.RawPalette[0][color][2]));
				FillRect(Debugger.PaletteDC, &rect, brush);
				DeleteObject(brush);
			}
		}
		RedrawWindow(GetDlgItem(Debugger.PPUWnd, IDC_DEBUG_PPU_PALETTE), NULL, NULL, RDW_INVALIDATE);
		Debugger.PalChanged = FALSE;
	}

	if (Debugger.PatChanged)
	{
		int t;
		unsigned long PatternArray[D_PAT_W * D_PAT_H];
		BITMAPINFO bmi;

		/* updating pattern table also makes nametable and sprites dirty */
		Debugger.NTabChanged = TRUE;
		Debugger.SprChanged = TRUE;
		if (Debugger.DetailType == DEBUG_DETAIL_PATTERN)
			Debugger.DetChanged = TRUE;

		for (t = 0; t < 2; t++)
		{
			for (y = 0; y < 16; y++)
			{
				for (x = 0; x < 16; x++)
				{
					MemAddr = (t << 12) | (y << 8) | (x << 4);
					Debug_DrawTile(PatternArray + y * 8 * D_PAT_W + x * 8 + t * 128, MemAddr, Debugger.Palette, 1, 1, D_PAT_W);
				}
			}
		}

		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth = D_PAT_W;
		bmi.bmiHeader.biHeight = -D_PAT_H;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = 0;
		bmi.bmiHeader.biXPelsPerMeter = 0;
		bmi.bmiHeader.biYPelsPerMeter = 0;
		bmi.bmiHeader.biClrUsed = 0;
		bmi.bmiHeader.biClrImportant = 0;
		SetDIBits(Debugger.PatternDC, Debugger.PatternBMP, 0, D_PAT_H, PatternArray, &bmi, DIB_RGB_COLORS);

		RedrawWindow(GetDlgItem(Debugger.PPUWnd, IDC_DEBUG_PPU_PATTERN), NULL, NULL, RDW_INVALIDATE);
		Debugger.PatChanged = FALSE;
	}

	if (Debugger.NTabChanged)
	{
		int AttribVal, AttribNum;
		int NT = Debugger.Nametable;
		unsigned long NameArray[D_NAM_W * D_NAM_H];
		BITMAPINFO bmi;

		if (Debugger.DetailType == DEBUG_DETAIL_NAMETABLE)
			Debugger.DetChanged = TRUE;

		for (y = 0; y < 30; y++)
		{
			for (x = 0; x < 32; x++)
			{
				AttribNum = (((x & 2) >> 1) | (y & 2)) << 1;
				AttribVal = (PPU.CHRPointer[8 | NT][0x3C0 | ((y << 1) & 0x38) | (x >> 2)] >> AttribNum) & 3;
				MemAddr = ((PPU.Reg2000 & 0x10) << 8) | (PPU.CHRPointer[8 | NT][x | (y << 5)] << 4);
				Debug_DrawTile(NameArray + y * 8 * D_NAM_W + x * 8, MemAddr, AttribVal, 1, 1, D_NAM_W);
			}
		}

		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth = D_NAM_W;
		bmi.bmiHeader.biHeight = -D_NAM_H;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = 0;
		bmi.bmiHeader.biXPelsPerMeter = 0;
		bmi.bmiHeader.biYPelsPerMeter = 0;
		bmi.bmiHeader.biClrUsed = 0;
		bmi.bmiHeader.biClrImportant = 0;
		SetDIBits(Debugger.NameDC, Debugger.NameBMP, 0, D_NAM_H, NameArray, &bmi, DIB_RGB_COLORS);

		RedrawWindow(GetDlgItem(Debugger.PPUWnd, IDC_DEBUG_PPU_NAMETABLE), NULL, NULL, RDW_INVALIDATE);
		Debugger.NTabChanged = FALSE;
	}

	if (Debugger.SprChanged)
	{
		int SprNum, Attr, TileNum;
		unsigned long SprArray[D_SPR_W * D_SPR_H];
		BITMAPINFO bmi;

		if (Debugger.DetailType == DEBUG_DETAIL_SPRITE)
			Debugger.DetChanged = TRUE;

		x = GetSysColor(COLOR_BTNFACE);
		// convert color to RGB (from Windows BGR)
		x = (GetRValue(x) << 16) | (GetGValue(x) << 8) | (GetBValue(x) << 0);
		for (y = 0; y < D_SPR_H * D_SPR_W; y++)
			SprArray[y] = x;

		for (y = 0; y < 4; y++)
		{
			for (x = 0; x < 16; x++)
			{
				SprNum = (y << 4) | x;
				TileNum = PPU.Sprite[(SprNum << 2) | 1];
				Attr = PPU.Sprite[(SprNum << 2) | 2];
				if (PPU.Reg2000 & 0x20)
				{
					MemAddr = ((TileNum & 0xFE) << 4) | ((TileNum & 0x01) << 12);
					Debug_DrawTile(SprArray + y * 24 * D_SPR_W + x * 16, MemAddr, 4 | (Attr & 3), 1, 1, D_SPR_W);
					Debug_DrawTile(SprArray + y * 24 * D_SPR_W + x * 16 + 8 * D_SPR_W, MemAddr + 16, 4 | (Attr & 3), 1, 1, D_SPR_W);
				}
				else
				{
					MemAddr = (TileNum << 4) | ((PPU.Reg2000 & 0x08) << 9);
					Debug_DrawTile(SprArray + y * 24 * D_SPR_W + x * 16, MemAddr, 4 | (Attr & 3), 1, 1, D_SPR_W);
				}
			}
		}

		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth = D_SPR_W;
		bmi.bmiHeader.biHeight = -D_SPR_H;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = 0;
		bmi.bmiHeader.biXPelsPerMeter = 0;
		bmi.bmiHeader.biYPelsPerMeter = 0;
		bmi.bmiHeader.biClrUsed = 0;
		bmi.bmiHeader.biClrImportant = 0;
		SetDIBits(Debugger.SpriteDC, Debugger.SpriteBMP, 0, D_SPR_H, SprArray, &bmi, DIB_RGB_COLORS);

		RedrawWindow(GetDlgItem(Debugger.PPUWnd, IDC_DEBUG_PPU_SPRITE), NULL, NULL, RDW_INVALIDATE);
		Debugger.SprChanged = FALSE;
	}

	if (Debugger.DetChanged)
	{
		unsigned char tile, color;
		TCHAR tpstr[16];
		HBRUSH brush;
		RECT rect;
		unsigned long TileArray[D_TIL_W * D_TIL_H];
		BITMAPINFO bmi;
		BOOL DrawBitmap = FALSE;

		x = GetSysColor(COLOR_BTNFACE);
		// convert color to RGB (from Windows BGR)
		x = (GetRValue(x) << 16) | (GetGValue(x) << 8) | (GetBValue(x) << 0);
		for (y = 0; y < D_TIL_H * D_TIL_W; y++)
			TileArray[y] = x;

		switch (Debugger.DetailType)
		{
		case DEBUG_DETAIL_NONE:
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_SELTYPE, _T("None"));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP1TYPE, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP1VAL, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP2TYPE, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP2VAL, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP3TYPE, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP3VAL, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP4TYPE, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP4VAL, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP5TYPE, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP5VAL, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP6TYPE, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP6VAL, _T(""));

			DrawBitmap = TRUE;
			break;
		case DEBUG_DETAIL_NAMETABLE:
			tile = DebugMemPPU(0x2000 | Debugger.DetailNum);
			color = DebugMemPPU(0x23C0 | (Debugger.DetailNum & 0xC00) | ((Debugger.DetailNum >> 4) & 0x38) | ((Debugger.DetailNum >> 2) & 0x07)) >> ((Debugger.DetailNum & 2) | ((Debugger.DetailNum >> 4) & 4)) & 0x3;

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_SELTYPE, _T("Nametable"));

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP1TYPE, _T("Address"));
			_stprintf(tpstr, _T("%04X"), 0x2000 | Debugger.DetailNum);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP1VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP2TYPE, _T("X"));
			_stprintf(tpstr, _T("%02X"), Debugger.DetailNum & 0x1F);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP2VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP3TYPE, _T("Y"));
			_stprintf(tpstr, _T("%02X"), (Debugger.DetailNum >> 5) & 0x1F);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP3VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP4TYPE, _T("Table"));
			_stprintf(tpstr, _T("%i"), (Debugger.DetailNum >> 10) & 0x3);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP4VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP5TYPE, _T("Tile"));
			_stprintf(tpstr, _T("%02X"), tile);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP5VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP6TYPE, _T("Palette"));
			_stprintf(tpstr, _T("%i"), color);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP6VAL, tpstr);

			MemAddr = (tile << 4) | ((PPU.Reg2000 & 0x10) << 8);
			Debug_DrawTile(TileArray, MemAddr, color, 8, 8, D_TIL_W);
			DrawBitmap = TRUE;
			break;
		case DEBUG_DETAIL_SPRITE:
			tile = PPU.Sprite[(Debugger.DetailNum << 2) | 1];
			color = PPU.Sprite[(Debugger.DetailNum << 2) | 2] & 3;

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_SELTYPE, _T("Sprite"));

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP1TYPE, _T("Number"));
			_stprintf(tpstr, _T("%02X"), Debugger.DetailNum);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP1VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP2TYPE, _T("X"));
			_stprintf(tpstr, _T("%02X"), PPU.Sprite[(Debugger.DetailNum << 2) | 3]);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP2VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP3TYPE, _T("Y"));
			_stprintf(tpstr, _T("%02X"), PPU.Sprite[(Debugger.DetailNum << 2) | 0]);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP3VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP4TYPE, _T("Tile"));
			_stprintf(tpstr, _T("%02X"), tile);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP4VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP5TYPE, _T("Color"));
			_stprintf(tpstr, _T("%i"), color);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP5VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP6TYPE, _T("Flags"));
			tpstr[0] = 0;
			if (PPU.Sprite[(Debugger.DetailNum << 2) | 2] & 0x40)
				_tcscat(tpstr, _T("H "));
			if (PPU.Sprite[(Debugger.DetailNum << 2) | 2] & 0x80)
				_tcscat(tpstr, _T("V "));
			if (PPU.Sprite[(Debugger.DetailNum << 2) | 2] & 0x20)
				_tcscat(tpstr, _T("BG "));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP6VAL, tpstr);

			if (PPU.Reg2000 & 0x20)
			{
				MemAddr = ((tile & 0xFE) << 4) | ((tile & 0x01) << 12);
				Debug_DrawTile(TileArray + 16, MemAddr, color | 4, 4, 4, D_TIL_W);
				Debug_DrawTile(TileArray + 16 + D_TIL_W * 32, MemAddr + 16, color | 4, 4, 4, D_TIL_W);
			}
			else
			{
				MemAddr = (tile << 4) | ((PPU.Reg2000 & 0x08) << 7);
				Debug_DrawTile(TileArray, MemAddr, color | 4, 8, 8, D_TIL_W);
			}
			DrawBitmap = TRUE;
			break;
		case DEBUG_DETAIL_PATTERN:
			tile = Debugger.DetailNum & 0xFF;
			color = Debugger.Palette;
			MemAddr = Debugger.DetailNum << 4;

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_SELTYPE, _T("Pattern Table"));

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP1TYPE, _T("Address"));
			_stprintf(tpstr, _T("%04X"), MemAddr);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP1VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP2TYPE, _T("Tile"));
			_stprintf(tpstr, _T("%02X"), tile);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP2VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP3TYPE, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP3VAL, _T(""));

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP4TYPE, _T("Table"));
			_stprintf(tpstr, _T("%i"), Debugger.DetailNum >> 8);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP4VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP5TYPE, _T("Usage"));
			tpstr[0] = 0;
			if (((PPU.Reg2000 & 0x10) << 4) == (Debugger.DetailNum & 0x100))
				_tcscat(tpstr, _T("BG "));
			if (PPU.Reg2000 & 0x20)
				_tcscat(tpstr, _T("SPR16 "));
			else if (((PPU.Reg2000 & 0x08) << 5) == (Debugger.DetailNum & 0x100))
				_tcscat(tpstr, _T("SPR "));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP5VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP6TYPE, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP6VAL, _T(""));

			Debug_DrawTile(TileArray, MemAddr, color, 8, 8, D_TIL_W);
			DrawBitmap = TRUE;
			break;
		case DEBUG_DETAIL_PALETTE:
			color = PPU.Palette[Debugger.DetailNum];
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_SELTYPE, _T("Palette"));

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP1TYPE, _T("Address"));
			_stprintf(tpstr, _T("%04X"), 0x3F00 | Debugger.DetailNum);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP1VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP2TYPE, _T("ID"));
			_stprintf(tpstr, _T("%s%i"), (Debugger.DetailNum & 0x10) ? _T("SPR") : _T("BG"), (Debugger.DetailNum >> 2) & 0x3);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP2VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP3TYPE, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP3VAL, _T(""));

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP4TYPE, _T("Color"));
			_stprintf(tpstr, _T("%02X"), color);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP4VAL, tpstr);

			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP5TYPE, _T("Offset"));
			_stprintf(tpstr, _T("%i"), Debugger.DetailNum & 0x3);
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP5VAL, tpstr);
			
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP6TYPE, _T(""));
			SetDlgItemText(Debugger.PPUWnd, IDC_DEBUG_PPU_PROP6VAL, _T(""));

			GetClientRect(GetDlgItem(Debugger.PPUWnd, IDC_DEBUG_PPU_TILE), &rect);
			brush = CreateSolidBrush(RGB(GFX.RawPalette[0][color][0], GFX.RawPalette[0][color][1], GFX.RawPalette[0][color][2]));
			FillRect(Debugger.TileDC, &rect, brush);
			DeleteObject(brush);
			break;
		}
		if (DrawBitmap)
		{
			bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
			bmi.bmiHeader.biWidth = D_TIL_W;
			bmi.bmiHeader.biHeight = -D_TIL_H;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;
			bmi.bmiHeader.biSizeImage = 0;
			bmi.bmiHeader.biXPelsPerMeter = 0;
			bmi.bmiHeader.biYPelsPerMeter = 0;
			bmi.bmiHeader.biClrUsed = 0;
			bmi.bmiHeader.biClrImportant = 0;
			SetDIBits(Debugger.TileDC, Debugger.TileBMP, 0, D_TIL_H, TileArray, &bmi, DIB_RGB_COLORS);
		}
		RedrawWindow(GetDlgItem(Debugger.PPUWnd, IDC_DEBUG_PPU_TILE), NULL, NULL, RDW_INVALIDATE);
		Debugger.DetChanged = FALSE;
	}
}

void	Debugger_Update (void)
{
	if (Debugger.Mode & DEBUG_MODE_CPU)
		Debugger_UpdateCPU();
	if (Debugger.Mode & DEBUG_MODE_PPU)
		Debugger_UpdatePPU();
}

void	Debugger_SetDetail (int type, int num)
{
	if ((Debugger.DetailType == type) && (Debugger.DetailNum == num))
		return;
	Debugger.DetailType = type;
	Debugger.DetailNum = num;
	Debugger.DetChanged = TRUE;
	Debugger_UpdatePPU();
}

void	Debugger_DumpCPU (void)
{
	TCHAR filename[MAX_PATH];
	struct tm *newtime;
	time_t aclock;
	FILE *out;
	int i;

	if (!NES.ROMLoaded)
		return;

	time(&aclock);
	newtime = localtime(&aclock);

	_stprintf(filename, _T("%s\\Dumps\\%s.%04i%02i%02i_%02i%02i%02i.cpumem"), DataPath, States.BaseFilename, 
		newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
	out = _tfopen(filename, _T("wb"));
	fwrite(CPU_RAM, 1, 0x800, out);
	for (i = 4; i < 16; i++)
		if (CPU.PRGPointer[i])
			fwrite(CPU.PRGPointer[i], 1, 0x1000, out);
	fclose(out);
}

void	Debugger_DumpPPU (void)
{
	TCHAR filename[MAX_PATH];
	struct tm *newtime;
	time_t aclock;
	FILE *out;
	int i;

	if (!NES.ROMLoaded)
		return;

	time(&aclock);
	newtime = localtime(&aclock);

	_stprintf(filename, _T("%s\\Dumps\\%s.%04i%02i%02i_%02i%02i%02i.ppumem"), DataPath, States.BaseFilename, 
		newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
	out = _tfopen(filename, _T("wb"));
	for (i = 0; i < 12; i++)
		fwrite(PPU.CHRPointer[i], 1, 0x400, out);
	fwrite(PPU.Sprite, 1, 0x100, out);
	fwrite(PPU.Palette, 1, 0x20, out);
	fclose(out);
}

void	Debugger_CacheBreakpoints (void)
{
	int i;
	struct tBreakpoint *bp;
	ZeroMemory(Debugger.BPcache, sizeof(Debugger.BPcache));
	for (bp = Debugger.Breakpoints; bp != NULL; bp = bp->next)
	{
		if (!bp->enabled)
			continue;
		if (bp->type & (DEBUG_BREAK_EXEC | DEBUG_BREAK_READ | DEBUG_BREAK_WRITE))
		{
			for (i = bp->addr_start; i <= bp->addr_end; i++)
				Debugger.BPcache[i] |= bp->type;
		}
		if (bp->type & DEBUG_BREAK_OPCODE)
			Debugger.BPcache[0x10000 | bp->opcode] |= bp->type;
		if (bp->type & (DEBUG_BREAK_NMI | DEBUG_BREAK_IRQ | DEBUG_BREAK_BRK))
			Debugger.BPcache[0x10100] |= bp->type;
	}
}

struct tBreakpoint *Debugger_GetBreakpoint (HWND hwndDlg, int *_line)
{
	struct tBreakpoint *bp;
	TCHAR *str;
	int line, len;

	line = SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_GETCURSEL, 0, 0);
	if (line == -1)
	{
		EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_EDIT), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_DELETE), FALSE);
		return NULL;
	}

	EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_EDIT), TRUE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_DELETE), TRUE);

	len = SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_GETTEXTLEN, line, 0);
	str = malloc((len + 1) * sizeof(TCHAR));
	SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_GETTEXT, line, (LPARAM)str);

	// try to find it in the breakpoint list
	bp = Debugger.Breakpoints;
	while (bp != NULL)
	{
		if (!_tcscmp(bp->desc, str))
			break;
		bp = bp->next;
	}
	free(str);
	if (_line != NULL)
		*_line = line;
	return bp;
}

void	Debugger_SetBreakpoint (HWND hwndDlg, struct tBreakpoint *bp)
{
	int line;
	if (bp == NULL)
	{
		SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_SETCURSEL, (WPARAM)-1, 0);
		EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_EDIT), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_DELETE), FALSE);
		return;
	}

	line = SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)bp->desc);
	SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_SETCURSEL, line, 0);
	if (line == -1)
	{
		EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_EDIT), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_DELETE), FALSE);
	}
	else
	{
		EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_EDIT), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_DELETE), TRUE);
	}
}

INT_PTR CALLBACK BreakpointProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	TCHAR tpc[8];
	static struct tBreakpoint *bp = NULL;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		bp = (struct tBreakpoint *)lParam;
		if (bp == (struct tBreakpoint *)(size_t)0xFFFFFFFF)
		{
			bp = NULL;
			CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_BRK, IDC_BREAK_EXEC);
			{
				int line, len, Addr;
				TCHAR *str;

				line = SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_TRACE_LIST, LB_GETCURSEL, 0, 0);
				if (line == -1)
					break;
				len = SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_TRACE_LIST, LB_GETTEXTLEN, line, 0);
				str = malloc((len + 1) * sizeof(TCHAR));
				SendDlgItemMessage(Debugger.CPUWnd, IDC_DEBUG_TRACE_LIST, LB_GETTEXT, line, (LPARAM)str);
				Addr = _tcstol(str, NULL, 16);
				_stprintf(tpc, _T("%04X"), Addr);
				SetDlgItemText(hwndDlg, IDC_BREAK_ADDR1, tpc);
				free(str);
			}
			SetDlgItemText(hwndDlg, IDC_BREAK_ADDR2, _T(""));
			SetDlgItemText(hwndDlg, IDC_BREAK_OPNUM, _T("00"));
			CheckDlgButton(hwndDlg, IDC_BREAK_ENABLED, BST_CHECKED);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
		}
		else if (bp != NULL)
		{
			switch (bp->type)
			{
			case DEBUG_BREAK_EXEC:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_BRK, IDC_BREAK_EXEC);
				break;
			case DEBUG_BREAK_READ:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_BRK, IDC_BREAK_READ);
				break;
			case DEBUG_BREAK_WRITE:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_BRK, IDC_BREAK_WRITE);
				break;
			case DEBUG_BREAK_READ | DEBUG_BREAK_WRITE:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_BRK, IDC_BREAK_ACCESS);
				break;
			case DEBUG_BREAK_OPCODE:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_BRK, IDC_BREAK_OPCODE);
				break;
			case DEBUG_BREAK_NMI:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_BRK, IDC_BREAK_NMI);
				break;
			case DEBUG_BREAK_IRQ:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_BRK, IDC_BREAK_IRQ);
				break;
			case DEBUG_BREAK_BRK:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_BRK, IDC_BREAK_BRK);
				break;
			}
			switch (bp->type)
			{
			case DEBUG_BREAK_EXEC:
			case DEBUG_BREAK_READ:
			case DEBUG_BREAK_WRITE:
			case DEBUG_BREAK_READ | DEBUG_BREAK_WRITE:
				_stprintf(tpc, _T("%04X"), bp->addr_start);
				SetDlgItemText(hwndDlg, IDC_BREAK_ADDR1, tpc);
				if (bp->addr_start == bp->addr_end)
					SetDlgItemText(hwndDlg, IDC_BREAK_ADDR2, _T(""));
				else
				{
					_stprintf(tpc, _T("%04X"), bp->addr_end);
					SetDlgItemText(hwndDlg, IDC_BREAK_ADDR2, tpc);
				}
				SetDlgItemText(hwndDlg, IDC_BREAK_OPNUM, _T("00"));
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
				break;
			case DEBUG_BREAK_OPCODE:
				SetDlgItemText(hwndDlg, IDC_BREAK_ADDR1, _T("0000"));
				SetDlgItemText(hwndDlg, IDC_BREAK_ADDR2, _T(""));
				_stprintf(tpc, _T("%02X"), bp->opcode);
				SetDlgItemText(hwndDlg, IDC_BREAK_OPNUM, tpc);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), TRUE);
				break;
			case DEBUG_BREAK_NMI:
			case DEBUG_BREAK_IRQ:
			case DEBUG_BREAK_BRK:
				SetDlgItemText(hwndDlg, IDC_BREAK_ADDR1, _T("0000"));
				SetDlgItemText(hwndDlg, IDC_BREAK_ADDR2, _T(""));
				SetDlgItemText(hwndDlg, IDC_BREAK_OPNUM, _T("00"));
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
				break;
			}
			CheckDlgButton(hwndDlg, IDC_BREAK_ENABLED, (bp->enabled) ? BST_CHECKED : BST_UNCHECKED);
		}
		else
		{
			CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_BRK, IDC_BREAK_EXEC);
			SetDlgItemText(hwndDlg, IDC_BREAK_ADDR1, _T("0000"));
			SetDlgItemText(hwndDlg, IDC_BREAK_ADDR2, _T(""));
			SetDlgItemText(hwndDlg, IDC_BREAK_OPNUM, _T("00"));
			CheckDlgButton(hwndDlg, IDC_BREAK_ENABLED, BST_CHECKED);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
		}
		SetFocus(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1));
		SendDlgItemMessage(hwndDlg, IDC_BREAK_ADDR1, EM_SETSEL, 0, -1);
		return FALSE;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 

		switch (wmId)
		{
		case IDC_BREAK_EXEC:
		case IDC_BREAK_READ:
		case IDC_BREAK_WRITE:
		case IDC_BREAK_ACCESS:
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
			break;
		case IDC_BREAK_OPCODE:
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), TRUE);
			break;
		case IDC_BREAK_NMI:
		case IDC_BREAK_IRQ:
		case IDC_BREAK_BRK:
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
			break;

		case IDOK:
			{
				int addr1, addr2, opcode, type, enabled;
				TCHAR desc[32];
				GetDlgItemText(hwndDlg, IDC_BREAK_ADDR1, tpc, 5);
				addr1 = _tcstol(tpc, NULL, 16);
				GetDlgItemText(hwndDlg, IDC_BREAK_ADDR2, tpc, 5);
				addr2 = _tcstol(tpc, NULL, 16);
				if (!_tcslen(tpc))
					addr2 = addr1;
				GetDlgItemText(hwndDlg, IDC_BREAK_OPNUM, tpc, 3);
				opcode = _tcstol(tpc, NULL, 16);
				enabled = (IsDlgButtonChecked(hwndDlg, IDC_BREAK_ENABLED) == BST_CHECKED);

				type = 0;
				if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_EXEC) == BST_CHECKED)
				{
					type = DEBUG_BREAK_EXEC;
					_stprintf(desc, _T("Exec: %04X-%04X"), addr1, addr2);
				}
				if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_READ) == BST_CHECKED)
				{
					type = DEBUG_BREAK_READ;
					_stprintf(desc, _T("Read: %04X-%04X"), addr1, addr2);
				}
				if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_WRITE) == BST_CHECKED)
				{
					type = DEBUG_BREAK_WRITE;
					_stprintf(desc, _T("Write: %04X-%04X"), addr1, addr2);
				}
				if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_ACCESS) == BST_CHECKED)
				{
					type = DEBUG_BREAK_READ | DEBUG_BREAK_WRITE;
					_stprintf(desc, _T("Access: %04X-%04X"), addr1, addr2);
				}
				if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_OPCODE) == BST_CHECKED)
				{
					type = DEBUG_BREAK_OPCODE;
					_stprintf(desc, _T("Opcode: %02X"), opcode);
				}
				if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_NMI) == BST_CHECKED)
				{
					type = DEBUG_BREAK_NMI;
					_stprintf(desc, _T("NMI"), opcode);
				}
				if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_IRQ) == BST_CHECKED)
				{
					type = DEBUG_BREAK_IRQ;
					_stprintf(desc, _T("IRQ"), opcode);
				}
				if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_BRK) == BST_CHECKED)
				{
					type = DEBUG_BREAK_BRK;
					_stprintf(desc, _T("BRK"), opcode);
				}
				if (enabled)
					_tcscat(desc, _T(" (+)"));
				else	_tcscat(desc, _T(" (-)"));

				if (type & (DEBUG_BREAK_EXEC | DEBUG_BREAK_READ | DEBUG_BREAK_WRITE))
				{
					if ((addr1 < 0x0000) || (addr1 > 0xFFFF) ||
						(addr2 < 0x0000) || (addr2 > 0xFFFF) ||
						(addr1 > addr2))
					{
						MessageBox(hwndDlg, _T("Invalid address range specified!"), _T("Breakpoint"), MB_ICONERROR);
						break;
					}
				}
				if (type & (DEBUG_BREAK_OPCODE))
				{
					if ((opcode < 0x00) || (opcode > 0xFF))
					{
						MessageBox(hwndDlg, _T("Invalid opcode specified!"), _T("Breakpoint"), MB_ICONERROR);
						break;
					}
				}
				if (bp == NULL)
				{
					bp = (struct tBreakpoint *)malloc(sizeof(struct tBreakpoint));
					if (bp == NULL)
					{
						MessageBox(hwndDlg, _T("Failed to add breakpoint!"), _T("Breakpoint"), MB_ICONERROR);
						EndDialog(hwndDlg, (INT_PTR)NULL);
						break;
					}
					bp->next = Debugger.Breakpoints;
					bp->prev = NULL;
					if (bp->next != NULL)
						bp->next->prev = bp;
					Debugger.Breakpoints = bp;
				}
				bp->type = (unsigned char)type;
				bp->opcode = (unsigned char)opcode;
				bp->enabled = (unsigned char)enabled;
				bp->addr_start = (unsigned short)addr1;
				bp->addr_end = (unsigned short)addr2;
				_tcscpy(bp->desc, desc);
			}
			EndDialog(hwndDlg, (INT_PTR)bp);
			break;
		case IDCANCEL:
			EndDialog(hwndDlg, (INT_PTR)NULL);
			break;
		}
		break;
	}
	return FALSE;

}

INT_PTR CALLBACK CPUProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	int wmId, wmEvent;
	/* const */ int mem_tabs[16] = {24, 34, 44, 54, 66, 76, 86, 96, 108, 118, 128, 138, 150, 160, 170, 180};
	/* const */ int trace_tabs[3] = {25, 60, 80};	// Win9x requires these to be in writable memory
	TCHAR tpc[8];
	SCROLLINFO sinfo;
	struct tBreakpoint *bp;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		if (Debugger.TraceOffset == -1)
		{
			SetDlgItemText(hwndDlg, IDC_DEBUG_CONT_SEEKADDR, _T(""));
			CheckRadioButton(hwndDlg, IDC_DEBUG_CONT_SEEKPC, IDC_DEBUG_CONT_SEEKTO, IDC_DEBUG_CONT_SEEKPC);
		}
		else
		{
			_stprintf(tpc, _T("%04X"), Debugger.TraceOffset);
			SetDlgItemText(hwndDlg, IDC_DEBUG_CONT_SEEKADDR, tpc);
			CheckRadioButton(hwndDlg, IDC_DEBUG_CONT_SEEKPC, IDC_DEBUG_CONT_SEEKTO, IDC_DEBUG_CONT_SEEKTO);
		}
		SendDlgItemMessage(hwndDlg, IDC_DEBUG_TRACE_LIST, LB_SETTABSTOPS, 3, (LPARAM)&trace_tabs);

		sinfo.cbSize = sizeof(SCROLLINFO);
		sinfo.fMask = SIF_PAGE | SIF_RANGE;
		sinfo.nMin = 0;
		sinfo.nMax = 0xFFFF;
		sinfo.nPage = 0x1000;
		SetScrollInfo(GetDlgItem(hwndDlg, IDC_DEBUG_TRACE_SCROLL), SB_CTL, &sinfo, TRUE);

		SendDlgItemMessage(hwndDlg, IDC_DEBUG_MEM_LIST, LB_SETTABSTOPS, 16, (LPARAM)&mem_tabs);

		sinfo.fMask = SIF_POS;
		sinfo.nPos = 0;
		SetScrollInfo(GetDlgItem(hwndDlg, IDC_DEBUG_MEM_SCROLL), SB_CTL, &sinfo, TRUE);
		CheckRadioButton(hwndDlg, IDC_DEBUG_MEM_CPU, IDC_DEBUG_MEM_PAL, IDC_DEBUG_MEM_CPU);

		for (bp = Debugger.Breakpoints; bp != NULL; bp = bp->next)
			SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_ADDSTRING, 0, (LPARAM)bp->desc);
		return FALSE;
	case WM_PAINT:
		hdc = BeginPaint(hwndDlg, &ps);
		EndPaint(hwndDlg, &ps);
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 

		// if Debugger_UpdateCPU() is running, don't listen to any notifications
		if (inUpdate)
			break;

		switch (wmId)
		{
		case IDC_DEBUG_REG_A:
			// Don't bother modifying registers while it's emulating at full speed
			if (NES.Running)
				break;
			GetDlgItemText(hwndDlg, IDC_DEBUG_REG_A, tpc, 3);
			CPU.A = (unsigned char)_tcstol(tpc, NULL, 16);
			if (wmEvent == EN_KILLFOCUS)
				Debugger_UpdateCPU();
			break;
		case IDC_DEBUG_REG_X:
			if (NES.Running)
				break;
			GetDlgItemText(hwndDlg, IDC_DEBUG_REG_X, tpc, 3);
			CPU.X = (unsigned char)_tcstol(tpc, NULL, 16);
			if (wmEvent == EN_KILLFOCUS)
				Debugger_UpdateCPU();
			break;
		case IDC_DEBUG_REG_Y:
			if (NES.Running)
				break;
			GetDlgItemText(hwndDlg, IDC_DEBUG_REG_Y, tpc, 3);
			CPU.Y = (unsigned char)_tcstol(tpc, NULL, 16);
			if (wmEvent == EN_KILLFOCUS)
				Debugger_UpdateCPU();
			break;
		case IDC_DEBUG_REG_P:
			if (NES.Running)
				break;
			GetDlgItemText(hwndDlg, IDC_DEBUG_REG_P, tpc, 3);
			CPU.P = (unsigned char)_tcstol(tpc, NULL, 16);
			CPU_SplitFlags();
			if (wmEvent == EN_KILLFOCUS)
				Debugger_UpdateCPU();
			break;
		case IDC_DEBUG_REG_SP:
			if (NES.Running)
				break;
			GetDlgItemText(hwndDlg, IDC_DEBUG_REG_SP, tpc, 3);
			CPU.SP = (unsigned char)_tcstol(tpc, NULL, 16);
			if (wmEvent == EN_KILLFOCUS)
				Debugger_UpdateCPU();
			break;
		case IDC_DEBUG_REG_PC:
			if (NES.Running)
				break;
			GetDlgItemText(hwndDlg, IDC_DEBUG_REG_PC, tpc, 5);
			CPU.PC = _tcstol(tpc, NULL, 16);
			if (wmEvent == EN_KILLFOCUS)
				Debugger_UpdateCPU();
			break;

		case IDC_DEBUG_IRQ_EXT:
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_IRQ_EXT) == BST_CHECKED)
				CPU.WantIRQ |= IRQ_EXTERNAL;
			else	CPU.WantIRQ &= ~IRQ_EXTERNAL;
			break;
		case IDC_DEBUG_IRQ_PCM:
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_IRQ_PCM) == BST_CHECKED)
				CPU.WantIRQ |= IRQ_DPCM;
			else	CPU.WantIRQ &= ~IRQ_DPCM;
			break;
		case IDC_DEBUG_IRQ_FRAME:
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_IRQ_FRAME) == BST_CHECKED)
				CPU.WantIRQ |= IRQ_FRAME;
			else	CPU.WantIRQ &= ~IRQ_FRAME;
			break;
		case IDC_DEBUG_IRQ_DEBUG:
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_IRQ_DEBUG) == BST_CHECKED)
				CPU.WantIRQ |= IRQ_DEBUG;
			else	CPU.WantIRQ &= ~IRQ_DEBUG;
			break;

		case IDC_DEBUG_FLAG_N:
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_FLAG_N) == BST_CHECKED)
				CPU.FN = 1;
			else	CPU.FN = 0;
			CPU_JoinFlags();
			Debugger_UpdateCPU();
			break;
		case IDC_DEBUG_FLAG_V:
			// or modify flags while running
			if (NES.Running)
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_FLAG_V) == BST_CHECKED)
				CPU.FV = 1;
			else	CPU.FV = 0;
			CPU_JoinFlags();
			Debugger_UpdateCPU();
			break;
		case IDC_DEBUG_FLAG_D:
			if (NES.Running)
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_FLAG_D) == BST_CHECKED)
				CPU.FD = 1;
			else	CPU.FD = 0;
			CPU_JoinFlags();
			Debugger_UpdateCPU();
			break;
		case IDC_DEBUG_FLAG_I:
			if (NES.Running)
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_FLAG_I) == BST_CHECKED)
				CPU.FI = 1;
			else	CPU.FI = 0;
			CPU_JoinFlags();
			Debugger_UpdateCPU();
			break;
		case IDC_DEBUG_FLAG_Z:
			if (NES.Running)
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_FLAG_Z) == BST_CHECKED)
				CPU.FZ = 1;
			else	CPU.FZ = 0;
			CPU_JoinFlags();
			Debugger_UpdateCPU();
			break;
		case IDC_DEBUG_FLAG_C:
			if (NES.Running)
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_FLAG_C) == BST_CHECKED)
				CPU.FC = 1;
			else	CPU.FC = 0;
			CPU_JoinFlags();
			Debugger_UpdateCPU();
			break;

		case IDC_DEBUG_BREAK_LIST:
			if (wmEvent == LBN_DBLCLK)
			{
				int line;
				// get selected breakpoint
				bp = Debugger_GetBreakpoint(hwndDlg, &line);

				if (bp == NULL)
					break;

				bp->enabled = !bp->enabled;
				if (bp->enabled)
					bp->desc[_tcslen(bp->desc) - 2] = _T('+');
				else	bp->desc[_tcslen(bp->desc) - 2] = _T('-');

				// update breakpoint description
				SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_DELETESTRING, line, 0);
				SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_ADDSTRING, 0, (LPARAM)bp->desc);
				// reselect it
				Debugger_SetBreakpoint(hwndDlg, bp);
				// then recache the breakpoints
				Debugger_CacheBreakpoints();
			}
			else	Debugger_GetBreakpoint(hwndDlg, NULL);	// enable the add/remove buttons
			break;
		case IDC_DEBUG_BREAK_ADD:
			bp = (struct tBreakpoint *)DialogBoxParam(hInst, (LPCTSTR)IDD_BREAKPOINT, hwndDlg, BreakpointProc, (LPARAM)NULL);
			// if user cancels, nothing was added
			if (bp == NULL)
				break;
			// add it to the breakpoint listbox
			SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_ADDSTRING, 0, (LPARAM)bp->desc);
			// select it
			Debugger_SetBreakpoint(hwndDlg, bp);
			// then recache the breakpoints
			Debugger_CacheBreakpoints();
			break;
		case IDC_DEBUG_BREAK_EDIT:
			{
				int line;
				// get selected breakpoint
				bp = Debugger_GetBreakpoint(hwndDlg, &line);
				if (bp == NULL)
					break;

				// then open the editor on it
				bp = (struct tBreakpoint *)DialogBoxParam(hInst, (LPCTSTR)IDD_BREAKPOINT, hwndDlg, BreakpointProc, (LPARAM)bp);
				// if user cancels, nothing was changed
				if (bp == NULL)
					break;
				// update breakpoint description
				SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_DELETESTRING, line, 0);
				SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_ADDSTRING, 0, (LPARAM)bp->desc);
				// reselect it
				Debugger_SetBreakpoint(hwndDlg, bp);
				// then recache the breakpoints
				Debugger_CacheBreakpoints();
			}
			break;
		case IDC_DEBUG_BREAK_DELETE:
			{
				int line;
				// get selected breakpoint
				bp = Debugger_GetBreakpoint(hwndDlg, &line);

				if (bp == NULL)
					break;

				// and take it out of the list
				if (bp->prev != NULL)
					bp->prev->next = bp->next;
				else	Debugger.Breakpoints = bp->next;
				if (bp->next != NULL)
					bp->next->prev = bp->prev;
				free(bp);
				SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_DELETESTRING, line, 0);
				Debugger_SetBreakpoint(hwndDlg, NULL);
				// then recache the breakpoints
				Debugger_CacheBreakpoints();
			}
			break;

		case IDC_DEBUG_CONT_RUN:
			if (NES.ROMLoaded)
				SendMessage(hMainWnd, WM_COMMAND, ID_CPU_RUN, 0);
			break;
		case IDC_DEBUG_CONT_STEP:
			if (NES.ROMLoaded)
				SendMessage(hMainWnd, WM_COMMAND, ID_CPU_STEP, 0);
			break;
		case IDC_DEBUG_CONT_RESET:
			if (NES.ROMLoaded)
				SendMessage(hMainWnd, WM_COMMAND, ID_CPU_SOFTRESET, 0);
			break;
		case IDC_DEBUG_CONT_POWER:
			if (NES.ROMLoaded)
				SendMessage(hMainWnd, WM_COMMAND, ID_CPU_HARDRESET, 0);
			break;
		case IDC_DEBUG_CONT_SEEKPC:
			Debugger.TraceOffset = -1;
			Debugger_UpdateCPU();
			break;
		case IDC_DEBUG_CONT_SEEKTO:
			GetDlgItemText(hwndDlg, IDC_DEBUG_CONT_SEEKADDR, tpc, 5);
			Debugger.TraceOffset = _tcstol(tpc, NULL, 16);
			Debugger_UpdateCPU();
			break;
		case IDC_DEBUG_CONT_SEEKSEL:
			{
				int line, len, Addr;
				TCHAR *str;

				line = SendDlgItemMessage(hwndDlg, IDC_DEBUG_TRACE_LIST, LB_GETCURSEL, 0, 0);
				if (line == -1)
					break;
				len = SendDlgItemMessage(hwndDlg, IDC_DEBUG_TRACE_LIST, LB_GETTEXTLEN, line, 0);
				str = malloc((len + 1) * sizeof(TCHAR));
				SendDlgItemMessage(hwndDlg, IDC_DEBUG_TRACE_LIST, LB_GETTEXT, line, (LPARAM)str);
				Addr = _tcstol(str, NULL, 16);
				_stprintf(tpc, _T("%04X"), Addr);
				SetDlgItemText(hwndDlg, IDC_DEBUG_CONT_SEEKADDR, tpc);
				free(str);
			}
			break;
		case IDC_DEBUG_CONT_SEEKADDR:
			if (Debugger.TraceOffset != -1)
			{
				GetDlgItemText(hwndDlg, IDC_DEBUG_CONT_SEEKADDR, tpc, 5);
				Debugger.TraceOffset = _tcstol(tpc, NULL, 16);
				Debugger_UpdateCPU();
			}
			break;
		case IDC_DEBUG_CONT_DUMPCPU:
			Debugger_DumpCPU();
			break;
		case IDC_DEBUG_CONT_DUMPPPU:
			Debugger_DumpPPU();
			break;
		case IDC_DEBUG_CONT_STARTLOG:
			Debugger_StartLogging();
			break;
		case IDC_DEBUG_CONT_STOPLOG:
			Debugger_StopLogging();
			break;

		case IDC_DEBUG_MEM_CPU:
		case IDC_DEBUG_MEM_PPU:
		case IDC_DEBUG_MEM_SPR:
		case IDC_DEBUG_MEM_PAL:
			// all of these should just jump to address 0 and redraw
			Debugger.MemOffset = 0;
			Debugger_UpdateCPU();
			break;

		case IDC_DEBUG_TRACE_LIST:
			if (wmEvent == LBN_DBLCLK)
			{
				bp = (struct tBreakpoint *)DialogBoxParam(hInst, (LPCTSTR)IDD_BREAKPOINT, hwndDlg, BreakpointProc, (LPARAM)0xFFFFFFFF);
				// if user cancels, nothing was added
				if (bp == NULL)
					break;
				// add it to the breakpoint listbox
				SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_ADDSTRING, 0, (LPARAM)bp->desc);
				// select it
				Debugger_SetBreakpoint(hwndDlg, bp);
				// then recache the breakpoints
				Debugger_CacheBreakpoints();
			}
			break;

		case IDCANCEL:
			Debugger_SetMode(Debugger.Mode & ~DEBUG_MODE_CPU);
			break;
		}
		break;
	case WM_VSCROLL:
		// if Debugger_UpdateCPU() is running, don't listen to any notifications
		if (inUpdate)
			break;

		if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_DEBUG_TRACE_SCROLL))
		{
			sinfo.cbSize = sizeof(SCROLLINFO);
			sinfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
			GetScrollInfo((HWND)lParam, SB_CTL, &sinfo);
			if (Debugger.TraceOffset == -1)
				Debugger.TraceOffset = CPU.PC;
			switch (LOWORD(wParam))
			{
			case SB_LINEUP:
				Debugger.TraceOffset--;
				break;
			case SB_LINEDOWN:
				Debugger.TraceOffset++;
				break;
			case SB_PAGEUP:
				Debugger.TraceOffset -= sinfo.nPage;
				break;
			case SB_PAGEDOWN:
				Debugger.TraceOffset += sinfo.nPage;
				break;
			case SB_THUMBPOSITION:
				Debugger.TraceOffset = HIWORD(wParam);
				break;
			default:
				return FALSE;
			}
			if (Debugger.TraceOffset < sinfo.nMin)
				Debugger.TraceOffset = sinfo.nMin;
			if (Debugger.TraceOffset > sinfo.nMax)
				Debugger.TraceOffset = sinfo.nMax;
			_stprintf(tpc, _T("%04X"), Debugger.TraceOffset);
			SetDlgItemText(hwndDlg, IDC_DEBUG_CONT_SEEKADDR, tpc);
			CheckRadioButton(hwndDlg, IDC_DEBUG_CONT_SEEKPC, IDC_DEBUG_CONT_SEEKTO, IDC_DEBUG_CONT_SEEKTO);
			Debugger_UpdateCPU();
		}
		if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_DEBUG_MEM_SCROLL))
		{
			sinfo.cbSize = sizeof(SCROLLINFO);
			sinfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
			GetScrollInfo((HWND)lParam, SB_CTL, &sinfo);
			switch (LOWORD(wParam))
			{
			case SB_LINEUP:
				Debugger.MemOffset--;
				break;
			case SB_LINEDOWN:
				Debugger.MemOffset++;
				break;
			case SB_PAGEUP:
				Debugger.MemOffset -= sinfo.nPage;
				break;
			case SB_PAGEDOWN:
				Debugger.MemOffset += sinfo.nPage;
				break;
			case SB_THUMBPOSITION:
				Debugger.MemOffset = HIWORD(wParam);
				break;
			default:
				return FALSE;
			}
			if (Debugger.MemOffset < sinfo.nMin)
				Debugger.MemOffset = sinfo.nMin;
			if (Debugger.MemOffset > sinfo.nMax)
				Debugger.MemOffset = sinfo.nMax;
			Debugger_UpdateCPU();
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK PPUProc_Nametable (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT point;
	WNDPROC proc;
	if (uMsg == WM_MOUSEMOVE)
	{
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		if ((point.x >= 0) && (point.x < 256) && (point.y >= 0) && (point.y < 240))
			Debugger_SetDetail(DEBUG_DETAIL_NAMETABLE, ((point.y << 2) & 0x3E0) | ((point.x >> 3) & 0x1F) | (Debugger.Nametable << 10));
		else	Debugger_SetDetail(Debugger.DetailTypeSave, Debugger.DetailNumSave);
		return 0;
	}
	if (uMsg == WM_RBUTTONDOWN)
	{
		Debugger.DetailTypeSave = Debugger.DetailType;
		Debugger.DetailNumSave = Debugger.DetailNum;
		return 0;
	}
	proc = (WNDPROC)GetWindowLong(hwndDlg, GWL_USERDATA);
	return CallWindowProc(proc, hwndDlg, uMsg, wParam, lParam);
}
INT_PTR CALLBACK PPUProc_Pattern (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT point;
	WNDPROC proc;
	if (uMsg == WM_MOUSEMOVE)
	{
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		if ((point.x >= 0) && (point.x < 256) && (point.y >= 0) && (point.y < 128))
			Debugger_SetDetail(DEBUG_DETAIL_PATTERN, ((point.x >> 3) & 0xF) | ((point.y << 1) & 0xF0) | ((point.x & 0x80) << 1));
		else	Debugger_SetDetail(Debugger.DetailTypeSave, Debugger.DetailNumSave);
		return 0;
	}
	if (uMsg == WM_RBUTTONDOWN)
	{
		Debugger.DetailTypeSave = Debugger.DetailType;
		Debugger.DetailNumSave = Debugger.DetailNum;
		return 0;
	}
	proc = (WNDPROC)GetWindowLong(hwndDlg, GWL_USERDATA);
	return CallWindowProc(proc, hwndDlg, uMsg, wParam, lParam);
}
INT_PTR CALLBACK PPUProc_Palette (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT point;
	WNDPROC proc;
	if (uMsg == WM_MOUSEMOVE)
	{
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		if ((point.x >= 0) && (point.x < 256) && (point.y >= 0) && (point.y < 32))
			Debugger_SetDetail(DEBUG_DETAIL_PALETTE, (point.y & 0x30) | (point.x >> 4));
		else	Debugger_SetDetail(Debugger.DetailTypeSave, Debugger.DetailNumSave);
		return 0;
	}
	if (uMsg == WM_RBUTTONDOWN)
	{
		Debugger.DetailTypeSave = Debugger.DetailType;
		Debugger.DetailNumSave = Debugger.DetailNum;
		return 0;
	}
	proc = (WNDPROC)GetWindowLong(hwndDlg, GWL_USERDATA);
	return CallWindowProc(proc, hwndDlg, uMsg, wParam, lParam);
}
INT_PTR CALLBACK PPUProc_Sprite (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT point;
	WNDPROC proc;
	if (uMsg == WM_MOUSEMOVE)
	{
		int height = (PPU.Reg2000 & 0x20) ? 16 : 8;
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		if ((point.x >= 0) && (point.x < 256) && (point.y >= 0) && (point.y < 96) && ((point.x % 16) < 8) && ((point.y % 24) < height))
			Debugger_SetDetail(DEBUG_DETAIL_SPRITE, ((point.y / 24) << 4) | (point.x >> 4));
		else	Debugger_SetDetail(Debugger.DetailTypeSave, Debugger.DetailNumSave);
		return 0;
	}
	if (uMsg == WM_RBUTTONDOWN)
	{
		Debugger.DetailTypeSave = Debugger.DetailType;
		Debugger.DetailNumSave = Debugger.DetailNum;
		return 0;
	}
	proc = (WNDPROC)GetWindowLong(hwndDlg, GWL_USERDATA);
	return CallWindowProc(proc, hwndDlg, uMsg, wParam, lParam);
}

INT_PTR CALLBACK PPUProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LPDRAWITEMSTRUCT lpDrawItem;
	const int dbgRadio[4] = { IDC_DEBUG_PPU_NT0, IDC_DEBUG_PPU_NT1, IDC_DEBUG_PPU_NT2, IDC_DEBUG_PPU_NT3 };
	HWND dlgItem;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		CheckRadioButton(hwndDlg, IDC_DEBUG_PPU_NT0, IDC_DEBUG_PPU_NT3, dbgRadio[Debugger.Nametable]);
		Debugger.NTabChanged = Debugger.PalChanged = Debugger.PatChanged = Debugger.SprChanged = Debugger.DetChanged = TRUE;

		dlgItem = GetDlgItem(hwndDlg, IDC_DEBUG_PPU_NAMETABLE);
		SetWindowLong(dlgItem, GWL_USERDATA, GetWindowLong(dlgItem, GWL_WNDPROC));
		SetWindowLong(dlgItem, GWL_WNDPROC, (LONG)PPUProc_Nametable);

		dlgItem = GetDlgItem(hwndDlg, IDC_DEBUG_PPU_PATTERN);
		SetWindowLong(dlgItem, GWL_USERDATA, GetWindowLong(dlgItem, GWL_WNDPROC));
		SetWindowLong(dlgItem, GWL_WNDPROC, (LONG)PPUProc_Pattern);

		dlgItem = GetDlgItem(hwndDlg, IDC_DEBUG_PPU_PALETTE);
		SetWindowLong(dlgItem, GWL_USERDATA, GetWindowLong(dlgItem, GWL_WNDPROC));
		SetWindowLong(dlgItem, GWL_WNDPROC, (LONG)PPUProc_Palette);

		dlgItem = GetDlgItem(hwndDlg, IDC_DEBUG_PPU_SPRITE);
		SetWindowLong(dlgItem, GWL_USERDATA, GetWindowLong(dlgItem, GWL_WNDPROC));
		SetWindowLong(dlgItem, GWL_WNDPROC, (LONG)PPUProc_Sprite);
		return FALSE;
	case WM_DRAWITEM:
		lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
		switch (lpDrawItem->CtlID)
		{
		case IDC_DEBUG_PPU_NAMETABLE:
			BitBlt(lpDrawItem->hDC, 0, 0, D_NAM_W, D_NAM_H, Debugger.NameDC, 0, 0, SRCCOPY);
			break;
		case IDC_DEBUG_PPU_PATTERN:
			BitBlt(lpDrawItem->hDC, 0, 0, D_PAT_W, D_PAT_H, Debugger.PatternDC, 0, 0, SRCCOPY);
			break;
		case IDC_DEBUG_PPU_PALETTE:
			BitBlt(lpDrawItem->hDC, 0, 0, D_PAL_W, D_PAL_H, Debugger.PaletteDC, 0, 0, SRCCOPY);
			break;
		case IDC_DEBUG_PPU_SPRITE:
			BitBlt(lpDrawItem->hDC, 0, 0, D_SPR_W, D_SPR_H, Debugger.SpriteDC, 0, 0, SRCCOPY);
			break;
		case IDC_DEBUG_PPU_TILE:
			BitBlt(lpDrawItem->hDC, 0, 0, D_TIL_W, D_TIL_H, Debugger.TileDC, 0, 0, SRCCOPY);
			break;
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 

		switch (wmId)
		{
		case IDC_DEBUG_PPU_NAMETABLE:
			Debugger.Nametable = (Debugger.Nametable + 1) & 3;
			CheckRadioButton(hwndDlg, IDC_DEBUG_PPU_NT0, IDC_DEBUG_PPU_NT3, dbgRadio[Debugger.Nametable]);
			Debugger.NTabChanged = TRUE;
			Debugger_UpdatePPU();
			break;
		case IDC_DEBUG_PPU_PATTERN:
			Debugger.Palette = (Debugger.Palette + 1) & 7;
			Debugger.PatChanged = TRUE;
			Debugger_UpdatePPU();
			break;
		case IDC_DEBUG_PPU_NT0:
			Debugger.Nametable = 0;
			Debugger.NTabChanged = TRUE;
			Debugger_UpdatePPU();
			break;
		case IDC_DEBUG_PPU_NT1:
			Debugger.Nametable = 1;
			Debugger.NTabChanged = TRUE;
			Debugger_UpdatePPU();
			break;
		case IDC_DEBUG_PPU_NT2:
			Debugger.Nametable = 2;
			Debugger.NTabChanged = TRUE;
			Debugger_UpdatePPU();
			break;
		case IDC_DEBUG_PPU_NT3:
			Debugger.Nametable = 3;
			Debugger.NTabChanged = TRUE;
			Debugger_UpdatePPU();
			break;
		case IDCANCEL:
			Debugger_SetMode(Debugger.Mode & ~DEBUG_MODE_PPU);
			break;
		}
		break;
	case WM_MOUSEMOVE:
		Debugger_SetDetail(Debugger.DetailTypeSave, Debugger.DetailNumSave);
		break;
	case WM_RBUTTONDOWN:
		Debugger.DetailTypeSave = DEBUG_DETAIL_NONE;
		Debugger.DetailNumSave = 0;
		Debugger_SetDetail(Debugger.DetailTypeSave, Debugger.DetailNumSave);
		break;
	}
	return FALSE;
}

#endif	/* ENABLE_DEBUGGER */
