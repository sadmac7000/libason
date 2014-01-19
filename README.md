# libason #

ASON is an extension of JSON which specifies a semantic, and allows for pattern
expressions that can specify or match groups of JSON values. libason is a
simple library for manipulating ASON programmatically in C.

## Installation ##
libason can be built and installed with the usual commands:

~~~
$ ./configure
$ make
$ make install
~~~

### Configure arguments ###
The `configure` script can take parameters to change the target install path,
like any Automake configuration script. You can use the `--help` option to
print a list of all options. Here are the libason-specific options:

* `--enable-spec` - Build the ASON specification document. Requires the
  `pdflatex` command.

* `--with-asonq` - Build and install the `asonq` utility as well as the
  library.

### Using the asonq utility ###
libason comes with a simple REPL called asonq. It takes no arguments on the
command line.

Typing an ASON value into asonq will reduce it to simplest form and repeat it.
You can assign to variables with the `:=` operator, such as:

	> foo := 6

Type `/quit` to exit asonq.
