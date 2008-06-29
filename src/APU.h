/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2008 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef APU_H
#define APU_H

#ifndef NSFPLAYER

#define DIRECTSOUND_VERSION 0x0700
#include <mmsystem.h>
#include <dsound.h>
#endif

struct	tAPU
{
	int			Cycles;
	int			BufPos;
#ifndef NSFPLAYER
	unsigned long		next_pos;
	BOOL			isEnabled;

	LPDIRECTSOUND		DirectSound;
	LPDIRECTSOUNDBUFFER	PrimaryBuffer;
	LPDIRECTSOUNDBUFFER	Buffer;

	short			*buffer;
	int			buflen;
	BYTE			Regs[0x18];
#endif /* NSFPLAYER */

	unsigned char		WantFPS;
	unsigned long		MHz;
	BOOL			PAL;
#ifndef NSFPLAYER
	unsigned long		LockSize;
#endif /* NSFPLAYER */
};
extern struct tAPU APU;

void	APU_Init		(void);
void	APU_Create		(void);
void	APU_Release		(void);
#ifndef NSFPLAYER
int	APU_Save		(FILE *);
int	APU_Load		(FILE *);
void	APU_SoundOFF		(void);
void	APU_SoundON		(void);
#endif /* NSFPLAYER */
void	APU_Reset		(void);
#ifndef NSFPLAYER
void	APU_Config		(HWND);
#endif /* NSFPLAYER */
void	APU_Run			(void);
void	APU_SetFPS		(int);
void	APU_WriteReg		(int,unsigned char);
unsigned char	APU_Read4015	(void);

#ifdef NSFPLAYER
extern	short	sample_pos;
extern	BOOL	sample_ok;
#endif /* NSFPLAYER */

#endif /* APU_H */