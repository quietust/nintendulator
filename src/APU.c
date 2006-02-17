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
#include "CPU.h"
#include "PPU.h"

struct tAPU APU;

#define	FREQ		44100
#define	BITS		16
#define	FRAMEBUF	4
const	unsigned int	LOCK_SIZE = FREQ * (BITS / 8);

const	unsigned long	DLays[32] = {0x05,0x7F,0x0A,0x01,0x14,0x02,0x28,0x03,0x50,0x04,0x1E,0x05,0x07,0x06,0x0E,0x07,0x06,0x08,0x0C,0x09,0x18,0x0A,0x30,0x0B,0x60,0x0C,0x24,0x0D,0x08,0x0E,0x10,0x0F};

static	struct
{
	unsigned char volume, envelope, wavehold, duty, swpspeed, swpdir, swpstep, swpenab, length;
	unsigned long freq;
	int Vol;
	unsigned char CurD;
	int Timer;
	int EnvCtr, Envelope, BendCtr;
	BOOL Enabled, ValidFreq;
	int Cycles;
	signed long Pos;
}	Square0, Square1;
const	signed long	Duties[4][16] = {{+4,+4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4},{+4,+4,+4,+4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4},{+4,+4,+4,+4,+4,+4,+4,+4,-4,-4,-4,-4,-4,-4,-4,-4},{+4,+4,+4,+4,+4,+4,+4,+4,+4,+4,+4,+4,-4,-4,-4,-4}};
__inline void	Square0_Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	Square0.volume = Val & 0xF;
		Square0.envelope = Val & 0x10;
		Square0.wavehold = Val & 0x20;
		Square0.duty = (Val >> 6) & 0x3;
		Square0.Vol = Square0.envelope ? Square0.volume : Square0.Envelope;
		Square0.Pos = Duties[Square0.duty][Square0.CurD] * Square0.Vol;	
		break;
	case 1:	Square0.swpspeed = Val & 0x07;
		Square0.swpdir = Val & 0x08;
		Square0.swpstep = (Val >> 4) & 0x7;
		Square0.swpenab = Val & 0x80;
		break;
	case 2:	Square0.freq &= 0x700;
		Square0.freq |= Val;
		Square0.ValidFreq = ((Square0.freq >= 0x8) && (Square0.freq <= 0x7FF));
		break;
	case 3:	Square0.freq &= 0xFF;
		Square0.freq |= (Val & 0x7) << 8;
		Square0.ValidFreq = ((Square0.freq >= 0x8) && (Square0.freq <= 0x7FF));
		Square0.length = (Val >> 3) & 0x1F;
		Square0.Timer = DLays[Square0.length];
		Square0.Envelope = 0xF;
		break;
	case 4:	if (Val)
			Square0.Enabled = TRUE;
		else
		{
			Square0.Enabled = FALSE;
			Square0.Pos = 0;
			Square0.Timer = 0;
		}
		break;
	}
}
__inline void	Square0_Run (void)
{
	if ((!Square0.Timer) || (!Square0.Enabled))
		return;
	Square0.Cycles--;
	if (!Square0.Cycles)
	{
		Square0.Cycles = Square0.freq + 1;
		Square0.CurD++;
		Square0.CurD &= 0xF;
		if (Square0.ValidFreq)
			Square0.Pos = Duties[Square0.duty][Square0.CurD] * Square0.Vol;
		else	Square0.Pos = 0;
	}
}
__inline void	Square0_QuarterFrame (void)
{
//	if (!Square0.Enabled)
//		return;
	if (!Square0.envelope)
	{
		Square0.EnvCtr--;
		if (!Square0.EnvCtr)
		{
			Square0.EnvCtr = Square0.volume + 1;
			Square0.Vol = Square0.Envelope;
			if (!Square0.Envelope)
				Square0.Envelope = (Square0.wavehold) ? 0xF : 0x0;
			else	Square0.Envelope--;
		}
	}
	else	Square0.Vol = Square0.volume;
}
__inline void	Square0_HalfFrame (void)
{
	if ((!Square0.Timer) || (!Square0.Enabled) || (!Square0.ValidFreq))
		return;
	if (Square0.swpenab)
	{
		Square0.BendCtr--;
		if (!Square0.BendCtr)
		{
			Square0.BendCtr = Square0.swpstep + 1;
			if (Square0.swpspeed)
			{
				int sweep = Square0.freq >> Square0.swpspeed;
				Square0.freq += (Square0.swpdir) ? (~sweep) : (sweep);
				Square0.ValidFreq = ((Square0.freq >= 0x8) && (Square0.freq <= 0x7FF));
			}
		}
	}
}
__inline void	Square0_WholeFrame (void)
{
	if ((!Square0.Timer) || (!Square0.Enabled))
		return;
	if (!Square0.wavehold)
	{
		Square0.Timer--;
		if (!Square0.Timer)
			Square0.Pos = 0;
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
		Square1.Pos = Duties[Square1.duty][Square1.CurD] * Square1.Vol;	
		break;
	case 1:	Square1.swpspeed = Val & 0x07;
		Square1.swpdir = Val & 0x08;
		Square1.swpstep = (Val >> 4) & 0x7;
		Square1.swpenab = Val & 0x80;
		break;
	case 2:	Square1.freq &= 0x700;
		Square1.freq |= Val;
		Square1.ValidFreq = ((Square1.freq >= 0x8) && (Square1.freq <= 0x7FF));
		break;
	case 3:	Square1.freq &= 0xFF;
		Square1.freq |= (Val & 0x7) << 8;
		Square1.ValidFreq = ((Square1.freq >= 0x8) && (Square1.freq <= 0x7FF));
		Square1.length = (Val >> 3) & 0x1F;
		Square1.Timer = DLays[Square1.length];
		Square1.Envelope = 0xF;
		break;
	case 4:	if (Val)
			Square1.Enabled = TRUE;
		else
		{
			Square1.Enabled = FALSE;
			Square1.Timer = 0;
			Square1.Pos = 0;
		}
		break;
	}
}
__inline void	Square1_Run (void)
{
	if ((!Square1.Timer) || (!Square1.Enabled))
		return;
	Square1.Cycles--;
	if (!Square1.Cycles)
	{
		Square1.Cycles = Square1.freq + 1;
		Square1.CurD++;
		Square1.CurD &= 0xF;
		if (Square1.ValidFreq)
			Square1.Pos = Duties[Square1.duty][Square1.CurD] * Square1.Vol;
		else	Square1.Pos = 0;
	}
}
__inline void	Square1_QuarterFrame (void)
{
//	if (!Square1.Enabled)
//		return;
	if (!Square1.envelope)
	{
		Square1.EnvCtr--;
		if (!Square1.EnvCtr)
		{
			Square1.EnvCtr = Square1.volume + 1;
			Square1.Vol = Square1.Envelope;
			if (!Square1.Envelope)
				Square1.Envelope = (Square1.wavehold) ? 0xF : 0x0;
			else	Square1.Envelope--;
		}
	}
	else	Square1.Vol = Square1.volume;
}
__inline void	Square1_HalfFrame (void)
{
	if ((!Square1.Timer) || (!Square1.Enabled) || (!Square1.ValidFreq))
		return;
	if (Square1.swpenab)
	{
		Square1.BendCtr--;
		if (!Square1.BendCtr)
		{
			Square1.BendCtr = Square1.swpstep + 1;
			if (Square1.swpspeed)
			{
				int sweep = Square1.freq >> Square1.swpspeed;
				Square1.freq += (Square1.swpdir) ? (-sweep) : (sweep);
				Square1.ValidFreq = ((Square1.freq >= 0x8) && (Square1.freq <= 0x7FF));
			}
		}
	}
}
__inline void	Square1_WholeFrame (void)
{
	if ((!Square1.Timer) || (!Square1.Enabled))
		return;
	if (!Square1.wavehold)
	{
		Square1.Timer--;
		if (!Square1.Timer)
			Square1.Pos = 0;
	}
}

static	struct
{
	unsigned char linear, wavehold, length;
	unsigned long freq;
	unsigned char CurD;
	int Timer, LinCtr, LinMode;
	BOOL Enabled;
	int Cycles;
	signed long Pos;
}	Triangle;
const	signed long	TriDuty[32] = {-8,-7,-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0,-1,-2,-3,-4,-5,-6,-7,-8};
__inline void	Triangle_Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	Triangle.linear = Val & 0x7F;
		if (Triangle.LinMode)
			Triangle.LinCtr = Triangle.linear;
		Triangle.wavehold = (Val >> 7) & 0x1;
		break;
	case 2:	Triangle.freq &= 0x700;
		Triangle.freq |= Val;
		break;
	case 3:	Triangle.freq &= 0xFF;
		Triangle.freq |= (Val & 0x7) << 8;
		Triangle.length = (Val >> 3) & 0x1F;
		Triangle.Timer = DLays[Triangle.length];
		Triangle.LinMode = 1;
		Triangle.LinCtr = Triangle.linear;
		break;
	case 4:	if (Val)
			Triangle.Enabled = TRUE;
		else
		{
			Triangle.Enabled = FALSE;
			Triangle.Timer = Triangle.LinCtr = 0;
			Triangle.Pos = 0;
		}
		break;
	}
}
__inline void	Triangle_Run (void)
{
	if ((!Triangle.Timer) || (!Triangle.LinCtr) || (!Triangle.Enabled))
		return;
	Triangle.Cycles--;
	if (!Triangle.Cycles)
	{
		Triangle.Cycles = Triangle.freq + 1;
		Triangle.CurD++;
		Triangle.CurD &= 0x1F;
		if (Triangle.freq >= 4)
			Triangle.Pos = TriDuty[Triangle.CurD] * 8;
	}
}
__inline void	Triangle_QuarterFrame (void)
{
	if (!Triangle.Enabled)
		return;
	if (Triangle.LinMode)
		Triangle.LinMode = Triangle.wavehold;
	if ((Triangle.LinCtr) && (!Triangle.LinMode))
	{
		Triangle.LinCtr--;
		if (!Triangle.LinCtr)
			Triangle.Pos = 0;
	}
}
__inline void	Triangle_WholeFrame (void)
{
	if ((!Triangle.Timer) || (!Triangle.Enabled))
		return;
	if (!Triangle.wavehold)
	{
		Triangle.Timer--;
		if (!Triangle.Timer)
			Triangle.Pos = 0;
	}
}

static	struct
{
	unsigned char volume, envelope, wavehold, datatype, length;
	unsigned long freq;
	unsigned long CurD;
	int Vol;
	int Timer;
	int EnvCtr, Envelope;
	BOOL Enabled;
	BOOL DoShift;
	int Cycles;
	signed long Pos;
}	Noise;
const	unsigned long	NoiseFreq[16] = {0x002,0x004,0x008,0x010,0x020,0x030,0x040,0x050,0x065,0x07F,0x0BE,0x0FE,0x17D,0x1FC,0x3F9,0x7F2};
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
	case 3:	Noise.length = (Val >> 3) & 0x1F;
		Noise.Timer = DLays[Noise.length];
		Noise.Envelope = 0xF;
		break;
	case 4:	if (Val)
			Noise.Enabled = TRUE;
		else
		{
			Noise.Enabled = FALSE;
			Noise.Timer = 0;
			Noise.Pos = 0;
		}
		break;
	}
}
__inline void	Noise_Run (void)
{
	if ((!Noise.Timer) || (!Noise.Enabled))
		return;
	Noise.Cycles--;
	if (!Noise.Cycles)
	{
		Noise.Cycles = NoiseFreq[Noise.freq] + 1;
		if (Noise.DoShift = !Noise.DoShift)
		{
			if (Noise.datatype)
				Noise.CurD = (Noise.CurD << 1) | (((Noise.CurD >> 14) ^ (Noise.CurD >> 8)) & 0x1);
			else	Noise.CurD = (Noise.CurD << 1) | (((Noise.CurD >> 14) ^ (Noise.CurD >> 13)) & 0x1);
		}
		Noise.Pos = ((Noise.CurD & 0x4000) ? -2 : 2) * Noise.Vol;
	}
}
__inline void	Noise_QuarterFrame (void)
{
	if (!Noise.Enabled)
		return;
	if (!Noise.envelope)
	{
		Noise.EnvCtr--;
		if (!Noise.EnvCtr)
		{
			Noise.EnvCtr = Noise.volume + 1;
			Noise.Vol = Noise.Envelope;
			if (!Noise.Envelope)
				Noise.Envelope = (Noise.wavehold) ? 0xF : 0x0;
			else	Noise.Envelope--;
		}
	}
	else	Noise.Vol = Noise.volume;
}
__inline void	Noise_WholeFrame (void)
{
	if ((!Noise.Timer) || (!Noise.Enabled))
		return;
	if (!Noise.wavehold)
	{
		Noise.Timer--;
		if (!Noise.Timer)
			Noise.Pos = 0;
	}
}

static	struct
{
	unsigned char Bits;
	BOOL IsIRQ;
	int Cycles;
	int Num;
}	Frame;
__inline void	Frame_Write (unsigned char Val)
{
	Frame.Bits = Val & 0xC0;
	Frame.Num = 0;
	if (Frame.Bits & 0x80)
		Frame.Cycles = 1;
	else	Frame.Cycles = APU.QuarterFrameLen;
	Frame.IsIRQ = FALSE;
}
__inline void	Frame_Run (void)
{
	Frame.Cycles--;
	if (!Frame.Cycles)
	{
		switch (Frame.Num++)
		{
		case 0:	Square0_QuarterFrame();
			Square1_QuarterFrame();
			Triangle_QuarterFrame();
			Noise_QuarterFrame();
			Frame.Cycles = APU.QuarterFrameLen;	break;
		case 1:	Square0_QuarterFrame();
			Square0_HalfFrame();
			Square1_QuarterFrame();
			Square1_HalfFrame();
			Triangle_QuarterFrame();
			Noise_QuarterFrame();
			Frame.Cycles = APU.QuarterFrameLen;	break;
		case 2:	Square0_QuarterFrame();
			Square1_QuarterFrame();
			Triangle_QuarterFrame();
			Noise_QuarterFrame();
			Frame.Cycles = APU.QuarterFrameLen;	break;
		case 3:	Square0_QuarterFrame();
			Square0_HalfFrame();
			Square0_WholeFrame();
			Square1_QuarterFrame();
			Square1_HalfFrame();
			Square1_WholeFrame();
			Triangle_QuarterFrame();
			Noise_QuarterFrame();
			Triangle_WholeFrame();
			Noise_WholeFrame();
			if (!(Frame.Bits & 0x40))
				Frame.IsIRQ = TRUE;
			Frame.Num = 0;
			if (Frame.Bits & 0x80)
				Frame.Cycles = 2 * APU.QuarterFrameLen;
			else	Frame.Cycles = APU.QuarterFrameLen;
								break;
		}
	}
	if (Frame.IsIRQ)
		CPU.WantIRQ = TRUE;
}

static	struct
{
	unsigned char freq, wavehold, doirq, pcmdata, addr, len;
	unsigned long CurAddr, SampleLen;
	unsigned char ShiftReg, PCMByte;
	BOOL IsIRQ;
	int LengthCtr;
	BOOL Enabled;
	int Cycles;
	signed long Pos;
}	DPCM;
const	unsigned long	DPCMFreqNTSC[16] = {428,380,340,320,286,254,226,214,190,160,142,128,106,85,72,54};
const	unsigned long	DPCMFreqPAL[16] = {397,353,316,297,266,236,210,199,177,149,132,119,99,79,67,50};
const	unsigned long	*DPCMFreq;
__inline void	DPCM_Write (int Reg, unsigned char Val)
{
	switch (Reg)
	{
	case 0:	DPCM.freq = Val & 0xF;
		DPCM.wavehold = (Val >> 6) & 0x1;
		DPCM.doirq = Val >> 7;
		if (!DPCM.doirq)
			DPCM.IsIRQ = FALSE;
		break;
	case 1:	DPCM.pcmdata = Val & 0x7F;
		break;
	case 2:	DPCM.addr = Val;
		break;
	case 3:	DPCM.len = Val;
		break;
	case 4:	if (Val)
		{
			if (!DPCM.Enabled)
			{
				DPCM.CurAddr = 0xC000 | (DPCM.addr << 6);
				DPCM.ShiftReg = 7;
				DPCM.LengthCtr = (DPCM.len << 4) + 1;
//				DPCM.Cycles = DPCMFreq[DPCM.freq];
				DPCM.Cycles = 1;
			}
			DPCM.Enabled = TRUE;
		}
		else	DPCM.Enabled = FALSE;
		DPCM.IsIRQ = FALSE;
		break;
	}
}
__inline void	DPCM_Run (void)
{
	if (DPCM.Enabled)
		DPCM.Cycles--;
	if (!DPCM.Cycles)
	{
		DPCM.Cycles = DPCMFreq[DPCM.freq];
		DPCM.ShiftReg++;
		if (DPCM.ShiftReg == 8)
		{
			DPCM.ShiftReg = 0;
			DPCM.PCMByte = CPU_MemGet(DPCM.CurAddr);
//			DPCM.Cycles--;
			DPCM.CurAddr++;
			if (DPCM.CurAddr == 0x10000)
				DPCM.CurAddr = 0x8000;
			DPCM.LengthCtr--;
			if (!DPCM.LengthCtr)
			{
				if (DPCM.wavehold)
				{
					DPCM.CurAddr = 0xC000 | (DPCM.addr << 6);
					DPCM.LengthCtr = (DPCM.len << 4) + 1;
				}
				else
				{
					DPCM.Enabled = FALSE;
					if (DPCM.doirq)
						CPU.WantIRQ = DPCM.IsIRQ = TRUE;
					return;
				}
			}
		}
		if ((DPCM.PCMByte >> DPCM.ShiftReg) & 1)
		{
			if (DPCM.pcmdata <= 0x7D)
				DPCM.pcmdata += 2;
		}
		else
		{
			if (DPCM.pcmdata >= 0x02)
				DPCM.pcmdata -= 2;
		}
	}
	DPCM.Pos = (DPCM.pcmdata - 0x40) * 4;
	if (DPCM.IsIRQ)
		CPU.WantIRQ = TRUE;
}

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
		APU.QuarterFrameLen = 8313; //6928
		DPCMFreq = DPCMFreqPAL;
	}
	else
	{
		MessageBox(mWnd,"Attempted to set indeterminate sound framerate!","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}
	APU.LockSize = LOCK_SIZE / APU.WantFPS;
}

void	APU_SetFPS (int FPS)
{
	BOOL Enabled = APU.isEnabled;
	APU_Release();
	APU_SetFPSVars(FPS);
	APU_Create();
	if (Enabled)
		APU_SoundON();
}

void	APU_Init (void)
{
	ZeroMemory(APU.Regs,sizeof(APU.Regs));
	APU.DirectSound		= NULL;
	APU.PrimaryBuffer	= NULL;
	APU.Buffer		= NULL;
	APU.buffer		= NULL;
	APU.isEnabled		= FALSE;
	APU.WantFPS		= 0;
	APU.MHz			= 0;
	APU.LockSize		= 0;
	APU.QuarterFrameLen	= 0;
	APU.Cycles		= 0;
	APU.BufPos		= 0;
	APU.last_pos		= 0;
}

void	APU_Create (void)
{
	DSBUFFERDESC DSBD;
	WAVEFORMATEX WFX;

	if (!APU.WantFPS)
		APU_SetFPSVars(60);

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
}

void	APU_Release (void)
{
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
}

void	APU_WriteReg (int Addy, unsigned char Val)
{
	APU.Regs[Addy] = Val;
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
	case 0x017:	APU.Regs[0x014] = Val;
			Frame_Write(Val);	break;
	default:	MessageBox(mWnd,"WTF? Invalid sound write!","Nintendulator",MB_OK);
						break;
	}
}
unsigned char	APU_Read4015 (void)
{
	unsigned char result =
		( Square0.Timer   ? 0x01 : 0) |
		( Square1.Timer   ? 0x02 : 0) |
		(Triangle.Timer   ? 0x04 : 0) |
		(   Noise.Timer   ? 0x08 : 0) |
		(    DPCM.Enabled ? 0x10 : 0) |
		(   Frame.IsIRQ   ? 0x40 : 0) |
		(    DPCM.IsIRQ   ? 0x80 : 0);
//	DPCM.IsIRQ = FALSE;
	Frame.IsIRQ = FALSE;
	return result;
}

void	APU_Reset  (void)
{
	ZeroMemory(APU.Regs,0x16);
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
}

void	APU_SoundOFF (void)
{
	APU.isEnabled = FALSE;
	if (APU.Buffer)
		IDirectSoundBuffer_Stop(APU.Buffer);
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

void	APU_Run (void)
{
	int NewBufPos, buflen = APU.LockSize / (BITS / 8);
	APU.Cycles++;
	NewBufPos = FREQ * APU.Cycles / APU.MHz;
	if (NewBufPos >= buflen)
	{
		int i;
		for (i = 0; i < buflen; i++)
			APU.buffer[i] <<= 6;
		if (APU.isEnabled)
		{
			LPVOID bufPtr;
			DWORD bufBytes;
			unsigned long play_pos, write_pos;
			do
			{
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
	Frame_Run();
	Square0_Run();
	Square1_Run();
	Triangle_Run();
	Noise_Run();
	DPCM_Run();
	if (NewBufPos != APU.BufPos)
	{
		APU.BufPos = NewBufPos;
		APU.buffer[APU.BufPos] = (short)(Square0.Pos + Square1.Pos + Triangle.Pos + Noise.Pos + DPCM.Pos);
		if ((MI) && (MI->MapperSnd))
			MI->MapperSnd(&APU.buffer[APU.BufPos],1);
	}
}
