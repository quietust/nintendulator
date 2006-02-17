# Microsoft Developer Studio Project File - Name="Nintendulator" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Nintendulator - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Nintendulator.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Nintendulator.mak" CFG="Nintendulator - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Nintendulator - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Nintendulator - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Nintendulator", PBAAAAAA"
# PROP Scc_LocalPath ".."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Nintendulator - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".."
# PROP Intermediate_Dir "..\obj_emu"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FAcs /Fa"..\asm\\" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib dxguid.lib dinput.lib dsound.lib ddraw.lib winmm.lib vfw32.lib /nologo /subsystem:windows /pdb:none /machine:I386

!ELSEIF  "$(CFG)" == "Nintendulator - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".."
# PROP Intermediate_Dir "..\obj_emu"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /Gr /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FAcs /Fa"..\asm\\" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib dxguid.lib dinput.lib dsound.lib ddraw.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Nintendulator - Win32 Release"
# Name "Nintendulator - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\APU.c
# End Source File
# Begin Source File

SOURCE=..\src\AVI.c
# End Source File
# Begin Source File

SOURCE=..\src\c_e_altkey.c
# End Source File
# Begin Source File

SOURCE=..\src\c_e_arkpad.c
# End Source File
# Begin Source File

SOURCE=..\src\c_e_fam4play.c
# End Source File
# Begin Source File

SOURCE=..\src\c_e_famtrain.c
# End Source File
# Begin Source File

SOURCE=..\src\c_e_fbkey.c
# End Source File
# Begin Source File

SOURCE=..\src\c_e_unconnected.c
# End Source File
# Begin Source File

SOURCE=..\src\c_s_arkpad.c
# End Source File
# Begin Source File

SOURCE=..\src\c_s_fourscore.c
# End Source File
# Begin Source File

SOURCE=..\src\c_s_powpad.c
# End Source File
# Begin Source File

SOURCE=..\src\c_s_stdcont.c
# End Source File
# Begin Source File

SOURCE=..\src\c_s_unconnected.c
# End Source File
# Begin Source File

SOURCE=..\src\c_s_zapper.c
# End Source File
# Begin Source File

SOURCE=..\src\Controllers.c
# End Source File
# Begin Source File

SOURCE=..\src\CPU.c
# End Source File
# Begin Source File

SOURCE=..\src\Debugger.c
# End Source File
# Begin Source File

SOURCE=..\src\GFX.c
# End Source File
# Begin Source File

SOURCE=..\src\MapperInterface.c
# End Source File
# Begin Source File

SOURCE=..\src\NES.c
# End Source File
# Begin Source File

SOURCE=..\src\Nintendulator.c
# End Source File
# Begin Source File

SOURCE=..\src\PPU.c
# End Source File
# Begin Source File

SOURCE=..\src\States.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\APU.h
# End Source File
# Begin Source File

SOURCE=..\src\AVI.h
# End Source File
# Begin Source File

SOURCE=..\src\Controllers.h
# End Source File
# Begin Source File

SOURCE=..\src\CPU.h
# End Source File
# Begin Source File

SOURCE=..\src\Debugger.h
# End Source File
# Begin Source File

SOURCE=..\src\GFX.h
# End Source File
# Begin Source File

SOURCE=..\src\MapperInterface.h
# End Source File
# Begin Source File

SOURCE=..\src\NES.h
# End Source File
# Begin Source File

SOURCE=..\src\Nintendulator.h
# End Source File
# Begin Source File

SOURCE=..\src\PPU.h
# End Source File
# Begin Source File

SOURCE=..\src\resource.h
# End Source File
# Begin Source File

SOURCE=..\src\States.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\src\Nintendulator.ico
# End Source File
# Begin Source File

SOURCE=..\src\Nintendulator.rc
# End Source File
# Begin Source File

SOURCE=..\src\small.ico
# End Source File
# End Group
# End Target
# End Project
