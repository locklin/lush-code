#############################################################################
#
# sn3.2 Makefile
#
#############################################################################

SN3DIR = /home/snwiz/sn3.2

###
### To make an ordinary sn3.2 executable, cd OBJS/$(OS),
### where OS can be SUNOS, SOLARIS, SGI or ALPHA,
### and type:
#
# make sn3.2
# make install       (moves sn3.2 to ../../../bin/OBJS/$(OS) )
#

###
### To make a "specialized" sn3.2 with external objects linked in statically,
### define EXT_OBJS1, EXT_OBJS2, EXT_OBJS3 in a file, (e.g, as 
### in file ../examples/obj-list) to specify the objects to link statically,
### and then run the script 'MakeMySn <obj-list> [<sn-name>]' in order to
### generate a version of sn3.2 with statically linked objects, named <sn-name>
### in the current directory. The default <sn-name> is '$(PWD)/my-sn'.
### For example the obj-list file can contain lines as follows:
#
# setenv EXT_OBJS1 "~/my_dir/OBJS/SOLARIS/my_obj1.o ~/my_dir/OBJS/SOLARIS/my_obj2.o ~/my_dir/OBJS/SOLARIS/my_obj3.o"
# setenv EXT_OBJS2 "~/my_dir/OBJS/SOLARIS/my_obj4.o ~/my_dir/OBJS/SOLARIS/my_obj5.o ~/my_dir/OBJS/SOLARIS/my_obj6.o"
# setenv EXT_OBJS3 "~/my_dir/OBJS/SOLARIS/my_obj7.o ~/my_dir/OBJS/SOLARIS/my_obj8.o ~/my_dir/OBJS/SOLARIS/my_obj9.o"
#
#

default: all
	@echo "Done"

###
### include operating system and machine specific defintions:
###
include MakeInclude.$(OS)
###


###
### To compile under debug (e.g. -g), override variable DEBUG
### and use the following 2 lines:
#
#DEBUG = -ggdb
#OPT = $(DEBUG)
#
### else use the following line:
DEBUG =
###
#
### (OPT is normally defined to -O4 or equivalent, in MakeInclude.$(OS) )


#############################################################################

# Default executable (but can be redefined outside using make -e)
#
VERSION = sn3.2
SN      = ${VERSION}
RCSREV	     = 

# By default the "util" stuff is loaded dynamically.
# but to link it statically, set this to libutil.a or other/more libraries
#
UTILIB       =

############################################################################
######################### BELOW, INDEPENDENT OF OS ########################## 
############################################################################

all: $(SN)

idx1.c idx2.c idx3.c idx4.c: idx.h

############# OBJS
              
 MAIN         = main.o

 OBJS1        = allocate.o at.o binary.o calls.o classify.o eval.o \
                fileio.o fltlib.o function.o io.o oostruct.o object.o remote.o\
                string.o symbol.o toplevel.o unix.o dld.o dldbfd.o hash.o\
                dz_machine.o dz_lisp.o storage.o index.o pseudo_io.o \
                gdbm_driver.o

 OBJS2        = graphics.o ps_driver.o lisp_driver.o x11_driver.o

 OBJS3        = dh.o lisp_c.o
 OBJS4        = idx1.o idx2.o idx3.o idx4.o check_func.o
 OBJS5        = att.o nr.o


# by default no external objects, but this can be redefined
# in the environment and using make -e.
 EXT_OBJS1     = 
 EXT_OBJS2    =
 EXT_OBJS3    =
 EXT_OBJS     = $(EXT_OBJS1) $(EXT_OBJS2) $(EXT_OBJS3) $(UOBJS)

OBJS = $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5)


$(MAIN):  fltlib.h define.h header.h ext_init.h init.h touch-to-recompile-main

$(OBJS):  fltlib.h define.h header.h
$(OBJS1):
$(OBJS2):  graphics.h
$(OBJS3):  dh.h 
$(OBJS4):  idxmac.h idxops.h check_func.h
$(OBJS5):  dh.h


##############################################################################
##############################################################################


CFLAGS = $(OPT) $(DEBUG) -I$(SN3DIR)/include $(CCFLAGS) -DSN3DIR=\"$(SN3DIR)\" -DVERSION=\"$(VERSION)\"

$(SN): $(MAIN) $(OBJS) $(EXT_OBJS) 
	-rm old$(SN)
	-mv $(SN) old$(SN)
	$(CC) -o $(SN) $(DEBUG) $(LDFLAGS) $(OBJS) $(EXT_OBJS) $(MAIN) $(EXT_LIBDIR) $(EXT_LIB) $(LIBRARIES)


init.h: $(OBJS) Makefile
	Makeinit.sh $(OBJS) 

ext_init.h: $(EXT_OBJS) touch-to-recompile-main
	mv init.h tmp
	Makeinit.sh $(EXT_OBJS)
	mv init.h ext_init.h
	mv tmp init.h

link:
	$(CC) -o $(SN) $(LDFLAGS) $(OBJS) $(MAIN) $(LIBRARIES)

##############################################################################
##############################################################################
                                 
 .c.o:
	$(CC) -c $(CFLAGS) $*.c -o $*.o
# .c.o:
#	cpp-g $*.c > $*-ppg.c
#	$(CC) -c $(CFLAGS) -o $*.o $*-ppg.c

co:
	co -r${RCSREV} *.c *.h Makefile

clean:
	rm -f *.o $(SN) sn2setkey

install:
	-mv $(BINDIR)/$(SN)@3@ $(BINDIR)/$(SN)@4@
	-mv $(BINDIR)/$(SN)@2@ $(BINDIR)/$(SN)@3@
	-mv $(BINDIR)/$(SN)@1@ $(BINDIR)/$(SN)@2@
	-mv $(BINDIR)/$(SN) $(BINDIR)/$(SN)@1@
	mv $(SN) $(BINDIR)
	chmod a+x $(BINDIR)/$(SN)


