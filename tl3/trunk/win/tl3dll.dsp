# Microsoft Developer Studio Project File - Name="tl3dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=tl3dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tl3dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tl3dll.mak" CFG="tl3dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tl3dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tl3dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tl3dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Ox /Ot /Og /Oi /Op /Ob2 /I "../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "TL3DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy ..\bin\tl3dll.lib ..\open\msvc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tl3dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "TL3DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "tl3dll - Win32 Release"
# Name "tl3dll - Win32 Debug"
# Begin Group "source files"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=..\Src\Allocate.c
# End Source File
# Begin Source File

SOURCE=..\Src\Array.c
# End Source File
# Begin Source File

SOURCE=..\Src\At.c
# End Source File
# Begin Source File

SOURCE=..\Src\Binary.c
# End Source File
# Begin Source File

SOURCE=..\Src\Calls.c
# End Source File
# Begin Source File

SOURCE=..\Src\Classify.c
# End Source File
# Begin Source File

SOURCE=..\Src\Date.c
# End Source File
# Begin Source File

SOURCE=..\Src\Dump.c
# End Source File
# Begin Source File

SOURCE=..\Src\Eval.c
# End Source File
# Begin Source File

SOURCE=..\Src\Fileio.c
# End Source File
# Begin Source File

SOURCE=..\Src\Fltlib.c
# End Source File
# Begin Source File

SOURCE=..\Src\Fmatops.c
# End Source File
# Begin Source File

SOURCE=..\Src\Function.c
# End Source File
# Begin Source File

SOURCE=..\Src\Graphics.c
# End Source File
# Begin Source File

SOURCE=..\Src\Htable.c
# End Source File
# Begin Source File

SOURCE=..\Src\Io.c
# End Source File
# Begin Source File

SOURCE=..\Src\Main.c
# End Source File
# Begin Source File

SOURCE=..\Src\Matrix.c
# End Source File
# Begin Source File

SOURCE=..\Src\Nan.c
# End Source File
# Begin Source File

SOURCE=..\Src\Oostruct.c
# End Source File
# Begin Source File

SOURCE=..\Src\ps_driver.c
# End Source File
# Begin Source File

SOURCE=..\Src\String.c
# End Source File
# Begin Source File

SOURCE=..\Src\Symbol.c
# End Source File
# Begin Source File

SOURCE=..\Src\Toplevel.c
# End Source File
# Begin Source File

SOURCE=..\Src\Win32.c
# End Source File
# Begin Source File

SOURCE=..\Src\Win_driver.c
# End Source File
# End Group
# Begin Group "header files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=..\Include\Define.h
# End Source File
# Begin Source File

SOURCE=..\Include\Fltlib.h
# End Source File
# Begin Source File

SOURCE=..\Include\Graphics.h
# End Source File
# Begin Source File

SOURCE=..\Include\Header.h
# End Source File
# Begin Source File

SOURCE=..\Include\Win32.h
# End Source File
# End Group
# End Target
# End Project
