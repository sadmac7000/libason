# libason #

ASON is an extension of JSON which specifies a semantic, and allows for pattern
expressions that can specify or match groups of JSON values. libason is a
simple library for manipulating ASON programmatically in C.

## Installation ##

`libason` depends on [GNU Readline](http://cnswww.cns.cwru.edu/php/chet/readline/rltop.html)
if you are going to build the `asonq` utility. Rendering the spec requires a
`LaTeX` distribution that provides the `pdflatex` command. Finally, all builds
of `libason` will require the [Lemon parser
generator](http://www.hwaci.com/sw/lemon/).

As of Fedora 21, the above are provided by the following RPMs in Fedora:
* `readline-devel`
* `texlive-latex-bin-bin`
* `texlive-texconfig`
* `texlive-texconfig-bin`
* `lemon`

`libason` can be built and installed with the usual commands:

~~~
$ ./configure
$ make
$ make install
~~~

### Configure arguments ###
The `configure` script can take parameters to change the target install path,
like any Automake configuration script. You can use the `--help` option to
print a list of all options. Here are the `libason`-specific options:

* `--enable-spec` - Build the ASON specification document. Requires the
  `pdflatex` command.

* `--enable-lcov` - Build library with LCOV support. `make lcov` will build
  a code coverage report.

* `--with-asonq` - Build and install the `asonq` utility as well as the
  library.

### Using the asonq utility ###
`libason` comes with a simple REPL called `asonq`. It takes no arguments on the
command line.

Typing an ASON value into `asonq` will reduce it to simplest form and repeat
it. You can assign to variables with the `:=` operator, such as:

	> foo := 6

Type `/quit` to exit `asonq`.
