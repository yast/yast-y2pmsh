AM_CPPFLAGS=-DUSETECLA=1 -I$(tecladir) -I/usr/include/YaST2

y2pmlibs = -ly2pm -ly2util
teclalibs = $(tecladir)/libtecla_r.a

bin_PROGRAMS = y2pmsh

y2pmsh_SOURCES = \
	cmdlineiface_tecla.cc cmdlineiface_tecla.h \
	testinstall.cc \
	show.cc \
	variable.cc variable.h \
	show_pm.cc show_pm.h \
	build.cc build.h \
	helpscreen.cc helpscreen.h \
	cmdlineiface.cc cmdlineiface.h \
	solverresults.cc solverresults.h \
	instsrc.cc instsrc.h \
	search.cc search.h \
	summary.cc summary.h \
	y2pmsh.cc y2pmsh.h \
	distrocheck.cc distrocheck.h \
	variables.cc variables.h \
	callbacks.cc callbacks.h

y2pmsh_LDADD=$(teclalibs) $(y2pmlibs)

y2pmbuildconfdir = $(sysconfdir)/y2pmbuild
	
intall-data-local:
	install -m 755 -d $(DESTDIR)/var/spool/y2pmbuild

install-exec-local:
	sed "s#^y2pmbuildconfdir=/etc/y2pmbuild#y2pmbuildconfdir=${y2pmbuildconfdir}#" < y2pmbuild > $(DESTDIR)$(bindir)/y2pmbuild
	chmod 755 $(DESTDIR)$(bindir)/y2pmbuild

EXTRA_DIST = y2pmbuild

#noinst_PROGRAMS = y2pmsh_static
#
#y2pmsh_static_SOURCES = $(y2pmsh_SOURCES)
#y2pmsh_static_LDFLAGS = $(teclalibs) -Wl,-Bstatic $(y2pmlibs) -Wl,-Bdynamic
