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
#include "PPU.h"
#include "CPU.h"
#include "GFX.h"
#include "Debugger.h"

struct	tPPU	PPU;
unsigned char	PPU_VRAM[0x4][0x400];

const	unsigned char	ReverseCHR[256] =
{
	0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
	0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
	0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
	0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
	0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
	0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
	0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
	0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
	0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
	0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
	0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
	0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
	0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
	0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
	0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
	0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF
};

const unsigned long	CHRLoBit[16] =
{
	0x00000000,0x00000001,0x00000100,0x00000101,0x00010000,0x00010001,0x00010100,0x00010101,
	0x01000000,0x01000001,0x01000100,0x01000101,0x01010000,0x01010001,0x01010100,0x01010101
};
const unsigned long	CHRHiBit[16] =
{
	0x00000000,0x00000002,0x00000200,0x00000202,0x00020000,0x00020002,0x00020200,0x00020202,
	0x02000000,0x02000002,0x02000200,0x02000202,0x02020000,0x02020002,0x02020200,0x02020202
};
const unsigned long	AttribBits[4] =
{
	0x00000000,0x04040404,0x08080808,0x0C0C0C0C
};

const unsigned long	AttribShift[0x80] =
{
	0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,
	4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6
};

static	void	(__cdecl *PPU_HBlank)		(int,int);
static	int	(__cdecl *PPU_TileHandler)	(int,int,int);

static	void	__cdecl	PPU_NoHBlank		(int Scanline, int Byte2001)	{ }
static	int	__cdecl	PPU_NoTileHandler	(int Tile, int Bank, int Index)	{ return 0; }

void	PPU_Init (void)
{
}
void	PPU_GetHandlers (void)
{
	if ((MI) && (MI->HBlank))
		PPU_HBlank = MI->HBlank;
	else	PPU_HBlank = PPU_NoHBlank;
	if ((MI) && (MI->TileHandler))
		PPU_TileHandler = MI->TileHandler;
	else	PPU_TileHandler = PPU_NoTileHandler;
}
void	PPU_SetMirroring (unsigned char M1, unsigned char M2, unsigned char M3, unsigned char M4)
{
	PPU.CHRPointer[0x8] = PPU_VRAM[M1];	PPU.CHRPointer[0x9] = PPU_VRAM[M2];
	PPU.CHRPointer[0xA] = PPU_VRAM[M3];	PPU.CHRPointer[0xB] = PPU_VRAM[M4];
	PPU.Writable[0x8] = PPU.Writable[0x9] = PPU.Writable[0xA] = PPU.Writable[0xB] = TRUE;
	Debugger.NTabChanged = TRUE;
}
__inline static	void	MemSet (unsigned long Addy, unsigned char Val)
{
	if (Addy & 0x2000)
	{
		if ((Addy & 0x3F00) == 0x3F00)
		{
			Val &= 0x3F;
			Debugger.PalChanged = TRUE;
			if (Addy & 0x3)
				PPU.Palette[Addy & 0x1F] = Val;
			else	PPU.Palette[Addy & 0x0F] = PPU.Palette[0x10 | (Addy & 0xF)] = Val;
		}
		else
		{
			if (!PPU.Writable[8 | ((Addy >> 10) & 0x3)])
				return;
			Debugger.NTabChanged = TRUE;
			PPU.CHRPointer[8 | ((Addy >> 10) & 0x3)][Addy & 0x3FF] = Val;
		}
	}
	else
	{
		if (!PPU.Writable[(Addy >> 10) & 0x7])
			return;
		Debugger.PatChanged = TRUE;
		PPU.CHRPointer[(Addy >> 10) & 0x7][Addy & 0x3FF] = Val;
	}
}
__inline static	unsigned char MemGet (unsigned long Addy)
{
	if (Addy & 0x2000)
		return PPU.CHRPointer[8 | ((Addy >> 10) & 0x3)][Addy & 0x3FF];
	else	return PPU.CHRPointer[(Addy >> 10) & 0x7][Addy & 0x3FF];
}
__inline static	unsigned char	VMemory (unsigned long Addy) { return PPU.CHRPointer[(Addy >> 10) & 0x7][Addy & 0x3FF]; }
__inline static	void	DiscoverSprites (void)
{
	int SprHeight = (PPU.Reg2000 & 0x20) ? 16 : 8;
	int SL = PPU.SLnum;
	int spt;
	PPU.SprCount = 0;
	PPU.Spr0InLine = FALSE;
	for (spt = 0; spt < 256; spt += 4)
	{
		if ((SL >= PPU.Sprite[spt]) && (SL < (PPU.Sprite[spt] + SprHeight)))
		{
			if (PPU.SprCount == 32)
			{
				PPU.Reg2002 |= 0x20;
				break;
			}
			PPU.SprBuff[PPU.SprCount++] = SL - PPU.Sprite[spt];
			PPU.SprBuff[PPU.SprCount++] = PPU.Sprite[spt | 1];
			PPU.SprBuff[PPU.SprCount++] = PPU.Sprite[spt | 2];
			PPU.SprBuff[PPU.SprCount++] = PPU.Sprite[spt | 3];
			if (!spt)
				PPU.Spr0InLine = TRUE;
		}
	}
}
void	PPU_GetGFXPtr (void)
{
	PPU.GfxData = (unsigned char *)GFX.DrawArray + PPU.SLnum*GFX.Pitch;
}

void	PPU_PowerOn()
{
	PPU.Reg2000 = 0;
	PPU.Reg2001 = 0;
	PPU.VRAMAddr = PPU.IntReg = 0;
	PPU.SLnum = 0;
	PPU.IntX = PPU.IntXBak = 0;
	PPU.ppuLatch = 0;
	PPU.Reg2002 = 0;
	PPU.HVTog = TRUE;
	PPU.ShortSL = TRUE;
	ZeroMemory(PPU_VRAM,sizeof(PPU_VRAM));
	ZeroMemory(PPU.Palette,sizeof(PPU.Palette));
	ZeroMemory(PPU.Sprite,sizeof(PPU.Sprite));
	PPU_GetHandlers();
}

static	void	RunNoSkip (int NumTicks)
{
	static unsigned char TileNum;
	static unsigned long PatAddr;
	register unsigned long TL;
	register unsigned char TC;

	register int SprNum;
	register unsigned char *CurTileData;

	int i, y;
	for (i = 0; i < NumTicks; i++)
	{
		PPU.Clockticks++;
		switch (PPU.Clockticks)
		{
		case 0:	PPU.SLnum++;
			if (PPU.SLnum < 240)
				PPU_GetGFXPtr();
			NES.Scanline = TRUE;
			if (PPU.SLnum == 241)
			{
				if (PPU.Reg2000 & 0x80)
					CPU.WantNMI = TRUE;
			}
			else if (PPU.SLnum == PPU.SLEndFrame - 1)
			{
				PPU.Reg2002 &= 0x60;		// Clear VBlank flag
				PPU.ShortSL = !PPU.ShortSL;
				PPU.SLnum = -1;
				GFX_DrawScreen();
			}
			else if (PPU.SLnum >= PPU.SLEndFrame)
			{
				MessageBox(mWnd,"WTF?","Nintendulator",MB_OK);
			}
			break;
			// BEGIN BACKGROUND
		case   1:	case   9:	case  17:	case  25:	case  33:	case  41:	case  49:	case  57:
		case  65:	case  73:	case  81:	case  89:	case  97:	case 105:	case 113:	case 121:
		case 129:	case 137:	case 145:	case 153:	case 161:	case 169:	case 177:	case 185:
		case 193:	case 201:	case 209:	case 217:	case 225:	case 233:	case 241:	case 249:
			if ((PPU.SLnum < 0) || (PPU.SLnum >= 240) || !(PPU.Reg2001 & 0x18))
				break;
			TileNum = PPU.CHRPointer[8 | ((PPU.VRAMAddr >> 10) & 0x3)][PPU.VRAMAddr & 0x3FF];
			PatAddr = (TileNum << 4) | (PPU.VRAMAddr >> 12) | ((PPU.Reg2000 & 0x10) << 8);
			PPU_TileHandler((PatAddr >> 10) & 0x4,TileNum,PPU.VRAMAddr & 0x3FF);
			break;
		case   3:	case  11:	case  19:	case  27:	case  35:	case  43:	case  51:	case  59:
		case  67:	case  75:	case  83:	case  91:	case  99:	case 107:	case 115:	case 123:
		case 131:	case 139:	case 147:	case 155:	case 163:	case 171:	case 179:	case 187:
		case 195:	case 203:	case 211:	case 219:	case 227:	case 235:	case 243:
			if ((PPU.SLnum < 0) || (PPU.SLnum >= 240) || !(PPU.Reg2001 & 0x18))
				break;
			if (PPU.Reg2001 & 0x08)
			{
				CurTileData = &PPU.TileData[PPU.Clockticks + 13];
				TL = AttribBits[(PPU.CHRPointer[8 | ((PPU.VRAMAddr >> 10) & 0x3)][0x3C0 | ((PPU.VRAMAddr >> 2) & 0x7) | ((PPU.VRAMAddr >> 4) & 0x38)] >> AttribShift[PPU.VRAMAddr & 0x7F]) & 3];
				((unsigned long *)CurTileData)[0] = TL;
				((unsigned long *)CurTileData)[1] = TL;
			}
			if ((PPU.VRAMAddr & 0x1F) == 0x1F)
				PPU.VRAMAddr ^= 0x41F;
			else	PPU.VRAMAddr++;
			break;
		case 250:
			if ((PPU.SLnum < 0) || (PPU.SLnum >= 240) || !(PPU.Reg2001 & 0x18))
				break;
			if (PPU.Reg2001 & 0x08)
			{
				CurTileData = &PPU.TileData[PPU.Clockticks + 13];
				TL = AttribBits[(PPU.CHRPointer[8 | ((PPU.VRAMAddr >> 10) & 0x3)][0x3C0 | ((PPU.VRAMAddr >> 2) & 0x7) | ((PPU.VRAMAddr >> 4) & 0x38)] >> AttribShift[PPU.VRAMAddr & 0x7F]) & 3];
				((unsigned long *)CurTileData)[0] = TL;
				((unsigned long *)CurTileData)[1] = TL;
			}
			PPU.IntX = PPU.IntXBak;
			PPU.VRAMAddr &= ~0x41F;
			PPU.VRAMAddr |= PPU.IntReg & 0x41F;
			if ((PPU.VRAMAddr & 0x7000) == 0x7000)
			{
				register int YScroll = PPU.VRAMAddr & 0x3E0;
				PPU.VRAMAddr &= 0xFFF;
				if (YScroll == 0x3A0)
					PPU.VRAMAddr ^= 0xBA0;
				else if (YScroll == 0x3E0)
					PPU.VRAMAddr ^= 0x3E0;
				else	PPU.VRAMAddr += 0x20;
			}
			else	PPU.VRAMAddr += 0x1000;
			break;
		case   5:	case  13:	case  21:	case  29:	case  37:	case  45:	case  53:	case  61:
		case  69:	case  77:	case  85:	case  93:	case 101:	case 109:	case 117:	case 125:
		case 133:	case 141:	case 149:	case 157:	case 165:	case 173:	case 181:	case 189:
		case 197:	case 205:	case 213:	case 221:	case 229:	case 237:	case 245:	case 253:
			if ((PPU.SLnum < 0) || (PPU.SLnum >= 240) || !(PPU.Reg2001 & 0x08))
				break;
			CurTileData = &PPU.TileData[PPU.Clockticks + 11];
			TC = ReverseCHR[VMemory(PatAddr)];
			((unsigned long *)CurTileData)[0] |= CHRLoBit[TC & 0xF];
			((unsigned long *)CurTileData)[1] |= CHRLoBit[TC >> 4];
			break;
		case   7:	case  15:	case  23:	case  31:	case  39:	case  47:	case  55:	case  63:
		case  71:	case  79:	case  87:	case  95:	case 103:	case 111:	case 119:	case 127:
		case 135:	case 143:	case 151:	case 159:	case 167:	case 175:	case 183:	case 191:
		case 199:	case 207:	case 215:	case 223:	case 231:	case 239:	case 247:	case 255:
			if ((PPU.SLnum < 0) || (PPU.SLnum >= 240) || !(PPU.Reg2001 & 0x08))
				break;
			CurTileData = &PPU.TileData[PPU.Clockticks + 9];
			TC = ReverseCHR[VMemory(PatAddr | 8)];
			((unsigned long *)CurTileData)[0] |= CHRHiBit[TC & 0xF];
			((unsigned long *)CurTileData)[1] |= CHRHiBit[TC >> 4];
			break;
			// END BACKGROUND
		case 256:
			if (PPU.SLnum < 240)
			{
				DiscoverSprites();
				ZeroMemory(PPU.TileData,sizeof(PPU.TileData));
			}
			if (PPU.SLnum == -1)
			{
				PPU.Reg2002 = 0;
				PPU.SprAddr = 0;
				PPU.IntX = PPU.IntXBak;
				if (PPU.Reg2001 & 0x18)
					PPU.VRAMAddr = PPU.IntReg;
			}
			break;
			// BEGIN SPRITES
		case 257:	case 265:	case 273:	case 281:	case 289:	case 297:	case 305:	case 313:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x18))
				break;
			TileNum = PPU.SprBuff[((PPU.Clockticks >> 1) & 0x1C) | 1];
			break;
		case 259:	case 267:	case 275:	case 283:	case 291:	case 299:	case 307:	case 315:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x18))
				break;
			SprNum = (PPU.Clockticks >> 1) & 0x1C;
			if (PPU.Reg2000 & 0x20)
				PatAddr = ((TileNum & 0xFE) << 4) | ((TileNum & 0x01) << 12) | ((PPU.SprBuff[SprNum] & 7) ^ ((PPU.SprBuff[SprNum | 2] & 0x80) ? 0x17 : 0x00) ^ ((PPU.SprBuff[SprNum] & 8) ? 0x10 : 0x00));
			else	PatAddr = (TileNum << 4) | (PPU.SprBuff[SprNum] ^ ((PPU.SprBuff[SprNum | 2] & 0x80) ? 0x7 : 0x0)) | ((PPU.Reg2000 & 0x08) << 9);
			PPU_TileHandler((PatAddr >> 10) & 0x4,TileNum,-1);
			break;
		case 260:
			PPU_HBlank(PPU.SLnum,PPU.Reg2001);
			break;
		case 261:	case 269:	case 277:	case 285:	case 293:	case 301:	case 309:	case 317:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x18))
				break;
			SprNum = (PPU.Clockticks >> 1) & 0x1C;
			TC = VMemory(PatAddr);
			if (!(PPU.SprBuff[SprNum | 2] & 0x40))
				TC = ReverseCHR[TC];
			CurTileData = PPU.SprData[SprNum >> 2];
			TL = AttribBits[PPU.SprBuff[SprNum | 2] & 0x3];
			((unsigned long *)CurTileData)[0] = CHRLoBit[TC & 0xF] | TL;
			((unsigned long *)CurTileData)[1] = CHRLoBit[TC >> 4] | TL;
			break;
		case 263:	case 271:	case 279:	case 287:	case 295:	case 303:	case 311:	case 319:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x18))
				break;
			SprNum = (PPU.Clockticks >> 1) & 0x1C;
			TC = VMemory(PatAddr | 8);
			if (!(PPU.SprBuff[SprNum | 2] & 0x40))
				TC = ReverseCHR[TC];
			CurTileData = PPU.SprData[SprNum >> 2];
			((unsigned long *)CurTileData)[0] |= CHRHiBit[TC & 0xF];
			((unsigned long *)CurTileData)[1] |= CHRHiBit[TC >> 4];
			break;
			// END SPRITES
			// BEGIN BACKGROUND
		case 321:	case 329:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x18))
				break;
			TileNum = PPU.CHRPointer[8 | ((PPU.VRAMAddr >> 10) & 0x3)][PPU.VRAMAddr & 0x3FF];
			PatAddr = (TileNum << 4) | (PPU.VRAMAddr >> 12) | ((PPU.Reg2000 & 0x10) << 8);
			PPU_TileHandler((PatAddr >> 10) & 0x4,TileNum,PPU.VRAMAddr & 0x3FF);
			break;
		case 323:	case 331:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x18))
				break;
			if (PPU.Reg2001 & 0x08)
			{
				CurTileData = &PPU.TileData[PPU.Clockticks - 323];
				TL = AttribBits[(PPU.CHRPointer[8 | ((PPU.VRAMAddr >> 10) & 0x3)][0x3C0 | ((PPU.VRAMAddr >> 2) & 0x7) | ((PPU.VRAMAddr >> 4) & 0x38)] >> AttribShift[PPU.VRAMAddr & 0x7F]) & 3];
				((unsigned long *)CurTileData)[0] = TL;
				((unsigned long *)CurTileData)[1] = TL;
			}
			if ((PPU.VRAMAddr & 0x1F) == 0x1F)
				PPU.VRAMAddr ^= 0x41F;
			else	PPU.VRAMAddr++;
			break;
		case 325:	case 333:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x08))
				break;
			CurTileData = &PPU.TileData[PPU.Clockticks - 325];
			TC = ReverseCHR[VMemory(PatAddr)];
			((unsigned long *)CurTileData)[0] |= CHRLoBit[TC & 0xF];
			((unsigned long *)CurTileData)[1] |= CHRLoBit[TC >> 4];
			break;
		case 327:	case 335:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x08))
				break;
			CurTileData = &PPU.TileData[PPU.Clockticks - 327];
			TC = ReverseCHR[VMemory(PatAddr | 8)];
			((unsigned long *)CurTileData)[0] |= CHRHiBit[TC & 0xF];
			((unsigned long *)CurTileData)[1] |= CHRHiBit[TC >> 4];
			if (!(PPU.Reg2001 & 0x02))
			{
				int BgLoc = PPU.Clockticks + PPU.IntX - 335;
				if (BgLoc < -4)
				{
					((unsigned long *)CurTileData)[0] = 0;
					((unsigned long *)CurTileData)[1] = 0;
				}
				else if (BgLoc < 0)
					((unsigned long *)CurTileData)[1] = 0;
				else if (BgLoc < 4)
					((unsigned long *)CurTileData)[0] &= 0xFFFFFFFF << (BgLoc << 3);
				else
				{
					((unsigned long *)CurTileData)[0] = 0;
					((unsigned long *)CurTileData)[1] &= 0xFFFFFFFF << ((BgLoc & 3) << 3);
				}
			}
			break;
			// END BACKGROUND
		case 336:
			if (PPU.SLnum == 240)
				PPU.Reg2002 |= 0x80;
			break;
		case 339:
			if ((PPU.SLnum != -1) || (!PPU.ShortSL))
				break;
			// else fall through
		case 340:
			PPU.Clockticks = -1;
			continue;
		}
		if ((PPU.SLnum >= 0) && (PPU.SLnum < 240) && (PPU.Clockticks < 256))
		{
			int PalIndex;
			TC = PPU.TileData[PPU.Clockticks + PPU.IntX];
			if ((PPU.Reg2001 & 0x10) && ((PPU.Reg2001 & 0x04) || (PPU.Clockticks >= 8)))
				for (y = 0; y < PPU.SprCount; y += 4)
				{
					register int SprPixel = PPU.Clockticks - PPU.SprBuff[y | 3];
					unsigned char SprFlags, SprDat;
					if ((SprPixel < 0) || (SprPixel > 7))
						continue;
					SprFlags = PPU.SprBuff[y | 2];
					SprDat = PPU.SprData[y >> 2][SprPixel];
					if (SprDat & 0x3)
					{
						if ((y == 0) && (PPU.Spr0InLine) && (TC & 0x3))
							PPU.Reg2002 |= 0x40;	// Sprite 0 hit
						if ((!(TC & 0x3)) || (!(SprFlags & 0x20)))
							TC = SprDat | 0x10;
						break;
					}
				}
			if (TC & 0x3)
				PalIndex = PPU.Palette[TC & 0x1F];
			else	PalIndex = PPU.Palette[0];
			if (PPU.Reg2001 & 0x01)
				PalIndex &= 0x30;
			PalIndex |= (PPU.Reg2001 & 0xE0) << 1;
			if (GFX.Depth == 32)
				((unsigned long *)PPU.GfxData)[PPU.Clockticks] = GFX.FixedPalette[PalIndex];
			else	((unsigned short *)PPU.GfxData)[PPU.Clockticks] = (unsigned short)GFX.FixedPalette[PalIndex];
		}
	}
}

static	void	RunSkip (int NumTicks)
{
	static unsigned char TileNum;
	static unsigned long PatAddr;

	register unsigned char TC;
	register int SprNum;
	register unsigned char *CurTileData;
	int i;

	for (i = 0; i < NumTicks; i++)
	{
		PPU.Clockticks++;
		switch (PPU.Clockticks)
		{
		case 0:	PPU.SLnum++;
			NES.Scanline = TRUE;
			if (PPU.SLnum == 241)
			{
				if (PPU.Reg2000 & 0x80)
					CPU.WantNMI = TRUE;
			}
			else if (PPU.SLnum == PPU.SLEndFrame - 1)
			{
				PPU.Reg2002 &= 0x60;
				PPU.ShortSL = !PPU.ShortSL;
				PPU.SLnum = -1;
				GFX_DrawScreen();
			}
			break;
			// BEGIN BACKGROUND
		case   1:	case   9:	case  17:	case  25:	case  33:	case  41:	case  49:	case  57:
		case  65:	case  73:	case  81:	case  89:	case  97:	case 105:	case 113:	case 121:
		case 129:	case 137:	case 145:	case 153:	case 161:	case 169:	case 177:	case 185:
		case 193:	case 201:	case 209:	case 217:	case 225:	case 233:	case 241:	case 249:
			if ((PPU.SLnum < 0) || (PPU.SLnum >= 240) || !(PPU.Reg2001 & 0x18))
				break;
			TileNum = PPU.CHRPointer[8 | ((PPU.VRAMAddr >> 10) & 0x3)][PPU.VRAMAddr & 0x3FF];
			PatAddr = (TileNum << 4) | (PPU.VRAMAddr >> 12) | ((PPU.Reg2000 & 0x10) << 8);
			PPU_TileHandler((PatAddr >> 10) & 0x4,TileNum,PPU.VRAMAddr & 0x3FF);
			break;
		case   3:	case  11:	case  19:	case  27:	case  35:	case  43:	case  51:	case  59:
		case  67:	case  75:	case  83:	case  91:	case  99:	case 107:	case 115:	case 123:
		case 131:	case 139:	case 147:	case 155:	case 163:	case 171:	case 179:	case 187:
		case 195:	case 203:	case 211:	case 219:	case 227:	case 235:	case 243:
			if ((PPU.SLnum < 0) || (PPU.SLnum >= 240) || !(PPU.Reg2001 & 0x18))
				break;
			if ((PPU.VRAMAddr & 0x1F) == 0x1F)
				PPU.VRAMAddr ^= 0x41F;
			else	PPU.VRAMAddr++;
			break;
		case 250:
			if ((PPU.SLnum < 0) || (PPU.SLnum >= 240) || !(PPU.Reg2001 & 0x18))
				break;
			PPU.IntX = PPU.IntXBak;
			PPU.VRAMAddr &= ~0x41F;
			PPU.VRAMAddr |= PPU.IntReg & 0x41F;
			if ((PPU.VRAMAddr & 0x7000) == 0x7000)
			{
				int YScroll = PPU.VRAMAddr & 0x3E0;
				PPU.VRAMAddr &= 0xFFF;
				if (YScroll == 0x3A0)
					PPU.VRAMAddr ^= 0xBA0;
				else if (YScroll == 0x3E0)
					PPU.VRAMAddr ^= 0x3E0;
				else	PPU.VRAMAddr += 0x20;
			}
			else	PPU.VRAMAddr += 0x1000;
			break;
		case   5:	case  13:	case  21:	case  29:	case  37:	case  45:	case  53:	case  61:
		case  69:	case  77:	case  85:	case  93:	case 101:	case 109:	case 117:	case 125:
		case 133:	case 141:	case 149:	case 157:	case 165:	case 173:	case 181:	case 189:
		case 197:	case 205:	case 213:	case 221:	case 229:	case 237:	case 245:	case 253:
			if ((PPU.SLnum < 0) || (PPU.SLnum >= 240) || !(PPU.Reg2001 & 0x08) || (!PPU.Spr0InLine))
				break;
			CurTileData = &PPU.TileData[PPU.Clockticks + 11];
			TC = ReverseCHR[VMemory(PatAddr)];
			((unsigned long *)CurTileData)[0] |= CHRLoBit[TC & 0xF];
			((unsigned long *)CurTileData)[1] |= CHRLoBit[TC >> 4];
			break;
		case   7:	case  15:	case  23:	case  31:	case  39:	case  47:	case  55:	case  63:
		case  71:	case  79:	case  87:	case  95:	case 103:	case 111:	case 119:	case 127:
		case 135:	case 143:	case 151:	case 159:	case 167:	case 175:	case 183:	case 191:
		case 199:	case 207:	case 215:	case 223:	case 231:	case 239:	case 247:	case 255:
			if ((PPU.SLnum < 0) || (PPU.SLnum >= 240) || !(PPU.Reg2001 & 0x08) || (!PPU.Spr0InLine))
				break;
			CurTileData = &PPU.TileData[PPU.Clockticks + 9];
			TC = ReverseCHR[VMemory(PatAddr | 8)];
			((unsigned long *)CurTileData)[0] |= CHRHiBit[TC & 0xF];
			((unsigned long *)CurTileData)[1] |= CHRHiBit[TC >> 4];
			break;
			// END BACKGROUND
		case 256:
			if (PPU.SLnum < 240)
			{
				DiscoverSprites();
				if (PPU.Spr0InLine)
					ZeroMemory(PPU.TileData,sizeof(PPU.TileData));
			}
			if (PPU.SLnum == -1)
			{
				PPU.Reg2002 = 0;
				PPU.SprAddr = 0;
				PPU.IntX = PPU.IntXBak;
				if (PPU.Reg2001 & 0x18)
					PPU.VRAMAddr = PPU.IntReg;
			}
			break;
			// BEGIN SPRITES
		case 257:	case 265:	case 273:	case 281:	case 289:	case 297:	case 305:	case 313:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x18))
				break;
			TileNum = PPU.SprBuff[((PPU.Clockticks >> 1) & 0x1C) | 1];
			break;
		case 259:	case 267:	case 275:	case 283:	case 291:	case 299:	case 307:	case 315:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x18))
				break;
			SprNum = (PPU.Clockticks >> 1) & 0x1C;
			if (PPU.Reg2000 & 0x20)
				PatAddr = ((TileNum & 0xFE) << 4) | ((TileNum & 0x01) << 12) | ((PPU.SprBuff[SprNum] & 7) ^ ((PPU.SprBuff[SprNum | 2] & 0x80) ? 0x17 : 0x00) ^ ((PPU.SprBuff[SprNum] & 8) ? 0x10 : 0x00));
			else	PatAddr = (TileNum << 4) | (PPU.SprBuff[SprNum] ^ ((PPU.SprBuff[SprNum | 2] & 0x80) ? 0x7 : 0x0)) | ((PPU.Reg2000 & 0x08) << 9);
			PPU_TileHandler((PatAddr >> 10) & 0x4,TileNum,-1);
			break;
		case 260:
			PPU_HBlank(PPU.SLnum,PPU.Reg2001);
			break;
		case 261:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x18) || (!PPU.Spr0InLine))
				break;
			TC = VMemory(PatAddr);
			if (!(PPU.SprBuff[2] & 0x40))
				TC = ReverseCHR[TC];
			((unsigned long *)(PPU.SprData[0]))[0] = CHRLoBit[TC & 0xF];
			((unsigned long *)(PPU.SprData[0]))[1] = CHRLoBit[TC >> 4];
			break;
		case 263:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x18) || (!PPU.Spr0InLine))
				break;
			TC = VMemory(PatAddr | 8);
			if (!(PPU.SprBuff[2] & 0x40))
				TC = ReverseCHR[TC];
			((unsigned long *)(PPU.SprData[0]))[0] |= CHRHiBit[TC & 0xF];
			((unsigned long *)(PPU.SprData[0]))[1] |= CHRHiBit[TC >> 4];
			break;
			// END SPRITES
			// BEGIN BACKGROUND
		case 321:	case 329:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x18))
				break;
			TileNum = PPU.CHRPointer[8 | ((PPU.VRAMAddr >> 10) & 0x3)][PPU.VRAMAddr & 0x3FF];
			PatAddr = (TileNum << 4) | (PPU.VRAMAddr >> 12) | ((PPU.Reg2000 & 0x10) << 8);
			PPU_TileHandler((PatAddr & 0x1000) >> 10, TileNum, PPU.VRAMAddr & 0x3FF);
			break;
		case 323:	case 331:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x18))
				break;
			if ((PPU.VRAMAddr & 0x1F) == 0x1F)
				PPU.VRAMAddr ^= 0x41F;
			else	PPU.VRAMAddr++;
			break;
		case 325:	case 333:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x08) || (!PPU.Spr0InLine))
				break;
			CurTileData = &PPU.TileData[PPU.Clockticks - 325];
			TC = ReverseCHR[VMemory(PatAddr)];
			((unsigned long *)CurTileData)[0] |= CHRLoBit[TC & 0xF];
			((unsigned long *)CurTileData)[1] |= CHRLoBit[TC >> 4];
			break;
		case 327:	case 335:
			if ((PPU.SLnum >= 239) || !(PPU.Reg2001 & 0x08) || (!PPU.Spr0InLine))
				break;
			CurTileData = &PPU.TileData[PPU.Clockticks - 327];
			TC = ReverseCHR[VMemory(PatAddr | 8)];
			((unsigned long *)CurTileData)[0] |= CHRHiBit[TC & 0xF];
			((unsigned long *)CurTileData)[1] |= CHRHiBit[TC >> 4];
			if (!(PPU.Reg2001 & 0x02))
			{
				int BgLoc = PPU.Clockticks + PPU.IntX - 335;
				if (BgLoc < -4)
				{
					((unsigned long *)CurTileData)[0] = 0;
					((unsigned long *)CurTileData)[1] = 0;
				}
				else if (BgLoc < 0)
					((unsigned long *)CurTileData)[1] = 0;
				else if (BgLoc < 4)
					((unsigned long *)CurTileData)[0] &= 0xFFFFFFFF << (BgLoc << 3);
				else
				{
					((unsigned long *)CurTileData)[0] = 0;
					((unsigned long *)CurTileData)[1] &= 0xFFFFFFFF << ((BgLoc & 3) << 3);
				}
			}
			break;
			// END BACKGROUND
		case 336:
			if (PPU.SLnum == 240)
				PPU.Reg2002 |= 0x80;
			break;
		case 339:
			if ((PPU.SLnum != -1) || (!PPU.ShortSL))
				break;
			// else fall through
		case 340:
			PPU.Clockticks = -1;
			continue;
		}
		if ((PPU.Spr0InLine) && (PPU.SLnum >= 0) && (PPU.SLnum < 240) && (PPU.Clockticks < 256))
		{
			if ((PPU.Reg2001 & 0x10) && ((PPU.Reg2001 & 0x04) || (PPU.Clockticks >= 8)))
			{
				int SprPixel = PPU.Clockticks - PPU.SprBuff[3];
				if ((SprPixel < 0) || (SprPixel > 7))
					break;
				if ((PPU.SprData[0][SprPixel] & 3) && (PPU.TileData[PPU.Clockticks + PPU.IntX] & 0x3))
					PPU.Reg2002 |= 0x40;	// Sprite 0 hit
			}
		}
	}
}
void	PPU_Run (void)
{
	register int Cycles = 3;
	if (PPU.IsPAL)
	{
		static int overflow = 0;
		overflow++;
		if (overflow == 5)
		{
			Cycles++;
			overflow = 0;
		}
	}
	if (GFX.FPSCnt < GFX.FSkip)
		RunSkip(Cycles);
	else	RunNoSkip(Cycles);
}

static	int	__fastcall	Read01356 (void)
{
	return PPU.ppuLatch;
}

static	int	__fastcall	Read2 (void)
{
	register unsigned char tmp;
	PPU.HVTog = TRUE;
	tmp = (PPU.ppuLatch & 0x1F) | PPU.Reg2002;
	if (tmp & 0x80)
		PPU.Reg2002 &= 0x60;
	return tmp;
}

static	int	__fastcall	Read4 (void)
{
	return PPU.Sprite[PPU.SprAddr++];
}

static	int	__fastcall	Read7 (void)
{
	register unsigned char tmp;
	tmp = PPU.ppuLatch;
	PPU.ppuLatch = MemGet(PPU.VRAMAddr);
	if (PPU.Reg2000 & 0x04)
		PPU.VRAMAddr += 32;
	else	PPU.VRAMAddr++;
	PPU.VRAMAddr &= 0x3FFF;
	return tmp;
}

int	__cdecl	PPU_Read (int Bank, int Where)
{
	static int (__fastcall *funcs[8])(void) = {Read01356,Read01356,Read2,Read01356,Read4,Read01356,Read01356,Read7};
	return funcs[Where & 7]();
/*	register unsigned char tmp;
	switch (Where & 7)
	{
	case 0:	return PPU.ppuLatch;			break;
	case 1:	return PPU.ppuLatch;			break;
	case 2:	PPU.HVTog = TRUE;
		tmp = (PPU.ppuLatch & 0x1F) | PPU.Reg2002;
		if (tmp & 0x80)
			PPU.Reg2002 &= 0x60;
		return tmp;				break;
	case 3:	return PPU.ppuLatch;			break;
	case 4:	return PPU.Sprite[PPU.SprAddr++];	break;
	case 5:	return PPU.ppuLatch;			break;
	case 6:	return PPU.ppuLatch;			break;
	case 7:	tmp = PPU.ppuLatch;
		PPU.ppuLatch = MemGet(PPU.VRAMAddr);
		if (PPU.Reg2000 & 0x04)
			PPU.VRAMAddr += 32;
		else	PPU.VRAMAddr++;
		PPU.VRAMAddr &= 0x3FFF;
		return tmp;				break;
	}
	return 0;*/
}

static	void	__fastcall	Write0 (int What)
{
	PPU.Reg2000 = What;
	PPU.IntReg &= 0x73FF;
	PPU.IntReg |= (What & 3) << 10;
	Debugger.NTabChanged = TRUE;
}

static	void	__fastcall	Write1 (int What)
{
	PPU.Reg2001 = What;
}

static	void	__fastcall	Write2 (int What)
{
}

static	void	__fastcall	Write3 (int What)
{
	PPU.SprAddr = What;
}

static	void	__fastcall	Write4 (int What)
{
	PPU.Sprite[PPU.SprAddr++] = What;
}

static	void	__fastcall	Write5 (int What)
{
	if (PPU.HVTog)
	{
		PPU.IntReg &= 0x7FE0;
		PPU.IntReg |= (What & 0xF8) >> 3;
		PPU.IntXBak = What & 7;
	}
	else
	{
		PPU.IntReg &= 0x0C1F;
		PPU.IntReg |= (What & 0x07) << 12;
		PPU.IntReg |= (What & 0xF8) << 2;
	}
	PPU.HVTog = !PPU.HVTog;
}

static	void	__fastcall	Write6 (int What)
{
	if (PPU.HVTog)
	{
		PPU.IntReg &= 0x00FF;
		PPU.IntReg |= (What & 0x3F) << 8;
	}
	else
	{
		PPU.IntReg &= 0x7F00;
		PPU.IntReg |= What;
		PPU.VRAMAddr = PPU.IntReg;
	}
	PPU.HVTog = !PPU.HVTog;
}

static	void	__fastcall	Write7 (int What)
{
	MemSet(PPU.VRAMAddr,What);
	if (PPU.Reg2000 & 0x04)
		PPU.VRAMAddr += 32;
	else	PPU.VRAMAddr++;
	PPU.VRAMAddr &= 0x3FFF;
}

void	__cdecl	PPU_Write (int Bank, int Where, int What)
{
	static void (__fastcall *funcs[8])(int) = {Write0,Write1,Write2,Write3,Write4,Write5,Write6,Write7};
	PPU.ppuLatch = What;
	funcs[Where & 7](What);
/*	PPU.ppuLatch = What;
	switch (Where & 0x7)
	{
	case 0:	PPU.Reg2000 = What;
		PPU.IntReg &= 0x73FF;
		PPU.IntReg |= (What & 3) << 10;
		Debugger.NTabChanged = TRUE;		break;
	case 1:	PPU.Reg2001 = What;			break;
	case 2:						break;
	case 3:	PPU.SprAddr = What;			break;
	case 4:	PPU.Sprite[PPU.SprAddr++] = What;	break;
	case 5:	if (PPU.HVTog)
		{
			PPU.IntReg &= 0x7FE0;
			PPU.IntReg |= (What & 0xF8) >> 3;
			PPU.IntXBak = What & 7;
		}
		else
		{
			PPU.IntReg &= 0x0C1F;
			PPU.IntReg |= (What & 0x07) << 12;
			PPU.IntReg |= (What & 0xF8) << 2;
		}
		PPU.HVTog = !PPU.HVTog;			break;
	case 6:	if (PPU.HVTog)
		{
			PPU.IntReg &= 0x00FF;
			PPU.IntReg |= (What & 0x3F) << 8;
		}
		else
		{
			PPU.IntReg &= 0x7F00;
			PPU.IntReg |= What;
			PPU.VRAMAddr = PPU.IntReg;
		}
		PPU.HVTog = !PPU.HVTog;			break;
	case 7:	MemSet(PPU.VRAMAddr,What);
		if (PPU.Reg2000 & 0x04)
			PPU.VRAMAddr += 32;
		else	PPU.VRAMAddr++;
		PPU.VRAMAddr &= 0x3FFF;			break;
	}*/
}
