/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2009 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifdef	NSFPLAYER
# include "in_nintendulator.h"
# include "MapperInterface.h"
# include "APU.h"
# include "CPU.h"
#else	/* !NSFPLAYER */
# include "stdafx.h"
# include "Nintendulator.h"
# include "MapperInterface.h"
# include "NES.h"
# include "APU.h"
# include "CPU.h"
# include "PPU.h"
# include "AVI.h"
#endif	/* NSFPLAYER */

#define	SOUND_FILTERING

namespace APU
{
int			Cycles;
int			BufPos;
#ifndef	NSFPLAYER
unsigned long		next_pos;
BOOL			isEnabled;

LPDIRECTSOUND		DirectSound;
LPDIRECTSOUNDBUFFER	PrimaryBuffer;
LPDIRECTSOUNDBUFFER	Buffer;

short			*buffer;
int			buflen;
BYTE			Regs[0x18];
#endif	/* !NSFPLAYER */

int			WantFPS;
unsigned long		MHz;
BOOL			PAL;
#ifndef	NSFPLAYER
unsigned long		LockSize;
#endif	/* !NSFPLAYER */
int			InternalClock;

#ifndef	NSFPLAYER
#define	FREQ		44100
#define	BITS		16
#define	FRAMEBUF	4
const	unsigned int	LOCK_SIZE = FREQ * (BITS / 8);
#endif	/* !NSFPLAYER */

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
const	signed char	SquareDuty[4][8] = {
	{-4,+4,-4,-4,-4,-4,-4,-4},
	{-4,+4,+4,-4,-4,-4,-4,-4},
	{-4,+4,+4,+4,+4,-4,-4,-4},
	{+4,-4,-4,+4,+4,+4,+4,+4}
};
const	signed char	TriangleDuty[32] = {
	-8,-7,-6,-5,-4,-3,-2,-1,
	+0,+1,+2,+3,+4,+5,+6,+7,
	+7,+6,+5,+4,+3,+2,+1,+0,
	-1,-2,-3,-4,-5,-6,-7,-8
};
const	unsigned long	NoiseFreqNTSC[16] = {
	0x004,0x008,0x010,0x020,0x040,0x060,0x080,0x0A0,
	0x0CA,0x0FE,0x17C,0x1FC,0x2FA,0x3F8,0x7F2,0xFE4
};
const	unsigned long	NoiseFreqPAL[16] = {
	0x004,0x007,0x00E,0x01E,0x03C,0x058,0x076,0x094,
	0x0BC,0x0EC,0x162,0x1D8,0x2C4,0x3B0,0x762,0xEC2
};
const	unsigned long	DPCMFreqNTSC[16] = {
	0x1AC,0x17C,0x154,0x140,0x11E,0x0FE,0x0E2,0x0D6,
	0x0BE,0x0A0,0x08E,0x080,0x06A,0x054,0x048,0x036
};
const	unsigned long	DPCMFreqPAL[16] = {
	0x18E,0x162,0x13C,0x12A,0x114,0x0EC,0x0D2,0x0C6,
	0x0B0,0x094,0x084,0x076,0x062,0x04E,0x042,0x032
};
const	int	FrameCyclesNTSC_0[7] = { 7459,7456,7458,7457,1,1,7457 };
const	int	FrameCyclesNTSC_1[6] = { 1,7458,7456,7458,7456,7454 };
const	int	FrameCyclesPAL_0[7] = { 8315,8314,8312,8313,1,1,8313 };
const	int	FrameCyclesPAL_1[6] = { 1,8314,8314,8312,8314,8312 };

namespace Race
{
	unsigned char Square0_wavehold, Square0_timer1, Square0_timer2;
	unsigned char Square1_wavehold, Square1_timer1, Square1_timer2;
	unsigned char Triangle_wavehold, Triangle_timer1, Triangle_timer2;
	unsigned char Noise_wavehold, Noise_timer1, Noise_timer2;
	void Run (void);
} // namespace race

namespace Square0
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

void	Reset (void)
{
	volume = envelope = wavehold = duty = swpspeed = swpdir = swpstep = swpenab = 0;
	freq = 0;
	Vol = 0;
	CurD = 0;
	Timer = 0;
	Envelope = 0;
	Enabled = ValidFreq = Active = FALSE;
	EnvClk = SwpClk = FALSE;
	Pos = 0;
	Cycles = 1;
	EnvCtr = 1;
	BendCtr = 1;
}
inline void	CheckActive (void)
{
	ValidFreq = (freq >= 0x8) && ((swpdir) || !((freq + (freq >> swpstep)) & 0x800));
	Active = Timer && ValidFreq;
	Pos = Active ? (SquareDuty[duty][CurD] * Vol) : 0;
}
inline void	Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	volume = Val & 0xF;
		envelope = Val & 0x10;
		Race::Square0_wavehold = Val & 0x20;
		duty = (Val >> 6) & 0x3;
		Vol = envelope ? volume : Envelope;
		break;
	case 1:	swpstep = Val & 0x07;
		swpdir = Val & 0x08;
		swpspeed = (Val >> 4) & 0x7;
		swpenab = Val & 0x80;
		SwpClk = TRUE;
		break;
	case 2:	freq &= 0x700;
		freq |= Val;
		break;
	case 3:	freq &= 0xFF;
		freq |= (Val & 0x7) << 8;
		if (Enabled)
		{
			Race::Square0_timer1 = LengthCounts[(Val >> 3) & 0x1F];
			Race::Square0_timer2 = Timer;
		}
		CurD = 0;
		EnvClk = TRUE;
		break;
	case 4:	Enabled = Val ? TRUE : FALSE;
		if (!Enabled)
			Timer = 0;
		break;
	}
	CheckActive();
}

inline void	Run (void)
{
	if (!--Cycles)
	{
		Cycles = (freq + 1) << 1;
		CurD = (CurD + 1) & 0x7;
		if (Active)
			Pos = SquareDuty[duty][CurD] * Vol;
	}
}
inline void	QuarterFrame (void)
{
	if (EnvClk)
	{
		EnvClk = FALSE;
		Envelope = 0xF;
		EnvCtr = volume + 1;
	}
	else if (!--EnvCtr)
	{
		EnvCtr = volume + 1;
		if (Envelope)
			Envelope--;
		else	Envelope = wavehold ? 0xF : 0x0;
	}
	Vol = envelope ? volume : Envelope;
	CheckActive();
}
inline void	HalfFrame (void)
{
	if (!--BendCtr)
	{
		BendCtr = swpspeed + 1;
		if (swpenab && swpstep && ValidFreq)
		{
			int sweep = freq >> swpstep;
			freq += swpdir ? ~sweep : sweep;
		}
	}
	if (SwpClk)
	{
		SwpClk = FALSE;
		BendCtr = swpspeed + 1;
	}
	if (Timer && !wavehold)
		Timer--;
	CheckActive();
}
} // namespace Square0

namespace Square1
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

void	Reset (void)
{
	volume = envelope = wavehold = duty = swpspeed = swpdir = swpstep = swpenab = 0;
	freq = 0;
	Vol = 0;
	CurD = 0;
	Timer = 0;
	Envelope = 0;
	Enabled = ValidFreq = Active = FALSE;
	EnvClk = SwpClk = FALSE;
	Pos = 0;
	Cycles = 1;
	EnvCtr = 1;
	BendCtr = 1;
}
inline void	CheckActive (void)
{
	ValidFreq = (freq >= 0x8) && ((swpdir) || !((freq + (freq >> swpstep)) & 0x800));
	Active = Timer && ValidFreq;
	Pos = Active ? (SquareDuty[duty][CurD] * Vol) : 0;
}
inline void	Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	volume = Val & 0xF;
		envelope = Val & 0x10;
		Race::Square1_wavehold = Val & 0x20;
		duty = (Val >> 6) & 0x3;
		Vol = envelope ? volume : Envelope;
		break;
	case 1:	swpstep = Val & 0x07;
		swpdir = Val & 0x08;
		swpspeed = (Val >> 4) & 0x7;
		swpenab = Val & 0x80;
		SwpClk = TRUE;
		break;
	case 2:	freq &= 0x700;
		freq |= Val;
		break;
	case 3:	freq &= 0xFF;
		freq |= (Val & 0x7) << 8;
		if (Enabled)
		{
			Race::Square1_timer1 = LengthCounts[(Val >> 3) & 0x1F];
			Race::Square1_timer2 = Timer;
		}
		CurD = 0;
		EnvClk = TRUE;
		break;
	case 4:	Enabled = Val ? TRUE : FALSE;
		if (!Enabled)
			Timer = 0;
		break;
	}
	CheckActive();
}
inline void	Run (void)
{
	if (!--Cycles)
	{
		Cycles = (freq + 1) << 1;
		CurD = (CurD + 1) & 0x7;
		if (Active)
			Pos = SquareDuty[duty][CurD] * Vol;
	}
}
inline void	QuarterFrame (void)
{
	if (EnvClk)
	{
		EnvClk = FALSE;
		Envelope = 0xF;
		EnvCtr = volume + 1;
	}
	else if (!--EnvCtr)
	{
		EnvCtr = volume + 1;
		if (Envelope)
			Envelope--;
		else	Envelope = wavehold ? 0xF : 0x0;
	}
	Vol = envelope ? volume : Envelope;
	CheckActive();
}
inline void	HalfFrame (void)
{
	if (!--BendCtr)
	{
		BendCtr = swpspeed + 1;
		if (swpenab && swpstep && ValidFreq)
		{
			int sweep = freq >> swpstep;
			freq += swpdir ? -sweep : sweep;
		}
	}
	if (SwpClk)
	{
		SwpClk = FALSE;
		BendCtr = swpspeed + 1;
	}
	if (Timer && !wavehold)
		Timer--;
	CheckActive();
}
} // namespace Square1

namespace Triangle
{
	unsigned char linear, wavehold;
	unsigned long freq;	// short
	unsigned char CurD;
	unsigned char Timer, LinCtr;
	BOOL Enabled, Active;
	BOOL LinClk;
	unsigned long Cycles;	// short
	signed long Pos;

void	Reset (void)
{
	linear = wavehold = 0;
	freq = 0;
	CurD = 0;
	Timer = LinCtr = 0;
	Enabled = Active = FALSE;
        LinClk = FALSE;
	Pos = 0;
	Cycles = 1;
}
inline void	CheckActive (void)
{
	Active = Timer && LinCtr;
	if (freq < 4)
		Pos = 0;	// beyond hearing range
	else	Pos = TriangleDuty[CurD] * 8;
}
inline void	Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	linear = Val & 0x7F;
		Race::Triangle_wavehold = (Val >> 7) & 0x1;
		break;
	case 2:	freq &= 0x700;
		freq |= Val;
		break;
	case 3:	freq &= 0xFF;
		freq |= (Val & 0x7) << 8;
		if (Enabled)
		{
			Race::Triangle_timer1 = LengthCounts[(Val >> 3) & 0x1F];
			Race::Triangle_timer2 = Timer;
		}
		LinClk = TRUE;
		break;
	case 4:	Enabled = Val ? TRUE : FALSE;
		if (!Enabled)
			Timer = 0;
		break;
	}
	CheckActive();
}
inline void	Run (void)
{
	if (!--Cycles)
	{
		Cycles = freq + 1;
		if (Active)
		{
			CurD++;
			CurD &= 0x1F;
			if (freq < 4)
				Pos = 0;	// beyond hearing range
			else	Pos = TriangleDuty[CurD] * 8;
		}
	}
}
inline void	QuarterFrame (void)
{
	if (LinClk)
		LinCtr = linear;
	else	if (LinCtr)
		LinCtr--;
	if (!wavehold)
		LinClk = FALSE;
	CheckActive();
}
inline void	HalfFrame (void)
{
	if (Timer && !wavehold)
		Timer--;
	CheckActive();
}
} // namespace Triangle

namespace Noise
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

const unsigned long	*FreqTable;
void	Reset (void)
{
	volume = envelope = wavehold = datatype = 0;
	freq = 0;
	Vol = 0;
	Timer = 0;
	Envelope = 0;
	Enabled = FALSE;
	EnvClk = FALSE;
	Pos = 0;
	CurD = 1;
	Cycles = 1;
	EnvCtr = 1;
}
inline void	Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	volume = Val & 0x0F;
		envelope = Val & 0x10;
		Race::Noise_wavehold = Val & 0x20;
		Vol = envelope ? volume : Envelope;
		if (Timer)
			Pos = ((CurD & 0x4000) ? -2 : 2) * Vol;
		break;
	case 2:	freq = Val & 0xF;
		datatype = Val & 0x80;
		break;
	case 3:	if (Enabled)
		{
			Race::Noise_timer1 = LengthCounts[(Val >> 3) & 0x1F];
			Race::Noise_timer2 = Timer;
		}
		EnvClk = TRUE;
		break;
	case 4:	Enabled = Val ? TRUE : FALSE;
		if (!Enabled)
			Timer = 0;
		break;
	}
}
inline void	Run (void)
{
	if (!--Cycles)
	{
		Cycles = FreqTable[freq];	/* no + 1 here */
		if (datatype)
			CurD = (CurD << 1) | (((CurD >> 14) ^ (CurD >> 8)) & 0x1);
		else	CurD = (CurD << 1) | (((CurD >> 14) ^ (CurD >> 13)) & 0x1);
		if (Timer)
			Pos = ((CurD & 0x4000) ? -2 : 2) * Vol;
	}
}
inline void	QuarterFrame (void)
{
	if (EnvClk)
	{
		EnvClk = FALSE;
		Envelope = 0xF;
		EnvCtr = volume + 1;
	}
	else if (!--EnvCtr)
	{
		EnvCtr = volume + 1;
		if (Envelope)
			Envelope--;
		else	Envelope = wavehold ? 0xF : 0x0;
	}
	Vol = envelope ? volume : Envelope;
	if (Timer)
		Pos = ((CurD & 0x4000) ? -2 : 2) * Vol;
}
inline void	HalfFrame (void)
{
	if (Timer && !wavehold)
		Timer--;
}
} // namespace Noise

namespace DPCM
{
	unsigned char freq, wavehold, doirq, pcmdata, addr, len;
	unsigned long CurAddr, SampleLen;	// short
	BOOL outmode, buffull;
	unsigned char shiftreg, outbits, buffer;
	unsigned long LengthCtr;	// short
	unsigned long Cycles;	// short
	signed long Pos;

const	unsigned long	*FreqTable;
void	Reset (void)
{
	freq = wavehold = doirq = pcmdata = addr = len = 0;
	CurAddr = SampleLen = 0;
	outmode = FALSE;
	shiftreg = buffer = 0;
	LengthCtr = 0;
	Pos = 0;

	Cycles = 1;
	buffull = TRUE;
	outbits = 8;
}
inline void	Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	freq = Val & 0xF;
		wavehold = (Val >> 6) & 0x1;
		doirq = Val >> 7;
		if (!doirq)
			CPU::WantIRQ &= ~IRQ_DPCM;
		break;
	case 1:	pcmdata = Val & 0x7F;
		Pos = (pcmdata - 0x40) * 3;
		break;
	case 2:	addr = Val;
		break;
	case 3:	len = Val;
		break;
	case 4:	if (Val)
		{
			if (!LengthCtr)
			{
				CurAddr = 0xC000 | (addr << 6);
				LengthCtr = (len << 4) + 1;
			}
		}
		else	LengthCtr = 0;
		CPU::WantIRQ &= ~IRQ_DPCM;
		break;
	}
}
inline void	Run (void)
{
	if (!--Cycles)
	{
		Cycles = FreqTable[freq];
		if (outmode)
		{
			if (shiftreg & 1)
			{
				if (pcmdata <= 0x7D)
					pcmdata += 2;
			}
			else
			{
				if (pcmdata >= 0x02)
					pcmdata -= 2;
			}
			shiftreg >>= 1;
			Pos = (pcmdata - 0x40) * 3;
		}
		if (!--outbits)
		{
			outbits = 8;
			if (buffull && LengthCtr)
			{
				outmode = TRUE;
				shiftreg = buffer;
				buffull = FALSE;
				CPU::PCMCycles = 4;
			}
			else	outmode = FALSE;
		}
	}
}

void	Fetch (void)
{
	buffer = CPU::MemGet(CurAddr);
	buffull = TRUE;
	if (++CurAddr == 0x10000)
		CurAddr = 0x8000;
	if (!--LengthCtr)
	{
		if (wavehold)
		{
			CurAddr = 0xC000 | (addr << 6);
			LengthCtr = (len << 4) + 1;
		}
		else if (doirq)
			CPU::WantIRQ |= IRQ_DPCM;
	}
}
} // namespace DPCM

namespace Frame
{
	unsigned char Bits;
	unsigned long Cycles;
	int Num;

const	int	*CycleTable_0;
const	int	*CycleTable_1;
void	Reset (void)
{
	Bits = 0;
	Num = 0;
	Cycles = CycleTable_0[0];
}
inline void	Write (unsigned char Val)
{
	Bits = Val & 0xC0;
	Num = 0;
	if (Bits & 0x80)
		Cycles = CycleTable_1[0];
	else	Cycles = CycleTable_0[0];
	if (InternalClock & 1)
		Cycles += 2;
	else	Cycles++;
	if (Bits & 0x40)
		CPU::WantIRQ &= ~IRQ_FRAME;
}
inline void	Run (void)
{
	if (!--Cycles)
	{
		if (Bits & 0x80)
		{
			if (Num < 4)
			{
				Square0::QuarterFrame();
				Square1::QuarterFrame();
				Triangle::QuarterFrame();
				Noise::QuarterFrame();
				if ((Num == 0) || (Num == 2))
				{
					Square0::HalfFrame();
					Square1::HalfFrame();
					Triangle::HalfFrame();
					Noise::HalfFrame();
				}
			}
			Cycles = CycleTable_1[Num + 1];
			Num++;
			if (Num == 5)
				Num = 0;
		}
		else
		{
			if ((Num == 0) || (Num == 1) || (Num == 2) ||(Num == 4))
			{
				Square0::QuarterFrame();
				Square1::QuarterFrame();
				Triangle::QuarterFrame();
				Noise::QuarterFrame();
			}
			if ((Num == 1) || (Num == 4))
			{
				Square0::HalfFrame();
				Square1::HalfFrame();
				Triangle::HalfFrame();
				Noise::HalfFrame();
			}
			if (((Num == 3) || (Num == 4) || (Num == 5)) && !(Bits & 0x40))
				CPU::WantIRQ |= IRQ_FRAME;
			Cycles = CycleTable_0[Num + 1];
			Num++;
			if (Num == 6)
				Num = 0;
		}
	}
}
} // namespace Frame

void	Race::Run (void)
{
	Square0::wavehold = Square0_wavehold;
	if (Square0_timer1)
	{
		if (Square0::Timer == Square0_timer2)
			Square0::Timer = Square0_timer1;
		Square0_timer1 = 0;
	}

	Square1::wavehold = Square1_wavehold;
	if (Square1_timer1)
	{
		if (Square1::Timer == Square1_timer2)
			Square1::Timer = Square1_timer1;
		Square1_timer1 = 0;
	}

	Triangle::wavehold = Triangle_wavehold;
	if (Triangle_timer1)
	{
		if (Triangle::Timer == Triangle_timer2)
			Triangle::Timer = Triangle_timer1;
		Triangle_timer1 = 0;
	}

	Noise::wavehold = Noise_wavehold;
	if (Noise_timer1)
	{
		if (Noise::Timer == Noise_timer2)
			Noise::Timer = Noise_timer1;
		Noise_timer1 = 0;
	}
}

void	WriteReg (int Addr, unsigned char Val)
{
#ifndef	NSFPLAYER
	if (Addr < 0x018)
		Regs[Addr] = Val;
#endif	/* !NSFPLAYER */
	switch (Addr)
	{
	case 0x000:	Square0::Write(0,Val);	break;
	case 0x001:	Square0::Write(1,Val);	break;
	case 0x002:	Square0::Write(2,Val);	break;
	case 0x003:	Square0::Write(3,Val);	break;
	case 0x004:	Square1::Write(0,Val);	break;
	case 0x005:	Square1::Write(1,Val);	break;
	case 0x006:	Square1::Write(2,Val);	break;
	case 0x007:	Square1::Write(3,Val);	break;
	case 0x008:	Triangle::Write(0,Val);	break;
	case 0x009:	Triangle::Write(1,Val);	break;
	case 0x00A:	Triangle::Write(2,Val);	break;
	case 0x00B:	Triangle::Write(3,Val);	break;
	case 0x00C:	Noise::Write(0,Val);	break;
	case 0x00D:	Noise::Write(1,Val);	break;
	case 0x00E:	Noise::Write(2,Val);	break;
	case 0x00F:	Noise::Write(3,Val);	break;
	case 0x010:	DPCM::Write(0,Val);	break;
	case 0x011:	DPCM::Write(1,Val);	break;
	case 0x012:	DPCM::Write(2,Val);	break;
	case 0x013:	DPCM::Write(3,Val);	break;
	case 0x015:	Square0::Write(4,Val & 0x1);
			Square1::Write(4,Val & 0x2);
			Triangle::Write(4,Val & 0x4);
			Noise::Write(4,Val & 0x8);
			DPCM::Write(4,Val & 0x10);
						break;
	case 0x017:	Frame::Write(Val);	break;
#ifndef	NSFPLAYER
	default:	MessageBox(hMainWnd,_T("ERROR: Invalid sound write!"),_T("Nintendulator"),MB_OK);
#else	/* NSFPLAYER */
	default:	MessageBox(mod.hMainWindow,"ERROR: Invalid sound write!","in_nintendulator",MB_OK);
#endif	/* !NSFPLAYER */
						break;
	}
}
unsigned char	Read4015 (void)
{
	unsigned char result =
		((            Square0::Timer) ? 0x01 : 0) |
		((            Square1::Timer) ? 0x02 : 0) |
		((           Triangle::Timer) ? 0x04 : 0) |
		((              Noise::Timer) ? 0x08 : 0) |
		((           DPCM::LengthCtr) ? 0x10 : 0) |
		(((CPU::WantIRQ & IRQ_FRAME)) ? 0x40 : 0) |
		(((CPU::WantIRQ &  IRQ_DPCM)) ? 0x80 : 0);
	CPU::WantIRQ &= ~(IRQ_FRAME | IRQ_DPCM);
	return result;
}

#ifndef	NSFPLAYER
#define	Try(action,errormsg)\
{\
	if (FAILED(action))\
	{\
		Release();\
		Create();\
		if (FAILED(action))\
		{\
			SoundOFF();\
			MessageBox(hMainWnd,errormsg,_T("Nintendulator"),MB_OK | MB_ICONERROR);\
			return;\
		}\
	}\
}
#endif	/* !NSFPLAYER */

void	SetFPSVars (int FPS)
{
	WantFPS = FPS;
	if (WantFPS == 60)
	{
		PAL = FALSE;
		MHz = 1789773;
		Noise::FreqTable = NoiseFreqNTSC;
		DPCM::FreqTable = DPCMFreqNTSC;
		Frame::CycleTable_0 = FrameCyclesNTSC_0;
		Frame::CycleTable_1 = FrameCyclesNTSC_1;
	}
	else if (WantFPS == 50)
	{
		PAL = TRUE;
		MHz = 1662607;
		Noise::FreqTable = NoiseFreqPAL;
		DPCM::FreqTable = DPCMFreqPAL;
		Frame::CycleTable_0 = FrameCyclesPAL_0;
		Frame::CycleTable_1 = FrameCyclesPAL_1;
	}
	else
	{
#ifndef	NSFPLAYER
		MessageBox(hMainWnd,_T("Attempted to set indeterminate sound framerate!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
#else	/* NSFPLAYER */
		MessageBox(mod.hMainWindow,"Attempted to set indeterminate sound framerate!","in_nintendulator",MB_OK | MB_ICONERROR);
#endif	/* !NSFPLAYER */
		return;
	}
#ifndef	NSFPLAYER
	LockSize = LOCK_SIZE / WantFPS;
	buflen = LockSize / (BITS / 8);
	if (buffer)
		free(buffer);
	buffer = (short *)malloc(LockSize);
#endif	/* !NSFPLAYER */
}

void	SetFPS (int FPS)
{
#ifndef	NSFPLAYER
	BOOL Enabled = isEnabled;
	Release();
#endif	/* !NSFPLAYER */
	SetFPSVars(FPS);
#ifndef	NSFPLAYER
	Create();
	if (Enabled)
		SoundON();
#endif	/* !NSFPLAYER */
}

void	Init (void)
{
#ifndef	NSFPLAYER
	ZeroMemory(Regs,sizeof(Regs));
	DirectSound		= NULL;
	PrimaryBuffer	= NULL;
	Buffer		= NULL;
	buffer		= NULL;
	isEnabled		= FALSE;
#endif	/* !NSFPLAYER */
	WantFPS		= 0;
	MHz			= 1;
	PAL			= FALSE;
#ifndef	NSFPLAYER
	LockSize		= 0;
	buflen		= 0;
#endif	/* !NSFPLAYER */
	Cycles		= 0;
	BufPos		= 0;
#ifndef	NSFPLAYER
	next_pos		= 0;
#endif	/* !NSFPLAYER */
}

void	Create (void)
{
#ifndef	NSFPLAYER
	DSBUFFERDESC DSBD;
	WAVEFORMATEX WFX;
#endif	/* !NSFPLAYER */

	if (!WantFPS)
		SetFPSVars(60);

#ifndef	NSFPLAYER
	if (FAILED(DirectSoundCreate(&DSDEVID_DefaultPlayback,&DirectSound,NULL)))
	{
		Release();
		MessageBox(hMainWnd,_T("Failed to create DirectSound interface!"),_T("Nintendulator"),MB_OK);
		return;
	}
	if (FAILED(DirectSound->SetCooperativeLevel(hMainWnd,DSSCL_PRIORITY)))
	{
		Release();
		MessageBox(hMainWnd,_T("Failed to set cooperative level!"),_T("Nintendulator"),MB_OK);
		return;
	}

	ZeroMemory(&DSBD,sizeof(DSBUFFERDESC));
	DSBD.dwSize = sizeof(DSBUFFERDESC);
	DSBD.dwFlags = DSBCAPS_PRIMARYBUFFER;
	DSBD.dwBufferBytes = 0;
	DSBD.lpwfxFormat = NULL;
	if (FAILED(DirectSound->CreateSoundBuffer(&DSBD,&PrimaryBuffer,NULL)))
	{
		Release();
		MessageBox(hMainWnd,_T("Failed to create primary buffer!"),_T("Nintendulator"),MB_OK);
		return;
	}

	ZeroMemory(&WFX,sizeof(WAVEFORMATEX));
	WFX.wFormatTag = WAVE_FORMAT_PCM;
	WFX.nChannels = 1;
	WFX.nSamplesPerSec = FREQ;
	WFX.wBitsPerSample = BITS;
	WFX.nBlockAlign = WFX.wBitsPerSample / 8 * WFX.nChannels;
	WFX.nAvgBytesPerSec = WFX.nSamplesPerSec * WFX.nBlockAlign;
	if (FAILED(PrimaryBuffer->SetFormat(&WFX)))
	{
		Release();
		MessageBox(hMainWnd,_T("Failed to set output format!"),_T("Nintendulator"),MB_OK);
		return;
	}
	if (FAILED(PrimaryBuffer->Play(0,0,DSBPLAY_LOOPING)))
	{
		Release();
		MessageBox(hMainWnd,_T("Failed to start playing primary buffer!"),_T("Nintendulator"),MB_OK);
		return;
	}

	DSBD.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCSOFTWARE | DSBCAPS_GETCURRENTPOSITION2;
	DSBD.dwBufferBytes = LockSize * FRAMEBUF;
	DSBD.lpwfxFormat = &WFX;

	if (FAILED(DirectSound->CreateSoundBuffer(&DSBD,&Buffer,NULL)))
	{
		Release();
		MessageBox(hMainWnd,_T("Failed to create secondary buffer!"),_T("Nintendulator"),MB_OK);
		return;
	}
	EI.DbgOut(_T("Created %iHz %i bit audio buffer, %i frames (%i bytes per frame)"), WFX.nSamplesPerSec, WFX.wBitsPerSample, DSBD.dwBufferBytes / LockSize, LockSize);
#endif	/* !NSFPLAYER */
}

void	Release (void)
{
#ifndef	NSFPLAYER
	if (Buffer)
	{
		SoundOFF();
		Buffer->Release();
		Buffer = NULL;
	}
	if (PrimaryBuffer)
	{
		PrimaryBuffer->Stop();
		PrimaryBuffer->Release();
		PrimaryBuffer = NULL;
	}
	if (DirectSound)
	{
		DirectSound->Release();
		DirectSound = NULL;
	}
#endif	/* !NSFPLAYER */
}

void	Reset  (void)
{
#ifndef	NSFPLAYER
	ZeroMemory(Regs,0x18);
#endif	/* !NSFPLAYER */
	Frame::Reset();
	Square0::Reset();
	Square1::Reset();
	Triangle::Reset();
	Noise::Reset();
	DPCM::Reset();
	Cycles = 1;
	CPU::WantIRQ &= ~(IRQ_FRAME | IRQ_DPCM);
	InternalClock = 0;
}

#ifndef	NSFPLAYER
void	SoundOFF (void)
{
	isEnabled = FALSE;
	if (Buffer)
		Buffer->Stop();
}

void	SoundON (void)
{
	LPVOID bufPtr;
	DWORD bufBytes;
	if (!Buffer)
	{
		isEnabled = FALSE;
		Create();
		if (!Buffer)
			return;
	}
	Try(Buffer->Lock(0,0,&bufPtr,&bufBytes,NULL,0,DSBLOCK_ENTIREBUFFER),_T("Error locking sound buffer (Clear)"))
	ZeroMemory(bufPtr,bufBytes);
	Try(Buffer->Unlock(bufPtr,bufBytes,NULL,0),_T("Error unlocking sound buffer (Clear)"))
	isEnabled = TRUE;
	Try(Buffer->Play(0,0,DSBPLAY_LOOPING),_T("Unable to start playing buffer"))
	next_pos = 0;
}

void	Config (HWND hWnd)
{
}

int	Save (FILE *out)
{
	int clen = 0;
	unsigned char tpc;
	tpc = Regs[0x15] & 0xF;
	fwrite(&tpc,1,1,out);			clen++;		//	uint8		Last value written to $4015, lower 4 bits

	fwrite(&Regs[0x01],1,1,out);	clen++;		//	uint8		Last value written to $4001
	fwrite(&Square0::freq,2,1,out);		clen += 2;	//	uint16		Square0 frequency
	fwrite(&Square0::Timer,1,1,out);		clen++;		//	uint8		Square0 timer
	fwrite(&Square0::CurD,1,1,out);		clen++;		//	uint8		Square0 duty cycle pointer
	tpc = (Square0::EnvClk ? 0x2 : 0x0) | (Square0::SwpClk ? 0x1 : 0x0);
	fwrite(&tpc,1,1,out);			clen++;		//	uint8		Boolean flags for whether Square0 envelope(2)/sweep(1) needs a reload
	fwrite(&Square0::EnvCtr,1,1,out);	clen++;		//	uint8		Square0 envelope counter
	fwrite(&Square0::Envelope,1,1,out);	clen++;		//	uint8		Square0 envelope value
	fwrite(&Square0::BendCtr,1,1,out);	clen++;		//	uint8		Square0 bend counter
	fwrite(&Square0::Cycles,2,1,out);	clen += 2;	//	uint16		Square0 cycles
	fwrite(&Regs[0x00],1,1,out);	clen++;		//	uint8		Last value written to $4000

	fwrite(&Regs[0x05],1,1,out);	clen++;		//	uint8		Last value written to $4005
	fwrite(&Square1::freq,2,1,out);		clen += 2;	//	uint16		Square1 frequency
	fwrite(&Square1::Timer,1,1,out);		clen++;		//	uint8		Square1 timer
	fwrite(&Square1::CurD,1,1,out);		clen++;		//	uint8		Square1 duty cycle pointer
	tpc = (Square1::EnvClk ? 0x2 : 0x0) | (Square1::SwpClk ? 0x1 : 0x0);
	fwrite(&tpc,1,1,out);			clen++;		//	uint8		Boolean flags for whether Square1 envelope(2)/sweep(1) needs a reload
	fwrite(&Square1::EnvCtr,1,1,out);	clen++;		//	uint8		Square1 envelope counter
	fwrite(&Square1::Envelope,1,1,out);	clen++;		//	uint8		Square1 envelope value
	fwrite(&Square1::BendCtr,1,1,out);	clen++;		//	uint8		Square1 bend counter
	fwrite(&Square1::Cycles,2,1,out);	clen += 2;	//	uint16		Square1 cycles
	fwrite(&Regs[0x04],1,1,out);	clen++;		//	uint8		Last value written to $4004

	fwrite(&Triangle::freq,2,1,out);		clen += 2;	//	uint16		Triangle frequency
	fwrite(&Triangle::Timer,1,1,out);	clen++;		//	uint8		Triangle timer
	fwrite(&Triangle::CurD,1,1,out);		clen++;		//	uint8		Triangle duty cycle pointer
	fwrite(&Triangle::LinClk,1,1,out);	clen++;		//	uint8		Boolean flag for whether linear counter needs reload
	fwrite(&Triangle::LinCtr,1,1,out);	clen++;		//	uint8		Triangle linear counter
	fwrite(&Triangle::Cycles,1,1,out);	clen++;		//	uint16		Triangle cycles
	fwrite(&Regs[0x08],1,1,out);	clen++;		//	uint8		Last value written to $4008

	fwrite(&Regs[0x0E],1,1,out);	clen++;		//	uint8		Last value written to $400E
	fwrite(&Noise::Timer,1,1,out);		clen++;		//	uint8		Noise timer
	fwrite(&Noise::CurD,2,1,out);		clen += 2;	//	uint16		Noise duty cycle pointer
	fwrite(&Noise::EnvClk,1,1,out);		clen++;		//	uint8		Boolean flag for whether Noise envelope needs a reload
	fwrite(&Noise::EnvCtr,1,1,out);		clen++;		//	uint8		Noise envelope counter
	fwrite(&Noise::Envelope,1,1,out);	clen++;		//	uint8		Noise  envelope value
	fwrite(&Noise::Cycles,2,1,out);		clen += 2;	//	uint16		Noise cycles
	fwrite(&Regs[0x0C],1,1,out);	clen++;		//	uint8		Last value written to $400C

	fwrite(&Regs[0x10],1,1,out);	clen++;		//	uint8		Last value written to $4010
	fwrite(&Regs[0x11],1,1,out);	clen++;		//	uint8		Last value written to $4011
	fwrite(&Regs[0x12],1,1,out);	clen++;		//	uint8		Last value written to $4012
	fwrite(&Regs[0x13],1,1,out);	clen++;		//	uint8		Last value written to $4013
	fwrite(&DPCM::CurAddr,2,1,out);		clen += 2;	//	uint16		DPCM current address
	fwrite(&DPCM::SampleLen,2,1,out);	clen += 2;	//	uint16		DPCM current length
	fwrite(&DPCM::shiftreg,1,1,out);		clen++;		//	uint8		DPCM shift register
	tpc = (DPCM::buffull ? 0x1 : 0x0) | (DPCM::outmode ? 0x2 : 0x0);
	fwrite(&tpc,1,1,out);			clen++;		//	uint8		DPCM output mode(2)/buffer full(1)
	fwrite(&DPCM::outbits,1,1,out);		clen++;		//	uint8		DPCM shift count
	fwrite(&DPCM::buffer,1,1,out);		clen++;		//	uint8		DPCM read buffer
	fwrite(&DPCM::Cycles,2,1,out);		clen += 2;	//	uint16		DPCM cycles
	fwrite(&DPCM::LengthCtr,2,1,out);	clen += 2;	//	uint16		DPCM length counter

	fwrite(&Regs[0x17],1,1,out);	clen++;		//	uint8		Last value written to $4017
	fwrite(&Frame::Cycles,2,1,out);		clen += 2;	//	uint16		Frame counter cycles
	fwrite(&Frame::Num,1,1,out);		clen++;		//	uint8		Frame counter phase

	tpc = CPU::WantIRQ & (IRQ_DPCM | IRQ_FRAME);
	fwrite(&tpc,1,1,out);			clen++;		//	uint8		APU-related IRQs (PCM and FRAME, as-is)

	return clen;
}

int	Load (FILE *in)
{
	int clen = 0;
	unsigned char tpc;

	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4015, lower 4 bits
	WriteReg(0x015,tpc);	// this will ACK any DPCM IRQ

	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4001
	WriteReg(0x001,tpc);
	fread(&Square0::freq,2,1,in);		clen += 2;	//	uint16		Square0 frequency
	fread(&Square0::Timer,1,1,in);		clen++;		//	uint8		Square0 timer
	fread(&Square0::CurD,1,1,in);		clen++;		//	uint8		Square0 duty cycle pointer
	fread(&tpc,1,1,in);			clen++;		//	uint8		Boolean flags for whether Square0 envelope(2)/sweep(1) needs a reload
	Square0::EnvClk = (tpc & 0x2);
	Square0::SwpClk = (tpc & 0x1);
	fread(&Square0::EnvCtr,1,1,in);		clen++;		//	uint8		Square0 envelope counter
	fread(&Square0::Envelope,1,1,in);	clen++;		//	uint8		Square0 envelope value
	fread(&Square0::BendCtr,1,1,in);		clen++;		//	uint8		Square0 bend counter
	fread(&Square0::Cycles,2,1,in);		clen += 2;	//	uint16		Square0 cycles
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4000
	WriteReg(0x000,tpc);

	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4005
	WriteReg(0x005,tpc);
	fread(&Square1::freq,2,1,in);		clen += 2;	//	uint16		Square1 frequency
	fread(&Square1::Timer,1,1,in);		clen++;		//	uint8		Square1 timer
	fread(&Square1::CurD,1,1,in);		clen++;		//	uint8		Square1 duty cycle pointer
	fread(&tpc,1,1,in);			clen++;		//	uint8		Boolean flags for whether Square1 envelope(2)/sweep(1) needs a reload
	Square1::EnvClk = (tpc & 0x2);
	Square1::SwpClk = (tpc & 0x1);
	fread(&Square1::EnvCtr,1,1,in);		clen++;		//	uint8		Square1 envelope counter
	fread(&Square1::Envelope,1,1,in);	clen++;		//	uint8		Square1 envelope value
	fread(&Square1::BendCtr,1,1,in);		clen++;		//	uint8		Square1 bend counter
	fread(&Square1::Cycles,2,1,in);		clen += 2;	//	uint16		Square1 cycles
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4004
	WriteReg(0x004,tpc);

	fread(&Triangle::freq,2,1,in);		clen += 2;	//	uint16		Triangle frequency
	fread(&Triangle::Timer,1,1,in);		clen++;		//	uint8		Triangle timer
	fread(&Triangle::CurD,1,1,in);		clen++;		//	uint8		Triangle duty cycle pointer
	fread(&Triangle::LinClk,1,1,in);		clen++;		//	uint8		Boolean flag for whether linear counter needs reload
	fread(&Triangle::LinCtr,1,1,in);		clen++;		//	uint8		Triangle linear counter
	fread(&Triangle::Cycles,1,1,in);		clen++;		//	uint16		Triangle cycles
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4008
	WriteReg(0x008,tpc);

	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $400E
	WriteReg(0x00E,tpc);
	fread(&Noise::Timer,1,1,in);		clen++;		//	uint8		Noise timer
	fread(&Noise::CurD,2,1,in);		clen += 2;	//	uint16		Noise duty cycle pointer
	fread(&Noise::EnvClk,1,1,in);		clen++;		//	uint8		Boolean flag for whether Noise envelope needs a reload
	fread(&Noise::EnvCtr,1,1,in);		clen++;		//	uint8		Noise envelope counter
	fread(&Noise::Envelope,1,1,in);		clen++;		//	uint8		Noise  envelope value
	fread(&Noise::Cycles,2,1,in);		clen += 2;	//	uint16		Noise cycles
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $400C
	WriteReg(0x00C,tpc);

	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4010
	WriteReg(0x010,tpc);
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4011
	WriteReg(0x011,tpc);
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4012
	WriteReg(0x012,tpc);
	fread(&tpc,1,1,in);			clen++;		//	uint8		Last value written to $4013
	WriteReg(0x013,tpc);
	fread(&DPCM::CurAddr,2,1,in);		clen += 2;	//	uint16		DPCM current address
	fread(&DPCM::SampleLen,2,1,in);		clen += 2;	//	uint16		DPCM current length
	fread(&DPCM::shiftreg,1,1,in);		clen++;		//	uint8		DPCM shift register
	fread(&tpc,1,1,in);			clen++;		//	uint8		DPCM output mode(2)/buffer full(1)
	DPCM::buffull = tpc & 0x1;
	DPCM::outmode = tpc & 0x2;
	fread(&DPCM::outbits,1,1,in);		clen++;		//	uint8		DPCM shift count
	fread(&DPCM::buffer,1,1,in);		clen++;		//	uint8		DPCM read buffer
	fread(&DPCM::Cycles,2,1,in);		clen += 2;	//	uint16		DPCM cycles
	fread(&DPCM::LengthCtr,2,1,in);		clen += 2;	//	uint16		DPCM length counter

	fread(&tpc,1,1,in);			clen++;		//	uint8		Frame counter bits (last write to $4017)
	WriteReg(0x017,tpc);	// and this will ACK any frame IRQ
	fread(&Frame::Cycles,2,1,in);		clen += 2;	//	uint16		Frame counter cycles
	fread(&Frame::Num,1,1,in);		clen++;		//	uint8		Frame counter phase

	fread(&tpc,1,1,in);			clen++;		//	uint8		APU-related IRQs (PCM and FRAME, as-is)
	CPU::WantIRQ |= tpc;		// so we can reload them here
	
	return clen;
}
#else	/* NSFPLAYER */
short	sample_pos = 0;
BOOL	sample_ok = FALSE;
#endif	/* !NSFPLAYER */

void	Run (void)
{
	static int sampcycles = 0, samppos = 0;
#ifndef	NSFPLAYER
	int NewBufPos = FREQ * ++Cycles / MHz;
	if (NewBufPos >= buflen)
	{
		LPVOID bufPtr;
		DWORD bufBytes;
		unsigned long rpos, wpos;

		Cycles = NewBufPos = 0;
		if (AVI::handle)
			AVI::AddAudio();
		do
		{
			Sleep(1);
			if (!isEnabled)
				break;
			Try(Buffer->GetCurrentPosition(&rpos,&wpos),_T("Error getting audio position"))
			rpos /= LockSize;
			wpos /= LockSize;
			if (wpos < rpos)
				wpos += FRAMEBUF;
		} while ((rpos <= next_pos) && (next_pos <= wpos));
		if (isEnabled)
		{
			Try(Buffer->Lock(next_pos * LockSize,LockSize,&bufPtr,&bufBytes,NULL,0,0),_T("Error locking sound buffer"))
			memcpy(bufPtr,buffer,bufBytes);
			Try(Buffer->Unlock(bufPtr,bufBytes,NULL,0),_T("Error unlocking sound buffer"))
			next_pos = (next_pos + 1) % FRAMEBUF;
		}
	}
#else	/* NSFPLAYER */
	int NewBufPos = SAMPLERATE * ++Cycles / MHz;
	if (NewBufPos == SAMPLERATE)	/* we've generated 1 second, so we can reset our counters now */
		Cycles = NewBufPos = 0;
#endif	/* !NSFPLAYER */
	InternalClock++;
	Frame::Run();
	Race::Run();
	Square0::Run();
	Square1::Run();
	Triangle::Run();
	Noise::Run();
	DPCM::Run();

#ifdef	SOUND_FILTERING
	samppos += Square0::Pos + Square1::Pos + Triangle::Pos + Noise::Pos + DPCM::Pos;
#endif	/* SOUND_FILTERING */
	sampcycles++;
	
	if (NewBufPos != BufPos)
	{
		BufPos = NewBufPos;
#ifdef	SOUND_FILTERING
		samppos = (samppos << 6) / sampcycles;
#else	/* !SOUND_FILTERING */
		samppos = (Square0::Pos + Square1::Pos + Triangle::Pos + Noise::Pos + DPCM::Pos) << 6;
#endif	/* SOUND_FILTERING */
		if ((MI) && (MI->GenSound))
			samppos += MI->GenSound(sampcycles);
		if (samppos < -0x8000)
			samppos = -0x8000;
		if (samppos > 0x7FFF)
			samppos = 0x7FFF;
#ifndef	NSFPLAYER
		buffer[BufPos] = (short)samppos;
#else	/* NSFPLAYER */
		sample_pos = (short)samppos;
		sample_ok = TRUE;
#endif	/* !NSFPLAYER */
		samppos = sampcycles = 0;
	}
}

} // namespace APU