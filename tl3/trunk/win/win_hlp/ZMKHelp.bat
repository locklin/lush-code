@echo off

rem -- Edit the following line and indicate
rem    where to find the MSVC 5.x binaries
set msvc=c:\Progra~1\DevStudio\Vc\bin

REM -- Try a few well known locations
if exist %msvc%\hcrtf.exe goto found
set msvc=c:\DevStudio\Vc\bin
if exist %msvc%\hcrtf.exe goto found
echo You must edit the ZMKHelp.bat file and
echo specify where to find the MSVC5.x executables.
goto end
:found

if not exist ..\win_src\resource.h goto usehm
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // -- Used by WINTL3.HPJ. > "WinTL3.hm"
echo. >> "WinTL3.hm"
echo // Commands (ID_* and IDM_*) >> "WinTL3.hm"
%msvc%\makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 ..\win_src\resource.h >> "WinTL3.hm"
echo. >> "WinTL3.hm"
echo // Prompts (IDP_*) >> "WinTL3.hm"
%msvc%\makehm IDP_,HIDP_,0x30000 ..\win_src\resource.h >> "WinTL3.hm"
echo. >> "WinTL3.hm"
echo // Resources (IDR_*) >> "WinTL3.hm"
%msvc%\makehm IDR_,HIDR_,0x20000 ..\win_src\resource.h >> "WinTL3.hm"
echo. >> "WinTL3.hm"
echo // Dialogs (IDD_*) >> "WinTL3.hm"
%msvc%\makehm IDD_,HIDD_,0x20000 ..\win_src\resource.h >> "WinTL3.hm"
echo. >> "WinTL3.hm"
echo // Frame Controls (IDW_*) >> "WinTL3.hm"
%msvc%\makehm IDW_,HIDW_,0x50000 ..\win_src\resource.h >> "WinTL3.hm"
echo // Custom Entries (IDH_*) >> "WinTL3.hm"
%msvc%\makehm IDH_,HIDH_,0x00000 ..\win_src\resource.h >> "WinTL3.hm"

REM -- Make help for Project WinTL3
:usehm
echo Building Win32 Help files
start /wait %msvc%\hcrtf -x "WinTL3.hpj"
echo.

echo Moving help file to bin
if not exist ..\..\bin\wintl3.exe goto end
move wintl3.hlp ..\..\bin
:end
