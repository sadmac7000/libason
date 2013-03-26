lemon = $(lemon_$(V))$(LEMON)
lemon_ = $(lemon_$(AM_DEFAULT_VERBOSITY))
lemon_0 = @echo "  LEMON  " $@;

AM_CFLAGS = --std=gnu99 -Wall -Wextra -D_GNU_SOURCE
lib_LTLIBRARIES = libason.la
libason_la_SOURCES = \
	value.c \
	output.c

.y.c:
	$(lemon) $<
