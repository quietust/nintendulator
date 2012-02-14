/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * $URL$
 * $Id$
 */

#pragma once

#ifndef	NSFPLAYER
#include <mmsystem.h>
#define DIRECTSOUND_VERSION 0x0800
#include <dsound.h>
#endif	/* !NSFPLAYER */

namespace APU
{
extern short	*buffer;

#ifdef	NSFPLAYER
extern	short	sample_pos;
extern	BOOL	sample_ok;
#endif	/* NSFPLAYER */

namespace DPCM
{
	void	Fetch (void);
}

void	Init		(void);
void	Create		(void);
void	Release		(void);
#ifndef	NSFPLAYER
int	Save		(FILE *);
int	Load		(FILE *);
void	SoundOFF	(void);
void	SoundON		(void);
#endif	/* !NSFPLAYER */
void	PowerOn		(void);
void	Reset		(void);
#ifndef	NSFPLAYER
void	Config		(HWND);
#endif	/* !NSFPLAYER */
void	Run		(void);
void	SetRegion	(void);
int	MAPINT	IntRead (int, int);
void	MAPINT	IntWrite (int, int, int);
} // namespace APU
