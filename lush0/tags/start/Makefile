# Generated automatically from Makefile.in by configure.

SHELL=/bin/sh
top=.
srcdir=.
bindir=/usr/local/bin
libdir=/usr/local/lib
lushdir=${libdir}/lush/lush
LN_S=ln -s
INSTALL=${top}/config/install-sh -c
SUBDIRS=src sys doc
exe=
dll=.so

# Main rule

all:
	@echo "+++ Compiling..."
	@for subdir in ${SUBDIRS}; do (cd $$subdir;${MAKE} $@ ) || exit 10; done
	@echo "+++ Compilation Done."

clean: 
	@for subdir in ${SUBDIRS}; do (cd $$subdir; ${MAKE} $@ ); done
	-rm 2>/dev/null -rf bin packages

distclean: configure include/config.h.in
	@for subdir in ${SUBDIRS} ; do (cd $$subdir; ${MAKE} $@ ); done	
	-rm 2>/dev/null Makefile config.cache config.status config.log 
	-rm 2>/dev/null include/config.h include/autoconf.h
	-rm 2>/dev/null -rf bin packages

.PHONY: clean distclean all


# Autoconf

${srcdir}/configure: configure.in 
	cd ${srcdir} && autoconf

${srcdir}/include/config.h.in: configure.in
	echo > ${srcdir}/include/config.h.in
	cd ${srcdir} && autoheader

