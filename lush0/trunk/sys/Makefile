# Generated automatically from Makefile.in by configure.

SHELL=/bin/sh
top=..
srcdir=.
LN_S=ln -s

TOKENIZE=${srcdir}/../etc/tokenize

LUSHCFILES=sysenv.lshc dz-compile.lshc

all: lushfiles ${LUSHCFILES} 

.SUFFIXES: .lshc .lsh
.PHONY: clean distclean all lushfiles

lushfiles:
	@for m in ${srcdir}/*.lsh ; do \
	  test -r `basename $$m` || $(LN_S) $$m ./`basename $$m` ;\
	done

.lsh.lshc:
	${TOKENIZE} ./$* ./$@

clean: 
	-rm 2>/dev/null *.lshc *.lshc

distclean:
	-rm 2>/dev/null *.lshc *.lshc
	-rm 2>/dev/null Makefile

