# This is so I can do either an autoconf build or a local one.
ifneq ($(wildcard Makefile),)
include Makefile

# Doesn't exist in autoconf makefiles.
realclean:
	$(MAKE) -f Makefile.rdb realclean
else
include Makefile.rdb
endif

