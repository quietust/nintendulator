/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2008 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef MOVIE_H
#define MOVIE_H

#define	MOV_PLAY	0x01
#define	MOV_RECORD	0x02
#define	MOV_REVIEW	0x04

struct tMovie
{
	unsigned char	Mode;
	FILE *		Data;
	unsigned char	ControllerTypes[4];
	int		ReRecords;
	TCHAR		Filename[MAX_PATH];
	TCHAR *		Description;
	int		Len;
	int		Pos;
	int		FrameLen;
};
extern struct tMovie Movie;

void		Movie_ShowFrame	(void);
void		Movie_Play	(void);
void		Movie_Record	(void);
void		Movie_Stop	(void);
unsigned char	Movie_LoadInput	(void);
void		Movie_SaveInput	(unsigned char);
int		Movie_Save	(FILE *);
int		Movie_Load	(FILE *);

#endif	/* !MOVIE_H */
