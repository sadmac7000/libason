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

dist_man3_MANS = ason_values.3

ason_spec.pdf: ason_spec.tex
	$(pdflatex) $< >/dev/null

install-exec-hook:
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_create_number.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_create_list.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_create_object.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_create_string.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_union.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_intersect.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_join.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_complement.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_representation_in.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_equality.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_create_list_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_create_object_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_union_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_intersect_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_join_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_complement_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_representation_in_d.3
	$(LN_S) $(DESTDIR)$(mandir)/man3/ason_values.3 $(DESTDIR)$(mandir)/man3/ason_equality_d.3
