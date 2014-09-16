AC_INIT([libason], [0.0.1], [casey.dahlin@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability foreign
                  parallel-tests subdir-objects])
AM_SILENT_RULES([yes])
AC_PROG_CC
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
AC_PROG_LN_S

AC_ARG_ENABLE([gcov], [AS_HELP_STRING(
	       [--enable-gcov],
	       [Compile in gcov support])
], [], [enable_gcov=yes])

gcov_CFLAGS=

AS_IF([test "x$enable_gcov" = xyes], [
       gcov_CFLAGS="-fprofile-arcs -ftest-coverage"
       ], [])
AC_ARG_VAR([gcov_CFLAGS], [C compiler flags for gcov])

AC_ARG_WITH([asonq], [AS_HELP_STRING(
	     [--with-asonq],
	 [Build the asonq binary])
], [], [with_asonq=yes])

readline_LIBS=
readline_CFLAGS=

AS_IF([test "x$with_asonq" != xno], [
	AC_CHECK_LIB([readline], [readline], [
		readline_LIBS=-lreadline
	], [
		AC_MSG_FAILURE([--with-asonq requires readline])
	])
])

AC_ARG_VAR([readline_CFLAGS], [C compiler flags for readline])
AC_ARG_VAR([readline_LIBS], [linker flags for readline])

AM_CONDITIONAL([BUILD_ASONQ], [ test "x$with_asonq" != xno ])

AC_PATH_PROG([LEMON], [lemon], [no])
test "x$LEMON" = "xno" && AC_MSG_ERROR([Lemon parser generator not found])

AC_ARG_ENABLE([spec], [AS_HELP_STRING(
	     [--enable-spec],
	     [Build the ASON specification document])
], [], [with_spec=yes])

AC_PATH_PROG([PDFLATEX], [pdflatex], [no])

test "x$PDFLATEX" = "xno" -a "x$with_spec" != xno && \
	AC_MSG_ERROR([PDFLatex not found])

AM_CONDITIONAL([INSTALL_SPEC], [ test "x$with_spec" != xno ])

AC_ARG_VAR([LEMON], [Lemon parser generator command])
AC_ARG_VAR([PDFLATEX], [PDFLatex command])
LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	include/Makefile
	tests/Makefile
	doc/Makefile
])
AC_OUTPUT
