include ../../common.mk
CFLAGS = $(DEFAULT_CFLAGS)

SRC = txt89t.c bin2calc.c

all: txt89t

txt89t: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@

include ../../platform.mk
clean:
distclean scratchclean:
	$(RM) txt89t$(BIN_SUFFIX)
