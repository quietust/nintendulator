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

enum	STDCONT_TYPE
{
	STD_UNCONNECTED,
	STD_STDCONTROLLER,
	STD_ZAPPER,
	STD_ARKANOIDPADDLE,
	STD_POWERPAD,
	STD_FOURSCORE,
	STD_SNESCONTROLLER,
	STD_VSZAPPER,
	STD_FOURSCORE2,		// This must always be the 2nd-last entry
	STD_MAX
};
class StdPort
{
public:
	virtual			~StdPort(void) {} \
	virtual void 		Init	(int *) = 0;
	virtual unsigned char	Read	(void) = 0;
	virtual void		Write	(unsigned char) = 0;
	virtual void		Config	(HWND) = 0;
	virtual void		Frame	(unsigned char) = 0;
	STDCONT_TYPE	Type;
	int		*Buttons;
	int		NumButtons;
	int		DataLen;
	void		*Data;
	int		MovLen;
	unsigned char	*MovData;
};
#define DEF_STDCONT(NAME) \
class StdPort_##NAME : public StdPort \
{ \
public: \
	virtual void 		Init	(int *); \
	StdPort_##NAME (int *buttons) { Init(buttons); } \
	virtual			~StdPort_##NAME(void); \
	virtual unsigned char	Read	(void); \
	virtual void		Write	(unsigned char); \
	virtual void		Config	(HWND); \
	virtual void		Frame	(unsigned char); \
};
DEF_STDCONT(Unconnected)
DEF_STDCONT(StdController)
DEF_STDCONT(Zapper)
DEF_STDCONT(ArkanoidPaddle)
DEF_STDCONT(PowerPad)
DEF_STDCONT(FourScore)
DEF_STDCONT(SnesController)
DEF_STDCONT(VSZapper)
DEF_STDCONT(FourScore2)
#undef DEF_STDCONT

void	StdPort_SetControllerType (StdPort *&, STDCONT_TYPE, int *);
#define SET_STDCONT(PORT,TYPE) StdPort_SetControllerType(PORT, TYPE, PORT##_Buttons)
extern const TCHAR	*StdPort_Mappings[STD_MAX];

enum	EXPCONT_TYPE
{
	EXP_UNCONNECTED,
	EXP_FAMI4PLAY,
	EXP_ARKANOIDPADDLE,
	EXP_FAMILYBASICKEYBOARD,
	EXP_ALTKEYBOARD,
	EXP_FAMTRAINER,
	EXP_TABLET,
	EXP_MAX
};
class ExpPort
{
public:
	virtual			~ExpPort(void) {} \
	virtual void 		Init	(int *) = 0;
	virtual unsigned char	Read1	(void) = 0;
	virtual unsigned char	Read2	(void) = 0;
	virtual void		Write	(unsigned char) = 0;
	virtual void		Config	(HWND) = 0;
	virtual void		Frame	(unsigned char) = 0;
	EXPCONT_TYPE	Type;
	int		*Buttons;
	int		NumButtons;
	int		DataLen;
	void		*Data;
	int		MovLen;
	unsigned char	*MovData;
};
#define DEF_EXPCONT(NAME) \
class ExpPort_##NAME : public ExpPort \
{ \
public: \
	virtual void 		Init	(int *); \
	ExpPort_##NAME (int *buttons) { Init(buttons); } \
	virtual			~ExpPort_##NAME(void); \
	virtual unsigned char	Read1	(void); \
	virtual unsigned char	Read2	(void); \
	virtual void		Write	(unsigned char); \
	virtual void		Config	(HWND); \
	virtual void		Frame	(unsigned char); \
};
DEF_EXPCONT(Unconnected)
DEF_EXPCONT(Fami4Play)
DEF_EXPCONT(ArkanoidPaddle)
DEF_EXPCONT(FamilyBasicKeyboard)
DEF_EXPCONT(AltKeyboard)
DEF_EXPCONT(FamTrainer)
DEF_EXPCONT(Tablet)
#undef DEF_STDCONT

void	ExpPort_SetControllerType (ExpPort *&, EXPCONT_TYPE, int *);
#define SET_EXPCONT(PORT,TYPE) ExpPort_SetControllerType(PORT, TYPE, PORT##_Buttons)
extern const TCHAR	*ExpPort_Mappings[EXP_MAX];

enum	JOY_AXIS	{ AXIS_X, AXIS_Y, AXIS_Z, AXIS_RX, AXIS_RY, AXIS_RZ, AXIS_S0, AXIS_S1 };

extern StdPort *Port1, *Port2;
extern StdPort *FSPort1, *FSPort2, *FSPort3, *FSPort4;
extern ExpPort *PortExp;

extern int	Port1_Buttons[CONTROLLERS_MAXBUTTONS],
		Port2_Buttons[CONTROLLERS_MAXBUTTONS];
extern int	FSPort1_Buttons[CONTROLLERS_MAXBUTTONS],
		FSPort2_Buttons[CONTROLLERS_MAXBUTTONS],
		FSPort3_Buttons[CONTROLLERS_MAXBUTTONS],
		FSPort4_Buttons[CONTROLLERS_MAXBUTTONS];
extern int	PortExp_Buttons[CONTROLLERS_MAXBUTTONS];

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
