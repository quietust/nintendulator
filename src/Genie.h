/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef GENIE_H
#define GENIE_H

namespace Genie
{
extern unsigned char CodeStat;

int	MAPINT	Read (int, int);
int	MAPINT	Read1 (int, int);
int	MAPINT	Read2 (int, int);
int	MAPINT	Read3 (int, int);

void	Reset (void);
void	Init (void);
int	Save (FILE *);
int	Load (FILE *);
} // namespace Genie
#endif	/* !GENIE_H */
