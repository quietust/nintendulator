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

#ifndef GENIE_H
#define GENIE_H

struct tGenie
{
	int Code1B, Code2B, Code3B;
	int Code1A, Code2A, Code3A;
	int Code1O, Code2O, Code3O;
	int Code1V, Code2V, Code3V;
	unsigned char CodeStat;
};
extern	struct tGenie Genie;

void	Genie_Reset (void);
void	Genie_Init (void);
int	Genie_Save (FILE *);
int	Genie_Load (FILE *);

#endif /* GENIE_H */