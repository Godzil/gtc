all:
ifndef CALCULATOR
%: make89-% make9x-% makev2-%
	@# dummy command, otherwise make ignores the rule
make89-%:
	$(MAKE) $(patsubst make89-%,%,$@) CALCULATOR=89
make9x-%:
	$(MAKE) $(patsubst make9x-%,%,$@) CALCULATOR=9x
makev2-%:
	$(MAKE) $(patsubst makev2-%,%,$@) CALCULATOR=v2
else
PRETTY_CALCULATOR=$(if $(subst 89,,$(CALCULATOR)),$(if $(subst 9x,,$(CALCULATOR)),v200,92p),89)
DIR = bin-$(PRETTY_CALCULATOR)
MK = mkinfo-$(PRETTY_CALCULATOR)-

all: ide h pch flashapp
ide: $(MK)ide
h: $(MK)h
pch: $(MK)pch
flashapp: $(MK)flashapp

clean:
distclean: clean
scratchclean: distclean
	$(RM) $(MK)*
	$(RM) -r $(DIR)

$(MK)h         : ../h/processed/*
$(MK)pch       : ../pch/*
$(MK)h         : filedir = ../h/processed
$(MK)pch       : filedir = ../pch
$(MK)h         : fileext = t
$(MK)pch       : fileext = y
$(MK)h $(MK)pch: files = $(wildcard $(filedir)/*.89$(fileext))
$(MK)h $(MK)pch:
	mkdir -p $(DIR)
	$(foreach file,$(files),cp $(file) $(patsubst $(filedir)/%.89$(fileext),$(DIR)/%.$(CALCULATOR)$(fileext),$(file));)
	touch $@
$(MK)ide: ide/bin/*
	mkdir -p $(DIR)
	cp ide/bin/gtc-ide.$(CALCULATOR)z $(DIR)
	touch $@

$(MK)flashapp: srcdata/flashapp/*
$(MK)flashapp: tifs_calc = $(if $(subst 89,,$(CALCULATOR)),92p,89)
$(MK)flashapp: tifs_ext = $(if $(subst 89,,$(CALCULATOR)),9xk,89k)
$(MK)flashapp:
	mkdir -p $(DIR)
	cp srcdata/flashapp/gtc-$(tifs_calc).$(tifs_ext) $(DIR)/gtc.$(CALCULATOR)k
	touch $@
endif
