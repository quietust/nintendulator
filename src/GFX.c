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

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "MapperInterface.h"
#include "NES.h"
#include "GFX.h"
#include "PPU.h"
#include "AVI.h"
#include <commctrl.h>

struct tGFX GFX;

#define	GFX_Try(action,errormsg,failaction)\
{\
	if (FAILED(action))\
	{\
		GFX_Release();\
		MessageBox(mWnd,errormsg _T(", retrying"),_T("Nintendulator"),MB_OK | MB_ICONWARNING);\
		GFX_Create();\
		PPU_GetGFXPtr();\
		if (FAILED(action))\
		{\
			MessageBox(mWnd,_T("Error: ") errormsg,_T("Nintendulator"),MB_OK | MB_ICONERROR);\
			failaction;\
			return;\
		}\
	}\
}
void	GFX_Init (void)
{
	ZeroMemory(&GFX.SurfDesc,sizeof(GFX.SurfDesc));
	ZeroMemory(GFX.Palette15,sizeof(GFX.Palette15));
	ZeroMemory(GFX.Palette16,sizeof(GFX.Palette16));
	ZeroMemory(GFX.Palette32,sizeof(GFX.Palette32));
	GFX.DirectDraw = NULL;
	GFX.PrimarySurf = NULL;
	GFX.SecondarySurf = NULL;
	GFX.Clipper = NULL;
	GFX.Pitch = 0;
	GFX.WantFPS = 0;
	GFX.FPSCnt = 0;
	GFX.FPSnum = 0;
	GFX.aFPScnt = 0;
	GFX.aFPSnum = 0;
	GFX.FSkip = 0;
	GFX.aFSkip = TRUE;
	GFX.Depth = 0;
	GFX.ClockFreq.QuadPart = 0;
	GFX.LastClockVal.QuadPart = 0;
	GFX.PaletteNTSC = 0;
	GFX.PalettePAL = 1;
	GFX.NTSChue = 330;
	GFX.NTSCsat = 50;
	GFX_Create();
	GFX.Semaphore = CreateSemaphore(NULL,1,1,NULL);
}

void	GFX_Create (void)
{
	if (!QueryPerformanceFrequency(&GFX.ClockFreq))
	{
		MessageBox(mWnd,_T("Failed to determine performance counter frequency!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}

	if (FAILED(DirectDrawCreateEx(NULL,(LPVOID *)&GFX.DirectDraw,&IID_IDirectDraw7,NULL)))
	{
		GFX_Release();
		MessageBox(mWnd,_T("Failed to initialize DirectDraw 7"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}

	if (FAILED(IDirectDraw7_SetCooperativeLevel(GFX.DirectDraw,mWnd,DDSCL_NORMAL)))
	{

		GFX_Release();
		MessageBox(mWnd,_T("Failed to set DirectDraw cooperative level"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}

	ZeroMemory(&GFX.SurfDesc,sizeof(GFX.SurfDesc));
	GFX.SurfDesc.dwSize = sizeof(GFX.SurfDesc);
	GFX.SurfDesc.dwFlags = DDSD_CAPS;
	GFX.SurfDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	if (FAILED(IDirectDraw7_CreateSurface(GFX.DirectDraw,&GFX.SurfDesc,&GFX.PrimarySurf,NULL)))
	{
		GFX_Release();
		MessageBox(mWnd,_T("Failed to create primary surface"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}

	if (FAILED(IDirectDraw7_CreateClipper(GFX.DirectDraw,0,&GFX.Clipper,NULL)))
	{
		GFX_Release();
		MessageBox(mWnd,_T("Failed to create clipper"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}

	if (FAILED(IDirectDrawClipper_SetHWnd(GFX.Clipper,0,mWnd)))
	{
		GFX_Release();
		MessageBox(mWnd,_T("Failed to set clipper window"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}

	if (FAILED(IDirectDrawSurface7_SetClipper(GFX.PrimarySurf,GFX.Clipper)))
	{
		GFX_Release();
		MessageBox(mWnd,_T("Failed to assign clipper to primary surface"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
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
		MessageBox(mWnd,_T("Failed to create secondary surface"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}

	ZeroMemory(&GFX.SurfDesc,sizeof(GFX.SurfDesc));
	GFX.SurfDesc.dwSize = sizeof(GFX.SurfDesc);

	if (FAILED(IDirectDrawSurface7_GetSurfaceDesc(GFX.SecondarySurf,&GFX.SurfDesc)))
	{
		GFX_Release();
		MessageBox(mWnd,_T("Failed to retrieve surface description"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}

	GFX.FPSCnt = GFX.FSkip;

	switch (GFX.SurfDesc.ddpfPixelFormat.dwRGBBitCount)
	{
	case 16:if (GFX.SurfDesc.ddpfPixelFormat.dwRBitMask == 0xF800)
			GFX.Depth = 16;
		else	GFX.Depth = 15;	break;
	case 32:GFX.Depth = 32;		break;
	default:
		GFX_Release();
		MessageBox(mWnd,_T("Invalid bit depth detected!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;			break;
	}

	GFX.SurfDesc.dwSize = sizeof(GFX.SurfDesc);
	if (FAILED(IDirectDrawSurface7_Lock(GFX.SecondarySurf,NULL,&GFX.SurfDesc,DDLOCK_WAIT | DDLOCK_NOSYSLOCK,0)))
	{
		GFX_Release();
		MessageBox(mWnd,_T("Failed to lock secondary surface (init)"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}
	GFX.Pitch = GFX.SurfDesc.lPitch;
	memset(GFX.SurfDesc.lpSurface,0,GFX.SurfDesc.lPitch*GFX.SurfDesc.dwHeight);
	if (FAILED(IDirectDrawSurface7_Unlock(GFX.SecondarySurf,NULL)))
	{
		GFX_Release();
		MessageBox(mWnd,_T("Failed to unlock secondary surface (init)"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
		return;
	}
	if (WaitForSingleObject(GFX.Semaphore,0) == WAIT_OBJECT_0)
	{
		ReleaseSemaphore(GFX.Semaphore,1,NULL);
		GFX_Update();
	}
	GFX_LoadPalette(PPU.IsPAL ? GFX.PalettePAL : GFX.PaletteNTSC);
}

void	GFX_Release (void)
{
	if (GFX.Clipper)	IDirectDrawClipper_Release(GFX.Clipper);	GFX.Clipper = NULL;
	if (GFX.SecondarySurf)	IDirectDrawSurface7_Release(GFX.SecondarySurf);	GFX.SecondarySurf = NULL;
	if (GFX.PrimarySurf)	IDirectDrawSurface7_Release(GFX.PrimarySurf);	GFX.PrimarySurf = NULL;
	if (GFX.DirectDraw)	IDirectDraw7_Release(GFX.DirectDraw);		GFX.DirectDraw = NULL;
}

void	GFX_DrawScreen (void)
{
	LARGE_INTEGER TmpClockVal;
	static int TitleDelay = 0;
	if (aviout)
		AVI_AddVideo();
	if (GFX.SlowDown)
		Sleep(GFX.SlowRate * 1000 / GFX.WantFPS);
	if (++GFX.FPSCnt > GFX.FSkip)
	{
		GFX_Update();
		GFX.FPSCnt = 0;
	}
	QueryPerformanceCounter(&TmpClockVal);
	GFX.aFPSnum += (int)(TmpClockVal.QuadPart - GFX.LastClockVal.QuadPart);
	GFX.LastClockVal = TmpClockVal;
	if (++GFX.aFPScnt >= 20)
	{
		GFX.FPSnum = (int)((GFX.ClockFreq.QuadPart * GFX.aFPScnt) / GFX.aFPSnum);
		if (GFX.aFSkip)
		{
			if ((GFX.FSkip < 9) && (GFX.FPSnum <= (GFX.WantFPS * 9 / 10)))
				GFX.FSkip++;
			if ((GFX.FSkip > 0) && (GFX.FPSnum >= (GFX.WantFPS - 1)))
				GFX.FSkip--;
			GFX_SetFrameskip();
		}
		GFX.aFPScnt = 0;
		GFX.aFPSnum = 0;
	}
	if (!TitleDelay--)
	{
		UpdateTitlebar();
		TitleDelay = 10;
	}
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
	register unsigned short *src = DrawArray;
	if (!GFX.DirectDraw)
		return;
	WaitForSingleObject(GFX.Semaphore,INFINITE);
	GFX_Try(IDirectDrawSurface7_Lock(GFX.SecondarySurf,NULL,&GFX.SurfDesc,DDLOCK_WAIT | DDLOCK_NOSYSLOCK | DDLOCK_WRITEONLY,NULL),_T("Failed to lock secondary surface"),ReleaseSemaphore(GFX.Semaphore,1,NULL))
	if (GFX.Depth == 32)
	{
		int x, y;
		for (y = 0; y < 240; y++)
		{
			register unsigned long *dst = (unsigned long *)((unsigned char *)GFX.SurfDesc.lpSurface + y*GFX.Pitch);
			for (x = 0; x < 256; x++)
				dst[x] = GFX.Palette32[src[x]];
			src += x;
		}
	}
	else if (GFX.Depth == 16)
	{
		int x, y;
		for (y = 0; y < 240; y++)
		{
			register unsigned short *dst = (unsigned short *)((unsigned char *)GFX.SurfDesc.lpSurface + y*GFX.Pitch);
			for (x = 0; x < 256; x++)
				dst[x] = GFX.Palette16[src[x]];
			src += x;
		}
	}
	else
	{
		int x, y;
		for (y = 0; y < 240; y++)
		{
			register unsigned short *dst = (unsigned short *)((unsigned char *)GFX.SurfDesc.lpSurface + y*GFX.Pitch);
			for (x = 0; x < 256; x++)
				dst[x] = GFX.Palette15[src[x]];
			src += x;
		}
	}
	GFX_Try(IDirectDrawSurface7_Unlock(GFX.SecondarySurf,NULL),_T("Failed to unlock secondary surface"),ReleaseSemaphore(GFX.Semaphore,1,NULL))
	ReleaseSemaphore(GFX.Semaphore,1,NULL);
	GFX_Repaint();
}

void	GFX_Repaint (void)
{
	RECT rect;
	POINT pt = {0,0};
	if (!GFX.DirectDraw)
		return;
	GetClientRect(mWnd,&rect);
	if ((rect.right == 0) || (rect.bottom == 0))
		return;
	ClientToScreen(mWnd,&pt);
	rect.left += pt.x;
	rect.right += pt.x;
	rect.top += pt.y;
	rect.bottom += pt.y;
	WaitForSingleObject(GFX.Semaphore,INFINITE);
	GFX_Try(IDirectDrawSurface7_Blt(GFX.PrimarySurf,&rect,GFX.SecondarySurf,NULL,0,NULL),_T("Failed to blit to primary surface"),ReleaseSemaphore(GFX.Semaphore,1,NULL))
	ReleaseSemaphore(GFX.Semaphore,1,NULL);
}

#define	PALETTE_NTSC	0
#define	PALETTE_PAL	1
#define	PALETTE_PC10	2
#define	PALETTE_EXT	3
#define	PALETTE_TEMP	4

static unsigned char cPalette[5][64][3] = {
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
		{0x80,0x80,0x80},{0x00,0x3D,0xA6},{0x00,0x12,0xB0},{0x44,0x00,0x96},{0xA1,0x00,0x5E},{0xC7,0x00,0x28},{0xBA,0x06,0x00},{0x8C,0x17,0x00},
		{0x5C,0x2F,0x00},{0x10,0x45,0x00},{0x05,0x4A,0x00},{0x00,0x47,0x2E},{0x00,0x41,0x66},{0x00,0x00,0x00},{0x05,0x05,0x05},{0x05,0x05,0x05},
		{0xC7,0xC7,0xC7},{0x00,0x77,0xFF},{0x21,0x55,0xFF},{0x82,0x37,0xFA},{0xEB,0x2F,0xB5},{0xFF,0x29,0x50},{0xFF,0x22,0x00},{0xD6,0x32,0x00},
		{0xC4,0x62,0x00},{0x35,0x80,0x00},{0x05,0x8F,0x00},{0x00,0x8A,0x55},{0x00,0x99,0xCC},{0x21,0x21,0x21},{0x09,0x09,0x09},{0x09,0x09,0x09},
		{0xFF,0xFF,0xFF},{0x0F,0xD7,0xFF},{0x69,0xA2,0xFF},{0xD4,0x80,0xFF},{0xFF,0x45,0xF3},{0xFF,0x61,0x8B},{0xFF,0x88,0x33},{0xFF,0x9C,0x12},
		{0xFA,0xBC,0x20},{0x9F,0xE3,0x0E},{0x2B,0xF0,0x35},{0x0C,0xF0,0xA4},{0x05,0xFB,0xFF},{0x5E,0x5E,0x5E},{0x0D,0x0D,0x0D},{0x0D,0x0D,0x0D},
		{0xFF,0xFF,0xFF},{0xA6,0xFC,0xFF},{0xB3,0xEC,0xFF},{0xDA,0xAB,0xEB},{0xFF,0xA8,0xF9},{0xFF,0xAB,0xB3},{0xFF,0xD2,0xB0},{0xFF,0xEF,0xA6},
		{0xFF,0xF7,0x9C},{0xD7,0xE8,0x95},{0xA6,0xED,0xAF},{0xA2,0xF2,0xDA},{0x99,0xFF,0xFC},{0xDD,0xDD,0xDD},{0x11,0x11,0x11},{0x11,0x11,0x11}
	},
	{	/* RGB (hardcoded) */
		{0x6D,0x6D,0x6D},{0x00,0x24,0x92},{0x00,0x00,0xDB},{0x6D,0x49,0xDB},{0x92,0x00,0x6D},{0xB6,0x00,0x6D},{0xB6,0x24,0x00},{0x92,0x49,0x00},
		{0x6D,0x49,0x00},{0x24,0x49,0x00},{0x00,0x6D,0x24},{0x00,0x92,0x00},{0x00,0x49,0x49},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0xB6,0xB6,0xB6},{0x00,0x6D,0xDB},{0x00,0x49,0xFF},{0x92,0x00,0xFF},{0xB6,0x00,0xFF},{0xFF,0x00,0x92},{0xFF,0x00,0x00},{0xDB,0x6D,0x00},
		{0x92,0x6D,0x00},{0x24,0x92,0x00},{0x00,0x92,0x00},{0x00,0xB6,0x6D},{0x00,0x92,0x92},{0x24,0x24,0x24},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0xFF,0xFF,0xFF},{0x6D,0xB6,0xFF},{0x92,0x92,0xFF},{0xDB,0x6D,0xFF},{0xFF,0x00,0xFF},{0xFF,0x6D,0xFF},{0xFF,0x92,0x00},{0xFF,0xB6,0x00},
		{0xDB,0xDB,0x00},{0x6D,0xDB,0x00},{0x00,0xFF,0x00},{0x49,0xFF,0xDB},{0x00,0xFF,0xFF},{0x49,0x49,0x49},{0x00,0x00,0x00},{0x00,0x00,0x00},
		{0xFF,0xFF,0xFF},{0xB6,0xDB,0xFF},{0xDB,0xB6,0xFF},{0xFF,0xB6,0xFF},{0xFF,0x92,0xFF},{0xFF,0xB6,0xB6},{0xFF,0xDB,0x92},{0xFF,0xFF,0x49},
		{0xFF,0xFF,0x6D},{0xB6,0xFF,0x49},{0x92,0xFF,0x6D},{0x49,0xFF,0xDB},{0x92,0xDB,0xFF},{0x92,0x92,0x92},{0x00,0x00,0x00},{0x00,0x00,0x00}
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
	},
	{	/* Temporary (used in config) */
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

void	GFX_GenerateNTSC (int hue, int sat, BOOL config)
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
			int p = config ? PALETTE_TEMP : PALETTE_NTSC;
			s = sat / 100.0;
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
			cPalette[p][(x << 4) | z][0] = ir;
			cPalette[p][(x << 4) | z][1] = ig;
			cPalette[p][(x << 4) | z][2] = ib;
		}
	}
}

BOOL	GFX_ImportPalette (TCHAR *filename, BOOL config)
{
	int p = config ? PALETTE_TEMP : PALETTE_EXT;
	int i;
	FILE *pal = _tfopen(filename,_T("rb"));
	if (!pal)
		return FALSE;
	fseek(pal,0xC0,SEEK_SET);
	if (ftell(pal) != 0xC0)
	{
		fclose(pal);
		return FALSE;
	}
	fseek(pal,0,SEEK_SET);
	for (i = 0; i < 64; i++)
	{
		fread(&cPalette[p][i][0],1,1,pal);
		fread(&cPalette[p][i][1],1,1,pal);
		fread(&cPalette[p][i][2],1,1,pal);
	}
	fclose(pal);
	return TRUE;
}

void	GFX_LoadPalette (int PalNum)
{
	const double EmphChanges[8][3] = {
		{1.000,1.000,1.000},	/* black */
		{1.239,0.915,0.743},	/* red */
		{0.794,1.086,0.882},	/* green */
		{1.019,0.980,0.653},	/* yellow */
		{0.905,1.026,1.277},	/* blue */
		{1.023,0.908,0.979},	/* magenta */
		{0.741,0.987,1.001},	/* cyan */
		{0.750,0.750,0.750}	/* white */
	};

	unsigned int RV, GV, BV;
	int i;
	if (PalNum == PALETTE_NTSC)
		GFX_GenerateNTSC(GFX.NTSChue,GFX.NTSCsat,FALSE);
	if (PalNum == PALETTE_CUST)
	{
		if (!GFX_ImportPalette(PPU.IsPAL ? GFX.CustPalettePAL : GFX.CustPaletteNTSC,FALSE))
		{
			MessageBox(mWnd,_T("Unable to load the specified palette! Reverting to default!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
			if (PPU.IsPAL)
				GFX.PalettePAL = PalNum = PALETTE_PAL;
			else	GFX.PaletteNTSC = PalNum = PALETTE_NTSC;
		}
	}
	for (i = 0; i < 0x200; i++)
	{
		if (PalNum == PALETTE_PC10)
		{
			RV = (i & 0x040) ? 0xFF : cPalette[PalNum][i & 0x3F][0];
			GV = (i & 0x080) ? 0xFF : cPalette[PalNum][i & 0x3F][1];
			BV = (i & 0x100) ? 0xFF : cPalette[PalNum][i & 0x3F][2];
		}
		else
		{
			RV = (unsigned int)((cPalette[PalNum][i & 0x3F][0] & 0xFF) * EmphChanges[i >> 6][0]);
			GV = (unsigned int)((cPalette[PalNum][i & 0x3F][1] & 0xFF) * EmphChanges[i >> 6][1]);
			BV = (unsigned int)((cPalette[PalNum][i & 0x3F][2] & 0xFF) * EmphChanges[i >> 6][2]);
		}
		if (RV > 0xFF)	RV = 0xFF;
		if (GV > 0xFF)	GV = 0xFF;
		if (BV > 0xFF)	BV = 0xFF;

		GFX.Palette15[i] = ((RV << 7) & 0x7C00) | ((GV << 2) & 0x03E0) | (BV >> 3);
		GFX.Palette16[i] = ((RV << 8) & 0xF800) | ((GV << 3) & 0x07E0) | (BV >> 3);
		GFX.Palette32[i] = (RV << 16) | (GV << 8) | BV;
	}
}

void	UpdatePalette (HWND hDlg, int pal)
{
	EnableWindow(GetDlgItem(hDlg,IDC_PAL_HUESLIDER),(pal == 0));
	EnableWindow(GetDlgItem(hDlg,IDC_PAL_HUE),(pal == 0));
	EnableWindow(GetDlgItem(hDlg,IDC_PAL_SATSLIDER),(pal == 0));
	EnableWindow(GetDlgItem(hDlg,IDC_PAL_SAT),(pal == 0));
	EnableWindow(GetDlgItem(hDlg,IDC_PAL_CUSTFILE),(pal == 3));
	EnableWindow(GetDlgItem(hDlg,IDC_PAL_BROWSE),(pal == 3));
	RedrawWindow(hDlg,NULL,NULL,RDW_INVALIDATE);
}

LRESULT	CALLBACK	PaletteConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const int paltable[4] = {IDC_PAL_NTSC,IDC_PAL_PAL,IDC_PAL_RGB,IDC_PAL_CUSTOM};
	const int PalEntries[64] = {
		IDC_PAL_00,IDC_PAL_01,IDC_PAL_02,IDC_PAL_03,IDC_PAL_04,IDC_PAL_05,IDC_PAL_06,IDC_PAL_07,IDC_PAL_08,IDC_PAL_09,IDC_PAL_0A,IDC_PAL_0B,IDC_PAL_0C,IDC_PAL_0D,IDC_PAL_0E,IDC_PAL_0F,
		IDC_PAL_10,IDC_PAL_11,IDC_PAL_12,IDC_PAL_13,IDC_PAL_14,IDC_PAL_15,IDC_PAL_16,IDC_PAL_17,IDC_PAL_18,IDC_PAL_19,IDC_PAL_1A,IDC_PAL_1B,IDC_PAL_1C,IDC_PAL_1D,IDC_PAL_1E,IDC_PAL_1F,
		IDC_PAL_20,IDC_PAL_21,IDC_PAL_22,IDC_PAL_23,IDC_PAL_24,IDC_PAL_25,IDC_PAL_26,IDC_PAL_27,IDC_PAL_28,IDC_PAL_29,IDC_PAL_2A,IDC_PAL_2B,IDC_PAL_2C,IDC_PAL_2D,IDC_PAL_2E,IDC_PAL_2F,
		IDC_PAL_30,IDC_PAL_31,IDC_PAL_32,IDC_PAL_33,IDC_PAL_34,IDC_PAL_35,IDC_PAL_36,IDC_PAL_37,IDC_PAL_38,IDC_PAL_39,IDC_PAL_3A,IDC_PAL_3B,IDC_PAL_3C,IDC_PAL_3D,IDC_PAL_3E,IDC_PAL_3F
	};

	int wmId, wmEvent;
	TCHAR filename[256];
	OPENFILENAME ofn;
	PAINTSTRUCT ps;
	HDC hdc;

	static BOOL ispal;
	static int hue, sat, pal, i;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		ispal = PPU.IsPAL;
		hue = GFX.NTSChue;
		sat = GFX.NTSCsat;
		pal = ispal ? GFX.PalettePAL : GFX.PaletteNTSC;
		if (pal == 0)
			GFX_GenerateNTSC(hue,sat,TRUE);
		else	memcpy(cPalette[PALETTE_TEMP],cPalette[pal],64*3);
		SendDlgItemMessage(hDlg,IDC_PAL_HUESLIDER,TBM_SETRANGE,FALSE,MAKELONG(300,360));
		SendDlgItemMessage(hDlg,IDC_PAL_HUESLIDER,TBM_SETTICFREQ,5,0);
		SendDlgItemMessage(hDlg,IDC_PAL_HUESLIDER,TBM_SETPOS,TRUE,hue);
		SetDlgItemInt(hDlg,IDC_PAL_HUE,hue,FALSE);
		SendDlgItemMessage(hDlg,IDC_PAL_SATSLIDER,TBM_SETRANGE,FALSE,MAKELONG(10,90));
		SendDlgItemMessage(hDlg,IDC_PAL_SATSLIDER,TBM_SETTICFREQ,5,0);
		SendDlgItemMessage(hDlg,IDC_PAL_SATSLIDER,TBM_SETPOS,TRUE,sat);
		SetDlgItemInt(hDlg,IDC_PAL_SAT,sat,FALSE);
		SetDlgItemText(hDlg,IDC_PAL_CUSTFILE,ispal ? GFX.CustPalettePAL : GFX.CustPaletteNTSC);
		CheckRadioButton(hDlg,IDC_PAL_NTSC,IDC_PAL_CUSTOM,paltable[pal]);
		UpdatePalette(hDlg,pal);
		return TRUE;
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		switch (wmId)
		{
		case IDC_PAL_NTSC:
			pal = PALETTE_NTSC;
			GFX_GenerateNTSC(hue,sat,TRUE);
			UpdatePalette(hDlg,pal);
			break;
		case IDC_PAL_PAL:
			pal = PALETTE_PAL;
			memcpy(cPalette[PALETTE_TEMP],cPalette[pal],64*3);
			UpdatePalette(hDlg,pal);
			break;
		case IDC_PAL_RGB:
			pal = PALETTE_PC10;
			memcpy(cPalette[PALETTE_TEMP],cPalette[pal],64*3);
			UpdatePalette(hDlg,pal);
			break;
		case IDC_PAL_CUSTOM:
			GetDlgItemText(hDlg,IDC_PAL_CUSTFILE,filename,256);
			pal = PALETTE_EXT;
			GFX_ImportPalette(filename,TRUE);
			UpdatePalette(hDlg,pal);
			break;
		case IDC_PAL_BROWSE:
			filename[0] = 0;
			ZeroMemory(&ofn,sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hDlg;
			ofn.hInstance = hInst;
			ofn.lpstrFilter = _T("Palette file (*.PAL)\0") _T("*.PAL\0") _T("\0");
			ofn.lpstrCustomFilter = NULL;
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = filename;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = Path_PAL;
			ofn.Flags = OFN_FILEMUSTEXIST;
			ofn.lpstrDefExt = NULL;
			ofn.lCustData = 0;
			ofn.lpfnHook = NULL;
			ofn.lpTemplateName = NULL;
			if (GetOpenFileName(&ofn))
			{
				_tcscpy(Path_PAL,filename);
				Path_PAL[ofn.nFileOffset-1] = 0;
				if (GFX_ImportPalette(filename,TRUE))
				{
					pal = 3;
					SetDlgItemText(hDlg,IDC_PAL_CUSTFILE,filename);
					UpdatePalette(hDlg,pal);
				}
				else	MessageBox(hDlg,_T("Selected file is not a valid palette!"),_T("Nintendulator"),MB_OK | MB_ICONERROR);
			}
			break;
		case IDOK:
			if (pal == 0)
			{
				GFX.NTSChue = hue;
				GFX.NTSCsat = sat;
			}
			if (pal == 3)
				GetDlgItemText(hDlg,IDC_PAL_CUSTFILE,ispal ? GFX.CustPalettePAL : GFX.CustPaletteNTSC,256);
			if (ispal)
				GFX.PalettePAL = pal;
			else	GFX.PaletteNTSC = pal;
			GFX_LoadPalette(pal);
		case IDCANCEL:
			EndDialog(hDlg,0);
			break;
		};
		break;
	case WM_HSCROLL:
		if (lParam == (LPARAM)GetDlgItem(hDlg,IDC_PAL_HUESLIDER))
		{
			hue = SendDlgItemMessage(hDlg,IDC_PAL_HUESLIDER,TBM_GETPOS,0,0);
			SetDlgItemInt(hDlg,IDC_PAL_HUE,hue,FALSE);
			GFX_GenerateNTSC(hue,sat,TRUE);
			UpdatePalette(hDlg,pal);
		}
		if (lParam == (LPARAM)GetDlgItem(hDlg,IDC_PAL_SATSLIDER))
		{
			sat = SendDlgItemMessage(hDlg,IDC_PAL_SATSLIDER,TBM_GETPOS,0,0);
			SetDlgItemInt(hDlg,IDC_PAL_SAT,sat,FALSE);
			GFX_GenerateNTSC(hue,sat,TRUE);
			UpdatePalette(hDlg,pal);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hDlg,&ps);
		{
			HDC compdc = CreateCompatibleDC(hdc);
			HBITMAP bmp;
			POINT wcl = {0,0};
			RECT wrect, rect;
			ClientToScreen(hDlg,&wcl);
			GetWindowRect(GetDlgItem(hDlg,PalEntries[0]),&rect);
			wrect.top = rect.top - wcl.y;
			wrect.left = rect.left - wcl.x;
			GetWindowRect(GetDlgItem(hDlg,PalEntries[63]),&rect);
			wrect.bottom = rect.bottom - wcl.y;
			wrect.right = rect.right - wcl.x;
			bmp = CreateCompatibleBitmap(hdc,wrect.right-wrect.left,wrect.bottom-wrect.top);
			SelectObject(compdc,bmp);
			for (i = 0; i < 64; i++)
			{
				HWND dlgitem = GetDlgItem(hDlg,PalEntries[i]);
				HBRUSH brush = CreateSolidBrush(cPalette[PALETTE_TEMP][i][0] | (cPalette[PALETTE_TEMP][i][1] << 8) | (cPalette[PALETTE_TEMP][i][2] << 16));
				GetWindowRect(dlgitem,&rect);
				rect.top -= wcl.y + wrect.top;
				rect.bottom -= wcl.y + wrect.top;
				rect.left -= wcl.x + wrect.left;
				rect.right -= wcl.x + wrect.left;
				FillRect(compdc,&rect,brush);
				DeleteObject(brush);
			}
			BitBlt(hdc,wrect.left,wrect.top,wrect.right-wrect.left,wrect.bottom-wrect.top,compdc,0,0,SRCCOPY);
			DeleteDC(compdc);
			DeleteObject(bmp);
		}
		EndPaint(hDlg,&ps);
		break;
	}

	return FALSE;
}
void	GFX_PaletteConfig (void)
{
	DialogBox(hInst,(LPCTSTR)IDD_PALETTE,mWnd,PaletteConfigProc);
}
