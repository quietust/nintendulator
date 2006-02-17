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
#include "GFX.h"
#include "PPU.h"
#include "AVI.h"
#include <math.h>

struct tGFX GFX;

#define	GFX_SAFE_LOCK	/* only locks the secondary surface when necessary */

#define	GFX_Try(action,errormsg)\
{\
	if (FAILED(action))\
	{\
		GFX_Release();\
		GFX_Create();\
		PPU_GetGFXPtr();\
		if (FAILED(action))\
		{\
			MessageBox(mWnd,errormsg,"Nintendulator",MB_OK | MB_ICONERROR);\
			return;\
		}\
	}\
}

void	GFX_Init (void)
{
	ZeroMemory(&GFX.SurfDesc,sizeof(GFX.SurfDesc));
	ZeroMemory(GFX.FixedPalette,sizeof(GFX.FixedPalette));
	ZeroMemory(GFX.TextDisp,sizeof(GFX.TextDisp));
	GFX.DirectDraw = NULL;
	GFX.PrimarySurf = NULL;
	GFX.SecondarySurf = NULL;
	GFX.Clipper = NULL;
	GFX.DrawArray = NULL;
#ifdef	GFX_SAFE_LOCK
	GFX.SurfSize = 0;
#endif
	GFX.Pitch = 0;
	GFX.WantFPS = 0;
	GFX.FPSCnt = 0;
	GFX.FPSnum = 0;
	GFX.aFPScnt = 0;
	GFX.aFPSnum = 0;
	GFX.FSkip = 0;
	GFX.aFSkip = TRUE;
	GFX.ShowTextCounter = 0;
	GFX.Depth = 0;
	GFX.ClockFreq.QuadPart = 0;
	GFX.LastClockVal.QuadPart = 0;
	GFX.PaletteNum = 0;
	GFX_Create();
}

void	GFX_Create (void)
{
	QueryPerformanceFrequency(&GFX.ClockFreq);

	if (FAILED(DirectDrawCreateEx(NULL,(LPVOID *)&GFX.DirectDraw,&IID_IDirectDraw7,NULL)))
	{
		GFX_Release();
		MessageBox(mWnd,"Failed to initialize DirectDraw 7","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}

	if (FAILED(IDirectDraw7_SetCooperativeLevel(GFX.DirectDraw,mWnd,DDSCL_NORMAL)))
	{

		GFX_Release();
		MessageBox(mWnd,"Failed to set DirectDraw cooperative level","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}

	ZeroMemory(&GFX.SurfDesc,sizeof(GFX.SurfDesc));
	GFX.SurfDesc.dwSize = sizeof(GFX.SurfDesc);
	GFX.SurfDesc.dwFlags = DDSD_CAPS;
	GFX.SurfDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	if (FAILED(IDirectDraw7_CreateSurface(GFX.DirectDraw,&GFX.SurfDesc,&GFX.PrimarySurf,NULL)))
	{
		GFX_Release();
		MessageBox(mWnd,"Failed to create primary surface","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}

	if (FAILED(IDirectDraw7_CreateClipper(GFX.DirectDraw,0,&GFX.Clipper,NULL)))
	{
		GFX_Release();
		MessageBox(mWnd,"Failed to create clipper","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}

	if (FAILED(IDirectDrawClipper_SetHWnd(GFX.Clipper,0,mWnd)))
	{
		GFX_Release();
		MessageBox(mWnd,"Failed to set clipper window","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}

	if (FAILED(IDirectDrawSurface7_SetClipper(GFX.PrimarySurf,GFX.Clipper)))
	{
		GFX_Release();
		MessageBox(mWnd,"Failed to assign clipper to primary surface","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}

	ZeroMemory(&GFX.SurfDesc,sizeof(GFX.SurfDesc));
	GFX.SurfDesc.dwSize = sizeof(GFX.SurfDesc);
	GFX.SurfDesc.dwWidth = 256;
	GFX.SurfDesc.dwHeight = 240;
	GFX.SurfDesc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	GFX.SurfDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

	if (FAILED(IDirectDraw7_CreateSurface(GFX.DirectDraw,&GFX.SurfDesc,&GFX.SecondarySurf,NULL)))
	{
		GFX_Release();
		MessageBox(mWnd,"Failed to create secondary surface","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}

	ZeroMemory(&GFX.SurfDesc,sizeof(GFX.SurfDesc));
	GFX.SurfDesc.dwSize = sizeof(GFX.SurfDesc);

	if (FAILED(IDirectDrawSurface7_GetSurfaceDesc(GFX.SecondarySurf,&GFX.SurfDesc)))
	{
		GFX_Release();
		MessageBox(mWnd,"Failed to retrieve surface description","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}

	GFX.FPSCnt = GFX.FSkip;

	switch (GFX.SurfDesc.ddpfPixelFormat.dwRGBBitCount)
	{
	case 16:if (GFX.SurfDesc.ddpfPixelFormat.dwRBitMask == 0xF800)
			GFX.Depth = 16;
		else	GFX.Depth = 15;	break;
	case 32:GFX.Depth = 32;		break;
	default:MessageBox(mWnd,"Invalid bit depth detected!","Nintendulator",MB_OK | MB_ICONERROR);
		return;			break;
	}

	GFX.SurfDesc.dwSize = sizeof(GFX.SurfDesc);
	if (FAILED(IDirectDrawSurface7_Lock(GFX.SecondarySurf,NULL,&GFX.SurfDesc,DDLOCK_WAIT | DDLOCK_NOSYSLOCK,0)))
	{
		GFX_Release();
		MessageBox(mWnd,"Failed to lock secondary surface (init)","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}
#ifdef	GFX_SAFE_LOCK
	GFX.SurfSize = GFX.SurfDesc.lPitch*GFX.SurfDesc.dwHeight;
	GFX.DrawArray = malloc(GFX.SurfSize);
	memset(GFX.DrawArray,0,GFX.SurfSize);
#else
	GFX.DrawArray = GFX.SurfDesc.lpSurface;
#endif
	GFX.Pitch = GFX.SurfDesc.lPitch;
	memset(GFX.SurfDesc.lpSurface,0,GFX.SurfDesc.lPitch*GFX.SurfDesc.dwHeight);
#ifdef	GFX_SAFE_LOCK
	if (FAILED(IDirectDrawSurface7_Unlock(GFX.SecondarySurf,NULL)))
	{
		GFX_Release();
		MessageBox(mWnd,"Failed to unlock secondary surface (init)","Nintendulator",MB_OK | MB_ICONERROR);
		return;
	}
#endif
	GFX_Update();
	GFX_LoadPalette(GFX.PaletteNum);
}

void	GFX_Release (void)
{
	if (GFX.Clipper)	IDirectDrawClipper_Release(GFX.Clipper);	GFX.Clipper = NULL;
	if (GFX.SecondarySurf)	IDirectDrawSurface7_Release(GFX.SecondarySurf);	GFX.SecondarySurf = NULL;
	if (GFX.PrimarySurf)	IDirectDrawSurface7_Release(GFX.PrimarySurf);	GFX.PrimarySurf = NULL;
	if (GFX.DirectDraw)	IDirectDraw7_Release(GFX.DirectDraw);		GFX.DirectDraw = NULL;
#ifdef	GFX_SAFE_LOCK
	if (GFX.DrawArray)	free(GFX.DrawArray);
#endif
	GFX.DrawArray = NULL;
}

void	GFX_DrawScreen (void)
{
	static int TitleDelay = 0;
	GFX.FPSCnt++;
	if (GFX.FPSCnt > GFX.FSkip)
	{
		int tFPSnum;
		LARGE_INTEGER TmpClockVal;

		GFX.FPSCnt = 0;

		GFX_Update();

		QueryPerformanceCounter(&TmpClockVal);
		if (GFX.FSkip == -1)
			tFPSnum = (int) ((GFX.ClockFreq.QuadPart) / (TmpClockVal.QuadPart - GFX.LastClockVal.QuadPart));
		else	tFPSnum = (int) ((GFX.FSkip+1)*(GFX.ClockFreq.QuadPart) / (TmpClockVal.QuadPart - GFX.LastClockVal.QuadPart));
		GFX.LastClockVal = TmpClockVal;

		GFX.aFPSnum += tFPSnum;
		GFX.aFPScnt++;
		if (GFX.aFPScnt == 4)
		{
			GFX.FPSnum = GFX.aFPSnum / 4;
			if (GFX.aFSkip)
			{
				if ((GFX.FPSnum <= (GFX.WantFPS * 9 / 10)) && (GFX.FSkip < 5))
					GFX.FSkip++;
				if ((GFX.FSkip > 0) && (GFX.FPSnum >= GFX.WantFPS))
					GFX.FSkip--;
				GFX_SetFrameskip();
			}
			GFX.aFPScnt = 0;
			GFX.aFPSnum = 0;
		}
	}
	if (TitleDelay == 0)
	{
		GFX_UpdateTitlebar();
		TitleDelay = 10;
	}
	else	TitleDelay--;
}

void	GFX_SetFrameskip (void)
{
	HMENU hMenu = GetMenu(mWnd);
	switch (GFX.FSkip)
	{
	case 0:	CheckMenuRadioItem(hMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_0,MF_BYCOMMAND);	break;
	case 1:	CheckMenuRadioItem(hMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_1,MF_BYCOMMAND);	break;
	case 2:	CheckMenuRadioItem(hMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_2,MF_BYCOMMAND);	break;
	case 3:	CheckMenuRadioItem(hMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_3,MF_BYCOMMAND);	break;
	case 4:	CheckMenuRadioItem(hMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_4,MF_BYCOMMAND);	break;
	case 5:	CheckMenuRadioItem(hMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_5,MF_BYCOMMAND);	break;
	case 6:	CheckMenuRadioItem(hMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_6,MF_BYCOMMAND);	break;
	case 7:	CheckMenuRadioItem(hMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_7,MF_BYCOMMAND);	break;
	case 8:	CheckMenuRadioItem(hMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_8,MF_BYCOMMAND);	break;
	case 9:	CheckMenuRadioItem(hMenu,ID_PPU_FRAMESKIP_0,ID_PPU_FRAMESKIP_9,ID_PPU_FRAMESKIP_9,MF_BYCOMMAND);	break;
	}
}

void	GFX_Update (void)
{
	if (aviout)
		AVI_AddVideo();
#ifdef	GFX_SAFE_LOCK
	GFX_Try(IDirectDrawSurface7_Lock(GFX.SecondarySurf,NULL,&GFX.SurfDesc,DDLOCK_WAIT | DDLOCK_NOSYSLOCK | DDLOCK_WRITEONLY,NULL),"Failed to lock secondary surface")
	memcpy(GFX.SurfDesc.lpSurface,GFX.DrawArray,GFX.SurfSize);
	GFX_Try(IDirectDrawSurface7_Unlock(GFX.SecondarySurf,NULL),"Failed to unlock secondary surface")
#endif
	GFX_Repaint();
}

void	GFX_Repaint (void)
{
	if (GFX.DirectDraw)
	{
		RECT rect;
		POINT pt = {0,0};
		GetClientRect(mWnd,&rect);
		if ((rect.right == 0) || (rect.bottom == 0))
			return;
		ClientToScreen(mWnd,&pt);
		rect.left += pt.x;
		rect.right += pt.x;
		rect.top += pt.y;
		rect.bottom += pt.y;
#ifdef	GFX_SAFE_LOCK
		GFX_Try(IDirectDrawSurface7_Blt(GFX.PrimarySurf,&rect,GFX.SecondarySurf,NULL,0,NULL),"Failed to blit to primary surface")
#else
		GFX_Try(IDirectDrawSurface7_Unlock(GFX.SecondarySurf,NULL),"Failed to unlock secondary surface")
		GFX_Try(IDirectDrawSurface7_Blt(GFX.PrimarySurf,&rect,GFX.SecondarySurf,NULL,0,NULL),"Failed to blit to primary surface")
		GFX_Try(IDirectDrawSurface7_Lock(GFX.SecondarySurf,NULL,&GFX.SurfDesc,DDLOCK_WAIT | DDLOCK_NOSYSLOCK | DDLOCK_WRITEONLY,NULL),"Failed to lock secondary surface")
#endif
	}
}

static unsigned char cPalette[3][64][3] = {
	{	/* NTSC (generated) */
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00}
	},
	{	/* PAL (hardcoded) */
		{0x5B,0x5B,0x5B},{0x00,0x00,0xB4},{0x00,0x00,0xA0},{0x3D,0x00,0xB1},{0x69,0x00,0x74},{0x5B,0x00,0x00},{0x5F,0x00,0x00},{0x41,0x18,0x00},
		{0x10,0x2F,0x00},{0x08,0x4A,0x08},{0x00,0x67,0x00},{0x00,0x42,0x12},{0x00,0x28,0x6D},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0xE7,0xD5,0xC4},{0x00,0x40,0xFF},{0x22,0x0E,0xDC},{0x6B,0x47,0xFF},{0x9F,0x00,0xD7},{0xD7,0x0A,0x68},{0xC8,0x00,0x00},{0xB1,0x54,0x00},
		{0x5B,0x6A,0x00},{0x03,0x8C,0x00},{0x00,0x99,0x00},{0x00,0xA2,0x2C},{0x00,0x72,0xA4},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0xF8,0xF8,0xF8},{0x64,0xB4,0xFF},{0x8E,0xAB,0xFF},{0xC5,0x5B,0xFF},{0xF2,0x48,0xFF},{0xFF,0x49,0xDF},{0xFF,0x6D,0x48},{0xE2,0xA7,0x49},
		{0xFF,0xE0,0x00},{0x75,0xE3,0x00},{0x8A,0xE5,0x71},{0x2E,0xB8,0x78},{0x13,0xE2,0xE5},{0x78,0x78,0x78},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0xFF,0xFF,0xFF},{0xBE,0xF2,0xFF},{0xB8,0xB8,0xF8},{0xD8,0xB8,0xF8},{0xFF,0xB6,0xFF},{0xFF,0xC3,0xFF},{0xFF,0xD1,0xC7},{0xFF,0xE6,0xC8},
		{0xF8,0xED,0x88},{0xDD,0xFF,0x83},{0xB8,0xF8,0xB8},{0xD7,0xF8,0xF5},{0xC5,0xFF,0xFF},{0xF8,0xD8,0xF8},{0x00,0x00,0x00},{0x00,0x00,0x00}
	},
	{	/* Custom (loaded from file) */
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00}
	}
};

void	GFX_GenerateNTSC (int hue, double tint)
{
	const int chroma[0x10] = {0,240,210,180,150,120,90,60,30,0,330,300,270,0,0,0};
	const double brightness[3][4] = {{0.50,0.75,1.00,1.00},{0.29,0.45,0.73,0.90},{0.00,0.24,0.47,0.77}};
	const double pi = 3.14159265359;
	int x;
	for (x = 0; x < 4; x++)
	{
		int z;
		for (z = 0; z < 16; z++)
		{
			double s, y, r, g, b, theta;
			unsigned char ir, ig, ib;
			s = tint;
			if ((z < 1) || (z > 12))
				s = 0;
			if (z == 0)
				y = brightness[0][x];
			else if (z == 13)
				y = brightness[2][x];
			else if (z >= 14)
				y = 0;
			else	y = brightness[1][x];

			theta = pi * (chroma[z] + hue) / 180.0;
			r = y + s * sin(theta);
			g = y - (27.0 / 53.0) * s * sin(theta) + (10.0 / 53.0) * s * cos(theta);
			b = y - s * cos(theta);

#define CLIP(x,min,max) ((x > max) ? max : ((x < min) ? min : x))
			r *= 256;
			g *= 256;
			b *= 256;

			ir = (unsigned char)CLIP(r,0,255);
			ig = (unsigned char)CLIP(g,0,255);
			ib = (unsigned char)CLIP(b,0,255);
#undef CLIP
			cPalette[0][(x << 4) | z][0] = ir;
			cPalette[0][(x << 4) | z][1] = ig;
			cPalette[0][(x << 4) | z][2] = ib;
		}
	}
}

void	GFX_LoadPalette (int PalNum)
{
	const double EmphChanges[8][3] = {
		{1.000,1.000,1.000},
		{1.239,0.915,0.743},
		{0.794,1.086,0.882},
		{1.019,0.980,0.653},
		{0.905,1.026,1.277},
		{1.023,0.908,0.979},
		{0.741,0.987,1.001},
		{0.750,0.750,0.750}
	};

	unsigned int RV, GV, BV;
	int i;
	if (PalNum == 0)
		GFX_GenerateNTSC(340,0.5);
	for (i = 0; i < 0x200; i++)
	{
		RV = (unsigned int)((cPalette[PalNum][i & 0x3F][0] & 0xFF) * EmphChanges[i >> 6][0]);
		GV = (unsigned int)((cPalette[PalNum][i & 0x3F][1] & 0xFF) * EmphChanges[i >> 6][1]);
		BV = (unsigned int)((cPalette[PalNum][i & 0x3F][2] & 0xFF) * EmphChanges[i >> 6][2]);
		if (RV > 0xFF)	RV = 0xFF;
		if (GV > 0xFF)	GV = 0xFF;
		if (BV > 0xFF)	BV = 0xFF;

		switch (GFX.Depth)
		{
		case 15:RV >>= 3;
			GV >>= 3;
			BV >>= 3;
			GFX.FixedPalette[i] = (RV << 10) | (GV << 5) | (BV << 0);
			break;
		case 16:RV >>= 3;
			GV >>= 2;
			BV >>= 3;
			GFX.FixedPalette[i] = (RV << 11) | (GV << 5) | (BV << 0);
			break;
		case 32:GFX.FixedPalette[i] = (RV << 16) | (GV << 8) | (BV << 0);
			break;
		}
	}
}

void	__cdecl	GFX_ShowText (char *Text, ...)
{
	va_list marker;
	va_start(marker,Text);
	vsprintf(GFX.TextDisp,Text,marker);
	va_end(marker);
	GFX.ShowTextCounter = 15;
	GFX_UpdateTitlebar();
}

void	GFX_UpdateTitlebar (void)
{
	char titlebar[256];
	if (NES.Stopped)
		strcpy(titlebar,"Nintendulator - Stopped");
	else	sprintf(titlebar,"Nintendulator - %i FPS (%i %sFSkip)",GFX.FPSnum,GFX.FSkip,GFX.aFSkip?"Auto":"");
	if (GFX.ShowTextCounter > 0)
	{
		if (NES.Stopped)
			GFX.ShowTextCounter = 0;
		else	GFX.ShowTextCounter--;
		strcat(titlebar," - ");
		strcat(titlebar,GFX.TextDisp);
	}
	SetWindowText(mWnd,titlebar);
}