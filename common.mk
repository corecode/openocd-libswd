
# common flags used in openocd build
AM_CPPFLAGS = -I$(top_srcdir)/src \
			  -I$(top_builddir)/src \
			  -DPKGDATADIR=\"$(pkgdatadir)\"

if INTERNAL_JIMTCL
AM_CPPFLAGS += -I$(top_srcdir)/jimtcl \
			   -I$(top_builddir)/jimtcl
endif

if INTERNAL_LIBSWD
AM_CPPFLAGS += -I$(top_srcdir)/submodules/libswd/src \
			-I$(top_builddir)/submodules/libswd/src
endif

