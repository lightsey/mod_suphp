if COND_APUSERGROUP
  DEFINE_USERGROUP = -DSUPHP_USE_USERGROUP
endif

AM_CFLAGS=-I@APXS_INCLUDEDIR@ @APXS_EXTRA_CFLAGS@
AM_CFLAGS+=-DSUPHP_PATH_TO_SUPHP=\"${sbindir}/suphp\" $(DEFINE_USERGROUP)

noinst_LTLIBRARIES = mod_suphp.la
mod_suphp_la_SOURCES = mod_suphp.c

mod_suphp_la_LDFLAGS = -module -rpath '@APXS_LIBEXECDIR@'

install-data-local:
	${INSTALL_SCRIPT} -d ${DESTDIR}'@APXS_LIBEXECDIR@'
	${INSTALL_SCRIPT} -m 0755 .libs/mod_suphp.so ${DESTDIR}'@APXS_LIBEXECDIR@'/mod_suphp.so
