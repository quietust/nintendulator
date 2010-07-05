/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2010 QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "MapperInterface.h"
#include "NES.h"
#include "PPU.h"
#include "CPU.h"
#include "GFX.h"
#include "Debugger.h"

namespace PPU
{
FPPURead	ReadHandler[0x10];
FPPUWrite	WriteHandler[0x10];
int SLEndFrame;
int Clockticks;
int SLnum;
unsigned char *CHRPointer[0x10];
BOOL Writable[0x10];

#ifdef	ACCURATE_SPRITES
unsigned char Sprite[0x120];
#else	/* !ACCURATE_SPRITES */
unsigned char Sprite[0x100];
#endif	/* ACCURATE_SPRITES */
unsigned char SprAddr;

unsigned char Palette[0x20];

unsigned char readLatch;

unsigned char Reg2000, Reg2001, Reg2002;

unsigned char HVTog, ShortSL, IsRendering, OnScreen;

unsigned long VRAMAddr, IntReg;
unsigned char IntX;
unsigned char TileData[272];

unsigned long GrayScale;	// ANDed with palette index (0x30 if grayscale, 0x3F if not)
unsigned long ColorEmphasis;	// ORed with palette index (upper 8 bits of $2001 shifted left 1 bit)

unsigned long RenderAddr;
unsigned long IOAddr;
unsigned char IOVal;
unsigned char IOMode;	// Start at 6 for writes, 5 for reads - counts down and eventually hits zero
unsigned char buf2007;

#ifdef	ACCURATE_SPRITES
unsigned char *SprBuff;
#else	/* !ACCURATE_SPRITES */
unsigned char SprBuff[32];
#endif	/* ACCURATE_SPRITES */
BOOL Spr0InLine;
int SprCount;
#ifdef	ACCURATE_SPRITES
unsigned char SprData[8][10];
#else	/* !ACCURATE_SPRITES */
unsigned char SprData[8][8];
#endif	/* ACCURATE_SPRITES */
unsigned short *GfxData;
BOOL IsPAL;
unsigned char PALsubticks;
unsigned char	VRAM[0x4][0x400];
unsigned short	DrawArray[256*240];

/*#define	SHORQ	/* Enable ShoRQ(tm) technology */

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

const	unsigned long	CHRLoBit[16] =
{
	0x00000000,0x00000001,0x00000100,0x00000101,0x00010000,0x00010001,0x00010100,0x00010101,
	0x01000000,0x01000001,0x01000100,0x01000101,0x01010000,0x01010001,0x01010100,0x01010101
};
const	unsigned long	CHRHiBit[16] =
{
	0x00000000,0x00000002,0x00000200,0x00000202,0x00020000,0x00020002,0x00020200,0x00020202,
	0x02000000,0x02000002,0x02000200,0x02000202,0x02020000,0x02020002,0x02020200,0x02020202
};

const	unsigned char	AttribLoc[256] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
	0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
	0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
	0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
	0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F
};
const	unsigned long	AttribBits[4] =
{
	0x00000000,0x04040404,0x08080808,0x0C0C0C0C
};

const	unsigned char	AttribShift[128] =
{
	0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,
	4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6
};

unsigned char	OpenBus[0x400] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
	0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
	0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
	0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
	0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
	0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
	0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
	0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
	0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
	0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
	0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
	0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
	0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
	0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
	0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
	0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
	0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
	0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
	0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
	0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
	0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
	0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
	0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
	0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
	0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};

void	(MAPINT *PPUCycle)		(int,int,int,int);
void	MAPINT	NoPPUCycle		(int Addr, int Scanline, int Cycle, int IsRendering)	{ }

int	MAPINT	BusRead (int Bank, int Addr)
{
//	if (!Readable[Bank])
//		return Addr & 0xFF;
	return CHRPointer[Bank][Addr];
}

void	MAPINT	BusWriteCHR (int Bank, int Addr, int Val)
{
	if (!Writable[Bank])
		return;
#ifdef	ENABLE_DEBUGGER
	Debugger::PatChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
	CHRPointer[Bank][Addr] = (unsigned char)Val;
}

void	MAPINT	BusWriteNT (int Bank, int Addr, int Val)
{
	if (!Writable[Bank])
		return;
#ifdef	ENABLE_DEBUGGER
	Debugger::NTabChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
	CHRPointer[Bank][Addr] = (unsigned char)Val;
}

void	Reset (void)
{
	SprAddr = 0;
	IsRendering = FALSE;
	OnScreen = FALSE;
	IOAddr = 0;
	IOVal = 0;
	IOMode = 0;
	Clockticks = 0;
	PALsubticks = 0;
	SLnum = 241;
#ifdef	ACCURATE_SPRITES
	SprBuff = &Sprite[256];
#endif	/* ACCURATE_SPRITES */
	GetHandlers();
}
void	GetHandlers (void)
{
	if ((MI) && (MI->PPUCycle))
		PPUCycle = MI->PPUCycle;
	else	PPUCycle = NoPPUCycle;
	GetGFXPtr();
}

#ifdef	ACCURATE_SPRITES
int	SpritePtr;
__inline void	ProcessSprites (void)
{
	static int sprpos, sprnum, spridx, sprsub, sprtmp, sprstate, sprtotal, sprzero;
	if (Clockticks < 64)
		Sprite[SpritePtr = 0x100 | (Clockticks >> 1)] = 0xFF;
	else if (Clockticks < 256)
	{
		if (Clockticks == 64)
		{
			sprpos = 0x100;
			sprnum = 0;
			spridx = (SprAddr & 0xF8) >> 2;
			sprsub = 0;
			sprstate = 0;
			sprtotal = 0;
			sprzero = FALSE;
		}
		switch (sprstate)
		{
		case 0:	// evaluate current Y coordinate
			if (Clockticks & 1)
			{
				Sprite[SpritePtr = sprpos] = (unsigned char)sprtmp;
				if ((SLnum >= sprtmp) && (SLnum <= sprtmp + ((Reg2000 & 0x20) ? 0xF : 0x7)))
				{
					if (sprnum == 0)
						sprzero = TRUE;
					sprstate = 1;	// sprite is in range, fetch the rest of it
					sprsub = 1;
					sprpos++;
					sprtotal++;
				}
				else
				{
					if (++spridx, ++sprnum == 64)
					{
						spridx = 0;
						sprstate = 4;
					}
					else if (sprpos == 0x120)
						sprstate = 2;
				}
			}
			else
			{
				if (sprnum == 2)
					spridx = sprnum;
				sprtmp = Sprite[SpritePtr = (spridx << 2)];
			}
			break;
		case 1:	// Y-coordinate is in range, copy remaining bytes
			if (Clockticks & 1)
			{
				Sprite[SpritePtr = sprpos++] = (unsigned char)sprtmp;
				if (sprsub++ == 3)
				{
					sprsub = 0;
					if (++spridx, ++sprnum == 64)
					{
						spridx = 0;
						sprstate = 4;
					}
					else if (sprpos == 0x120)
						sprstate = 2;
					else	sprstate = 0;
				}
			}
			else	sprtmp = Sprite[SpritePtr = (spridx << 2) | sprsub];
			break;
		case 2:	// exactly 8 sprites detected, go through 'weird' evaluation
			if (Clockticks & 1)
			{
				SpritePtr = 0x100;	// failed write
				if ((SLnum >= sprtmp) && (SLnum <= sprtmp + ((Reg2000 & 0x20) ? 0xF : 0x7)))
				{	// 9th sprite found "in range"
					sprstate = 3;
					sprsub = (sprsub + 1) & 3;
					if (!sprsub)
						spridx = (spridx + 1) & 63;
					sprpos = 1;
					Reg2002 |= 0x20;	// set sprite overflow flag
				}
				else
				{
					sprsub = (sprsub + 1) & 3;
					if (++spridx, ++sprnum == 64)
					{
						spridx = 0;
						sprstate = 4;
					}
				}
			}
			else	sprtmp = Sprite[SpritePtr = (spridx << 2) | sprsub];
			break;
		case 3:	// 9th sprite detected, fetch next 3 bytes
			if (Clockticks & 1)
			{
				SpritePtr = 0x100;
				sprsub = (sprsub + 1) & 3;
				if (!sprsub)
					spridx = (spridx + 1) & 63;
				if (sprpos++ == 3)
				{
					if (sprsub == 3)
						spridx = (spridx + 1) & 63;
					sprstate = 4;
				}
			}
			else	sprtmp = Sprite[SpritePtr = (spridx << 2) | sprsub];
			break;
		case 4:	// no more sprites to evaluate, thrash until HBLANK
			if (Clockticks & 1)
			{
				SpritePtr = sprpos;
				spridx = (spridx + 1) & 63;
			}
			else	sprtmp = Sprite[SpritePtr = spridx << 2];
			break;
		}
	}
	else if (Clockticks < 320)
	{
		if (Clockticks == 256)
		{
			SprCount = sprtotal * 4;
			Spr0InLine = sprzero;
		}
		if ((Clockticks & 7) < 4)
			SpritePtr = 0x100 | (Clockticks & 3) | (((Clockticks - 256) & 0x38) >> 1);
		else	SpritePtr = 0x103 | (((Clockticks - 256) & 0x38) >> 1);
	}
	else
	{
		SpritePtr = 0x100;
	}

//*** Cycles 0-63: Secondary OAM (32-byte buffer for current sprites on scanline) is initialized to $FF - attempting to read $2004 will return $FF
//*** Cycles 64-255: Sprite evaluation
//* On even cycles, data is read from (primary) OAM
//* On odd cycles, data is written to secondary OAM (unless writes are inhibited, in which case it will read the value in secondary OAM instead)
//1. Starting at n = 0, read a sprite's Y-coordinate (OAM[n][0], copying it to the next open slot in secondary OAM (unless 8 sprites have been found, in which case the write is ignored).
//1a. If Y-coordinate is in range, copy remaining bytes of sprite data (OAM[n][1] thru OAM[n][3]) into secondary OAM.
//2. Increment n
//2a. If n has overflowed back to zero (all 64 sprites evaluated), go to 4
//2b. If less than 8 sprites have been found, go to 1
//2c. If exactly 8 sprites have been found, disable writes to secondary OAM
//3. Starting at m = 0, evaluate OAM[n][m] as a Y-coordinate.
//3a. If the value is in range, set the sprite overflow flag in $2002 and read the next 3 entries of OAM (incrementing 'm' after each byte and incrementing 'n' when 'm' overflows); if m = 3, increment n
//3b. If the value is not in range, increment n AND m (without carry). If n overflows to 0, go to 4; otherwise go to 3
//4. Attempt (and fail) to copy OAM[n][0] into the next free slot in secondary OAM, and increment n (repeat until HBLANK is reached)
//*** Cycles 256-319: Sprite fetches (8 sprites total, 8 cycles per sprite)
//1-4: Read the Y-coordinate, tile number, attributes, and X-coordinate of the selected sprite
//5-8: Read the X-coordinate of the selected sprite 4 times.
//* On the first empty sprite slot, read the Y-coordinate of sprite #63 followed by $FF for the remaining 7 cycles
//* On all subsequent empty sprite slots, read $FF for all 8 reads
//*** Cycles 320-340: Background render pipeline initialization
//* Read the first byte in secondary OAM (the Y-coordinate of the first sprite found, sprite #63 if no sprites were found)
}
#else	/* !ACCURATE_SPRITES */
__inline void	DiscoverSprites (void)
{
	int SprHeight = 7 | ((Reg2000 & 0x20) >> 2);
	int SL = SLnum;
	int spt;
	SprCount = 0;
	Spr0InLine = FALSE;
	if (!IsRendering)
		return;
	for (spt = 0; spt < 32; spt += 4)
		SprBuff[spt+1] = 0xFF;	// pre-init sprite buffer tile indices
	for (spt = 0; spt < 256; spt += 4)
	{
		unsigned char off;
		if (spt < 8)
			off = spt + (SprAddr & 0xF8);
		else	off = spt;
		if ((SL - Sprite[off]) & ~SprHeight)
			continue;
		if (SprCount == 32)
		{
			Reg2002 |= 0x20;
			break;
		}
		SprBuff[SprCount] = SL - Sprite[off];
		SprBuff[SprCount+1] = Sprite[off | 1];
		SprBuff[SprCount+2] = Sprite[off | 2];
		SprBuff[SprCount+3] = Sprite[off | 3];
		SprCount += 4;
		if (!spt)
			Spr0InLine = TRUE;
	}
}
#endif	/* ACCURATE_SPRITES */
void	GetGFXPtr (void)
{
	GfxData = DrawArray;
}

void	PowerOn()
{
	Reg2000 = 0;
	Reg2001 = 0;
	ColorEmphasis = 0;
	GrayScale = 0x3F;
	VRAMAddr = IntReg = 0;
	IntX = 0;
	readLatch = 0;
	Reg2002 = 0;
	buf2007 = 0;
	HVTog = TRUE;
	ShortSL = TRUE;
	ZeroMemory(VRAM, sizeof(VRAM));
	memset(Palette, 0x3F, sizeof(Palette));
	memset(Sprite, 0xFF, sizeof(Sprite));
	GetHandlers();
}

int	Save (FILE *out)
{
	int clen = 0;
	writeArray(VRAM, 0x1000);	//	NTAR	uint8[0x1000]	4 KB of name/attribute table RAM
	writeArray(Sprite, 0x100);	//	SPRA	uint8[0x100]	256 bytes of sprite RAM
	writeArray(Palette, 0x20);	//	PRAM	uint8[0x20]	32 bytes of palette index RAM
	writeByte(Reg2000);		//	R2000	uint8		Last value written to $2000
	writeByte(Reg2001);		//	R2001	uint8		Last value written to $2001
	writeByte(Reg2002);		//	R2002	uint8		Current contents of $2002
	writeByte(SprAddr);		//	SPADR	uint8		SPR-RAM Address ($2003)

	writeByte(IntX);		//	XOFF	uint8		Tile X-offset.

	writeByte(HVTog);		//	VTOG	uint8		Toggle used by $2005 and $2006.
	writeWord(VRAMAddr)		//	RADD	uint16		VRAM Address
	writeWord(IntReg);		//	TADD	uint16		VRAM Address Latch
	writeByte(buf2007);		//	VBUF	uint8		VRAM Read Buffer
	writeByte(readLatch);		//	PGEN	uint8		PPU "general" latch

	writeWord(Clockticks | (PALsubticks << 12));
					//	TICKS	uint16		Clock Ticks (0..340) with PAL subticks stored in upper 4 bits
	writeWord(SLnum);		//	SLNUM	uint16		Scanline number
	writeByte(ShortSL);		//	SHORT	uint8		Short frame (last scanline 1 clock tick shorter)

	writeWord(IOAddr);		//	IOADD	uint16		External I/O Address
	writeByte(IOVal);		//	IOVAL	uint8		External I/O Value
	writeByte(IOMode);		//	IOMOD	uint8		External I/O Mode/Counter

	writeByte(IsPAL);		//	NTSCP	uint8		0 for NTSC, 1 for PAL
	return clen;
}

int	Load (FILE *in)
{
	int clen = 0;
	unsigned short tps;
	readArray(VRAM, 0x1000);	//	NTAR	uint8[0x1000]	4 KB of name/attribute table RAM
	readArray(Sprite, 0x100);	//	SPRA	uint8[0x100]	256 bytes of sprite RAM
	readArray(Palette, 0x20);	//	PRAM	uint8[0x20]	32 bytes of palette index RAM
	readByte(Reg2000);		//	R2000	uint8		Last value written to $2000
	readByte(Reg2001);		//	R2001	uint8		Last value written to $2001
	readByte(Reg2002);		//	R2002	uint8		Current contents of $2002
	readByte(SprAddr);		//	SPADR	uint8		SPR-RAM Address ($2003)

	readByte(IntX);			//	XOFF	uint8		Tile X-offset.

	readByte(HVTog);		//	VTOG	uint8		Toggle used by $2005 and $2006.
	readWord(VRAMAddr);		//	RADD	uint16		VRAM Address
	readWord(IntReg);		//	TADD	uint16		VRAM Address Latch
	readByte(buf2007);		//	VBUF	uint8		VRAM Read Buffer
	readByte(readLatch);		//	PGEN	uint8		PPU "general" latch.

	readWord(tps);			//	TICKS	uint16		Clock Ticks (0..340) with PAL subticks stored in upper 4 bits
	Clockticks = tps & 0xFFF;
	PALsubticks = (unsigned char)(tps >> 12);

	readWord(SLnum);		//	SLNUM	uint16		Scanline number
	readByte(ShortSL);		//	SHORT	uint8		Short frame (last scanline 1 clock tick shorter)

	readWord(IOAddr);		//	IOADD	uint16		External I/O Address
	readByte(IOVal);		//	IOVAL	uint8		External I/O Value
	readByte(IOMode);		//	IOMOD	uint8		External I/O Mode/Counter

	readByte(IsPAL);		//	NTSCP	uint8		0 for NTSC, 1 for PAL

	IsRendering = OnScreen = FALSE;
	ColorEmphasis = (Reg2001 & 0xE0) << 1;
	GrayScale = (Reg2001 & 0x01) ? 0x30 : 0x3F;
	NES::SetCPUMode(IsPAL);
	return clen;
}

int EndSLTicks = 341;
unsigned long PatAddr;
unsigned char RenderData[4];

__inline void	RunNoSkip (int NumTicks)
{
	register unsigned long TL;
	register unsigned char TC;

	register int SprNum;
#ifdef	ACCURATE_SPRITES
	register unsigned char SprSL;
#endif	/* ACCURATE_SPRITES */
	register unsigned char *CurTileData;

	register int i, y;
	for (i = 0; i < NumTicks; i++)
	{
		Clockticks++;
		if (Clockticks == 256)
		{
			if (SLnum < 240)
			{
#ifndef	ACCURATE_SPRITES
				DiscoverSprites();
#endif	/* !ACCURATE_SPRITES */
				ZeroMemory(TileData, sizeof(TileData));
			}
			if (SLnum == -1)
			{
				if ((ShortSL) && (IsRendering) && (!IsPAL))
					EndSLTicks = 340;
				else	EndSLTicks = 341;
			}
			else	EndSLTicks = 341;
		}
		else if (Clockticks == 304)
		{
			if ((IsRendering) && (SLnum == -1))
				VRAMAddr = IntReg;
		}
		else if (Clockticks == EndSLTicks)
		{
			Clockticks = 0;
			SLnum++;
			NES::Scanline = TRUE;
			if (SLnum < 240)
				OnScreen = TRUE;
			else if (SLnum == 240)
				IsRendering = OnScreen = FALSE;
			else if (SLnum == 241)
			{
				GetGFXPtr();
				GFX::DrawScreen();
				Reg2002 |= 0x80;
				if (Reg2000 & 0x80)
					CPU::WantNMI = TRUE;
//				SprAddr = 0;
			}
			else if (SLnum == SLEndFrame - 1)
			{
				SLnum = -1;
				ShortSL = !ShortSL;
				if (Reg2001 & 0x18)
					IsRendering = TRUE;
			}
		}
		// VBL flag gets cleared a cycle late
		else if ((SLnum == -1) && (Clockticks == 1))
			Reg2002 = 0;
		if (IsRendering)
		{
#ifdef	ACCURATE_SPRITES
			ProcessSprites();
#endif	/* ACCURATE_SPRITES */
			if (Clockticks & 1)
			{
				if (IOMode)
				{
					RenderData[(Clockticks >> 1) & 3] = 0;	// seems to force black in this case
					if (IOMode == 2)
						WriteHandler[RenderAddr >> 10](RenderAddr >> 10, RenderAddr & 0x3FF, IOVal = RenderData[(Clockticks >> 1) & 3]);
				}
				else if (ReadHandler[RenderAddr >> 10] == BusRead)
					RenderData[(Clockticks >> 1) & 3] = CHRPointer[RenderAddr >> 10][RenderAddr & 0x3FF];
				else	RenderData[(Clockticks >> 1) & 3] = (unsigned char)ReadHandler[RenderAddr >> 10](RenderAddr >> 10, RenderAddr & 0x3FF);
			}
			switch (Clockticks)
			{
				// BEGIN BACKGROUND
			case   0:	case   8:	case  16:	case  24:	case  32:	case  40:	case  48:	case  56:
			case  64:	case  72:	case  80:	case  88:	case  96:	case 104:	case 112:	case 120:
			case 128:	case 136:	case 144:	case 152:	case 160:	case 168:	case 176:	case 184:
			case 192:	case 200:	case 208:	case 216:	case 224:	case 232:	case 240:	case 248:
			case 320:	case 328:
				RenderAddr = 0x2000 | (VRAMAddr & 0xFFF);
				break;
			case   1:	case   9:	case  17:	case  25:	case  33:	case  41:	case  49:	case  57:
			case  65:	case  73:	case  81:	case  89:	case  97:	case 105:	case 113:	case 121:
			case 129:	case 137:	case 145:	case 153:	case 161:	case 169:	case 177:	case 185:
			case 193:	case 201:	case 209:	case 217:	case 225:	case 233:	case 241:	case 249:
			case 321:	case 329:
				PatAddr = (RenderData[0] << 4) | (VRAMAddr >> 12) | ((Reg2000 & 0x10) << 8);
				break;
			case   2:	case  10:	case  18:	case  26:	case  34:	case  42:	case  50:	case  58:
			case  66:	case  74:	case  82:	case  90:	case  98:	case 106:	case 114:	case 122:
			case 130:	case 138:	case 146:	case 154:	case 162:	case 170:	case 178:	case 186:
			case 194:	case 202:	case 210:	case 218:	case 226:	case 234:	case 242:	case 250:
			case 322:	case 330:
				RenderAddr = 0x23C0 | (VRAMAddr & 0xC00) | AttribLoc[(VRAMAddr >> 2) & 0xFF];
				break;
			case   3:	case  11:	case  19:	case  27:	case  35:	case  43:	case  51:	case  59:
			case  67:	case  75:	case  83:	case  91:	case  99:	case 107:	case 115:	case 123:
			case 131:	case 139:	case 147:	case 155:	case 163:	case 171:	case 179:	case 187:
			case 195:	case 203:	case 211:	case 219:	case 227:	case 235:	case 243:
				CurTileData = &TileData[Clockticks + 13];
				TL = AttribBits[(RenderData[1] >> AttribShift[VRAMAddr & 0x7F]) & 3];
				((unsigned long *)CurTileData)[0] = TL;
				((unsigned long *)CurTileData)[1] = TL;
				if ((VRAMAddr & 0x1F) == 0x1F)
					VRAMAddr ^= 0x41F;
				else	VRAMAddr++;
				break;
			case 251:
				CurTileData = &TileData[Clockticks + 13];
				TL = AttribBits[(RenderData[1] >> AttribShift[VRAMAddr & 0x7F]) & 3];
				((unsigned long *)CurTileData)[0] = TL;
				((unsigned long *)CurTileData)[1] = TL;
				if ((VRAMAddr & 0x1F) == 0x1F)
					VRAMAddr ^= 0x41F;
				else	VRAMAddr++;
				if ((VRAMAddr & 0x7000) == 0x7000)
				{
					register int YScroll = VRAMAddr & 0x3E0;
					VRAMAddr &= 0xFFF;
					if (YScroll == 0x3A0)
						VRAMAddr ^= 0xBA0;
					else if (YScroll == 0x3E0)
						VRAMAddr ^= 0x3E0;
					else	VRAMAddr += 0x20;
				}
				else	VRAMAddr += 0x1000;
				break;
			case 323:	case 331:
				CurTileData = &TileData[Clockticks - 323];
				TL = AttribBits[(RenderData[1] >> AttribShift[VRAMAddr & 0x7F]) & 3];
				((unsigned long *)CurTileData)[0] = TL;
				((unsigned long *)CurTileData)[1] = TL;
				if ((VRAMAddr & 0x1F) == 0x1F)
					VRAMAddr ^= 0x41F;
				else	VRAMAddr++;
				break;
			case   4:	case  12:	case  20:	case  28:	case  36:	case  44:	case  52:	case  60:
			case  68:	case  76:	case  84:	case  92:	case 100:	case 108:	case 116:	case 124:
			case 132:	case 140:	case 148:	case 156:	case 164:	case 172:	case 180:	case 188:
			case 196:	case 204:	case 212:	case 220:	case 228:	case 236:	case 244:	case 252:
			case 324:	case 332:
				RenderAddr = PatAddr;
				break;
			case   5:	case  13:	case  21:	case  29:	case  37:	case  45:	case  53:	case  61:
			case  69:	case  77:	case  85:	case  93:	case 101:	case 109:	case 117:	case 125:
			case 133:	case 141:	case 149:	case 157:	case 165:	case 173:	case 181:	case 189:
			case 197:	case 205:	case 213:	case 221:	case 229:	case 237:	case 245:	case 253:
				TC = ReverseCHR[RenderData[2]];
				CurTileData = &TileData[Clockticks + 11];
				((unsigned long *)CurTileData)[0] |= CHRLoBit[TC & 0xF];
				((unsigned long *)CurTileData)[1] |= CHRLoBit[TC >> 4];
				break;
			case 325:	case 333:
				TC = ReverseCHR[RenderData[2]];
				CurTileData = &TileData[Clockticks - 325];
				((unsigned long *)CurTileData)[0] |= CHRLoBit[TC & 0xF];
				((unsigned long *)CurTileData)[1] |= CHRLoBit[TC >> 4];
				break;
			case   6:	case  14:	case  22:	case  30:	case  38:	case  46:	case  54:	case  62:
			case  70:	case  78:	case  86:	case  94:	case 102:	case 110:	case 118:	case 126:
			case 134:	case 142:	case 150:	case 158:	case 166:	case 174:	case 182:	case 190:
			case 198:	case 206:	case 214:	case 222:	case 230:	case 238:	case 246:	case 254:
			case 326:	case 334:
				RenderAddr = PatAddr | 8;
				break;
			case   7:	case  15:	case  23:	case  31:	case  39:	case  47:	case  55:	case  63:
			case  71:	case  79:	case  87:	case  95:	case 103:	case 111:	case 119:	case 127:
			case 135:	case 143:	case 151:	case 159:	case 167:	case 175:	case 183:	case 191:
			case 199:	case 207:	case 215:	case 223:	case 231:	case 239:	case 247:	case 255:
				TC = ReverseCHR[RenderData[3]];
				CurTileData = &TileData[Clockticks + 9];
				((unsigned long *)CurTileData)[0] |= CHRHiBit[TC & 0xF];
				((unsigned long *)CurTileData)[1] |= CHRHiBit[TC >> 4];
				break;
			case 327:	case 335:
				TC = ReverseCHR[RenderData[3]];
				CurTileData = &TileData[Clockticks - 327];
				((unsigned long *)CurTileData)[0] |= CHRHiBit[TC & 0xF];
				((unsigned long *)CurTileData)[1] |= CHRHiBit[TC >> 4];
				break;
				// END BACKGROUND
				// BEGIN SPRITES
			case 256:	case 264:	case 272:	case 280:	case 288:	case 296:	case 304:	case 312:
				RenderAddr = 0x2000 | (VRAMAddr & 0xFFF);
				break;
			case 257:
				VRAMAddr &= ~0x41F;
				VRAMAddr |= IntReg & 0x41F;
					case 265:	case 273:	case 281:	case 289:	case 297:	case 305:	case 313:
				break;
			case 258:	case 266:	case 274:	case 282:	case 290:	case 298:	case 306:	case 314:
				RenderAddr = 0x2000 | (VRAMAddr & 0xFFF);
				break;
			case 259:	case 267:	case 275:	case 283:	case 291:	case 299:	case 307:	case 315:
				SprNum = (Clockticks >> 1) & 0x1C;
				TC = SprBuff[SprNum | 1];
#ifdef	ACCURATE_SPRITES
				SprSL = (unsigned char)(SLnum - SprBuff[SprNum]);
 				if (Reg2000 & 0x20)
					PatAddr = ((TC & 0xFE) << 4) | ((TC & 0x01) << 12) | ((SprSL & 7) ^ ((SprBuff[SprNum | 2] & 0x80) ? 0x17 : 0x00) ^ ((SprSL & 0x8) << 1));
				else	PatAddr = (TC << 4) | ((SprSL & 7) ^ ((SprBuff[SprNum | 2] & 0x80) ? 0x7 : 0x0)) | ((Reg2000 & 0x08) << 9);
#else	/* !ACCURATE_SPRITES */
 				if (Reg2000 & 0x20)
					PatAddr = ((TC & 0xFE) << 4) | ((TC & 0x01) << 12) | ((SprBuff[SprNum] & 7) ^ ((SprBuff[SprNum | 2] & 0x80) ? 0x17 : 0x00) ^ ((SprBuff[SprNum] & 0x8) << 1));
				else	PatAddr = (TC << 4) | (SprBuff[SprNum] ^ ((SprBuff[SprNum | 2] & 0x80) ? 0x7 : 0x0)) | ((Reg2000 & 0x08) << 9);
#endif	/* ACCURATE_SPRITES */
				break;
			case 260:	case 268:	case 276:	case 284:	case 292:	case 300:	case 308:	case 316:
				RenderAddr = PatAddr;
				break;
			case 261:	case 269:	case 277:	case 285:	case 293:	case 301:	case 309:	case 317:
				SprNum = (Clockticks >> 1) & 0x1E;
				if (SprBuff[SprNum] & 0x40)
					TC = RenderData[2];
				else	TC = ReverseCHR[RenderData[2]];
				TL = AttribBits[SprBuff[SprNum] & 0x3];
				CurTileData = SprData[SprNum >> 2];
				((unsigned long *)CurTileData)[0] = CHRLoBit[TC & 0xF] | TL;
				((unsigned long *)CurTileData)[1] = CHRLoBit[TC >> 4] | TL;
				break;
			case 262:	case 270:	case 278:	case 286:	case 294:	case 302:	case 310:	case 318:
				RenderAddr = PatAddr | 8;
				break;
			case 263:	case 271:	case 279:	case 287:	case 295:	case 303:	case 311:	case 319:
				SprNum = (Clockticks >> 1) & 0x1E;
				if (SprBuff[SprNum] & 0x40)
					TC = RenderData[3];
				else	TC = ReverseCHR[RenderData[3]];
				CurTileData = SprData[SprNum >> 2];
				((unsigned long *)CurTileData)[0] |= CHRHiBit[TC & 0xF];
				((unsigned long *)CurTileData)[1] |= CHRHiBit[TC >> 4];
#ifdef	ACCURATE_SPRITES
				CurTileData[8] = SprBuff[SprNum];
				CurTileData[9] = SprBuff[SprNum | 1];
#endif	/* ACCURATE_SPRITES */
				break;
				// END SPRITES
			case 336:	case 338:
				RenderAddr = 0x2000 | (VRAMAddr & 0xFFF);
			case 337:	case 339:
				break;
			case 340:
				break;
			}
			if (!(Clockticks & 1))
			{
				PPUCycle(RenderAddr, SLnum, Clockticks, 1);
				if (IOMode == 2)
					WriteHandler[RenderAddr >> 10](RenderAddr >> 10, RenderAddr & 0x3FF, RenderAddr & 0xFF);
			}
		}
		if (IOMode)
		{
			unsigned short addr = (unsigned short)(IOAddr & 0x3FFF);
			if ((IOMode >= 5) && (!IsRendering))
				PPUCycle(addr, SLnum, Clockticks, IsRendering);
			else if (IOMode == 2)
			{
				if (!IsRendering)
					WriteHandler[addr >> 10](addr >> 10, addr & 0x3FF, IOVal);
			}
			else if (IOMode == 1)
			{
				IOMode++;
				if (!IsRendering)
				{
					if (ReadHandler[addr >> 10] == BusRead)
						buf2007 = CHRPointer[addr >> 10][addr & 0x3FF];
					else	buf2007 = (unsigned char)ReadHandler[addr >> 10](addr >> 10, addr & 0x3FF);
				}
			}
			IOMode -= 2;
		}
		if (!IsRendering && !IOMode)
			PPUCycle(VRAMAddr, SLnum, Clockticks, 0);
		if ((Clockticks < 256) && (OnScreen))
		{
			register int PalIndex;
			if ((Reg2001 & 0x08) && ((Clockticks >= 8) || (Reg2001 & 0x02)))
				TC = TileData[Clockticks + IntX];
			else	TC = 0;
			if ((Reg2001 & 0x10) && ((Clockticks >= 8) || (Reg2001 & 0x04)))
				for (y = 0; y < SprCount; y += 4)
				{
#ifdef	ACCURATE_SPRITES
					register int SprPixel = Clockticks - SprData[y >> 2][9];
#else	/* !ACCURATE_SPRITES */
					register int SprPixel = Clockticks - SprBuff[y | 3];
#endif	/* ACCURATE_SPRITES */
					register unsigned char SprDat;
					if (SprPixel & ~7)
						continue;
					SprDat = SprData[y >> 2][SprPixel];
					if (SprDat & 0x3)
					{
						if ((Spr0InLine) && (y == 0) && (TC & 0x3) && (Clockticks < 255))
						{
							Reg2002 |= 0x40;	// Sprite 0 hit
							Spr0InLine = FALSE;
						}
#ifdef	ACCURATE_SPRITES
						if (!((TC & 0x3) && (SprData[y >> 2][8] & 0x20)))
#else	/* !ACCURATE_SPRITES */
						if (!((TC & 0x3) && (SprBuff[y | 2] & 0x20)))
#endif	/* ACCURATE_SPRITES */
							TC = SprDat | 0x10;
						break;
					}
				}
			if (TC & 0x3)
				PalIndex = Palette[TC & 0x1F];
			else	PalIndex = Palette[0];
			if ((!IsRendering) && ((VRAMAddr & 0x3F00) == 0x3F00))
				PalIndex = Palette[VRAMAddr & 0x1F];
			PalIndex &= GrayScale;
			PalIndex |= ColorEmphasis;
#ifdef	SHORQ
			if (CPU.WantIRQ)
				PalIndex ^= 0x80;
#endif	/* SHORQ */
			*GfxData = (unsigned short)PalIndex;
			GfxData++;
		}
	}
}
__inline void	RunSkip (int NumTicks)
{
	register unsigned char TC;

	register int SprNum;
	register unsigned char *CurTileData;

	register int i;
	for (i = 0; i < NumTicks; i++)
	{
		Clockticks++;
		if (Clockticks == 256)
		{
			if (SLnum < 240)
			{
#ifndef	ACCURATE_SPRITES
				DiscoverSprites();
#endif	/* !ACCURATE_SPRITES */
				if (Spr0InLine)
					ZeroMemory(TileData, sizeof(TileData));
			}
			if (SLnum == -1)
			{
				if ((ShortSL) && (IsRendering) && (!IsPAL))
					EndSLTicks = 340;
				else	EndSLTicks = 341;
			}
			else	EndSLTicks = 341;
		}
		else if (Clockticks == 304)
		{
			if ((IsRendering) && (SLnum == -1))
				VRAMAddr = IntReg;
		}
		else if (Clockticks == EndSLTicks)
		{
			Clockticks = 0;
			SLnum++;
			NES::Scanline = TRUE;
			if (SLnum < 240)
				OnScreen = TRUE;
			else if (SLnum == 240)
				IsRendering = OnScreen = FALSE;
			else if (SLnum == 241)
			{
				GetGFXPtr();
				GFX::DrawScreen();
				Reg2002 |= 0x80;
				if (Reg2000 & 0x80)
					CPU::WantNMI = TRUE;
//				SprAddr = 0;
			}
			else if (SLnum == SLEndFrame - 1)
			{
				SLnum = -1;
				ShortSL = !ShortSL;
				if (Reg2001 & 0x18)
					IsRendering = TRUE;
			}
		}
		// VBL flag gets cleared a cycle late
		else if ((SLnum == -1) && (Clockticks == 1))
			Reg2002 = 0;
		if (IsRendering)
		{
#ifdef	ACCURATE_SPRITES
			ProcessSprites();
#endif	/* ACCURATE_SPRITES */
			if (Clockticks & 1)
			{
				if (IOMode)
				{
					RenderData[(Clockticks >> 1) & 3] = 0;	// seems to force black in this case
					if (IOMode == 2)
						WriteHandler[RenderAddr >> 10](RenderAddr >> 10, RenderAddr & 0x3FF, IOVal = RenderData[(Clockticks >> 1) & 3]);
				}
				else if (ReadHandler[RenderAddr >> 10] == BusRead)
					RenderData[(Clockticks >> 1) & 3] = CHRPointer[RenderAddr >> 10][RenderAddr & 0x3FF];
				else	RenderData[(Clockticks >> 1) & 3] = (unsigned char)ReadHandler[RenderAddr >> 10](RenderAddr >> 10, RenderAddr & 0x3FF);
			}
			switch (Clockticks)
			{
				// BEGIN BACKGROUND
			case   0:	case   8:	case  16:	case  24:	case  32:	case  40:	case  48:	case  56:
			case  64:	case  72:	case  80:	case  88:	case  96:	case 104:	case 112:	case 120:
			case 128:	case 136:	case 144:	case 152:	case 160:	case 168:	case 176:	case 184:
			case 192:	case 200:	case 208:	case 216:	case 224:	case 232:	case 240:	case 248:
			case 320:	case 328:
				RenderAddr = 0x2000 | (VRAMAddr & 0xFFF);
				break;
			case   1:	case   9:	case  17:	case  25:	case  33:	case  41:	case  49:	case  57:
			case  65:	case  73:	case  81:	case  89:	case  97:	case 105:	case 113:	case 121:
			case 129:	case 137:	case 145:	case 153:	case 161:	case 169:	case 177:	case 185:
			case 193:	case 201:	case 209:	case 217:	case 225:	case 233:	case 241:	case 249:
			case 321:	case 329:
				PatAddr = (RenderData[0] << 4) | (VRAMAddr >> 12) | ((Reg2000 & 0x10) << 8);
				break;
			case   2:	case  10:	case  18:	case  26:	case  34:	case  42:	case  50:	case  58:
			case  66:	case  74:	case  82:	case  90:	case  98:	case 106:	case 114:	case 122:
			case 130:	case 138:	case 146:	case 154:	case 162:	case 170:	case 178:	case 186:
			case 194:	case 202:	case 210:	case 218:	case 226:	case 234:	case 242:	case 250:
			case 322:	case 330:
				RenderAddr = 0x23C0 | (VRAMAddr & 0xC00) | AttribLoc[(VRAMAddr >> 2) & 0xFF];
				break;
			case   3:	case  11:	case  19:	case  27:	case  35:	case  43:	case  51:	case  59:
			case  67:	case  75:	case  83:	case  91:	case  99:	case 107:	case 115:	case 123:
			case 131:	case 139:	case 147:	case 155:	case 163:	case 171:	case 179:	case 187:
			case 195:	case 203:	case 211:	case 219:	case 227:	case 235:	case 243:
			case 323:	case 331:
				if ((VRAMAddr & 0x1F) == 0x1F)
					VRAMAddr ^= 0x41F;
				else	VRAMAddr++;
				break;
			case 251:
				if ((VRAMAddr & 0x1F) == 0x1F)
					VRAMAddr ^= 0x41F;
				else	VRAMAddr++;
				if ((VRAMAddr & 0x7000) == 0x7000)
				{
					register int YScroll = VRAMAddr & 0x3E0;
					VRAMAddr &= 0xFFF;
					if (YScroll == 0x3A0)
						VRAMAddr ^= 0xBA0;
					else if (YScroll == 0x3E0)
						VRAMAddr ^= 0x3E0;
					else	VRAMAddr += 0x20;
				}
				else	VRAMAddr += 0x1000;
				break;
			case   4:	case  12:	case  20:	case  28:	case  36:	case  44:	case  52:	case  60:
			case  68:	case  76:	case  84:	case  92:	case 100:	case 108:	case 116:	case 124:
			case 132:	case 140:	case 148:	case 156:	case 164:	case 172:	case 180:	case 188:
			case 196:	case 204:	case 212:	case 220:	case 228:	case 236:	case 244:	case 252:
			case 324:	case 332:
				RenderAddr = PatAddr;
				break;
			case   5:	case  13:	case  21:	case  29:	case  37:	case  45:	case  53:	case  61:
			case  69:	case  77:	case  85:	case  93:	case 101:	case 109:	case 117:	case 125:
			case 133:	case 141:	case 149:	case 157:	case 165:	case 173:	case 181:	case 189:
			case 197:	case 205:	case 213:	case 221:	case 229:	case 237:	case 245:	case 253:
				if (Spr0InLine)
				{
					TC = ReverseCHR[RenderData[2]];
					CurTileData = &TileData[Clockticks + 11];
					((unsigned long *)CurTileData)[0] |= CHRLoBit[TC & 0xF];
					((unsigned long *)CurTileData)[1] |= CHRLoBit[TC >> 4];
				}
				break;
			case 325:	case 333:
				if (Spr0InLine)
				{
					TC = ReverseCHR[RenderData[2]];
					CurTileData = &TileData[Clockticks - 325];
					((unsigned long *)CurTileData)[0] |= CHRLoBit[TC & 0xF];
					((unsigned long *)CurTileData)[1] |= CHRLoBit[TC >> 4];
				}
				break;
			case   6:	case  14:	case  22:	case  30:	case  38:	case  46:	case  54:	case  62:
			case  70:	case  78:	case  86:	case  94:	case 102:	case 110:	case 118:	case 126:
			case 134:	case 142:	case 150:	case 158:	case 166:	case 174:	case 182:	case 190:
			case 198:	case 206:	case 214:	case 222:	case 230:	case 238:	case 246:	case 254:
			case 326:	case 334:
				RenderAddr = PatAddr | 8;
				break;
			case   7:	case  15:	case  23:	case  31:	case  39:	case  47:	case  55:	case  63:
			case  71:	case  79:	case  87:	case  95:	case 103:	case 111:	case 119:	case 127:
			case 135:	case 143:	case 151:	case 159:	case 167:	case 175:	case 183:	case 191:
			case 199:	case 207:	case 215:	case 223:	case 231:	case 239:	case 247:	case 255:
				if (Spr0InLine)
				{
					TC = ReverseCHR[RenderData[3]];
					CurTileData = &TileData[Clockticks + 9];
					((unsigned long *)CurTileData)[0] |= CHRHiBit[TC & 0xF];
					((unsigned long *)CurTileData)[1] |= CHRHiBit[TC >> 4];
				}
				break;
			case 327:	case 335:
				if (Spr0InLine)
				{
					TC = ReverseCHR[RenderData[3]];
					CurTileData = &TileData[Clockticks - 327];
					((unsigned long *)CurTileData)[0] |= CHRHiBit[TC & 0xF];
					((unsigned long *)CurTileData)[1] |= CHRHiBit[TC >> 4];
				}
				break;
				// END BACKGROUND
				// BEGIN SPRITES
			case 256:	case 264:	case 272:	case 280:	case 288:	case 296:	case 304:	case 312:
				RenderAddr = 0x2000 | (VRAMAddr & 0xFFF);
				break;
			case 257:
				VRAMAddr &= ~0x41F;
				VRAMAddr |= IntReg & 0x41F;
					case 265:	case 273:	case 281:	case 289:	case 297:	case 305:	case 313:
				break;
			case 258:	case 266:	case 274:	case 282:	case 290:	case 298:	case 306:	case 314:
				RenderAddr = 0x2000 | (VRAMAddr & 0xFFF);
				break;
			case 259:	case 267:	case 275:	case 283:	case 291:	case 299:	case 307:	case 315:
				SprNum = (Clockticks >> 1) & 0x1C;
				TC = SprBuff[SprNum | 1];
				if (Reg2000 & 0x20)
					PatAddr = ((TC & 0xFE) << 4) | ((TC & 0x01) << 12) | ((SprBuff[SprNum] & 7) ^ ((SprBuff[SprNum | 2] & 0x80) ? 0x17 : 0x00) ^ ((SprBuff[SprNum] & 0x8) << 1));
				else	PatAddr = (TC << 4) | (SprBuff[SprNum] ^ ((SprBuff[SprNum | 2] & 0x80) ? 0x7 : 0x0)) | ((Reg2000 & 0x08) << 9);
				break;
			case 260:	case 268:	case 276:	case 284:	case 292:	case 300:	case 308:	case 316:
				RenderAddr = PatAddr;
				break;
			case 261:
				if (Spr0InLine)
				{
					if (SprBuff[2] & 0x40)
						TC = RenderData[2];
					else	TC = ReverseCHR[RenderData[2]];
					((unsigned long *)SprData[0])[0] = CHRLoBit[TC & 0xF];
					((unsigned long *)SprData[0])[1] = CHRLoBit[TC >> 4];
				}
				break;
			case 269:	case 277:	case 285:	case 293:	case 301:	case 309:	case 317:
				break;
			case 262:	case 270:	case 278:	case 286:	case 294:	case 302:	case 310:	case 318:
				RenderAddr = PatAddr | 8;
				break;
			case 263:
				if (Spr0InLine)
				{
					SprNum = (Clockticks >> 1) & 0x1E;
					if (SprBuff[2] & 0x40)
						TC = RenderData[3];
					else	TC = ReverseCHR[RenderData[3]];
					((unsigned long *)SprData[0])[0] |= CHRHiBit[TC & 0xF];
					((unsigned long *)SprData[0])[1] |= CHRHiBit[TC >> 4];
				}
				break;
			case 271:	case 279:	case 287:	case 295:	case 303:	case 311:	case 319:
				break;
				// END SPRITES
			case 336:	case 338:
				RenderAddr = 0x2000 | (VRAMAddr & 0xFFF);
			case 337:	case 339:
				break;
			case 340:
				break;
			}
			if (!(Clockticks & 1))
			{
				PPUCycle(RenderAddr, SLnum, Clockticks, 1);
				if (IOMode == 2)
					WriteHandler[RenderAddr >> 10](RenderAddr >> 10, RenderAddr & 0x3FF, RenderAddr & 0xFF);
			}
		}
		if (IOMode)
		{
			unsigned short addr = (unsigned short)(IOAddr & 0x3FFF);
			if ((IOMode >= 5) && (!IsRendering))
				PPUCycle(addr, SLnum, Clockticks, IsRendering);
			else if (IOMode == 2)
			{
				if (!IsRendering)
					WriteHandler[addr >> 10](addr >> 10, addr & 0x3FF, IOVal);
			}
			else if (IOMode == 1)
			{
				IOMode++;
				if (!IsRendering)
				{
					if (ReadHandler[addr >> 10] == BusRead)
						buf2007 = CHRPointer[addr >> 10][addr & 0x3FF];
					else	buf2007 = (unsigned char)ReadHandler[addr >> 10](addr >> 10, addr & 0x3FF);
				}
			}
			IOMode -= 2;
		}
		if (!IsRendering && !IOMode)
			PPUCycle(VRAMAddr, SLnum, Clockticks, 0);
		if ((Spr0InLine) && (Clockticks < 255) && (OnScreen) && ((Reg2001 & 0x18) == 0x18) && ((Clockticks >= 8) || ((Reg2001 & 0x06) == 0x06)))
		{
			register int SprPixel = Clockticks - SprBuff[3];
			if (!(SprPixel & ~7) && (SprData[0][SprPixel] & 0x3) && (TileData[Clockticks + IntX] & 0x3))
				Reg2002 |= 0x40;	// Sprite 0 hit
		}
	}
}

void	Run (void)
{
	if (IsPAL)
	{
		register int cycles = 3;
		if (++PALsubticks == 5)
		{
			PALsubticks = 0;
			cycles = 4;
		}
		if (GFX::FPSCnt < GFX::FSkip)
			RunSkip(cycles);
		else	RunNoSkip(cycles);
	}
	else
	{
		if (GFX::FPSCnt < GFX::FSkip)
			RunSkip(3);
		else	RunNoSkip(3);
	}
}

int	__fastcall	Read01356 (void)
{
	return readLatch;
}

int	__fastcall	Read2 (void)
{
	register unsigned char tmp;
	HVTog = TRUE;
	tmp = Reg2002 | (readLatch & 0x1F);
	if (tmp & 0x80)
		Reg2002 &= 0x60;
	// race conditions
	if (SLnum == 241)
	{
		if ((Clockticks == 0))
			tmp &= ~0x80;
		if (Clockticks < 3)
			CPU::WantNMI = FALSE;
	}
	return readLatch = tmp;
}

int	__fastcall	Read4 (void)
{
	if (IsRendering)
#ifdef	ACCURATE_SPRITES
		readLatch = Sprite[SpritePtr];
#else	/* !ACCURATE_SPRITES */
	{
		if (Clockticks < 64)
			readLatch = 0xFF;
		else if (Clockticks < 192)
			readLatch = Sprite[((Clockticks - 64) << 1) & 0xFC];
		else if (Clockticks < 256)
			readLatch = (Clockticks & 1) ? Sprite[0xFC] : Sprite[((Clockticks - 192) << 1) & 0xFC];
		else if (Clockticks < 320)
			readLatch = 0xFF;
		else	readLatch = Sprite[0];
	}
#endif	/* ACCURATE_SPRITES */
	else	readLatch = Sprite[SprAddr];
	return readLatch;
}

int	__fastcall	Read7 (void)
{
	IOAddr = VRAMAddr & 0x3FFF;
	IOMode = 5;
	if (IsRendering)
	{
		// while rendering, it seems to drop by 1 scanline, regardless of increment mode
		if ((VRAMAddr & 0x7000) == 0x7000)
		{
			register int YScroll = VRAMAddr & 0x3E0;
			VRAMAddr &= 0xFFF;
			if (YScroll == 0x3A0)
				VRAMAddr ^= 0xBA0;
			else if (YScroll == 0x3E0)
				VRAMAddr ^= 0x3E0;
			else	VRAMAddr += 0x20;
		}
		else	VRAMAddr += 0x1000;
	}
	else
	{
		if (Reg2000 & 0x04)
			VRAMAddr += 32;
		else	VRAMAddr++;
		VRAMAddr &= 0x7FFF;
	}
	if ((IOAddr & 0x3F00) == 0x3F00)
	{
		if (Reg2001 & 0x01)
			return readLatch = Palette[IOAddr & 0x1F] & 0x30;
		else	return readLatch = Palette[IOAddr & 0x1F];
	}
	else	return readLatch = buf2007;
}
typedef int (__fastcall *PPU_IntRead)(void);
int	MAPINT	IntRead (int Bank, int Addr)
{
	const PPU_IntRead funcs[8] = {Read01356,Read01356,Read2,Read01356,Read4,Read01356,Read01356,Read7};
	return funcs[Addr & 7]();
}

void	__fastcall	Write0 (int Val)
{
	if ((Val & 0x80) && !(Reg2000 & 0x80) && (Reg2002 & 0x80))
		CPU::WantNMI = TRUE;
	// race condition
	if ((SLnum == 241) && !(Val & 0x80) && (Clockticks < 3))
		CPU::WantNMI = FALSE;
#ifdef	ENABLE_DEBUGGER
	if ((Reg2000 ^ Val) & 0x28)
		Debugger::SprChanged = TRUE;
	if ((Reg2000 ^ Val) & 0x10)
		Debugger::NTabChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
	Reg2000 = (unsigned char)Val;
	IntReg &= 0x73FF;
	IntReg |= (Val & 3) << 10;
}

void	__fastcall	Write1 (int Val)
{
	Reg2001 = (unsigned char)Val;
	if ((Val & 0x18) && (SLnum < 240))
		IsRendering = TRUE;
	else	IsRendering = FALSE;
	ColorEmphasis = (Val & 0xE0) << 1;
	GrayScale = (Val & 0x01) ? 0x30 : 0x3F;
}

void	__fastcall	Write2 (int Val)
{
}

void	__fastcall	Write3 (int Val)
{
	SprAddr = (unsigned char)Val;
}

void	__fastcall	Write4 (int Val)
{
	if (IsRendering)
		Val = 0xFF;
	if ((SprAddr & 0x03) == 0x02)
		Val &= 0xE3;
	Sprite[SprAddr++] = (unsigned char)Val;
#ifdef	ENABLE_DEBUGGER
	Debugger::SprChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
}

void	__fastcall	Write5 (int Val)
{
	if (HVTog)
	{
		IntReg &= 0x7FE0;
		IntReg |= (Val & 0xF8) >> 3;
		IntX = (unsigned char)(Val & 7);
	}
	else
	{
		IntReg &= 0x0C1F;
		IntReg |= (Val & 0x07) << 12;
		IntReg |= (Val & 0xF8) << 2;
	}
	HVTog = !HVTog;
}

void	__fastcall	Write6 (int Val)
{
	if (HVTog)
	{
		IntReg &= 0x00FF;
		IntReg |= (Val & 0x3F) << 8;
	}
	else
	{
		IntReg &= 0x7F00;
		IntReg |= Val;
		VRAMAddr = IntReg;
	}
	HVTog = !HVTog;
}

void	__fastcall	Write7 (int Val)
{
	if ((VRAMAddr & 0x3F00) == 0x3F00)
	{
		register unsigned char Addr = (unsigned char)VRAMAddr & 0x1F;
		Val = Val & 0x3F;
#ifdef	ENABLE_DEBUGGER
		Debugger::PalChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
		Palette[Addr] = (unsigned char)Val;
		if (!(Addr & 0x3))
			Palette[Addr ^ 0x10] = (unsigned char)Val;
	}
	else
	{
		IOAddr = VRAMAddr & 0x3FFF;
		IOVal = (unsigned char)Val;
		IOMode = 6;
	}
	if (IsRendering)
	{
		// while rendering, it seems to drop by 1 scanline, regardless of increment mode
		if ((VRAMAddr & 0x7000) == 0x7000)
		{
			register int YScroll = VRAMAddr & 0x3E0;
			VRAMAddr &= 0xFFF;
			if (YScroll == 0x3A0)
				VRAMAddr ^= 0xBA0;
			else if (YScroll == 0x3E0)
				VRAMAddr ^= 0x3E0;
			else	VRAMAddr += 0x20;
		}
		else	VRAMAddr += 0x1000;
	}
	else
	{
		if (Reg2000 & 0x04)
			VRAMAddr += 32;
		else	VRAMAddr++;
		VRAMAddr &= 0x7FFF;
	}
}

typedef void (__fastcall *PPU_IntWrite)(int Val);
void	MAPINT	IntWrite (int Bank, int Addr, int Val)
{
	const PPU_IntWrite funcs[8] = {Write0,Write1,Write2,Write3,Write4,Write5,Write6,Write7};
	readLatch = (unsigned char)Val;
	funcs[Addr & 7](Val);
}
} // namespace PPU