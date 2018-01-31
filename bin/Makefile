include ../platform.mk
vpath ../tools

PROGRAMS = obj2ti$(BIN_SUFFIX) pchmaker$(BIN_SUFFIX) txt89t$(BIN_SUFFIX) dat89y$(BIN_SUFFIX)

all: $(PROGRAMS)

clean:
distclean:
	$(RM) $(PROGRAMS)
scratchclean: distclean

obj2ti$(BIN_SUFFIX): ../tools/obj2ti/obj2ti$(BIN_SUFFIX)
	cp $< $@
pchmaker$(BIN_SUFFIX): ../tools/pchmaker/pchmaker$(BIN_SUFFIX)
	cp $< $@
txt89t$(BIN_SUFFIX): ../tools/txt89t/txt89t$(BIN_SUFFIX)
	cp $< $@
dat89y$(BIN_SUFFIX): ../tools/dat89y/dat89y$(BIN_SUFFIX)
	cp $< $@
