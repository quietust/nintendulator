/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) QMT Productions
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
#include <vector>

#ifdef	ENABLE_DEBUGGER

// Needed for window subclassing (to allow breakpoint on scanline -1)
#pragma comment(lib, "comctl32.lib")

namespace Debugger
{
struct tBreakpoint
{
	TCHAR desc[32];
	unsigned short addr_start, addr_end;
	unsigned char opcode;
	unsigned char type;
	unsigned char enabled;
	unsigned short scanline;
	void updateDescription ()
	{
		switch (type)
		{
		case DEBUG_BREAK_EXEC:
			if (addr_start == addr_end)
				_stprintf(desc, _T("Exec: $%04X"), addr_start);
			else	_stprintf(desc, _T("Exec: $%04X-$%04X"), addr_start, addr_end);
			break;
		case DEBUG_BREAK_READ:
			if (addr_start == addr_end)
				_stprintf(desc, _T("Read: $%04X"), addr_start);
			else	_stprintf(desc, _T("Read: $%04X-$%04X"), addr_start, addr_end);
			break;
		case DEBUG_BREAK_WRITE:
			if (addr_start == addr_end)
				_stprintf(desc, _T("Write: $%04X"), addr_start);
			else	_stprintf(desc, _T("Write: $%04X-$%04X"), addr_start, addr_end);
			break;
		case DEBUG_BREAK_READ | DEBUG_BREAK_WRITE:
			if (addr_start == addr_end)
				_stprintf(desc, _T("Access: $%04X"), addr_start);
			else	_stprintf(desc, _T("Access: $%04X-$%04X"), addr_start, addr_end);
			break;
		case DEBUG_BREAK_OPCODE:
			_stprintf(desc, _T("Opcode: $%02X"), opcode);
			break;
		case DEBUG_BREAK_NMI:
			_stprintf(desc, _T("Interrupt: NMI"));
			break;
		case DEBUG_BREAK_IRQ:
			_stprintf(desc, _T("Interrupt: IRQ"));
			break;
		case DEBUG_BREAK_BRK:
			_stprintf(desc, _T("Interrupt: BRK"));
			break;
		case DEBUG_BREAK_SCANLINE:
			_stprintf(desc, _T("Scanline: %i"), scanline - 1);
			break;
		default:
			_stprintf(desc, _T("UNDEFINED"));
			break;
		}
		if (enabled)
			_tcscat(desc, _T(" (+)"));
		else	_tcscat(desc, _T(" (-)"));
	}
};
	
BOOL	Enabled;
int	Mode;

BOOL	NTabChanged, PalChanged, PatChanged, SprChanged, DetChanged;

int	DetailType, DetailNum;
int	DetailTypeSave, DetailNumSave;

BOOL	Logging, Step;

int	Palette, Nametable, TileFlip;

HDC	PaletteDC;	// Palette
HBITMAP	PaletteBMP;

HDC	PatternDC;	// Pattern tables
HBITMAP	PatternBMP;

HDC	NameDC;		// Nametable
HBITMAP	NameBMP;

HDC	SpriteDC;	// Sprites
HBITMAP	SpriteBMP;

HDC	TileDC;		// Preview Tile
HBITMAP	TileBMP;

HWND	CPUWnd;
HWND	PPUWnd;

int	TraceOffset;	// -1 to center on PC, otherwise center on TraceOffset
int	MemOffset;

FILE	*LogFile;
unsigned char	BPcache[0x10000];
unsigned char	PalCache[0x20];
std::vector<struct tBreakpoint> Breakpoints;
int	LastScanline;

BOOL inUpdate = FALSE;

enum ADDRMODE { IMP, ACC, IMM, ADR, ABS, IND, REL, ABX, ABY, ZPG, ZPX, ZPY, INX, INY, ERR, NUM_ADDR_MODES };

const enum ADDRMODE TraceAddrMode[256] =
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

const unsigned char AddrBytes[NUM_ADDR_MODES] = {1, 1, 2, 3, 3, 3, 2, 3, 3, 2, 2, 2, 2, 2, 1};

const char TraceArr[256][5] =
{
	" BRK", " ORA", "*HLT", "*SLO", "*NOP", " ORA", " ASL", "*SLO", " PHP", " ORA", " ASL", "*AAC", "*NOP", " ORA", " ASL", "*SLO",
	" BPL", " ORA", "*HLT", "*SLO", "*NOP", " ORA", " ASL", "*SLO", " CLC", " ORA", "*NOP", "*SLO", "*NOP", " ORA", " ASL", "*SLO",
	" JSR", " AND", "*HLT", "*RLA", " BIT", " AND", " ROL", "*RLA", " PLP", " AND", " ROL", "*AAC", " BIT", " AND", " ROL", "*RLA",
	" BMI", " AND", "*HLT", "*RLA", "*NOP", " AND", " ROL", "*RLA", " SEC", " AND", "*NOP", "*RLA", "*NOP", " AND", " ROL", "*RLA",
	" RTI", " EOR", "*HLT", "*SRE", "*NOP", " EOR", " LSR", "*SRE", " PHA", " EOR", " LSR", "*ASR", " JMP", " EOR", " LSR", "*SRE",
	" BVC", " EOR", "*HLT", "*SRE", "*NOP", " EOR", " LSR", "*SRE", " CLI", " EOR", "*NOP", "*SRE", "*NOP", " EOR", " LSR", "*SRE",
	" RTS", " ADC", "*HLT", "*RRA", "*NOP", " ADC", " ROR", "*RRA", " PLA", " ADC", " ROR", "*ARR", " JMP", " ADC", " ROR", "*RRA",
	" BVS", " ADC", "*HLT", "*RRA", "*NOP", " ADC", " ROR", "*RRA", " SEI", " ADC", "*NOP", "*RRA", "*NOP", " ADC", " ROR", "*RRA",
	"*NOP", " STA", "*NOP", "*SAX", " STY", " STA", " STX", "*SAX", " DEY", "*NOP", " TXA", " ???", " STY", " STA", " STX", "*SAX",
	" BCC", " STA", "*HLT", " ???", " STY", " STA", " STX", "*SAX", " TYA", " STA", " TXS", " ???", " ???", " STA", " ???", " ???",
	" LDY", " LDA", " LDX", "*LAX", " LDY", " LDA", " LDX", "*LAX", " TAY", " LDA", " TAX", "*ATX", " LDY", " LDA", " LDX", "*LAX",
	" BCS", " LDA", "*HLT", "*LAX", " LDY", " LDA", " LDX", "*LAX", " CLV", " LDA", " TSX", " ???", " LDY", " LDA", " LDX", "*LAX",
	" CPY", " CMP", "*NOP", "*DCP", " CPY", " CMP", " DEC", "*DCP", " INY", " CMP", " DEX", "*AXS", " CPY", " CMP", " DEC", "*DCP",
	" BNE", " CMP", "*HLT", "*DCP", "*NOP", " CMP", " DEC", "*DCP", " CLD", " CMP", "*NOP", "*DCP", "*NOP", " CMP", " DEC", "*DCP",
	" CPX", " SBC", "*NOP", "*ISB", " CPX", " SBC", " INC", "*ISB", " INX", " SBC", " NOP", "*SBC", " CPX", " SBC", " INC", "*ISB",
	" BEQ", " SBC", "*HLT", "*ISB", "*NOP", " SBC", " INC", "*ISB", " SED", " SBC", "*NOP", "*ISB", "*NOP", " SBC", " INC", "*ISB"
};

#define	BP_NA (0)
#define	BP_RD (DEBUG_BREAK_READ)
#define	BP_WR (DEBUG_BREAK_WRITE)
#define	BP_RW (DEBUG_BREAK_READ | DEBUG_BREAK_WRITE)

const unsigned char TraceIO[256] =
{
	BP_WR, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_WR, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_WR, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_RD, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_RD, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_WR, BP_RD, BP_NA, BP_RD, BP_NA, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_RD, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_RD, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
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
void	StartLogging (void);
void	StopLogging (void);
void	CacheBreakpoints (void);

void	Init (void)
{
	Enabled = FALSE;
	Mode = 0;

	NTabChanged = PalChanged = PatChanged = SprChanged = DetChanged = FALSE;
	DetailType = DetailNum = 0;
	DetailTypeSave = DetailNumSave = 0;
	
	Logging = FALSE;
	Step = FALSE;

	Palette = 0;
	Nametable = 0;
	TileFlip = 0;

	HDC TpHDC = GetDC(hMainWnd);

	PaletteDC = CreateCompatibleDC(TpHDC);
	PaletteBMP = CreateCompatibleBitmap(TpHDC, D_PAL_W, D_PAL_H);
	SelectObject(PaletteDC, PaletteBMP);
	BitBlt(PaletteDC, 0, 0, D_PAL_W, D_PAL_H, NULL, 0, 0, BLACKNESS);

	PatternDC = CreateCompatibleDC(TpHDC);
	PatternBMP = CreateCompatibleBitmap(TpHDC, D_PAT_W, D_PAT_H);
	SelectObject(PatternDC, PatternBMP);
	BitBlt(PatternDC, 0, 0, D_PAT_W, D_PAT_H, NULL, 0, 0, BLACKNESS);

	NameDC = CreateCompatibleDC(TpHDC);
	NameBMP = CreateCompatibleBitmap(TpHDC, D_NAM_W, D_NAM_H);
	SelectObject(NameDC, NameBMP);
	BitBlt(NameDC, 0, 0, D_NAM_W, D_NAM_H, NULL, 0, 0, BLACKNESS);

	SpriteDC = CreateCompatibleDC(TpHDC);
	SpriteBMP = CreateCompatibleBitmap(TpHDC, D_SPR_W, D_SPR_H);
	SelectObject(SpriteDC, SpriteBMP);
	BitBlt(SpriteDC, 0, 0, D_SPR_W, D_SPR_H, NULL, 0, 0, BLACKNESS);

	TileDC = CreateCompatibleDC(TpHDC);
	TileBMP = CreateCompatibleBitmap(TpHDC, D_TIL_W, D_TIL_H);
	SelectObject(TileDC, TileBMP);
	BitBlt(TileDC, 0, 0, D_TIL_W, D_TIL_H, NULL, 0, 0, BLACKNESS);

	ReleaseDC(hMainWnd, TpHDC);

	CPUWnd = NULL;
	PPUWnd = NULL;

	Breakpoints.clear();
	TraceOffset = -1;
	MemOffset = 0;

	LogFile = NULL;
	CacheBreakpoints();
}

void	Destroy (void)
{
	StopLogging();
	SetMode(0);
	Breakpoints.clear();
}

void	SetMode (int NewMode)
{
	RECT wRect;
	GetWindowRect(hMainWnd,&wRect);

	Mode = NewMode;
	Enabled = (Mode > 0) ? TRUE : FALSE;

	if ((Mode & DEBUG_MODE_CPU) && !CPUWnd)
	{
		CPUWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DEBUGGER_CPU), hMainWnd, CPUProc);
		SetWindowPos(CPUWnd, hMainWnd, wRect.right, wRect.top, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOOWNERZORDER);
	}
	else if (!(Mode & DEBUG_MODE_CPU) && CPUWnd)
	{
		DestroyWindow(CPUWnd);
		CPUWnd = NULL;
	}
	if ((Mode & DEBUG_MODE_PPU) && !PPUWnd)
	{
		PPUWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DEBUGGER_PPU), hMainWnd, PPUProc);
		SetWindowPos(PPUWnd, hMainWnd, wRect.left, wRect.bottom, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOOWNERZORDER);
		NTabChanged = TRUE;
		PalChanged = TRUE;
		PatChanged = TRUE;
		SprChanged = TRUE;
	}
	else if (!(Mode & DEBUG_MODE_PPU) && PPUWnd)
	{
		DestroyWindow(PPUWnd);
		PPUWnd = NULL;
	}

	if (Mode & DEBUG_MODE_CPU)
		CheckMenuItem(hMenu,ID_DEBUG_CPU,MF_CHECKED);
	else	CheckMenuItem(hMenu,ID_DEBUG_CPU,MF_UNCHECKED);
	if (Mode & DEBUG_MODE_PPU)
		CheckMenuItem(hMenu,ID_DEBUG_PPU,MF_CHECKED);
	else	CheckMenuItem(hMenu,ID_DEBUG_PPU,MF_UNCHECKED);

	Update(DEBUG_MODE_CPU | DEBUG_MODE_PPU);
	SetFocus(hMainWnd);
}

void	StartLogging (void)
{
	TCHAR filename[MAX_PATH];
	struct tm *newtime;
	time_t aclock;

	if (!NES::ROMLoaded)
		return;

	time(&aclock);
	newtime = localtime(&aclock);

	_stprintf(filename, _T("%s\\Dumps\\%s.%04i%02i%02i_%02i%02i%02i.debug"), DataPath, States::BaseFilename,
		newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);

	Logging = TRUE;
	LogFile = _tfopen(filename, _T("a+"));
}

void	StopLogging (void)
{
	if (Logging)
		fclose(LogFile);
	Logging = FALSE;
}

unsigned char DebugMemCPU (unsigned short Addr)
{
	int Bank = (Addr >> 12) & 0xF;
	return CPU::ReadHandlerDebug[Bank](Bank, Addr & 0xFFF);
}

unsigned char DebugMemPPU (unsigned short Addr)
{
	int Bank = (Addr >> 10) & 0xF;
	return PPU::ReadHandlerDebug[Bank](Bank, Addr & 0x3FF);
}

BOOL IsBreakpoint (unsigned char opcode, unsigned short address, BOOL force_read)
{
	// read opcode, effective address has read breakpoint
	if ((TraceIO[opcode] & DEBUG_BREAK_READ) && (BPcache[address] & DEBUG_BREAK_READ))
		return TRUE;
	// write opcode, effective address has write breakpoint, unless it's an explicit read (e.g. indirection or page-cross correction)
	if ((TraceIO[opcode] & DEBUG_BREAK_WRITE) && (BPcache[address] & (force_read ? DEBUG_BREAK_READ : DEBUG_BREAK_WRITE)))
		return TRUE;
	return FALSE;
}

// Decodes an instruction into plain text, suitable for displaying in the debugger or writing to a logfile
// Returns whether or not a breakpoint was encountered
BOOL	DecodeInstruction (unsigned short Addr, char *str1, TCHAR *str2, BOOL checkBreakpoints)
{
	unsigned char OpData[3] = {0, 0, 0};
	unsigned short Operand = 0, MidAddr = 0;
	int EffectiveAddr = -1, FixupAddr = 0;
	OpData[0] = DebugMemCPU(Addr);

	BOOL is_break = FALSE;
	if (checkBreakpoints)
	{
		// opcode breakpoint
		if (BPcache[OpData[0]] & DEBUG_BREAK_OPCODE)
			is_break = TRUE;
		// exec breakpoint
		if (BPcache[Addr] & DEBUG_BREAK_EXEC)
			is_break = TRUE;
		// interrupt breakpoints
		if ((CPU::GotInterrupt == INTERRUPT_NMI) && (BPcache[0] & DEBUG_BREAK_NMI))
			is_break = TRUE;
		if ((CPU::GotInterrupt == INTERRUPT_IRQ) && (BPcache[0] & DEBUG_BREAK_IRQ))
			is_break = TRUE;
		if ((CPU::GotInterrupt == INTERRUPT_BRK) && (BPcache[0] & DEBUG_BREAK_BRK))
			is_break = TRUE;
		if ((LastScanline != PPU::SLnum) && (BPcache[PPU::SLnum + 1] & DEBUG_BREAK_SCANLINE))
			is_break = TRUE;
	}

	switch (TraceAddrMode[OpData[0]])
	{
	case IND:
		OpData[1] = DebugMemCPU(Addr+1);
		OpData[2] = DebugMemCPU(Addr+2);
		Operand = OpData[1] | (OpData[2] << 8);
		MidAddr = (Operand & 0xFF00) | ((Operand+1) & 0xFF);
		// Used only by JMP indirect, which does NOT handle page crossing correctly
		EffectiveAddr = DebugMemCPU(Operand) | (DebugMemCPU(MidAddr) << 8);
		if (checkBreakpoints && (
			IsBreakpoint(OpData[0], Operand, FALSE) ||
			IsBreakpoint(OpData[0], MidAddr, FALSE)
			))
			is_break = TRUE;
		break;
	case ADR:
		OpData[1] = DebugMemCPU(Addr+1);
		OpData[2] = DebugMemCPU(Addr+2);Operand = OpData[1] | (OpData[2] << 8);
		break;
	case ABS:
		OpData[1] = DebugMemCPU(Addr+1);
		OpData[2] = DebugMemCPU(Addr+2);Operand = OpData[1] | (OpData[2] << 8);
		EffectiveAddr = Operand;
		if (checkBreakpoints && IsBreakpoint(OpData[0], EffectiveAddr, FALSE))
			is_break = TRUE;
		break;
	case ABX:
		OpData[1] = DebugMemCPU(Addr+1);
		OpData[2] = DebugMemCPU(Addr+2);Operand = OpData[1] | (OpData[2] << 8);
		FixupAddr = (Operand & 0xFF00) | ((Operand + CPU::X) & 0xFF);
		EffectiveAddr = (Operand + CPU::X) & 0xFFFF;
		if (checkBreakpoints && (
			IsBreakpoint(OpData[0], FixupAddr, TRUE) ||
			IsBreakpoint(OpData[0], EffectiveAddr, FALSE)
			))
			is_break = TRUE;
		break;
	case ABY:
		OpData[1] = DebugMemCPU(Addr+1);
		OpData[2] = DebugMemCPU(Addr+2);Operand = OpData[1] | (OpData[2] << 8);
		FixupAddr = (Operand & 0xFF00) | ((Operand + CPU::Y) & 0xFF);
		EffectiveAddr = (Operand + CPU::Y) & 0xFFFF;
		if (checkBreakpoints && (
			IsBreakpoint(OpData[0], FixupAddr, TRUE) ||
			IsBreakpoint(OpData[0], EffectiveAddr, FALSE)
			))
			is_break = TRUE;
		break;
	case IMM:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = OpData[1];
		break;
	case ZPG:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = OpData[1];
		EffectiveAddr = Operand;
		if (checkBreakpoints && IsBreakpoint(OpData[0], EffectiveAddr, FALSE))
			is_break = TRUE;
		break;
	case ZPX:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = OpData[1];
		EffectiveAddr = (Operand + CPU::X) & 0xFF;
		if (checkBreakpoints && IsBreakpoint(OpData[0], EffectiveAddr, FALSE))
			is_break = TRUE;
		break;
	case ZPY:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = OpData[1];
		EffectiveAddr = (Operand + CPU::Y) & 0xFF;
		if (checkBreakpoints && IsBreakpoint(OpData[0], EffectiveAddr, FALSE))
			is_break = TRUE;
		break;
	case INX:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = OpData[1];
		MidAddr = (Operand + CPU::X) & 0xFF;
		EffectiveAddr = DebugMemCPU(MidAddr) | (DebugMemCPU((MidAddr+1) & 0xFF) << 8);
		if (checkBreakpoints && (
			IsBreakpoint(OpData[0], MidAddr, TRUE) ||
			IsBreakpoint(OpData[0], (MidAddr + 1) & 0xFF, TRUE) ||
			IsBreakpoint(OpData[0], EffectiveAddr, FALSE)
			))
			is_break = TRUE;
		break;
	case INY:
		OpData[1] = DebugMemCPU(Addr+1);
		Operand = OpData[1];
		MidAddr = DebugMemCPU(Operand) | (DebugMemCPU((Operand+1) & 0xFF) << 8);
		FixupAddr = (MidAddr & 0xFF00) | ((MidAddr + CPU::Y) & 0xFF);
		EffectiveAddr = (MidAddr + CPU::Y) & 0xFFFF;
		if (checkBreakpoints && (
			IsBreakpoint(OpData[0], Operand, TRUE) ||
			IsBreakpoint(OpData[0], (Operand + 1) & 0xFF, TRUE) ||
			IsBreakpoint(OpData[0], FixupAddr, TRUE) ||
			IsBreakpoint(OpData[0], EffectiveAddr, FALSE)
			))
			is_break = TRUE;
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
	// Special cases for Stack-modifying opcodes
	switch (OpData[0])
	{
	case 0x00: // BRK - push 3
		if (checkBreakpoints && IsBreakpoint(OpData[0], 0x100 | (CPU::SP - 2), FALSE))
			is_break = TRUE;
	case 0x20: // JSR - push 2
		if (checkBreakpoints && IsBreakpoint(OpData[0], 0x100 | (CPU::SP - 1), FALSE))
			is_break = TRUE;
	case 0x08: // PHP - push 1
	case 0x48: // PHA - push 1
		if (checkBreakpoints && IsBreakpoint(OpData[0], 0x100 | CPU::SP, FALSE))
			is_break = TRUE;
		break;

	case 0x40: // RTI - pop 3
		if (checkBreakpoints && IsBreakpoint(OpData[0], 0x100 | (CPU::SP + 3), FALSE))
			is_break = TRUE;

	case 0x60: // RTS - pop 2
		if (checkBreakpoints && IsBreakpoint(OpData[0], 0x100 | (CPU::SP + 2), FALSE))
			is_break = TRUE;

	case 0x28: // PLP - pop 1
	case 0x68: // PLA - pop 1
		if (checkBreakpoints && IsBreakpoint(OpData[0], 0x100 | (CPU::SP + 1), FALSE))
			is_break = TRUE;
		break;
	}
	// Use this for outputting to debug logfile
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
	// Use this for outputting to the debugger's Trace pane
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
	return is_break;
}

bool	UpdateCPU (void)
{
	TCHAR tps[64];
	int i, TpVal;
	unsigned short Addr;
	SCROLLINFO sinfo;

	// if we chose "Step", stop emulation
	if (Step)
		NES::DoStop = STOPMODE_NOW | STOPMODE_BREAK;
	// check for breakpoints
	if (DecodeInstruction((unsigned short)CPU::PC, NULL, NULL, TRUE))
		NES::DoStop = STOPMODE_NOW | STOPMODE_BREAK;

	LastScanline = PPU::SLnum;
	// if emulation wasn't stopped, don't bother updating the dialog
	if (!(NES::DoStop & STOPMODE_NOW))
		return false;

	inUpdate = TRUE;

	_stprintf(tps, _T("%04X"), CPU::PC);
	SetDlgItemText(CPUWnd, IDC_DEBUG_REG_PC, tps);

	_stprintf(tps, _T("%02X"), CPU::A);
	SetDlgItemText(CPUWnd, IDC_DEBUG_REG_A, tps);

	_stprintf(tps, _T("%02X"), CPU::X);
	SetDlgItemText(CPUWnd, IDC_DEBUG_REG_X, tps);

	_stprintf(tps, _T("%02X"), CPU::Y);
	SetDlgItemText(CPUWnd, IDC_DEBUG_REG_Y, tps);

	_stprintf(tps, _T("%02X"), CPU::SP);
	SetDlgItemText(CPUWnd, IDC_DEBUG_REG_SP, tps);

	CPU::JoinFlags();
	_stprintf(tps, _T("%02X"), CPU::P);
	SetDlgItemText(CPUWnd, IDC_DEBUG_REG_P, tps);

	CheckDlgButton(CPUWnd, IDC_DEBUG_FLAG_N, CPU::FN ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(CPUWnd, IDC_DEBUG_FLAG_V, CPU::FV ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(CPUWnd, IDC_DEBUG_FLAG_D, CPU::FD ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(CPUWnd, IDC_DEBUG_FLAG_I, CPU::FI ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(CPUWnd, IDC_DEBUG_FLAG_Z, CPU::FZ ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(CPUWnd, IDC_DEBUG_FLAG_C, CPU::FC ? BST_CHECKED : BST_UNCHECKED);

	CheckDlgButton(CPUWnd, IDC_DEBUG_IRQ_EXT, (CPU::WantIRQ & IRQ_EXTERNAL) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(CPUWnd, IDC_DEBUG_IRQ_PCM, (CPU::WantIRQ & IRQ_DPCM) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(CPUWnd, IDC_DEBUG_IRQ_FRAME, (CPU::WantIRQ & IRQ_FRAME) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(CPUWnd, IDC_DEBUG_IRQ_DEBUG, (CPU::WantIRQ & IRQ_DEBUG) ? BST_CHECKED : BST_UNCHECKED);

	SetDlgItemInt(CPUWnd, IDC_DEBUG_TIMING_CPU, CPU::Cycles, FALSE);

	SetDlgItemInt(CPUWnd, IDC_DEBUG_TIMING_SCANLINE, PPU::SLnum, TRUE);
	SetDlgItemInt(CPUWnd, IDC_DEBUG_TIMING_PPU, PPU::Clockticks, TRUE);

	_stprintf(tps, _T("%04X"), PPU::VRAMAddr);
	SetDlgItemText(CPUWnd, IDC_DEBUG_TIMING_VRAM, tps);

	for (i = 0; i < 16; i++)
	{
		static const int CPUPages[16] = {
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
		SetDlgItemText(CPUWnd, CPUPages[i], tps);
	}
	for (i = 0; i < 16; i++)
	{
		static const int PPUPages[16] = {
			IDC_DEBUG_BANK_PPU00, IDC_DEBUG_BANK_PPU04, IDC_DEBUG_BANK_PPU08, IDC_DEBUG_BANK_PPU0C, 
			IDC_DEBUG_BANK_PPU10, IDC_DEBUG_BANK_PPU14, IDC_DEBUG_BANK_PPU18, IDC_DEBUG_BANK_PPU1C, 
			IDC_DEBUG_BANK_PPU20, IDC_DEBUG_BANK_PPU24, IDC_DEBUG_BANK_PPU28, IDC_DEBUG_BANK_PPU2C,
			IDC_DEBUG_BANK_PPU30, IDC_DEBUG_BANK_PPU34, IDC_DEBUG_BANK_PPU38, IDC_DEBUG_BANK_PPU3C
		};

		if (EI.GetCHR_ROM1(i) >= 0)
			_stprintf(tps, _T("%03X"), EI.GetCHR_ROM1(i));
		else if (EI.GetCHR_RAM1(i) >= 0)
			_stprintf(tps, _T("A%02X"), EI.GetCHR_RAM1(i));
		else if (EI.GetCHR_NT1(i) >= 0)
			_stprintf(tps, _T("N%02X"), EI.GetCHR_NT1(i));
		else	_stprintf(tps, _T("???"));
		SetDlgItemText(CPUWnd, PPUPages[i], tps);
	}

	// update trace window - turn off redraw
	SendDlgItemMessage(CPUWnd, IDC_DEBUG_TRACE_LIST, WM_SETREDRAW, FALSE, 0);
	SendDlgItemMessage(CPUWnd, IDC_DEBUG_TRACE_LIST, LB_RESETCONTENT, 0, 0);

	Addr = (unsigned short)((TraceOffset == -1) ? CPU::PC : TraceOffset);
	TpVal = -1;

	sinfo.cbSize = sizeof(SCROLLINFO);
	sinfo.fMask = SIF_POS;
	sinfo.nPos = Addr;
	SetScrollInfo(GetDlgItem(CPUWnd, IDC_DEBUG_TRACE_SCROLL), SB_CTL, &sinfo, TRUE);

	for (i = 0; i < DEBUG_TRACELINES; i++)
	{
		if (Addr == CPU::PC)
			TpVal = i;
		DecodeInstruction(Addr, NULL, tps, FALSE);
		SendDlgItemMessage(CPUWnd, IDC_DEBUG_TRACE_LIST, LB_ADDSTRING, 0, (LPARAM)tps);
		Addr = Addr + AddrBytes[TraceAddrMode[DebugMemCPU(Addr)]];
	}
	// re-enable redraw just before we set the selection
	SendDlgItemMessage(CPUWnd, IDC_DEBUG_TRACE_LIST, WM_SETREDRAW, TRUE, 0);
	SendDlgItemMessage(CPUWnd, IDC_DEBUG_TRACE_LIST, LB_SETCURSEL, TpVal, 0);

	// update memory window - turn off redraw
	SendDlgItemMessage(CPUWnd, IDC_DEBUG_MEM_LIST, WM_SETREDRAW, FALSE, 0);
	SendDlgItemMessage(CPUWnd, IDC_DEBUG_MEM_LIST, LB_RESETCONTENT, 0, 0);

	if (IsDlgButtonChecked(CPUWnd, IDC_DEBUG_MEM_CPU))
	{
		sinfo.cbSize = sizeof(SCROLLINFO);
		sinfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
		sinfo.nPos = MemOffset;
		sinfo.nPage = DEBUG_MEMLINES;
		sinfo.nMin = 0;
		sinfo.nMax = 0xFFF;
		SetScrollInfo(GetDlgItem(CPUWnd, IDC_DEBUG_MEM_SCROLL), SB_CTL, &sinfo, TRUE);
		for (i = 0; i < DEBUG_MEMLINES; i++)
		{
			// past the end?
			if ((MemOffset + i) * 0x10 >= 0x10000)
				break;
			_stprintf(tps, _T("%04X:\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X"),
				(MemOffset + i) * 0x10,
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0x0)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0x1)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0x2)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0x3)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0x4)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0x5)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0x6)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0x7)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0x8)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0x9)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0xA)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0xB)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0xC)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0xD)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0xE)),
				DebugMemCPU((unsigned short)((MemOffset + i) * 0x10 + 0xF)));
			SendDlgItemMessage(CPUWnd, IDC_DEBUG_MEM_LIST, LB_ADDSTRING, 0, (LPARAM)tps);
		}
	}
	else if (IsDlgButtonChecked(CPUWnd, IDC_DEBUG_MEM_PPU))
	{
		sinfo.cbSize = sizeof(SCROLLINFO);
		sinfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
		sinfo.nPos = MemOffset;
		sinfo.nPage = DEBUG_MEMLINES;
		sinfo.nMin = 0;
		sinfo.nMax = 0x3FF;
		SetScrollInfo(GetDlgItem(CPUWnd, IDC_DEBUG_MEM_SCROLL), SB_CTL, &sinfo, TRUE);
		for (i = 0; i < DEBUG_MEMLINES; i++)
		{
			// past the end?
			if ((MemOffset + i) * 0x10 >= 0x4000)
				break;
			_stprintf(tps, _T("%04X:\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X"),
				(MemOffset + i) * 0x10,
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0x0)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0x1)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0x2)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0x3)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0x4)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0x5)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0x6)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0x7)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0x8)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0x9)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0xA)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0xB)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0xC)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0xD)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0xE)),
				DebugMemPPU((unsigned short)((MemOffset + i) * 0x10 + 0xF)));
			SendDlgItemMessage(CPUWnd, IDC_DEBUG_MEM_LIST, LB_ADDSTRING, 0, (LPARAM)tps);
		}
	}
	else if (IsDlgButtonChecked(CPUWnd, IDC_DEBUG_MEM_SPR))
	{
		sinfo.cbSize = sizeof(SCROLLINFO);
		sinfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
		sinfo.nPos = MemOffset;
		sinfo.nPage = DEBUG_MEMLINES;
		sinfo.nMin = 0;
		sinfo.nMax = 0xF;
		SetScrollInfo(GetDlgItem(CPUWnd, IDC_DEBUG_MEM_SCROLL), SB_CTL, &sinfo, TRUE);
		for (i = 0; i < DEBUG_MEMLINES; i++)
		{
			// past the end?
			if ((MemOffset + i) * 0x10 >= 0x100)
				break;
			_stprintf(tps, _T("%02X:\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X"),
				(MemOffset + i) * 0x10,
				PPU::Sprite[(MemOffset + i) * 0x10 + 0x0],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0x1],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0x2],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0x3],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0x4],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0x5],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0x6],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0x7],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0x8],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0x9],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0xA],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0xB],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0xC],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0xD],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0xE],
				PPU::Sprite[(MemOffset + i) * 0x10 + 0xF]);
			SendDlgItemMessage(CPUWnd, IDC_DEBUG_MEM_LIST, LB_ADDSTRING, 0, (LPARAM)tps);
		}
	}
	else if (IsDlgButtonChecked(CPUWnd, IDC_DEBUG_MEM_PAL))
	{
		sinfo.cbSize = sizeof(SCROLLINFO);
		sinfo.fMask = SIF_DISABLENOSCROLL | SIF_RANGE;
		sinfo.nPage = 0;
		sinfo.nMin = 0;
		sinfo.nMax = 0;
		SetScrollInfo(GetDlgItem(CPUWnd, IDC_DEBUG_MEM_SCROLL), SB_CTL, &sinfo, TRUE);
		for (i = 0; i < DEBUG_MEMLINES; i++)
		{
			// past the end?
			if ((MemOffset + i) * 0x10 >= 0x20)
				break;
			_stprintf(tps, _T("%02X:\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X"),
				(MemOffset + i) * 0x10,
				PPU::Palette[(MemOffset + i) * 0x10 + 0x0],
				PPU::Palette[(MemOffset + i) * 0x10 + 0x1],
				PPU::Palette[(MemOffset + i) * 0x10 + 0x2],
				PPU::Palette[(MemOffset + i) * 0x10 + 0x3],
				PPU::Palette[(MemOffset + i) * 0x10 + 0x4],
				PPU::Palette[(MemOffset + i) * 0x10 + 0x5],
				PPU::Palette[(MemOffset + i) * 0x10 + 0x6],
				PPU::Palette[(MemOffset + i) * 0x10 + 0x7],
				PPU::Palette[(MemOffset + i) * 0x10 + 0x8],
				PPU::Palette[(MemOffset + i) * 0x10 + 0x9],
				PPU::Palette[(MemOffset + i) * 0x10 + 0xA],
				PPU::Palette[(MemOffset + i) * 0x10 + 0xB],
				PPU::Palette[(MemOffset + i) * 0x10 + 0xC],
				PPU::Palette[(MemOffset + i) * 0x10 + 0xD],
				PPU::Palette[(MemOffset + i) * 0x10 + 0xE],
				PPU::Palette[(MemOffset + i) * 0x10 + 0xF]);
			SendDlgItemMessage(CPUWnd, IDC_DEBUG_MEM_LIST, LB_ADDSTRING, 0, (LPARAM)tps);
		}
	}

	// re-enable redraw now that we're done drawing it
	SendDlgItemMessage(CPUWnd, IDC_DEBUG_MEM_LIST, WM_SETREDRAW, TRUE, 0);
	inUpdate = FALSE;
	return true;
}

void	AddInst (void)
{
	if (Logging)
	{
		unsigned short Addr = (unsigned short)CPU::PC;
		char tps[64];
		DecodeInstruction(Addr, tps, NULL, FALSE);
		fwrite(tps, 1, strlen(tps), LogFile);
		CPU::JoinFlags();
		sprintf(tps, "  A:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3i,%3i CYC:%i\n", CPU::A, CPU::X, CPU::Y, CPU::P, CPU::SP, PPU::SLnum + ((PPU::SLnum < 0) ? PPU::SLEndFrame : 0), PPU::Clockticks, CPU::Cycles);
		fwrite(tps, 1, strlen(tps), LogFile);
	}
}

void	DrawTile (unsigned long *dest, int PPUaddr, int palette, int pitch, bool hflip = false, bool vflip = false)
{
	int sy, sx;
	int byte0, byte1, color;
	for (sy = 0; sy < 8; sy++)
	{
		byte0 = DebugMemPPU((unsigned short)(PPUaddr + (vflip ? 7 - sy : sy)));
		byte1 = DebugMemPPU((unsigned short)(PPUaddr + (vflip ? 7 - sy : sy) + 8));
		if (hflip)
		{
			byte0 = PPU::ReverseCHR[byte0];
			byte1 = PPU::ReverseCHR[byte1];
		}
		for (sx = 0; sx < 8; sx++)
		{
			color = ((byte0 & 0x80) >> 7) | ((byte1 & 0x80) >> 6);
			if (color)
				color |= palette << 2;
			color = GFX::Palette32[PPU::Palette[color]];
			dest[sx] = color;
			byte0 <<= 1;
			byte1 <<= 1;
		}
		dest += pitch;
	}
}

void	DrawTileStretch (unsigned long *dest, int PPUaddr, int palette, int width, int height, int pitch, bool hflip = false, bool vflip = false)
{
	int sy, sx, py, px;
	int byte0, byte1, color;
	for (sy = 0; sy < 8; sy++)
	{
		byte0 = DebugMemPPU((unsigned short)(PPUaddr + (vflip ? 7 - sy : sy)));
		byte1 = DebugMemPPU((unsigned short)(PPUaddr + (vflip ? 7 - sy : sy) + 8));
		if (hflip)
		{
			byte0 = PPU::ReverseCHR[byte0];
			byte1 = PPU::ReverseCHR[byte1];
		}
		for (sx = 0; sx < 8; sx++)
		{
			color = ((byte0 & 0x80) >> 7) | ((byte1 & 0x80) >> 6);
			if (color)
				color |= palette << 2;
			color = GFX::Palette32[PPU::Palette[color]];
			for (py = 0; py < height; py++)
				for (px = 0; px < width; px++)
					dest[px + sx * width + py * pitch] = color;
			byte0 <<= 1;
			byte1 <<= 1;
		}
		dest += pitch * height;
	}
}

int	CheckFlip (int tile)
{
	if (TileFlip)
		tile = (tile & 0x1EE) | ((tile & 0x10) >> 4) | ((tile & 0x1) << 4);
	return tile;
}

void	UpdatePPU (void)
{
	int MemAddr;
	int x, y;

	// If a Sprite is being previewed and it changed, repaint the Nametable pane too
	if ((DetailType == DEBUG_DETAIL_SPRITE) && SprChanged)
		NTabChanged = TRUE;

	if (PalChanged)
	{
		BOOL palChanged[8] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
		if (PalCache[0] != PPU::Palette[0])
		{
			palChanged[0] = palChanged[1] = palChanged[2] = palChanged[3] = TRUE;
			palChanged[4] = palChanged[5] = palChanged[6] = palChanged[7] = TRUE;
		}
		for (int i = 1; i < 0x20; i++)
		{
			if (PalCache[i] != PPU::Palette[i])
				palChanged[i >> 2] = TRUE;
			PalCache[i] = PPU::Palette[i];
		}

		unsigned char color;
		// updating palette also invalidates pattern tables, nametable, and sprites
		if (palChanged[Palette])
			PatChanged = TRUE;
		if (palChanged[0] || palChanged[1] || palChanged[2] || palChanged[3])
			NTabChanged = TRUE;
		if (palChanged[4] || palChanged[5] || palChanged[6] || palChanged[7])
			SprChanged = TRUE;
		if (DetailType == DEBUG_DETAIL_PALETTE)
			DetChanged = TRUE;

		for (x = 0; x < 16; x++)
		{
			for (y = 0; y < 2; y++)
			{
				HBRUSH brush;
				RECT rect;
				color = PPU::Palette[y * 16 + x];
				rect.top = y * (D_PAL_H / 2);
				rect.bottom = rect.top + (D_PAL_H / 2);
				rect.left = x * (D_PAL_W / 16);
				rect.right = rect.left + (D_PAL_W / 16);
				brush = CreateSolidBrush(RGB(GFX::RawPalette[0][color][0], GFX::RawPalette[0][color][1], GFX::RawPalette[0][color][2]));
				FillRect(PaletteDC, &rect, brush);
				DeleteObject(brush);
			}
		}
		RedrawWindow(GetDlgItem(PPUWnd, IDC_DEBUG_PPU_PALETTE), NULL, NULL, RDW_INVALIDATE);
		PalChanged = FALSE;
	}

	if (PatChanged)
	{
		int t;
		static unsigned long PatternArray[D_PAT_W * D_PAT_H];
		BITMAPINFO bmi;

		// updating pattern table also makes nametable and sprites dirty
		NTabChanged = TRUE;
		SprChanged = TRUE;
		if (DetailType == DEBUG_DETAIL_PATTERN)
			DetChanged = TRUE;

		for (t = 0; t < 2; t++)
		{
			for (y = 0; y < 16; y++)
			{
				for (x = 0; x < 16; x++)
				{
					MemAddr = CheckFlip((t << 8) | (y << 4) | x) << 4;
					DrawTile(PatternArray + y * 8 * D_PAT_W + x * 8 + t * 128, MemAddr, Palette, D_PAT_W);
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
		SetDIBits(PatternDC, PatternBMP, 0, D_PAT_H, PatternArray, &bmi, DIB_RGB_COLORS);

		RedrawWindow(GetDlgItem(PPUWnd, IDC_DEBUG_PPU_PATTERN), NULL, NULL, RDW_INVALIDATE);
		PatChanged = FALSE;
	}

	if (NTabChanged)
	{
		int AttribVal, AttribNum;
		int NT = Nametable;
		static unsigned long NameArray[D_NAM_W * D_NAM_H];
		BITMAPINFO bmi;

		if (DetailType == DEBUG_DETAIL_NAMETABLE)
			DetChanged = TRUE;

		for (y = 0; y < 30; y++)
		{
			for (x = 0; x < 32; x++)
			{
				AttribNum = (((x & 2) >> 1) | (y & 2)) << 1;
				AttribVal = (DebugMemPPU(0x23C0 | (NT << 10) | ((y << 1) & 0x38) | (x >> 2)) >> AttribNum) & 3;
				MemAddr = ((PPU::Reg2000 & 0x10) << 8) | (DebugMemPPU(0x2000 | (NT << 10) | (y << 5) | x) << 4);
				DrawTile(NameArray + y * 8 * D_NAM_W + x * 8, MemAddr, AttribVal, D_NAM_W);
			}
		}

		// If a sprite is being viewed in the Detail pane, draw a border around its position in the Nametable pane
		if (DetailType == DEBUG_DETAIL_SPRITE)
		{
			x = PPU::Sprite[(DetailNum << 2) | 3];
			y = PPU::Sprite[(DetailNum << 2) | 0] + 1;
			int height = (PPU::Reg2000 & 0x20) ? 16 : 8;

			// Make sure the sprite is actually visible
			if (y < D_NAM_H)
			{
				// Draw vertical lines for the left and right edges
				for (int sy = y; sy < y + height; sy++)
				{
					if (sy >= D_NAM_H)
						break;
					// Any explicit color could blend in with the background
					// Instead, use the RGB inverse so it'll always be visible
					NameArray[sy * D_NAM_W + x] ^= 0xFFFFFF;
					if (x + 7 < D_NAM_W)
						NameArray[sy * D_NAM_W + x + 7] ^= 0xFFFFFF;
				}

				// Draw horizontal lines for the top and bottom edges
				// Skip the far left and far right pixels, since we already did them
				for (int sx = x + 1; sx < x + 7; sx++)
				{
					if (sx >= D_NAM_W)
						break;
					NameArray[y * D_NAM_W + sx] ^= 0xFFFFFF;
					if (y + height - 1 < D_NAM_H)
						NameArray[(y + height - 1) * D_NAM_W + sx] ^= 0xFFFFFF;
				}
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
		SetDIBits(NameDC, NameBMP, 0, D_NAM_H, NameArray, &bmi, DIB_RGB_COLORS);

		RedrawWindow(GetDlgItem(PPUWnd, IDC_DEBUG_PPU_NAMETABLE), NULL, NULL, RDW_INVALIDATE);
		NTabChanged = FALSE;
	}

	if (SprChanged)
	{
		int SprNum, Attr, TileNum;
		static unsigned long SprArray[D_SPR_W * D_SPR_H];
		BITMAPINFO bmi;

		if (DetailType == DEBUG_DETAIL_SPRITE)
			DetChanged = TRUE;

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
				TileNum = PPU::Sprite[(SprNum << 2) | 1];
				Attr = PPU::Sprite[(SprNum << 2) | 2];
				if (PPU::Reg2000 & 0x20)
				{
					MemAddr = ((TileNum & 0xFE) << 4) | ((TileNum & 0x01) << 12);
					DrawTile(SprArray + y * 24 * D_SPR_W + x * 16, MemAddr + ((Attr & 0x80) ? 16 : 0), 4 | (Attr & 3), D_SPR_W, Attr & 0x40, Attr & 0x80);
					DrawTile(SprArray + y * 24 * D_SPR_W + x * 16 + 8 * D_SPR_W, MemAddr + ((Attr & 0x80) ? 0 : 16), 4 | (Attr & 3), D_SPR_W, Attr & 0x40, Attr & 0x80);
				}
				else
				{
					MemAddr = (TileNum << 4) | ((PPU::Reg2000 & 0x08) << 9);
					DrawTile(SprArray + y * 24 * D_SPR_W + x * 16, MemAddr, 4 | (Attr & 3), D_SPR_W, Attr & 0x40, Attr & 0x80);
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
		SetDIBits(SpriteDC, SpriteBMP, 0, D_SPR_H, SprArray, &bmi, DIB_RGB_COLORS);

		RedrawWindow(GetDlgItem(PPUWnd, IDC_DEBUG_PPU_SPRITE), NULL, NULL, RDW_INVALIDATE);
		SprChanged = FALSE;
	}

	if (DetChanged)
	{
		unsigned char tile, color, attr;
		TCHAR tpstr[16];
		HBRUSH brush;
		RECT rect;
		static unsigned long TileArray[D_TIL_W * D_TIL_H];
		BITMAPINFO bmi;
		BOOL DrawBitmap = FALSE;

		x = GetSysColor(COLOR_BTNFACE);
		// convert color to RGB (from Windows BGR)
		x = (GetRValue(x) << 16) | (GetGValue(x) << 8) | (GetBValue(x) << 0);
		for (y = 0; y < D_TIL_H * D_TIL_W; y++)
			TileArray[y] = x;

		switch (DetailType)
		{
		case DEBUG_DETAIL_NONE:
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_SELTYPE, _T("None"));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP1TYPE, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP1VAL, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP2TYPE, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP2VAL, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP3TYPE, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP3VAL, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP4TYPE, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP4VAL, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP5TYPE, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP5VAL, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP6TYPE, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP6VAL, _T(""));

			DrawBitmap = TRUE;
			break;
		case DEBUG_DETAIL_NAMETABLE:
			tile = DebugMemPPU(0x2000 | DetailNum);
			color = DebugMemPPU(0x23C0 | (DetailNum & 0xC00) | ((DetailNum >> 4) & 0x38) | ((DetailNum >> 2) & 0x07)) >> ((DetailNum & 2) | ((DetailNum >> 4) & 4)) & 0x3;

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_SELTYPE, _T("Nametable"));

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP1TYPE, _T("Address"));
			_stprintf(tpstr, _T("%04X"), 0x2000 | DetailNum);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP1VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP2TYPE, _T("X"));
			_stprintf(tpstr, _T("%02X"), DetailNum & 0x1F);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP2VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP3TYPE, _T("Y"));
			_stprintf(tpstr, _T("%02X"), (DetailNum >> 5) & 0x1F);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP3VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP4TYPE, _T("Table"));
			_stprintf(tpstr, _T("%i"), (DetailNum >> 10) & 0x3);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP4VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP5TYPE, _T("Tile"));
			_stprintf(tpstr, _T("%02X"), tile);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP5VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP6TYPE, _T("Palette"));
			_stprintf(tpstr, _T("%i"), color);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP6VAL, tpstr);

			MemAddr = (tile << 4) | ((PPU::Reg2000 & 0x10) << 8);
			DrawTileStretch(TileArray, MemAddr, color, 8, 8, D_TIL_W);
			DrawBitmap = TRUE;
			break;
		case DEBUG_DETAIL_SPRITE:
			tile = PPU::Sprite[(DetailNum << 2) | 1];
			color = PPU::Sprite[(DetailNum << 2) | 2] & 3;
			attr = PPU::Sprite[(DetailNum << 2) | 2] & 0xC0;

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_SELTYPE, _T("Sprite"));

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP1TYPE, _T("Number"));
			_stprintf(tpstr, _T("%02X"), DetailNum);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP1VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP2TYPE, _T("X"));
			_stprintf(tpstr, _T("%02X"), PPU::Sprite[(DetailNum << 2) | 3]);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP2VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP3TYPE, _T("Y"));
			_stprintf(tpstr, _T("%02X"), PPU::Sprite[(DetailNum << 2) | 0]);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP3VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP4TYPE, _T("Tile"));
			_stprintf(tpstr, _T("%02X"), tile);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP4VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP5TYPE, _T("Color"));
			_stprintf(tpstr, _T("%i"), color);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP5VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP6TYPE, _T("Flags"));
			tpstr[0] = 0;
			if (PPU::Sprite[(DetailNum << 2) | 2] & 0x40)
				_tcscat(tpstr, _T("H "));
			if (PPU::Sprite[(DetailNum << 2) | 2] & 0x80)
				_tcscat(tpstr, _T("V "));
			if (PPU::Sprite[(DetailNum << 2) | 2] & 0x20)
				_tcscat(tpstr, _T("BG "));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP6VAL, tpstr);

			if (PPU::Reg2000 & 0x20)
			{
				MemAddr = ((tile & 0xFE) << 4) | ((tile & 0x01) << 12);
				DrawTileStretch(TileArray + 16, MemAddr + ((attr & 0x80) ? 16 : 0), color | 4, 4, 4, D_TIL_W, attr & 0x40, attr & 0x80);
				DrawTileStretch(TileArray + 16 + D_TIL_W * 32, MemAddr + ((attr & 0x80) ? 0 : 16), color | 4, 4, 4, D_TIL_W, attr & 0x40, attr & 0x80);
			}
			else
			{
				MemAddr = (tile << 4) | ((PPU::Reg2000 & 0x08) << 9);
				DrawTileStretch(TileArray, MemAddr, color | 4, 8, 8, D_TIL_W, attr & 0x40, attr & 0x80);
			}
			DrawBitmap = TRUE;
			break;
		case DEBUG_DETAIL_PATTERN:
			tile = DetailNum & 0xFF;
			color = Palette;
			MemAddr = DetailNum << 4;

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_SELTYPE, _T("Pattern Table"));

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP1TYPE, _T("Address"));
			_stprintf(tpstr, _T("%04X"), MemAddr);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP1VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP2TYPE, _T("Tile"));
			_stprintf(tpstr, _T("%02X"), tile);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP2VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP3TYPE, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP3VAL, _T(""));

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP4TYPE, _T("Table"));
			_stprintf(tpstr, _T("%i"), DetailNum >> 8);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP4VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP5TYPE, _T("Usage"));
			tpstr[0] = 0;
			if (((PPU::Reg2000 & 0x10) << 4) == (DetailNum & 0x100))
				_tcscat(tpstr, _T("BG "));
			if (PPU::Reg2000 & 0x20)
				_tcscat(tpstr, _T("SPR16 "));
			else if (((PPU::Reg2000 & 0x08) << 5) == (DetailNum & 0x100))
				_tcscat(tpstr, _T("SPR "));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP5VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP6TYPE, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP6VAL, _T(""));

			DrawTileStretch(TileArray, MemAddr, color, 8, 8, D_TIL_W);
			DrawBitmap = TRUE;
			break;
		case DEBUG_DETAIL_PALETTE:
			color = PPU::Palette[DetailNum];
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_SELTYPE, _T("Palette"));

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP1TYPE, _T("Address"));
			_stprintf(tpstr, _T("%04X"), 0x3F00 | DetailNum);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP1VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP2TYPE, _T("ID"));
			_stprintf(tpstr, _T("%s%i"), (DetailNum & 0x10) ? _T("SPR") : _T("BG"), (DetailNum >> 2) & 0x3);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP2VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP3TYPE, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP3VAL, _T(""));

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP4TYPE, _T("Color"));
			_stprintf(tpstr, _T("%02X"), color);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP4VAL, tpstr);

			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP5TYPE, _T("Offset"));
			_stprintf(tpstr, _T("%i"), DetailNum & 0x3);
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP5VAL, tpstr);
			
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP6TYPE, _T(""));
			SetDlgItemText(PPUWnd, IDC_DEBUG_PPU_PROP6VAL, _T(""));

			GetClientRect(GetDlgItem(PPUWnd, IDC_DEBUG_PPU_TILE), &rect);
			brush = CreateSolidBrush(RGB(GFX::RawPalette[0][color][0], GFX::RawPalette[0][color][1], GFX::RawPalette[0][color][2]));
			FillRect(TileDC, &rect, brush);
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
			SetDIBits(TileDC, TileBMP, 0, D_TIL_H, TileArray, &bmi, DIB_RGB_COLORS);
		}
		RedrawWindow(GetDlgItem(PPUWnd, IDC_DEBUG_PPU_TILE), NULL, NULL, RDW_INVALIDATE);
		DetChanged = FALSE;
	}
}

void	Update (int UpdateMode)
{
	bool force = false;
	if ((Mode & DEBUG_MODE_CPU) && (UpdateMode & DEBUG_MODE_CPU))
		force = UpdateCPU();
	if ((Mode & DEBUG_MODE_PPU) && ((UpdateMode & DEBUG_MODE_PPU) || force))
		UpdatePPU();
}

void	SetDetail (int type, int num)
{
	if ((DetailType == type) && (DetailNum == num))
		return;

	// If a Sprite is (or was) being previewed, update the Nametable pane too
	if ((DetailType == DEBUG_DETAIL_SPRITE) || (type == DEBUG_DETAIL_SPRITE))
		NTabChanged = TRUE;

	DetailType = type;
	DetailNum = num;
	DetChanged = TRUE;
	UpdatePPU();
}

void	DumpCPU (void)
{
	TCHAR filename[MAX_PATH];
	struct tm *newtime;
	time_t aclock;
	FILE *out;
	int i;

	if (!NES::ROMLoaded)
		return;

	time(&aclock);
	newtime = localtime(&aclock);

	_stprintf(filename, _T("%s\\Dumps\\%s.%04i%02i%02i_%02i%02i%02i.cpumem"), DataPath, States::BaseFilename, 
		newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
	out = _tfopen(filename, _T("wb"));
	fwrite(CPU::RAM, 1, 0x800, out);
	for (i = 4; i < 16; i++)
		if (CPU::PRGPointer[i])
			fwrite(CPU::PRGPointer[i], 1, 0x1000, out);
	fclose(out);
}

void	DumpPPU (void)
{
	TCHAR filename[MAX_PATH];
	struct tm *newtime;
	time_t aclock;
	FILE *out;
	int i;

	if (!NES::ROMLoaded)
		return;

	time(&aclock);
	newtime = localtime(&aclock);

	_stprintf(filename, _T("%s\\Dumps\\%s.%04i%02i%02i_%02i%02i%02i.ppumem"), DataPath, States::BaseFilename, 
		newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
	out = _tfopen(filename, _T("wb"));
	for (i = 0; i < 12; i++)
		fwrite(PPU::CHRPointer[i], 1, 0x400, out);
	fwrite(PPU::Sprite, 1, 0x100, out);
	fwrite(PPU::Palette, 1, 0x20, out);
	fclose(out);
}

void	CacheBreakpoints (void)
{
	int i;
	ZeroMemory(BPcache, sizeof(BPcache));
	for (auto bp = Breakpoints.begin(); bp != Breakpoints.end(); bp++)
	{
		if (!bp->enabled)
			continue;
		if (bp->type & (DEBUG_BREAK_EXEC | DEBUG_BREAK_READ | DEBUG_BREAK_WRITE))
		{
			for (i = bp->addr_start; i <= bp->addr_end; i++)
				BPcache[i] |= bp->type;
		}
		if (bp->type & DEBUG_BREAK_OPCODE)
			BPcache[bp->opcode] |= bp->type;
		if (bp->type & (DEBUG_BREAK_NMI | DEBUG_BREAK_IRQ | DEBUG_BREAK_BRK))
			BPcache[0] |= bp->type;
		if (bp->type & DEBUG_BREAK_SCANLINE)
			BPcache[bp->scanline] |= bp->type;
	}
}

struct tBreakpoint *GetBreakpoint (HWND hwndDlg, int &line)
{
	line = SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_GETCURSEL, 0, 0);
	if (line != -1)
		return &Breakpoints[line];
	else	return NULL;
}

void	SetBreakpoint (HWND hwndDlg, int line)
{
	if (line == -2)
		line = SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_GETCURSEL, 0, 0);
	int lines = SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_GETCOUNT, 0, 0);
	if (line >= lines)
		line = lines - 1;
	SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_SETCURSEL, (WPARAM)line, 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_EDIT), (line != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_DELETE), (line != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_UP), (line > 0));
	EnableWindow(GetDlgItem(hwndDlg, IDC_DEBUG_BREAK_DOWN), ((line != -1) && (line < lines - 1)));
}

// Editbox subclass to allow typing negative scanline numbers
// Adapted from code borrowed from The Old New Thing
// https://devblogs.microsoft.com/oldnewthing/20190222-00/?p=101064
LRESULT CALLBACK SignedIntegerSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, SignedIntegerSubclassProc, uIdSubclass);
		break;
	case WM_CHAR:
		TCHAR ch = (TCHAR)wParam;
		if (ch < _T(' '))
			break;	// let control character through
		else if ((ch == _T('-')) && (LOWORD(SendMessage(hwnd, EM_GETSEL, 0, 0)) == 0))
			break;	// let hyphen-minus through, but only as first character
		else if ((ch >= _T('0')) && (ch <= _T('9')))
			break;	// let digit through
		MessageBeep(0);	// otherwise invalid
		return 0;
	}
	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

INT_PTR CALLBACK BreakpointProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	TCHAR tpc[8];
	struct tBreakpoint *bp = (struct tBreakpoint *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	int line, len, Addr;
	TCHAR *str;

	int addr1, addr2, opcode, type, scanline, enabled;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		SetWindowSubclass(GetDlgItem(hwndDlg, IDC_BREAK_SLNUM), SignedIntegerSubclassProc, 0, 0);
		bp = (struct tBreakpoint *)lParam;
		if (bp == (struct tBreakpoint *)(size_t)0xFFFFFFFF)
		{
			bp = NULL;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, NULL);
			CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_BRK, IDC_BREAK_EXEC);

			line = SendDlgItemMessage(CPUWnd, IDC_DEBUG_TRACE_LIST, LB_GETCURSEL, 0, 0);
			if (line == -1)
				break;
			len = SendDlgItemMessage(CPUWnd, IDC_DEBUG_TRACE_LIST, LB_GETTEXTLEN, line, 0);
			str = new TCHAR[len + 1];
			SendDlgItemMessage(CPUWnd, IDC_DEBUG_TRACE_LIST, LB_GETTEXT, line, (LPARAM)str);
			Addr = _tcstol(str, NULL, 16);
			_stprintf(tpc, _T("%04X"), Addr);
			SetDlgItemText(hwndDlg, IDC_BREAK_ADDR1, tpc);
			delete[] str;

			SetDlgItemText(hwndDlg, IDC_BREAK_ADDR2, _T(""));
			SetDlgItemText(hwndDlg, IDC_BREAK_OPNUM, _T("00"));
			SetDlgItemInt(hwndDlg, IDC_BREAK_SLNUM, 0, TRUE);
			CheckDlgButton(hwndDlg, IDC_BREAK_ENABLED, BST_CHECKED);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_SLNUM), FALSE);
			SetFocus(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1));
			SendDlgItemMessage(hwndDlg, IDC_BREAK_ADDR1, EM_SETSEL, 0, -1);
		}
		else if (bp != NULL)
		{
			switch (bp->type)
			{
			case DEBUG_BREAK_EXEC:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_SCANLINE, IDC_BREAK_EXEC);
				break;
			case DEBUG_BREAK_READ:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_SCANLINE, IDC_BREAK_READ);
				break;
			case DEBUG_BREAK_WRITE:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_SCANLINE, IDC_BREAK_WRITE);
				break;
			case DEBUG_BREAK_READ | DEBUG_BREAK_WRITE:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_SCANLINE, IDC_BREAK_ACCESS);
				break;
			case DEBUG_BREAK_OPCODE:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_SCANLINE, IDC_BREAK_OPCODE);
				break;
			case DEBUG_BREAK_NMI:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_SCANLINE, IDC_BREAK_NMI);
				break;
			case DEBUG_BREAK_IRQ:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_SCANLINE, IDC_BREAK_IRQ);
				break;
			case DEBUG_BREAK_BRK:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_SCANLINE, IDC_BREAK_BRK);
				break;
			case DEBUG_BREAK_SCANLINE:
				CheckRadioButton(hwndDlg, IDC_BREAK_EXEC, IDC_BREAK_SCANLINE, IDC_BREAK_SCANLINE);
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
				SetDlgItemInt(hwndDlg, IDC_BREAK_SLNUM, 0, TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_SLNUM), FALSE);
				SetFocus(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1));
				SendDlgItemMessage(hwndDlg, IDC_BREAK_ADDR1, EM_SETSEL, 0, -1);
				break;
			case DEBUG_BREAK_OPCODE:
				SetDlgItemText(hwndDlg, IDC_BREAK_ADDR1, _T("0000"));
				SetDlgItemText(hwndDlg, IDC_BREAK_ADDR2, _T(""));
				_stprintf(tpc, _T("%02X"), bp->opcode);
				SetDlgItemText(hwndDlg, IDC_BREAK_OPNUM, tpc);
				SetDlgItemInt(hwndDlg, IDC_BREAK_SLNUM, 0, TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_SLNUM), FALSE);
				SetFocus(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM));
				SendDlgItemMessage(hwndDlg, IDC_BREAK_OPNUM, EM_SETSEL, 0, -1);
				break;
			case DEBUG_BREAK_NMI:
			case DEBUG_BREAK_IRQ:
			case DEBUG_BREAK_BRK:
				SetDlgItemText(hwndDlg, IDC_BREAK_ADDR1, _T("0000"));
				SetDlgItemText(hwndDlg, IDC_BREAK_ADDR2, _T(""));
				SetDlgItemText(hwndDlg, IDC_BREAK_OPNUM, _T("00"));
				SetDlgItemInt(hwndDlg, IDC_BREAK_SLNUM, 0, TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_SLNUM), FALSE);
				SetFocus(GetDlgItem(hwndDlg, IDC_BREAK_ENABLED));
				break;
			case DEBUG_BREAK_SCANLINE:
				SetDlgItemText(hwndDlg, IDC_BREAK_ADDR1, _T("0000"));
				SetDlgItemText(hwndDlg, IDC_BREAK_ADDR2, _T(""));
				SetDlgItemText(hwndDlg, IDC_BREAK_OPNUM, _T("00"));
				SetDlgItemInt(hwndDlg, IDC_BREAK_SLNUM, bp->scanline - 1, TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_SLNUM), TRUE);
				SetFocus(GetDlgItem(hwndDlg, IDC_BREAK_SLNUM));
				SendDlgItemMessage(hwndDlg, IDC_BREAK_SLNUM, EM_SETSEL, 0, -1);
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
			SetDlgItemInt(hwndDlg, IDC_BREAK_SLNUM, 0, TRUE);
			CheckDlgButton(hwndDlg, IDC_BREAK_ENABLED, BST_CHECKED);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_SLNUM), FALSE);
			SetFocus(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1));
			SendDlgItemMessage(hwndDlg, IDC_BREAK_ADDR1, EM_SETSEL, 0, -1);
		}
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
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_SLNUM), FALSE);
			return TRUE;
		case IDC_BREAK_OPCODE:
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_SLNUM), FALSE);
			return TRUE;
		case IDC_BREAK_NMI:
		case IDC_BREAK_IRQ:
		case IDC_BREAK_BRK:
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_SLNUM), FALSE);
			return TRUE;
		case IDC_BREAK_SCANLINE:
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR1), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_ADDR2), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_OPNUM), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BREAK_SLNUM), TRUE);
			return TRUE;

		case IDOK:
			GetDlgItemText(hwndDlg, IDC_BREAK_ADDR1, tpc, 5);
			addr1 = _tcstol(tpc, NULL, 16);
			GetDlgItemText(hwndDlg, IDC_BREAK_ADDR2, tpc, 5);
			addr2 = _tcstol(tpc, NULL, 16);
			if (!_tcslen(tpc))
				addr2 = addr1;
			GetDlgItemText(hwndDlg, IDC_BREAK_OPNUM, tpc, 3);
			opcode = _tcstol(tpc, NULL, 16);
			scanline = (int)GetDlgItemInt(hwndDlg, IDC_BREAK_SLNUM, NULL, TRUE);
			enabled = (IsDlgButtonChecked(hwndDlg, IDC_BREAK_ENABLED) == BST_CHECKED);

			type = 0;
			if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_EXEC) == BST_CHECKED)
				type = DEBUG_BREAK_EXEC;
			if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_READ) == BST_CHECKED)
				type = DEBUG_BREAK_READ;
			if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_WRITE) == BST_CHECKED)
				type = DEBUG_BREAK_WRITE;
			if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_ACCESS) == BST_CHECKED)
				type = DEBUG_BREAK_READ | DEBUG_BREAK_WRITE;
			if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_OPCODE) == BST_CHECKED)
				type = DEBUG_BREAK_OPCODE;
			if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_NMI) == BST_CHECKED)
				type = DEBUG_BREAK_NMI;
			if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_IRQ) == BST_CHECKED)
				type = DEBUG_BREAK_IRQ;
			if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_BRK) == BST_CHECKED)
				type = DEBUG_BREAK_BRK;
			if (IsDlgButtonChecked(hwndDlg, IDC_BREAK_SCANLINE) == BST_CHECKED)
				type = DEBUG_BREAK_SCANLINE;

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
			if (type & (DEBUG_BREAK_SCANLINE))
			{
				if ((scanline < -1) || (scanline >= PPU::SLEndFrame))
				{
					MessageBox(hwndDlg, _T("Invalid scanline specified!"), _T("Breakpoint"), MB_ICONERROR);
					break;
				}
				// End-of-frame scanline is converted to -1 so we don't
				// have to translate the state retrieved from the PPU
				if (scanline == PPU::SLEndFrame - 1)
					scanline = -1;
				// Scanline numbers are also biased by +1
				// to avoid storing negative values
				scanline++;
			}
			if (bp == NULL)
			{
				Breakpoints.push_back(tBreakpoint());
				bp = &Breakpoints[Breakpoints.size() - 1];
			}
			bp->type = (unsigned char)type;
			bp->opcode = (unsigned char)opcode;
			bp->scanline = (unsigned short)scanline;
			bp->enabled = (unsigned char)enabled;
			bp->addr_start = (unsigned short)addr1;
			bp->addr_end = (unsigned short)addr2;
			bp->updateDescription();
			EndDialog(hwndDlg, (INT_PTR)bp);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwndDlg, (INT_PTR)NULL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK CPUProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	static /* const */ int mem_tabs[16] = {24, 34, 44, 54, 66, 76, 86, 96, 108, 118, 128, 138, 150, 160, 170, 180};
	static /* const */ int trace_tabs[3] = {25, 60, 80};	// Win9x requires these to be in writable memory
	TCHAR tpc[8];
	SCROLLINFO sinfo;
	struct tBreakpoint *bp;
	int line, len, Addr;
	TCHAR *str;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		if (TraceOffset == -1)
		{
			SetDlgItemText(hwndDlg, IDC_DEBUG_CONT_SEEKADDR, _T(""));
			CheckRadioButton(hwndDlg, IDC_DEBUG_CONT_SEEKPC, IDC_DEBUG_CONT_SEEKTO, IDC_DEBUG_CONT_SEEKPC);
		}
		else
		{
			_stprintf(tpc, _T("%04X"), TraceOffset);
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

		for (auto iter = Breakpoints.begin(); iter != Breakpoints.end(); iter++)
			SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_ADDSTRING, 0, (LPARAM)iter->desc);
		SetBreakpoint(hwndDlg, 0);
		return FALSE;

	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 

		// if UpdateCPU() is running, don't listen to any notifications
		if (inUpdate)
			break;

		switch (wmId)
		{
		case IDC_DEBUG_REG_A:
			// Don't bother modifying registers while it's emulating at full speed
			if (NES::Running)
				break;
			GetDlgItemText(hwndDlg, IDC_DEBUG_REG_A, tpc, 3);
			CPU::A = (unsigned char)_tcstol(tpc, NULL, 16);
			if (wmEvent == EN_KILLFOCUS)
				UpdateCPU();
			return TRUE;
		case IDC_DEBUG_REG_X:
			if (NES::Running)
				break;
			GetDlgItemText(hwndDlg, IDC_DEBUG_REG_X, tpc, 3);
			CPU::X = (unsigned char)_tcstol(tpc, NULL, 16);
			if (wmEvent == EN_KILLFOCUS)
				UpdateCPU();
			return TRUE;
		case IDC_DEBUG_REG_Y:
			if (NES::Running)
				break;
			GetDlgItemText(hwndDlg, IDC_DEBUG_REG_Y, tpc, 3);
			CPU::Y = (unsigned char)_tcstol(tpc, NULL, 16);
			if (wmEvent == EN_KILLFOCUS)
				UpdateCPU();
			return TRUE;
		case IDC_DEBUG_REG_P:
			if (NES::Running)
				break;
			GetDlgItemText(hwndDlg, IDC_DEBUG_REG_P, tpc, 3);
			CPU::P = (unsigned char)_tcstol(tpc, NULL, 16);
			CPU::SplitFlags();
			if (wmEvent == EN_KILLFOCUS)
				UpdateCPU();
			return TRUE;
		case IDC_DEBUG_REG_SP:
			if (NES::Running)
				break;
			GetDlgItemText(hwndDlg, IDC_DEBUG_REG_SP, tpc, 3);
			CPU::SP = (unsigned char)_tcstol(tpc, NULL, 16);
			if (wmEvent == EN_KILLFOCUS)
				UpdateCPU();
			return TRUE;
		case IDC_DEBUG_REG_PC:
			if (NES::Running)
				break;
			GetDlgItemText(hwndDlg, IDC_DEBUG_REG_PC, tpc, 5);
			CPU::PC = _tcstol(tpc, NULL, 16);
			if (wmEvent == EN_KILLFOCUS)
				UpdateCPU();
			return TRUE;

		case IDC_DEBUG_IRQ_EXT:
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_IRQ_EXT) == BST_CHECKED)
				CPU::WantIRQ |= IRQ_EXTERNAL;
			else	CPU::WantIRQ &= ~IRQ_EXTERNAL;
			return TRUE;
		case IDC_DEBUG_IRQ_PCM:
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_IRQ_PCM) == BST_CHECKED)
				CPU::WantIRQ |= IRQ_DPCM;
			else	CPU::WantIRQ &= ~IRQ_DPCM;
			return TRUE;
		case IDC_DEBUG_IRQ_FRAME:
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_IRQ_FRAME) == BST_CHECKED)
				CPU::WantIRQ |= IRQ_FRAME;
			else	CPU::WantIRQ &= ~IRQ_FRAME;
			return TRUE;
		case IDC_DEBUG_IRQ_DEBUG:
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_IRQ_DEBUG) == BST_CHECKED)
				CPU::WantIRQ |= IRQ_DEBUG;
			else	CPU::WantIRQ &= ~IRQ_DEBUG;
			return TRUE;

			// or modify flags while running
		case IDC_DEBUG_FLAG_N:
			if (NES::Running)
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_FLAG_N) == BST_CHECKED)
				CPU::FN = 1;
			else	CPU::FN = 0;
			CPU::JoinFlags();
			UpdateCPU();
			return TRUE;
		case IDC_DEBUG_FLAG_V:
			if (NES::Running)
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_FLAG_V) == BST_CHECKED)
				CPU::FV = 1;
			else	CPU::FV = 0;
			CPU::JoinFlags();
			UpdateCPU();
			return TRUE;
		case IDC_DEBUG_FLAG_D:
			if (NES::Running)
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_FLAG_D) == BST_CHECKED)
				CPU::FD = 1;
			else	CPU::FD = 0;
			CPU::JoinFlags();
			UpdateCPU();
			return TRUE;
		case IDC_DEBUG_FLAG_I:
			if (NES::Running)
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_FLAG_I) == BST_CHECKED)
				CPU::FI = 1;
			else	CPU::FI = 0;
			CPU::JoinFlags();
			UpdateCPU();
			return TRUE;
		case IDC_DEBUG_FLAG_Z:
			if (NES::Running)
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_FLAG_Z) == BST_CHECKED)
				CPU::FZ = 1;
			else	CPU::FZ = 0;
			CPU::JoinFlags();
			UpdateCPU();
			return TRUE;
		case IDC_DEBUG_FLAG_C:
			if (NES::Running)
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_DEBUG_FLAG_C) == BST_CHECKED)
				CPU::FC = 1;
			else	CPU::FC = 0;
			CPU::JoinFlags();
			UpdateCPU();
			return TRUE;

		case IDC_DEBUG_BREAK_LIST:
			if (wmEvent == LBN_DBLCLK)
			{
				// get selected breakpoint
				bp = GetBreakpoint(hwndDlg, line);
				if (bp == NULL)
					break;

				// toggle it
				bp->enabled = !bp->enabled;
				bp->updateDescription();

				// update its description and reselect it
				SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_DELETESTRING, line, 0);
				SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_INSERTSTRING, line, (LPARAM)bp->desc);
				SetBreakpoint(hwndDlg, line);

				// finally, update cache
				CacheBreakpoints();
			}
			else	SetBreakpoint(hwndDlg, -2);	// enable buttons according to current selection
			return TRUE;
		case IDC_DEBUG_BREAK_UP:
			// get selected breakpoint
			bp = GetBreakpoint(hwndDlg, line);
			if (bp == NULL)
				break;

			// if we're already at the top, there's nowhere to go
			if (line == 0)
				break;

			// Move the line up in the list
			SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_DELETESTRING, line, 0);
			SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_INSERTSTRING, line - 1, (LPARAM)bp->desc);

			// Then move the actual breakpoint up
			std::iter_swap(Breakpoints.begin() + line, Breakpoints.begin() + (line - 1));

			// And reselect it
			SetBreakpoint(hwndDlg, line - 1);
			return TRUE;
		case IDC_DEBUG_BREAK_DOWN:
			// get selected breakpoint
			bp = GetBreakpoint(hwndDlg, line);
			if (bp == NULL)
				break;

			// if we're already at the bottom, there's nowhere to go
			if (line == Breakpoints.size() - 1)
				break;

			// Move the line down in the list
			SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_DELETESTRING, line, 0);
			SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_INSERTSTRING, line + 1, (LPARAM)bp->desc);

			// Then move the actual breakpoint down
			std::iter_swap(Breakpoints.begin() + line, Breakpoints.begin() + (line + 1));

			// And reselect it
			SetBreakpoint(hwndDlg, line + 1);
			return TRUE;
		case IDC_DEBUG_BREAK_ADD:
			bp = (struct tBreakpoint *)DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_BREAKPOINT), hwndDlg, BreakpointProc, (LPARAM)NULL);
			// if user cancels, nothing was added
			if (bp == NULL)
				break;
			// add it to the breakpoint listbox
			line = SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_ADDSTRING, 0, (LPARAM)bp->desc);
			// select it
			SetBreakpoint(hwndDlg, line);
			// then recache the breakpoints
			CacheBreakpoints();
			return TRUE;
		case IDC_DEBUG_BREAK_EDIT:
			// get selected breakpoint
			bp = GetBreakpoint(hwndDlg, line);
			if (bp == NULL)
				break;

			// then open the editor on it
			bp = (struct tBreakpoint *)DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_BREAKPOINT), hwndDlg, BreakpointProc, (LPARAM)bp);
			// if user cancels, nothing was changed
			if (bp == NULL)
				break;
			// update breakpoint description
			SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_DELETESTRING, line, 0);
			SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_INSERTSTRING, line, (LPARAM)bp->desc);
			// reselect it
			SetBreakpoint(hwndDlg, line);
			// then recache the breakpoints
			CacheBreakpoints();
			return TRUE;
		case IDC_DEBUG_BREAK_DELETE:
			// get selected breakpoint
			bp = GetBreakpoint(hwndDlg, line);
			if (bp == NULL)
				break;

			// and take it out of the list
			Breakpoints.erase(Breakpoints.begin() + line);
			SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_DELETESTRING, line, 0);

			// and select the next entry in the list
			SetBreakpoint(hwndDlg, line);
			// then recache the breakpoints
			CacheBreakpoints();
			return TRUE;

		case IDC_DEBUG_CONT_RUN:
			if (NES::ROMLoaded)
				SendMessage(hMainWnd, WM_COMMAND, ID_CPU_RUN, 0);
			return TRUE;
		case IDC_DEBUG_CONT_STEP:
			if (NES::ROMLoaded)
				SendMessage(hMainWnd, WM_COMMAND, ID_CPU_STEP, 0);
			return TRUE;
		case IDC_DEBUG_CONT_RESET:
			if (NES::ROMLoaded)
				SendMessage(hMainWnd, WM_COMMAND, ID_CPU_SOFTRESET, 0);
			return TRUE;
		case IDC_DEBUG_CONT_POWER:
			if (NES::ROMLoaded)
				SendMessage(hMainWnd, WM_COMMAND, ID_CPU_HARDRESET, 0);
			return TRUE;
		case IDC_DEBUG_CONT_SEEKPC:
			TraceOffset = -1;
			UpdateCPU();
			return TRUE;
		case IDC_DEBUG_CONT_SEEKTO:
			GetDlgItemText(hwndDlg, IDC_DEBUG_CONT_SEEKADDR, tpc, 5);
			TraceOffset = _tcstol(tpc, NULL, 16);
			UpdateCPU();
			return TRUE;
		case IDC_DEBUG_CONT_SEEKSEL:
			line = SendDlgItemMessage(hwndDlg, IDC_DEBUG_TRACE_LIST, LB_GETCURSEL, 0, 0);
			if (line == -1)
				break;
			len = SendDlgItemMessage(hwndDlg, IDC_DEBUG_TRACE_LIST, LB_GETTEXTLEN, line, 0);
			str = new TCHAR[len + 1];
			SendDlgItemMessage(hwndDlg, IDC_DEBUG_TRACE_LIST, LB_GETTEXT, line, (LPARAM)str);
			Addr = _tcstol(str, NULL, 16);
			_stprintf(tpc, _T("%04X"), Addr);
			SetDlgItemText(hwndDlg, IDC_DEBUG_CONT_SEEKADDR, tpc);
			delete[] str;
			return TRUE;
		case IDC_DEBUG_CONT_SEEKADDR:
			if (TraceOffset != -1)
			{
				GetDlgItemText(hwndDlg, IDC_DEBUG_CONT_SEEKADDR, tpc, 5);
				TraceOffset = _tcstol(tpc, NULL, 16);
				UpdateCPU();
				return TRUE;
			}
			break;
		case IDC_DEBUG_CONT_DUMPCPU:
			DumpCPU();
			return TRUE;
		case IDC_DEBUG_CONT_DUMPPPU:
			DumpPPU();
			return TRUE;
		case IDC_DEBUG_CONT_STARTLOG:
			StartLogging();
			return TRUE;
		case IDC_DEBUG_CONT_STOPLOG:
			StopLogging();
			return TRUE;

		case IDC_DEBUG_MEM_CPU:
		case IDC_DEBUG_MEM_PPU:
		case IDC_DEBUG_MEM_SPR:
		case IDC_DEBUG_MEM_PAL:
			// all of these should just jump to address 0 and redraw
			MemOffset = 0;
			UpdateCPU();
			return TRUE;

		case IDC_DEBUG_TRACE_LIST:
			if (wmEvent == LBN_DBLCLK)
			{
				bp = (struct tBreakpoint *)DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_BREAKPOINT), hwndDlg, BreakpointProc, (LPARAM)0xFFFFFFFF);
				// if user cancels, nothing was added
				if (bp == NULL)
					break;
				// add it to the breakpoint listbox
				line = SendDlgItemMessage(hwndDlg, IDC_DEBUG_BREAK_LIST, LB_ADDSTRING, 0, (LPARAM)bp->desc);
				// select it
				SetBreakpoint(hwndDlg, line);
				// then recache the breakpoints
				CacheBreakpoints();
				return TRUE;
			}
			break;

		case IDCANCEL:
			SetMode(Mode & ~DEBUG_MODE_CPU);
			return TRUE;
		}
		break;
	case WM_VSCROLL:
		// if UpdateCPU() is running, don't listen to any notifications
		if (inUpdate)
			break;

		if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_DEBUG_TRACE_SCROLL))
		{
			sinfo.cbSize = sizeof(SCROLLINFO);
			sinfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
			GetScrollInfo((HWND)lParam, SB_CTL, &sinfo);
			if (TraceOffset == -1)
				TraceOffset = CPU::PC;
			switch (LOWORD(wParam))
			{
			case SB_LINEUP:
				TraceOffset--;
				break;
			case SB_LINEDOWN:
				TraceOffset++;
				break;
			case SB_PAGEUP:
				TraceOffset -= sinfo.nPage;
				break;
			case SB_PAGEDOWN:
				TraceOffset += sinfo.nPage;
				break;
			case SB_THUMBPOSITION:
				TraceOffset = HIWORD(wParam);
				break;
			default:
				return FALSE;
			}
			if (TraceOffset < sinfo.nMin)
				TraceOffset = sinfo.nMin;
			if (TraceOffset > sinfo.nMax)
				TraceOffset = sinfo.nMax;
			_stprintf(tpc, _T("%04X"), TraceOffset);
			SetDlgItemText(hwndDlg, IDC_DEBUG_CONT_SEEKADDR, tpc);
			CheckRadioButton(hwndDlg, IDC_DEBUG_CONT_SEEKPC, IDC_DEBUG_CONT_SEEKTO, IDC_DEBUG_CONT_SEEKTO);
			UpdateCPU();
			return TRUE;
		}
		if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_DEBUG_MEM_SCROLL))
		{
			sinfo.cbSize = sizeof(SCROLLINFO);
			sinfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
			GetScrollInfo((HWND)lParam, SB_CTL, &sinfo);
			switch (LOWORD(wParam))
			{
			case SB_LINEUP:
				MemOffset--;
				break;
			case SB_LINEDOWN:
				MemOffset++;
				break;
			case SB_PAGEUP:
				MemOffset -= sinfo.nPage;
				break;
			case SB_PAGEDOWN:
				MemOffset += sinfo.nPage;
				break;
			case SB_THUMBPOSITION:
				MemOffset = HIWORD(wParam);
				break;
			default:
				return FALSE;
			}
			if (MemOffset < sinfo.nMin)
				MemOffset = sinfo.nMin;
			if (MemOffset > sinfo.nMax)
				MemOffset = sinfo.nMax;
			UpdateCPU();
			return TRUE;
		}
		break;
	}
	return FALSE;
}

LRESULT CALLBACK PPUProc_Nametable (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT point;
	if (uMsg == WM_MOUSEMOVE)
	{
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		if ((point.x >= 0) && (point.x < D_NAM_W) && (point.y >= 0) && (point.y < D_NAM_H))
			SetDetail(DEBUG_DETAIL_NAMETABLE, ((point.y << 2) & 0x3E0) | ((point.x >> 3) & 0x1F) | (Nametable << 10));
		else	SetDetail(DetailTypeSave, DetailNumSave);
		return 0;
	}
	if (uMsg == WM_RBUTTONDOWN)
	{
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		if ((point.x >= 0) && (point.x < D_NAM_W) && (point.y >= 0) && (point.y < D_NAM_H))
		{
			DetailTypeSave = DetailType;
			DetailNumSave = DetailNum;
		}
		else
		{
			DetailTypeSave = DEBUG_DETAIL_NONE;
			DetailNumSave = 0;
			SetDetail(DetailTypeSave, DetailNumSave);
		}
		return 0;
	}
	return CallWindowProc((WNDPROC)GetWindowLongPtr(hWnd, GWLP_USERDATA), hWnd, uMsg, wParam, lParam);
}
LRESULT CALLBACK PPUProc_Pattern (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT point;
	if (uMsg == WM_MOUSEMOVE)
	{
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		if ((point.x >= 0) && (point.x < D_PAT_W) && (point.y >= 0) && (point.y < D_PAT_H))
			SetDetail(DEBUG_DETAIL_PATTERN, CheckFlip(((point.x >> 3) & 0xF) | ((point.y << 1) & 0xF0) | ((point.x & 0x80) << 1)));
		else	SetDetail(DetailTypeSave, DetailNumSave);
		return 0;
	}
	if (uMsg == WM_RBUTTONDOWN)
	{
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		if ((point.x >= 0) && (point.x < D_PAT_W) && (point.y >= 0) && (point.y < D_PAT_H))
		{
			DetailTypeSave = DetailType;
			DetailNumSave = DetailNum;
		}
		else
		{
			DetailTypeSave = DEBUG_DETAIL_NONE;
			DetailNumSave = 0;
			SetDetail(DetailTypeSave, DetailNumSave);
		}
		return 0;
	}
	return CallWindowProc((WNDPROC)GetWindowLongPtr(hWnd, GWLP_USERDATA), hWnd, uMsg, wParam, lParam);
}
LRESULT CALLBACK PPUProc_Palette (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT point;
	if (uMsg == WM_MOUSEMOVE)
	{
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		if ((point.x >= 0) && (point.x < D_PAL_W) && (point.y >= 0) && (point.y < D_PAL_H))
			SetDetail(DEBUG_DETAIL_PALETTE, (point.y & 0x30) | (point.x >> 4));
		else	SetDetail(DetailTypeSave, DetailNumSave);
		return 0;
	}
	if (uMsg == WM_RBUTTONDOWN)
	{
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		if ((point.x >= 0) && (point.x < D_PAL_W) && (point.y >= 0) && (point.y < D_PAL_H))
		{
			DetailTypeSave = DetailType;
			DetailNumSave = DetailNum;
		}
		else
		{
			DetailTypeSave = DEBUG_DETAIL_NONE;
			DetailNumSave = 0;
			SetDetail(DetailTypeSave, DetailNumSave);
		}
		return 0;
	}
	return CallWindowProc((WNDPROC)GetWindowLongPtr(hWnd, GWLP_USERDATA), hWnd, uMsg, wParam, lParam);
}
LRESULT CALLBACK PPUProc_Sprite (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT point;
	if (uMsg == WM_MOUSEMOVE)
	{
		int height = (PPU::Reg2000 & 0x20) ? 16 : 8;
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		if ((point.x >= 0) && (point.x < D_SPR_W) && (point.y >= 0) && (point.y < D_SPR_H) && ((point.x % 16) < 8) && ((point.y % 24) < height))
			SetDetail(DEBUG_DETAIL_SPRITE, ((point.y / 24) << 4) | (point.x >> 4));
		else	SetDetail(DetailTypeSave, DetailNumSave);
		return 0;
	}
	if (uMsg == WM_RBUTTONDOWN)
	{
		int height = (PPU::Reg2000 & 0x20) ? 16 : 8;
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		if ((point.x >= 0) && (point.x < D_SPR_W) && (point.y >= 0) && (point.y < D_SPR_H) && ((point.x % 16) < 8) && ((point.y % 24) < height))
		{
			DetailTypeSave = DetailType;
			DetailNumSave = DetailNum;
		}
		else
		{
			DetailTypeSave = DEBUG_DETAIL_NONE;
			DetailNumSave = 0;
			SetDetail(DetailTypeSave, DetailNumSave);
		}
		return 0;
	}
	return CallWindowProc((WNDPROC)GetWindowLongPtr(hWnd, GWLP_USERDATA), hWnd, uMsg, wParam, lParam);
}

INT_PTR CALLBACK PPUProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LPDRAWITEMSTRUCT lpDrawItem;
	static const int dbgRadio[4] = { IDC_DEBUG_PPU_NT0, IDC_DEBUG_PPU_NT1, IDC_DEBUG_PPU_NT2, IDC_DEBUG_PPU_NT3 };
	HWND dlgItem;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		CheckRadioButton(hwndDlg, IDC_DEBUG_PPU_NT0, IDC_DEBUG_PPU_NT3, dbgRadio[Nametable]);
		CheckDlgButton(hwndDlg, IDC_DEBUG_PPU_TILEFLIP, TileFlip ? BST_CHECKED : BST_UNCHECKED);
		NTabChanged = PalChanged = PatChanged = SprChanged = DetChanged = TRUE;

		dlgItem = GetDlgItem(hwndDlg, IDC_DEBUG_PPU_NAMETABLE);
		SetWindowLongPtr(dlgItem, GWLP_USERDATA, SetWindowLongPtr(dlgItem, GWLP_WNDPROC, (LONG_PTR)PPUProc_Nametable));

		dlgItem = GetDlgItem(hwndDlg, IDC_DEBUG_PPU_PATTERN);
		SetWindowLongPtr(dlgItem, GWLP_USERDATA, SetWindowLongPtr(dlgItem, GWLP_WNDPROC, (LONG_PTR)PPUProc_Pattern));

		dlgItem = GetDlgItem(hwndDlg, IDC_DEBUG_PPU_PALETTE);
		SetWindowLongPtr(dlgItem, GWLP_USERDATA, SetWindowLongPtr(dlgItem, GWLP_WNDPROC, (LONG_PTR)PPUProc_Palette));

		dlgItem = GetDlgItem(hwndDlg, IDC_DEBUG_PPU_SPRITE);
		SetWindowLongPtr(dlgItem, GWLP_USERDATA, SetWindowLongPtr(dlgItem, GWLP_WNDPROC, (LONG_PTR)PPUProc_Sprite));
		return FALSE;
	case WM_DRAWITEM:
		lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
		switch (lpDrawItem->CtlID)
		{
		case IDC_DEBUG_PPU_NAMETABLE:
			BitBlt(lpDrawItem->hDC, 0, 0, D_NAM_W, D_NAM_H, NameDC, 0, 0, SRCCOPY);
			return TRUE;
		case IDC_DEBUG_PPU_PATTERN:
			BitBlt(lpDrawItem->hDC, 0, 0, D_PAT_W, D_PAT_H, PatternDC, 0, 0, SRCCOPY);
			return TRUE;
		case IDC_DEBUG_PPU_PALETTE:
			BitBlt(lpDrawItem->hDC, 0, 0, D_PAL_W, D_PAL_H, PaletteDC, 0, 0, SRCCOPY);
			return TRUE;
		case IDC_DEBUG_PPU_SPRITE:
			BitBlt(lpDrawItem->hDC, 0, 0, D_SPR_W, D_SPR_H, SpriteDC, 0, 0, SRCCOPY);
			return TRUE;
		case IDC_DEBUG_PPU_TILE:
			BitBlt(lpDrawItem->hDC, 0, 0, D_TIL_W, D_TIL_H, TileDC, 0, 0, SRCCOPY);
			return TRUE;
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 

		switch (wmId)
		{
		case IDC_DEBUG_PPU_NAMETABLE:
			Nametable = (Nametable + 1) & 3;
			CheckRadioButton(hwndDlg, IDC_DEBUG_PPU_NT0, IDC_DEBUG_PPU_NT3, dbgRadio[Nametable]);
			NTabChanged = TRUE;
			UpdatePPU();
			return TRUE;
		case IDC_DEBUG_PPU_NT0:
			Nametable = 0;
			NTabChanged = TRUE;
			UpdatePPU();
			return TRUE;
		case IDC_DEBUG_PPU_NT1:
			Nametable = 1;
			NTabChanged = TRUE;
			UpdatePPU();
			return TRUE;
		case IDC_DEBUG_PPU_NT2:
			Nametable = 2;
			NTabChanged = TRUE;
			UpdatePPU();
			return TRUE;
		case IDC_DEBUG_PPU_NT3:
			Nametable = 3;
			NTabChanged = TRUE;
			UpdatePPU();
			return TRUE;
		case IDC_DEBUG_PPU_PATTERN:
			Palette = (Palette + 1) & 7;
			PatChanged = TRUE;
			UpdatePPU();
			return TRUE;
		case IDC_DEBUG_PPU_TILEFLIP:
			TileFlip = IsDlgButtonChecked(hwndDlg, IDC_DEBUG_PPU_TILEFLIP);
			PatChanged = TRUE;
			UpdatePPU();
			return TRUE;
		case IDCANCEL:
			SetMode(Mode & ~DEBUG_MODE_PPU);
			return TRUE;
		}
		break;
	case WM_MOUSEMOVE:
		SetDetail(DetailTypeSave, DetailNumSave);
		return TRUE;
	case WM_RBUTTONDOWN:
		DetailTypeSave = DEBUG_DETAIL_NONE;
		DetailNumSave = 0;
		SetDetail(DetailTypeSave, DetailNumSave);
		return TRUE;
	}
	return FALSE;
}
} // namespace Debugger
#endif	/* ENABLE_DEBUGGER */
