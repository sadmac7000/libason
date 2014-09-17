lcov = $(lcov_$(V))$(LCOV)
lcov_ = $(lcov_$(AM_DEFAULT_VERBOSITY))
lcov_0 = @echo "  LCOV    " $@;

genhtml = $(genhtml_$(V))$(GENHTML)
genhtml_ = $(genhtml_$(AM_DEFAULT_VERBOSITY))
genhtml_0 = @echo "  GENHTML " $@;

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src include tests doc
EXTRA_DIST=README.md NEWS.md

.PHONY: lcov
lcov: coverage

coverage.info:
	lcov --capture --directory . --output-file $@

coverage: coverage.info
	genhtml $^ --output-dir $@

MOSTLYCLEANFILES = coverage.info

clean-local:
	-rm -rf coverage
