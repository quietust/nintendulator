/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2009 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef AVI_H
#define AVI_H

DECLARE_HANDLE(HAVI);
extern HAVI aviout;

void AVI_Start(void);
void AVI_AddVideo(void);
void AVI_AddAudio(void);
void AVI_End(void);

#endif	/* !AVI_H */
