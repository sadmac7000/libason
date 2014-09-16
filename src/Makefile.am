GCOV_FLAGS=-b

lemon = $(lemon_$(V))$(LEMON)
lemon_ = $(lemon_$(AM_DEFAULT_VERBOSITY))
lemon_0 = @echo "  LEMON   " $@;

gcov = $(gcov_$(V))$(LIBTOOL) "--mode=execute" $(GCOV) $(GCOV_FLAGS)
gcov_ = $(gcov_$(AM_DEFAULT_VERBOSITY))
gcov_0 = @echo "  GCOV    " $@;

AM_CFLAGS = --std=gnu99 -Wall -Wextra -D_GNU_SOURCE -fvisibility=hidden $(gcov_CFLAGS)
lib_LTLIBRARIES = libason.la
libason_la_SOURCES = \
	value.c \
	output.c \
	iter.c \
	stringfunc.c \
	namespace.c \
	namespace_ram.c \
	blockfile.c \
	btree.c \
	crc.c \
	parse.c
MOSTLYCLEANFILES = parse.c parse.h parse.out *.gcda *.gcno *.gcov *.gcov_report

if BUILD_ASONQ
bin_PROGRAMS = asonq
asonq_SOURCES = asonq.c
asonq_LDADD = libason.la
asonq_LDFLAGS = $(readline_LIBS)
endif

%.h %.c: %.y
	$(lemon) $<

%.gcov %.gcov_report: %.c %.o
	$(gcov) $< > $*.gcov_report

.PHONY: gcov-local
gcov-local: $(libason_la_SOURCES:.c=.gcov) $(asonq_SOURCES:.c=.gcov)
