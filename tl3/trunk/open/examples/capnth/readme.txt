
TL/OPEN EXAMPLE: CAPITALIZE-NTH



This directory contains a simple example of TL/Open.
Additional comments can be found in section
"String Capitalization Example" of the TL/Open manual.



------- Files:

capnth.mdp		Workspace file for visual c++
capnth.mak		Makefile for visual c++
readme.txt		This file
user_dll.c		Implementation file
tlopen.h		TL/Open header file
capnth.sn		TLisp file for loading extension


-------- How to compile and run this example (Windows):

Double click on icon "capnth.mdp".
The visual C++ window appears.
Press key F7 for compiling the TL/Open extension.

You can now start WinTL3 and load file "capnth.sn"

	? (load "../open/examples/capnth/capnth")
	 [capnth.dll]

Function <capitalize_nth> is now loaded and documented.

	? ^Acap

	----------------------------------
	Search List
	----------------------------------

	Found:
	   1. [capnth]      File "capnth.sn".
	   2. [capnth]      (capitalize-nth <s> <n>)
	   3. [open]        String Capitalization Example.
	(V)iew, (1-3) select, (S)earch list, (Q)uit :?

