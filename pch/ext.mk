all: $(patsubst %.o,%.ext,$(wildcard *.o))
#	grep fl54.ext G-rayO-n.ref
#	mv -f gray096b7.ext fl54.ext
%.ext: %.o
	$(O2EXT) "$<"
