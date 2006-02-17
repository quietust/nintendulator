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
#include "APU.h"
#include "Debugger.h"
#include "CPU.h"
#include "PPU.h"
#include "GFX.h"
#include "Genie.h"

#ifdef ENABLE_DEBUGGER

struct tDebugger Debugger;

LRESULT CALLBACK PaletteProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK PatternProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NameProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DumpProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK TraceProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK RegProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

enum ADDRMODE { IMP, ACC, IMM, ADR, ABS, IND, REL, ABX, ABY, ZPG, ZPX, ZPY, INX, INY, ERR, NUM_ADDR_MODES };

enum ADDRMODE TraceAddyMode[256] =
{
	IMM,INX,ERR,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,ACC,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPX,ZPX,IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	ADR,INX,ERR,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,ACC,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPX,ZPX,IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	IMP,INX,ERR,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,ACC,IMM,ADR,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPX,ZPX,IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	IMP,INX,ERR,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,ACC,IMM,IND,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPX,ZPX,IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	IMM,INX,IMM,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,IMP,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPY,ZPY,IMP,ABY,IMP,ABY,ABX,ABX,ABY,ABY,
	IMM,INX,IMM,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,IMP,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPY,ZPY,IMP,ABY,IMP,ABY,ABX,ABX,ABY,ABY,
	IMM,INX,IMM,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,IMP,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPX,ZPX,IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	IMM,INX,IMM,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,IMP,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPX,ZPX,IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX
};

unsigned char AddrBytes[NUM_ADDR_MODES] = {1,1,2,3,3,3,2,3,3,2,2,2,2,2,1};

char TraceArr[256][5] =
{
	" BRK"," ORA","*HLT","*SLO","*NOP"," ORA"," ASL","*SLO"," PHP"," ORA"," ASL"," ???","*NOP"," ORA"," ASL","*SLO",
	" BPL"," ORA","*HLT","*SLO","*NOP"," ORA"," ASL","*SLO"," CLC"," ORA","*NOP","*SLO","*NOP"," ORA"," ASL","*SLO",
	" JSR"," AND","*HLT","*RLA"," BIT"," AND"," ROL","*RLA"," PLP"," AND"," ROL"," ???"," BIT"," AND"," ROL","*RLA",
	" BMI"," AND","*HLT","*RLA","*NOP"," AND"," ROL","*RLA"," SEC"," AND","*NOP","*RLA","*NOP"," AND"," ROL","*RLA",
	" RTI"," EOR","*HLT","*SRE","*NOP"," EOR"," LSR","*SRE"," PHA"," EOR"," LSR"," ???"," JMP"," EOR"," LSR","*SRE",
	" BVC"," EOR","*HLT","*SRE","*NOP"," EOR"," LSR","*SRE"," CLI"," EOR","*NOP","*SRE","*NOP"," EOR"," LSR","*SRE",
	" RTS"," ADC","*HLT","*RRA","*NOP"," ADC"," ROR","*RRA"," PLA"," ADC"," ROR"," ???"," JMP"," ADC"," ROR","*RRA",
	" BVS"," ADC","*HLT","*RRA","*NOP"," ADC"," ROR","*RRA"," SEI"," ADC","*NOP","*RRA","*NOP"," ADC"," ROR","*RRA",
	"*NOP"," STA","*NOP","*SAX"," STY"," STA"," STX","*SAX"," DEY","*NOP"," TXA"," ???"," STY"," STA"," STX","*SAX",
	" BCC"," STA","*HLT"," ???"," STY"," STA"," STX","*SAX"," TYA"," STA"," TXS"," ???"," ???"," STA"," ???"," ???",
	" LDY"," LDA"," LDX","*LAX"," LDY"," LDA"," LDX","*LAX"," TAY"," LDA"," TAX"," ???"," LDY"," LDA"," LDX","*LAX",
	" BCS"," LDA","*HLT","*LAX"," LDY"," LDA"," LDX","*LAX"," CLV"," LDA"," TSX"," ???"," LDY"," LDA"," LDX","*LAX",
	" CPY"," CMP","*NOP","*DCP"," CPY"," CMP"," DEC","*DCP"," INY"," CMP"," DEX"," ???"," CPY"," CMP"," DEC","*DCP",
	" BNE"," CMP","*HLT","*DCP","*NOP"," CMP"," DEC","*DCP"," CLD"," CMP","*NOP","*DCP","*NOP"," CMP"," DEC","*DCP",
	" CPX"," SBC","*NOP","*ISB"," CPX"," SBC"," INC","*ISB"," INX"," SBC"," NOP","*SBC"," CPX"," SBC"," INC","*ISB",
	" BEQ"," SBC","*HLT","*ISB","*NOP"," SBC"," INC","*ISB"," SED"," SBC","*NOP","*ISB","*NOP"," SBC"," INC","*ISB"
};


unsigned char VMemory (unsigned long Addy) { return PPU.CHRPointer[(Addy & 0x1C00) >> 10][Addy & 0x03FF]; }

enum {
	D_PAL_W = 256,
	D_PAL_H = 32,
	D_PAT_W = 256,
	D_PAT_H = 128,
	D_NAM_W = 512,
	D_NAM_H = 480,
	D_REG_W = 224,
	D_REG_H = 184,
	D_TRC_W = 256,
	D_TRC_H = 256
};

void	Debugger_Init (void)
{
	HFONT tpf;
	HDC TpHDC = GetDC(mWnd);
	int nHeight;

	ZeroMemory(Debugger.BreakP,sizeof(Debugger.BreakP));
	Debugger.TraceOffset = -1;
	
	Debugger.PatPalBase = 0;

	Debugger.Logging = FALSE;

	Debugger.FontWidth = 7;
	Debugger.FontHeight = 10;
	
	Debugger.Depth = GetDeviceCaps(TpHDC,BITSPIXEL);
	ReleaseDC(GetDesktopWindow(), TpHDC);

	TpHDC = GetWindowDC(GetDesktopWindow());
	
	Debugger.PaletteDC = CreateCompatibleDC(TpHDC);
	Debugger.PaletteBMP = CreateCompatibleBitmap(TpHDC,D_PAL_W,D_PAL_H);
	SelectObject(Debugger.PaletteDC,Debugger.PaletteBMP);
	BitBlt(Debugger.PaletteDC,0,0,D_PAL_W,D_PAL_H,NULL,0,0,BLACKNESS);
	ZeroMemory(Debugger.PaletteArray,sizeof(Debugger.PaletteArray));

	Debugger.PatternDC = CreateCompatibleDC(TpHDC);
	Debugger.PatternBMP = CreateCompatibleBitmap(TpHDC,D_PAT_W,D_PAT_H);
	SelectObject(Debugger.PatternDC,Debugger.PatternBMP);
	BitBlt(Debugger.PatternDC,0,0,D_PAT_W,D_PAT_H,NULL,0,0,BLACKNESS);
	ZeroMemory(Debugger.PatternArray,sizeof(Debugger.PatternArray));

	Debugger.NameDC = CreateCompatibleDC(TpHDC);
	Debugger.NameBMP = CreateCompatibleBitmap(TpHDC,D_NAM_W,D_NAM_H);
	SelectObject(Debugger.NameDC,Debugger.NameBMP);
	BitBlt(Debugger.NameDC,0,0,D_NAM_W,D_NAM_H,NULL,0,0,BLACKNESS);
	ZeroMemory(Debugger.NameArray,sizeof(Debugger.NameArray));

	Debugger.TraceDC = CreateCompatibleDC(TpHDC);
	Debugger.TraceBMP = CreateCompatibleBitmap(TpHDC,D_TRC_W,D_TRC_H);
	SelectObject(Debugger.TraceDC,Debugger.TraceBMP);
	BitBlt(Debugger.TraceDC,0,0,D_TRC_W,D_TRC_H,NULL,0,0,BLACKNESS);

	Debugger.RegDC = CreateCompatibleDC(TpHDC);
	Debugger.RegBMP = CreateCompatibleBitmap(TpHDC,D_REG_W,D_REG_H);
	SelectObject(Debugger.RegDC,Debugger.RegBMP);
	BitBlt(Debugger.RegDC,0,0,D_REG_W,D_REG_H,NULL,0,0,BLACKNESS);

	nHeight = -MulDiv(Debugger.FontHeight, GetDeviceCaps(TpHDC, LOGPIXELSY), 72);
	tpf = CreateFont(nHeight, Debugger.FontWidth, 0, 0, 0, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, FF_MODERN, "Courier New");
	SelectObject(Debugger.RegDC, tpf);
	SelectObject(Debugger.TraceDC, tpf);
	DeleteObject(tpf);
	SetBkMode(Debugger.RegDC,TRANSPARENT);
	SetBkMode(Debugger.TraceDC,TRANSPARENT);

	ReleaseDC(mWnd,TpHDC);
	Debugger.Step = FALSE;
}

void	Debugger_Release (void)
{
}

void	DPrint (char *text)
{
	if (Debugger.Logging)
		fwrite(text,1,strlen(text),Debugger.LogFile);
}

void	Debugger_SetMode (int NewMode)
{
	RECT rect;

	Debugger.Mode = NewMode;
	Debugger.Enabled = (Debugger.Mode > 0) ? TRUE : FALSE;

	Debugger.NTabChanged = TRUE;
	Debugger.PalChanged = TRUE;
	Debugger.PatChanged = TRUE;
	
	if ((Debugger.Enabled) && (SizeMult == 1))	/* window positions will overlap if at 1X size */
	{
		SizeMult = 2;
		CheckMenuRadioItem(GetMenu(mWnd),ID_PPU_SIZE_1X,ID_PPU_SIZE_4X,ID_PPU_SIZE_2X,MF_BYCOMMAND);
		SetWindowClientArea(mWnd,512,480);
	}

	CPU.WantIRQ &= ~IRQ_DEBUG;
	if (Debugger.DumpWnd)
	{
		DestroyWindow(Debugger.DumpWnd);
		Debugger.DumpWnd = NULL;
	}
	if (Debugger.PaletteWnd)
	{
		DestroyWindow(Debugger.PaletteWnd);
		Debugger.PaletteWnd = NULL;
	}
	if (Debugger.PatternWnd)
	{
		DestroyWindow(Debugger.PatternWnd);
		Debugger.PatternWnd = NULL;
	}
	if (Debugger.NameWnd)
	{
		DestroyWindow(Debugger.NameWnd);
		Debugger.NameWnd = NULL;
	}
	if (Debugger.TraceWnd)
	{
		DestroyWindow(Debugger.TraceWnd);
		Debugger.TraceWnd = NULL;
	}
	if (Debugger.RegWnd)
	{
		DestroyWindow(Debugger.RegWnd);
		Debugger.RegWnd = NULL;
	}

	if (Debugger.Mode >= 1)
	{
		Debugger.PatternWnd = CreateDialog(hInst, (LPCTSTR) IDD_DEBUGGER_PATTERN, mWnd, PatternProc);
		GetWindowRect(mWnd,&rect);
		SetWindowPos(Debugger.PatternWnd,HWND_TOP,rect.left,rect.bottom,0,0,SWP_SHOWWINDOW);
		SetWindowClientArea(Debugger.PatternWnd,D_PAT_W,D_PAT_H);

		Debugger.PaletteWnd = CreateDialog(hInst, (LPCTSTR) IDD_DEBUGGER_PALETTE, mWnd, PaletteProc);
		GetWindowRect(Debugger.PatternWnd,&rect);
		SetWindowPos(Debugger.PaletteWnd,HWND_TOP,rect.left,rect.bottom,0,0,SWP_SHOWWINDOW);
		SetWindowClientArea(Debugger.PaletteWnd,D_PAL_W,D_PAL_H);

		Debugger.DumpWnd = CreateDialog(hInst, (LPCTSTR) IDD_DEBUGGER_DUMPS, mWnd, DumpProc);
		GetWindowRect(Debugger.PatternWnd,&rect);
		SetWindowPos(Debugger.DumpWnd,HWND_TOP,rect.right,rect.top,0,0,SWP_SHOWWINDOW | SWP_NOSIZE);
	}
	if (Debugger.Mode >= 2)
	{
		Debugger.NameWnd = CreateDialog(hInst, (LPCTSTR) IDD_DEBUGGER_NAME, mWnd, NameProc);
		GetWindowRect(mWnd,&rect);
		SetWindowPos(Debugger.NameWnd,HWND_TOP,rect.right,rect.top,0,0,SWP_SHOWWINDOW);
		SetWindowClientArea(Debugger.NameWnd,D_NAM_W,D_NAM_H);
	}
	if (Debugger.Mode >= 3)
	{
		Debugger.RegWnd = CreateDialog(hInst, (LPCTSTR) IDD_DEBUGGER_REGISTERS, mWnd, RegProc);
		GetWindowRect(Debugger.NameWnd,&rect);
		SetWindowPos(Debugger.RegWnd,HWND_TOP,rect.left,rect.bottom,0,0,SWP_SHOWWINDOW);
		SetWindowClientArea(Debugger.RegWnd,D_REG_W,D_REG_H);

		Debugger.TraceWnd = CreateDialog(hInst, (LPCTSTR) IDD_DEBUGGER_TRACE, mWnd, TraceProc);
		GetWindowRect(Debugger.RegWnd,&rect);
		SetWindowPos(Debugger.TraceWnd,HWND_TOP,rect.right,rect.top,0,0,SWP_SHOWWINDOW);
		SetWindowClientArea(Debugger.TraceWnd,D_TRC_W,D_TRC_H);

		Debugger_Update();
	}

	SetFocus(mWnd);
}

void	Debugger_StartLogging (void)
{
	Debugger.Logging = TRUE;
	Debugger.LogFile = fopen("debug.txt","a+");
}

void	Debugger_StopLogging (void)
{
	fclose(Debugger.LogFile);
	Debugger.Logging = FALSE;
}

unsigned char TraceMem (unsigned long Addr)
{
	int Bank = (Addr >> 12) & 0xF;
	FCPURead Read = CPU.ReadHandler[Bank];
	if ((Read == CPU_ReadRAM) || (Read == CPU_ReadPRG) || (Read == GenieRead) || (Read == GenieRead1) || (Read == GenieRead2) || (Read == GenieRead3))
		return Read(Bank, Addr & 0xFFF);
	else	return 0xFF;
}

void	DecodeInstruction (unsigned short Addy, char *str)
{
	unsigned char OpData[3] = {TraceMem(Addy),0,0};
	unsigned short Operand = 0, MidAddy = 0, EffectiveAddy = 0;
	switch (TraceAddyMode[OpData[0]])
	{
	case IND:	OpData[1] = TraceMem(Addy+1);	OpData[2] = TraceMem(Addy+2);	Operand = OpData[1] | (OpData[2] << 8);	EffectiveAddy = TraceMem(Operand) | (TraceMem(Operand+1) << 8);		break;
	case ADR:	OpData[1] = TraceMem(Addy+1);	OpData[2] = TraceMem(Addy+2);	Operand = OpData[1] | (OpData[2] << 8);										break;
	case ABS:	OpData[1] = TraceMem(Addy+1);	OpData[2] = TraceMem(Addy+2);	Operand = OpData[1] | (OpData[2] << 8);										break;
	case ABX:	OpData[1] = TraceMem(Addy+1);	OpData[2] = TraceMem(Addy+2);	Operand = OpData[1] | (OpData[2] << 8);	EffectiveAddy = Operand + CPU.X;					break;
	case ABY:	OpData[1] = TraceMem(Addy+1);	OpData[2] = TraceMem(Addy+2);	Operand = OpData[1] | (OpData[2] << 8);	EffectiveAddy = Operand + CPU.Y;					break;
	case IMM:	OpData[1] = TraceMem(Addy+1);	Operand = OpData[1];																break;
	case ZPG:	OpData[1] = TraceMem(Addy+1);	Operand = OpData[1];																break;
	case ZPX:	OpData[1] = TraceMem(Addy+1);	Operand = OpData[1];	EffectiveAddy = (Operand + CPU.X) & 0xFF;										break;
	case ZPY:	OpData[1] = TraceMem(Addy+1);	Operand = OpData[1];	EffectiveAddy = (Operand + CPU.Y) & 0xFF;										break;
	case INX:	OpData[1] = TraceMem(Addy+1);	Operand = OpData[1];	MidAddy = (Operand + CPU.X) & 0xFF;	EffectiveAddy = TraceMem(MidAddy) | (TraceMem((MidAddy+1) & 0xFF) << 8);	break;
	case INY:	OpData[1] = TraceMem(Addy+1);	Operand = OpData[1];	MidAddy = TraceMem(Operand) | (TraceMem((Operand+1) & 0xFF) << 8);	EffectiveAddy = MidAddy + CPU.Y;		break;
	case IMP:																							break;
	case ACC:																							break;
	case ERR:																							break;
	case REL:	OpData[1] = TraceMem(Addy+1);	Operand = Addy+2+(signed char)OpData[1];													break;
	}

	switch (TraceAddyMode[TraceMem(Addy)])
	{
	case IMP:
	case ERR:sprintf(str,"%04X  %02X       %s                           ",		Addy,OpData[0],				TraceArr[OpData[0]]);								break;
	case ACC:sprintf(str,"%04X  %02X       %s A                         ",		Addy,OpData[0],				TraceArr[OpData[0]]);								break;
	case IMM:sprintf(str,"%04X  %02X %02X    %s #$%02X                      ",	Addy,OpData[0],OpData[1],		TraceArr[OpData[0]],Operand);							break;
	case REL:sprintf(str,"%04X  %02X %02X    %s $%04X                     ",	Addy,OpData[0],OpData[1],		TraceArr[OpData[0]],Operand);							break;
	case ZPG:sprintf(str,"%04X  %02X %02X    %s $%02X = %02X                  ",	Addy,OpData[0],OpData[1],		TraceArr[OpData[0]],Operand,TraceMem(Operand));					break;
	case ZPX:sprintf(str,"%04X  %02X %02X    %s $%02X,X @ %02X = %02X           ",	Addy,OpData[0],OpData[1],		TraceArr[OpData[0]],Operand,EffectiveAddy,TraceMem(EffectiveAddy));		break;
	case ZPY:sprintf(str,"%04X  %02X %02X    %s $%02X,Y @ %02X = %02X           ",	Addy,OpData[0],OpData[1],		TraceArr[OpData[0]],Operand,EffectiveAddy,TraceMem(EffectiveAddy));		break;
	case INX:sprintf(str,"%04X  %02X %02X    %s ($%02X,X) @ %02X = %04X = %02X  ",	Addy,OpData[0],OpData[1],		TraceArr[OpData[0]],Operand,MidAddy,EffectiveAddy,TraceMem(EffectiveAddy));	break;
	case INY:sprintf(str,"%04X  %02X %02X    %s ($%02X),Y = %04X @ %04X = %02X",	Addy,OpData[0],OpData[1],		TraceArr[OpData[0]],Operand,MidAddy,EffectiveAddy,TraceMem(EffectiveAddy));	break;
	case ADR:sprintf(str,"%04X  %02X %02X %02X %s $%04X                     ",	Addy,OpData[0],OpData[1],OpData[2],	TraceArr[OpData[0]],Operand);							break;
	case ABS:sprintf(str,"%04X  %02X %02X %02X %s $%04X = %02X                ",	Addy,OpData[0],OpData[1],OpData[2],	TraceArr[OpData[0]],Operand,TraceMem(Operand));					break;
	case IND:sprintf(str,"%04X  %02X %02X %02X %s ($%04X) = %04X            ",	Addy,OpData[0],OpData[1],OpData[2],	TraceArr[OpData[0]],Operand,EffectiveAddy);					break;
	case ABX:sprintf(str,"%04X  %02X %02X %02X %s $%04X,X @ %04X = %02X       ",	Addy,OpData[0],OpData[1],OpData[2],	TraceArr[OpData[0]],Operand,EffectiveAddy,TraceMem(EffectiveAddy));		break;
	case ABY:sprintf(str,"%04X  %02X %02X %02X %s $%04X,Y @ %04X = %02X       ",	Addy,OpData[0],OpData[1],OpData[2],	TraceArr[OpData[0]],Operand,EffectiveAddy,TraceMem(EffectiveAddy));		break;
	default : strcpy(str,"                                             ");																	break;
	}
}

void	Debugger_DrawTraceLine (unsigned short Addy, short y)
{
	char tpc[64];
	Debugger.AddyLine[y/Debugger.FontHeight] = Addy;
	
	DecodeInstruction(Addy,tpc);
	if (Debugger.BreakP[Addy])
	{
		HBRUSH RBrush = CreateSolidBrush(RGB(255,0,0));
		RECT trect;
		trect.left = 0;
		trect.top = y;
		trect.right = D_TRC_W;
		trect.bottom = y+Debugger.FontHeight;
		FillRect(Debugger.TraceDC, &trect, RBrush);
		DeleteObject(RBrush);
	}

	TextOut(Debugger.TraceDC, 1, y-3, tpc, (int)strlen(tpc));
}

void	Debugger_Update (void)
{
	if (Debugger.Mode >= 3)
	{
		if (CPU.WantIRQ & IRQ_DEBUG)
			CPU.WantIRQ &= ~IRQ_DEBUG;
		if (Debugger.BreakP[CPU.PC] || Debugger.Step)
			NES.Stop = TRUE;
		if (NES.Stop)
		{
			HBRUSH WBrush = CreateSolidBrush(0xFFFFFF);
			RECT trect;
			char tpc[40];
			int i, TpVal, CurrY;
			unsigned short StartAddy;
			unsigned short CurrAddy;
			unsigned short BackAddies[0x100];
			int FoundPtr = 0xFF;
			int StartY;
			short MaxY;

			trect.left = 0;
			trect.top = 0;
			trect.right = D_REG_W;
			trect.bottom = D_REG_H;
			FillRect(Debugger.RegDC, &trect, WBrush);

			trect.left = 0;
			trect.top = 0;
			trect.right = D_TRC_W;
			trect.bottom = D_TRC_H;
			FillRect(Debugger.TraceDC, &trect, WBrush);
			DeleteObject(WBrush);

			sprintf(tpc,"PC: %04X",CPU.PC);	TextOut(Debugger.RegDC,0, 0*Debugger.FontHeight,tpc,(int)strlen(tpc));
			sprintf(tpc,"A: %02X",CPU.A);	TextOut(Debugger.RegDC,0, 1*Debugger.FontHeight,tpc,(int)strlen(tpc));
			sprintf(tpc,"X: %02X",CPU.X);	TextOut(Debugger.RegDC,0, 2*Debugger.FontHeight,tpc,(int)strlen(tpc));
			sprintf(tpc,"Y: %02X",CPU.Y);	TextOut(Debugger.RegDC,0, 3*Debugger.FontHeight,tpc,(int)strlen(tpc));
			sprintf(tpc,"SP: %02X",CPU.SP);	TextOut(Debugger.RegDC,0, 4*Debugger.FontHeight,tpc,(int)strlen(tpc));
			CPU_JoinFlags();
			sprintf(tpc,"P: %02X",CPU.P);	TextOut(Debugger.RegDC,0, 5*Debugger.FontHeight,tpc,(int)strlen(tpc));
			sprintf(tpc,"->%s%s%s%s%s%s  IRQ:%s %s %s %s",CPU.FN?"N":" ",CPU.FV?"V":" ",CPU.FI?"I":" ",CPU.FD?"D":" ",CPU.FZ?"Z":" ",CPU.FC?"C":" ",(CPU.WantIRQ&IRQ_DPCM)?"DMC":"   ",(CPU.WantIRQ&IRQ_FRAME)?"FRM":"   ",(CPU.WantIRQ&IRQ_EXTERNAL)?"EXT":"   ",(CPU.WantIRQ&IRQ_DEBUG)?"DBG":"   ");
							TextOut(Debugger.RegDC,0, 6*Debugger.FontHeight,tpc,(int)strlen(tpc));

			if ((PPU.SLnum >= 0) && (PPU.SLnum < 240))
				sprintf(tpc,"SLnum: %i",PPU.SLnum);
			else	sprintf(tpc,"SLnum: %i (VBlank)",PPU.SLnum);
							TextOut(Debugger.RegDC,0, 7*Debugger.FontHeight,tpc,(int)strlen(tpc));

			sprintf(tpc,"CPU Ticks: %i/%.3f",PPU.Clockticks,PPU.Clockticks / (PPU.IsPAL ? 3.2 : 3.0));
							TextOut(Debugger.RegDC,0, 8*Debugger.FontHeight,tpc,(int)strlen(tpc));
			sprintf(tpc,"VRAMAddy: %04X",PPU.VRAMAddr);
							TextOut(Debugger.RegDC,0, 9*Debugger.FontHeight,tpc,(int)strlen(tpc));

			sprintf(tpc,"CPU Pages:");	TextOut(Debugger.RegDC,0,11*Debugger.FontHeight,tpc,(int)strlen(tpc));
			for (i = 0; i < 16; i++)
			{
				if (EI.GetPRG_ROM4(i) >= 0)
					sprintf(tpc,"%03X",EI.GetPRG_ROM4(i));
				else if (EI.GetPRG_RAM4(i) >= 0)
					sprintf(tpc,"A%02X",EI.GetPRG_RAM4(i));
				else	sprintf(tpc,"???");
							TextOut(Debugger.RegDC,(i&7)*4*Debugger.FontWidth,(12 + (i >> 3))*Debugger.FontHeight,tpc,(int)strlen(tpc));
			}
			sprintf(tpc,"PPU Pages:");	TextOut(Debugger.RegDC,0,15*Debugger.FontHeight,tpc,(int)strlen(tpc));
			for (i = 0; i < 12; i++)
			{
				if (EI.GetCHR_ROM1(i) >= 0)
					sprintf(tpc,"%03X",EI.GetCHR_ROM1(i));
				else if (EI.GetCHR_RAM1(i) >= 0)
					sprintf(tpc,"A%02X",EI.GetCHR_RAM1(i));
				else if (EI.GetCHR_NT1(i) >= 0)
					sprintf(tpc,"N%02X",EI.GetCHR_NT1(i));
				else	sprintf(tpc,"???");
							TextOut(Debugger.RegDC,(i&7)*4*Debugger.FontWidth,(16 + (i >> 3))*Debugger.FontHeight,tpc,(int)strlen(tpc));
			}
			RedrawWindow(Debugger.RegWnd,NULL,NULL,RDW_INVALIDATE);

			for (i = 0; i < 64; i++)
				Debugger.AddyLine[i] = -1;

			StartAddy = (unsigned short)((Debugger.TraceOffset == -1) ? CPU.PC : Debugger.TraceOffset);
			StartY = D_TRC_H / 2;
			StartY = StartY - (StartY % Debugger.FontHeight);
		
			CurrAddy = StartAddy;
			MaxY = D_TRC_H;
			MaxY = (int) (MaxY/Debugger.FontHeight)*Debugger.FontHeight;
			if (!((MaxY/Debugger.FontHeight) & 1))
				MaxY+=Debugger.FontHeight;
			for (CurrY=StartY; CurrY<MaxY; CurrY+=Debugger.FontHeight)
			{
				Debugger_DrawTraceLine(CurrAddy, CurrY);
				CurrAddy += AddrBytes[TraceAddyMode[TraceMem(CurrAddy)]];
			}
			CurrAddy = StartAddy-0x100;
			for (TpVal=0;TpVal<0x100;TpVal++)
			{
				BackAddies[TpVal] = CurrAddy;
				CurrAddy += AddrBytes[TraceAddyMode[TraceMem(CurrAddy)]];
				if (CurrAddy == StartAddy) FoundPtr = TpVal;
			}
			
			CurrAddy = BackAddies[FoundPtr--];
			for (CurrY=StartY-Debugger.FontHeight; CurrY>=0; CurrY-=Debugger.FontHeight)
			{
				Debugger_DrawTraceLine(CurrAddy, CurrY);
				CurrAddy = BackAddies[FoundPtr--];
			}

			MoveToEx(Debugger.TraceDC, 0, StartY, NULL);
			LineTo(Debugger.TraceDC, D_TRC_W, StartY);

			MoveToEx(Debugger.TraceDC, 0, StartY+Debugger.FontHeight, NULL);
			LineTo(Debugger.TraceDC, D_TRC_W, StartY+Debugger.FontHeight);
			RedrawWindow(Debugger.TraceWnd,NULL,NULL,RDW_INVALIDATE);

			Debugger_UpdateGraphics();
		}
	}
}

void	Debugger_AddInst (void)
{
	if (Debugger.Logging)
	{
		unsigned short Addy = (unsigned short)CPU.PC;
		char tpc[64];
		DecodeInstruction(Addy,tpc);
		DPrint(tpc);
		CPU_JoinFlags();
		sprintf(tpc,"  A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3i SL:%i\n",CPU.A,CPU.X,CPU.Y,CPU.P,CPU.SP,PPU.Clockticks,PPU.SLnum);
		DPrint(tpc);
	}
}

void	Debugger_UpdateGraphics (void)
{
	register unsigned char DInc = (Debugger.Depth) == 32 ? 4 : 2;
	long PatPtr;
	unsigned char PalVal;
	int MemAddy;
	if (Debugger.Mode >= 1)
	{
		if (Debugger.PalChanged)
		{
			int x, y, z;
			Debugger.PalChanged = FALSE;
			for (z = 0; z < 16; z++)
				for (x = 0; x < 16; x++)
					for (y = 0; y < 16; y++)
					{
						int BaseVal = (y << 8) | (z << 4) | x;
						if (Debugger.Depth == 32)
						{
							((unsigned long *)Debugger.PaletteArray)[BaseVal         ] = GFX.Palette32[PPU.Palette[       z]];
							((unsigned long *)Debugger.PaletteArray)[BaseVal | 0x1000] = GFX.Palette32[PPU.Palette[0x10 | z]];
						}
						else if (Debugger.Depth == 16)
						{
							((unsigned short *)Debugger.PaletteArray)[BaseVal         ] = GFX.Palette16[PPU.Palette[       z]];
							((unsigned short *)Debugger.PaletteArray)[BaseVal | 0x1000] = GFX.Palette16[PPU.Palette[0x10 | z]];
						}
						else
						{
							((unsigned short *)Debugger.PaletteArray)[BaseVal         ] = GFX.Palette15[PPU.Palette[       z]];
							((unsigned short *)Debugger.PaletteArray)[BaseVal | 0x1000] = GFX.Palette15[PPU.Palette[0x10 | z]];
						}
					}
		}
		SetBitmapBits(Debugger.PaletteBMP,DInc*D_PAL_W*D_PAL_H,&Debugger.PaletteArray);
		RedrawWindow(Debugger.PaletteWnd,NULL,NULL,RDW_INVALIDATE);

		if (Debugger.PatChanged)
		{
			int i, t, x, y, sx, sy;
			static unsigned char PatPal[4];
			Debugger.PatChanged = FALSE;

			PatPal[0] = PPU.Palette[0];
			for (i = 1; i < 4; i++)
				PatPal[i] = PPU.Palette[(Debugger.PatPalBase << 2) | i];

			for (t = 0; t < 2; t++)
				for (y = 0; y < 16; y++)
					for (x = 0; x < 16; x++)
					{
						MemAddy = (t << 12) | (y << 8) | (x << 4);
						for (sy = 0; sy < 8; sy++)
						{
							PatPtr = (((y << 3) + sy) << 8) + (t << 7) + (x << 3);
							for (sx = 0; sx < 8; sx++)
							{
								PalVal = (VMemory(MemAddy) & (0x80 >> sx)) >> (7-sx);
								PalVal |= ((VMemory(MemAddy+8) & (0x80 >> sx)) >> (7-sx)) << 1;
								if (Debugger.Depth == 32)
									((unsigned long *)Debugger.PatternArray)[PatPtr] = GFX.Palette32[PatPal[PalVal]];
								else if (Debugger.Depth == 16)
									((unsigned short *)Debugger.PatternArray)[PatPtr] = GFX.Palette16[PatPal[PalVal]];
								else	((unsigned short *)Debugger.PatternArray)[PatPtr] = GFX.Palette15[PatPal[PalVal]];
								PatPtr++;
							}
							MemAddy++;
						}
					}
		}
		SetBitmapBits(Debugger.PatternBMP,DInc*D_PAT_W*D_PAT_H,&Debugger.PatternArray);
		RedrawWindow(Debugger.PatternWnd,NULL,NULL,RDW_INVALIDATE);
	}
	if (Debugger.Mode >= 2)
	{
		if (Debugger.NTabChanged)
		{
			int AttribTableVal, AttribNum;
			int NT, TileY, TileX, py, px;
			Debugger.NTabChanged = FALSE;
			for (NT = 0; NT < 4; NT ++)
				for (TileY = 0; TileY < 30; TileY++)
					for (TileX = 0; TileX < 32; TileX++)
					{
						AttribNum = (((TileX & 2) >> 1) | (TileY & 2)) << 1;
						AttribTableVal = (PPU.CHRPointer[8 | NT][0x3C0 | ((TileY >> 2) << 3) | (TileX >> 2)] & (3 << AttribNum)) >> AttribNum;
						MemAddy = ((PPU.Reg2000 & 0x10) << 8) | (PPU.CHRPointer[8 | NT][TileX | (TileY << 5)] << 4);
						for (py = 0; py < 8; py ++)
						{
							if (NT & 2)
								PatPtr = (512*240) + ((NT&1) << 8) + (TileY << 12) | (py << 9) | (TileX << 3);
							else	PatPtr = (NT << 8) + (TileY << 12) | (py << 9) | (TileX << 3);
							
							for (px = 0; px < 8; px ++)
							{
								PalVal = (VMemory(MemAddy) & (0x80 >> px)) >> (7-px);
								PalVal |= ((VMemory(MemAddy+8) & (0x80 >> px)) >> (7-px)) << 1;
								if (PalVal)
								{
									PalVal |= (AttribTableVal << 2);
									if (Debugger.Depth == 32)
										((unsigned long *)Debugger.NameArray)[PatPtr] = GFX.Palette32[PPU.Palette[PalVal]];
									else if (Debugger.Depth == 16)
										((unsigned short *)Debugger.NameArray)[PatPtr] = GFX.Palette16[PPU.Palette[PalVal]];
									else	((unsigned short *)Debugger.NameArray)[PatPtr] = GFX.Palette15[PPU.Palette[PalVal]];
								}
								else
								{
									if (Debugger.Depth == 32)
										((unsigned long *)Debugger.NameArray)[PatPtr] = GFX.Palette32[PPU.Palette[0]];
									else if (Debugger.Depth == 16)
										((unsigned short *)Debugger.NameArray)[PatPtr] = GFX.Palette16[PPU.Palette[0]];
									else	((unsigned short *)Debugger.NameArray)[PatPtr] = GFX.Palette15[PPU.Palette[0]];
								}
								PatPtr++;
							}
							MemAddy++;
						}
					}
		}

		SetBitmapBits(Debugger.NameBMP,DInc*D_NAM_W*D_NAM_H,&Debugger.NameArray);
		RedrawWindow(Debugger.NameWnd,NULL,NULL,RDW_INVALIDATE);
	}
}

LRESULT CALLBACK PaletteProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	switch (uMsg)
	{
	case WM_PAINT:
		hdc = BeginPaint(hwndDlg,&ps);
		BitBlt(hdc,0,0,D_PAL_W,D_PAL_H,Debugger.PaletteDC,0,0,SRCCOPY);
		EndPaint(hwndDlg,&ps);
		break;
	}
	return FALSE;
}

LRESULT CALLBACK PatternProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	switch (uMsg)
	{
	case WM_PAINT:
		hdc = BeginPaint(hwndDlg,&ps);
		BitBlt(hdc,0,0,D_PAT_W,D_PAT_H,Debugger.PatternDC,0,0,SRCCOPY);
		EndPaint(hwndDlg,&ps);
		break;
	case WM_LBUTTONUP:
		Debugger.PatPalBase = (Debugger.PatPalBase + 1) & 7;
		Debugger.PatChanged = TRUE;
		Debugger_UpdateGraphics();
		break;
	}
	return FALSE;
}

LRESULT CALLBACK NameProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	switch (uMsg)
	{
	case WM_PAINT:
		hdc = BeginPaint(hwndDlg,&ps);
		BitBlt(hdc,0,0,D_NAM_W,D_NAM_H,Debugger.NameDC,0,0,SRCCOPY);
		EndPaint(hwndDlg,&ps);
		break;
	}
	return FALSE;
}

LRESULT CALLBACK DumpProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	switch (uMsg)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		switch (wmId)
		{
		case IDC_DUMP_CPU_MEM:
		{
			FILE *CPUMemOut = fopen("CPU.Mem","wb");
			int i;
			fwrite(CPU_RAM,1,0x800,CPUMemOut);
			for (i = 4; i < 16; i++)
				if (CPU.PRGPointer[i])
					fwrite(CPU.PRGPointer[i],1,0x1000,CPUMemOut);
			fclose(CPUMemOut);
		}	break;
		case IDC_DUMP_PPU_MEM:
		{
			FILE *PPUMemOut = fopen("PPU.Mem","wb");
			int i;
			for (i = 0; i < 12; i++)
				fwrite(PPU.CHRPointer[i],1,0x400,PPUMemOut);
			fwrite(PPU.Sprite,1,0x100,PPUMemOut);
			fwrite(PPU.Palette,1,0x20,PPUMemOut);
			fclose(PPUMemOut);
		}	break;
		case IDC_START_LOGGING:
			Debugger_StartLogging();
			break;
		case IDC_STOP_LOGGING:
			Debugger_StopLogging();
			break;
		case IDC_IRQBUTTON:
			CPU.WantIRQ |= IRQ_DEBUG;
			break;
		}
		break;
	}
	return FALSE;
}

LRESULT CALLBACK TraceProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	switch (uMsg)
	{
	case WM_PAINT:
		hdc = BeginPaint(hwndDlg,&ps);
		BitBlt(hdc,0,0,D_TRC_W,D_TRC_H,Debugger.TraceDC,0,0,SRCCOPY);
		EndPaint(hwndDlg,&ps);
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_HOME:
			if (Debugger.TraceOffset == -1)
				Debugger.TraceOffset = CPU.PC - 0x1000;
			else	Debugger.TraceOffset -= 0x1000;
			Debugger_Update();
			break;
		case VK_END:
			if (Debugger.TraceOffset == -1)
				Debugger.TraceOffset = CPU.PC + 0x1000;
			else	Debugger.TraceOffset += 0x1000;
			Debugger_Update();
			break;
		case VK_PRIOR:{
			unsigned short NewAddy = Debugger.AddyLine[0];
			if (NewAddy == CPU.PC)
				NewAddy = -1;
			Debugger.TraceOffset = NewAddy;
			Debugger_Update();
			}break;
		case VK_UP:{
			int i;
			short MaxAddy = 0;
			unsigned short NewAddy;
			for (i = 0; i < 64; i++)
			{
				if (Debugger.AddyLine[i] != -1)
					MaxAddy = i;
			}
			NewAddy = Debugger.AddyLine[(MaxAddy/2) - 1];
			if (NewAddy == CPU.PC)
				Debugger.TraceOffset = -1;
			else	Debugger.TraceOffset = NewAddy;
			Debugger_Update();
			}break;
		case VK_DOWN:{
			int i;
			short MaxAddy = 0;
			unsigned short NewAddy;
			for (i=0;i<64;i++)
			{
				if (Debugger.AddyLine[i] != -1)
				{
					MaxAddy = i;
				}
			}
			NewAddy = Debugger.AddyLine[(MaxAddy/2) + 1];
			if (NewAddy == CPU.PC)
				Debugger.TraceOffset = -1;
			else	Debugger.TraceOffset = NewAddy;
			Debugger_Update();
			}break;
		case VK_NEXT:{
			int i;
			short MaxAddy = 0;
			unsigned short NewAddy;
			for (i=0;i<64;i++)
			{
				if (Debugger.AddyLine[i] != -1)
					MaxAddy = i;
			}
			NewAddy = Debugger.AddyLine[MaxAddy];
			if (NewAddy == CPU.PC)
				Debugger.TraceOffset = -1;
			else	Debugger.TraceOffset = NewAddy;
			Debugger_Update();
			}break;
		}
		break;
	case WM_LBUTTONDOWN:{
		short xPos = LOWORD(lParam);
		short yPos = HIWORD(lParam);
		if ((xPos > 0) && (xPos < D_TRC_W) && (yPos > 0) && (yPos < D_TRC_H))
		{
			Debugger.BreakP[Debugger.AddyLine[yPos/Debugger.FontHeight]] ^= 1;
			Debugger_Update();
			SetFocus(mWnd);
		}
		}break;
	}
	
	return FALSE;
}

LRESULT CALLBACK RegProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	switch (uMsg)
	{
	case WM_PAINT:
		hdc = BeginPaint(hwndDlg,&ps);
		BitBlt(hdc,0,0,D_REG_W,D_REG_H,Debugger.RegDC,0,0,SRCCOPY);
		EndPaint(hwndDlg,&ps);
		break;
	}
	return FALSE;
}

#endif	/* ENABLE_DEBUGGER */