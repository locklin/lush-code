# -*-Shell-script-*-
# ----------------------------------------
# Initialization
# ----------------------------------------

AC_PREREQ(2.50)
AC_INIT(lush, 1.0)
AC_REVISION($Id: configure.ac,v 1.21 2003-01-26 20:05:02 leonb Exp $)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(README)
AC_CONFIG_HEADER(include/config.h)
AC_PREFIX_DEFAULT(/usr/local)
AC_CANONICAL_HOST

AC_ARG_WITH(extra-includes,
            AC_HELP_STRING([--with-extra-includes=DIR],
                       [Define an additional directory for include files]),
        [ if test -d "$withval" ; then
            CPPFLAGS="$CPPFLAGS -I$withval" 
          else
            AC_MSG_ERROR([Cannot stat directory $withval])
          fi ] )

AC_ARG_WITH(extra-libraries,
            AC_HELP_STRING([--with-extra-libraries=DIR],
                       [Define an additional directory for library files]),
        [ if test -d "$withval" ; then
           LDFLAGS="$LDFLAGS -L$withval"
          else
            AC_MSG_ERROR([Cannot stat directory $withval])
          fi ] )


curwd=`pwd`
if test -d $curwd/gnu/include ; then
   CPPFLAGS="$CPPFLAGS -I$curwd/gnu/include"
fi
if test -d $curwd/gnu/lib ; then
   LDFLAGS="$LDFLAGS -L$curwd/gnu/lib"
fi


# ----------------------------------------
# Programs
# ----------------------------------------

AC_PROG_MAKE_SET

AC_PROG_CC
AC_CC_OPTIMIZE
AC_SUBST(GCC)
AC_PROG_CXX
AC_PROG_F77
AC_PROG_CPP


AC_PROG_RANLIB
AC_PROG_LN_S
AC_PATH_PROG(MV, mv)
AC_PATH_PROG(CP, cp)
AC_PATH_PROG(RM, rm)
AC_PATH_PROG(AR, ar)
AC_PATH_PROG(TOUCH, touch)

AC_PATH_PROG(INDENT, indent)
if test -x $INDENT ; then
  AC_MSG_CHECKING([if $INDENT accepts GNU options])
  if $INDENT -gnu </dev/null 2>/dev/null ; then 
    AC_MSG_RESULT(yes)
  else 
    AC_MSG_RESULT(no)
    unset INDENT
  fi
fi

INSTALL='${top_srcdir}/config/install-sh -c' 
AC_SUBST(INSTALL)

# ----------------------------------------
# Libraries
# ----------------------------------------

# misc
AC_CHECK_LIB(m,sqrt)
AC_CHECK_LIB(dl,dlopen)
AC_CHECK_LIB(dld,shl_load)
AC_CHECK_LIB(util,openpty)

# bfd
AC_ARG_WITH(bfd,
    AC_HELP_STRING([--without-bfd],
        [Compile Lush without the BFD library.]),
    [require_bfd=$withval],[require_bfd=yes])

if test x$require_bfd = xno ; then
    BFD_YES='#'
    AC_MSG_WARN([Compiling Lush without BFD])
else
    BFD_YES=''
    has_bfd=yes
    AC_CHECK_LIB(iberty, xmalloc)
    AC_CHECK_HEADER(bfd.h,,has_bfd=no)
    AC_CHECK_LIB(bfd, bfd_init,,[has_bfd=no])
    if test x$has_bfd = xno ; then
        AC_MSG_ERROR([Unable to locate BFD development files.
-------------------------------------------------------------
We were unable to locate required GNU binutils files.
- Some Linux distributions do not install these by default.
  You need to install the 'libbinutils-devel' package.
- On other platforms, get GNU binutils on www.fsf.org.
You can configure Lush to run without these files by using 
option "--without-bfd".  The dynamic loader/linker/compiler 
will not work and Lush will be seriously crippled.
-----------------------------------------------------------------])
    fi
fi
AC_SUBST(BFD_YES)


# x11
AC_PATH_X
AC_PATH_XTRA
X_LIBS=
test -n "$x_libraries" && X_LIBS="-L$x_libraries"
X_YES='#'
if test "${no_x-no}" != yes ; then 
  X_LIBS="$X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS"
  X_YES=''
fi
AC_SUBST(X_LIBS)
AC_SUBST(X_YES)

# threads
AC_PATH_PTHREAD

# readline
AC_CHECK_LIB(curses,tputs)
AC_CHECK_LIB(readline,readline)


# ----------------------------------------
# Header Files
# ----------------------------------------

AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_HEADER_SYS_WAIT     
AC_CHECK_HEADERS(memory.h string.h strings.h)
AC_CHECK_HEADERS(unistd.h sys/mman.h termios.h pty.h util.h)
AC_CHECK_HEADERS(dlfcn.h dl.h ieeefp.h fpu_control.h fenv.h)
AC_CHECK_HEADERS(stropts.h sys/stropts.h sys/select.h sys/ttold.h)
AC_CHECK_HEADERS(sys/time.h sys/timeb.h readline/readline.h bfd.h)
AC_SYS_LARGEFILE

# ----------------------------------------
# Types
# ----------------------------------------

AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_PID_T

# ----------------------------------------
# Functions
# ----------------------------------------

AC_FUNC_MMAP
AC_FUNC_FORK
AC_FUNC_STRFTIME
AC_CHECK_FUNCS(strchr memcpy sigaction sigvec strerror getcwd)
AC_CHECK_FUNCS(feenableexcept ieee_handler fpsetmask __setfpucw)
AC_CHECK_FUNCS(tanh sincos dlopen sigsetjmp waitpid openpty)
AC_CHECK_FUNCS(mprotect getpagesize gettimeofday ftime cfree)
AC_CHECK_FUNCS(gethostbyname flockfile sysconf fpathconf tcsetattr)
AC_CHECK_FUNCS(setenv putenv fseeko ftello)


# ----------------------------------------
# Xtra
# ----------------------------------------

LDCC=${CC}
SOEXT=so
CC_EXP_FLAG=
CC_PIC_FLAG=
MAKESO=

if test x${GCC} = xyes ; then
    CC_PIC_FLAG='-fPIC'
    MAKESO="$LDCC -shared -o"
    # Linking the main executable with g++ helps
    # when dynlinking c++ object files.
    if test x${GXX} = xyes ; then
       LDCC=${CXX}
    fi
fi

# Tested?
case "$host" in
    *-linux*)
        CC_EXP_FLAG='-Wl,-export-dynamic'
        ;;
    *-freebsd2*)
        CC_PIC_FLAG='-fPIC'
        MAKESO="$LDCC -Wl,-Bshareable"
        test -r /usr/lib/c++rt0.o && MAKESO="$MAKESO,/usr/lib/c++rt0.o"
        MAKESO="$MAKESO -o"
        ;;
    *-hpux-*)
        SOEXT=sl
        test x$GCC != xyes && CC_PIC_FLAG='+Z'
        test x$GCC != xyes && CC_EXP_FLAG='-Wl,-E'
        test x$GCC != xyes && MAKESO='ld -b +s +b -E -o'
        ;;
    *-irix*)
        test x$GCC != xyes && CC_PIC_FLAG='-KPIC'
        ;;
    *-osf*)
        text x$GCC = yes && MAKESO="$LDCC -shared -Wl,-expect_unresolved -o"
        test x$GCC != xyes && MAKESO="$LDCC -Wl,-shared,-expect_unresolved -o"
        ;;
    *-solaris*|*-sunos5*)
        test x$GCC != xyes && CC_PIC_FLAG='-KPIC'
        test x$GCC != xyes && MAKESO='$LDCC -Wl,-G -o'
        ;;
esac

AC_SUBST(LDCC)
AC_SUBST(SOEXT)
AC_SUBST(CC_EXP_FLAG)
AC_SUBST(CC_PIC_FLAG)
AC_SUBST(MAKESO)

if test x$EXEEXT = xexe ; then
    SOEXT=dll
    DEFS="$DEFS -DWIN32"
    UNIX_YES='#'
    WIN32_YES=''
else
    UNIX_YES=''
    WIN32_YES='#'
fi
AC_SUBST(UNIX_YES)
AC_SUBST(WIN32_YES)

AC_DEFINE_UNQUOTED(EXT_DLL, ".$SOEXT",  [Extension for shared libraries])
AC_DEFINE_UNQUOTED(EXT_OBJ, ".$OBJEXT", [Extension for object files])


# ----------------------------------------
# Output
# ----------------------------------------

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(src/Makefile)
AC_CONFIG_FILES(include/autoconf.h)
AC_OUTPUT

