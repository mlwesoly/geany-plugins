include $(top_srcdir)/build/vars.build.mk
plugin = xtvc

geanyplugins_LTLIBRARIES = xtvc.la

xtvc_la_SOURCES = tvc.c tinyexpr.h tinyexpr.c
xtvc_la_CPPFLAGS = $(AM_CPPFLAGS) -DG_LOG_DOMAIN=\"xtvc\"
xtvc_la_LIBADD = $(COMMONLIBS) -lm

include $(top_srcdir)/build/cppcheck.mk
