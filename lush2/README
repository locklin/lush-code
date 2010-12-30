------------------------------------------------------------------------
LUSH2: The Lisp Universal SHell, version 2.x
------------------------------------------------------------------------

Lush is an object-oriented Lisp interpreter/compiler with features
designed to please people who want to prototype large numerical
applications. Lush includes an extensive library of
vector/matrix/tensor manipulation, numerous numerical libraries
(including GSL, LAPACK, and BLAS), a set of graphic functions, a
simple GUI toolkit, and interfaces to various graphic and multimedia
libraries such as OpenGL, SDL, Video4Linux, and ALSA (video/audio
grabbing), and others. Lush is an ideal frontend script language for
programming projects written in C or other languages.  Lush also has
libraries for Machine Learning, Neural Nets and statistical estimation
(which are the main interests of the authors).

What sets Lush apart from other languages such as Python, Perl,
Matlab, or S+ is the efficiency of its compiled numerical code, and
its almost seamless integration with C. It is very easy to interface
an existing C library to Lush and call its functions from the
interpreter.  It is even possible to freely intermix Lisp and C code
within one function.  Another advantage of Lush is the extreme
simplicity of its syntax (a far cry from, say, Perl or Matlab).

Historically, Lush has been used mainly in R&D environments to develop
applications in signal processing, machine learning data analysis,
computer vision, optimization, image processing, statistics,
visualisation, etc.... But Lush is a general language that can be used
for a variety of applications. For example, Lush has been used to
write simple video games, or to teach kids to program (being an
interpreter with a very simple syntax).

Some may say "this sounds like Matlab, or S+, or Python". The difference 
is that Lush is much more efficient than these systems, both in terms of 
memory and execution time, thanks to its efficient compilation to C and
its rather unusual memory management. This efficiency allows 
semi-real-time applications such as audio and video processing or 
real-time computer games (using OpenGL and/or SDL).

Lush is the direct descendent of the SN system which was originally
developed by Leon Bottou and Yann LeCun as the front-end of a neural 
network simulator. Various incarnations of SN have been developed 
continuously since 1987, some of which were sold commercially by
Neuristique S.A. in France, and eventually grew into a full-fledged
prototyping and development environment.

The version developed at AT&T Bell Labs, and then at AT&T Labs 
was used to build many successful technologies and products.
The most notable ones are:
- a handwriting recognition system used by many banks 
  across the world to read checks automatically.
  In fact, some ATM machines made by NCR (that can read 
  checks) run compiled SN code on embedded DSP boards.
- the prototype of the DjVu image compression system.
- numerous machine learning algorithms developed 
  at AT&T since 1988, including the "LeNet" family of
  convolutional neural networks and some early 
  implementations of the Support Vector Machine 
  algorithm.

------------------------------------------------------------------------
COPYING AND USING LUSH2
------------------------------------------------------------------------

Lush2 is distributed under the LGPL. 

If you are a researcher and publish a paper on result primarily 
obtained with Lush, particularly if you use Lush's more exotic
machine learning libraries, we would apreciate an acknowledgement, 
for example in the form of a reference to: "Bottou, L. and LeCun, Y: 
the Lush Reference Manual,  http://lush.sourceforge.net". 
This is only a suggestion, by no means a demand or requirement,
but we sure would apreciate it.


------------------------------------------------------------------------
WHAT IS IN THIS DISTRIBUTION
------------------------------------------------------------------------

demos: A set of executable Lush scripts that demonstrate
     its capabilities.

config: contains configuration scripts and information

src: source code of the interpreter

lib: .a or .so files (currently unused)

etc: various shell scripts and utilities

include: .h files

sys: core libraries (lush sources) without which Lush cannot run.
     A minimal/customized version of Lush needs only that directory to run.

lsh: library files (lush sources) that are part of the standard distribution.
     Although they are not required for Lush to run, life would 
     really suck without them.

packages: library files (lush sources) for special applications or platforms, or
     programs that have been contributed by users and cannot be assumed to be 
     present/working in all installations of Lush.

local: (empty) lush libraries that are specific to your site.

doc: documentation in PDF, LaTeX.


------------------------------------------------------------------------
PRE-REQUISITES TO INSTALLATION
------------------------------------------------------------------------

Before you install Lush, you must make sure that certain packages
are installed on your system. 

ABSOLUTE PREREQUISITES (without this, Lush doesn't work):

- Under LINUX: libiberty and libbfd: 
  These libraries are included in most Linux 
  distros, but are not necessarily installed by default. They are
  generally found in one of the "binutils" or "libbinutils" packages 
  (For example, Mandrake 8.2 has it in libbinutils2-devel-2.11.92.0.12-6mdk.rpm
  and Mandrake 9.0 in libbinutils2-devel-2.12.90.0.15-1mdk.i586.rpm).
  CAUTION: if you are on Linux, install those from the RPMs that
  come with your distro. DO NOT try to download, compile, and install 
  binutils from the FSF as this may override some pretty low-level
  C libraries and screw up your system. If you are on a non-Linux
  system, you are probably OK installing this from source.


OPTIONAL PREREQUISITES (without these, Lush is a lot less fun):
[ A list of the corresponding RPM packages for various 
  Linux distros is given at the bottom of this file]

- GSL, BLAS, LAPACK: Most Linux distros come with some version of the Gnu 
  Scientific Library (GSL), and the Linear Algebra Packages (LAPACK) on 
  the CD, but the corresponding packages are generally not installed by 
  default. Go to you CD and install gsl or libgsl, lapack or liblapack, 
  and the corresponding -devel packages. You can also download the source
  and install them by hand.
- SDL: Most Linux distros come with some version of the Simple Directmedia 
  Layer library (SDL), but many do not install the -devel packages by default.
  (e.g. on Mandrake 8.2 libSDL1.2-1.2.3-6mdk and libSDL1.2-devel-1.2.3-6mdk).
- OpenGL: Lush has support for 3D graphics with OpenGL. Make sure
  you have accelerated OpenGL correctly set up on your machine
  if you want to use that.
- OpenCV, OpenRM, ALSA, HTK, libaudiofile: those packages are used by
  some Lush packages. Install them if you need them.


------------------------------------------------------------------------
COMPILING AND INSTALLING LUSH UNDER UNIX
------------------------------------------------------------------------

1) COMPILATION

cd to the directory where Lush was unpacked 
(say $HOME/lush2), then type:

  % ./configure
  % make

Scan the output of "make" for possible error messages.  
The Lush executable is  $HOME/lush2/bin/lush2. 
You can run the Lush executable directly, or you can add $HOME/lush2/bin 
to your shell path, or you can also create a symbolic link from somewhere 
in your path to the Lush executable. Creating the link can be done with:
  % ln -s $HOME/lush2/bin/lush2  /somwhere-in-your-path/lush2

Alternatively, you can su to root and type "make install" in the Lush directory. 
This will perform a system-level installation of Lush. The installation 
directories by default are /usr/local/{bin,src/lush2,share/lush2,man/man1).

------------------------------------------------------------------------
RUNNING LUSH INTERACTIVELY
------------------------------------------------------------------------

You can then run the executable "bin/lush2".  The documentation
browsing tool can be fired up by typing (helptool) at the Lush
prompt. You can leave Lush by typing CTRL-D at the prompt.

On startup, Lush loads various libraries from the sys and lsh
directories, as well as a .lush2/lushrc.lsh file in the user's 
home directory.

Some users add a directory lsh in their home directory and include the
line (addpath "your-home-directory/lsh") to their lushrc so that Lush
can automatically search for programs in that directory.

It is quite convenient to run Lush from within Emacs.  simply add the
following line to your .emacs file:
  (load "the-lush-directory/etc/lush.el")
Then in Emacs, type "Meta-X lush" (i.e. "ESC-X lush" or "ALT-X lush").


------------------------------------------------------------------------
RUNNING NON-INTERACTIVE LUSH SCRIPTS
------------------------------------------------------------------------

In Unix, Lush can be used to write scripts that can be
called from a shell prompt (like shell or Perl scripts).
A list of command-line arguments are put in the argv variable.

Here is an example: create a file (say "capargs") with the following 
content (replacing the first line by the path to your lush executable):

  #!/bin/sh
  exec lush2 "$0" "$@"
  !#
  (printf "capitalizing the arguments:\n")
  (each ((arg argv)) (printf "%s %s\n" arg (upcase arg)))

then, make capargs executable: chmod a+x capargs.
You can now invoke capargs at the shell prompt:

  % capargs asd gfdf
  capitalizing the arguments:
  capargs CAPARGS
  asd ASD
  gfdf GFDF

------------------------------------------------------------------------
REPORTING BUGS
------------------------------------------------------------------------

To submit a bug report, go to:
http://sourceforge.net/tracker/?atid=410163&group_id=34223&func=browse
click "Submit New", and enter a bug report

If you want to request a new feature, go to:
http://sourceforge.net/tracker/?atid=410166&group_id=34223&func=browse
and click "Submit New".


------------------------------------------------------------------------
HISTORY
------------------------------------------------------------------------

Lush is the direct descendant of the SN system. SN was first developed 
as a neural network simulator with a Lisp-like scripting language.
The project was started in 1987 by Leon Bottou and Yann LeCun, and 
rewritten several times since then. SN was used at AT&T for many research 
projects in machine learning, pattern recognition, and image processing. 
Its various incarnations were used at AT&T Bell Labs, AT&T Labs, the Salk 
Institute, the University of Toronto, Universite of Montreal, UC Berkeley, 
and many other research institutions. The commercial versions of SN were 
used in several large companies as a prototyping tool: Thomson-CSF, ONERA,....

Contributors include: Leon Bottou, Yann LeCun, Patrice Simard,
Yoshua Bengio, Jean Bourrelly, Patrick Haffner, Pascal Vincent,
Sergey Ioffe, and many others.

Here is a family tree of the various incarnations of SN and Lush:

SN(1987) neural network simulator for AmigaOS (Leon Bottou, Yann LeCun)
 |
SN1(1988) ported to SunOS. Added shared-weight neural nets and graphics (LeCun)
 |   \
 |   SN1.3(1989) commercial version for Unix (Neuristique)
 |   /
SN2(1990) new lisp interpreter and graphic functions (Bottou)
   |   \
   |   SN2.2(1991) commercial version (Neuristique)
   |    |
   |   SN2.5(1991) ogre GUI toolkit (Neuristique)
   |   / \
    \ /  SN2.8(1993+) enhanced version (Neuristique)
     |     \
     |   TL3(1993+) lisp interpreter for Unix and Win32 (Neuristique)
     |      [GPL]
     |        \_______________________________________________
     |                                                        |
   SN27ATT(1991) custom AT&T version                          |
     |        (LeCun, Bottou, Simard, AT&T Labs)              |
     |                                                        |
   SN3(1992) IDX matrix engine, Lisp->C compiler/loader and   |
     |       gradient-based learning library                  |
     |       (Bottou, LeCun, AT&T)                            |
     |                                                        |
   SN3.1(1995) redesigned compiler, added OpenGL and SGI VL   |
     |         support (Bottou, LeCun, Simard, AT&T Labs)     |
     |                                                        |
   SN3.2(2000) hardened/cleanup SN3.x code,                   |
     |         added SDL support (LeCun)                      |
     | _______________________________________________________|
     |/
     |
   ATTLUSH(2001) merging of TL3 interpreter + SN3.2 compiler
   [GPL]         and libraries (Bottou, LeCun, AT&T Labs).
     |
   LUSH(2002) rewrote the compiler/loader (Bottou, NEC Research Institute)
   [GPL]
     |
   LUSH(2002) rewrote library, documentation, and interfaced packages
   [GPL]      (LeCun, Huang-Fu, NEC) 
     | \    
     |  \
     | PSU LUSH(2005) incremental API redesign, Gnuplot & other bindings
     | [GPL]          (Juengling @ Portland State University)
     |   |  
     |   |  versions 1.1, 1.2, 1.3 (2005 - 2008)
     |   |  concurrent garbage collector (SoC 2008)
     |  /
     | /
   LUSH2 (2009-current)
   [LGPL]
     
------------------------------------------------------------------------
LIST OF PACKAGES THAT SHOULD BE INSTALLED BEFORE INSTALLING LUSH
------------------------------------------------------------------------

REQUIRED:
libbinutils2-2.12.90.0.15-1mdk.i586.rpm (installed)
libbinutils2-devel-2.12.90.0.15-1mdk.i586.rpm

OPTIONAL:
libgsl0-1.2-2mdk.i586.rpm
libgsl0-devel-1.2-2mdk.i586.rpm
gsl-doc-1.2-2mdk.i586.rpm
gsl-progs-1.2-2mdk.i586.rpm

libblas3-3.0-1mdk.i586.rpm
libblas3-devel-3.0-1mdk.i586.rpm
liblapack3-3.0-1mdk.i586.rpm
liblapack3-devel-3.0-1mdk.i586.rpm

libalsa2-0.9.0-0.8rc2mdk.i586.rpm (installed)
libalsa-data-0.9.0-0.8rc2mdk.i586.rpm (installed)
libalsa2-devel-0.9.0-0.8rc2mdk.i586.rpm
libalsa2-docs-0.9.0-0.8rc2mdk.i586.rpm

libSDL1.2-1.2.4-11mdk.i586.rpm (installed)
libSDL_image1.2-1.2.2-3mdk.i586.rpm (installed)
libSDL_mixer1.2-1.2.4-5mdk.i586.rpm (installed)
libSDL_net1.2-1.2.4-5mdk.i586.rpm (installed)
libSDL_ttf2.0-2.0.5-3mdk.i586.rpm (installed)

libSDL1.2-devel-1.2.4-11mdk.i586.rpm
libSDL_image1.2-devel-1.2.2-3mdk.i586.rpm
libSDL_mixer1.2-devel-1.2.4-5mdk.i586.rpm
libSDL_net1.2-devel-1.2.4-5mdk.i586.rpm
libSDL_ttf2.0-devel-2.0.5-3mdk.i586.rpm

libMesaGL1-4.0.3-6mdk.i586.rpm (installed)
libMesaGLU1-4.0.3-6mdk.i586.rpm (installed)
libMesaglut3-4.0.3-6mdk.i586.rpm (installed)
Mesa-4.0.3-6mdk.i586.rpm (installed)
Mesa-demos-4.0.3-6mdk.i586.rpm (installed)
libMesaGLU1-devel-4.0.3-6mdk.i586.rpm
libMesaglut3-devel-4.0.3-6mdk.i586.rpm
