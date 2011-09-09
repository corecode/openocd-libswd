include $(top_srcdir)/common.mk

METASOURCES = AUTO
noinst_LTLIBRARIES = libtransport.la
libtransport_la_SOURCES = \
	transport.c \
	swd_core.c \
	swd_tcl.c \
	swd_libswd_drv_openocd.c

noinst_HEADERS = \
	transport.h \
	swd.h \
	swd_libswd_drv_openocd.h

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
