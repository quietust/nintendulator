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

struct tGFX GFX;

/* #define	GFX_SAFE_LOCK	/* only locks the secondary surface when necessary */

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
	GFX.SurfDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

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

void	GFX_Update (void)
{
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

void	GFX_LoadPalette (int PalNum)
{
	const float EmphChanges[8][3] = {
		{1.000f,1.000f,1.000f},
		{1.239f,0.915f,0.743f},
		{0.794f,1.086f,0.882f},
		{1.019f,0.980f,0.653f},
		{0.905f,1.026f,1.277f},
		{1.023f,0.908f,0.979f},
		{0.741f,0.987f,1.001f},
		{0.750f,0.750f,0.750f}
	};

	const unsigned long cPalette[2][64] = {
		{	//NTSC
		0x808080,0x185885,0x3C4295,0x642F92,0x85227B,0x952157,0x922A2F,0x7B3C0F,0x575100,0x2F6502,0x0F7118,0x00733C,0x026A64,0x000000,0x000000,0x000000,
		0xC0C0C0,0x4181AE,0x656BBE,0x8D58BB,0xAE4BA4,0xBE4A80,0xBB5358,0xA46538,0x807A27,0x588E2B,0x389A41,0x279C65,0x2B938D,0x3D3D3D,0x000000,0x000000,
		0xFFFFFF,0x89C8F5,0xADB3FF,0xD59FFF,0xF593EC,0xFF91C8,0xFF9BA0,0xECAC80,0xC8C26F,0xA0D572,0x80E289,0x6FE3AD,0x72DAD5,0x787878,0x000000,0x000000,
		0xFFFFFF,0xB5F4FF,0xD9DEFF,0xFFCBFF,0xFFBFFF,0xFFBDF3,0xFFC6CC,0xFFD8AB,0xF3ED9A,0xCCFF9E,0xABFFB5,0x9AFFD9,0x9EFFFF,0xC5C5C5,0x000000,0x000000
/*		0x808080,0x003da6,0x0012b0,0x440096,0xa1005e,0xc70028,0xba0600,0x8c1700,0x5c2f00,0x104500,0x054a00,0x00472e,0x004166,0x000000,0x050505,0x050505,
		0xc7c7c7,0x0077ff,0x2155ff,0x8237fa,0xeb2fb5,0xff2950,0xff2200,0xd63200,0xc46200,0x358000,0x058f00,0x008a55,0x0099cc,0x212121,0x090909,0x090909,
		0xffffff,0x0fd7ff,0x69a2ff,0xd480ff,0xff45f3,0xff618b,0xff8833,0xff9c12,0xfabc20,0x9fe30e,0x2bf035,0x0cf0a4,0x05fbff,0x5e5e5e,0x0d0d0d,0x0d0d0d,
		0xffffff,0xa6fcff,0xb3ecff,0xdaabeb,0xffa8f9,0xffabb3,0xffd2b0,0xffefa6,0xfff79c,0xd7e895,0xa6edaf,0xa2f2da,0x99fffc,0xdddddd,0x111111,0x111111*/
		},
		{	//PAL
		0x5B5B5B,0x0000B4,0x0000A0,0x3D00B1,0x690074,0x5B0000,0x5F0000,0x411800,0x102F00,0x084A08,0x006700,0x004212,0x00286D,0x000000,0x000000,0x000000,
		0xE7D5C4,0x0040FF,0x220EDC,0x6B47FF,0x9F00D7,0xD70A68,0xC80000,0xB15400,0x5B6A00,0x038C00,0x009900,0x00A22C,0x0072A4,0x000000,0x000000,0x000000,
		0xF8F8F8,0x64B4FF,0x8EABFF,0xC55BFF,0xF248FF,0xFF49DF,0xFF6D48,0xE2A749,0xFFE000,0x75E300,0x8AE571,0x2EB878,0x13E2E5,0x787878,0x000000,0x000000,
		0xFFFFFF,0xBEF2FF,0xB8B8F8,0xD8B8F8,0xFFB6FF,0xFFC3FF,0xFFD1C7,0xFFE6C8,0xF8ED88,0xDDFF83,0xB8F8B8,0xD7F8F5,0xC5FFFF,0xF8D8F8,0x000000,0x000000
		}
	};

	unsigned int RV, GV, BV;
	int i;
	for (i = 0; i < 0x200; i++)
	{
		RV = (unsigned int)(((cPalette[PalNum][i & 0x3F] >> 16) & 0xFF) * EmphChanges[i >> 6][0]);
		GV = (unsigned int)(((cPalette[PalNum][i & 0x3F] >>  8) & 0xFF) * EmphChanges[i >> 6][1]);
		BV = (unsigned int)(((cPalette[PalNum][i & 0x3F] >>  0) & 0xFF) * EmphChanges[i >> 6][2]);
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