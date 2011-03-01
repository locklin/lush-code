# -*-Shell-script-*-
# ----------------------------------------
# Initialization
# ----------------------------------------

AC_PREREQ(2.50)
AC_INIT(Lush2, 2.0)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(README)
AC_CONFIG_HEADER(include/lushconf.h)
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

# Always look into a "gnu" directory.
curwd=`pwd`
if test -d $curwd/gnu/include ; then
   CPPFLAGS="$CPPFLAGS -I$curwd/gnu/include"
fi
if test -d $curwd/gnu/lib ; then
   LDFLAGS="$LDFLAGS -L$curwd/gnu/lib"
fi

# Check for fink on macs.
case "$host" in
  *-darwin* | *-macos10*)
     if test -d /opt/local ; then
       CPPFLAGS="$CPPFLAGS -I/opt/local/include"
       LDFLAGS="$LDFLAGS -L/opt/local/lib"
     elif test -d /sw ; then
       CPPFLAGS="$CPPFLAGS -I/sw/include"
       LDFLAGS="$LDFLAGS -L/sw/lib"
     fi
  ;;
esac


# ----------------------------------------
# Overrides
# ----------------------------------------

require_readline=yes
AC_ARG_WITH(readline,
    AC_HELP_STRING([--without-readline],
        [Build Lush without command line editing support.]),
    [require_readline=$withval])

require_bfd=yes
AC_ARG_WITH(bfd,
    AC_HELP_STRING([--without-bfd],
        [Build Lush without the BFD library.]),
    [require_bfd=$withval])

require_snapshot_gc=no
AC_ARG_WITH(snapshot-gc,
    AC_HELP_STRING([--with-snapshot-gc], [Use child processes for garbage collection.]),
    [ if test $withval != no ; then
        require_snapshot_gc=yes
    fi ])
if test $require_snapshot_gc = yes ; then
    AC_DEFINE(MM_SNAPSHOT_GC,1,[Define when compiling MM with snapshot GC.])
fi
    
ac_have_mpi=no
AC_ARG_WITH(mpi,
            AC_HELP_STRING([--with-mpi],
                       [Build Lush with MPI support.]),
        [ if test $withval != no ; then
            ac_have_mpi=yes
          fi ] )
if test $ac_have_mpi = yes ; then
  AC_PATH_PROG(CC, mpicc, none)
  AC_PATH_PROG(CXX, mpiCC, none)
  AC_PATH_PROG(F77, mpif77, none)
  if test $CC = none || test $CXX = none || test $F77 = none ; then
    AC_MSG_ERROR([Cannot locate MPI compiler drivers])
  fi
  AC_DEFINE(HAVE_MPI,1,[Define if compiling with MPI.])
fi


# ----------------------------------------
# Programs
# ----------------------------------------

AC_PROG_MAKE_SET

AC_PROG_CC_C99
if test $ac_cv_prog_cc_c99 = no; then
    AC_MSG_ERROR([Cannot find C99 compliant compiler.])
fi
AC_CC_COMPLEX
AC_CC_OPTIMIZE
AC_SUBST(GCC)
AC_PROG_CXX
AC_PROG_F77
AC_PROG_CPP

AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_LN_S
AC_PATH_PROG(MV, mv)
AC_PATH_PROG(CP, cp)
AC_PATH_PROG(RM, rm)
AC_PATH_PROG(AR, ar)
AC_PATH_PROG(TOUCH, touch)
AC_PATH_PROG(INDENT, indent)

if test -x "$INDENT" ; then
  AC_MSG_CHECKING([if $INDENT accepts GNU options])
  if $INDENT -gnu </dev/null 2>/dev/null ; then 
    AC_MSG_RESULT(yes)
  else 
    AC_MSG_RESULT(no)
    unset INDENT
  fi
fi


# ----------------------------------------
# Libraries
# ----------------------------------------

# misc
AC_CHECK_LIB(m,sqrt)
AC_CHECK_LIB(dl,dlopen)
AC_CHECK_LIB(dld,shl_load)
AC_CHECK_LIB(util,openpty)

# NSModule API
AC_CHECK_FUNCS(NSLinkModule,[require_bfd=no])

# bfd
if test x$require_bfd = xno ; then
    BFD_YES='#'
else
    BFD_YES=''
    has_bfd=yes
    AC_CHECK_HEADER(bfd.h,,has_bfd=no)
    AC_CHECK_LIB(bfd, bfd_init,,[has_bfd=no])
fi
AC_SUBST(BFD_YES)

# Check if it compiles
if test x$has_bfd = xyes ; then
    n_LIBS=$LIBS
    has_intl=
    AC_CHECK_LIB(intl, dcgettext, [has_intl=yes],[has_intl=no])
    i_LIBS=`echo $n_LIBS | sed -e 's/-liberty/& -lintl/'`
    sn_LIBS=`echo $n_LIBS | sed -e 's/-lbfd\( -liberty\)*/-Wl,-Bstatic & -Wl,-Bdynamic/'`
    si_LIBS=`echo $i_LIBS | sed -e 's/-lbfd\( -liberty\)*/-Wl,-Bstatic & -Wl,-Bdynamic/'`
    LIBS=$sn_LIBS
    AC_MSG_CHECKING([whether bfd works with -Bstatic])
    AC_TRY_LINK([#include "bfd.h"],
                [bfd_openr("junk1","default"); ],
		[okay_bfd=yes],[okay_bfd=no]);
    AC_MSG_RESULT($okay_bfd)
    if test x$okay_bfd = xno ; then
	if test x$has_intl = xyes ; then
	    LIBS=$si_LIBS
	    AC_MSG_CHECKING([whether bfd works with -Bstatic and -lintl])
	    AC_TRY_LINK([#include "bfd.h"],
		    [bfd_openr("junk1","default"); ],
		    [okay_bfd=yes],[okay_bfd=no]);
	    AC_MSG_RESULT($okay_bfd)
	fi
	if test x$okay_bfd = xno ; then
	    LIBS=$n_LIBS
	    AC_MSG_CHECKING([whether bfd works alone])
	    AC_TRY_LINK([#include "bfd.h"],
		    [bfd_openr("junk1","default"); ],
		    [okay_bfd=yes],[okay_bfd=no]);
	    AC_MSG_RESULT($okay_bfd)
	    if test x$okay_bfd = xno ; then
		if test x$has_intl = xyes ; then
		    LIBS=$i_LIBS
		    AC_MSG_CHECKING([whether bfd works with -lintl])
		    AC_TRY_LINK([#include "bfd.h"],
			    [bfd_openr("junk1","default"); ],
			    [okay_bfd=yes],
			    [okay_bfd=no]);
		    AC_MSG_RESULT($okay_bfd)
		fi
		if test x$okay_bfd = xno ; then
		    LIBS=$n_LIBS
		fi
	    fi
	fi
    fi
    if test x$okay_bfd = xyes ; then
	AC_MSG_CHECKING([whether bfd_hash_table_init wants two arguments])
        AC_TRY_COMPILE([#include "bfd.h"],
                       [struct bfd_hash_table t; bfd_hash_table_init(&t,(void*)0);],
                       [bfd_hash_init_2_args=yes],
                       [bfd_hash_init_2_args=no])
        AC_MSG_RESULT($bfd_hash_init_2_args)
        if test x$bfd_hash_init_2_args = xyes ; then
            AC_DEFINE(HAVE_BFD_HASH_TABLE_INIT_WANTS_2_ARGS,1,
                      [Define if bfd_hash_table_init() takes two arguments])
        fi
    fi
    if test x$okay_bfd = xno ; then
	has_bfd=no
    fi
fi

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

# x11
AC_PATH_XTRA
if test "${no_x-no}" != yes ; then
  X_LIBS="$X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS" 
  X_YES=''
else
  X_YES='#'
fi
AC_PATH_XFT
AC_SUBST(X_LIBS)
AC_SUBST(X_CFLAGS)
AC_SUBST(X_YES)

# threads
AC_PATH_PTHREAD

# libreadline
if test $require_readline = yes; then
    AC_CHECK_LIB(readline,readline)
fi

# libiconv
AC_CHECK_LIB(iconv,libiconv_open)

# ----------------------------------------
# Header Files
# ----------------------------------------

AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_HEADER_SYS_WAIT     
AC_CHECK_HEADERS(wchar.h wctype.h langinfo.h iconv.h)
AC_CHECK_HEADERS(memory.h string.h strings.h limits.h)
AC_CHECK_HEADERS(unistd.h sys/mman.h termios.h pty.h util.h)
AC_CHECK_HEADERS(dlfcn.h dl.h ieeefp.h fpu_control.h fenv.h)
AC_CHECK_HEADERS(stropts.h sys/stropts.h sys/select.h sys/types.h sys/ttold.h)
AC_CHECK_HEADERS(sys/time.h sys/timeb.h locale.h bfd.h)
if test $require_readline = yes; then
    AC_CHECK_HEADERS(readline/readline.h readline/history.h)
fi
AC_SYS_LARGEFILE

if test $ac_have_mpi = yes ; then
  AC_CHECK_HEADERS(mpi.h)
fi


# ----------------------------------------
# Types
# ----------------------------------------

AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_CHECK_TYPES(long long int)
AC_CHECK_TYPES(wchar_t,,,[#include "stddef.h"])
AC_CHECK_TYPES(wint_t,,,[#include "wchar.h"])
AC_CHECK_TYPES(mbstate_t,,,[#include "wchar.h"])



# ----------------------------------------
# Functions
# ----------------------------------------

AC_FUNC_MMAP
AC_FUNC_FORK
AC_FUNC_STRFTIME
AC_CHECK_FUNCS(strchr memcpy sigaction sigvec strerror getcwd)
AC_CHECK_FUNCS(feenableexcept fpsetmask __setfpucw)
AC_CHECK_FUNCS(tanh dlopen sigsetjmp waitpid openpty)
AC_CHECK_FUNCS(mprotect getpagesize gettimeofday ftime cfree)
AC_CHECK_FUNCS(gethostbyname flockfile sysconf fpathconf tcsetattr)
AC_CHECK_FUNCS(setenv putenv fseeko ftello)
AC_CHECK_FUNCS(rl_getc)
AC_FUNC_MBRTOWC
AC_CHECK_FUNCS(setlocale nl_langinfo)


# iconv function might be defined as libiconv in iconv.h
AC_MSG_CHECKING([for iconv])
ac_has_iconv=no
AC_TRY_LINK([#include "iconv.h"],
            [iconv((iconv_t)0, (char**)0, (size_t*)0, (char**)0, (size_t*)0);],
            [ac_has_iconv=yes])
AC_MSG_RESULT($ac_has_iconv)
if test x$ac_has_iconv = xyes ; then
    AC_DEFINE(HAVE_ICONV,1,
        [Define to 1 if you have the 'iconv' function.])
fi




# ----------------------------------------
# Xtra
# ----------------------------------------

LDCC=${CC}
SOEXT=so
CC_EXP_FLAG=
CC_PIC_FLAG=
MAKESO=

CPP_YES='#'
if test x${GCC} = xyes ; then
    test x${GXX} = xyes && LDCC=${CXX} && CPP_YES=''
    CC_PIC_FLAG='-fPIC'
    MAKESO="$LDCC -shared -o"
fi
AC_SUBST(CPP_YES)

# Tested?
case "$host" in
    *-linux*)
        CC_EXP_FLAG='-Wl,-export-dynamic'
        ;;
    *-darwin* | *-macos10* | *-rhapsody*)
        SOEXT=dylib
        MAKESO="$LDCC -bundle -flat_namespace -undefined suppress -o"
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

UNIX_YES=''
WIN32_YES='#'
AC_SUBST(UNIX_YES)
AC_SUBST(WIN32_YES)

AC_DEFINE_UNQUOTED(EXT_DLL, ".$SOEXT",  [Extension for shared libraries])
AC_DEFINE_UNQUOTED(EXT_OBJ, ".$OBJEXT", [Extension for object files])


# ----------------------------------------
# Output
# ----------------------------------------

AC_DEFINE_INSTALL_PATHS

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(src/Makefile)
AC_CONFIG_FILES(include/lushmake.h)
AC_OUTPUT

# ----------------------------------------
# CONFIG template
# ----------------------------------------

AH_TOP([
#ifndef LUSHCONF_H
#define LUSHCONF_H
/* lushconf.h: begin */
])

AH_BOTTOM([
/* lushconf.h: custom */
# ifndef UNIX
#  define UNIX
# endif
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif
/* lushconf.h: end */
#endif
])
