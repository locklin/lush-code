# Generated automatically from Makefile.in by configure.

SHELL=/bin/sh
top=..
srcdir=.
LN_S=ln -s

HLPFILES=boolean.hlp control.hlp date.hlp functions.hlp graphics.hlp	\
         helptool.hlp htable.hlp inputoutput.hlp list.hlp matrix.hlp	\
         miscellaneous.hlp numbers.hlp objects.hlp ogre.hlp open.hlp	\
         strings.hlp symbol.hlp lush.hlp tltools.hlp copying.hlp         \
         dz.hlp  

TOPFILES= copying.hlp lush.hlp ogre.hlp tltools.hlp open.hlp

BKFILES= copying.bk lush.bk ogre.bk tltools.bk open.bk

all: hlpfiles ${BKFILES} bkfiles

.PHONY: clean distclean all 
.PHONY: bkfiles hlpfiles

hlpfiles:
	@for n in ${HLPFILES} ; do \
	  test -r $$n || ${LN_S} ${srcdir}/$$n ./$$n ;\
	done

${BKFILES} : ${HLPFILES}
	@touch ./REINDEX

bkfiles:
	@if [ -r ./REINDEX ] ; then \
	  rm -f ./REINDEX ;\
	  echo ${srcdir}/makeindex ${TOPFILES} ;\
	  ${srcdir}/makeindex ${TOPFILES} ;\
	fi

clean: 
	-rm 2>/dev/null ${BKFILES}

distclean:
	-rm 2>/dev/null ${BKFILES}
	-rm 2>/dev/null Makefile
