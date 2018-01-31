VPATH = src

ifndef DEBUG_FLAG
CCBASEFLAGS=-O2 -Wall -Wno-unused-functions
LDFLAGS=-s
else
CCBASEFLAGS=-g -Wall -Wno-switch -DDEBUGGING
LDFLAGS=
endif
ifdef MINGW32
ifeq (Cygwin,$(shell uname -o))
CCBASEFLAGS += -mno-cygwin
LDFLAGS     += -mno-cygwin
endif
endif
include ../config.mk
include ../platform.mk
CDEFINES=$(GTC_ARCH_DEFINES) -DINCL_PATH=\"$(prefix)/share/gtc/include\"
CFLAGS=$(CCBASEFLAGS) $(CDEFINES)
SOURCEFILES=$(wildcard src/*.c src/*.h src/gtpack/*)
HEADERS=$(wildcard src/*.h)
OBJFILES=\
	analyze.o \
	bin2calc.o \
	cmain.o \
	decl.o \
	expr.o \
	func.o \
	gen68k.o \
	genffp.o \
	genstmt.o \
	getasm.o \
	getsym.o \
	init.o \
	intexpr.o \
	memmgt.o \
	optimize.o \
	out68k_as.o \
	peep68k.o \
	preproc.o \
	reg68k.o \
	searchkw.o \
	stmt.o \
	sunpack.o \
	symbol.o \


.PHONY: all clean install

all:	gtc

clean:
	$(RM) $(OBJFILES) src/tags
distclean: clean
	$(RM) gtc$(BIN_SUFFIX)
scratchclean: distclean

install: gtc
	mkdir -p $(prefix)/bin
	install gtc $(prefix)/bin/gtc

splint:
	splint \
		-nestcomment -paramuse -nullassign -initallelements -fullinitblock \
		+charint +boolint -boolops +ptrnegate -shiftnegative \
		-nullret -retvalint -retvalother \
		-noeffect \
		-globstate -mustfreeonly -mustfreefresh -onlytrans -statictrans -observertrans -dependenttrans -temptrans -branchstate \
		$(CDEFINES) $(OBJFILES:%.o=%.c)
	# -nestcomment     is for '//#define foo bar /* this is a comment */'
	# -paramuse        is for unused func params
	# -nullassign      is for 'int x CGLOB;'
	# -initallelements is for 'int x[5] CGLOBL;'
	# -fullinitblock   is for 'TI_SHORT x CGLOBL;'
	# +charint         is for 'char c = 2+1;' or 'if (c < 2+1)'
	# +boolint         is for 'int z = (x==y);'
	# -boolops         is for 'ptr && x'
	# +ptrnegate       is for '!ptr'
	# -shiftnegative   is for '1 << my_int' where 'my_int' has no known range
	# -noeffect        is for '(void)0'
	# -nullret         is for 'return NULL;' without '/*@null@*/' annotation
	# -retvalint       is for 'myfunc(x);' without subsequent cast to void
	# -retvalother     is for 'myfunc(x);' without subsequent cast to void

src/protos.h: src/*.c
	ruby -e '' && perl -e '' && (grep "^[^/\#*} 	].*(.*{" src/*.c | perl -pe 's/ {/;/ or die $$_' | perl -pe '$$m=$$_;s/\/\*.*?\*\// /g;/(\w+)\(/ or die $$_;print "$$1:";$$_=$$m' | perl -pe 's/\bTYP\b/struct typ/g;s/\bSYM\b/struct sym/g;s/\bTABLE\b/struct stab/g;s/\bHTABLE\b/struct hstab/g;s/\bSTACK_IMAGE\b/struct _stackimg/g;s/\bREGS_IMAGE\b/struct _regsimg/g;s/\b_pc_bcd\b/struct _pc_bcd_s/g' | sort | uniq | ruby script/mkprotos.rb > $@) || echo 1>&2 "warning: unable to rebuild 'protos.h'"

gtc: $(SOURCEFILES)
	$(MAKE) gtc-rebuild

#$(OBJFILES): $(HEADERS)
gtc-rebuild: $(OBJFILES)
	$(CC) -o gtc $(LDFLAGS) $(OBJFILES)

%.o: %.c vcg.c error.c *.h gtpack/*.h Makefile config.mk
	$(CC) -c $(CFLAGS) $<
