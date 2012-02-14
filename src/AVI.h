/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * $URL$
 * $Id$
 */

#pragma once

DECLARE_HANDLE(HAVI);

namespace AVI
{
extern HAVI handle;

void	Init		(void);
void	Start		(void);
void	AddVideo	(void);
void	AddAudio	(void);
void	End		(void);
} // namespace AVI
