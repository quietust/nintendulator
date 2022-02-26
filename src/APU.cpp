/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) QMT Productions
 */

#ifdef	NSFPLAYER
# include "in_nintendulator.h"
# include "MapperInterface.h"
# include "APU.h"
# include "CPU.h"
#else	/* !NSFPLAYER */
# include "stdafx.h"
# include "Nintendulator.h"
# include "resource.h"
# include "MapperInterface.h"
# include "NES.h"
# include "APU.h"
# include "CPU.h"
# include "PPU.h"
# include "AVI.h"
# include "Controllers.h"

# pragma comment(lib, "dsound.lib")
# pragma comment(lib, "dxguid.lib")
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
int			volumes[7];
BYTE			Regs[0x18];
#endif	/* !NSFPLAYER */

unsigned long		MHz;
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
	0x09,0xFD,
	0x13,0x01,
	0x27,0x03,
	0x4F,0x05,
	0x9F,0x07,
	0x3B,0x09,
	0x0D,0x0B,
	0x19,0x0D,

	0x0B,0x0F,
	0x17,0x11,
	0x2F,0x13,
	0x5F,0x15,
	0xBF,0x17,
	0x47,0x19,
	0x0F,0x1B,
	0x1F,0x1D
};
const	signed char	SquareDuty[4][8] = {
	{-4,-4,-4,-4,-4,-4,-4,+4},
	{-4,-4,-4,-4,-4,-4,+4,+4},
	{-4,-4,-4,-4,+4,+4,+4,+4},
	{+4,+4,+4,+4,+4,+4,-4,-4},
};
const	signed char	TriangleDuty[32] = {
	+7,+6,+5,+4,+3,+2,+1,+0,
	-1,-2,-3,-4,-5,-6,-7,-8,
	-8,-7,-6,-5,-4,-3,-2,-1,
	+0,+1,+2,+3,+4,+5,+6,+7,
};
const	unsigned long	NoiseFreqNTSC[16] = {
	0x002,0x004,0x008,0x010,0x020,0x030,0x040,0x050,
	0x065,0x07F,0x0BE,0x0FE,0x17D,0x1FC,0x3F9,0x7F2,
};
const	unsigned long	NoiseFreqPAL[16] = {
	0x002,0x004,0x007,0x00F,0x01E,0x02C,0x03B,0x04A,
	0x05E,0x076,0x0B1,0x0EC,0x162,0x1D8,0x3B1,0x761,
};
const	unsigned char	DPCMFreqNTSC[16] = {
	0xD6,0xBE,0xAA,0xA0,0x8F,0x7F,0x71,0x6B,
	0x5F,0x50,0x47,0x40,0x35,0x2A,0x24,0x1B,
};
const	unsigned char	DPCMFreqPAL[16] = {
	0xC7,0xB1,0x9E,0x95,0x8A,0x76,0x69,0x63,
	0x58,0x4A,0x42,0x3B,0x31,0x27,0x21,0x19,
};
const	int	FrameCyclesNTSC[5] = { 3728,7456,11185,14914,18640 };
const	int	FrameCyclesPAL[5] = { 4156,8313,12469,16626,20782 };

namespace Race
{
	unsigned char Square0_wavehold, Square0_LengthCtr1, Square0_LengthCtr2;
	unsigned char Square1_wavehold, Square1_LengthCtr1, Square1_LengthCtr2;
	unsigned char Triangle_wavehold, Triangle_LengthCtr1, Triangle_LengthCtr2;
	unsigned char Noise_wavehold, Noise_LengthCtr1, Noise_LengthCtr2;
	void Run (void);

void	PowerOn (void)
{
	Reset();
	Triangle_wavehold = Triangle_LengthCtr1 = Triangle_LengthCtr2 = 0;
}
void	Reset (void)
{
	Square0_wavehold = Square0_LengthCtr1 = Square0_LengthCtr2 = 0;
	Square1_wavehold = Square1_LengthCtr1 = Square1_LengthCtr2 = 0;
	Noise_wavehold = Noise_LengthCtr1 = Noise_LengthCtr2 = 0;
}
} // namespace race

namespace Square0
{
	unsigned char volume, envelope, wavehold, duty, swpspeed, swpdir, swpstep, swpenab;
	unsigned long freq;	// short
	unsigned char Vol;
	unsigned char CurD;
	unsigned char LengthCtr;
	unsigned char EnvCtr, Envelope, BendCtr;
	BOOL Enabled, ValidFreq, Active;
	BOOL EnvClk, SwpClk;
	unsigned long Cycles;	// short
	signed long Pos;

void	PowerOn (void)
{
	Reset();
}
void	Reset (void)
{
	volume = envelope = wavehold = duty = swpspeed = swpdir = swpstep = swpenab = 0;
	freq = 0;
	Vol = 0;
	CurD = 0;
	LengthCtr = 0;
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
	Active = LengthCtr && ValidFreq;
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
			Race::Square0_LengthCtr1 = LengthCounts[(Val >> 3) & 0x1F] + 1;
			Race::Square0_LengthCtr2 = LengthCtr;
		}
		CurD = 0;
		EnvClk = TRUE;
		break;
	case 4:	Enabled = Val ? TRUE : FALSE;
		if (!Enabled)
			LengthCtr = 0;
		break;
	}
	CheckActive();
}

inline void	Run (void)
{
	// Only run on odd clocks
	if (!(InternalClock & 1))
		return;
	if (!Cycles--)
	{
		Cycles = freq;
		CurD = (CurD - 1) & 0x7;
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
		EnvCtr = volume;
	}
	else if (!EnvCtr--)
	{
		EnvCtr = volume;
		if (Envelope)
			Envelope--;
		else	Envelope = wavehold ? 0xF : 0x0;
	}
	Vol = envelope ? volume : Envelope;
	CheckActive();
}
inline void	HalfFrame (void)
{
	if (!BendCtr--)
	{
		BendCtr = swpspeed;
		if (swpenab && swpstep && ValidFreq)
		{
			int sweep = freq >> swpstep;
			freq += swpdir ? ~sweep : sweep;
		}
	}
	if (SwpClk)
	{
		SwpClk = FALSE;
		BendCtr = swpspeed;
	}
	if (LengthCtr && !wavehold)
		LengthCtr--;
	CheckActive();
}
} // namespace Square0

namespace Square1
{
	unsigned char volume, envelope, wavehold, duty, swpspeed, swpdir, swpstep, swpenab;
	unsigned long freq;	// short
	unsigned char Vol;
	unsigned char CurD;
	unsigned char LengthCtr;
	unsigned char EnvCtr, Envelope, BendCtr;
	BOOL Enabled, ValidFreq, Active;
	BOOL EnvClk, SwpClk;
	unsigned long Cycles;	// short
	signed long Pos;

void	PowerOn (void)
{
	Reset();
}
void	Reset (void)
{
	volume = envelope = wavehold = duty = swpspeed = swpdir = swpstep = swpenab = 0;
	freq = 0;
	Vol = 0;
	CurD = 0;
	LengthCtr = 0;
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
	Active = LengthCtr && ValidFreq;
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
			Race::Square1_LengthCtr1 = LengthCounts[(Val >> 3) & 0x1F] + 1;
			Race::Square1_LengthCtr2 = LengthCtr;
		}
		CurD = 0;
		EnvClk = TRUE;
		break;
	case 4:	Enabled = Val ? TRUE : FALSE;
		if (!Enabled)
			LengthCtr = 0;
		break;
	}
	CheckActive();
}
inline void	Run (void)
{
	// Only run on odd clocks
	if (!(InternalClock & 1))
		return;
	if (!Cycles--)
	{
		Cycles = freq;
		CurD = (CurD - 1) & 0x7;
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
		EnvCtr = volume;
	}
	else if (!EnvCtr--)
	{
		EnvCtr = volume;
		if (Envelope)
			Envelope--;
		else	Envelope = wavehold ? 0xF : 0x0;
	}
	Vol = envelope ? volume : Envelope;
	CheckActive();
}
inline void	HalfFrame (void)
{
	if (!BendCtr--)
	{
		BendCtr = swpspeed;
		if (swpenab && swpstep && ValidFreq)
		{
			int sweep = freq >> swpstep;
			freq += swpdir ? -sweep : sweep;
		}
	}
	if (SwpClk)
	{
		SwpClk = FALSE;
		BendCtr = swpspeed;
	}
	if (LengthCtr && !wavehold)
		LengthCtr--;
	CheckActive();
}
} // namespace Square1

namespace Triangle
{
	unsigned char linear, wavehold;
	unsigned long freq;	// short
	unsigned char CurD;
	unsigned char LengthCtr, LinCtr;
	BOOL Enabled, Active;
	BOOL LinClk;
	unsigned long Cycles;	// short
	signed long Pos;

void	PowerOn (void)
{
	Reset();
}
void	Reset (void)
{
	linear = wavehold = 0;
	freq = 0;
	CurD = 0;
	LengthCtr = LinCtr = 0;
	Enabled = Active = FALSE;
        LinClk = FALSE;
	Pos = 0;
	Cycles = 1;
}
inline void	CheckActive (void)
{
	Active = LengthCtr && LinCtr;
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
			Race::Triangle_LengthCtr1 = LengthCounts[(Val >> 3) & 0x1F] + 1;
			Race::Triangle_LengthCtr2 = LengthCtr;
		}
		LinClk = TRUE;
		break;
	case 4:	Enabled = Val ? TRUE : FALSE;
		if (!Enabled)
			LengthCtr = 0;
		break;
	}
	CheckActive();
}
inline void	Run (void)
{
	if (!Cycles--)
	{
		Cycles = freq;
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
	if (LengthCtr && !wavehold)
		LengthCtr--;
	CheckActive();
}
} // namespace Triangle

namespace Noise
{
	unsigned char volume, envelope, wavehold, datatype;
	unsigned long freq;	// short
	unsigned long CurD;	// short
	unsigned char Vol;
	unsigned char LengthCtr;
	unsigned char EnvCtr, Envelope;
	BOOL Enabled;
	BOOL EnvClk;
	unsigned long Cycles;	// short
	signed long Pos;

const unsigned long	*FreqTable;
void	PowerOn (void)
{
	Reset();
}
void	Reset (void)
{
	volume = envelope = wavehold = datatype = 0;
	freq = 0;
	Vol = 0;
	LengthCtr = 0;
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
		if (LengthCtr)
			Pos = ((CurD & 0x4000) ? -2 : 2) * Vol;
		break;
	case 2:	freq = Val & 0xF;
		datatype = Val & 0x80;
		break;
	case 3:	if (Enabled)
		{
			Race::Noise_LengthCtr1 = LengthCounts[(Val >> 3) & 0x1F] + 1;
			Race::Noise_LengthCtr2 = LengthCtr;
		}
		EnvClk = TRUE;
		break;
	case 4:	Enabled = Val ? TRUE : FALSE;
		if (!Enabled)
			LengthCtr = 0;
		break;
	}
}
inline void	Run (void)
{
	// Only run on odd clocks
	if (!(InternalClock & 1))
		return;
	// this uses pre-decrement due to the lookup table
	if (!--Cycles)
	{
		Cycles = FreqTable[freq];
		if (datatype)
			CurD = (CurD << 1) | (((CurD >> 14) ^ (CurD >> 8)) & 0x1);
		else	CurD = (CurD << 1) | (((CurD >> 14) ^ (CurD >> 13)) & 0x1);
		if (LengthCtr)
			Pos = ((CurD & 0x4000) ? -2 : 2) * Vol;
	}
}
inline void	QuarterFrame (void)
{
	if (EnvClk)
	{
		EnvClk = FALSE;
		Envelope = 0xF;
		EnvCtr = volume;
	}
	else if (!EnvCtr--)
	{
		EnvCtr = volume;
		if (Envelope)
			Envelope--;
		else	Envelope = wavehold ? 0xF : 0x0;
	}
	Vol = envelope ? volume : Envelope;
	if (LengthCtr)
		Pos = ((CurD & 0x4000) ? -2 : 2) * Vol;
}
inline void	HalfFrame (void)
{
	if (LengthCtr && !wavehold)
		LengthCtr--;
}
} // namespace Noise

namespace DPCM
{
	unsigned char freq, wavehold, doirq, pcmdata, addr, len;
	unsigned long CurAddr, SampleLen;	// short
	BOOL silenced, bufempty, fetching;
	unsigned char shiftreg, outbits, buffer;
	unsigned long LengthCtr;	// short
	unsigned long Cycles;	// short
	signed long Pos;
	int DoStart, DoInc;

const	unsigned char	*FreqTable;
void	PowerOn (void)
{
	Reset();
}
void	Reset (void)
{
	freq = wavehold = doirq = pcmdata = addr = len = 0;
	CurAddr = SampleLen = 0;
	silenced = TRUE;
	shiftreg = buffer = 0;
	LengthCtr = 0;
	Pos = 0;

	Cycles = 511;
	bufempty = TRUE;
	fetching = FALSE;
	outbits = 8;
	DoStart = DoInc = 0;
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
			// If channel is silent, schedule a reload
			if (!LengthCtr)
				DoStart = 1;
		}
		else
		{
			DoStart = 0;
			DoInc = 0;
			LengthCtr = 0;
		}
		CPU::WantIRQ &= ~IRQ_DPCM;
		break;
	}
}
inline void	Run (void)
{
	// On odd clock, trigger reload on $4015 write
	if (InternalClock & 1)
	{
		if (DoInc && !--DoInc)
		{
			if (++CurAddr == 0x10000)
				CurAddr = 0x8000;
			if (LengthCtr && !--LengthCtr)
			{
				if (wavehold)
					DoStart = 1;
				else if (doirq)
					CPU::WantIRQ |= IRQ_DPCM;
			}
		}
		if (DoStart && !--DoStart)
		{
			CurAddr = 0xC000 | (addr << 6);
			LengthCtr = (len << 4) + 1;
		}
	}

	// Do everything else on even clocks
	if (!(InternalClock & 1))
	{
		// This uses pre-decrement due to the lookup table
		if (!--Cycles)
		{
			Cycles = FreqTable[freq];
			if (!silenced)
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
				if (!bufempty)
				{
					shiftreg = buffer;
					bufempty = TRUE;
					silenced = FALSE;
				}
				else	silenced = TRUE;
			}
		}
	}
	// If the buffer is empty and there's a sample to play, schedule DMA
	if (bufempty && !fetching && LengthCtr)
	{
		fetching = TRUE;
		CPU::EnableDMA |= DMA_PCM;
	}
}

void	Fetch (void)
{
	buffer = CPU::MemGetDMA(CurAddr);
	bufempty = FALSE;
	fetching = FALSE;
	DoInc = 1;
}
} // namespace DPCM

namespace Frame
{
	unsigned char Bits;
	int Cycles;
	BOOL Quarter, Half, IRQ, Zero;

const	int	*CycleTable;
void	PowerOn (void)
{
	Bits = 0;
	Cycles = 0;
	Quarter = Half = IRQ = Zero = FALSE;
}
void	Reset (void)
{
	Cycles = 0;
	Quarter = Half = IRQ = Zero = FALSE;
}
inline void	Write (unsigned char Val)
{
	Bits = Val & 0xC0;
	Zero = TRUE;
	if (Bits & 0x40)
		CPU::WantIRQ &= ~IRQ_FRAME;
}
inline void	Run (void)
{
	// Only run on odd clocks
	if (!(InternalClock & 1))
	{
		if (Quarter)
		{
			Square0::QuarterFrame();
			Square1::QuarterFrame();
			Triangle::QuarterFrame();
			Noise::QuarterFrame();
			Quarter = FALSE;
		}
		if (Half)
		{
			Square0::HalfFrame();
			Square1::HalfFrame();
			Triangle::HalfFrame();
			Noise::HalfFrame();
			Half = FALSE;
		}
		return;
	}

	if (IRQ)
	{
		if (!Bits)
			CPU::WantIRQ |= IRQ_FRAME;
		IRQ = FALSE;
	}

	if (Zero)
	{
		if (Bits & 0x80)
		{
			Quarter = TRUE;
			Half = TRUE;
		}
		Cycles = -1;
		Zero = FALSE;
	}
	// step A
	else if (Cycles == CycleTable[0])
	{
		Quarter = TRUE;
	}
	// step B
	else if (Cycles == CycleTable[1])
	{
		Quarter = TRUE;
		Half = TRUE;
	}
	// step C
	else if (Cycles == CycleTable[2])
	{
		Quarter = TRUE;
	}
	// step D
	else if (Cycles == CycleTable[3])
	{
		if (!(Bits & 0x80))
		{
			Quarter = TRUE;
			Half = TRUE;
			IRQ = TRUE;
			Cycles = -1;
			if (!Bits)
				CPU::WantIRQ |= IRQ_FRAME;
		}
	}
	// step E
	else if (Cycles == CycleTable[4])
	{
		Quarter = TRUE;
		Half = TRUE;
		Cycles = -1;
	}

	Cycles++;
}
} // namespace Frame

void	Race::Run (void)
{
	Square0::wavehold = Square0_wavehold;
	if (Square0_LengthCtr1)
	{
		if (Square0::LengthCtr == Square0_LengthCtr2)
			Square0::LengthCtr = Square0_LengthCtr1;
		Square0_LengthCtr1 = 0;
	}

	Square1::wavehold = Square1_wavehold;
	if (Square1_LengthCtr1)
	{
		if (Square1::LengthCtr == Square1_LengthCtr2)
			Square1::LengthCtr = Square1_LengthCtr1;
		Square1_LengthCtr1 = 0;
	}

	Triangle::wavehold = Triangle_wavehold;
	if (Triangle_LengthCtr1)
	{
		if (Triangle::LengthCtr == Triangle_LengthCtr2)
			Triangle::LengthCtr = Triangle_LengthCtr1;
		Triangle_LengthCtr1 = 0;
	}

	Noise::wavehold = Noise_wavehold;
	if (Noise_LengthCtr1)
	{
		if (Noise::LengthCtr == Noise_LengthCtr2)
			Noise::LengthCtr = Noise_LengthCtr1;
		Noise_LengthCtr1 = 0;
	}
}

void	MAPINT	IntWrite (int Bank, int Addr, int Val)
{
#ifndef	NSFPLAYER
	if (Addr < 0x018)
		Regs[Addr] = Val;
#endif	/* !NSFPLAYER */
	switch (Addr)
	{
	case 0x000:	Square0::Write(0, Val);		break;
	case 0x001:	Square0::Write(1, Val);		break;
	case 0x002:	Square0::Write(2, Val);		break;
	case 0x003:	Square0::Write(3, Val);		break;
	case 0x004:	Square1::Write(0, Val);		break;
	case 0x005:	Square1::Write(1, Val);		break;
	case 0x006:	Square1::Write(2, Val);		break;
	case 0x007:	Square1::Write(3, Val);		break;
	case 0x008:	Triangle::Write(0, Val);	break;
	case 0x00A:	Triangle::Write(2, Val);	break;
	case 0x00B:	Triangle::Write(3, Val);	break;
	case 0x00C:	Noise::Write(0, Val);		break;
	case 0x00E:	Noise::Write(2, Val);		break;
	case 0x00F:	Noise::Write(3, Val);		break;
	case 0x010:	DPCM::Write(0, Val);		break;
	case 0x011:	DPCM::Write(1, Val);		break;
	case 0x012:	DPCM::Write(2, Val);		break;
	case 0x013:	DPCM::Write(3, Val);		break;
	case 0x014:	CPU::EnableDMA |= DMA_SPR;
			CPU::DMAPage = Val;		break;
	case 0x015:	Square0::Write(4, Val & 0x1);
			Square1::Write(4, Val & 0x2);
			Triangle::Write(4, Val & 0x4);
			Noise::Write(4, Val & 0x8);
			DPCM::Write(4, Val & 0x10);	break;
#ifndef	NSFPLAYER
	case 0x016:	Controllers::Write(Val);	break;
#else	/* NSFPLAYER */
#endif	/* !NSFPLAYER */
	case 0x017:	Frame::Write(Val);		break;
	}
}

int	MAPINT	IntRead (int Bank, int Addr)
{
	int result = -1;
	switch (Addr)
	{
	case 0x015:
		result =
			((      Square0::LengthCtr) ? 0x01 : 0) |
			((      Square1::LengthCtr) ? 0x02 : 0) |
			((     Triangle::LengthCtr) ? 0x04 : 0) |
			((        Noise::LengthCtr) ? 0x08 : 0) |
			((         DPCM::LengthCtr) ? 0x10 : 0) |
			((CPU::WantIRQ & IRQ_FRAME) ? 0x40 : 0) |
			((CPU::WantIRQ &  IRQ_DPCM) ? 0x80 : 0);
		CPU::WantIRQ &= ~IRQ_FRAME;	// DPCM flag doesn't get reset
		break;
#ifndef	NSFPLAYER
	case 0x016:
		result = CPU::LastRead & 0xC0;
		result |= Controllers::Port1->Read() & 0x19;
		result |= Controllers::PortExp->Read1() & 0x1F;
		break;
	case 0x017:
		result = CPU::LastRead & 0xC0;
		result |= Controllers::Port2->Read() & 0x19;
		result |= Controllers::PortExp->Read2() & 0x1F;
		break;
#endif	/* !NSFPLAYER */
	}
	return result;
}

#ifndef	NSFPLAYER
#define	Try(action, errormsg) do {\
	if (FAILED(action))\
	{\
		Stop();\
		Start();\
		if (FAILED(action))\
		{\
			SoundOFF();\
			MessageBox(hMainWnd, errormsg, _T("Nintendulator"), MB_OK | MB_ICONERROR);\
			return;\
		}\
	}\
} while (false)
#endif	/* !NSFPLAYER */

void	SetRegion (void)
{
#ifndef	NSFPLAYER
	BOOL Enabled = isEnabled;
	BOOL Started = (Buffer != NULL);
	if (Enabled)
		SoundOFF();
	if (Started)
		Stop();
#endif	/* !NSFPLAYER */
	int WantFPS = 60;
	switch (NES::CurRegion)
	{
	case NES::REGION_NTSC:
		WantFPS = 60;
		MHz = 1789773;
		Noise::FreqTable = NoiseFreqNTSC;
		DPCM::FreqTable = DPCMFreqNTSC;
		Frame::CycleTable = FrameCyclesNTSC;
		break;
	case NES::REGION_PAL:
		WantFPS = 50;
		MHz = 1662607;
		Noise::FreqTable = NoiseFreqPAL;
		DPCM::FreqTable = DPCMFreqPAL;
		Frame::CycleTable = FrameCyclesPAL;
		break;
	case NES::REGION_DENDY:
		WantFPS = 50;
		MHz = 1773447;
		Noise::FreqTable = NoiseFreqNTSC;
		DPCM::FreqTable = DPCMFreqNTSC;
		Frame::CycleTable = FrameCyclesNTSC;
		break;
	default:
		EI.DbgOut(_T("Invalid APU region selected!"));
		break;
	}
#ifndef	NSFPLAYER
	LockSize = LOCK_SIZE / WantFPS;
	buflen = LockSize / (BITS / 8);
	if (buffer)
		delete buffer;
	buffer = new short[buflen];
	if (Started)
		Start();
	if (Enabled)
		SoundON();
#endif	/* !NSFPLAYER */
}

void	Init (void)
{
#ifndef	NSFPLAYER
	DirectSound	= NULL;
	PrimaryBuffer	= NULL;
	Buffer		= NULL;
	buffer		= new short[1];
	isEnabled	= FALSE;
#endif	/* !NSFPLAYER */
	MHz		= 1;
#ifndef	NSFPLAYER
	LockSize	= 1;
	buflen		= 0;
#endif	/* !NSFPLAYER */
	BufPos		= 0;
#ifndef	NSFPLAYER
	next_pos	= 0;


	if (FAILED(DirectSoundCreate(&DSDEVID_DefaultPlayback, &DirectSound, NULL)))
	{
		Destroy();
		MessageBox(hMainWnd, _T("Failed to create DirectSound interface!"), _T("Nintendulator"), MB_OK);
		return;
	}
	if (FAILED(DirectSound->SetCooperativeLevel(hMainWnd, DSSCL_PRIORITY)))
	{
		Destroy();
		MessageBox(hMainWnd, _T("Failed to set cooperative level!"), _T("Nintendulator"), MB_OK);
		return;
	}
#endif	/* !NSFPLAYER */
}

void	Destroy (void)
{
	Stop();
#ifndef	NSFPLAYER
	if (DirectSound)
	{
		DirectSound->Release();
		DirectSound = NULL;
	}
#endif	/* !NSFPLAYER */
}

void	Start (void)
{
#ifndef	NSFPLAYER
	WAVEFORMATEX WFX;
	DSBUFFERDESC DSBD;
	if (!DirectSound)
		return;

	ZeroMemory(&DSBD, sizeof(DSBUFFERDESC));
	DSBD.dwSize = sizeof(DSBUFFERDESC);
	DSBD.dwFlags = DSBCAPS_PRIMARYBUFFER;
	DSBD.dwBufferBytes = 0;
	DSBD.lpwfxFormat = NULL;
	if (FAILED(DirectSound->CreateSoundBuffer(&DSBD, &PrimaryBuffer, NULL)))
	{
		Stop();
		MessageBox(hMainWnd, _T("Failed to create primary buffer!"), _T("Nintendulator"), MB_OK);
		return;
	}

	ZeroMemory(&WFX, sizeof(WAVEFORMATEX));
	WFX.wFormatTag = WAVE_FORMAT_PCM;
	WFX.nChannels = 1;
	WFX.nSamplesPerSec = FREQ;
	WFX.wBitsPerSample = BITS;
	WFX.nBlockAlign = WFX.wBitsPerSample / 8 * WFX.nChannels;
	WFX.nAvgBytesPerSec = WFX.nSamplesPerSec * WFX.nBlockAlign;
	if (FAILED(PrimaryBuffer->SetFormat(&WFX)))
	{
		Stop();
		MessageBox(hMainWnd, _T("Failed to set output format!"), _T("Nintendulator"), MB_OK);
		return;
	}
	if (FAILED(PrimaryBuffer->Play(0, 0, DSBPLAY_LOOPING)))
	{
		Stop();
		MessageBox(hMainWnd, _T("Failed to start playing primary buffer!"), _T("Nintendulator"), MB_OK);
		return;
	}

	DSBD.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCSOFTWARE | DSBCAPS_GETCURRENTPOSITION2;
	DSBD.dwBufferBytes = LockSize * FRAMEBUF;
	DSBD.lpwfxFormat = &WFX;

	if (FAILED(DirectSound->CreateSoundBuffer(&DSBD, &Buffer, NULL)))
	{
		Stop();
		MessageBox(hMainWnd, _T("Failed to create secondary buffer!"), _T("Nintendulator"), MB_OK);
		return;
	}
	EI.DbgOut(_T("Created %iHz %i bit audio buffer, %i frames (%i bytes per frame)"), WFX.nSamplesPerSec, WFX.wBitsPerSample, DSBD.dwBufferBytes / LockSize, LockSize);
#endif	/* !NSFPLAYER */
}

void	Stop (void)
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
	if (buffer)
	{
		delete buffer;
		buffer = NULL;
	}
#endif	/* !NSFPLAYER */
}

void	PowerOn  (void)
{
#ifndef	NSFPLAYER
	ZeroMemory(Regs, 0x18);
#endif	/* !NSFPLAYER */
	Frame::PowerOn();
	Square0::PowerOn();
	Square1::PowerOn();
	Triangle::PowerOn();
	Noise::PowerOn();
	DPCM::PowerOn();
	Race::PowerOn();
	Cycles = 1;
	CPU::WantIRQ &= ~(IRQ_FRAME | IRQ_DPCM);
	InternalClock = 0;
}
void	Reset  (void)
{
#ifndef	NSFPLAYER
	ZeroMemory(Regs, 0x18);
#endif	/* !NSFPLAYER */
	Frame::Reset();
	Square0::Reset();
	Square1::Reset();
	Triangle::Reset();
	Noise::Reset();
	DPCM::Reset();
	Race::Reset();
	Cycles = 1;
	CPU::WantIRQ &= ~(IRQ_FRAME | IRQ_DPCM);
	InternalClock = 0;
}

#ifndef	NSFPLAYER
void	SoundOFF (void)
{
	if (!isEnabled)
		return;
	isEnabled = FALSE;
	if (Buffer)
		Buffer->Stop();
}

void	SoundON (void)
{
	LPVOID bufPtr;
	DWORD bufBytes;
	if (isEnabled)
		return;
	if (!Buffer)
	{
		Start();
		if (!Buffer)
			return;
	}
	Try(Buffer->Lock(0, 0, &bufPtr, &bufBytes, NULL, 0, DSBLOCK_ENTIREBUFFER), _T("Error locking sound buffer (Clear)"));
	ZeroMemory(bufPtr, bufBytes);
	Try(Buffer->Unlock(bufPtr, bufBytes, NULL, 0), _T("Error unlocking sound buffer (Clear)"));
	isEnabled = TRUE;
	Try(Buffer->Play(0, 0, DSBPLAY_LOOPING), _T("Unable to start playing buffer"));
	next_pos = 0;
}

INT_PTR	CALLBACK	VolumeConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const int vol_sliders[7] = {IDC_AUDIO_VOL_MASTER, IDC_AUDIO_VOL_SQ0, IDC_AUDIO_VOL_SQ1, IDC_AUDIO_VOL_TRI, IDC_AUDIO_VOL_NOI, IDC_AUDIO_VOL_PCM, IDC_AUDIO_VOL_EXT};
	const int vol_mutes[7] = {IDC_AUDIO_MUTE_MASTER, IDC_AUDIO_MUTE_SQ0, IDC_AUDIO_MUTE_SQ1, IDC_AUDIO_MUTE_TRI, IDC_AUDIO_MUTE_NOI, IDC_AUDIO_MUTE_PCM, IDC_AUDIO_MUTE_EXT};
	int wmId, wmEvent;
	int i;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		for (i = 0; i < 7; i++)
		{
			SendDlgItemMessage(hDlg, vol_sliders[i], TBM_SETRANGE, FALSE, MAKELONG(0, 100));
			SendDlgItemMessage(hDlg, vol_sliders[i], TBM_SETTICFREQ, 10, 0);
			if (volumes[i] >= 0)
			{
				SendDlgItemMessage(hDlg, vol_sliders[i], TBM_SETPOS, TRUE, 100 - volumes[i]);
				CheckDlgButton(hDlg, vol_mutes[i], BST_UNCHECKED);
			}
			else
			{
				SendDlgItemMessage(hDlg, vol_sliders[i], TBM_SETPOS, TRUE, 100 + volumes[i]);
				CheckDlgButton(hDlg, vol_mutes[i], BST_CHECKED);
			}
		}
		return TRUE;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		for (i = 0; i < 7; i++)
		{
			if (wmId == vol_mutes[i])
			{
				int vol = 100 - SendDlgItemMessage(hDlg, vol_sliders[i], TBM_GETPOS, 0, 0);
				if (IsDlgButtonChecked(hDlg, vol_mutes[i]) == BST_CHECKED)
					volumes[i] = -vol;
				else	volumes[i] = vol;
				return TRUE;
			}
		}
		if (wmId == IDOK)
		{
			EndDialog(hDlg, 0);
			return TRUE;
		}
		break;
	case WM_VSCROLL:
		for (i = 0; i < 7; i++)
		{
			if (lParam == (LPARAM)GetDlgItem(hDlg, vol_sliders[i]))
			{
				int vol = 100 - SendDlgItemMessage(hDlg, vol_sliders[i], TBM_GETPOS, 0, 0);
				if (IsDlgButtonChecked(hDlg, vol_mutes[i]) == BST_CHECKED)
					volumes[i] = -vol;
				else	volumes[i] = vol;
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}

void	Config (void)
{
	DialogBox(hInst, MAKEINTRESOURCE(IDD_VOLUME), hMainWnd, VolumeConfigProc);
}

int	Save (FILE *out)
{
	int clen = 0;
	unsigned char tpc;
	tpc = Regs[0x15] & 0xF;
	writeByte(tpc);			//	uint8		Last value written to $4015, lower 4 bits

	writeByte(Regs[0x01]);		//	uint8		Last value written to $4001
	writeWord(Square0::freq);	//	uint16		Square0 frequency
	writeByte(Square0::LengthCtr);	//	uint8		Square0 timer
	writeByte(Square0::CurD);	//	uint8		Square0 duty cycle pointer
	tpc = (Square0::EnvClk ? 0x2 : 0x0) | (Square0::SwpClk ? 0x1 : 0x0);
	writeByte(tpc);			//	uint8		Boolean flags for whether Square0 envelope(2)/sweep(1) needs a reload
	writeByte(Square0::EnvCtr);	//	uint8		Square0 envelope counter
	writeByte(Square0::Envelope);	//	uint8		Square0 envelope value
	writeByte(Square0::BendCtr);	//	uint8		Square0 bend counter
	writeWord(Square0::Cycles);	//	uint16		Square0 cycles
	writeByte(Regs[0x00]);		//	uint8		Last value written to $4000

	writeByte(Regs[0x05]);		//	uint8		Last value written to $4005
	writeWord(Square1::freq);	//	uint16		Square1 frequency
	writeByte(Square1::LengthCtr);	//	uint8		Square1 timer
	writeByte(Square1::CurD);	//	uint8		Square1 duty cycle pointer
	tpc = (Square1::EnvClk ? 0x2 : 0x0) | (Square1::SwpClk ? 0x1 : 0x0);
	writeByte(tpc);			//	uint8		Boolean flags for whether Square1 envelope(2)/sweep(1) needs a reload
	writeByte(Square1::EnvCtr);	//	uint8		Square1 envelope counter
	writeByte(Square1::Envelope);	//	uint8		Square1 envelope value
	writeByte(Square1::BendCtr);	//	uint8		Square1 bend counter
	writeWord(Square1::Cycles);	//	uint16		Square1 cycles
	writeByte(Regs[0x04]);		//	uint8		Last value written to $4004

	writeWord(Triangle::freq);	//	uint16		Triangle frequency
	writeByte(Triangle::LengthCtr);	//	uint8		Triangle timer
	writeByte(Triangle::CurD);	//	uint8		Triangle duty cycle pointer
	writeByte(Triangle::LinClk);	//	uint8		Boolean flag for whether linear counter needs reload
	writeByte(Triangle::LinCtr);	//	uint8		Triangle linear counter
	writeByte(Triangle::Cycles);	//	uint16		Triangle cycles
	writeByte(Regs[0x08]);		//	uint8		Last value written to $4008

	writeByte(Regs[0x0E]);		//	uint8		Last value written to $400E
	writeByte(Noise::LengthCtr);	//	uint8		Noise timer
	writeWord(Noise::CurD);		//	uint16		Noise duty cycle pointer
	writeByte(Noise::EnvClk);	//	uint8		Boolean flag for whether Noise envelope needs a reload
	writeByte(Noise::EnvCtr);	//	uint8		Noise envelope counter
	writeByte(Noise::Envelope);	//	uint8		Noise  envelope value
	writeWord(Noise::Cycles);	//	uint16		Noise cycles
	writeByte(Regs[0x0C]);		//	uint8		Last value written to $400C

	writeByte(Regs[0x10]);		//	uint8		Last value written to $4010
	writeByte(Regs[0x11]);		//	uint8		Last value written to $4011
	writeByte(Regs[0x12]);		//	uint8		Last value written to $4012
	writeByte(Regs[0x13]);		//	uint8		Last value written to $4013
	writeWord(DPCM::CurAddr);	//	uint16		DPCM current address
	writeWord(DPCM::SampleLen);	//	uint16		DPCM current length
	writeByte(DPCM::shiftreg);	//	uint8		DPCM shift register
	tpc = (DPCM::DoInc ? 0x10 : 0) | (DPCM::DoStart ? 0x8 : 0) | (DPCM::fetching ? 0x4 : 0x0) | (DPCM::silenced ? 0x0 : 0x2) | (DPCM::bufempty ? 0x0 : 0x1);	// variables were renamed and inverted
	writeByte(tpc);			//	uint8		DPCM incrementing(D4)/resetting(D3)/fetching(D2)/!silenced(D1)/!empty(D0)
	writeByte(DPCM::outbits);	//	uint8		DPCM shift count
	writeByte(DPCM::buffer);	//	uint8		DPCM read buffer
	writeWord(DPCM::Cycles);	//	uint16		DPCM cycles
	writeWord(DPCM::LengthCtr);	//	uint16		DPCM length counter

	writeByte(Regs[0x17]);		//	uint8		Last value written to $4017
	writeWord(Frame::Cycles);	//	uint16		Frame counter cycles
	tpc = (Frame::Zero ? 0x8 : 0) | (Frame::IRQ ? 0x4 : 0) | (Frame::Half ? 0x2 : 0) | (Frame::Quarter ? 0x1 : 0);
	writeByte(tpc);			//	uint8		Frame counter Zero(D3)/IRQ(D2)/Half(D1)/Quarter(D0) pending

	tpc = CPU::WantIRQ & (IRQ_DPCM | IRQ_FRAME);
	writeByte(tpc);			//	uint8		APU-related IRQs (PCM and FRAME, as-is)
	tpc = InternalClock & 0xFF;
	writeByte(tpc);			//	uint8		APU clock, lower 8 bits (for phase)

	return clen;
}

int	Load (FILE *in, int version_id)
{
	int clen = 0;
	unsigned char tpc;

	readByte(tpc);			//	uint8		Last value written to $4015, lower 4 bits
	IntWrite(0x4, 0x015, tpc);	// this will ACK any DPCM IRQ

	readByte(tpc);			//	uint8		Last value written to $4001
	IntWrite(0x4, 0x001, tpc);
	readWord(Square0::freq);	//	uint16		Square0 frequency
	readByte(Square0::LengthCtr);	//	uint8		Square0 timer
	readByte(Square0::CurD);	//	uint8		Square0 duty cycle pointer
	readByte(tpc);			//	uint8		Boolean flags for whether Square0 envelope(2)/sweep(1) needs a reload
	Square0::EnvClk = (tpc & 0x2);
	Square0::SwpClk = (tpc & 0x1);
	readByte(Square0::EnvCtr);	//	uint8		Square0 envelope counter
	readByte(Square0::Envelope);	//	uint8		Square0 envelope value
	readByte(Square0::BendCtr);	//	uint8		Square0 bend counter
	readWord(Square0::Cycles);	//	uint16		Square0 cycles
	if (version_id < 1004)
		Square0::Cycles >>= 1;
	readByte(tpc);			//	uint8		Last value written to $4000
	IntWrite(0x4, 0x000, tpc);

	readByte(tpc);			//	uint8		Last value written to $4005
	IntWrite(0x4, 0x005, tpc);
	readWord(Square1::freq);	//	uint16		Square1 frequency
	readByte(Square1::LengthCtr);	//	uint8		Square1 timer
	readByte(Square1::CurD);	//	uint8		Square1 duty cycle pointer
	readByte(tpc);			//	uint8		Boolean flags for whether Square1 envelope(2)/sweep(1) needs a reload
	Square1::EnvClk = (tpc & 0x2);
	Square1::SwpClk = (tpc & 0x1);
	readByte(Square1::EnvCtr);	//	uint8		Square1 envelope counter
	readByte(Square1::Envelope);	//	uint8		Square1 envelope value
	readByte(Square1::BendCtr);	//	uint8		Square1 bend counter
	readWord(Square1::Cycles);	//	uint16		Square1 cycles
	if (version_id < 1004)
		Square1::Cycles >>= 1;
	readByte(tpc);			//	uint8		Last value written to $4004
	IntWrite(0x4, 0x004, tpc);

	readWord(Triangle::freq);	//	uint16		Triangle frequency
	readByte(Triangle::LengthCtr);	//	uint8		Triangle timer
	readByte(Triangle::CurD);	//	uint8		Triangle duty cycle pointer
	readByte(Triangle::LinClk);	//	uint8		Boolean flag for whether linear counter needs reload
	readByte(Triangle::LinCtr);	//	uint8		Triangle linear counter
	readByte(Triangle::Cycles);	//	uint16		Triangle cycles
	readByte(tpc);			//	uint8		Last value written to $4008
	IntWrite(0x4, 0x008, tpc);

	readByte(tpc);			//	uint8		Last value written to $400E
	IntWrite(0x4, 0x00E, tpc);
	readByte(Noise::LengthCtr);	//	uint8		Noise timer
	readWord(Noise::CurD);		//	uint16		Noise duty cycle pointer
	readByte(Noise::EnvClk);	//	uint8		Boolean flag for whether Noise envelope needs a reload
	readByte(Noise::EnvCtr);	//	uint8		Noise envelope counter
	readByte(Noise::Envelope);	//	uint8		Noise  envelope value
	readWord(Noise::Cycles);	//	uint16		Noise cycles
	if (version_id < 1004)
		Noise::Cycles >>= 1;
	readByte(tpc);			//	uint8		Last value written to $400C
	IntWrite(0x4, 0x00C, tpc);

	readByte(tpc);			//	uint8		Last value written to $4010
	IntWrite(0x4, 0x010, tpc);
	readByte(tpc);			//	uint8		Last value written to $4011
	IntWrite(0x4, 0x011, tpc);
	readByte(tpc);			//	uint8		Last value written to $4012
	IntWrite(0x4, 0x012, tpc);
	readByte(tpc);			//	uint8		Last value written to $4013
	IntWrite(0x4, 0x013, tpc);
	readWord(DPCM::CurAddr);	//	uint16		DPCM current address
	readWord(DPCM::SampleLen);	//	uint16		DPCM current length
	readByte(DPCM::shiftreg);	//	uint8		DPCM shift register
	readByte(tpc);			//	uint8		DPCM incrementing(D4)/resetting(D3)/fetching(D2)/!silenced(D1)/!empty(D0)
	if (version_id >= 1004)
	{
		DPCM::DoInc = !!(tpc & 0x10);
		DPCM::DoStart = !!(tpc & 0x8);
	}
	else	DPCM::DoInc = DPCM::DoStart = FALSE;
	DPCM::fetching = !!(tpc & 0x4);
	DPCM::silenced = !(tpc & 0x2);	// variable was renamed and inverted
	DPCM::bufempty = !(tpc & 0x1);	// variable was renamed and inverted
	readByte(DPCM::outbits);	//	uint8		DPCM shift count
	readByte(DPCM::buffer);		//	uint8		DPCM read buffer
	readWord(DPCM::Cycles);		//	uint16		DPCM cycles
	if (version_id < 1004)
		DPCM::Cycles >>= 1;
	readWord(DPCM::LengthCtr);	//	uint16		DPCM length counter

	readByte(tpc);			//	uint8		Frame counter bits (last write to $4017)
	IntWrite(0x4, 0x017, tpc);	// and this will ACK any frame IRQ
	readWord(Frame::Cycles);	//	uint16		Frame counter cycles
	if (version_id < 1004)
		Frame::Cycles >>= 1;
	if (version_id < 1001)
		readByte(_val);		//	uint8		Frame counter phase
	if (version_id >= 1004)
	{
		readByte(tpc);		//	uint8		Frame counter Zero(D3)/IRQ(D2)/Half(D1)/Quarter(D0) pending
		Frame::Zero = !!(tpc & 0x8);
		Frame::IRQ = !!(tpc & 0x4);
		Frame::Half = !!(tpc & 0x2);
		Frame::Quarter = !!(tpc & 0x1);
	}
	else	Frame::Zero = Frame::IRQ = Frame::Half = Frame::Quarter = FALSE;

	readByte(tpc);			//	uint8		APU-related IRQs (PCM and FRAME, as-is)
	CPU::WantIRQ |= tpc;	// so we can reload them here

	readByte(tpc);			//	uint8		APU clock, lower 8 bits (for phase)
	InternalClock = tpc;

	return clen;
}

void	SaveSettings (HKEY SettingsBase)
{
	RegSetValueEx(SettingsBase, _T("VolMaster"), 0, REG_DWORD, (LPBYTE)&volumes[0], sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("VolSq0")   , 0, REG_DWORD, (LPBYTE)&volumes[1], sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("VolSq1")   , 0, REG_DWORD, (LPBYTE)&volumes[2], sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("VolTri")   , 0, REG_DWORD, (LPBYTE)&volumes[3], sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("VolNoi")   , 0, REG_DWORD, (LPBYTE)&volumes[4], sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("VolPCM")   , 0, REG_DWORD, (LPBYTE)&volumes[5], sizeof(DWORD));
	RegSetValueEx(SettingsBase, _T("VolExt")   , 0, REG_DWORD, (LPBYTE)&volumes[6], sizeof(DWORD));
}

void	LoadSettings (HKEY SettingsBase)
{
	unsigned long Size;

	// Defaults
	for (int i = 0; i < 7; i++)
		volumes[i] = 100;

	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("VolMaster"), 0, NULL, (LPBYTE)&volumes[0], &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("VolSq0")   , 0, NULL, (LPBYTE)&volumes[1], &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("VolSq1")   , 0, NULL, (LPBYTE)&volumes[2], &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("VolTri")   , 0, NULL, (LPBYTE)&volumes[3], &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("VolNoi")   , 0, NULL, (LPBYTE)&volumes[4], &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("VolPCM")   , 0, NULL, (LPBYTE)&volumes[5], &Size);
	Size = sizeof(DWORD);	RegQueryValueEx(SettingsBase, _T("VolExt")   , 0, NULL, (LPBYTE)&volumes[6], &Size);
}

#else	/* NSFPLAYER */
short	sample_pos = 0;
BOOL	sample_ok = FALSE;
#endif	/* !NSFPLAYER */
int sampcycles = 0, samppos = 0;

void	Run (void)
{
#ifndef	NSFPLAYER
	int NewBufPos = FREQ * ++Cycles / MHz;
	if (NewBufPos >= buflen)
	{
		LPVOID bufPtr;
		DWORD bufBytes;
		unsigned long rpos, wpos;

		Cycles = NewBufPos = 0;
		if (AVI::IsActive())
			AVI::AddAudio();
		do
		{
			Sleep(1);
			if (!isEnabled)
				break;
			Try(Buffer->GetCurrentPosition(&rpos, &wpos), _T("Error getting audio position"));
			rpos /= LockSize;
			wpos /= LockSize;
			if (wpos < rpos)
				wpos += FRAMEBUF;
		} while ((rpos <= next_pos) && (next_pos <= wpos));
		if (isEnabled)
		{
			Try(Buffer->Lock(next_pos * LockSize, LockSize, &bufPtr, &bufBytes, NULL, 0, 0), _T("Error locking sound buffer"));
			memcpy(bufPtr, buffer, bufBytes);
			Try(Buffer->Unlock(bufPtr, bufBytes, NULL, 0), _T("Error unlocking sound buffer"));
			next_pos = (next_pos + 1) % FRAMEBUF;
		}
	}
#define	VolAdjust(pos, vol) ((volumes[vol] > 0) ? (((pos) * volumes[vol]) / 100) : 0)
#else	/* NSFPLAYER */
#define	VolAdjust(pos, vol) (pos)
	int NewBufPos = SAMPLERATE * ++Cycles / MHz;
	if (NewBufPos == SAMPLERATE)	// we've generated 1 second, so we can reset our counters now
		Cycles = NewBufPos = 0;
#endif	/* !NSFPLAYER */
	Frame::Run();
	Race::Run();
	Square0::Run();
	Square1::Run();
	Triangle::Run();
	Noise::Run();
	DPCM::Run();

#ifdef	SOUND_FILTERING
	samppos += VolAdjust(Square0::Pos, 1) + VolAdjust(Square1::Pos, 2) + VolAdjust(Triangle::Pos, 3) + VolAdjust(Noise::Pos, 4) + VolAdjust(DPCM::Pos, 5);
#endif	/* SOUND_FILTERING */
	sampcycles++;
	
	if (NewBufPos != BufPos)
	{
		BufPos = NewBufPos;
#ifdef	SOUND_FILTERING
		samppos = (samppos << 6) / sampcycles;
#else	/* !SOUND_FILTERING */
		samppos = (VolAdjust(Square0::Pos, 1) + VolAdjust(Square1::Pos, 2) + VolAdjust(Triangle::Pos, 3) + VolAdjust(Noise::Pos, 4) + VolAdjust(DPCM::Pos, 5)) << 6;
#endif	/* SOUND_FILTERING */
		if ((MI) && (MI->GenSound))
			samppos += VolAdjust(MI->GenSound(sampcycles), 6);
		samppos = VolAdjust(samppos, 0);
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
