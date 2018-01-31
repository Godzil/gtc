include ../../common.mk
CFLAGS = $(DEFAULT_CFLAGS)

SRC = dat89y.c bin2calc.c

all: dat89y

dat89y: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@

include ../../platform.mk
clean:
distclean scratchclean:
	$(RM) dat89y$(BIN_SUFFIX)
