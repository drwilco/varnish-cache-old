#

TESTS_PARALLELISM = 3
check: varnishtest
	./varnishtest -j$(TESTS_PARALLELISM) $(srcdir)/tests/*.vtc
	@echo "==================="
	@echo "All tests succeeded"
	@echo "==================="

DISTCLEANFILES = _.ok

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib/libvgz

bin_PROGRAMS =	varnishtest

dist_man_MANS = varnishtest.1

varnishtest_SOURCES = \
		vtc.c \
		vtc.h \
		vtc_client.c \
		vtc_http.c \
		vtc_main.c \
		vtc_log.c \
		vtc_sema.c \
		vtc_server.c \
		vtc_varnish.c

varnishtest_LDADD = \
		$(top_builddir)/lib/libvarnish/libvarnish.la \
		$(top_builddir)/lib/libvarnishcompat/libvarnishcompat.la \
		$(top_builddir)/lib/libvarnishapi/libvarnishapi.la \
		$(top_builddir)/lib/libvgz/libvgz.la \
		${LIBM} ${PTHREAD_LIBS}

varnishtest_CFLAGS = \
		-DTOP_BUILDDIR='"${top_builddir}"'

EXTRA_DIST = $(top_srcdir)/bin/varnishtest/tests/*.vtc \
	$(top_srcdir)/bin/varnishtest/tests/README

varnishtest.1: $(top_srcdir)/doc/sphinx/reference/varnishtest.rst
if HAVE_RST2MAN
	${RST2MAN} $? $@
else
	@echo "========================================"
	@echo "You need rst2man installed to make dist"
	@echo "========================================"
	@false
endif
