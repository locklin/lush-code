
TL/OPEN EXAMPLE: COMPLEX
========================


This directory contains an example demonstrating
advanced features of TL/Open.
Additional comments can be found in section
"Complex Number Example" of the TL/Open manual.




------- Files:

complex.mdp		Workspace file for visual c++
complex.mak		Makefile for visual c++
readme.txt		This file
user_dll.c		Implementation file
tlopen.h		TL/Open header file
complex.sn		TLisp file for loading extension


-------- How to compile and run this example (Windows):

Double click on icon "complex.mdp".
The visual C++ window appears.
Press key F7 for compiling the TL/Open extension.

You can now start WinTL3 and load file "capnth.sn"

	? (load "../open/examples/complex/complex")
	 [complex.dll]

This package defines a new type of primitive lisp
objects for representing complex numbers as well
as new primitive functions for using these numbers.

Documentation can be obtained by typing 

	? ^Acomplex.sn
