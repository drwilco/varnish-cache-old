INCLUDES = -I$(top_srcdir)/include @PCRE_CFLAGS@

pkglib_LTLIBRARIES = libvarnish.la

libvarnish_la_LDFLAGS = -avoid-version

libvarnish_la_SOURCES = \
	argv.c \
	assert.c \
	binary_heap.c \
	subproc.c \
	cli_auth.c \
	cli_common.c \
	cli_serve.c \
	flopen.c \
	num.c \
	vcs_version.c \
	time.c \
	tcp.c \
	vct.c \
	version.c \
	vev.c \
	vin.c \
	vlu.c \
	vmb.c \
	vpf.c \
	vre.c \
	vsb.c \
	vsha256.c \
	vss.c \
	vtmpfile.c

libvarnish_la_CFLAGS = -DVARNISH_STATE_DIR='"${VARNISH_STATE_DIR}"'
libvarnish_la_LIBADD = ${RT_LIBS} ${NET_LIBS} ${LIBM} @PCRE_LIBS@

DISTCLEANFILES = vcs_version.c
vcs_version.c: FORCE
	if [ -d "$(top_srcdir)/.git" ]; then \
		V="$$(git describe --always)" \
		H="$$(head -n 1 vcs_version.c 2>/dev/null || true)"; \
		if [ "/* $$V */" != "$$H" ]; then \
		( \
		  echo "/* $$V */" ;\
		  echo '/*' ;\
		  echo ' * NB:  This file is machine generated, DO NOT EDIT!' ;\
		  echo ' *' ;\
		  echo ' * Run make to regenerate' ;\
		  echo ' *'  ;\
		  echo ' */' ;\
		  echo '' ;\
		  echo "#include <libvarnish.h>" ;\
		  echo "const char* vcs_version(void)" ;\
		  echo "{" ;\
		  echo "	const char* VCS_Version = \"$$V\";" ;\
		  echo "	return VCS_Version;" ;\
		  echo "}" ;\
		) > vcs_version.c ; \
		fi \
	fi
FORCE:

if ENABLE_TESTS
TESTS = num_c_test

noinst_PROGRAMS = ${TESTS}

num_c_test_SOURCES = num.c
num_c_test_CFLAGS = -DNUM_C_TEST -include config.h
num_c_test_LDADD = ${LIBM}

test: ${TESTS}
	@for test in ${TESTS} ; do ./$${test} ; done
endif
