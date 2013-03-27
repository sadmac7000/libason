lemon = $(lemon_$(V))$(LEMON)
lemon_ = $(lemon_$(AM_DEFAULT_VERBOSITY))
lemon_0 = @echo "  LEMON  " $@;

AM_CFLAGS = --std=gnu99 -Wall -Wextra -D_GNU_SOURCE
lib_LTLIBRARIES = libason.la
libason_la_SOURCES = \
	value.c \
	output.c \
	parse.whole.c
MOSTLYCLEANFILES = parse.whole.c parse.c parse.h parse.out

read.c: parse.h

parse.whole.c: parse.c parse.append.c
	$(AM_V_GEN)cat parse.c parse.append.c > parse.whole.c

.y.h:
	$(lemon) $<

.y.c:
	$(lemon) $<
