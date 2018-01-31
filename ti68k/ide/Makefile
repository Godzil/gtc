# You will have to define environment variables corresponding to your system.

ifdef GTCROOT
TIGCC = $(GTCROOT)/tigcc_for_ide/tigcc
LIBS = '$(shell $(TOTIGCCPATH) $(GTCROOT)/tigcc_for_ide/extgraph.a)'
endif
ifdef EXTGRAPH
LIBS = $(EXTGRAPH)
endif
ifndef NOT_CYGWIN
TOTIGCCPATH = cygpath -aw
INCLUDES = -I'$(shell $(TOTIGCCPATH) $(GTCROOT)/base_dir)' -I'$(shell $(TOTIGCCPATH) .)'
endif
ifndef TIGCC
TIGCC = tigcc
endif

SWITCHES = -W -Wall -Wno-unused -O2 -fno-strict-aliasing -mregparm=3 -DFORCE_SMALL_FONT


all: bin/gtc-ide.89z bin/gtc-ide.9xz bin/gtc-ide.v2z

superscratchclean:
	rm -f bin/*

bin/gtc-ide.89z : calc = _89
bin/gtc-ide.89z : ext  = 89
bin/gtc-ide.9xz : calc = _92
bin/gtc-ide.9xz : ext  = 9x
bin/gtc-ide.v2z : calc = _92
bin/gtc-ide.v2z : ext  = v2

bin/gtc-ide.89z bin/gtc-ide.9xz bin/gtc-ide.v2z: *.c *.h
	-mkdir tmp.ide$(ext)
	cp main.c sunpack.c tmp.ide$(ext)
	cd tmp.ide$(ext) && $(TIGCC) $(INCLUDES) $(SWITCHES) -D$(calc) -o gtc_ide main.c sunpack.c $(LIBS)
	cp tmp.ide$(ext)/gtc_ide.$(ext)z bin/gtc-ide.$(ext)z
	rm -rf tmp.ide$(ext)


pedrom: bin/gtc-ide-pedrom.89z bin/gtc-ide-pedrom.9xz bin/gtc-ide-pedrom.v2z

bin/gtc-ide-pedrom.89z : calc = _89
bin/gtc-ide-pedrom.89z : ext  = 89
bin/gtc-ide-pedrom.9xz : calc = _92
bin/gtc-ide-pedrom.9xz : ext  = 9x
bin/gtc-ide-pedrom.v2z : calc = _92
bin/gtc-ide-pedrom.v2z : ext  = v2

bin/gtc-ide-pedrom.89z bin/gtc-ide-pedrom.9xz bin/gtc-ide-pedrom.v2z: *.c *.h
	-mkdir tmp.idepedrom$(ext)
	cp main.c sunpack.c tmp.idepedrom$(ext)
	cd tmp.idepedrom$(ext) && $(TIGCC) -DPEDROM $(INCLUDES) $(SWITCHES) -D$(calc) -o gtc_ide main.c sunpack.c $(LIBS)
	cp tmp.idepedrom$(ext)/gtc_ide.$(ext)z bin/gtc-ide-pedrom.$(ext)z
	rm -rf tmp.idepedrom$(ext)
