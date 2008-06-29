/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2008 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef STATES_H
#define STATES_H

#define	STATES_VERSION	"0960"
#define	STATES_BETA	"0955"
#define	STATES_PREV	"0950"

struct tStates
{
	TCHAR BaseFilename[MAX_PATH];
	int SelSlot;
};

extern	struct	tStates	States;

void	States_Init (void);
void	States_SetFilename (TCHAR *);
void	States_SetSlot (int Slot);
int	States_SaveData (FILE *);
BOOL	States_LoadData (FILE *, int);
void	States_SaveState (void);
void	States_LoadState (void);

#endif /* STATES_H */