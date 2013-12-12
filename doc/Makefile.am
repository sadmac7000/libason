pdflatex = $(pdflatex_$(V))$(PDFLATEX)
pdflatex_ = $(pdflatex_$(AM_DEFAULT_VERBOSITY))
pdflatex_0 = @echo "  PDFLATEX   " $@;

if INSTALL_SPEC
dist_doc_DATA = ason_spec.pdf
MOSTLYCLEANFILES = ason_spec.pdf ason_spec.log ason_spec.aux
endif

if BUILD_ASONQ
man1_MANS = asonq.1
endif

ason_spec.pdf: ason_spec.tex
	$(pdflatex) $< >/dev/null
