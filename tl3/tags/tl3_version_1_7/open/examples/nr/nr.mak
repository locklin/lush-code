# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=nr - Win32 Release
!MESSAGE No configuration specified.  Defaulting to nr - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "nr - Win32 Release" && "$(CFG)" != "nr - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "nr.mak" CFG="nr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nr - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "nr - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "nr - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "nr - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\nr.dll"

CLEAN : 
	-@erase "$(INTDIR)\amoeba.obj"
	-@erase "$(INTDIR)\anneal.obj"
	-@erase "$(INTDIR)\avevar.obj"
	-@erase "$(INTDIR)\balanc.obj"
	-@erase "$(INTDIR)\bcucof.obj"
	-@erase "$(INTDIR)\bcuint.obj"
	-@erase "$(INTDIR)\bessi.obj"
	-@erase "$(INTDIR)\bessi0.obj"
	-@erase "$(INTDIR)\bessi1.obj"
	-@erase "$(INTDIR)\bessj.obj"
	-@erase "$(INTDIR)\bessj0.obj"
	-@erase "$(INTDIR)\bessj1.obj"
	-@erase "$(INTDIR)\bessk.obj"
	-@erase "$(INTDIR)\bessk0.obj"
	-@erase "$(INTDIR)\bessk1.obj"
	-@erase "$(INTDIR)\bessy.obj"
	-@erase "$(INTDIR)\bessy0.obj"
	-@erase "$(INTDIR)\bessy1.obj"
	-@erase "$(INTDIR)\beta.obj"
	-@erase "$(INTDIR)\betacf.obj"
	-@erase "$(INTDIR)\betai.obj"
	-@erase "$(INTDIR)\bico.obj"
	-@erase "$(INTDIR)\bksub.obj"
	-@erase "$(INTDIR)\bnldev.obj"
	-@erase "$(INTDIR)\brent.obj"
	-@erase "$(INTDIR)\bsstep.obj"
	-@erase "$(INTDIR)\caldat.obj"
	-@erase "$(INTDIR)\cel.obj"
	-@erase "$(INTDIR)\chder.obj"
	-@erase "$(INTDIR)\chebev.obj"
	-@erase "$(INTDIR)\chebft.obj"
	-@erase "$(INTDIR)\chebpc.obj"
	-@erase "$(INTDIR)\chint.obj"
	-@erase "$(INTDIR)\chsone.obj"
	-@erase "$(INTDIR)\chstwo.obj"
	-@erase "$(INTDIR)\cntab1.obj"
	-@erase "$(INTDIR)\cntab2.obj"
	-@erase "$(INTDIR)\convlv.obj"
	-@erase "$(INTDIR)\correl.obj"
	-@erase "$(INTDIR)\covsrt.obj"
	-@erase "$(INTDIR)\crank.obj"
	-@erase "$(INTDIR)\dbrent.obj"
	-@erase "$(INTDIR)\ddpoly.obj"
	-@erase "$(INTDIR)\des.obj"
	-@erase "$(INTDIR)\desks.obj"
	-@erase "$(INTDIR)\dfpmin.obj"
	-@erase "$(INTDIR)\eclass.obj"
	-@erase "$(INTDIR)\eclazz.obj"
	-@erase "$(INTDIR)\eigsrt.obj"
	-@erase "$(INTDIR)\el2.obj"
	-@erase "$(INTDIR)\elmhes.obj"
	-@erase "$(INTDIR)\erf.obj"
	-@erase "$(INTDIR)\erfc.obj"
	-@erase "$(INTDIR)\erfcc.obj"
	-@erase "$(INTDIR)\eulsum.obj"
	-@erase "$(INTDIR)\evlmem.obj"
	-@erase "$(INTDIR)\expdev.obj"
	-@erase "$(INTDIR)\f1dim.obj"
	-@erase "$(INTDIR)\factln.obj"
	-@erase "$(INTDIR)\factrl.obj"
	-@erase "$(INTDIR)\fgauss.obj"
	-@erase "$(INTDIR)\fit.obj"
	-@erase "$(INTDIR)\fleg.obj"
	-@erase "$(INTDIR)\flmoon.obj"
	-@erase "$(INTDIR)\four1.obj"
	-@erase "$(INTDIR)\fourn.obj"
	-@erase "$(INTDIR)\fpoly.obj"
	-@erase "$(INTDIR)\frprmn.obj"
	-@erase "$(INTDIR)\ftest.obj"
	-@erase "$(INTDIR)\gamdev.obj"
	-@erase "$(INTDIR)\gammln.obj"
	-@erase "$(INTDIR)\gammp.obj"
	-@erase "$(INTDIR)\gammq.obj"
	-@erase "$(INTDIR)\gasdev.obj"
	-@erase "$(INTDIR)\gauleg.obj"
	-@erase "$(INTDIR)\gaussj.obj"
	-@erase "$(INTDIR)\gcf.obj"
	-@erase "$(INTDIR)\golden.obj"
	-@erase "$(INTDIR)\gser.obj"
	-@erase "$(INTDIR)\hqr.obj"
	-@erase "$(INTDIR)\hunt.obj"
	-@erase "$(INTDIR)\indexx.obj"
	-@erase "$(INTDIR)\irbit1.obj"
	-@erase "$(INTDIR)\irbit2.obj"
	-@erase "$(INTDIR)\jacobi.obj"
	-@erase "$(INTDIR)\julday.obj"
	-@erase "$(INTDIR)\kendl1.obj"
	-@erase "$(INTDIR)\kendl2.obj"
	-@erase "$(INTDIR)\ksone.obj"
	-@erase "$(INTDIR)\kstwo.obj"
	-@erase "$(INTDIR)\lfit.obj"
	-@erase "$(INTDIR)\linmin.obj"
	-@erase "$(INTDIR)\locate.obj"
	-@erase "$(INTDIR)\lubksb.obj"
	-@erase "$(INTDIR)\ludcmp.obj"
	-@erase "$(INTDIR)\mdian1.obj"
	-@erase "$(INTDIR)\mdian2.obj"
	-@erase "$(INTDIR)\medfit.obj"
	-@erase "$(INTDIR)\memcof.obj"
	-@erase "$(INTDIR)\midexp.obj"
	-@erase "$(INTDIR)\midinf.obj"
	-@erase "$(INTDIR)\midpnt.obj"
	-@erase "$(INTDIR)\midsql.obj"
	-@erase "$(INTDIR)\midsqu.obj"
	-@erase "$(INTDIR)\mmid.obj"
	-@erase "$(INTDIR)\mnbrak.obj"
	-@erase "$(INTDIR)\moment.obj"
	-@erase "$(INTDIR)\mprove.obj"
	-@erase "$(INTDIR)\mrqcof.obj"
	-@erase "$(INTDIR)\mrqmin.obj"
	-@erase "$(INTDIR)\odeint.obj"
	-@erase "$(INTDIR)\pcshft.obj"
	-@erase "$(INTDIR)\pearsn.obj"
	-@erase "$(INTDIR)\piksr2.obj"
	-@erase "$(INTDIR)\piksrt.obj"
	-@erase "$(INTDIR)\pinvs.obj"
	-@erase "$(INTDIR)\plgndr.obj"
	-@erase "$(INTDIR)\poidev.obj"
	-@erase "$(INTDIR)\polcoe.obj"
	-@erase "$(INTDIR)\polcof.obj"
	-@erase "$(INTDIR)\poldiv.obj"
	-@erase "$(INTDIR)\polin2.obj"
	-@erase "$(INTDIR)\polint.obj"
	-@erase "$(INTDIR)\powell.obj"
	-@erase "$(INTDIR)\predic.obj"
	-@erase "$(INTDIR)\probks.obj"
	-@erase "$(INTDIR)\pzextr.obj"
	-@erase "$(INTDIR)\qcksrt.obj"
	-@erase "$(INTDIR)\qgaus.obj"
	-@erase "$(INTDIR)\qromb.obj"
	-@erase "$(INTDIR)\qromo.obj"
	-@erase "$(INTDIR)\qroot.obj"
	-@erase "$(INTDIR)\qsimp.obj"
	-@erase "$(INTDIR)\qtrap.obj"
	-@erase "$(INTDIR)\ran0.obj"
	-@erase "$(INTDIR)\ran1.obj"
	-@erase "$(INTDIR)\ran2.obj"
	-@erase "$(INTDIR)\ran3.obj"
	-@erase "$(INTDIR)\ran4.obj"
	-@erase "$(INTDIR)\rank.obj"
	-@erase "$(INTDIR)\ratint.obj"
	-@erase "$(INTDIR)\realft.obj"
	-@erase "$(INTDIR)\red.obj"
	-@erase "$(INTDIR)\rk4.obj"
	-@erase "$(INTDIR)\rkdumb.obj"
	-@erase "$(INTDIR)\rkqc.obj"
	-@erase "$(INTDIR)\rofunc.obj"
	-@erase "$(INTDIR)\rtbis.obj"
	-@erase "$(INTDIR)\rtflsp.obj"
	-@erase "$(INTDIR)\rtnewt.obj"
	-@erase "$(INTDIR)\rtsafe.obj"
	-@erase "$(INTDIR)\rtsec.obj"
	-@erase "$(INTDIR)\rzextr.obj"
	-@erase "$(INTDIR)\scrsho.obj"
	-@erase "$(INTDIR)\shell.obj"
	-@erase "$(INTDIR)\simp1.obj"
	-@erase "$(INTDIR)\simp2.obj"
	-@erase "$(INTDIR)\simp3.obj"
	-@erase "$(INTDIR)\simplx.obj"
	-@erase "$(INTDIR)\sinft.obj"
	-@erase "$(INTDIR)\smooft.obj"
	-@erase "$(INTDIR)\sncndn.obj"
	-@erase "$(INTDIR)\sor.obj"
	-@erase "$(INTDIR)\sort.obj"
	-@erase "$(INTDIR)\sort2.obj"
	-@erase "$(INTDIR)\sort3.obj"
	-@erase "$(INTDIR)\sparse.obj"
	-@erase "$(INTDIR)\spctrm.obj"
	-@erase "$(INTDIR)\spear.obj"
	-@erase "$(INTDIR)\splie2.obj"
	-@erase "$(INTDIR)\splin2.obj"
	-@erase "$(INTDIR)\spline.obj"
	-@erase "$(INTDIR)\splint.obj"
	-@erase "$(INTDIR)\svbksb.obj"
	-@erase "$(INTDIR)\svdcmp.obj"
	-@erase "$(INTDIR)\svdfit.obj"
	-@erase "$(INTDIR)\svdvar.obj"
	-@erase "$(INTDIR)\toeplz.obj"
	-@erase "$(INTDIR)\tptest.obj"
	-@erase "$(INTDIR)\tqli.obj"
	-@erase "$(INTDIR)\trapzd.obj"
	-@erase "$(INTDIR)\tred2.obj"
	-@erase "$(INTDIR)\tridag.obj"
	-@erase "$(INTDIR)\ttest.obj"
	-@erase "$(INTDIR)\tutest.obj"
	-@erase "$(INTDIR)\twofft.obj"
	-@erase "$(INTDIR)\user_dll.obj"
	-@erase "$(INTDIR)\vander.obj"
	-@erase "$(INTDIR)\zbrac.obj"
	-@erase "$(INTDIR)\zbrak.obj"
	-@erase "$(INTDIR)\zbrent.obj"
	-@erase "$(OUTDIR)\nr.dll"
	-@erase "$(OUTDIR)\nr.exp"
	-@erase "$(OUTDIR)\nr.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /D "_MBCS" /Fp"$(INTDIR)/nr.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/nr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/nr.pdb" /machine:I386 /out:"$(OUTDIR)/nr.dll"\
 /implib:"$(OUTDIR)/nr.lib" 
LINK32_OBJS= \
	"$(INTDIR)\amoeba.obj" \
	"$(INTDIR)\anneal.obj" \
	"$(INTDIR)\avevar.obj" \
	"$(INTDIR)\balanc.obj" \
	"$(INTDIR)\bcucof.obj" \
	"$(INTDIR)\bcuint.obj" \
	"$(INTDIR)\bessi.obj" \
	"$(INTDIR)\bessi0.obj" \
	"$(INTDIR)\bessi1.obj" \
	"$(INTDIR)\bessj.obj" \
	"$(INTDIR)\bessj0.obj" \
	"$(INTDIR)\bessj1.obj" \
	"$(INTDIR)\bessk.obj" \
	"$(INTDIR)\bessk0.obj" \
	"$(INTDIR)\bessk1.obj" \
	"$(INTDIR)\bessy.obj" \
	"$(INTDIR)\bessy0.obj" \
	"$(INTDIR)\bessy1.obj" \
	"$(INTDIR)\beta.obj" \
	"$(INTDIR)\betacf.obj" \
	"$(INTDIR)\betai.obj" \
	"$(INTDIR)\bico.obj" \
	"$(INTDIR)\bksub.obj" \
	"$(INTDIR)\bnldev.obj" \
	"$(INTDIR)\brent.obj" \
	"$(INTDIR)\bsstep.obj" \
	"$(INTDIR)\caldat.obj" \
	"$(INTDIR)\cel.obj" \
	"$(INTDIR)\chder.obj" \
	"$(INTDIR)\chebev.obj" \
	"$(INTDIR)\chebft.obj" \
	"$(INTDIR)\chebpc.obj" \
	"$(INTDIR)\chint.obj" \
	"$(INTDIR)\chsone.obj" \
	"$(INTDIR)\chstwo.obj" \
	"$(INTDIR)\cntab1.obj" \
	"$(INTDIR)\cntab2.obj" \
	"$(INTDIR)\convlv.obj" \
	"$(INTDIR)\correl.obj" \
	"$(INTDIR)\covsrt.obj" \
	"$(INTDIR)\crank.obj" \
	"$(INTDIR)\dbrent.obj" \
	"$(INTDIR)\ddpoly.obj" \
	"$(INTDIR)\des.obj" \
	"$(INTDIR)\desks.obj" \
	"$(INTDIR)\dfpmin.obj" \
	"$(INTDIR)\eclass.obj" \
	"$(INTDIR)\eclazz.obj" \
	"$(INTDIR)\eigsrt.obj" \
	"$(INTDIR)\el2.obj" \
	"$(INTDIR)\elmhes.obj" \
	"$(INTDIR)\erf.obj" \
	"$(INTDIR)\erfc.obj" \
	"$(INTDIR)\erfcc.obj" \
	"$(INTDIR)\eulsum.obj" \
	"$(INTDIR)\evlmem.obj" \
	"$(INTDIR)\expdev.obj" \
	"$(INTDIR)\f1dim.obj" \
	"$(INTDIR)\factln.obj" \
	"$(INTDIR)\factrl.obj" \
	"$(INTDIR)\fgauss.obj" \
	"$(INTDIR)\fit.obj" \
	"$(INTDIR)\fleg.obj" \
	"$(INTDIR)\flmoon.obj" \
	"$(INTDIR)\four1.obj" \
	"$(INTDIR)\fourn.obj" \
	"$(INTDIR)\fpoly.obj" \
	"$(INTDIR)\frprmn.obj" \
	"$(INTDIR)\ftest.obj" \
	"$(INTDIR)\gamdev.obj" \
	"$(INTDIR)\gammln.obj" \
	"$(INTDIR)\gammp.obj" \
	"$(INTDIR)\gammq.obj" \
	"$(INTDIR)\gasdev.obj" \
	"$(INTDIR)\gauleg.obj" \
	"$(INTDIR)\gaussj.obj" \
	"$(INTDIR)\gcf.obj" \
	"$(INTDIR)\golden.obj" \
	"$(INTDIR)\gser.obj" \
	"$(INTDIR)\hqr.obj" \
	"$(INTDIR)\hunt.obj" \
	"$(INTDIR)\indexx.obj" \
	"$(INTDIR)\irbit1.obj" \
	"$(INTDIR)\irbit2.obj" \
	"$(INTDIR)\jacobi.obj" \
	"$(INTDIR)\julday.obj" \
	"$(INTDIR)\kendl1.obj" \
	"$(INTDIR)\kendl2.obj" \
	"$(INTDIR)\ksone.obj" \
	"$(INTDIR)\kstwo.obj" \
	"$(INTDIR)\lfit.obj" \
	"$(INTDIR)\linmin.obj" \
	"$(INTDIR)\locate.obj" \
	"$(INTDIR)\lubksb.obj" \
	"$(INTDIR)\ludcmp.obj" \
	"$(INTDIR)\mdian1.obj" \
	"$(INTDIR)\mdian2.obj" \
	"$(INTDIR)\medfit.obj" \
	"$(INTDIR)\memcof.obj" \
	"$(INTDIR)\midexp.obj" \
	"$(INTDIR)\midinf.obj" \
	"$(INTDIR)\midpnt.obj" \
	"$(INTDIR)\midsql.obj" \
	"$(INTDIR)\midsqu.obj" \
	"$(INTDIR)\mmid.obj" \
	"$(INTDIR)\mnbrak.obj" \
	"$(INTDIR)\moment.obj" \
	"$(INTDIR)\mprove.obj" \
	"$(INTDIR)\mrqcof.obj" \
	"$(INTDIR)\mrqmin.obj" \
	"$(INTDIR)\odeint.obj" \
	"$(INTDIR)\pcshft.obj" \
	"$(INTDIR)\pearsn.obj" \
	"$(INTDIR)\piksr2.obj" \
	"$(INTDIR)\piksrt.obj" \
	"$(INTDIR)\pinvs.obj" \
	"$(INTDIR)\plgndr.obj" \
	"$(INTDIR)\poidev.obj" \
	"$(INTDIR)\polcoe.obj" \
	"$(INTDIR)\polcof.obj" \
	"$(INTDIR)\poldiv.obj" \
	"$(INTDIR)\polin2.obj" \
	"$(INTDIR)\polint.obj" \
	"$(INTDIR)\powell.obj" \
	"$(INTDIR)\predic.obj" \
	"$(INTDIR)\probks.obj" \
	"$(INTDIR)\pzextr.obj" \
	"$(INTDIR)\qcksrt.obj" \
	"$(INTDIR)\qgaus.obj" \
	"$(INTDIR)\qromb.obj" \
	"$(INTDIR)\qromo.obj" \
	"$(INTDIR)\qroot.obj" \
	"$(INTDIR)\qsimp.obj" \
	"$(INTDIR)\qtrap.obj" \
	"$(INTDIR)\ran0.obj" \
	"$(INTDIR)\ran1.obj" \
	"$(INTDIR)\ran2.obj" \
	"$(INTDIR)\ran3.obj" \
	"$(INTDIR)\ran4.obj" \
	"$(INTDIR)\rank.obj" \
	"$(INTDIR)\ratint.obj" \
	"$(INTDIR)\realft.obj" \
	"$(INTDIR)\red.obj" \
	"$(INTDIR)\rk4.obj" \
	"$(INTDIR)\rkdumb.obj" \
	"$(INTDIR)\rkqc.obj" \
	"$(INTDIR)\rofunc.obj" \
	"$(INTDIR)\rtbis.obj" \
	"$(INTDIR)\rtflsp.obj" \
	"$(INTDIR)\rtnewt.obj" \
	"$(INTDIR)\rtsafe.obj" \
	"$(INTDIR)\rtsec.obj" \
	"$(INTDIR)\rzextr.obj" \
	"$(INTDIR)\scrsho.obj" \
	"$(INTDIR)\shell.obj" \
	"$(INTDIR)\simp1.obj" \
	"$(INTDIR)\simp2.obj" \
	"$(INTDIR)\simp3.obj" \
	"$(INTDIR)\simplx.obj" \
	"$(INTDIR)\sinft.obj" \
	"$(INTDIR)\smooft.obj" \
	"$(INTDIR)\sncndn.obj" \
	"$(INTDIR)\sor.obj" \
	"$(INTDIR)\sort.obj" \
	"$(INTDIR)\sort2.obj" \
	"$(INTDIR)\sort3.obj" \
	"$(INTDIR)\sparse.obj" \
	"$(INTDIR)\spctrm.obj" \
	"$(INTDIR)\spear.obj" \
	"$(INTDIR)\splie2.obj" \
	"$(INTDIR)\splin2.obj" \
	"$(INTDIR)\spline.obj" \
	"$(INTDIR)\splint.obj" \
	"$(INTDIR)\svbksb.obj" \
	"$(INTDIR)\svdcmp.obj" \
	"$(INTDIR)\svdfit.obj" \
	"$(INTDIR)\svdvar.obj" \
	"$(INTDIR)\toeplz.obj" \
	"$(INTDIR)\tptest.obj" \
	"$(INTDIR)\tqli.obj" \
	"$(INTDIR)\trapzd.obj" \
	"$(INTDIR)\tred2.obj" \
	"$(INTDIR)\tridag.obj" \
	"$(INTDIR)\ttest.obj" \
	"$(INTDIR)\tutest.obj" \
	"$(INTDIR)\twofft.obj" \
	"$(INTDIR)\user_dll.obj" \
	"$(INTDIR)\vander.obj" \
	"$(INTDIR)\zbrac.obj" \
	"$(INTDIR)\zbrak.obj" \
	"$(INTDIR)\zbrent.obj"

"$(OUTDIR)\nr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "nr - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\nr.dll"

CLEAN : 
	-@erase "$(INTDIR)\amoeba.obj"
	-@erase "$(INTDIR)\anneal.obj"
	-@erase "$(INTDIR)\avevar.obj"
	-@erase "$(INTDIR)\balanc.obj"
	-@erase "$(INTDIR)\bcucof.obj"
	-@erase "$(INTDIR)\bcuint.obj"
	-@erase "$(INTDIR)\bessi.obj"
	-@erase "$(INTDIR)\bessi0.obj"
	-@erase "$(INTDIR)\bessi1.obj"
	-@erase "$(INTDIR)\bessj.obj"
	-@erase "$(INTDIR)\bessj0.obj"
	-@erase "$(INTDIR)\bessj1.obj"
	-@erase "$(INTDIR)\bessk.obj"
	-@erase "$(INTDIR)\bessk0.obj"
	-@erase "$(INTDIR)\bessk1.obj"
	-@erase "$(INTDIR)\bessy.obj"
	-@erase "$(INTDIR)\bessy0.obj"
	-@erase "$(INTDIR)\bessy1.obj"
	-@erase "$(INTDIR)\beta.obj"
	-@erase "$(INTDIR)\betacf.obj"
	-@erase "$(INTDIR)\betai.obj"
	-@erase "$(INTDIR)\bico.obj"
	-@erase "$(INTDIR)\bksub.obj"
	-@erase "$(INTDIR)\bnldev.obj"
	-@erase "$(INTDIR)\brent.obj"
	-@erase "$(INTDIR)\bsstep.obj"
	-@erase "$(INTDIR)\caldat.obj"
	-@erase "$(INTDIR)\cel.obj"
	-@erase "$(INTDIR)\chder.obj"
	-@erase "$(INTDIR)\chebev.obj"
	-@erase "$(INTDIR)\chebft.obj"
	-@erase "$(INTDIR)\chebpc.obj"
	-@erase "$(INTDIR)\chint.obj"
	-@erase "$(INTDIR)\chsone.obj"
	-@erase "$(INTDIR)\chstwo.obj"
	-@erase "$(INTDIR)\cntab1.obj"
	-@erase "$(INTDIR)\cntab2.obj"
	-@erase "$(INTDIR)\convlv.obj"
	-@erase "$(INTDIR)\correl.obj"
	-@erase "$(INTDIR)\covsrt.obj"
	-@erase "$(INTDIR)\crank.obj"
	-@erase "$(INTDIR)\dbrent.obj"
	-@erase "$(INTDIR)\ddpoly.obj"
	-@erase "$(INTDIR)\des.obj"
	-@erase "$(INTDIR)\desks.obj"
	-@erase "$(INTDIR)\dfpmin.obj"
	-@erase "$(INTDIR)\eclass.obj"
	-@erase "$(INTDIR)\eclazz.obj"
	-@erase "$(INTDIR)\eigsrt.obj"
	-@erase "$(INTDIR)\el2.obj"
	-@erase "$(INTDIR)\elmhes.obj"
	-@erase "$(INTDIR)\erf.obj"
	-@erase "$(INTDIR)\erfc.obj"
	-@erase "$(INTDIR)\erfcc.obj"
	-@erase "$(INTDIR)\eulsum.obj"
	-@erase "$(INTDIR)\evlmem.obj"
	-@erase "$(INTDIR)\expdev.obj"
	-@erase "$(INTDIR)\f1dim.obj"
	-@erase "$(INTDIR)\factln.obj"
	-@erase "$(INTDIR)\factrl.obj"
	-@erase "$(INTDIR)\fgauss.obj"
	-@erase "$(INTDIR)\fit.obj"
	-@erase "$(INTDIR)\fleg.obj"
	-@erase "$(INTDIR)\flmoon.obj"
	-@erase "$(INTDIR)\four1.obj"
	-@erase "$(INTDIR)\fourn.obj"
	-@erase "$(INTDIR)\fpoly.obj"
	-@erase "$(INTDIR)\frprmn.obj"
	-@erase "$(INTDIR)\ftest.obj"
	-@erase "$(INTDIR)\gamdev.obj"
	-@erase "$(INTDIR)\gammln.obj"
	-@erase "$(INTDIR)\gammp.obj"
	-@erase "$(INTDIR)\gammq.obj"
	-@erase "$(INTDIR)\gasdev.obj"
	-@erase "$(INTDIR)\gauleg.obj"
	-@erase "$(INTDIR)\gaussj.obj"
	-@erase "$(INTDIR)\gcf.obj"
	-@erase "$(INTDIR)\golden.obj"
	-@erase "$(INTDIR)\gser.obj"
	-@erase "$(INTDIR)\hqr.obj"
	-@erase "$(INTDIR)\hunt.obj"
	-@erase "$(INTDIR)\indexx.obj"
	-@erase "$(INTDIR)\irbit1.obj"
	-@erase "$(INTDIR)\irbit2.obj"
	-@erase "$(INTDIR)\jacobi.obj"
	-@erase "$(INTDIR)\julday.obj"
	-@erase "$(INTDIR)\kendl1.obj"
	-@erase "$(INTDIR)\kendl2.obj"
	-@erase "$(INTDIR)\ksone.obj"
	-@erase "$(INTDIR)\kstwo.obj"
	-@erase "$(INTDIR)\lfit.obj"
	-@erase "$(INTDIR)\linmin.obj"
	-@erase "$(INTDIR)\locate.obj"
	-@erase "$(INTDIR)\lubksb.obj"
	-@erase "$(INTDIR)\ludcmp.obj"
	-@erase "$(INTDIR)\mdian1.obj"
	-@erase "$(INTDIR)\mdian2.obj"
	-@erase "$(INTDIR)\medfit.obj"
	-@erase "$(INTDIR)\memcof.obj"
	-@erase "$(INTDIR)\midexp.obj"
	-@erase "$(INTDIR)\midinf.obj"
	-@erase "$(INTDIR)\midpnt.obj"
	-@erase "$(INTDIR)\midsql.obj"
	-@erase "$(INTDIR)\midsqu.obj"
	-@erase "$(INTDIR)\mmid.obj"
	-@erase "$(INTDIR)\mnbrak.obj"
	-@erase "$(INTDIR)\moment.obj"
	-@erase "$(INTDIR)\mprove.obj"
	-@erase "$(INTDIR)\mrqcof.obj"
	-@erase "$(INTDIR)\mrqmin.obj"
	-@erase "$(INTDIR)\odeint.obj"
	-@erase "$(INTDIR)\pcshft.obj"
	-@erase "$(INTDIR)\pearsn.obj"
	-@erase "$(INTDIR)\piksr2.obj"
	-@erase "$(INTDIR)\piksrt.obj"
	-@erase "$(INTDIR)\pinvs.obj"
	-@erase "$(INTDIR)\plgndr.obj"
	-@erase "$(INTDIR)\poidev.obj"
	-@erase "$(INTDIR)\polcoe.obj"
	-@erase "$(INTDIR)\polcof.obj"
	-@erase "$(INTDIR)\poldiv.obj"
	-@erase "$(INTDIR)\polin2.obj"
	-@erase "$(INTDIR)\polint.obj"
	-@erase "$(INTDIR)\powell.obj"
	-@erase "$(INTDIR)\predic.obj"
	-@erase "$(INTDIR)\probks.obj"
	-@erase "$(INTDIR)\pzextr.obj"
	-@erase "$(INTDIR)\qcksrt.obj"
	-@erase "$(INTDIR)\qgaus.obj"
	-@erase "$(INTDIR)\qromb.obj"
	-@erase "$(INTDIR)\qromo.obj"
	-@erase "$(INTDIR)\qroot.obj"
	-@erase "$(INTDIR)\qsimp.obj"
	-@erase "$(INTDIR)\qtrap.obj"
	-@erase "$(INTDIR)\ran0.obj"
	-@erase "$(INTDIR)\ran1.obj"
	-@erase "$(INTDIR)\ran2.obj"
	-@erase "$(INTDIR)\ran3.obj"
	-@erase "$(INTDIR)\ran4.obj"
	-@erase "$(INTDIR)\rank.obj"
	-@erase "$(INTDIR)\ratint.obj"
	-@erase "$(INTDIR)\realft.obj"
	-@erase "$(INTDIR)\red.obj"
	-@erase "$(INTDIR)\rk4.obj"
	-@erase "$(INTDIR)\rkdumb.obj"
	-@erase "$(INTDIR)\rkqc.obj"
	-@erase "$(INTDIR)\rofunc.obj"
	-@erase "$(INTDIR)\rtbis.obj"
	-@erase "$(INTDIR)\rtflsp.obj"
	-@erase "$(INTDIR)\rtnewt.obj"
	-@erase "$(INTDIR)\rtsafe.obj"
	-@erase "$(INTDIR)\rtsec.obj"
	-@erase "$(INTDIR)\rzextr.obj"
	-@erase "$(INTDIR)\scrsho.obj"
	-@erase "$(INTDIR)\shell.obj"
	-@erase "$(INTDIR)\simp1.obj"
	-@erase "$(INTDIR)\simp2.obj"
	-@erase "$(INTDIR)\simp3.obj"
	-@erase "$(INTDIR)\simplx.obj"
	-@erase "$(INTDIR)\sinft.obj"
	-@erase "$(INTDIR)\smooft.obj"
	-@erase "$(INTDIR)\sncndn.obj"
	-@erase "$(INTDIR)\sor.obj"
	-@erase "$(INTDIR)\sort.obj"
	-@erase "$(INTDIR)\sort2.obj"
	-@erase "$(INTDIR)\sort3.obj"
	-@erase "$(INTDIR)\sparse.obj"
	-@erase "$(INTDIR)\spctrm.obj"
	-@erase "$(INTDIR)\spear.obj"
	-@erase "$(INTDIR)\splie2.obj"
	-@erase "$(INTDIR)\splin2.obj"
	-@erase "$(INTDIR)\spline.obj"
	-@erase "$(INTDIR)\splint.obj"
	-@erase "$(INTDIR)\svbksb.obj"
	-@erase "$(INTDIR)\svdcmp.obj"
	-@erase "$(INTDIR)\svdfit.obj"
	-@erase "$(INTDIR)\svdvar.obj"
	-@erase "$(INTDIR)\toeplz.obj"
	-@erase "$(INTDIR)\tptest.obj"
	-@erase "$(INTDIR)\tqli.obj"
	-@erase "$(INTDIR)\trapzd.obj"
	-@erase "$(INTDIR)\tred2.obj"
	-@erase "$(INTDIR)\tridag.obj"
	-@erase "$(INTDIR)\ttest.obj"
	-@erase "$(INTDIR)\tutest.obj"
	-@erase "$(INTDIR)\twofft.obj"
	-@erase "$(INTDIR)\user_dll.obj"
	-@erase "$(INTDIR)\vander.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\zbrac.obj"
	-@erase "$(INTDIR)\zbrak.obj"
	-@erase "$(INTDIR)\zbrent.obj"
	-@erase "$(OUTDIR)\nr.dll"
	-@erase "$(OUTDIR)\nr.exp"
	-@erase "$(OUTDIR)\nr.ilk"
	-@erase "$(OUTDIR)\nr.lib"
	-@erase "$(OUTDIR)\nr.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_USRDLL" /D "_MBCS" /Fp"$(INTDIR)/nr.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/nr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/nr.pdb" /debug /machine:I386 /out:"$(OUTDIR)/nr.dll"\
 /implib:"$(OUTDIR)/nr.lib" 
LINK32_OBJS= \
	"$(INTDIR)\amoeba.obj" \
	"$(INTDIR)\anneal.obj" \
	"$(INTDIR)\avevar.obj" \
	"$(INTDIR)\balanc.obj" \
	"$(INTDIR)\bcucof.obj" \
	"$(INTDIR)\bcuint.obj" \
	"$(INTDIR)\bessi.obj" \
	"$(INTDIR)\bessi0.obj" \
	"$(INTDIR)\bessi1.obj" \
	"$(INTDIR)\bessj.obj" \
	"$(INTDIR)\bessj0.obj" \
	"$(INTDIR)\bessj1.obj" \
	"$(INTDIR)\bessk.obj" \
	"$(INTDIR)\bessk0.obj" \
	"$(INTDIR)\bessk1.obj" \
	"$(INTDIR)\bessy.obj" \
	"$(INTDIR)\bessy0.obj" \
	"$(INTDIR)\bessy1.obj" \
	"$(INTDIR)\beta.obj" \
	"$(INTDIR)\betacf.obj" \
	"$(INTDIR)\betai.obj" \
	"$(INTDIR)\bico.obj" \
	"$(INTDIR)\bksub.obj" \
	"$(INTDIR)\bnldev.obj" \
	"$(INTDIR)\brent.obj" \
	"$(INTDIR)\bsstep.obj" \
	"$(INTDIR)\caldat.obj" \
	"$(INTDIR)\cel.obj" \
	"$(INTDIR)\chder.obj" \
	"$(INTDIR)\chebev.obj" \
	"$(INTDIR)\chebft.obj" \
	"$(INTDIR)\chebpc.obj" \
	"$(INTDIR)\chint.obj" \
	"$(INTDIR)\chsone.obj" \
	"$(INTDIR)\chstwo.obj" \
	"$(INTDIR)\cntab1.obj" \
	"$(INTDIR)\cntab2.obj" \
	"$(INTDIR)\convlv.obj" \
	"$(INTDIR)\correl.obj" \
	"$(INTDIR)\covsrt.obj" \
	"$(INTDIR)\crank.obj" \
	"$(INTDIR)\dbrent.obj" \
	"$(INTDIR)\ddpoly.obj" \
	"$(INTDIR)\des.obj" \
	"$(INTDIR)\desks.obj" \
	"$(INTDIR)\dfpmin.obj" \
	"$(INTDIR)\eclass.obj" \
	"$(INTDIR)\eclazz.obj" \
	"$(INTDIR)\eigsrt.obj" \
	"$(INTDIR)\el2.obj" \
	"$(INTDIR)\elmhes.obj" \
	"$(INTDIR)\erf.obj" \
	"$(INTDIR)\erfc.obj" \
	"$(INTDIR)\erfcc.obj" \
	"$(INTDIR)\eulsum.obj" \
	"$(INTDIR)\evlmem.obj" \
	"$(INTDIR)\expdev.obj" \
	"$(INTDIR)\f1dim.obj" \
	"$(INTDIR)\factln.obj" \
	"$(INTDIR)\factrl.obj" \
	"$(INTDIR)\fgauss.obj" \
	"$(INTDIR)\fit.obj" \
	"$(INTDIR)\fleg.obj" \
	"$(INTDIR)\flmoon.obj" \
	"$(INTDIR)\four1.obj" \
	"$(INTDIR)\fourn.obj" \
	"$(INTDIR)\fpoly.obj" \
	"$(INTDIR)\frprmn.obj" \
	"$(INTDIR)\ftest.obj" \
	"$(INTDIR)\gamdev.obj" \
	"$(INTDIR)\gammln.obj" \
	"$(INTDIR)\gammp.obj" \
	"$(INTDIR)\gammq.obj" \
	"$(INTDIR)\gasdev.obj" \
	"$(INTDIR)\gauleg.obj" \
	"$(INTDIR)\gaussj.obj" \
	"$(INTDIR)\gcf.obj" \
	"$(INTDIR)\golden.obj" \
	"$(INTDIR)\gser.obj" \
	"$(INTDIR)\hqr.obj" \
	"$(INTDIR)\hunt.obj" \
	"$(INTDIR)\indexx.obj" \
	"$(INTDIR)\irbit1.obj" \
	"$(INTDIR)\irbit2.obj" \
	"$(INTDIR)\jacobi.obj" \
	"$(INTDIR)\julday.obj" \
	"$(INTDIR)\kendl1.obj" \
	"$(INTDIR)\kendl2.obj" \
	"$(INTDIR)\ksone.obj" \
	"$(INTDIR)\kstwo.obj" \
	"$(INTDIR)\lfit.obj" \
	"$(INTDIR)\linmin.obj" \
	"$(INTDIR)\locate.obj" \
	"$(INTDIR)\lubksb.obj" \
	"$(INTDIR)\ludcmp.obj" \
	"$(INTDIR)\mdian1.obj" \
	"$(INTDIR)\mdian2.obj" \
	"$(INTDIR)\medfit.obj" \
	"$(INTDIR)\memcof.obj" \
	"$(INTDIR)\midexp.obj" \
	"$(INTDIR)\midinf.obj" \
	"$(INTDIR)\midpnt.obj" \
	"$(INTDIR)\midsql.obj" \
	"$(INTDIR)\midsqu.obj" \
	"$(INTDIR)\mmid.obj" \
	"$(INTDIR)\mnbrak.obj" \
	"$(INTDIR)\moment.obj" \
	"$(INTDIR)\mprove.obj" \
	"$(INTDIR)\mrqcof.obj" \
	"$(INTDIR)\mrqmin.obj" \
	"$(INTDIR)\odeint.obj" \
	"$(INTDIR)\pcshft.obj" \
	"$(INTDIR)\pearsn.obj" \
	"$(INTDIR)\piksr2.obj" \
	"$(INTDIR)\piksrt.obj" \
	"$(INTDIR)\pinvs.obj" \
	"$(INTDIR)\plgndr.obj" \
	"$(INTDIR)\poidev.obj" \
	"$(INTDIR)\polcoe.obj" \
	"$(INTDIR)\polcof.obj" \
	"$(INTDIR)\poldiv.obj" \
	"$(INTDIR)\polin2.obj" \
	"$(INTDIR)\polint.obj" \
	"$(INTDIR)\powell.obj" \
	"$(INTDIR)\predic.obj" \
	"$(INTDIR)\probks.obj" \
	"$(INTDIR)\pzextr.obj" \
	"$(INTDIR)\qcksrt.obj" \
	"$(INTDIR)\qgaus.obj" \
	"$(INTDIR)\qromb.obj" \
	"$(INTDIR)\qromo.obj" \
	"$(INTDIR)\qroot.obj" \
	"$(INTDIR)\qsimp.obj" \
	"$(INTDIR)\qtrap.obj" \
	"$(INTDIR)\ran0.obj" \
	"$(INTDIR)\ran1.obj" \
	"$(INTDIR)\ran2.obj" \
	"$(INTDIR)\ran3.obj" \
	"$(INTDIR)\ran4.obj" \
	"$(INTDIR)\rank.obj" \
	"$(INTDIR)\ratint.obj" \
	"$(INTDIR)\realft.obj" \
	"$(INTDIR)\red.obj" \
	"$(INTDIR)\rk4.obj" \
	"$(INTDIR)\rkdumb.obj" \
	"$(INTDIR)\rkqc.obj" \
	"$(INTDIR)\rofunc.obj" \
	"$(INTDIR)\rtbis.obj" \
	"$(INTDIR)\rtflsp.obj" \
	"$(INTDIR)\rtnewt.obj" \
	"$(INTDIR)\rtsafe.obj" \
	"$(INTDIR)\rtsec.obj" \
	"$(INTDIR)\rzextr.obj" \
	"$(INTDIR)\scrsho.obj" \
	"$(INTDIR)\shell.obj" \
	"$(INTDIR)\simp1.obj" \
	"$(INTDIR)\simp2.obj" \
	"$(INTDIR)\simp3.obj" \
	"$(INTDIR)\simplx.obj" \
	"$(INTDIR)\sinft.obj" \
	"$(INTDIR)\smooft.obj" \
	"$(INTDIR)\sncndn.obj" \
	"$(INTDIR)\sor.obj" \
	"$(INTDIR)\sort.obj" \
	"$(INTDIR)\sort2.obj" \
	"$(INTDIR)\sort3.obj" \
	"$(INTDIR)\sparse.obj" \
	"$(INTDIR)\spctrm.obj" \
	"$(INTDIR)\spear.obj" \
	"$(INTDIR)\splie2.obj" \
	"$(INTDIR)\splin2.obj" \
	"$(INTDIR)\spline.obj" \
	"$(INTDIR)\splint.obj" \
	"$(INTDIR)\svbksb.obj" \
	"$(INTDIR)\svdcmp.obj" \
	"$(INTDIR)\svdfit.obj" \
	"$(INTDIR)\svdvar.obj" \
	"$(INTDIR)\toeplz.obj" \
	"$(INTDIR)\tptest.obj" \
	"$(INTDIR)\tqli.obj" \
	"$(INTDIR)\trapzd.obj" \
	"$(INTDIR)\tred2.obj" \
	"$(INTDIR)\tridag.obj" \
	"$(INTDIR)\ttest.obj" \
	"$(INTDIR)\tutest.obj" \
	"$(INTDIR)\twofft.obj" \
	"$(INTDIR)\user_dll.obj" \
	"$(INTDIR)\vander.obj" \
	"$(INTDIR)\zbrac.obj" \
	"$(INTDIR)\zbrak.obj" \
	"$(INTDIR)\zbrent.obj"

"$(OUTDIR)\nr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "nr - Win32 Release"
# Name "nr - Win32 Debug"

!IF  "$(CFG)" == "nr - Win32 Release"

!ELSEIF  "$(CFG)" == "nr - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\user_dll.c
DEP_CPP_USER_=\
	"..\..\src\define.h"\
	"..\..\src\fltlib.h"\
	"..\..\src\header.h"\
	".\tlopen.h"\
	{$(INCLUDE)}"\sys\types.h"\
	
NODEP_CPP_USER_=\
	".\header.h"\
	

"$(INTDIR)\user_dll.obj" : $(SOURCE) $(DEP_CPP_USER_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\gcf.c
DEP_CPP_GCF_C=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\gcf.obj" : $(SOURCE) $(DEP_CPP_GCF_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\amoeba.c
DEP_CPP_AMOEB=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\amoeba.obj" : $(SOURCE) $(DEP_CPP_AMOEB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\anneal.c
DEP_CPP_ANNEA=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\anneal.obj" : $(SOURCE) $(DEP_CPP_ANNEA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\avevar.c
DEP_CPP_AVEVA=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\avevar.obj" : $(SOURCE) $(DEP_CPP_AVEVA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\balanc.c
DEP_CPP_BALAN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\balanc.obj" : $(SOURCE) $(DEP_CPP_BALAN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bcucof.c
DEP_CPP_BCUCO=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bcucof.obj" : $(SOURCE) $(DEP_CPP_BCUCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bcuint.c
DEP_CPP_BCUIN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bcuint.obj" : $(SOURCE) $(DEP_CPP_BCUIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bessi.c
DEP_CPP_BESSI=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bessi.obj" : $(SOURCE) $(DEP_CPP_BESSI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bessi0.c
DEP_CPP_BESSI0=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bessi0.obj" : $(SOURCE) $(DEP_CPP_BESSI0) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bessi1.c
DEP_CPP_BESSI1=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bessi1.obj" : $(SOURCE) $(DEP_CPP_BESSI1) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bessj.c
DEP_CPP_BESSJ=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bessj.obj" : $(SOURCE) $(DEP_CPP_BESSJ) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bessj0.c
DEP_CPP_BESSJ0=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bessj0.obj" : $(SOURCE) $(DEP_CPP_BESSJ0) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bessj1.c
DEP_CPP_BESSJ1=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bessj1.obj" : $(SOURCE) $(DEP_CPP_BESSJ1) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bessk.c
DEP_CPP_BESSK=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bessk.obj" : $(SOURCE) $(DEP_CPP_BESSK) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bessk0.c
DEP_CPP_BESSK0=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bessk0.obj" : $(SOURCE) $(DEP_CPP_BESSK0) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bessk1.c
DEP_CPP_BESSK1=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bessk1.obj" : $(SOURCE) $(DEP_CPP_BESSK1) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bessy.c
DEP_CPP_BESSY=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bessy.obj" : $(SOURCE) $(DEP_CPP_BESSY) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bessy0.c
DEP_CPP_BESSY0=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bessy0.obj" : $(SOURCE) $(DEP_CPP_BESSY0) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bessy1.c
DEP_CPP_BESSY1=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bessy1.obj" : $(SOURCE) $(DEP_CPP_BESSY1) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\beta.c
DEP_CPP_BETA_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\beta.obj" : $(SOURCE) $(DEP_CPP_BETA_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\betacf.c
DEP_CPP_BETAC=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\betacf.obj" : $(SOURCE) $(DEP_CPP_BETAC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\betai.c
DEP_CPP_BETAI=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\betai.obj" : $(SOURCE) $(DEP_CPP_BETAI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bico.c
DEP_CPP_BICO_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bico.obj" : $(SOURCE) $(DEP_CPP_BICO_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bksub.c
DEP_CPP_BKSUB=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bksub.obj" : $(SOURCE) $(DEP_CPP_BKSUB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bnldev.c
DEP_CPP_BNLDE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bnldev.obj" : $(SOURCE) $(DEP_CPP_BNLDE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\brent.c
DEP_CPP_BRENT=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\brent.obj" : $(SOURCE) $(DEP_CPP_BRENT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\bsstep.c
DEP_CPP_BSSTE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\bsstep.obj" : $(SOURCE) $(DEP_CPP_BSSTE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\caldat.c
DEP_CPP_CALDA=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\caldat.obj" : $(SOURCE) $(DEP_CPP_CALDA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\cel.c
DEP_CPP_CEL_C=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\cel.obj" : $(SOURCE) $(DEP_CPP_CEL_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\chder.c
DEP_CPP_CHDER=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\chder.obj" : $(SOURCE) $(DEP_CPP_CHDER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\chebev.c
DEP_CPP_CHEBE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\chebev.obj" : $(SOURCE) $(DEP_CPP_CHEBE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\chebft.c
DEP_CPP_CHEBF=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\chebft.obj" : $(SOURCE) $(DEP_CPP_CHEBF) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\chebpc.c
DEP_CPP_CHEBP=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\chebpc.obj" : $(SOURCE) $(DEP_CPP_CHEBP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\chint.c
DEP_CPP_CHINT=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\chint.obj" : $(SOURCE) $(DEP_CPP_CHINT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\chsone.c
DEP_CPP_CHSON=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\chsone.obj" : $(SOURCE) $(DEP_CPP_CHSON) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\chstwo.c
DEP_CPP_CHSTW=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\chstwo.obj" : $(SOURCE) $(DEP_CPP_CHSTW) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\cntab1.c
DEP_CPP_CNTAB=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\cntab1.obj" : $(SOURCE) $(DEP_CPP_CNTAB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\cntab2.c
DEP_CPP_CNTAB2=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\cntab2.obj" : $(SOURCE) $(DEP_CPP_CNTAB2) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\convlv.c
DEP_CPP_CONVL=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\convlv.obj" : $(SOURCE) $(DEP_CPP_CONVL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\correl.c
DEP_CPP_CORRE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\correl.obj" : $(SOURCE) $(DEP_CPP_CORRE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\covsrt.c
DEP_CPP_COVSR=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\covsrt.obj" : $(SOURCE) $(DEP_CPP_COVSR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\crank.c
DEP_CPP_CRANK=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\crank.obj" : $(SOURCE) $(DEP_CPP_CRANK) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\dbrent.c
DEP_CPP_DBREN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\dbrent.obj" : $(SOURCE) $(DEP_CPP_DBREN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\ddpoly.c
DEP_CPP_DDPOL=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\ddpoly.obj" : $(SOURCE) $(DEP_CPP_DDPOL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\des.c
DEP_CPP_DES_C=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\des.obj" : $(SOURCE) $(DEP_CPP_DES_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\desks.c
DEP_CPP_DESKS=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\desks.obj" : $(SOURCE) $(DEP_CPP_DESKS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\dfpmin.c
DEP_CPP_DFPMI=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\dfpmin.obj" : $(SOURCE) $(DEP_CPP_DFPMI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\eclass.c
DEP_CPP_ECLAS=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\eclass.obj" : $(SOURCE) $(DEP_CPP_ECLAS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\eclazz.c
DEP_CPP_ECLAZ=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\eclazz.obj" : $(SOURCE) $(DEP_CPP_ECLAZ) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\eigsrt.c
DEP_CPP_EIGSR=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\eigsrt.obj" : $(SOURCE) $(DEP_CPP_EIGSR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\el2.c
DEP_CPP_EL2_C=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\el2.obj" : $(SOURCE) $(DEP_CPP_EL2_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\elmhes.c
DEP_CPP_ELMHE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\elmhes.obj" : $(SOURCE) $(DEP_CPP_ELMHE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\erf.c
DEP_CPP_ERF_C=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\erf.obj" : $(SOURCE) $(DEP_CPP_ERF_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\erfc.c
DEP_CPP_ERFC_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\erfc.obj" : $(SOURCE) $(DEP_CPP_ERFC_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\erfcc.c
DEP_CPP_ERFCC=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\erfcc.obj" : $(SOURCE) $(DEP_CPP_ERFCC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\eulsum.c
DEP_CPP_EULSU=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\eulsum.obj" : $(SOURCE) $(DEP_CPP_EULSU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\evlmem.c
DEP_CPP_EVLME=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\evlmem.obj" : $(SOURCE) $(DEP_CPP_EVLME) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\expdev.c
DEP_CPP_EXPDE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\expdev.obj" : $(SOURCE) $(DEP_CPP_EXPDE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\f1dim.c
DEP_CPP_F1DIM=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\f1dim.obj" : $(SOURCE) $(DEP_CPP_F1DIM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\factln.c
DEP_CPP_FACTL=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\factln.obj" : $(SOURCE) $(DEP_CPP_FACTL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\factrl.c
DEP_CPP_FACTR=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\factrl.obj" : $(SOURCE) $(DEP_CPP_FACTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\fgauss.c
DEP_CPP_FGAUS=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\fgauss.obj" : $(SOURCE) $(DEP_CPP_FGAUS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\fit.c
DEP_CPP_FIT_C=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\fit.obj" : $(SOURCE) $(DEP_CPP_FIT_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\fleg.c
DEP_CPP_FLEG_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\fleg.obj" : $(SOURCE) $(DEP_CPP_FLEG_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\flmoon.c
DEP_CPP_FLMOO=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\flmoon.obj" : $(SOURCE) $(DEP_CPP_FLMOO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\four1.c
DEP_CPP_FOUR1=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\four1.obj" : $(SOURCE) $(DEP_CPP_FOUR1) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\fourn.c
DEP_CPP_FOURN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\fourn.obj" : $(SOURCE) $(DEP_CPP_FOURN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\fpoly.c
DEP_CPP_FPOLY=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\fpoly.obj" : $(SOURCE) $(DEP_CPP_FPOLY) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\frprmn.c
DEP_CPP_FRPRM=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\frprmn.obj" : $(SOURCE) $(DEP_CPP_FRPRM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\ftest.c
DEP_CPP_FTEST=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\ftest.obj" : $(SOURCE) $(DEP_CPP_FTEST) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\gamdev.c
DEP_CPP_GAMDE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\gamdev.obj" : $(SOURCE) $(DEP_CPP_GAMDE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\gammln.c
DEP_CPP_GAMML=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\gammln.obj" : $(SOURCE) $(DEP_CPP_GAMML) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\gammp.c
DEP_CPP_GAMMP=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\gammp.obj" : $(SOURCE) $(DEP_CPP_GAMMP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\gammq.c
DEP_CPP_GAMMQ=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\gammq.obj" : $(SOURCE) $(DEP_CPP_GAMMQ) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\gasdev.c
DEP_CPP_GASDE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\gasdev.obj" : $(SOURCE) $(DEP_CPP_GASDE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\gauleg.c
DEP_CPP_GAULE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\gauleg.obj" : $(SOURCE) $(DEP_CPP_GAULE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\gaussj.c
DEP_CPP_GAUSS=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\gaussj.obj" : $(SOURCE) $(DEP_CPP_GAUSS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\gser.c
DEP_CPP_GSER_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\gser.obj" : $(SOURCE) $(DEP_CPP_GSER_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\hqr.c
DEP_CPP_HQR_C=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\hqr.obj" : $(SOURCE) $(DEP_CPP_HQR_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\hunt.c
DEP_CPP_HUNT_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\hunt.obj" : $(SOURCE) $(DEP_CPP_HUNT_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\indexx.c
DEP_CPP_INDEX=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\indexx.obj" : $(SOURCE) $(DEP_CPP_INDEX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\irbit1.c
DEP_CPP_IRBIT=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\irbit1.obj" : $(SOURCE) $(DEP_CPP_IRBIT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\irbit2.c
DEP_CPP_IRBIT2=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\irbit2.obj" : $(SOURCE) $(DEP_CPP_IRBIT2) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\jacobi.c
DEP_CPP_JACOB=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\jacobi.obj" : $(SOURCE) $(DEP_CPP_JACOB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\julday.c
DEP_CPP_JULDA=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\julday.obj" : $(SOURCE) $(DEP_CPP_JULDA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\kendl1.c
DEP_CPP_KENDL=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\kendl1.obj" : $(SOURCE) $(DEP_CPP_KENDL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\kendl2.c
DEP_CPP_KENDL2=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\kendl2.obj" : $(SOURCE) $(DEP_CPP_KENDL2) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\ksone.c
DEP_CPP_KSONE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\ksone.obj" : $(SOURCE) $(DEP_CPP_KSONE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\kstwo.c
DEP_CPP_KSTWO=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\kstwo.obj" : $(SOURCE) $(DEP_CPP_KSTWO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\lfit.c
DEP_CPP_LFIT_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\lfit.obj" : $(SOURCE) $(DEP_CPP_LFIT_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\linmin.c
DEP_CPP_LINMI=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\linmin.obj" : $(SOURCE) $(DEP_CPP_LINMI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\locate.c
DEP_CPP_LOCAT=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\locate.obj" : $(SOURCE) $(DEP_CPP_LOCAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\lubksb.c
DEP_CPP_LUBKS=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\lubksb.obj" : $(SOURCE) $(DEP_CPP_LUBKS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\ludcmp.c
DEP_CPP_LUDCM=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\ludcmp.obj" : $(SOURCE) $(DEP_CPP_LUDCM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\mdian1.c
DEP_CPP_MDIAN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\mdian1.obj" : $(SOURCE) $(DEP_CPP_MDIAN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\mdian2.c
DEP_CPP_MDIAN2=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\mdian2.obj" : $(SOURCE) $(DEP_CPP_MDIAN2) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\medfit.c
DEP_CPP_MEDFI=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\medfit.obj" : $(SOURCE) $(DEP_CPP_MEDFI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\memcof.c
DEP_CPP_MEMCO=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\memcof.obj" : $(SOURCE) $(DEP_CPP_MEMCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\midexp.c
DEP_CPP_MIDEX=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\midexp.obj" : $(SOURCE) $(DEP_CPP_MIDEX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\midinf.c
DEP_CPP_MIDIN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\midinf.obj" : $(SOURCE) $(DEP_CPP_MIDIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\midpnt.c
DEP_CPP_MIDPN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\midpnt.obj" : $(SOURCE) $(DEP_CPP_MIDPN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\midsql.c
DEP_CPP_MIDSQ=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\midsql.obj" : $(SOURCE) $(DEP_CPP_MIDSQ) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\midsqu.c
DEP_CPP_MIDSQU=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\midsqu.obj" : $(SOURCE) $(DEP_CPP_MIDSQU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\mmid.c
DEP_CPP_MMID_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\mmid.obj" : $(SOURCE) $(DEP_CPP_MMID_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\mnbrak.c
DEP_CPP_MNBRA=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\mnbrak.obj" : $(SOURCE) $(DEP_CPP_MNBRA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\moment.c
DEP_CPP_MOMEN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\moment.obj" : $(SOURCE) $(DEP_CPP_MOMEN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\mprove.c
DEP_CPP_MPROV=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\mprove.obj" : $(SOURCE) $(DEP_CPP_MPROV) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\mrqcof.c
DEP_CPP_MRQCO=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\mrqcof.obj" : $(SOURCE) $(DEP_CPP_MRQCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\mrqmin.c
DEP_CPP_MRQMI=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\mrqmin.obj" : $(SOURCE) $(DEP_CPP_MRQMI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\odeint.c
DEP_CPP_ODEIN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\odeint.obj" : $(SOURCE) $(DEP_CPP_ODEIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\pcshft.c
DEP_CPP_PCSHF=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\pcshft.obj" : $(SOURCE) $(DEP_CPP_PCSHF) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\pearsn.c
DEP_CPP_PEARS=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\pearsn.obj" : $(SOURCE) $(DEP_CPP_PEARS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\piksr2.c
DEP_CPP_PIKSR=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\piksr2.obj" : $(SOURCE) $(DEP_CPP_PIKSR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\piksrt.c
DEP_CPP_PIKSRT=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\piksrt.obj" : $(SOURCE) $(DEP_CPP_PIKSRT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\pinvs.c
DEP_CPP_PINVS=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\pinvs.obj" : $(SOURCE) $(DEP_CPP_PINVS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\plgndr.c
DEP_CPP_PLGND=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\plgndr.obj" : $(SOURCE) $(DEP_CPP_PLGND) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\poidev.c
DEP_CPP_POIDE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\poidev.obj" : $(SOURCE) $(DEP_CPP_POIDE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\polcoe.c
DEP_CPP_POLCO=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\polcoe.obj" : $(SOURCE) $(DEP_CPP_POLCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\polcof.c
DEP_CPP_POLCOF=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\polcof.obj" : $(SOURCE) $(DEP_CPP_POLCOF) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\poldiv.c
DEP_CPP_POLDI=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\poldiv.obj" : $(SOURCE) $(DEP_CPP_POLDI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\polin2.c
DEP_CPP_POLIN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\polin2.obj" : $(SOURCE) $(DEP_CPP_POLIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\polint.c
DEP_CPP_POLINT=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\polint.obj" : $(SOURCE) $(DEP_CPP_POLINT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\powell.c
DEP_CPP_POWEL=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\powell.obj" : $(SOURCE) $(DEP_CPP_POWEL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\predic.c
DEP_CPP_PREDI=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\predic.obj" : $(SOURCE) $(DEP_CPP_PREDI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\probks.c
DEP_CPP_PROBK=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\probks.obj" : $(SOURCE) $(DEP_CPP_PROBK) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\pzextr.c
DEP_CPP_PZEXT=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\pzextr.obj" : $(SOURCE) $(DEP_CPP_PZEXT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\qcksrt.c
DEP_CPP_QCKSR=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\qcksrt.obj" : $(SOURCE) $(DEP_CPP_QCKSR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\qgaus.c
DEP_CPP_QGAUS=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\qgaus.obj" : $(SOURCE) $(DEP_CPP_QGAUS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\qromb.c
DEP_CPP_QROMB=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\qromb.obj" : $(SOURCE) $(DEP_CPP_QROMB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\qromo.c
DEP_CPP_QROMO=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\qromo.obj" : $(SOURCE) $(DEP_CPP_QROMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\qroot.c
DEP_CPP_QROOT=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\qroot.obj" : $(SOURCE) $(DEP_CPP_QROOT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\qsimp.c
DEP_CPP_QSIMP=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\qsimp.obj" : $(SOURCE) $(DEP_CPP_QSIMP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\qtrap.c
DEP_CPP_QTRAP=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\qtrap.obj" : $(SOURCE) $(DEP_CPP_QTRAP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\ran0.c
DEP_CPP_RAN0_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\ran0.obj" : $(SOURCE) $(DEP_CPP_RAN0_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\ran1.c
DEP_CPP_RAN1_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\ran1.obj" : $(SOURCE) $(DEP_CPP_RAN1_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\ran2.c
DEP_CPP_RAN2_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\ran2.obj" : $(SOURCE) $(DEP_CPP_RAN2_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\ran3.c
DEP_CPP_RAN3_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\ran3.obj" : $(SOURCE) $(DEP_CPP_RAN3_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\ran4.c
DEP_CPP_RAN4_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\ran4.obj" : $(SOURCE) $(DEP_CPP_RAN4_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\rank.c
DEP_CPP_RANK_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\rank.obj" : $(SOURCE) $(DEP_CPP_RANK_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\ratint.c
DEP_CPP_RATIN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\ratint.obj" : $(SOURCE) $(DEP_CPP_RATIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\realft.c
DEP_CPP_REALF=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\realft.obj" : $(SOURCE) $(DEP_CPP_REALF) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\red.c
DEP_CPP_RED_C=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\red.obj" : $(SOURCE) $(DEP_CPP_RED_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\rk4.c
DEP_CPP_RK4_C=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\rk4.obj" : $(SOURCE) $(DEP_CPP_RK4_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\rkdumb.c
DEP_CPP_RKDUM=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\rkdumb.obj" : $(SOURCE) $(DEP_CPP_RKDUM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\rkqc.c
DEP_CPP_RKQC_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\rkqc.obj" : $(SOURCE) $(DEP_CPP_RKQC_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\rofunc.c
DEP_CPP_ROFUN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\rofunc.obj" : $(SOURCE) $(DEP_CPP_ROFUN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\rtbis.c
DEP_CPP_RTBIS=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\rtbis.obj" : $(SOURCE) $(DEP_CPP_RTBIS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\rtflsp.c
DEP_CPP_RTFLS=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\rtflsp.obj" : $(SOURCE) $(DEP_CPP_RTFLS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\rtnewt.c
DEP_CPP_RTNEW=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\rtnewt.obj" : $(SOURCE) $(DEP_CPP_RTNEW) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\rtsafe.c
DEP_CPP_RTSAF=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\rtsafe.obj" : $(SOURCE) $(DEP_CPP_RTSAF) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\rtsec.c
DEP_CPP_RTSEC=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\rtsec.obj" : $(SOURCE) $(DEP_CPP_RTSEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\rzextr.c
DEP_CPP_RZEXT=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\rzextr.obj" : $(SOURCE) $(DEP_CPP_RZEXT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\scrsho.c
DEP_CPP_SCRSH=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\scrsho.obj" : $(SOURCE) $(DEP_CPP_SCRSH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\shell.c
DEP_CPP_SHELL=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\shell.obj" : $(SOURCE) $(DEP_CPP_SHELL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\simp1.c
DEP_CPP_SIMP1=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\simp1.obj" : $(SOURCE) $(DEP_CPP_SIMP1) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\simp2.c
DEP_CPP_SIMP2=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\simp2.obj" : $(SOURCE) $(DEP_CPP_SIMP2) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\simp3.c
DEP_CPP_SIMP3=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\simp3.obj" : $(SOURCE) $(DEP_CPP_SIMP3) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\simplx.c
DEP_CPP_SIMPL=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\simplx.obj" : $(SOURCE) $(DEP_CPP_SIMPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\sinft.c
DEP_CPP_SINFT=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\sinft.obj" : $(SOURCE) $(DEP_CPP_SINFT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\smooft.c
DEP_CPP_SMOOF=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\smooft.obj" : $(SOURCE) $(DEP_CPP_SMOOF) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\sncndn.c
DEP_CPP_SNCND=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\sncndn.obj" : $(SOURCE) $(DEP_CPP_SNCND) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\sor.c
DEP_CPP_SOR_C=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\sor.obj" : $(SOURCE) $(DEP_CPP_SOR_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\sort.c
DEP_CPP_SORT_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\sort.obj" : $(SOURCE) $(DEP_CPP_SORT_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\sort2.c
DEP_CPP_SORT2=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\sort2.obj" : $(SOURCE) $(DEP_CPP_SORT2) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\sort3.c
DEP_CPP_SORT3=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\sort3.obj" : $(SOURCE) $(DEP_CPP_SORT3) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\sparse.c
DEP_CPP_SPARS=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\sparse.obj" : $(SOURCE) $(DEP_CPP_SPARS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\spctrm.c
DEP_CPP_SPCTR=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\spctrm.obj" : $(SOURCE) $(DEP_CPP_SPCTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\spear.c
DEP_CPP_SPEAR=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\spear.obj" : $(SOURCE) $(DEP_CPP_SPEAR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\splie2.c
DEP_CPP_SPLIE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\splie2.obj" : $(SOURCE) $(DEP_CPP_SPLIE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\splin2.c
DEP_CPP_SPLIN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\splin2.obj" : $(SOURCE) $(DEP_CPP_SPLIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\spline.c
DEP_CPP_SPLINE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\spline.obj" : $(SOURCE) $(DEP_CPP_SPLINE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\splint.c
DEP_CPP_SPLINT=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\splint.obj" : $(SOURCE) $(DEP_CPP_SPLINT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\svbksb.c
DEP_CPP_SVBKS=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\svbksb.obj" : $(SOURCE) $(DEP_CPP_SVBKS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\svdcmp.c
DEP_CPP_SVDCM=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\svdcmp.obj" : $(SOURCE) $(DEP_CPP_SVDCM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\svdfit.c
DEP_CPP_SVDFI=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\svdfit.obj" : $(SOURCE) $(DEP_CPP_SVDFI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\svdvar.c
DEP_CPP_SVDVA=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\svdvar.obj" : $(SOURCE) $(DEP_CPP_SVDVA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\toeplz.c
DEP_CPP_TOEPL=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\toeplz.obj" : $(SOURCE) $(DEP_CPP_TOEPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\tptest.c
DEP_CPP_TPTES=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\tptest.obj" : $(SOURCE) $(DEP_CPP_TPTES) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\tqli.c
DEP_CPP_TQLI_=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\tqli.obj" : $(SOURCE) $(DEP_CPP_TQLI_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\trapzd.c
DEP_CPP_TRAPZ=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\trapzd.obj" : $(SOURCE) $(DEP_CPP_TRAPZ) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\tred2.c
DEP_CPP_TRED2=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\tred2.obj" : $(SOURCE) $(DEP_CPP_TRED2) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\tridag.c
DEP_CPP_TRIDA=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\tridag.obj" : $(SOURCE) $(DEP_CPP_TRIDA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\ttest.c
DEP_CPP_TTEST=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\ttest.obj" : $(SOURCE) $(DEP_CPP_TTEST) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\tutest.c
DEP_CPP_TUTES=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\tutest.obj" : $(SOURCE) $(DEP_CPP_TUTES) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\twofft.c
DEP_CPP_TWOFF=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\twofft.obj" : $(SOURCE) $(DEP_CPP_TWOFF) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\vander.c
DEP_CPP_VANDE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\vander.obj" : $(SOURCE) $(DEP_CPP_VANDE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\zbrac.c
DEP_CPP_ZBRAC=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\zbrac.obj" : $(SOURCE) $(DEP_CPP_ZBRAC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\zbrak.c
DEP_CPP_ZBRAK=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\zbrak.obj" : $(SOURCE) $(DEP_CPP_ZBRAK) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\zbrent.c
DEP_CPP_ZBREN=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\zbrent.obj" : $(SOURCE) $(DEP_CPP_ZBREN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\recipes\golden.c
DEP_CPP_GOLDE=\
	".\recipes\nrheader.h"\
	

"$(INTDIR)\golden.obj" : $(SOURCE) $(DEP_CPP_GOLDE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\tlopen.h

!IF  "$(CFG)" == "nr - Win32 Release"

!ELSEIF  "$(CFG)" == "nr - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\readme.txt

!IF  "$(CFG)" == "nr - Win32 Release"

!ELSEIF  "$(CFG)" == "nr - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
