@echo off
rem You may have to change the following lines:
set ishield=C:\Progra~1\InstallShield\InstallShield3\Program
set winsysdir=%windir%\system

rem CHECKING IS3 DIRECTORY
if exist %ishield%\setup.exe goto foundis3
echo You must edit the ZMKDist.bat file and
echo specify where to find the InstallShield3 executables.
goto end
:foundis3

rem CHECKING WINSYSDIR 
if exist %winsysdir%\ole32.dll goto foundws
echo You must edit the ZMKDist.bat file and
echo specify where to find the Windows system directory.
goto end
:foundws

REM CREATING DIRECTORIES
echo Creating Directories
if exist Disk1\disk1.id goto disk1
md Disk1
copy %ishield%\_inst32i.ex_ Disk1
copy %ishield%\_isdel.exe Disk1
copy %ishield%\_setup.dll Disk1
copy %ishield%\setup.exe Disk1
echo > Disk1\Disk1.id
:disk1
if exist Disk2\disk2.id goto disk2
md Disk2
echo > Disk2\Disk2.id
:disk2
if exist Shared\msvcrt.dll goto shared
md Shared
copy %winsysdir%\msvcrt.dll Shared
copy %winsysdir%\mfc42.dll Shared
:shared
if exist SetupLib\uninst.exe goto setuplib
md SetupLib
copy %ishield%\_isres.dll SetupLib
copy %ishield%\ctl3d32.dll SetupLib
copy %ishield%\uninst.exe SetupLib
:setuplib

REM PREPARING SETUP.LIB
if exist Disk1\_setup.lib del Disk1\_setup.lib
if exist ISUser\_isuser.dll copy ISUser\_isuser.dll setuplib
%ishield%\icomp SetupLib\*.* Disk1\_setup.lib

REM PREPARING COMPRESSED FILES
echo Compressing WinTL3 files...
if exist data.z del data.z
set tl3=..\..
set tl3ex=..\..\open\examples
%ishield%\icomp %tl3%\bin\             data.z wintl3\bin
%ishield%\icomp %tl3%\lib\*.*          data.z wintl3\lib
%ishield%\icomp %tl3%\help\*.*         data.z wintl3\help
%ishield%\icomp %tl3%\include\*.*      data.z wintl3\include
%ishield%\icomp %tl3%\open\*.*         data.z wintl3\open
%ishield%\icomp %tl3%\open\msvc\*.*    data.z wintl3\open\msvc
%ishield%\icomp %tl3ex%\*.*            data.z wintl3\open\examples
%ishield%\icomp %tl3ex%\capnth\*.*     data.z wintl3\open\examples\capnth
%ishield%\icomp %tl3ex%\complex\*.*    data.z wintl3\open\examples\complex
%ishield%\icomp %tl3ex%\nr\*.*         data.z wintl3\open\examples\nr
%ishield%\icomp %tl3ex%\nr\recipes\*.* data.z wintl3\open\examples\nr\recipes
%ishield%\icomp %tl3%\packages\*.*     data.z wintl3\packages
%ishield%\icomp shared\*.*             data.z shared 

rem These are the source files.
rem %ishield%\icomp %tl3%\*.*              data.z wintl3
rem %ishield%\icomp %tl3%\src\*.*          data.z wintl3\src
rem %ishield%\icomp %tl3%\config\*.*       data.z wintl3\config
rem %ishield%\icomp %tl3%\open\unix\*.*    data.z wintl3\open\unix

REM PACKAGING LIST
echo Building Packaging List...
%ishield%\packlist setup.lst
move >NUL /y setup.pkg Disk1

REM SPLITTING ARCHIVE
echo Splitting Archive...
%ishield%\split  data.z -F1400 -D1@600 -C
move >NUL /y data.1 Disk1
move >NUL /y data.2 Disk2
del data.z

REM SETUP CONFIG
if not exist setup.rul goto norul
:fast
echo Compiling setup script...
%ishield%\compile setup.rul
move >NUL /y setup.ins Disk1
:norul


REM FINISHED
echo ---------------------------
echo Done.
:end