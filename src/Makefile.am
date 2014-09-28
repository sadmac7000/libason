lemon = $(lemon_$(V))$(LEMON)
lemon_ = $(lemon_$(AM_DEFAULT_VERBOSITY))
lemon_0 = @echo "  LEMON   " $@;

AM_CFLAGS = --std=gnu99 -Wall -Wextra -D_GNU_SOURCE -fvisibility=hidden $(lcov_CFLAGS)
lib_LTLIBRARIES = libason.la
libason_la_SOURCES = \
	value.c \
	value.h \
	output.c \
	iter.c \
	iter.h \
	stringfunc.c \
	stringfunc.h \
	namespace.c \
	namespace_ram.c \
	crc.c \
	crc.h \
	util.h \
	parse.c \
	parse.h

MOSTLYCLEANFILES = parse.c parse.h parse.out *.gcda *.gcno *.gcov *.gcov_report

if BUILD_ASONQ
bin_PROGRAMS = asonq
asonq_SOURCES = asonq.c
asonq_LDADD = libason.la
asonq_LDFLAGS = $(readline_LIBS)
endif

%.h %.c: %.y
	$(lemon) $<
