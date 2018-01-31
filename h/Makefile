BIN=../bin
STRIP = perl script/strip.pl
TXT89 = $(BIN)/txt89t

all: src/*.h
	$(RM) *.89t
	$(RM) processed/*.89t
	$(RM) -r stripped
	mkdir stripped
	sh -c 'for f in src/*.h; do $(STRIP) "$$f" > "stripped/$${f#src/}"; done'
	$(TXT89) --compact -f gtchdr stripped/*.h
	mv *.89t processed
	$(RM) -r stripped
	touch all

clean:
distclean: clean
scratchclean: distclean
	$(RM) processed/*.89t all

include ../config.mk
install: all
	mkdir -p $(prefix)/share/gtc/include
	install -m 644 src/*.h $(prefix)/share/gtc/include
