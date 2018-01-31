include ../../common-gtc.mk

SRC = pchmaker.c spack.c

all: pchmaker
pchmaker: $(SRC) $(GTC_HEADERS)
	$(CC) $(CFLAGS) $(SRC) -o $@

include ../../platform.mk
clean:
distclean scratchclean:
	$(RM) pchmaker$(BIN_SUFFIX)
