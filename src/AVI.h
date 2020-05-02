/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) QMT Productions
 */

#pragma once

namespace AVI
{
BOOL	IsActive	(void);
void	Init		(void);
void	Start		(void);
void	AddVideo	(void);
void	AddAudio	(void);
void	End		(void);
} // namespace AVI
