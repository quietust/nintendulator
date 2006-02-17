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
	char		Filename[MAX_PATH];
	int		Len;
	int		Pos;
	int		FrameLen;
};
extern struct tMovie Movie;

void		Movie_ShowFrame	(void);
void		Movie_Play	(BOOL);
void		Movie_Record	(BOOL);
void		Movie_Stop	(void);
unsigned char	Movie_LoadInput	(void);
void		Movie_SaveInput	(unsigned char);
int		Movie_Save	(FILE *);
int		Movie_Load	(FILE *);

#endif /* MOVIE_H */