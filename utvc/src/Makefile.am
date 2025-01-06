include $(top_srcdir)/build/vars.build.mk
plugin = utvc

geanyplugins_LTLIBRARIES = utvc.la

utvc_la_SOURCES = utvc.c
utvc_la_CPPFLAGS = $(AM_CPPFLAGS) -DG_LOG_DOMAIN=\"utvc\"
utvc_la_LIBADD = $(COMMONLIBS)


include $(top_srcdir)/build/cppcheck.mk
