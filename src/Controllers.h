/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2009 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

#define	CONTROLLERS_MAXBUTTONS	32
#define	MAX_CONTROLLERS	32	// this includes keyboard and mouse

namespace Controllers
{

struct tStdPort
{
	unsigned char	(*Read)		(struct tStdPort *);
	void		(*Write)	(struct tStdPort *,unsigned char);
	void		(*Config)	(struct tStdPort *,HWND);
	void		(*Unload)	(struct tStdPort *);
	void		(*Frame)	(struct tStdPort *,unsigned char);
	int		Type;
	int		Buttons[CONTROLLERS_MAXBUTTONS];
	int		NumButtons;
	int		DataLen;
	unsigned long	*Data;
	int		MovLen;
	unsigned char	*MovData;
};
enum	STDCONT_TYPE	{ STD_UNCONNECTED, STD_STDCONTROLLER, STD_ZAPPER, STD_ARKANOIDPADDLE, STD_POWERPAD, STD_FOURSCORE, STD_SNESCONTROLLER, STD_VSZAPPER, STD_MAX };
void	StdPort_SetControllerType (struct tStdPort *,int);
extern const TCHAR	*StdPort_Mappings[STD_MAX];

struct tExpPort
{
	unsigned char	(*Read1)	(struct tExpPort *);
	unsigned char	(*Read2)	(struct tExpPort *);
	void		(*Write)	(struct tExpPort *,unsigned char);
	void		(*Config)	(struct tExpPort *,HWND);
	void		(*Unload)	(struct tExpPort *);
	void		(*Frame)	(struct tExpPort *,unsigned char);
	int		Type;
	int		Buttons[CONTROLLERS_MAXBUTTONS];
	int		NumButtons;
	int		DataLen;
	unsigned long	*Data;
	int		MovLen;
	unsigned char	*MovData;
};
enum	EXPCONT_TYPE	{ EXP_UNCONNECTED, EXP_FAMI4PLAY, EXP_ARKANOIDPADDLE, EXP_FAMILYBASICKEYBOARD, EXP_ALTKEYBOARD, EXP_FAMTRAINER, EXP_TABLET, EXP_MAX };
void	ExpPort_SetControllerType (struct tExpPort *,int);
extern const TCHAR	*ExpPort_Mappings[EXP_MAX];

enum	JOY_AXIS	{ AXIS_X, AXIS_Y, AXIS_Z, AXIS_RX, AXIS_RY, AXIS_RZ, AXIS_S0, AXIS_S1 };

extern struct tStdPort Port1, Port2;
extern struct tStdPort FSPort1, FSPort2, FSPort3, FSPort4;
extern struct tExpPort ExpPort;

extern BOOL	EnableOpposites;

extern BYTE		KeyState[256];
extern DIMOUSESTATE2	MouseState;

void	OpenConfig (void);
void	Init (void);
void	Release (void);
void	Write (unsigned char);
int	Save (FILE *);
int	Load (FILE *);
void	SetDeviceUsed (void);
void	Acquire (void);
void	UnAcquire (void);
void	UpdateInput (void);
void	ConfigButton (int *,int,HWND,BOOL);

BOOL	IsPressed (int);
void	ParseConfigMessages (HWND,int,int *,int *,int *,UINT,WPARAM,LPARAM);

} // namespace Controllers
#endif	/* !CONTROLLERS_H */
