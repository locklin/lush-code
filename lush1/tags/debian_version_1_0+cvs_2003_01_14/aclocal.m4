
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA



dnl -------------------------------------------------------
dnl @synopsis AC_CHECK_CC_OPT(OPTION, 
dnl                           ACTION-IF-OKAY,
dnl                           ACTION-IF-NOT-OKAY)
dnl Check if compiler accepts option OPTION.
dnl -------------------------------------------------------
AC_DEFUN(AC_CHECK_CC_OPT,[
 opt="$1"
 AC_MSG_CHECKING([if $CC accepts $opt])
 echo 'void f(){}' > conftest.c
 if test -z "`${CC} ${CFLAGS} ${OPTS} $opt -c conftest.c 2>&1`"; then
    AC_MSG_RESULT(yes)
    rm conftest.*
    $2
 else
    AC_MSG_RESULT(no)
    rm conftest.*
    $3
 fi
])

dnl -------------------------------------------------------
dnl @synopsis AC_CC_OPTIMIZE
dnl Setup option --enable-debug
dnl Collects optimization/debug option in variable OPTS
dnl Filter options from CFLAGS
dnl -------------------------------------------------------
AC_DEFUN(AC_CC_OPTIMIZE,[
   AC_REQUIRE([AC_CANONICAL_HOST])
   AC_ARG_ENABLE(debug,
        AC_HELP_STRING([--enable-debug],
                       [Compile with debugging options (default: no)]),
        [ac_debug=$enableval],[ac_debug=no])
   OPTS=
   AC_SUBST(OPTS)
   saved_CFLAGS="$CFLAGS"
   CFLAGS=
   for opt in $saved_CFLAGS ; do
     case $opt in
       -O*) ;;
       -g*) OPTS="$OPTS $opt" ;;
       *) CFLAGS="$CFLAGS $opt" ;;
     esac
   done
   if test x$ac_debug = xno ; then
     OPTS=-DNO_DEBUG
     AC_CHECK_CC_OPT([-Wall],[OPTS="$OPTS -Wall"])
     AC_CHECK_CC_OPT([-O3],[OPTS="$OPTS -O3"],
        [ AC_CHECK_CC_OPT([-O2], [OPTS="$OPTS -O2"] ) ] )
     AC_CHECK_CC_OPT([-funroll-loops], [OPTS="$OPTS -funroll-loops"])
     case "${host_cpu}" in
        i?86)
           opt="-mcpu=${host_cpu}"
           AC_CHECK_CC_OPT([$opt], [OPTS="$OPTS $opt"])
           ;;
      esac
   fi
])


dnl ------------------------------------------------------------------
dnl @synopsis AC_PATH_PTHREAD([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl This macro figures out how to build C programs using POSIX
dnl threads.  It sets the PTHREAD_LIBS output variable to the threads
dnl library and linker flags, and the PTHREAD_FLAGS output variable
dnl to any special C compiler flags that are needed.  (The user can also
dnl force certain compiler flags/libs to be tested by setting these
dnl environment variables.).
dnl ------------------------------------------------------------------
AC_DEFUN([AC_PATH_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
acx_pthread_ok=no
# First, check if the POSIX threads header, pthread.h, is available.
# If it isn't, don't bother looking for the threads libraries.
AC_CHECK_HEADER(pthread.h, , acx_pthread_ok=noheader)
# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).
# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works.
if test x${PTHREAD_LIBS+set} = xset ||
   test x${PTHREAD_FLAGS+set} = xset ; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_FLAGS"
        save_CXXFLAGS="$CXXFLAGS"
        CXXFLAGS="$CXXFLAGS $PTHREAD_FLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([provided PTHREAD_LIBS/PTHREAD_FLAGS.])
        AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
        AC_MSG_RESULT($acx_pthread_ok)
        if test x"$acx_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_FLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
        CXXFLAGS="$save_CXXFLAGS"
fi
# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all.
acx_pthread_flags="pthreads none -Kthread -kthread lthread
                   -pthread -pthreads -mthreads pthread
                   --thread-safe -mt"
# The ordering *is* (sometimes) important.  
# Some notes on the individual items follow:
# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
# pthread: Linux, etcetera
# --thread-safe: KAI C++
case "${host_cpu}-${host_os}" in
        *solaris*)
        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthread or
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:
        acx_pthread_flags="-pthread -pthreads pthread -mt $acx_pthread_flags"
        ;;
esac
if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do
        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;
                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_FLAGS="$flag"
                ;;
                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac
        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
        save_CXXFLAGS="$CXXFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CFLAGS="$CFLAGS $PTHREAD_FLAGS"
        CXXFLAGS="$CXXFLAGS $PTHREAD_FLAGS"
        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>],
                    [pthread_t th; pthread_join(th, 0);
                     pthread_attr_init(0); pthread_cleanup_push(0, 0);
                     pthread_create(0,0,0,0); pthread_cleanup_pop(0); ],
                    [acx_pthread_ok=yes])
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
        CXXFLAGS="$save_CXXFLAGS"
        AC_MSG_RESULT($acx_pthread_ok)
        if test "x$acx_pthread_ok" = xyes; then
                break;
        fi
        PTHREAD_LIBS=""
        PTHREAD_FLAGS=""
done
fi
# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_FLAGS"
        save_CXXFLAGS="$CXXFLAGS"
        CXXFLAGS="$CXXFLAGS $PTHREAD_FLAGS"
        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
                *-aix* | *-freebsd*)     flag="-D_THREAD_SAFE";;
                *solaris* | alpha*-osf*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
                PTHREAD_FLAGS="$flag $PTHREAD_FLAGS"
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
        CXXFLAGS="$save_CXXFLAGS"
fi
AC_ARG_VAR(PTHREAD_LIBS, [Flags for linking pthread programs.])
AC_ARG_VAR(PTHREAD_FLAGS, [Flags for compiling pthread programs.])
# execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
        AC_DEFINE(HAVE_PTHREAD,1,[Define if pthreads are available])
        ifelse([$1],,:,[$1])
else
        ifelse([$2],,:,[$2])
fi
])

