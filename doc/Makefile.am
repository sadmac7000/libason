pdflatex = $(pdflatex_$(V))$(PDFLATEX)
pdflatex_ = $(pdflatex_$(AM_DEFAULT_VERBOSITY))
pdflatex_0 = @echo "  PDFLATEX   " $@;

if INSTALL_SPEC
dist_doc_DATA = ason_spec.pdf
MOSTLYCLEANFILES = ason_spec.pdf ason_spec.log ason_spec.aux
endif

if BUILD_ASONQ
dist_man1_MANS = asonq.1
endif

dist_man3_MANS =		\
	ason_asprint.3		\
	ason_destroy.3		\
	ason_iterators.3	\
	ason_read.3		\
	ason_copy.3		\
	ason_inspect.3		\
	ason_values.3

ason_spec.aux: ason_spec.tex
	$(pdflatex) -halt-on-error -draftmode $< | grep '^!' && exit 1 || exit 0

ason_spec.pdf: ason_spec.tex ason_spec.aux
	$(pdflatex) -halt-on-error $< | grep '^!' && exit 1 || exit 0

install-exec-hook:
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_inspect.3 $(DESTDIR)$(mandir)/man3/ason_check_represented_in.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_inspect.3 $(DESTDIR)$(mandir)/man3/ason_check_equal.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_inspect.3 $(DESTDIR)$(mandir)/man3/ason_type.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_inspect.3 $(DESTDIR)$(mandir)/man3/ason_long.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_inspect.3 $(DESTDIR)$(mandir)/man3/ason_double.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_inspect.3 $(DESTDIR)$(mandir)/man3/ason_string.3

	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_asprint.3 $(DESTDIR)$(mandir)/man3/ason_asprint_unicode.3
	
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_iterators.3 $(DESTDIR)$(mandir)/man3/ason_iterate.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_iterators.3 $(DESTDIR)$(mandir)/man3/ason_iter_enter.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_iterators.3 $(DESTDIR)$(mandir)/man3/ason_iter_exit.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_iterators.3 $(DESTDIR)$(mandir)/man3/ason_iter_next.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_iterators.3 $(DESTDIR)$(mandir)/man3/ason_iter_prev.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_iterators.3 $(DESTDIR)$(mandir)/man3/ason_iter_long.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_iterators.3 $(DESTDIR)$(mandir)/man3/ason_iter_double.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_iterators.3 $(DESTDIR)$(mandir)/man3/ason_iter_string.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_iterators.3 $(DESTDIR)$(mandir)/man3/ason_iter_value.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_iterators.3 $(DESTDIR)$(mandir)/man3/ason_iter_key.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_iterators.3 $(DESTDIR)$(mandir)/man3/ason_iter_destroy.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_iterators.3 $(DESTDIR)$(mandir)/man3/ason_iter_type.3
	
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_read.3 $(DESTDIR)$(mandir)/man3/ason_readn.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_read.3 $(DESTDIR)$(mandir)/man3/ason_ns_read.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_read.3 $(DESTDIR)$(mandir)/man3/ason_ns_readn.3
