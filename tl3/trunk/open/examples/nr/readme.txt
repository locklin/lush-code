

NUMERICAL RECIPES INTERFACE
---------------------------


YOU MUST READ THIS:
  Before proceeding, you *must* make sure that you have 
  a licence from Numerical Recipes Software that allows 
  you to use the Numerical Recipes library on your computer.  
  Note the file "NR.DLL" produced by the procedure described
  below is not freely redistributable because it contains 
  software copyrighted by Numerical Recipes Software.


Here here a step by step procedure for interfacing TL3 and
the well known "Numerical Recipes in C" library (1st edition).
Note that the Numerical Recipes routines are seldom
the best choice for numerical computation. Yet these 
routines are well documented and widely distributed.

You may follow this procedure if you have access to this
library (the first edition is *required*). You can also
use file "user_dll.c" as a collection of examples
for interfacing new Numerical Recipes routines in 
your own TL/Open project.


(1) COPY THE NUMERICAL RECIPES SOURCE INTO DIRECTORY RECIPES
 
  Copy the Numerical Recipes files into directory "Recipes"


(2) FIX FILE NRHEADER.H

  Insert the following lines at the beginning of 
  file NRHEADER.H provided with the "Numerical Recipes" library. 

	#ifdef WIN32
	#pragma warning(disable: 4244 4101)
	#include <stdlib.h>
	#endif


(3) FIX FILE RAN0.C

  Insert the following lines at the beginning of 
  file RAN0.C provided with the "Numerical Recipes" library. 

	#include "nrheader.h"


(4) COMPILE THIS PROJECT

  With key F7.


(5) TEST NR.SN

  You can now load file "nr.sn" from WinTL3.
  This file will load the shared library "nr.dll"
  containing both the Numerical Recipes routines
  and the WinTL3 interface.
