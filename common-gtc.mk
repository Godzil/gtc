# This Makefile is used for source codes that depend on the GTC code tree.
# It is expected that the current working directory is of the form src/xxx/yyy.
#
# Note that GTC itself may need more C defines to be compiled, but they will
# only be required in .c files, not .h files, so they are not defined here.

include ../../common.mk
include ../../config.mk

GTC_SOURCE = ../../gtc/src
GTC_HEADERS = $(GTC_SOURCE)/*.h

CFLAGS = $(DEFAULT_CFLAGS) -I$(GTC_SOURCE) $(GTC_ARCH_DEFINES)
