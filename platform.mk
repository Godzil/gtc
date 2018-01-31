PLATFORM := $(shell uname)
BIN_SUFFIX := $(if $(patsubst CYGWIN%,,$(patsubst MINGW%,,$(PLATFORM))),,.exe)
