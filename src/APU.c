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
# include "AVI.h"
#endif

#define	SOUND_FILTERING
/* #define	SOUND_LOGGING /* */

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

const	unsigned char	LengthCounts[32] = {
	0x0A,0xFE,
	0x14,0x02,
	0x28,0x04,
	0x50,0x06,
	0xA0,0x08,
	0x3C,0x0A,
	0x0E,0x0C,
	0x1A,0x0E,

	0x0C,0x10,
	0x18,0x12,
	0x30,0x14,
	0x60,0x16,
	0xC0,0x18,
	0x48,0x1A,
	0x10,0x1C,
	0x20,0x1E
};

static	struct
{
	unsigned char volume, envelope, wavehold, duty, swpspeed, swpdir, swpstep, swpenab;
	unsigned long freq;	// short
	unsigned char Vol;
	unsigned char CurD;
	unsigned char Timer;
	unsigned char EnvCtr, Envelope, BendCtr;
	BOOL Enabled, ValidFreq, Active;
	BOOL EnvClk, SwpClk;
	unsigned long Cycles;	// short
	signed long Pos;
}	Square0, Square1;
const	signed char	Duties[4][8] = {
	{-4,+4,-4,-4,-4,-4,-4,-4},
	{-4,+4,+4,-4,-4,-4,-4,-4},
	{-4,+4,+4,+4,+4,-4,-4,-4},
	{+4,-4,-4,+4,+4,+4,+4,+4}
};
__inline void	Square0_CheckActive (void)
{
	if ((Square0.ValidFreq = ((Square0.freq >= 0x8) && ((Square0.swpdir) || !((Square0.freq + (Square0.freq >> Square0.swpstep)) & 0x800)))) && (Square0.Timer))
	{
		Square0.Active = TRUE;
		Square0.Pos = Duties[Square0.duty][Square0.CurD] * Square0.Vol;
	}
	else
	{
		Square0.Active = FALSE;
		Square0.Pos = 0;
	}
}
__inline void	Square0_Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	Square0.volume = Val & 0xF;
		Square0.envelope = Val & 0x10;
		Square0.wavehold = Val & 0x20;
		Square0.duty = (Val >> 6) & 0x3;
		Square0.Vol = Square0.envelope ? Square0.volume : Square0.Envelope;
		break;
	case 1:	Square0.swpstep = Val & 0x07;
		Square0.swpdir = Val & 0x08;
		Square0.swpspeed = (Val >> 4) & 0x7;
		Square0.swpenab = Val & 0x80;
		Square0.SwpClk = TRUE;
		break;
	case 2:	Square0.freq &= 0x700;
		Square0.freq |= Val;
		break;
	case 3:	Square0.freq &= 0xFF;
		Square0.freq |= (Val & 0x7) << 8;
		if (Square0.Enabled)
			Square0.Timer = LengthCounts[(Val >> 3) & 0x1F];
		Square0.CurD = 0;
		Square0.EnvClk = TRUE;
		break;
	case 4:	if (!(Square0.Enabled = Val ? TRUE : FALSE))
			Square0.Timer = 0;
		break;
	}
	Square0_CheckActive();
}
__inline void	Square0_Run (void)
{
	if (!--Square0.Cycles)
	{
		Square0.Cycles = (Square0.freq + 1) << 1;
		Square0.CurD = (Square0.CurD + 1) & 0x7;
		if (Square0.Active)
			Square0.Pos = Duties[Square0.duty][Square0.CurD] * Square0.Vol;
	}
}
__inline void	Square0_QuarterFrame (void)
{
	if (Square0.EnvClk)
	{
		Square0.EnvClk = FALSE;
		Square0.Envelope = 0xF;
		Square0.EnvCtr = Square0.volume + 1;
	}
	else if (!--Square0.EnvCtr)
	{
		Square0.EnvCtr = Square0.volume + 1;
		if (Square0.Envelope)
			Square0.Envelope--;
		else	Square0.Envelope = Square0.wavehold ? 0xF : 0x0;
	}
	Square0.Vol = Square0.envelope ? Square0.volume : Square0.Envelope;
	Square0_CheckActive();
}
__inline void	Square0_HalfFrame (void)
{
	if (!--Square0.BendCtr)
	{
		Square0.BendCtr = Square0.swpspeed + 1;
		if (Square0.swpenab && Square0.swpstep && Square0.ValidFreq)
		{
			int sweep = Square0.freq >> Square0.swpstep;
			Square0.freq += Square0.swpdir ? ~sweep : sweep;
		}
	}
	if (Square0.SwpClk)
	{
		Square0.SwpClk = FALSE;
		Square0.BendCtr = Square0.swpspeed + 1;
	}
	if (Square0.Timer && !Square0.wavehold)
		Square0.Timer--;
	Square0_CheckActive();
}
__inline void	Square1_CheckActive (void)
{
	if ((Square1.ValidFreq = ((Square1.freq >= 0x8) && ((Square1.swpdir) || !((Square1.freq + (Square1.freq >> Square1.swpstep)) & 0x800)))) && (Square1.Timer))
	{
		Square1.Active = TRUE;
		Square1.Pos = Duties[Square1.duty][Square1.CurD] * Square1.Vol;
	}
	else
	{
		Square1.Active = FALSE;
		Square1.Pos = 0;
	}
}
__inline void	Square1_Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	Square1.volume = Val & 0xF;
		Square1.envelope = Val & 0x10;
		Square1.wavehold = Val & 0x20;
		Square1.duty = (Val >> 6) & 0x3;
		Square1.Vol = Square1.envelope ? Square1.volume : Square1.Envelope;
		break;
	case 1:	Square1.swpstep = Val & 0x07;
		Square1.swpdir = Val & 0x08;
		Square1.swpspeed = (Val >> 4) & 0x7;
		Square1.swpenab = Val & 0x80;
		Square1.SwpClk = TRUE;
		break;
	case 2:	Square1.freq &= 0x700;
		Square1.freq |= Val;
		break;
	case 3:	Square1.freq &= 0xFF;
		Square1.freq |= (Val & 0x7) << 8;
		if (Square1.Enabled)
			Square1.Timer = LengthCounts[(Val >> 3) & 0x1F];
		Square1.CurD = 0;
		Square1.EnvClk = TRUE;
		break;
	case 4:	if (!(Square1.Enabled = Val ? TRUE : FALSE))
			Square1.Timer = 0;
		break;
	}
	Square1_CheckActive();
}
__inline void	Square1_Run (void)
{
	if (!--Square1.Cycles)
	{
		Square1.Cycles = (Square1.freq + 1) << 1;
		Square1.CurD = (Square1.CurD + 1) & 0x7;
		if (Square1.Active)
			Square1.Pos = Duties[Square1.duty][Square1.CurD] * Square1.Vol;
	}
}
__inline void	Square1_QuarterFrame (void)
{
	if (Square1.EnvClk)
	{
		Square1.EnvClk = FALSE;
		Square1.Envelope = 0xF;
		Square1.EnvCtr = Square1.volume + 1;
	}
	else if (!--Square1.EnvCtr)
	{
		Square1.EnvCtr = Square1.volume + 1;
		if (Square1.Envelope)
			Square1.Envelope--;
		else	Square1.Envelope = Square1.wavehold ? 0xF : 0x0;
	}
	Square1.Vol = Square1.envelope ? Square1.volume : Square1.Envelope;
	Square1_CheckActive();
}
__inline void	Square1_HalfFrame (void)
{
	if (!--Square1.BendCtr)
	{
		Square1.BendCtr = Square1.swpspeed + 1;
		if (Square1.swpenab && Square1.swpstep && Square1.ValidFreq)
		{
			int sweep = Square1.freq >> Square1.swpstep;
			Square1.freq += Square1.swpdir ? -sweep : sweep;
		}
	}
	if (Square1.SwpClk)
	{
		Square1.SwpClk = FALSE;
		Square1.BendCtr = Square1.swpspeed + 1;
	}
	if (Square1.Timer && !Square1.wavehold)
		Square1.Timer--;
	Square1_CheckActive();
}

static	struct
{
	unsigned char linear, wavehold;
	unsigned long freq;	// short
	unsigned char CurD;
	unsigned char Timer, LinCtr;
	BOOL Enabled, Active;
	BOOL LinClk;
	unsigned long Cycles;	// short
	signed long Pos;
}	Triangle;
const	signed char	TriDuty[32] = {
	-8,-7,-6,-5,-4,-3,-2,-1,
	+0,+1,+2,+3,+4,+5,+6,+7,
	+7,+6,+5,+4,+3,+2,+1,+0,
	-1,-2,-3,-4,-5,-6,-7,-8
};
__inline void	Triangle_CheckActive (void)
{
	Triangle.Active = Triangle.Timer && Triangle.LinCtr;
	if (Triangle.freq < 4)
		Triangle.Pos = 0;	// beyond hearing range
	else	Triangle.Pos = TriDuty[Triangle.CurD] * 8;
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
		Triangle.LinClk = TRUE;
		break;
	case 4:	if (!(Triangle.Enabled = Val ? TRUE : FALSE))
			Triangle.Timer = 0;
		break;
	}
	Triangle_CheckActive();
}
__inline void	Triangle_Run (void)
{
	if (!--Triangle.Cycles)
	{
		Triangle.Cycles = Triangle.freq + 1;
		if (Triangle.Active)
		{
			Triangle.CurD++;
			Triangle.CurD &= 0x1F;
			if (Triangle.freq < 4)
				Triangle.Pos = 0;	// beyond hearing range
			else	Triangle.Pos = TriDuty[Triangle.CurD] * 8;
		}
	}
}
__inline void	Triangle_QuarterFrame (void)
{
	if (Triangle.LinClk)
		Triangle.LinCtr = Triangle.linear;
	else	if (Triangle.LinCtr)
		Triangle.LinCtr--;
	if (!Triangle.wavehold)
		Triangle.LinClk = FALSE;
	Triangle_CheckActive();
}
__inline void	Triangle_HalfFrame (void)
{
	if (Triangle.Timer && !Triangle.wavehold)
		Triangle.Timer--;
	Triangle_CheckActive();
}

static	struct
{
	unsigned char volume, envelope, wavehold, datatype;
	unsigned long freq;	// short
	unsigned long CurD;	// short
	unsigned char Vol;
	unsigned char Timer;
	unsigned char EnvCtr, Envelope;
	BOOL Enabled;
	BOOL EnvClk;
	unsigned long Cycles;	// short
	signed long Pos;
}	Noise;
const	unsigned long	NoiseFreq[16] = {
	0x004,0x008,0x010,0x020,0x040,0x060,0x080,0x0A0,
	0x0CA,0x0FE,0x17C,0x1FC,0x2FA,0x3F8,0x7F2,0xFE4
};
__inline void	Noise_Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	Noise.volume = Val & 0x0F;
		Noise.envelope = Val & 0x10;
		Noise.wavehold = Val & 0x20;
		Noise.Vol = Noise.envelope ? Noise.volume : Noise.Envelope;
		if (Noise.Timer)
			Noise.Pos = ((Noise.CurD & 0x4000) ? -2 : 2) * Noise.Vol;
		break;
	case 2:	Noise.freq = Val & 0xF;
		Noise.datatype = Val & 0x80;
		break;
	case 3:	if (Noise.Enabled)
			Noise.Timer = LengthCounts[(Val >> 3) & 0x1F];
		Noise.EnvClk = TRUE;
		break;
	case 4:	if (!(Noise.Enabled = Val ? TRUE : FALSE))
			Noise.Timer = 0;
		break;
	}
}
__inline void	Noise_Run (void)
{
	if (!--Noise.Cycles)
	{
		Noise.Cycles = NoiseFreq[Noise.freq];	/* no + 1 here */
		if (Noise.datatype)
			Noise.CurD = (Noise.CurD << 1) | (((Noise.CurD >> 14) ^ (Noise.CurD >> 8)) & 0x1);
		else	Noise.CurD = (Noise.CurD << 1) | (((Noise.CurD >> 14) ^ (Noise.CurD >> 13)) & 0x1);
		if (Noise.Timer)
			Noise.Pos = ((Noise.CurD & 0x4000) ? -2 : 2) * Noise.Vol;
	}
}
__inline void	Noise_QuarterFrame (void)
{
	if (Noise.EnvClk)
	{
		Noise.EnvClk = FALSE;
		Noise.Envelope = 0xF;
		Noise.EnvCtr = Noise.volume + 1;
	}
	else if (!--Noise.EnvCtr)
	{
		Noise.EnvCtr = Noise.volume + 1;
		if (Noise.Envelope)
			Noise.Envelope--;
		else	Noise.Envelope = Noise.wavehold ? 0xF : 0x0;
	}
	Noise.Vol = Noise.envelope ? Noise.volume : Noise.Envelope;
	if (Noise.Timer)
		Noise.Pos = ((Noise.CurD & 0x4000) ? -2 : 2) * Noise.Vol;
}
__inline void	Noise_HalfFrame (void)
{
	if (Noise.Timer && !Noise.wavehold)
		Noise.Timer--;
}

static	struct
{
	unsigned char freq, wavehold, doirq, pcmdata, addr, len;
	unsigned long CurAddr, SampleLen;	// short
	BOOL outmode, buffull;
	unsigned char shiftreg, outbits, buffer;
	unsigned long LengthCtr;	// short
	unsigned long Cycles;	// short
	signed long Pos;
}	DPCM;
const	unsigned long	DPCMFreqNTSC[16] = {
	0x1AC,0x17C,0x154,0x140,0x11E,0x0FE,0x0E2,0x0D6,
	0x0BE,0x0A0,0x08E,0x080,0x06A,0x054,0x048,0x036
};
const	unsigned long	DPCMFreqPAL[16] = {
	0x18E,0x161,0x13C,0x129,0x10A,0x0EC,0x0D2,0x0C7,
	0x0B1,0x095,0x084,0x076,0x062,0x04E,0x042,0x033
};
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
		if (DPCM.outmode)
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
				DPCM.outmode = TRUE;
				DPCM.shiftreg = DPCM.buffer;
				DPCM.buffull = FALSE;
				CPU.PCMCycles = 4;
			}
			else	DPCM.outmode = FALSE;
		}
	}
}

void	DPCM_Fetch (void)
{
	if (DPCM.buffull || !DPCM.LengthCtr)
		return;
//	if (CPU.PCMCycles > 4)		// handle first-cycle-is-read
//		CPU.PCMCycles = 4;
	while (CPU.PCMCycles--)
		DPCM.buffer = CPU_MemGet(DPCM.CurAddr);
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
			if (!(Frame.Bits & 0xC0))
				CPU.WantIRQ |= IRQ_FRAME;
			Frame.Num = 0;
			if (Frame.Bits & 0x80)
				Frame.Cycles = 2 * APU.QuarterFrameLen;
			else	Frame.Cycles = APU.QuarterFrameLen;
								break;
		}
	}
}

void	APU_WriteReg (int Addr, unsigned char Val)
{
#ifndef	NSFPLAYER
	if (Addr < 0x018)
		APU.Regs[Addr] = Val;
#endif
	switch (Addr)
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
	case 0x017:	Frame_Write(Val);	break;
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
		((            Square0.Timer) ? 0x01 : 0) |
		((            Square1.Timer) ? 0x02 : 0) |
		((           Triangle.Timer) ? 0x04 : 0) |
		((              Noise.Timer) ? 0x08 : 0) |
		((           DPCM.LengthCtr) ? 0x10 : 0) |
		(((CPU.WantIRQ & IRQ_FRAME)) ? 0x40 : 0) |
		(((CPU.WantIRQ &  IRQ_DPCM)) ? 0x80 : 0);
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
	APU.buflen = APU.LockSize / (BITS / 8);
	if (APU.buffer)
		free(APU.buffer);
	APU.buffer = (short *)malloc(APU.LockSize);
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
	APU.buflen		= 0;
#endif
	APU.QuarterFrameLen	= 0;
	APU.Cycles		= 0;
	APU.BufPos		= 0;
#ifndef	NSFPLAYER
	APU.next_pos		= 0;
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
		APU_Release();
		MessageBox(mWnd,"Failed to create DirectSound interface!","Nintendulator",MB_OK);
		return;
	}

	if (FAILED(IDirectSound_SetCooperativeLevel(APU.DirectSound,mWnd,DSSCL_PRIORITY)))
	{
		APU_Release();
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
		APU_Release();
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
		APU_Release();
		MessageBox(mWnd,"Failed to set output format!","Nintendulator",MB_OK);
		return;
	}
	if (FAILED(IDirectSoundBuffer_Play(APU.PrimaryBuffer,0,0,DSBPLAY_LOOPING)))
	{
		APU_Release();
		MessageBox(mWnd,"Failed to start playing primary buffer!","Nintendulator",MB_OK);
		return;
	}

	DSBD.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCSOFTWARE | DSBCAPS_GETCURRENTPOSITION2;
	DSBD.dwBufferBytes = APU.LockSize * FRAMEBUF;
	DSBD.lpwfxFormat = &WFX;

	if (FAILED(IDirectSound_CreateSoundBuffer(APU.DirectSound,&DSBD,&APU.Buffer,NULL)))
	{
		APU_Release();
		MessageBox(mWnd,"Failed to create secondary buffer!","Nintendulator",MB_OK);
		return;
	}
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
	Frame.Bits = 0x40;	// some games will behave BADLY if we don't do this
	CPU.WantIRQ &= ~(IRQ_FRAME | IRQ_DPCM);
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
	if (!APU.Buffer)
	{
		APU.isEnabled = FALSE;
		APU_Create();
		if (!APU.Buffer)
			return;
	}
	APU_Try(IDirectSoundBuffer_Lock(APU.Buffer,0,0,&bufPtr,&bufBytes,NULL,0,DSBLOCK_ENTIREBUFFER),"Error locking sound buffer (Clear)")
	ZeroMemory(bufPtr,bufBytes);
	APU_Try(IDirectSoundBuffer_Unlock(APU.Buffer,bufPtr,bufBytes,NULL,0),"Error unlocking sound buffer (Clear)")
	APU.isEnabled = TRUE;
	APU_Try(IDirectSoundBuffer_Play(APU.Buffer,0,0,DSBPLAY_LOOPING),"Unable to start playing buffer");
	APU.next_pos = 0;
#ifdef	SOUND_LOGGING
	if (!soundlog)
		soundlog = fopen("c:\\nes.pcm","wb");
#endif
}

void	APU_Config (HWND hWnd)
{
}

int	APU_Save (FILE *out)
{
	int clen = 0;
	unsigned char tpc;
	tpc = APU.Regs[0x15] & 0xF;
	fwrite(&tpc,1,1,out);			clen++;		//	uint8		Last value written to $4015, lower 4 bits

	fwrite(&APU.Regs[0x01],1,1,out);	clen++;		//	uint8		Last value written to $4001
	fwrite(&Square0.freq,2,1,out);		clen += 2;	//	uint16		Square0 frequency
	fwrite(&Square0.Timer,1,1,out);		clen++;		//	uint8		Square0 timer
	fwrite(&Square0.CurD,1,1,out);		clen++;		//	uint8		Square0 duty cycle pointer
	tpc = (Square0.EnvClk ? 0x2 : 0x0) | (Square0.SwpClk ? 0x1 : 0x0);
	fwrite(&tpc,1,1,out);			clen++;		//	uint8		Boolean flags for whether Square0 envelope(2)/sweep(1) needs a reload
	fwrite(&Square0.EnvCtr,1,1,out);	clen++;		//	uint8		Square0 envelope counter
	fwrite(&Square0.Envelope,1,1,out);	clen++;		//	uint8		Square0 envelope value
	fwrite(&Square0.BendCtr,1,1,out);	clen++;		//	uint8		Square0 bend counter
	fwrite(&Square0.Cycles,2,1,out);	clen += 2;	//	uint16		Square0 cycles
	fwrite(&APU.Regs[0x00],1,1,out);	clen++;		//	uint8		Last value written to $4000

	fwrite(&APU.Regs[0x05],1,1,out);	clen++;		//	uint8		Last value written to $4005
	fwrite(&Square1.freq,2,1,out);		clen += 2;	//	uint16		Square1 frequency
	fwrite(&Square1.Timer,1,1,out);		clen++;		//	uint8		Square1 timer
	fwrite(&Square1.CurD,1,1,out);		clen++;		//	uint8		Square1 duty cycle pointer
	tpc = (Square1.EnvClk ? 0x2 : 0x0) | (Square1.SwpClk ? 0x1 : 0x0);
	fwrite(&tpc,1,1,out);			clen++;		//	uint8		Boolean flags for whether Square1 envelope(2)/sweep(1) needs a reload
	fwrite(&Square1.EnvCtr,1,1,out);	clen++;		//	uint8		Square1 envelope counter
	fwrite(&Square1.Envelope,1,1,out);	clen++;		//	uint8		Square1 envelope value
	fwrite(&Square1.BendCtr,1,1,out);	clen++;		//	uint8		Square1 bend counter
	fwrite(&Square1.Cycles,2,1,out);	clen += 2;	//	uint16		Square1 cycles
	fwrite(&APU.Regs[0x04],1,1,out);	clen++;		//	uint8		Last value written to $4004

	fwrite(&Triangle.freq,2,1,out);		clen += 2;	//	uint16		Triangle frequency
	fwrite(&Triangle.Timer,1,1,out);	clen++;		//	uint8		Triangle timer
	fwrite(&Triangle.CurD,1,1,out);		clen++;		//	uint8		Triangle duty cycle pointer
	fwrite(&Triangle.LinClk,1,1,out);	clen++;		//	uint8		Boolean flag for whether linear counter needs reload
	fwrite(&Triangle.LinCtr,1,1,out);	clen++;		//	uint8		Triangle linear counter
	fwrite(&Triangle.Cycles,1,1,out);	clen++;		//	uint16		Triangle cycles
	fwrite(&APU.Regs[0x08],1,1,out);	clen++;		//	uint8		Last value written to $4008

	fwrite(&APU.Regs[0x0E],1,1,out);	clen++;		//	uint8		Last value written to $400E
	fwrite(&Noise.Timer,1,1,out);		clen++;		//	uint8		Noise timer
	fwrite(&Noise.CurD,2,1,out);		clen += 2;	//	uint16		Noise duty cycle pointer
	fwrite(&Noise.EnvClk,1,1,out);		clen++;		//	uint8		Boolean flag for whether Noise envelope needs a reload
	fwrite(&Noise.EnvCtr,1,1,out);		clen++;		//	uint8		Noise envelope counter
	fwrite(&Noise.Envelope,1,1,out);	clen++;		//	uint8		Noise  envelope value
	fwrite(&Noise.Cycles,2,1,out);		clen += 2;	//	uint16		Noise cycles
	fwrite(&APU.Regs[0x0C],1,1,out);	clen++;		//	uint8		Last value written to $400C

	fwrite(&APU.Regs[0x10],1,1,out);	clen++;		//	uint8		Last value written to $4010
	fwrite(&APU.Regs[0x11],1,1,out);	clen++;		//	uint8		Last value written to $4011
	fwrite(&APU.Regs[0x12],1,1,out);	clen++;		//	uint8		Last value written to $4012
	fwrite(&APU.Regs[0x13],1,1,out);	clen++;		//	uint8		Last value written to $4013
	fwrite(&DPCM.CurAddr,2,1,out);		clen += 2;	//	uint16		DPCM current address
	fwrite(&DPCM.SampleLen,2,1,out);	clen += 2;	//	uint16		DPCM current length
	fwrite(&DPCM.shiftreg,1,1,out);		clen++;		//	uint8		DPCM shift register
	tpc = (DPCM.buffull ? 0x1 : 0x0) | (DPCM.outmode ? 0x2 : 0x0);
	fwrite(&tpc,1,1,out);			clen++;		//	uint8		DPCM output mode(2)/buffer full(1)
	fwrite(&DPCM.outbits,1,1,out);		clen++;		//	uint8		DPCM shift count
	fwrite(&DPCM.buffer,1,1,out);		clen++;		//	uint8		DPCM read buffer
	fwrite(&DPCM.Cycles,2,1,out);		clen += 2;	//	uint16		DPCM cycles
	fwrite(&DPCM.LengthCtr,2,1,out);	clen += 2;	//	uint16		DPCM length counter

	fwrite(&APU.Regs[0x17],1,1,out);	clen++;		//	uint8		Last value written to $4017
	fwrite(&Frame.Cycles,2,1,out);		clen += 2;	//	uint16		Frame counter cycles
	fwrite(&Frame.Num,1,1,out);		clen++;		//	uint8		Frame counter phase

	tpc = CPU.WantIRQ & (IRQ_DPCM | IRQ_FRAME);
	fwrite(&tpc,1,1,out);			clen++;		//	uint8		APU-related IRQs (PCM and FRAME, as-is)

	return clen;
}

int	APU_Load (FILE *in)
{
	int clen = 0;
	unsigned char tpc;

	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4015, lower 4 bits
	APU_WriteReg(0x015,tpc);	// this will ACK any DPCM IRQ

	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4001
	APU_WriteReg(0x001,tpc);
	fread(&Square0.freq,2,1,in);		clen += 2;	//	uint16		Square0 frequency
	fread(&Square0.Timer,1,1,in);		clen++;		//	uint8		Square0 timer
	fread(&Square0.CurD,1,1,in);		clen++;		//	uint8		Square0 duty cycle pointer
	fread(&tpc,1,1,in);			clen++;		//	uint8		Boolean flags for whether Square0 envelope(2)/sweep(1) needs a reload
	Square0.EnvClk = (tpc & 0x2);
	Square0.SwpClk = (tpc & 0x1);
	fread(&Square0.EnvCtr,1,1,in);		clen++;		//	uint8		Square0 envelope counter
	fread(&Square0.Envelope,1,1,in);	clen++;		//	uint8		Square0 envelope value
	fread(&Square0.BendCtr,1,1,in);		clen++;		//	uint8		Square0 bend counter
	fread(&Square0.Cycles,2,1,in);		clen += 2;	//	uint16		Square0 cycles
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4000
	APU_WriteReg(0x000,tpc);

	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4005
	APU_WriteReg(0x005,tpc);
	fread(&Square1.freq,2,1,in);		clen += 2;	//	uint16		Square1 frequency
	fread(&Square1.Timer,1,1,in);		clen++;		//	uint8		Square1 timer
	fread(&Square1.CurD,1,1,in);		clen++;		//	uint8		Square1 duty cycle pointer
	fread(&tpc,1,1,in);			clen++;		//	uint8		Boolean flags for whether Square1 envelope(2)/sweep(1) needs a reload
	Square1.EnvClk = (tpc & 0x2);
	Square1.SwpClk = (tpc & 0x1);
	fread(&Square1.EnvCtr,1,1,in);		clen++;		//	uint8		Square1 envelope counter
	fread(&Square1.Envelope,1,1,in);	clen++;		//	uint8		Square1 envelope value
	fread(&Square1.BendCtr,1,1,in);		clen++;		//	uint8		Square1 bend counter
	fread(&Square1.Cycles,2,1,in);		clen += 2;	//	uint16		Square1 cycles
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4004
	APU_WriteReg(0x004,tpc);

	fread(&Triangle.freq,2,1,in);		clen += 2;	//	uint16		Triangle frequency
	fread(&Triangle.Timer,1,1,in);		clen++;		//	uint8		Triangle timer
	fread(&Triangle.CurD,1,1,in);		clen++;		//	uint8		Triangle duty cycle pointer
	fread(&Triangle.LinClk,1,1,in);		clen++;		//	uint8		Boolean flag for whether linear counter needs reload
	fread(&Triangle.LinCtr,1,1,in);		clen++;		//	uint8		Triangle linear counter
	fread(&Triangle.Cycles,1,1,in);		clen++;		//	uint16		Triangle cycles
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4008
	APU_WriteReg(0x008,tpc);

	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $400E
	APU_WriteReg(0x00E,tpc);
	fread(&Noise.Timer,1,1,in);		clen++;		//	uint8		Noise timer
	fread(&Noise.CurD,2,1,in);		clen += 2;	//	uint16		Noise duty cycle pointer
	fread(&Noise.EnvClk,1,1,in);		clen++;		//	uint8		Boolean flag for whether Noise envelope needs a reload
	fread(&Noise.EnvCtr,1,1,in);		clen++;		//	uint8		Noise envelope counter
	fread(&Noise.Envelope,1,1,in);		clen++;		//	uint8		Noise  envelope value
	fread(&Noise.Cycles,2,1,in);		clen += 2;	//	uint16		Noise cycles
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $400C
	APU_WriteReg(0x00C,tpc);

	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4010
	APU_WriteReg(0x010,tpc);
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4011
	APU_WriteReg(0x011,tpc);
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4012
	APU_WriteReg(0x012,tpc);
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4013
	APU_WriteReg(0x013,tpc);
	fread(&DPCM.CurAddr,2,1,in);		clen += 2;	//	uint16		DPCM current address
	fread(&DPCM.SampleLen,2,1,in);		clen += 2;	//	uint16		DPCM current length
	fread(&DPCM.shiftreg,1,1,in);		clen++;		//	uint8		DPCM shift register
	fread(&tpc,1,1,in);			clen++;		//	uint8		DPCM output mode(2)/buffer full(1)
	DPCM.buffull = tpc & 0x1;
	DPCM.outmode = tpc & 0x2;
	fread(&DPCM.outbits,1,1,in);		clen++;		//	uint8		DPCM shift count
	fread(&DPCM.buffer,1,1,in);		clen++;		//	uint8		DPCM read buffer
	fread(&DPCM.Cycles,2,1,in);		clen += 2;	//	uint16		DPCM cycles
	fread(&DPCM.LengthCtr,2,1,in);		clen += 2;	//	uint16		DPCM length counter

	fread(&tpc,1,1,in);			clen++;		//	uint8		Frame counter bits (last write to $4017)
	APU_WriteReg(0x017,tpc);	// and this will ACK any frame IRQ
	fread(&Frame.Cycles,2,1,in);		clen += 2;	//	uint16		Frame counter cycles
	fread(&Frame.Num,1,1,in);		clen++;		//	uint8		Frame counter phase

	fread(&tpc,1,1,in);			clen++;		//	uint8		APU-related IRQs (PCM and FRAME, as-is)
	CPU.WantIRQ |= tpc;		// so we can reload them here
	
	return clen;
}
#else
short	sample_pos = 0;
BOOL	sample_ok = FALSE;
#endif

void	APU_Run (void)
{
	static int sampcycles = 0, samppos = 0;
#ifndef	NSFPLAYER
	int NewBufPos = FREQ * ++APU.Cycles / APU.MHz;
	if (NewBufPos >= APU.buflen)
	{
		LPVOID bufPtr;
		DWORD bufBytes;
		unsigned long rpos, wpos;

		APU.Cycles = NewBufPos = 0;
		if (aviout)
			AVI_AddAudio();
		do
		{
			Sleep(1);
			if (!APU.isEnabled)
				break;
			APU_Try(IDirectSoundBuffer_GetCurrentPosition(APU.Buffer,&rpos,&wpos),"Error getting audio position")
			rpos /= APU.LockSize;
			wpos /= APU.LockSize;
			if (wpos < rpos)
				wpos += FRAMEBUF;
		} while ((rpos <= APU.next_pos) && (APU.next_pos <= wpos));
		if (APU.isEnabled)
		{
			APU_Try(IDirectSoundBuffer_Lock(APU.Buffer,APU.next_pos * APU.LockSize,APU.LockSize,&bufPtr,&bufBytes,NULL,0,0),"Error locking sound buffer")
			memcpy(bufPtr,APU.buffer,bufBytes);
			APU_Try(IDirectSoundBuffer_Unlock(APU.Buffer,bufPtr,bufBytes,NULL,0),"Error unlocking sound buffer")
			APU.next_pos = (APU.next_pos + 1) % FRAMEBUF;
		}
	}
#else
	int NewBufPos = SAMPLERATE * ++APU.Cycles / APU.MHz;
	if (NewBufPos == SAMPLERATE)	/* we've generated 1 second, so we can reset our counters now */
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
		samppos = (samppos << 6) / sampcycles;
#else
		samppos = (Square0.Pos + Square1.Pos + Triangle.Pos + Noise.Pos + DPCM.Pos) << 6;
#endif
		if ((MI) && (MI->GenSound))
			samppos += MI->GenSound(sampcycles);
		if (samppos < -0x8000)
			samppos = -0x8000;
		if (samppos > 0x7FFF)
			samppos = 0x7FFF;
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
