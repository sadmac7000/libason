AM_CFLAGS = --std=gnu99 -Wall -Wextra -D_GNU_SOURCE -fvisibility=hidden $(lcov_CFLAGS)

valgrind = $(valgrind_$(V))$(LIBTOOL) --mode=execute $(VALGRIND)
valgrind_ = $(valgrind_$(AM_DEFAULT_VERBOSITY))
valgrind_0 = @echo "  VALGRND " $@;

TESTS = parser_test	  \
	print_object	  \
	iterator_test     \
	crc_test          \
	value_test        \
	ns_test
noinst_PROGRAMS = $(TESTS)

MOSTLYCLEANFILES=*.gcda *.gcno *.gcov *.valgrind

%.valgrind: %
	$(valgrind) --leak-check=full --log-file=$@ ./$^ --raw

.PHONY: valgrind-local
valgrind-local: $(patsubst %,%.valgrind,$(TESTS))

parser_test_SOURCES = parser_test.c harness.c
parser_test_LDADD = ../src/libason.la

print_object_SOURCES = print_object.c harness.c
print_object_LDADD = ../src/libason.la

iterator_test_SOURCES = iterator_test.c harness.c
iterator_test_LDADD = ../src/libason.la

ns_test_SOURCES = ns_test.c harness.c
ns_test_LDADD = ../src/libason.la

value_test_SOURCES = value_test.c harness.c
value_test_LDADD = ../src/libason.la
value_test_CFLAGS = -Wno-uninitialized

crc_test_SOURCES = crc_test.c harness.c \
			 ../src/crc.c ../src/crc.h
