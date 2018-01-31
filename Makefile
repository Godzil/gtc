SUBSYSTEMS = tools bin h pch gtc ti68k
INSTALLABLE_SUBSYSTEMS = h pch gtc
all: $(addprefix all-,$(SUBSYSTEMS))
	@echo 1>&2
	@echo 1>&2 '-- Compilation successful!'
	@echo 1>&2 '   You may use the calculator binaries now (e.g. for the TI-89 in ti68k/bin-89).'
	@echo 1>&2 '   Or if you want to go ahead and install the cross-compiler, just type "make install".'
	@echo 1>&2
install: $(addprefix all-,$(SUBSYSTEMS)) $(addprefix install-,$(INSTALLABLE_SUBSYSTEMS))
	@echo 1>&2
	@echo 1>&2 '-- Cross-compiler install successful!'
	@echo 1>&2 '   You should now be able to compile by typing "gtc myprogram.c".'
	@echo 1>&2 '   Remember that the calculator binaries are available, e.g. for the TI-89 in ti68k/bin-89.'
	@echo 1>&2
clean: $(addprefix clean-,$(SUBSYSTEMS))
distclean: $(addprefix distclean-,$(SUBSYSTEMS))
scratchclean: $(addprefix scratchclean-,$(SUBSYSTEMS))
distclean scratchclean:
	$(RM) config.mk

all-bin: all-tools
all-h: all-bin
all-pch: all-bin
all-gtc: all-bin
all-ti68k: all-bin all-h all-pch
$(addprefix install-,$(INSTALLABLE_SUBSYSTEMS)): $(addprefix all-,$(INSTALLABLE_SUBSYSTEMS))

all-%:
	$(MAKE) -C $(patsubst all-%,%,$@) all
install-%:
	$(MAKE) -C $(patsubst install-%,%,$@) install
clean-%:
	$(MAKE) -C $(patsubst clean-%,%,$@) clean
distclean-%:
	$(MAKE) -C $(patsubst distclean-%,%,$@) distclean
scratchclean-%:
	$(MAKE) -C $(patsubst scratchclean-%,%,$@) scratchclean
