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
	ason_values.3

ason_spec.aux: ason_spec.tex
	$(pdflatex) -draftmode $< >/dev/null

ason_spec.pdf: ason_spec.tex ason_spec.aux
	$(pdflatex) $< >/dev/null

install-exec-hook:
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_create_number.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_create_list.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_append_lists.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_create_object.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_create_string.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_union.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_intersect.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_join.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_complement.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_representation_in.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_equality.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_create_list_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_append_lists_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_create_object_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_union_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_intersect_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_join_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_complement_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_representation_in_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_equality_d.3
	
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
