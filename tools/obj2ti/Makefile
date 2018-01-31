include ../../common.mk
CXXFLAGS = $(DEFAULT_CFLAGS)

all: obj2ti

obj2ti: obj2ti.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

include ../../platform.mk
clean:
distclean scratchclean:
	$(RM) obj2ti$(BIN_SUFFIX)
