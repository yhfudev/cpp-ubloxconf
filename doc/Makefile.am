if ENABLE_DOXYGEN_DOC

EXTRA_DIST=genfooter.tex genheader.tex

all-local: doxygen.stamp

DOCSOURCES := Doxyfile.in $(top_srcdir)/README.md \
    $(wildcard $(top_srcdir)/src/*.c) \
    $(wildcard $(top_srcdir)/src/*.cc) \
    $(wildcard $(top_srcdir)/src/*.cpp) \
    $(wildcard $(top_srcdir)/src/*.h) \
    $(wildcard $(top_srcdir)/tools/*.c) \
    $(wildcard $(top_srcdir)/tools/*.cc) \
    $(wildcard $(top_srcdir)/tools/*.cpp) \
    $(wildcard $(top_srcdir)/tools/*.h) \
    $(wildcard $(top_srcdir)/tests/*.c) \
    $(wildcard $(top_srcdir)/tests/*.cc) \
    $(wildcard $(top_srcdir)/tests/*.cpp) \
    $(wildcard $(top_srcdir)/tests/*.h) \
    $(NULL)

doxygen.stamp: $(DOCSOURCES)
	$(DOXYGEN)
	touch doxygen.stamp

install-data-local: install-html

install-html:
	$(INSTALL) -d $(DESTDIR)$(htmldocdir)
	$(INSTALL_DATA) html/* $(DESTDIR)$(htmldocdir)

uninstall-local:
	rm -rf $(DESTDIR)$(htmldocdir)

clean-local:
	rm -rf html man latex
	rm -f doxygen.stamp

else

install-data-local:
uninstall-local:
clean-local:

endif

