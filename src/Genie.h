/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2008 QMT Productions
 *
 * $URL$
 * $Id$
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

int	MAPINT	GenieRead (int,int);
int	MAPINT	GenieRead1 (int,int);
int	MAPINT	GenieRead2 (int,int);
int	MAPINT	GenieRead3 (int,int);

void	Genie_Reset (void);
void	Genie_Init (void);
int	Genie_Save (FILE *);
int	Genie_Load (FILE *);

#endif	/* !GENIE_H */
