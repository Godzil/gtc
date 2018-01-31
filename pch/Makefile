# - no, it's not a true makefile...
# - yes, it's a .bat quick-and-dirtily converted to a makefile
# :-b

BIN=$(shell pwd)/../bin
MERGE=perl script/merge.pl
PCHMK=$(BIN)/pchmaker
DAT89=$(BIN)/dat89y -e HDR
O2EXT=$(BIN)/obj2ti -x

PCHFILES = stdhead.pch keywords.pch estack.pch estackle.pch events.pch homescr.pch tiosdlg.pch wingraph.pch xupaki.pch #gen.pch extgraph.pch
TIPCHFILES = $(patsubst %.pch,%.89y,$(PCHFILES))

all: src/* srcdata/* subprojects/xupaki.pch
	$(RM) lex.txt *.pchsource
	cp src/lex.txt src/*.pchsource .
	$(MAKE) $(TIPCHFILES) $(PCHFILES)
	$(RM) lex.txt pchlog.txt *.pchsource
	touch all
stdhead.pchsource: src/*
	$(MERGE) src/stdhead-light.pchmerge
tiosdlg.pchsource: src/*
	$(MERGE) src/tiosdlg-merge.pchmerge
keywords.pch: keywords.pchsource extfiles
	$(PCHMK) $<
stdhead.pch: stdhead.pchsource extfiles
	$(PCHMK) $<
estackle.pch: estackle.pchsource stdhead.pch extfiles
	$(PCHMK) -istdhead.def $<
estack.pch: estack.pchsource stdhead.pch estackle.pch extfiles
	$(PCHMK) -istdhead.def -iestackle.def $<
events.pch: events.pchsource stdhead.pch extfiles
	$(PCHMK) -istdhead.def $<
homescr.pch: homescr.pchsource stdhead.pch extfiles
	$(PCHMK) -istdhead.def $<
tiosdlg.pch: tiosdlg.pchsource stdhead.pch extfiles
	$(PCHMK) -istdhead.def $<
wingraph.pch: wingraph.pchsource stdhead.pch extfiles
	$(PCHMK) -istdhead.def $<
#gen.pch: srcdata/gen.pch
#	cp $< $@
#extgraph.pch: srcdata/extgraph.pch
#	cp $< $@
xupaki.pch: subprojects/xupaki.pch
	cp $< $@
%.89y: %.pch
	$(DAT89) -f zheader $<

clean:
	$(RM) lex.txt pchlog.txt *.pchsource
	$(RM) extfiles
	$(RM) -r a2ext

distclean: clean

scratchclean: distclean
	$(RM) *.pch *.89y *.def subprojects/*.pch all

include ../config.mk
install: all
	mkdir -p $(prefix)/share/gtc/include
	install -m 644 *.pch $(prefix)/share/gtc/include

try-import-tigcc-archive:
	mv -f src/tigcc-archive/tigcc.a srcdata/tigcc.a&&rm -f src/tigcc-archive/*.o||true

srcdata/tigcc.a: try-import-tigcc-archive

extfiles: srcdata/*
	$(RM) -r a2ext
	mkdir a2ext
	sh -c 'cd a2ext && ar x ../srcdata/tigcc.a'
	cp srcdata/export.dat srcdata/*.ext srcdata/*.ref a2ext
	$(MAKE) -C a2ext -f ../ext.mk O2EXT="$(O2EXT)"
	touch extfiles

subprojects/xupaki.pch: subprojects/xupaki/xupaki.pchsource subprojects/xupaki/unpack2.o
	$(RM) -r subprojects/xupaki/a2ext
	mkdir subprojects/xupaki/a2ext
	sh -c 'cd subprojects/xupaki/a2ext && cp ../export.dat ../*.o . && for i in *.o; do $(O2EXT) "$$i"; done'
	sh -c 'cd subprojects/xupaki && $(PCHMK) xupaki.pchsource && rm -f pchlog.txt xupaki.def && mv xupaki.pch ..'
