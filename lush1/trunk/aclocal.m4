
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


