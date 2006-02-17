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
#include "Debugger.h"
#include "CPU.h"
#include "PPU.h"
#include "GFX.h"

struct tDebugger Debugger;

extern	unsigned char CPU_Flags[0x10000];

#define DW_PAL_LEFT	0
#define DW_PAL_TOP	1
#define DW_PAT_LEFT	2
#define DW_PAT_TOP	3
#define DW_DMP_LEFT	4
#define DW_DMP_TOP	5
#define DW_DMP_WIDTH	6
#define DW_DMP_HEIGHT	7
#define DW_NAM_LEFT	8
#define DW_NAM_TOP	9
#define DW_TRC_LEFT	10
#define DW_TRC_TOP	11
#define DW_TRC_WIDTH	12
#define DW_TRC_HEIGHT	13
#define DW_REG_LEFT	14
#define DW_REG_TOP	15
#define DW_REG_WIDTH	16
#define DW_REG_HEIGHT	17
#define DW_MEM_LEFT	18
#define DW_MEM_TOP	19

BOOL CALLBACK PaletteProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PatternProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK NameProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DumpProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TraceProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK RegProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

const unsigned char CharSC[8] = {
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

enum ADDRMODE { IMP, ACC, IMM, ABS, IND, REL, ABX, ABY, ZPG, ZPX, ZPY, INX, INY, ERR, NUM_ADDR_MODES };

#define	FLAG_N	0x80
#define	FLAG_V	0x40
#define	FLAG_B	0x10
#define	FLAG_I	0x04
#define	FLAG_D	0x08
#define	FLAG_Z	0x02
#define	FLAG_C	0x01

enum ADDRMODE TraceAddyMode[256] =
{
	IMM,INX,ERR,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,ACC,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPX,ZPX,IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	ABS,INX,ERR,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,ACC,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPX,ZPX,IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	IMP,INX,ERR,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,ACC,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPX,ZPX,IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	IMP,INX,ERR,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,ACC,IMM,IND,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPX,ZPX,IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	IMM,INX,IMM,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,IMP,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPY,ZPY,IMP,ABY,IMP,ABY,ABX,ABX,ABY,ABY,
	IMM,INX,IMM,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,IMP,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPY,ZPY,IMP,ABY,IMP,ABY,ABX,ABX,ABY,ABY,
	IMM,INX,IMM,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,IMP,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPX,ZPX,IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	IMM,INX,IMM,INX,ZPG,ZPG,ZPG,ZPG,IMP,IMM,IMP,IMM,ABS,ABS,ABS,ABS,REL,INY,ERR,INY,ZPX,ZPX,ZPX,ZPX,IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX
};

unsigned char AddrBytes[NUM_ADDR_MODES] = {1,1,2,3,3,2,3,3,2,2,2,2,2,1};

unsigned char bytes[256] =
{
	1,2,1,2,2,2,2,2,1,2,1,2,3,3,3,3,2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
	3,2,1,2,2,2,2,2,1,2,1,2,3,3,3,3,2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
	1,2,1,2,2,2,2,2,1,2,1,2,3,3,3,3,2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
	1,2,1,2,2,2,2,2,1,2,1,2,3,3,3,3,2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
	2,2,2,2,2,2,2,2,1,2,1,2,3,3,3,3,2,2,1,2,2,2,2,2,1,3,1,3,3,3,1,3,
	2,2,2,2,2,2,2,2,1,2,1,2,3,3,3,3,2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
	2,2,2,2,2,2,2,2,1,2,1,2,3,3,3,3,2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
	2,2,2,2,2,2,2,2,1,2,1,2,3,3,3,3,2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3
};

char TraceArr[256][4] =
{
	"BRK","ORA","HLT","???","???","ORA","ASL","???","PHP","ORA","ASL","???","???","ORA","ASL","???",
	"BPL","ORA","HLT","???","???","ORA","ASL","???","CLC","ORA","???","???","???","ORA","ASL","???",
	"JSR","AND","HLT","???","BIT","AND","ROL","???","PLP","AND","ROL","???","BIT","AND","ROL","???",
	"BMI","AND","HLT","???","???","AND","ROL","???","SEC","AND","???","???","???","AND","ROL","???",
	"RTI","EOR","HLT","???","???","EOR","LSR","???","PHA","EOR","LSR","???","JMP","EOR","LSR","???",
	"BVC","EOR","HLT","???","???","EOR","LSR","???","CLI","EOR","???","???","???","EOR","LSR","???",
	"RTS","ADC","HLT","???","???","ADC","ROR","???","PLA","ADC","ROR","???","JMP","ADC","ROR","???",
	"BVS","ADC","HLT","???","???","ADC","ROR","???","SEI","ADC","???","???","???","ADC","ROR","???",
	"???","STA","???","???","STY","STA","STX","???","DEY","???","TXA","???","STY","STA","STX","???",
	"BCC","STA","HLT","???","STY","STA","STX","???","TYA","STA","TXS","???","???","STA","???","???",
	"LDY","LDA","LDX","???","LDY","LDA","LDX","???","TAY","LDA","TAX","???","LDY","LDA","LDX","???",
	"BCS","LDA","HLT","???","LDY","LDA","LDX","???","CLV","LDA","TSX","???","LDY","LDA","LDX","???",
	"CPY","CMP","???","???","CPY","CMP","DEC","???","INY","CMP","DEX","???","CPY","CMP","DEC","???",
	"BNE","CMP","HLT","???","???","CMP","DEC","???","CLD","CMP","???","???","???","CMP","DEC","???",
	"CPX","SBC","???","???","CPX","SBC","INC","???","INX","SBC","NOP","???","CPX","SBC","INC","???",
	"BEQ","SBC","HLT","???","???","SBC","INC","???","SED","SBC","???","???","???","SBC","INC","???"
};

unsigned char PatPal[4] = { 0x0F, 0x3C, 0x2A, 0x30 };

unsigned char VMemory (unsigned long Addy) { return PPU.CHRPointer[(Addy & 0x1C00) >> 10][Addy & 0x03FF]; }

short WindowPoses[2][32] = {
	//SingleSize Mode
	{
		256+8, 240+24+128+24,		//Palette
		256+8, 240+24,			//Pattern
		0, 240+48,			//Dumps L/T
		256+8, 32+24+128,		//Dumps W/H
		256+8, 0,			//Name Tables
		256+8+256+8, 32+24+128+48,	//Trace L/T
		256+8, 32+24+128+48+8,		//Trace W/H
		256+8+256+8, 0,			//Registers L/T
		256+8, 32+24+128+48,		//Registers W/H
		0, 0				//MemView L/T
	},
	//DoubleSize Mode
	{
		0, 480+48+128+24,	//Palette
		0, 480+48,		//Pattern
		256+8, 480+48,		//Dumps L/T
		256, 32+24+128+24,	//Dumps W/H
		512+8, 0,		//Name Tables
		512+8+244, 480+24,	//Trace L/T
		256+4, 32+24+128+48,	//Trace W/H
		512+8, 480+24,		//Registers L/T
		244, 32+24+128+48,	//Registers W/H
		0, 0			//MemView L/T
	}
};

void	Debugger_Init (void)
{
	HDC TpHDC = GetDC(mWnd);
	//Palette
	Debugger.PaletteDC = CreateCompatibleDC(TpHDC);
	Debugger.PaletteBMP = CreateCompatibleBitmap(TpHDC,256,32);
	//Pattern Tables
	Debugger.PatternDC = CreateCompatibleDC(TpHDC);
	Debugger.PatternBMP = CreateCompatibleBitmap(TpHDC,256,128);
	//Name Tables
	Debugger.NameDC = CreateCompatibleDC(TpHDC);
	//Trace Window
	Debugger.TraceDC = CreateCompatibleDC(TpHDC);
	//Register Window
	Debugger.RegDC = CreateCompatibleDC(TpHDC);

	ReleaseDC(mWnd,TpHDC);

	ZeroMemory(&Debugger.BreakP, sizeof(Debugger.BreakP));
	Debugger.TraceOffset = -1;
	
	Debugger.DoubleSize = 0;

	Debugger.PatPalBase = 0;

	Debugger.Logging = FALSE;

	Debugger.FontWidth = 7;
	Debugger.FontHeight = 10;
	
	TpHDC = GetWindowDC(GetDesktopWindow());
	Debugger.Depth = (GetDeviceCaps(TpHDC,BITSPIXEL) >> 3);
	ReleaseDC(GetDesktopWindow(), TpHDC);
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
	HDC TpHDC;
	if (Debugger.DoubleSize > 1)
		Debugger.DoubleSize = 0;

	Debugger.Mode = NewMode;
	Debugger.Enabled = (Debugger.Mode > 0) ? TRUE : FALSE;

	TpHDC = GetDC(mWnd);

	Debugger.NTabChanged = TRUE;
	Debugger.PalChanged = TRUE;
	Debugger.PatChanged = TRUE;
	
	SetWindowPos(mWnd,HWND_TOP,0,0,0,0,SWP_NOSIZE);		//Move Main Window to (0,0)
	//Release Handles - Assume Mode 0
	DestroyWindow(Debugger.DumpWnd);
	ReleaseDC(Debugger.PaletteWnd, Debugger.PaletteWDC);		//Palette
	DestroyWindow(Debugger.PaletteWnd);
	ReleaseDC(Debugger.PatternWnd, Debugger.PatternWDC);		//Pattern Tables
	DestroyWindow(Debugger.PatternWnd);
	ReleaseDC(Debugger.NameWnd, Debugger.NameWDC);			//Name Tables
	DestroyWindow(Debugger.NameWnd);
	DeleteObject(Debugger.NameBMP);
	Debugger.NameBMP = CreateCompatibleBitmap(TpHDC,256*(Debugger.DoubleSize+1),240*(Debugger.DoubleSize+1));
	ReleaseDC(Debugger.TraceWnd, Debugger.TraceWDC);			//Trace
	DestroyWindow(Debugger.TraceWnd);
	DeleteObject(Debugger.TraceBMP);
	Debugger.TraceBMP = CreateCompatibleBitmap(TpHDC,WindowPoses[Debugger.DoubleSize][DW_TRC_WIDTH]-8,WindowPoses[Debugger.DoubleSize][DW_TRC_HEIGHT]-24);
	SelectObject(Debugger.TraceDC,Debugger.TraceBMP);
	DestroyWindow(Debugger.RegWnd);					//Registers
	Debugger.RegBMP = CreateCompatibleBitmap(TpHDC,WindowPoses[Debugger.DoubleSize][DW_REG_WIDTH]-8,WindowPoses[Debugger.DoubleSize][DW_REG_HEIGHT]-24);
	SelectObject(Debugger.RegDC,Debugger.RegBMP);

	ReleaseDC(mWnd,TpHDC);

	if (Debugger.Mode >= 1)
	{	//Palette + Pattern Tables
		//Palette
		Debugger.PaletteWnd = CreateDialog(hInst, (LPCTSTR) IDD_DEBUGGER_PALETTE, mWnd, PaletteProc);
		SetWindowPos(Debugger.PaletteWnd,HWND_TOP,WindowPoses[Debugger.DoubleSize][DW_PAL_LEFT],WindowPoses[Debugger.DoubleSize][DW_PAL_TOP],256+8,32+24,SWP_SHOWWINDOW);
		Debugger.PaletteWDC = GetDC(Debugger.PaletteWnd);
		//Pattern Tables
		Debugger.PatternWnd = CreateDialog(hInst, (LPCTSTR) IDD_DEBUGGER_PATTERN, mWnd, PatternProc);
		SetWindowPos(Debugger.PatternWnd,HWND_TOP,WindowPoses[Debugger.DoubleSize][DW_PAT_LEFT],WindowPoses[Debugger.DoubleSize][DW_PAT_TOP],256+8,128+24,SWP_SHOWWINDOW);
		Debugger.PatternWDC = GetDC(Debugger.PatternWnd);
		//Dumps
		Debugger.DumpWnd = CreateDialog(hInst, (LPCTSTR) IDD_DEBUGGER_DUMPS, mWnd, DumpProc);
		SetWindowPos(Debugger.DumpWnd,HWND_TOP,WindowPoses[Debugger.DoubleSize][DW_DMP_LEFT],WindowPoses[Debugger.DoubleSize][DW_DMP_TOP],WindowPoses[Debugger.DoubleSize][DW_DMP_WIDTH],WindowPoses[Debugger.DoubleSize][DW_DMP_HEIGHT],SWP_SHOWWINDOW);
	}
	if (Debugger.Mode >= 2)
	{	// + Name Tables
		//Name Tables
		Debugger.NameWnd = CreateDialog(hInst, (LPCTSTR) IDD_DEBUGGER_NAME, mWnd, NameProc);
		SetWindowPos(Debugger.NameWnd,HWND_TOP,WindowPoses[Debugger.DoubleSize][DW_NAM_LEFT],WindowPoses[Debugger.DoubleSize][DW_NAM_TOP],(Debugger.DoubleSize+1)*256+8,(Debugger.DoubleSize+1)*240+24,SWP_SHOWWINDOW);
		Debugger.NameWDC = GetDC(Debugger.NameWnd);
	}
	if (Debugger.Mode >= 3)
	{	// + Registers
		HFONT tpf;
		int nHeight;
		//Registers
		Debugger.RegWnd = CreateDialog(hInst, (LPCTSTR) IDD_DEBUGGER_REGISTERS, mWnd, RegProc);
		SetWindowPos(Debugger.RegWnd,HWND_TOP,WindowPoses[Debugger.DoubleSize][DW_REG_LEFT],WindowPoses[Debugger.DoubleSize][DW_REG_TOP],WindowPoses[Debugger.DoubleSize][DW_REG_WIDTH],WindowPoses[Debugger.DoubleSize][DW_REG_HEIGHT],SWP_SHOWWINDOW);
		Debugger.RegWDC = GetDC(Debugger.RegWnd);
		//Trace Window
		Debugger.TraceWnd = CreateDialog(hInst, (LPCTSTR) IDD_DEBUGGER_TRACE, mWnd, TraceProc);
		SetWindowPos(Debugger.TraceWnd,HWND_TOP,WindowPoses[Debugger.DoubleSize][DW_TRC_LEFT],WindowPoses[Debugger.DoubleSize][DW_TRC_TOP],WindowPoses[Debugger.DoubleSize][DW_TRC_WIDTH],WindowPoses[Debugger.DoubleSize][DW_TRC_HEIGHT],SWP_SHOWWINDOW);
		Debugger.TraceWDC = GetDC(Debugger.TraceWnd);

		//Joint Shit
		nHeight = -MulDiv(Debugger.FontHeight, GetDeviceCaps(Debugger.RegWDC, LOGPIXELSY), 72);
		tpf = CreateFont(nHeight, Debugger.FontWidth, 0, 0, 0, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, FF_MODERN, "Courier New");
		SelectObject(Debugger.RegWDC, tpf);
		SelectObject(Debugger.RegDC, tpf);
		SelectObject(Debugger.TraceWDC, tpf);
		SelectObject(Debugger.TraceDC, tpf);
		DeleteObject(tpf);
		SetBkMode(Debugger.RegDC,TRANSPARENT);
		SetBkMode(Debugger.TraceDC,TRANSPARENT);
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

unsigned char TraceMem (unsigned short Addy)
{
	return CPU.ReadHandler[Addy >> 12](Addy >> 12, Addy & 0xFFF);
}

void	Debugger_DrawTraceLine (unsigned short Addy, short y)
{
	char tpc[32];
	y -= 3;
	Debugger.AddyLine[y/Debugger.FontHeight] = Addy;
	

	switch (TraceAddyMode[TraceMem(Addy)])
	{
	case IMP:sprintf(tpc,"%04X  %02X        %s",			Addy,TraceMem(Addy),					TraceArr[TraceMem(Addy)]);						break;
	case ACC:sprintf(tpc,"%04X  %02X        %s A",			Addy,TraceMem(Addy),					TraceArr[TraceMem(Addy)]);						break;
	case IMM:sprintf(tpc,"%04X  %02X %02X     %s #$%02X",		Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],TraceMem(Addy+1));				break;
	case ABS:sprintf(tpc,"%04X  %02X %02X %02X  %s $%04X",		Addy,TraceMem(Addy),TraceMem(Addy+1),TraceMem(Addy+2),	TraceArr[TraceMem(Addy)],TraceMem(Addy+1) | (TraceMem(Addy+2) << 8));	break;
	case IND:sprintf(tpc,"%04X  %02X %02X %02X  %s ($%04X)",	Addy,TraceMem(Addy),TraceMem(Addy+1),TraceMem(Addy+2),	TraceArr[TraceMem(Addy)],TraceMem(Addy+1) | (TraceMem(Addy+2) << 8));	break;
	case REL:sprintf(tpc,"%04X  %02X %02X     %s $%04X",		Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],Addy+2+(signed char)(TraceMem(Addy+1)));	break;
	case ABX:sprintf(tpc,"%04X  %02X %02X %02X  %s $%04X,X",	Addy,TraceMem(Addy),TraceMem(Addy+1),TraceMem(Addy+2),	TraceArr[TraceMem(Addy)],TraceMem(Addy+1) | (TraceMem(Addy+2) << 8));	break;
	case ABY:sprintf(tpc,"%04X  %02X %02X %02X  %s $%04X,Y",	Addy,TraceMem(Addy),TraceMem(Addy+1),TraceMem(Addy+2),	TraceArr[TraceMem(Addy)],TraceMem(Addy+1) | (TraceMem(Addy+2) << 8));	break;
	case ZPG:sprintf(tpc,"%04X  %02X %02X     %s $%02X",		Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],TraceMem(Addy+1));				break;
	case ZPX:sprintf(tpc,"%04X  %02X %02X     %s $%02X,X",		Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],TraceMem(Addy+1));				break;
	case ZPY:sprintf(tpc,"%04X  %02X %02X     %s $%02X,Y",		Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],TraceMem(Addy+1));				break;
	case INX:sprintf(tpc,"%04X  %02X %02X     %s ($%02X,X)",	Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],TraceMem(Addy+1));				break;
	case INY:sprintf(tpc,"%04X  %02X %02X     %s ($%02X),Y",	Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],TraceMem(Addy+1));				break;
	case ERR:sprintf(tpc,"%04X  %02X        %s",			Addy,TraceMem(Addy),					TraceArr[TraceMem(Addy)]);						break;
	default : strcpy(tpc,"");																					break;
	}

	if (Debugger.BreakP[Addy])
	{
		HBRUSH RBrush = CreateSolidBrush(RGB(255,0,0));
		RECT trect;
		trect.left = 0;
		trect.top = y+3;
		trect.right = WindowPoses[Debugger.DoubleSize][DW_TRC_WIDTH];
		trect.bottom = y+Debugger.FontHeight+3;
		FillRect(Debugger.TraceDC, &trect, RBrush);
		DeleteObject(RBrush);
	}

	TextOut(Debugger.TraceDC, 1, y, tpc, strlen(tpc));
}

void	Debugger_Update (void)
{
	if (Debugger.Mode >= 3)
	{
		//Check for BreakPoint
		if (Debugger.BreakP[CPU.PC])
			NES.Stop = TRUE;

//		if (NES.Stop)	//Only while stepping
		{
			MSG msg;
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

			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (!TranslateAccelerator(mWnd, hAccelTable, &msg)) 
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}

			trect.left = 0;
			trect.top = 0;
			trect.right = WindowPoses[Debugger.DoubleSize][DW_REG_WIDTH];
			trect.bottom = WindowPoses[Debugger.DoubleSize][DW_REG_HEIGHT];
			FillRect(Debugger.RegDC, &trect, WBrush);

			trect.left = 0;
			trect.top = 0;
			trect.right = WindowPoses[Debugger.DoubleSize][DW_TRC_WIDTH];
			trect.bottom = WindowPoses[Debugger.DoubleSize][DW_TRC_HEIGHT];
			FillRect(Debugger.TraceDC, &trect, WBrush);
			DeleteObject(WBrush);

			sprintf(tpc,"PC: %04X",CPU.PC);	TextOut(Debugger.RegDC,0, 0*Debugger.FontHeight,tpc,strlen(tpc));
			sprintf(tpc,"A: %02X",CPU.A);	TextOut(Debugger.RegDC,0, 1*Debugger.FontHeight,tpc,strlen(tpc));
			sprintf(tpc,"X: %02X",CPU.X);	TextOut(Debugger.RegDC,0, 2*Debugger.FontHeight,tpc,strlen(tpc));
			sprintf(tpc,"Y: %02X",CPU.Y);	TextOut(Debugger.RegDC,0, 3*Debugger.FontHeight,tpc,strlen(tpc));
			sprintf(tpc,"SP: %02X",CPU.SP);	TextOut(Debugger.RegDC,0, 4*Debugger.FontHeight,tpc,strlen(tpc));
			sprintf(tpc,"P: %02X",CPU.P);	TextOut(Debugger.RegDC,0, 5*Debugger.FontHeight,tpc,strlen(tpc));
			sprintf(tpc,"->%s%s%s%s%s%s%s%s",CPU.FN?"N":" ",CPU.FV?"V":" "," ",CPU.FB?"B":" ",CPU.FI?"I":" ",CPU.FD?"D":" ",CPU.FZ?"Z":" ",CPU.FC?"C":" ");
							TextOut(Debugger.RegDC,0, 6*Debugger.FontHeight,tpc,strlen(tpc));

			if ((PPU.SLnum >= 0) && (PPU.SLnum < 240))
				sprintf(tpc,"SLnum: %i",PPU.SLnum);
			else	sprintf(tpc,"SLnum: %i (VBlank)",PPU.SLnum);
							TextOut(Debugger.RegDC,0, 7*Debugger.FontHeight,tpc,strlen(tpc));

			sprintf(tpc,"CPU Ticks: %i/%.3f",PPU.Clockticks,PPU.Clockticks / (PPU.IsPAL ? 3.2 : 3.0));
							TextOut(Debugger.RegDC,0, 8*Debugger.FontHeight,tpc,strlen(tpc));
			sprintf(tpc,"VRAMAddy: %04X",PPU.VRAMAddr);
							TextOut(Debugger.RegDC,0, 9*Debugger.FontHeight,tpc,strlen(tpc));

			sprintf(tpc,"CPU Pages:");	TextOut(Debugger.RegDC,0,12*Debugger.FontHeight,tpc,strlen(tpc));
			for (i = 0; i < 8; i++)
			{
				if (MP.GetPRG_ROM4(i+8) >= 0)
					sprintf(tpc,"%03X",MP.GetPRG_ROM4(i+8));
				else if (MP.GetPRG_RAM4(i+8) >= 0)
					sprintf(tpc,"A%02X",MP.GetPRG_RAM4(i+8));
				else	sprintf(tpc,"???");
							TextOut(Debugger.RegDC,i*4*Debugger.FontWidth,13*Debugger.FontHeight,tpc,strlen(tpc));
			}
			sprintf(tpc,"PPU Pages:");	TextOut(Debugger.RegDC,0,14*Debugger.FontHeight,tpc,strlen(tpc));
			for (i = 0; i < 8; i++)
			{
				if (MP.GetCHR_ROM1(i) >= 0)
					sprintf(tpc,"%03X",MP.GetCHR_ROM1(i));
				else if (MP.GetCHR_RAM1(i) >= 0)
					sprintf(tpc,"A%02X",MP.GetCHR_RAM1(i));
				else	sprintf(tpc,"???");
							TextOut(Debugger.RegDC,i*4*Debugger.FontWidth,15*Debugger.FontHeight,tpc,strlen(tpc));
			}
			BitBlt(Debugger.RegWDC,0,0,WindowPoses[Debugger.DoubleSize][DW_REG_WIDTH],WindowPoses[Debugger.DoubleSize][DW_REG_HEIGHT],Debugger.RegDC,0,0,SRCCOPY);

			//Add Disasm
			for (i = 0; i < 64; i++)
				Debugger.AddyLine[i] = -1;

			StartAddy = (unsigned short)((Debugger.TraceOffset == -1) ? CPU.PC : Debugger.TraceOffset);
//			short StartY = (WindowPoses[DoubleSize][DW_TRC_HEIGHT]-24) / 2;
			StartY = 10*Debugger.FontHeight;
			StartY = StartY - (StartY % Debugger.FontHeight);
		
			CurrAddy = StartAddy;
			MaxY = WindowPoses[Debugger.DoubleSize][DW_TRC_HEIGHT]-24;
			MaxY = (int) (MaxY/Debugger.FontHeight)*Debugger.FontHeight;	//Little hack to make it scroll nicely
			if (!((MaxY/Debugger.FontHeight) & 1))		// "
				MaxY+=Debugger.FontHeight;				// "
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
			LineTo(Debugger.TraceDC, WindowPoses[Debugger.DoubleSize][DW_TRC_WIDTH], StartY);

			MoveToEx(Debugger.TraceDC, 0, StartY+Debugger.FontHeight, NULL);
			LineTo(Debugger.TraceDC, WindowPoses[Debugger.DoubleSize][DW_TRC_WIDTH], StartY+Debugger.FontHeight);

			BitBlt(Debugger.TraceWDC,0,0,WindowPoses[Debugger.DoubleSize][DW_TRC_WIDTH],WindowPoses[Debugger.DoubleSize][DW_TRC_HEIGHT],Debugger.TraceDC,0,0,SRCCOPY);

			Debugger_UpdateGraphics();		
		}
	}
}

void	Debugger_AddInst (void)
{
	if (Debugger.Logging)
	{
		unsigned short Addy = (unsigned short)CPU.PC;
		char out1[32], out2[40];
		ZeroMemory(Debugger.Out1, sizeof(Debugger.Out1));
		switch (TraceAddyMode[TraceMem(Addy)])
		{
		case IMP:sprintf(out1,"%04X  %02X        %s          ",		Addy,TraceMem(Addy),					TraceArr[TraceMem(Addy)]);						break;
		case ACC:sprintf(out1,"%04X  %02X        %s A        ",		Addy,TraceMem(Addy),					TraceArr[TraceMem(Addy)]);						break;
		case IMM:sprintf(out1,"%04X  %02X %02X     %s #$%02X     ",	Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],TraceMem(Addy+1));				break;
		case ABS:sprintf(out1,"%04X  %02X %02X %02X  %s $%04X    ",	Addy,TraceMem(Addy),TraceMem(Addy+1),TraceMem(Addy+2),	TraceArr[TraceMem(Addy)],TraceMem(Addy+1) | (TraceMem(Addy+2) << 8));	break;
		case IND:sprintf(out1,"%04X  %02X %02X %02X  %s ($%04X)  ",	Addy,TraceMem(Addy),TraceMem(Addy+1),TraceMem(Addy+2),	TraceArr[TraceMem(Addy)],TraceMem(Addy+1) | (TraceMem(Addy+2) << 8));	break;
		case REL:sprintf(out1,"%04X  %02X %02X     %s $%04X    ",	Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],Addy+2+(signed char)(TraceMem(Addy+1)));	break;
		case ABX:sprintf(out1,"%04X  %02X %02X %02X  %s $%04X,X  ",	Addy,TraceMem(Addy),TraceMem(Addy+1),TraceMem(Addy+2),	TraceArr[TraceMem(Addy)],TraceMem(Addy+1) | (TraceMem(Addy+2) << 8));	break;
		case ABY:sprintf(out1,"%04X  %02X %02X %02X  %s $%04X,Y  ",	Addy,TraceMem(Addy),TraceMem(Addy+1),TraceMem(Addy+2),	TraceArr[TraceMem(Addy)],TraceMem(Addy+1) | (TraceMem(Addy+2) << 8));	break;
		case ZPG:sprintf(out1,"%04X  %02X %02X     %s $%02X      ",	Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],TraceMem(Addy+1));				break;
		case ZPX:sprintf(out1,"%04X  %02X %02X     %s $%02X,X    ",	Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],TraceMem(Addy+1));				break;
		case ZPY:sprintf(out1,"%04X  %02X %02X     %s $%02X,Y    ",	Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],TraceMem(Addy+1));				break;
		case INX:sprintf(out1,"%04X  %02X %02X     %s ($%02X,X)  ",	Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],TraceMem(Addy+1));				break;
		case INY:sprintf(out1,"%04X  %02X %02X     %s ($%02X),Y  ",	Addy,TraceMem(Addy),TraceMem(Addy+1),			TraceArr[TraceMem(Addy)],TraceMem(Addy+1));				break;
		case ERR:sprintf(out1,"%04X  %02X        %s          ",		Addy,TraceMem(Addy),					TraceArr[TraceMem(Addy)]);						break;
		default : strcpy(out1,"");	break;
		};
		CPU_JoinFlags();
		sprintf(out2,"A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%i\n",CPU.A,CPU.X,CPU.Y,CPU.P,CPU.SP,PPU.Clockticks);
		DPrint(out1);
		DPrint(out2);
	}
}

void	Debugger_UpdateGraphics (void)
{
	char DInc = Debugger.Depth;
	long PatPtr;
	unsigned char PalVal;
	int MemAddy;
	if (Debugger.Mode >= 1)
	{
		//Palette
		if (Debugger.PalChanged)
		{
			int x, y, z;
			Debugger.PalChanged = FALSE;
			for (z = 0; z < 16; z++)
				for (x = 0; x < 16; x++)
					for (y = 0; y < 16; y++)
					{
						int BaseVal = ((y << 8) + ((z << 4) + x));
						memcpy(&Debugger.PaletteArray[BaseVal*Debugger.Depth],&GFX.FixedPalette[PPU.Palette[z]],DInc);
						memcpy(&Debugger.PaletteArray[(BaseVal+0x1000)*Debugger.Depth],&GFX.FixedPalette[PPU.Palette[0x10 | z]],DInc);
					}
		}

		SetBitmapBits(Debugger.PaletteBMP,DInc*256*32,&Debugger.PaletteArray);
		SelectObject(Debugger.PaletteDC,Debugger.PaletteBMP);
		BitBlt(Debugger.PaletteWDC,0,0,256,32,Debugger.PaletteDC,0,0,SRCCOPY);

		//Pattern Tables
		if (Debugger.PatChanged)
		{
			int i, t, x, y, sx, sy;
			PatPal[0] = PPU.Palette[0];
			for (i = 1; i < 4; i++)
				PatPal[i] = PPU.Palette[((Debugger.PatPalBase & 7) << 2) | i];

			Debugger.PatChanged = FALSE;
			for (t = 0; t < 2; t++)
			{
				for (y = 0; y < 16; y++)
				{
					for (x = 0; x < 16; x++)
					{
						MemAddy = (t << 12) | (y << 8) | (x << 4);
						for (sy = 0; sy < 8; sy++)
						{
							PatPtr = (((y << 3) + sy) << 8) + (t << 7) + (x << 3);
							for (sx = 0; sx < 8; sx++)
							{
								PalVal = (VMemory(MemAddy) & CharSC[sx]) >> (7-sx);
								PalVal |= ((VMemory(MemAddy+8) & CharSC[sx]) >> (7-sx)) << 1;
								memcpy(&Debugger.PatternArray[PatPtr*DInc],&GFX.FixedPalette[PatPal[PalVal]],DInc);
								PatPtr++;
							}
							MemAddy++;
						}
					}
				}
			}
		}
		SetBitmapBits(Debugger.PatternBMP,Debugger.Depth*256*128,&Debugger.PatternArray);
		SelectObject(Debugger.PatternDC,Debugger.PatternBMP);
		BitBlt(Debugger.PatternWDC,0,0,256,128,Debugger.PatternDC,0,0,SRCCOPY);
	}
	if (Debugger.Mode >= 2)
	{
		//Name Tables
		int DS = Debugger.DoubleSize;
		if (Debugger.NTabChanged)
		{
			int AttribTableVal, AttribNum;
			int i, NTy, NTx, TileY, TileX, py, px;
			Debugger.NTabChanged = FALSE;
			for (NTy = 0;NTy < 4; NTy += 2)
				for (NTx = 0; NTx < 2; NTx++)
					for (TileY = 0; TileY < 30; TileY++)
						for (TileX = 0; TileX < 32; TileX++)
						{
							AttribNum = (((TileX & 2) >> 1) | (TileY & 2)) << 1;
							AttribTableVal = (PPU.CHRPointer[8 | NTx | NTy][0x3C0 | ((TileY >> 2) << 3) | (TileX >> 2)] & (3 << AttribNum)) >> AttribNum;
							MemAddy = ((PPU.Reg2000 & 0x10) << 8) | (PPU.CHRPointer[8 | NTx | NTy][TileX | (TileY << 5)] << 4);
							for (py = 0; py < 8; py += 2-DS)
							{
								if (DS)
									PatPtr = ((NTy << 8)*240) + (TileY << 12) | (py << 9) | (NTx << 8) | (TileX << 3);
								else	PatPtr = ((((NTy >> 1) * 120)+(TileY << 2)) << 8) + (py << 7) + (NTx << 7) + (TileX << 2);
								
								for (px = 0; px < 8; px += 2-DS)
								{
									PalVal = (VMemory(MemAddy) & CharSC[px]) >> (7-px);
									PalVal |= ((VMemory(MemAddy+8) & CharSC[px]) >> (7-px)) << 1;

									if (PalVal)
									{
										PalVal |= (AttribTableVal << 2);
										memcpy(&Debugger.NameArray[PatPtr*DInc],&GFX.FixedPalette[PPU.Palette[PalVal]],DInc);
									}
									else	memcpy(&Debugger.NameArray[PatPtr*DInc],&GFX.FixedPalette[PPU.Palette[0]],DInc);
									PatPtr++;
								}
								MemAddy++;
							}
						}
			for (i = 0; i < (DS+1)*256; i++)
				memcpy(&Debugger.NameArray[(((120*(DS+1))<<(8+DS))+i)*DInc],&GFX.FixedPalette[0x41],DInc);
			for (i = 0; i < (DS+1)*240; i++)
				memcpy(&Debugger.NameArray[(i*(DS+1)*256+(128*(DS+1)))*DInc],&GFX.FixedPalette[0x41],DInc);
		}

		SetBitmapBits(Debugger.NameBMP,DInc*(256*(DS+1))*(240*(DS+1)),&Debugger.NameArray);
		SelectObject(Debugger.NameDC,Debugger.NameBMP);
		BitBlt(Debugger.NameWDC,0,0,256*(DS+1),240*(DS+1),Debugger.NameDC,0,0,SRCCOPY);
	}
}

BOOL CALLBACK PaletteProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		BitBlt(Debugger.PaletteWDC,0,0,256,32,Debugger.PaletteDC,0,0,SRCCOPY);
		break;
	}
	return FALSE;
}

BOOL CALLBACK PatternProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		BitBlt(Debugger.PatternWDC,0,0,256,128,Debugger.PatternDC,0,0,SRCCOPY);
		break;
	case WM_LBUTTONUP:
		Debugger.PatPalBase = (Debugger.PatPalBase + 1) & 7;
		Debugger.PatChanged = TRUE;
		Debugger_UpdateGraphics();
		break;
	}
	return FALSE;
}

BOOL CALLBACK NameProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		BitBlt(Debugger.NameWDC,0,0,256*(Debugger.DoubleSize+1),240*(Debugger.DoubleSize+1),Debugger.NameDC,0,0,SRCCOPY);
		break;
	}
	return FALSE;
}

BOOL CALLBACK DumpProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	switch (uMsg)
	{
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDC_DUMP_PPU_MEM:
					{
						FILE *PPUMemOut = fopen("PPU.Mem","wb");
						int i;
						for (i = 0; i < 12; i++)
							fwrite(PPU.CHRPointer[i],1,0x400,PPUMemOut);
						fwrite(PPU.Palette,1,0x20,PPUMemOut);
						fclose(PPUMemOut);
					}
					break;
				case IDC_START_LOGGING:
					Debugger_StartLogging();
					break;
				case IDC_STOP_LOGGING:
					Debugger_StopLogging();
					break;
				case IDC_IRQBUTTON:
					CPU.WantIRQ = TRUE;
					break;
/*				case IDC_DUMP_FLAGS:
					{
						FILE *ROMFlagsOut = fopen("ROM.flg","wb");
						fwrite(CPU_Flags,1,0x10000,ROMFlagsOut);
						fclose(ROMFlagsOut);
					}
					break;*/
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK TraceProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
			BitBlt(Debugger.TraceWDC,0,0,WindowPoses[Debugger.DoubleSize][DW_TRC_WIDTH],WindowPoses[Debugger.DoubleSize][DW_TRC_HEIGHT],Debugger.TraceDC,0,0,SRCCOPY);
			break;
		case WM_KEYDOWN:
			switch (wParam)
			{
				case VK_HOME:		//HAX0R!!!!
					if (Debugger.TraceOffset == -1)
						Debugger.TraceOffset = CPU.PC - 0x1000;
					else	Debugger.TraceOffset -= 0x1000;
					Debugger_Update();
					break;
				case VK_END:		//HAX0R!!!!
					if (Debugger.TraceOffset == -1)
						Debugger.TraceOffset = CPU.PC + 0x1000;
					else	Debugger.TraceOffset += 0x1000;
					Debugger_Update();
					break;
				case VK_PRIOR:
					{
						unsigned short NewAddy = Debugger.AddyLine[0];
						if (NewAddy == CPU.PC)
							NewAddy = -1;
						Debugger.TraceOffset = NewAddy;
						Debugger_Update();
					}
					break;
				case VK_UP:
					{
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
						NewAddy = Debugger.AddyLine[(MaxAddy/2) - 1];
						if (NewAddy == CPU.PC)
							Debugger.TraceOffset = -1;
						else	Debugger.TraceOffset = NewAddy;
						Debugger_Update();
					}
					break;
				case VK_DOWN:
					{
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
					}
					break;
				case VK_NEXT:
					{
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
					}
					break;
			
			}
			break;
		case WM_LBUTTONDOWN:
			{
				short xPos = LOWORD(lParam);
				short yPos = HIWORD(lParam);
				if ((xPos > 0) && (xPos < WindowPoses[Debugger.DoubleSize][DW_TRC_WIDTH]) && (yPos > 0) && (yPos < WindowPoses[Debugger.DoubleSize][DW_TRC_HEIGHT]))
				{
					Debugger.BreakP[Debugger.AddyLine[-1 + ((yPos-2)/Debugger.FontHeight)]] ^= TRUE;
					Debugger_Update();
					SetFocus(mWnd);
				}
			}
			break;
	}
	
	return FALSE;
}

BOOL CALLBACK RegProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		BitBlt(Debugger.RegWDC,0,0,WindowPoses[Debugger.DoubleSize][DW_REG_WIDTH],WindowPoses[Debugger.DoubleSize][DW_REG_HEIGHT],Debugger.RegDC,0,0,SRCCOPY);
		break;
	}
	return FALSE;
}