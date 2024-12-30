include $(top_srcdir)/build/vars.build.mk
plugin = tvctreebrowser

geanyplugins_LTLIBRARIES = tvctreebrowser.la

tvctreebrowser_la_SOURCES = tvctreebrowser.c \
							tvctreebrowser.h \
							tvcaddition.c \
							tvcaddition.h \
							bild.h

tvctreebrowser_la_CPPFLAGS = $(AM_CPPFLAGS) -DG_LOG_DOMAIN=\"TVCTreeBrowser\"
tvctreebrowser_la_CFLAGS = $(AM_CFLAGS) $(GIO_CFLAGS) \
	-I$(top_srcdir)/utils/src
tvctreebrowser_la_LIBADD = $(COMMONLIBS) $(GIO_LIBS) \
	$(top_builddir)/utils/src/libgeanypluginutils.la -lm

include $(top_srcdir)/build/cppcheck.mk
