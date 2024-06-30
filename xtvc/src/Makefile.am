include $(top_srcdir)/build/vars.build.mk
plugin = xtvc
#TINYEXPR_CFLAGS=-Wall -Wshadow -O2
#TINYEXPR_LIBS=/home/tvc/tinyexpr/

geanyplugins_LTLIBRARIES = xtvc.la

xtvc_la_SOURCES = tvc.c tinyexpr.h tinyexpr.c
xtvc_la_CPPFLAGS = $(AM_CPPFLAGS) -DG_LOG_DOMAIN=\"xtvc\"
xtvc_la_LIBADD = $(COMMONLIBS) -lm

# xtvc_la_CFLAGS = $(AM_CFLAGS) -lm   # -I/usr/local/dislin 


include $(top_srcdir)/build/cppcheck.mk
