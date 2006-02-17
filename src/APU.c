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

#ifdef	NSFPLAYER
# include "in_nintendulator.h"
# include "APU.h"
# include "CPU.h"
#else
# include "Nintendulator.h"
# include "NES.h"
# include "APU.h"
# include "CPU.h"
# include "PPU.h"
#endif

#define	SOUND_FILTERING
//#define	SOUND_LOGGING

#ifdef	NSFPLAYER
#undef	SOUND_LOGGING
#endif

struct tAPU APU;

#ifndef	NSFPLAYER
#define	FREQ		44100
#define	BITS		16
#define	FRAMEBUF	4
const	unsigned int	LOCK_SIZE = FREQ * (BITS / 8);
#endif

const	unsigned char	LengthCounts[32] = {0x0A,0xFE,0x14,0x02,0x28,0x04,0x50,0x06,0xA0,0x08,0x3C,0x0A,0x0E,0x0C,0x1A,0x0E,0x0C,0x10,0x18,0x12,0x30,0x14,0x60,0x16,0xC0,0x18,0x48,0x1A,0x10,0x1C,0x20,0x1E};

static	struct
{
	unsigned char volume, envelope, wavehold, duty, swpspeed, swpdir, swpstep, swpenab;
	unsigned long freq;
	int Vol;
	unsigned char CurD;
	int Timer;
	int EnvCtr, Envelope, BendCtr;
	BOOL Enabled, ValidFreq, Active;
	unsigned long Cycles;
	signed long Pos;
}	Square0, Square1;
const	signed char	Duties[4][16] = {{+4,+4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4},{+4,+4,+4,+4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4},{+4,+4,+4,+4,+4,+4,+4,+4,-4,-4,-4,-4,-4,-4,-4,-4},{+4,+4,+4,+4,+4,+4,+4,+4,+4,+4,+4,+4,-4,-4,-4,-4}};
__inline void	Square0_CheckActive (void)
{
	if (Square0.ValidFreq = ((Square0.freq >= 0x8) && (Square0.swpdir || !(Square0.freq + (Square0.freq >> Square0.swpstep) & 0x800))))
		Square0.Active = Square0.Enabled && Square0.Timer;
	else	Square0.Active = FALSE;
	if (Square0.Active)
		Square0.Pos = Duties[Square0.duty][Square0.CurD] * Square0.Vol;
	else	Square0.Pos = 0;
}
__inline void	Square0_Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	Square0.volume = Val & 0xF;
		Square0.EnvCtr = Square0.volume + 1;
		Square0.envelope = Val & 0x10;
		Square0.wavehold = Val & 0x20;
		Square0.duty = (Val >> 6) & 0x3;
		Square0.Vol = Square0.envelope ? Square0.volume : Square0.Envelope;
		break;
	case 1:	Square0.swpstep = Val & 0x07;
		Square0.swpdir = Val & 0x08;
		Square0.swpspeed = (Val >> 4) & 0x7;
		Square0.swpenab = Val & 0x80;
		break;
	case 2:	Square0.freq &= 0x700;
		Square0.freq |= Val;
		break;
	case 3:	Square0.freq &= 0xFF;
		Square0.freq |= (Val & 0x7) << 8;
		if (Square0.Enabled)
			Square0.Timer = LengthCounts[(Val >> 3) & 0x1F];
		Square0.Envelope = 0xF;
		Square0.CurD = 0;
		break;
	case 4:	if (!(Square0.Enabled = Val ? TRUE : FALSE))
			Square0.Timer = 0;
		break;
	}
	Square0_CheckActive();
}
__inline void	Square0_Run (void)
{
	if (Square0.Active && !--Square0.Cycles)
	{
		Square0.Cycles = Square0.freq + 1;
		Square0.CurD = (Square0.CurD + 1) & 0xF;
		Square0.Pos = Duties[Square0.duty][Square0.CurD] * Square0.Vol;
	}
}
__inline void	Square0_QuarterFrame (void)
{
	if (Square0.Active && !--Square0.EnvCtr)
	{
		Square0.EnvCtr = Square0.volume + 1;
		if (Square0.Envelope)
			Square0.Envelope--;
		else	Square0.Envelope = Square0.wavehold ? 0xF : 0x0;
		Square0.Vol = Square0.envelope ? Square0.volume : Square0.Envelope;
	}
}
__inline void	Square0_HalfFrame (void)
{
	if (Square0.Active && Square0.swpenab && !--Square0.BendCtr)
	{
		Square0.BendCtr = Square0.swpspeed + 1;
		if (Square0.swpstep)
		{
			int sweep = Square0.freq >> Square0.swpstep;
			Square0.freq += Square0.swpdir ? ~sweep : sweep;
		}
	}
	if (Square0.Active && !Square0.wavehold)
		Square0.Timer--;
	Square0_CheckActive();
}
__inline void	Square1_CheckActive (void)
{
	if (Square1.ValidFreq = ((Square1.freq >= 0x8) && (Square1.swpdir || !(Square1.freq + (Square1.freq >> Square1.swpstep) & 0x800))))
		Square1.Active = Square1.Enabled && Square1.Timer;
	else	Square1.Active = FALSE;
	if (Square1.Active)
		Square1.Pos = Duties[Square1.duty][Square1.CurD] * Square1.Vol;
	else	Square1.Pos = 0;
}
__inline void	Square1_Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	Square1.volume = Val & 0xF;
		Square1.EnvCtr = Square1.volume + 1;
		Square1.envelope = Val & 0x10;
		Square1.wavehold = Val & 0x20;
		Square1.duty = (Val >> 6) & 0x3;
		Square1.Vol = Square1.envelope ? Square1.volume : Square1.Envelope;
		break;
	case 1:	Square1.swpstep = Val & 0x07;
		Square1.swpdir = Val & 0x08;
		Square1.swpspeed = (Val >> 4) & 0x7;
		Square1.swpenab = Val & 0x80;
		break;
	case 2:	Square1.freq &= 0x700;
		Square1.freq |= Val;
		break;
	case 3:	Square1.freq &= 0xFF;
		Square1.freq |= (Val & 0x7) << 8;
		if (Square1.Enabled)
			Square1.Timer = LengthCounts[(Val >> 3) & 0x1F];
		Square1.Envelope = 0xF;
		Square1.CurD = 0;
		break;
	case 4:	if (!(Square1.Enabled = Val ? TRUE : FALSE))
			Square1.Timer = 0;
		break;
	}
	Square1_CheckActive();
}
__inline void	Square1_Run (void)
{
	if (Square1.Active && !--Square1.Cycles)
	{
		Square1.Cycles = Square1.freq + 1;
		Square1.CurD = (Square1.CurD + 1) & 0xF;
		Square1.Pos = Duties[Square1.duty][Square1.CurD] * Square1.Vol;
	}
}
__inline void	Square1_QuarterFrame (void)
{
	if (Square1.Active && !--Square1.EnvCtr)
	{
		Square1.EnvCtr = Square1.volume + 1;
		if (Square1.Envelope)
			Square1.Envelope--;
		else	Square1.Envelope = Square1.wavehold ? 0xF : 0x0;
		Square1.Vol = Square1.envelope ? Square1.volume : Square1.Envelope;
	}
}
__inline void	Square1_HalfFrame (void)
{
	if (Square1.Active && Square1.swpenab && !--Square1.BendCtr)
	{
		Square1.BendCtr = Square1.swpspeed + 1;
		if (Square1.swpstep)
		{
			int sweep = Square1.freq >> Square1.swpstep;
			Square1.freq += Square1.swpdir ? -sweep : sweep;
		}
	}
	if (Square1.Active && !Square1.wavehold)
		Square1.Timer--;
	Square1_CheckActive();
}

static	struct
{
	unsigned char linear, wavehold;
	unsigned long freq;
	unsigned char CurD;
	int Timer, LinCtr, LinMode;
	BOOL Enabled, Active;
	unsigned long Cycles;
	signed long Pos;
}	Triangle;
const	signed char	TriDuty[32] = {-8,-7,-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0,-1,-2,-3,-4,-5,-6,-7,-8};
__inline void	Triangle_CheckActive (void)
{
	if (Triangle.freq >= 4)
		Triangle.Active = Triangle.Enabled && Triangle.Timer && Triangle.LinCtr;
	else	Triangle.Active = FALSE;
	if (!Triangle.Active)
		Triangle.Pos = 0;
}
__inline void	Triangle_Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	Triangle.linear = Val & 0x7F;
		Triangle.wavehold = (Val >> 7) & 0x1;
		break;
	case 2:	Triangle.freq &= 0x700;
		Triangle.freq |= Val;
		break;
	case 3:	Triangle.freq &= 0xFF;
		Triangle.freq |= (Val & 0x7) << 8;
		if (Triangle.Enabled)
			Triangle.Timer = LengthCounts[(Val >> 3) & 0x1F];
		Triangle.LinMode = 1;
		break;
	case 4:	if (!(Triangle.Enabled = Val ? TRUE : FALSE))
			Triangle.Timer = 0;
		break;
	}
	Triangle_CheckActive();
}
__inline void	Triangle_Run (void)
{
	if (Triangle.Active && !--Triangle.Cycles)
	{
		Triangle.Cycles = Triangle.freq + 1;
		Triangle.CurD++;
		Triangle.CurD &= 0x1F;
		Triangle.Pos = TriDuty[Triangle.CurD] * 8;
	}
}
__inline void	Triangle_QuarterFrame (void)
{
	if (Triangle.Enabled)
	{
		if (Triangle.LinMode)
			Triangle.LinCtr = Triangle.linear;
		else	if (Triangle.LinCtr)
			Triangle.LinCtr--;
		if (!Triangle.wavehold)
			Triangle.LinMode = 0;
	}
	Triangle_CheckActive();
}
__inline void	Triangle_HalfFrame (void)
{
	if (Triangle.Active && !Triangle.wavehold)
		Triangle.Timer--;
	Triangle_CheckActive();
}

static	struct
{
	unsigned char volume, envelope, wavehold, datatype;
	unsigned long freq;
	unsigned long CurD;
	int Vol;
	int Timer;
	int EnvCtr, Envelope;
	BOOL Enabled, Active;
	unsigned long Cycles;
	signed long Pos;
}	Noise;
const	unsigned long	NoiseFreq[16] = {0x004,0x008,0x010,0x020,0x040,0x060,0x080,0x0A0,0x0CA,0x0FE,0x17C,0x1FC,0x2FA,0x3F8,0x7F2,0xFE4};
__inline void	Noise_CheckActive (void)
{
	if (!(Noise.Active = Noise.Enabled && Noise.Timer))
		Noise.Pos = 0;
}
__inline void	Noise_Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	Noise.volume = Val & 0x0F;
		Noise.envelope = Val & 0x10;
		Noise.wavehold = Val & 0x20;
		Noise.Vol = Noise.envelope ? Noise.volume : Noise.Envelope;
		Noise.Pos = ((Noise.CurD & 0x4000) ? -2 : 2) * Noise.Vol;
		break;
	case 2:	Noise.freq = Val & 0xF;
		Noise.datatype = Val & 0x80;
		break;
	case 3:	if (Noise.Enabled)
			Noise.Timer = LengthCounts[(Val >> 3) & 0x1F];
		Noise.Envelope = 0xF;
		break;
	case 4:	if (!(Noise.Enabled = Val ? TRUE : FALSE))
			Noise.Timer = 0;
		break;
	}
	Noise_CheckActive();
}
__inline void	Noise_Run (void)
{
	if (Noise.Active && !--Noise.Cycles)
	{
		Noise.Cycles = NoiseFreq[Noise.freq];	// no + 1 here
		if (Noise.datatype)
			Noise.CurD = (Noise.CurD << 1) | (((Noise.CurD >> 14) ^ (Noise.CurD >> 8)) & 0x1);
		else	Noise.CurD = (Noise.CurD << 1) | (((Noise.CurD >> 14) ^ (Noise.CurD >> 13)) & 0x1);
		Noise.Pos = ((Noise.CurD & 0x4000) ? -2 : 2) * Noise.Vol;
	}
}
__inline void	Noise_QuarterFrame (void)
{
	if (Noise.Active && !--Noise.EnvCtr)
	{
		Noise.EnvCtr = Noise.volume + 1;
		if (Noise.Envelope)
			Noise.Envelope--;
		else	Noise.Envelope = Noise.wavehold ? 0xF : 0x0;
		Noise.Vol = Noise.envelope ? Noise.volume : Noise.Envelope;
	}
}
__inline void	Noise_HalfFrame (void)
{
	if (Noise.Active && !Noise.wavehold)
		Noise.Timer--;
	Noise_CheckActive();
}

static	struct
{
	unsigned char freq, wavehold, doirq, pcmdata, addr, len;
	unsigned long CurAddr, SampleLen;
	unsigned char shiftreg, outmode, outbits, buffer, buffull;
	int LengthCtr;
//	BOOL Enabled;
	unsigned long Cycles;
	signed long Pos;
}	DPCM;
const	unsigned long	DPCMFreqNTSC[16] = {0x1AC,0x17C,0x154,0x140,0x11E,0x0FE,0x0E2,0x0D6,0x0BE,0x0A0,0x08E,0x080,0x06A,0x054,0x048,0x036};
const	unsigned long	DPCMFreqPAL[16]  = {0x18E,0x161,0x13C,0x129,0x10A,0x0EC,0x0D2,0x0C7,0x0B1,0x095,0x084,0x077,0x062,0x04E,0x043,0x032};
const	unsigned long	*DPCMFreq;
__inline void	DPCM_Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	DPCM.freq = Val & 0xF;
		DPCM.wavehold = (Val >> 6) & 0x1;
		DPCM.doirq = Val >> 7;
		if (!DPCM.doirq)
			CPU.WantIRQ &= ~IRQ_DPCM;
		break;
	case 1:	DPCM.pcmdata = Val & 0x7F;
		DPCM.Pos = (DPCM.pcmdata - 0x40) * 3;
		break;
	case 2:	DPCM.addr = Val;
		break;
	case 3:	DPCM.len = Val;
		break;
	case 4:	if (Val)
		{
			if (!DPCM.LengthCtr)
			{
				DPCM.CurAddr = 0xC000 | (DPCM.addr << 6);
				DPCM.LengthCtr = (DPCM.len << 4) + 1;
			}
		}
		else	DPCM.LengthCtr = 0;
		CPU.WantIRQ &= ~IRQ_DPCM;
		break;
	}
}
__inline void	DPCM_Run (void)
{
	if (!--DPCM.Cycles)
	{
		DPCM.Cycles = DPCMFreq[DPCM.freq];
		if (DPCM.outmode == 1)
		{
			if (DPCM.shiftreg & 1)
			{
				if (DPCM.pcmdata <= 0x7D)
					DPCM.pcmdata += 2;
			}
			else
			{
				if (DPCM.pcmdata >= 0x02)
					DPCM.pcmdata -= 2;
			}
			DPCM.shiftreg >>= 1;
			DPCM.Pos = (DPCM.pcmdata - 0x40) * 3;
		}
		if (!--DPCM.outbits)
		{
			DPCM.outbits = 8;
			if (DPCM.buffull)
			{
				DPCM.outmode = 1;
				DPCM.shiftreg = DPCM.buffer;
				DPCM.buffull = FALSE;
			}
			else	DPCM.outmode = 0;
		}
	}
}

void	DPCM_Fetch (void)
{
	if (DPCM.buffull || !DPCM.LengthCtr)
		return;
	DPCM.buffer = CPU_MemGet(DPCM.CurAddr);	// apparently, DPCM fetches take FOUR cycles
	DPCM.buffer = CPU_MemGet(DPCM.CurAddr);
	DPCM.buffer = CPU_MemGet(DPCM.CurAddr);
	DPCM.buffer = CPU_MemGet(DPCM.CurAddr);	// weird, huh?
	DPCM.buffull = TRUE;
	if (++DPCM.CurAddr == 0x10000)
		DPCM.CurAddr = 0x8000;
	if (!--DPCM.LengthCtr)
	{
		if (DPCM.wavehold)
		{
			DPCM.CurAddr = 0xC000 | (DPCM.addr << 6);
			DPCM.LengthCtr = (DPCM.len << 4) + 1;
		}
		else	if (DPCM.doirq)
			CPU.WantIRQ |= IRQ_DPCM;
	}
}

static	struct
{
	unsigned char Bits;
	unsigned long Cycles;
	int Num;
}	Frame;
__inline void	Frame_Write (unsigned char Val)
{
	Frame.Bits = Val & 0xC0;
	Frame.Num = 0;
	if (Frame.Bits & 0x80)
		Frame.Cycles = 1;
	else	Frame.Cycles = APU.QuarterFrameLen;
	CPU.WantIRQ &= ~IRQ_FRAME;
}
__inline void	Frame_Run (void)
{
	if (!--Frame.Cycles)
	{
		switch (Frame.Num++)
		{
		case 0:	Square0_QuarterFrame();
			Square1_QuarterFrame();
			Triangle_QuarterFrame();
			Noise_QuarterFrame();
			Frame.Cycles = APU.QuarterFrameLen;	break;
		case 1:	Square0_QuarterFrame();
			Square1_QuarterFrame();
			Triangle_QuarterFrame();
			Noise_QuarterFrame();
			Square0_HalfFrame();
			Square1_HalfFrame();
			Triangle_HalfFrame();
			Noise_HalfFrame();
			Frame.Cycles = APU.QuarterFrameLen;	break;
		case 2:	Square0_QuarterFrame();
			Square1_QuarterFrame();
			Triangle_QuarterFrame();
			Noise_QuarterFrame();
			Frame.Cycles = APU.QuarterFrameLen;	break;
		case 3:	Square0_QuarterFrame();
			Square1_QuarterFrame();
			Triangle_QuarterFrame();
			Noise_QuarterFrame();
			Square0_HalfFrame();
			Square1_HalfFrame();
			Triangle_HalfFrame();
			Noise_HalfFrame();
			if (!(Frame.Bits & 0xC0))	// 0x40
				CPU.WantIRQ |= IRQ_FRAME;
			Frame.Num = 0;
			if (Frame.Bits & 0x80)
				Frame.Cycles = 2 * APU.QuarterFrameLen;
			else	Frame.Cycles = APU.QuarterFrameLen;
								break;
		}
	}
}

void	APU_WriteReg (int Addy, unsigned char Val)
{
#ifndef	NSFPLAYER
	APU.Regs[Addy] = Val;
#endif
	switch (Addy)
	{
	case 0x000:	Square0_Write(0,Val);	break;
	case 0x001:	Square0_Write(1,Val);	break;
	case 0x002:	Square0_Write(2,Val);	break;
	case 0x003:	Square0_Write(3,Val);	break;
	case 0x004:	Square1_Write(0,Val);	break;
	case 0x005:	Square1_Write(1,Val);	break;
	case 0x006:	Square1_Write(2,Val);	break;
	case 0x007:	Square1_Write(3,Val);	break;
	case 0x008:	Triangle_Write(0,Val);	break;
	case 0x009:	Triangle_Write(1,Val);	break;
	case 0x00A:	Triangle_Write(2,Val);	break;
	case 0x00B:	Triangle_Write(3,Val);	break;
	case 0x00C:	Noise_Write(0,Val);	break;
	case 0x00D:	Noise_Write(1,Val);	break;
	case 0x00E:	Noise_Write(2,Val);	break;
	case 0x00F:	Noise_Write(3,Val);	break;
	case 0x010:	DPCM_Write(0,Val);	break;
	case 0x011:	DPCM_Write(1,Val);	break;
	case 0x012:	DPCM_Write(2,Val);	break;
	case 0x013:	DPCM_Write(3,Val);	break;
	case 0x015:	Square0_Write(4,Val & 0x1);
			Square1_Write(4,Val & 0x2);
			Triangle_Write(4,Val & 0x4);
			Noise_Write(4,Val & 0x8);
			DPCM_Write(4,Val & 0x10);
						break;
	case 0x017:
#ifndef	NSFPLAYER
			APU.Regs[0x014] = Val;
#endif
			Frame_Write(Val);	break;
#ifndef	NSFPLAYER
	default:	MessageBox(mWnd,"WTF? Invalid sound write!","Nintendulator",MB_OK);
#else
	default:	MessageBox(mod.hMainWindow,"WTF? Invalid sound write!","Nintendulator",MB_OK);
#endif
						break;
	}
}
unsigned char	APU_Read4015 (void)
{
	unsigned char result =
		(( Square0.Enabled &&  Square0.Active) ? 0x01 : 0) |
		(( Square1.Enabled &&  Square1.Active) ? 0x02 : 0) |
		((Triangle.Enabled && Triangle.Active) ? 0x04 : 0) |
		((   Noise.Enabled &&    Noise.Active) ? 0x08 : 0) |
		((                     DPCM.LengthCtr) ? 0x10 : 0) |
		((    (CPU.WantIRQ &       IRQ_FRAME)) ? 0x40 : 0) |
		((    (CPU.WantIRQ &        IRQ_DPCM)) ? 0x80 : 0);
	CPU.WantIRQ &= ~(IRQ_FRAME | IRQ_DPCM);
	return result;
}

#ifdef	NSFPLAYER
#define	APU_Try(action,errormsg)\
{\
	if (FAILED(action))\
	{\
		APU_Release();\
		APU_Create();\
		if (FAILED(action))\
		{\
			APU_SoundOFF();\
			MessageBox(mod.hMainWindow,errormsg,"Nintendulator",MB_OK | MB_ICONERROR);\
			return;\
		}\
	}\
}
#else
#define	APU_Try(action,errormsg)\
{\
	if (FAILED(action))\
	{\
		APU_Release();\
		APU_Create();\
		if (FAILED(action))\
		{\
			APU_SoundOFF();\
			MessageBox(mWnd,errormsg,"Nintendulator",MB_OK | MB_ICONERROR);\
			return;\
		}\
	}\
}
#endif

void	APU_SetFPSVars (int FPS)
{
	APU.WantFPS = FPS;
	if (APU.WantFPS == 60)
	{
		APU.MHz = 1789773;
		APU.QuarterFrameLen = 7457;
		DPCMFreq = DPCMFreqNTSC;
	}
	else if (APU.WantFPS == 50)
	{
		APU.MHz = 1662607;
		APU.QuarterFrameLen = 8313;
		DPCMFreq = DPCMFreqPAL;
	}
	else
	{
#ifndef	NSFPLAYER
		MessageBox(mWnd,"Attempted to set indeterminate sound framerate!","Nintendulator",MB_OK | MB_ICONERROR);
#else
		MessageBox(mod.hMainWindow,"Attempted to set indeterminate sound framerate!","Nintendulator",MB_OK | MB_ICONERROR);
#endif
		return;
	}
#ifndef	NSFPLAYER
	APU.LockSize = LOCK_SIZE / APU.WantFPS;
#endif
}

void	APU_SetFPS (int FPS)
{
#ifndef	NSFPLAYER
	BOOL Enabled = APU.isEnabled;
	APU_Release();
#endif
	APU_SetFPSVars(FPS);
#ifndef	NSFPLAYER
	APU_Create();
	if (Enabled)
		APU_SoundON();
#endif
}

void	APU_Init (void)
{
#ifndef	NSFPLAYER
	ZeroMemory(APU.Regs,sizeof(APU.Regs));
	APU.DirectSound		= NULL;
	APU.PrimaryBuffer	= NULL;
	APU.Buffer		= NULL;
	APU.buffer		= NULL;
	APU.isEnabled		= FALSE;
#endif
	APU.WantFPS		= 0;
	APU.MHz			= 1;
#ifndef	NSFPLAYER
	APU.LockSize		= 0;
#endif
	APU.QuarterFrameLen	= 0;
	APU.Cycles		= 0;
	APU.BufPos		= 0;
#ifndef	NSFPLAYER
	APU.last_pos		= 0;
#endif
}

void	APU_Create (void)
{
#ifndef	NSFPLAYER
	DSBUFFERDESC DSBD;
	WAVEFORMATEX WFX;
#endif

	if (!APU.WantFPS)
		APU_SetFPSVars(60);

#ifndef	NSFPLAYER
	if (FAILED(DirectSoundCreate(NULL,&APU.DirectSound,NULL)))
	{
		MessageBox(mWnd,"Failed to create DirectSound interface!","Nintendulator",MB_OK);
		return;
	}

	if (FAILED(IDirectSound_SetCooperativeLevel(APU.DirectSound,mWnd,DSSCL_PRIORITY)))
	{
		IDirectSound_Release(APU.DirectSound);
		MessageBox(mWnd,"Failed to set cooperative level!","Nintendulator",MB_OK);
		return;
	}

	ZeroMemory(&DSBD,sizeof(DSBUFFERDESC));
	DSBD.dwSize = sizeof(DSBUFFERDESC);
	DSBD.dwFlags = DSBCAPS_PRIMARYBUFFER;
	DSBD.dwBufferBytes = 0;
	DSBD.lpwfxFormat = NULL;
	if (FAILED(IDirectSound_CreateSoundBuffer(APU.DirectSound,&DSBD,&APU.PrimaryBuffer,NULL)))
	{
		IDirectSound_Release(APU.DirectSound);
		MessageBox(mWnd,"Failed to create primary buffer!","Nintendulator",MB_OK);
		return;
	}

	ZeroMemory(&WFX,sizeof(WAVEFORMATEX));
	WFX.wFormatTag = WAVE_FORMAT_PCM;
	WFX.nChannels = 1;
	WFX.nSamplesPerSec = FREQ;
	WFX.wBitsPerSample = BITS;
	WFX.nBlockAlign = WFX.wBitsPerSample / 8 * WFX.nChannels;
	WFX.nAvgBytesPerSec = WFX.nSamplesPerSec * WFX.nBlockAlign;
	if (FAILED(IDirectSoundBuffer_SetFormat(APU.PrimaryBuffer,&WFX)))
	{
		IDirectSoundBuffer_Release(APU.PrimaryBuffer);
		IDirectSound_Release(APU.DirectSound);
		MessageBox(mWnd,"Failed to set output format!","Nintendulator",MB_OK);
		return;
	}
	if (FAILED(IDirectSoundBuffer_Play(APU.PrimaryBuffer,0,0,DSBPLAY_LOOPING)))
	{
		IDirectSoundBuffer_Release(APU.PrimaryBuffer);
		IDirectSound_Release(APU.DirectSound);
		MessageBox(mWnd,"Failed to start playing primary buffer!","Nintendulator",MB_OK);
		return;
	}

	DSBD.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCSOFTWARE | DSBCAPS_GETCURRENTPOSITION2;
	DSBD.dwBufferBytes = APU.LockSize * FRAMEBUF;
	DSBD.lpwfxFormat = &WFX;

	if (FAILED(IDirectSound_CreateSoundBuffer(APU.DirectSound,&DSBD,&APU.Buffer,NULL)))
	{
		IDirectSoundBuffer_Release(APU.PrimaryBuffer);
		IDirectSound_Release(APU.DirectSound);
		MessageBox(mWnd,"Failed to create secondary buffer!","Nintendulator",MB_OK);
		return;
	}

	APU.buffer = (short *)malloc(APU.LockSize);
#endif
}

void	APU_Release (void)
{
#ifndef	NSFPLAYER
	if (APU.Buffer)
	{
		APU_SoundOFF();
		IDirectSoundBuffer_Release(APU.Buffer);
	}								APU.Buffer = NULL;
	if (APU.PrimaryBuffer)
	{
		IDirectSoundBuffer_Stop(APU.PrimaryBuffer);
		IDirectSoundBuffer_Release(APU.PrimaryBuffer);
	}								APU.PrimaryBuffer = NULL;
	if (APU.DirectSound)	IDirectSound_Release(APU.DirectSound);	APU.DirectSound = NULL;
	if (APU.buffer)		free(APU.buffer);			APU.buffer = NULL;
#endif
}

void	APU_Reset  (void)
{
#ifndef	NSFPLAYER
	ZeroMemory(APU.Regs,0x18);
#endif
	ZeroMemory(&Frame,sizeof(Frame));
	ZeroMemory(&Square0,sizeof(Square0));
	ZeroMemory(&Square1,sizeof(Square1));
	ZeroMemory(&Triangle,sizeof(Triangle));
	ZeroMemory(&Noise,sizeof(Noise));
	ZeroMemory(&DPCM,sizeof(DPCM));
	Noise.CurD = 1;
	APU.Cycles = 1;
	Square0.Cycles = 1;
	Square0.EnvCtr = 1;
	Square0.BendCtr = 1;
	Square1.Cycles = 1;
	Square1.EnvCtr = 1;
	Square1.BendCtr = 1;
	Triangle.Cycles = 1;
	Noise.Cycles = 1;
	Noise.EnvCtr = 1;
	DPCM.Cycles = 1;
	Frame.Cycles = 1;
	Frame.Bits = 0x40;
}
#ifdef	SOUND_LOGGING
FILE *soundlog = NULL;
#endif

#ifndef	NSFPLAYER
void	APU_SoundOFF (void)
{
	APU.isEnabled = FALSE;
	if (APU.Buffer)
		IDirectSoundBuffer_Stop(APU.Buffer);
#ifdef	SOUND_LOGGING
	if (soundlog)
	{
		fclose(soundlog);
		soundlog = NULL;
	}
#endif
}

void	APU_SoundON (void)
{
	LPVOID bufPtr;
	DWORD bufBytes;
	if ((!APU.buffer) || (!APU.Buffer))
	{
		MessageBox(mWnd,"Sound framerate indeterminate!","Nintendulator",MB_OK | MB_ICONSTOP);
		return;
	}
	APU_Try(IDirectSoundBuffer_Lock(APU.Buffer,0,0,&bufPtr,&bufBytes,NULL,0,DSBLOCK_ENTIREBUFFER),"Error locking sound buffer (Clear)")
	ZeroMemory(bufPtr,bufBytes);
	APU_Try(IDirectSoundBuffer_Unlock(APU.Buffer,bufPtr,bufBytes,NULL,0),"Error unlocking sound buffer (Clear)")
	APU.isEnabled = TRUE;
	APU_Try(IDirectSoundBuffer_Play(APU.Buffer,0,0,DSBPLAY_LOOPING),"Unable to start playing buffer");
	APU.last_pos = 0;
#ifdef	SOUND_LOGGING
	if (!soundlog)
		soundlog = fopen("c:\\nes.pcm","wb");
#endif
}

void	APU_Config (HWND hWnd)
{
}

void	APU_GetSoundRegisters (unsigned char *regs)
{
	memcpy(regs,APU.Regs,0x16);
}

void	APU_SetSoundRegisters (unsigned char *regs)
{
	int i;
	memcpy(APU.Regs,regs,0x16);
	for (i = 0; i <= 0x13; i++)
		APU_WriteReg(i,APU.Regs[i]);
	APU_WriteReg(0x15,APU.Regs[0x15]);
	APU_WriteReg(0x17,APU.Regs[0x14]);
}
#else
short	sample_pos = 0;
BOOL	sample_ok = FALSE;
#endif

void	APU_Run (void)
{
	static int sampcycles = 0, samppos = 0;
#ifndef	NSFPLAYER
	int NewBufPos, buflen = APU.LockSize / (BITS / 8);
	NewBufPos = FREQ * ++APU.Cycles / APU.MHz;
	if (NewBufPos >= buflen)
	{
		if (APU.isEnabled)
		{
			LPVOID bufPtr;
			DWORD bufBytes;
			unsigned long play_pos, write_pos;
			do
			{
				Sleep(1);
				APU_Try(IDirectSoundBuffer_GetCurrentPosition(APU.Buffer,&play_pos,NULL),"Error getting audio position")
				write_pos = ((play_pos / APU.LockSize + FRAMEBUF - 1) % FRAMEBUF) * APU.LockSize;
			} while (write_pos != ((APU.last_pos + APU.LockSize) % (APU.LockSize * FRAMEBUF)));
			APU_Try(IDirectSoundBuffer_Lock(APU.Buffer,write_pos,APU.LockSize,&bufPtr,&bufBytes,NULL,0,0),"Error locking sound buffer")
			memcpy(bufPtr,APU.buffer,bufBytes);
			APU_Try(IDirectSoundBuffer_Unlock(APU.Buffer,bufPtr,bufBytes,NULL,0),"Error unlocking sound buffer")
			APU.last_pos = write_pos;
		}
		APU.Cycles = NewBufPos = 0;
	}
#else
	int NewBufPos = SAMPLERATE * ++APU.Cycles / APU.MHz;
	if (NewBufPos == SAMPLERATE)	// we've generated 1 second, so we can reset our counters now
		APU.Cycles = NewBufPos = 0;
#endif
	Frame_Run();
	Square0_Run();
	Square1_Run();
	Triangle_Run();
	Noise_Run();
	DPCM_Run();

#ifdef	SOUND_FILTERING
	samppos += Square0.Pos + Square1.Pos + Triangle.Pos + Noise.Pos + DPCM.Pos;
#endif
	sampcycles++;
	
	if (NewBufPos != APU.BufPos)
	{
		APU.BufPos = NewBufPos;
#ifdef	SOUND_FILTERING
		samppos = (samppos / sampcycles) << 5;
#else
		samppos = (Square0.Pos + Square1.Pos + Triangle.Pos + Noise.Pos + DPCM.Pos) << 5;
#endif
		if ((MI) && (MI->GenSound))
			samppos += MI->GenSound(sampcycles);
#ifndef	NSFPLAYER
		APU.buffer[APU.BufPos] = samppos;
#else
		sample_pos = samppos;
		sample_ok = TRUE;
#endif
		samppos = sampcycles = 0;
#ifdef	SOUND_LOGGING
		if (soundlog)
			fwrite(&APU.buffer[APU.BufPos],2,1,soundlog);
#endif
	}
}
