all: all-obj2ti all-pchmaker all-dat89y all-txt89t
all-%:
	$(MAKE) -C $(patsubst all-%,%,$@) all
clean distclean scratchclean:
	$(MAKE) -C obj2ti $@
	$(MAKE) -C pchmaker $@
	$(MAKE) -C dat89y $@
	$(MAKE) -C txt89t $@
