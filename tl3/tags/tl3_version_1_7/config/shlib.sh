# This file is included from the configuration script
# If the system supports shared libraries, this
# script must define the following variables:

# -- set to yes if system supports shared libraries
have_shlib=no
# -- executable suffix
exe_suffix=
# -- shared library suffix
shlib_suffix=.so
# -- compiler flag for generating pic code (cc vs gcc)
cc_pic_flag=
gcc_pic_flag=
# -- compiler flag for exporting main program symbols
cc_exp_flag=
gcc_exp_flag=
# -- compiler flag for creating a shared library
cc_link_shlib=
gcc_link_shlib=
# -- custom command for creating shared library (has priority)
command_link_shlib=

# Systems with mention (check) must be checked...
# Variable 'have_shlib' is then set to 'maybe'

case "$host" in

    # BSD (check)
    # -- always uses GCC
    # -- version 2 must link c++ runtime
    # -- version 3 uses -shared
    *-freebsd2*|*-netbsd*|*-openbsd*)
	test x${GCC-no} = xyes && have_shlib=maybe
	gcc_pic_flag='-fPIC'
        gcc_link_shlib='-Wl,-Bshareable'
	if test -r /usr/lib/c++rt0.o ; then
	   gcc_link_shlib="$gcc_link_shlib,/usr/lib/c++rt0.o"
	fi
	;;
    *-freebsd3*)
	test x${GCC-no} = xyes && have_shlib=maybe
	gcc_pic_flag='-fPIC'
	gcc_link_shlib='-shared'
	;;

    # HPUX (check)
    # -- shared lib suffix is .sl
    *-hpux*)
	have_shlib=yes
	shlib_suffix=.sl
	cc_pic_flag='+Z'
	cc_exp_flag='-Wl,-E'
	gcc_pic_flag='-fPIC'
	gcc_exp_flag='-Wl,-E'
        command_link_shlib='ld -b +s +b -E -o'
	;;		

    # IRIX (check)
    # -- option -shared should work fine
    *-irix*)
	have_shlib=yes
	cc_pic_flag='-KPIC'
	cc_link_shlib='-shared'
	gcc_pic_flag='-fPIC'
	gcc_link_shlib='-shared'
	;;

    # LINUX
    # -- always uses GCC
    *-linux*)
	test x${GCC-no} = xyes && have_shlib=yes
	gcc_pic_flag='-fPIC'
	gcc_exp_flag='-Wl,-export-dynamic'
	gcc_link_shlib='-shared'
	;;

    # OSF (check)
    # -- option -shared should do it 
    *-osf*)
	have_shlib=yes
	cc_link_shlib='-Wl,-shared,-expect_unresolved,\*'
	gcc_link_shlib='-shared -Wl,-expect_unresolved,\*'
	;;

    # SCO (check)
    # -- sysVr4
    *-sco3.2v5*)
	have_shlib=maybe
	cc_pic_flag='-Kpic'
	cc_link_shlib='-Wl,-G'
	gcc_pic_flag='-fPIC'
	gcc_link_shlib='-Wl,-G'
	;;

    # SOLARIS (check)
    # -- sysVr4
    *-solaris2*|*-sunos5*)
	have_shlib=yes
	cc_pic_flag='-KPIC'
	cc_link_shlib='-Wl,-G'
	gcc_pic_flag='-fPIC'
	gcc_link_shlib='-shared'
	;;

    # SUNOS (check)
    # -- must use GCC
    *-sunos4*)
	test x${GCC-no} = xyes && have_shlib=yes
	gcc_pic_flag='-fPIC'
	gcc_link_shlib='-shared'
	;;

esac

