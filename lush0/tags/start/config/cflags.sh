# This file is included from the configuration script
# You can override the variable CFLAGS and LDFLAGS from here.
# You should pay attention to the compiler variable GCC.

# -- GCC compilation with full warnings
if test x${GCC-no} = xyes
then
    CFLAGS="$CFLAGS -Wall -Wno-uninitialized"
fi

# -- Various system dependent optimizations
case "$host" in

  *-hpux*)
      if test x${GCC-no} != xyes 
      then
        # CC: Ansi-extended, no warnings
        CFLAGS="${CFLAGS} -Ae -w" 
      fi
      ;;

  *-linux*)
      # Extra warnings enabled
      CFLAGS="$CFLAGS -Wstrict-prototypes"
      ;;

  *-irix*)
      if test x${GCC-no} != xyes
      then 
        # CC: no warnings
        CFLAGS="$CFLAGS -w"  
      fi
      ;;

esac



