# -*-Shell-script-*-
# ----------------------------------------
# Initialization
# ----------------------------------------

AC_PREREQ(2.50)
AC_INIT(lush, 1.0)
AC_REVISION($Id: configure.ac,v 1.9 2002-08-02 15:08:47 leonb Exp $)
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


# ----------------------------------------
# Programs
# ----------------------------------------

AC_PROG_MAKE_SET

AC_PROG_CC
AC_CC_OPTIMIZE
AC_SUBST(GCC)
AC_PROG_CXX
AC_PROG_F77


AC_PROG_RANLIB
AC_PROG_LN_S
AC_PATH_PROG(MV, mv)
AC_PATH_PROG(CP, cp)
AC_PATH_PROG(RM, rm)
AC_PATH_PROG(AR, ar)
AC_PATH_PROG(TOUCH, touch)
AC_PATH_PROG(INDENT, indent)
INSTALL='${top_srcdir}/config/install-sh -c' 
AC_SUBST(INSTALL)

# ----------------------------------------
# Libraries
# ----------------------------------------

AC_CHECK_LIB(m,sqrt)
AC_CHECK_LIB(dl,dlopen)
AC_CHECK_LIB(dld,shl_load)
AC_CHECK_LIB(dld,shl_load)

AC_CHECK_LIB(iberty, xmalloc)
AC_CHECK_LIB(bfd, bfd_init,,[ac_has_bfd=no])
BFD_YES=''
test x$ac_has_bfd = xno && BFD_YES='#'
AC_SUBST(BFD_YES)

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

AC_PATH_PTHREAD


# ----------------------------------------
# Header Files
# ----------------------------------------

AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_HEADER_SYS_WAIT     
AC_CHECK_HEADERS(memory.h string.h strings.h)
AC_CHECK_HEADERS(unistd.h sys/mman.h)
AC_CHECK_HEADERS(dlfcn.h dl.h ieeefp.h fpu_control.h)
AC_CHECK_HEADERS(stropts.h sys/stropts.h sys/select.h sys/ttold.h)
AC_CHECK_HEADERS(sys/time.h sys/timeb.h bfd.h)

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
AC_CHECK_FUNCS(ieee_handler fpsetmask __setfpucw)
AC_CHECK_FUNCS(tanh sincos dlopen sigsetjmp)
AC_CHECK_FUNCS(mprotect getpagesize gettimeofday ftime)


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
# Diagnostics
# ----------------------------------------

if test x$BFD_YES != x ; then
    AC_MSG_WARN([Compiling without the BFD library.
---------------------------------------------------
This substantially reduces the capability of lush.
* If you are using Linux, check that you have the
  binutils-devel package, and check that its version
  number matches the installed binutils.
* Otherwise you should download and compile the
  GNU binutils source on your system.
---------------------------------------------------])
fi

# ----------------------------------------
# Output
# ----------------------------------------

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(src/Makefile)
AC_CONFIG_FILES(include/autoconf.h)
AC_OUTPUT

