# Microsoft Developer Studio Project File - Name="wintl3" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=wintl3 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wintl3.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wintl3.mak" CFG="wintl3 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wintl3 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "wintl3 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "wintl3 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "wintl3__"
# PROP BASE Intermediate_Dir "wintl3__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "wintl3 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "wintl3_0"
# PROP BASE Intermediate_Dir "wintl3_0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "wintl3 - Win32 Release"
# Name "wintl3 - Win32 Debug"
# Begin Group "source files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\Win_src\ColorCombo.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\ConsFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\Console.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\FontCombo.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\Graphfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\GraphWin.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\propsheet.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\SnView.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\Splash.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\SplitFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\TextDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\TextView.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\TL3Com.cpp
# End Source File
# Begin Source File

SOURCE=.\Win_src\WinTL3.cpp
# End Source File
# End Group
# Begin Group "header files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\Win_src\ColorCombo.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\ConsFrm.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\Console.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\FontCombo.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\Graphfrm.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\GraphWin.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\propsheet.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\SnView.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\Splash.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\SplitFrm.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\TextDoc.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\TextView.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\Tl3Com.h
# End Source File
# Begin Source File

SOURCE=..\Include\Win32.h
# End Source File
# Begin Source File

SOURCE=.\Win_src\Wintl3.h
# End Source File
# End Group
# Begin Group "resource files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Win_src\Res\Consbar.bmp
# End Source File
# Begin Source File

SOURCE=.\Win_src\Res\Constype.ico
# End Source File
# Begin Source File

SOURCE=.\Win_src\Res\graphtype.ico
# End Source File
# Begin Source File

SOURCE=.\Win_src\Res\Sntype.ico
# End Source File
# Begin Source File

SOURCE=.\Win_src\Res\Splash.bmp
# End Source File
# Begin Source File

SOURCE=.\Win_src\Res\Splash16.bmp
# End Source File
# Begin Source File

SOURCE=.\Win_src\Res\Texttype.ico
# End Source File
# Begin Source File

SOURCE=.\Win_src\Res\TL3E.ico
# End Source File
# Begin Source File

SOURCE=.\Win_src\Res\TL3E.rc2
# End Source File
# Begin Source File

SOURCE=.\Win_src\Res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\Win_src\Res\TTimage.bmp
# End Source File
# Begin Source File

SOURCE=.\Win_src\Res\Ttmask.bmp
# End Source File
# Begin Source File

SOURCE=.\Win_src\WinTL3.rc
# End Source File
# End Group
# End Target
# End Project
