lemon = $(lemon_$(V))$(LEMON)
lemon_ = $(lemon_$(AM_DEFAULT_VERBOSITY))
lemon_0 = @echo "  LEMON   " $@;

AM_CFLAGS = --std=gnu99 -Wall -Wextra -D_GNU_SOURCE -fvisibility=hidden
lib_LTLIBRARIES = libason.la
libason_la_SOURCES = \
	value.c \
	output.c \
	stringfunc.c \
	namespace.c \
	namespace_ram.c \
	parse.c
MOSTLYCLEANFILES = parse.c parse.h parse.out *.gcda *.gcno *.gcov

if BUILD_ASONQ
bin_PROGRAMS = asonq
asonq_SOURCES = asonq.c
asonq_LDADD = libason.la
asonq_LDFLAGS = $(readline_LIBS)
endif

%.h %.c: %.y
	$(lemon) $<
