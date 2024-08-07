include $(top_srcdir)/build/vars.build.mk
plugin = tvcprint

geanyplugins_LTLIBRARIES = tvcprint.la

tvcprint_la_SOURCES = \
	tvcprint.c \
	tvcprint.h \
	tvcformatter.c \
	tvcformatter.h \
	dislin.h

tvcprint_la_CPPFLAGS = $(AM_CPPFLAGS) \
					 -DG_LOG_DOMAIN=\"tvcprint\"

tvcprint_la_LIBADD = -ldislin -lm $(COMMONLIBS)

include $(top_srcdir)/build/cppcheck.mk
