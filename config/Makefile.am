y2pmbuildconf_DATA = dists_common
y2pmbuildconfdir = $(sysconfdir)/y2pmbuild

y2pmbuildconfdists_DATA = 9.3 9.2 9.1 9.0 8.2
y2pmbuildconfdistsdir = $(y2pmbuildconfdir)/dists

y2pmbuildconffilesdir = $(y2pmbuildconfdir)/files

bin_SCRIPTS = sudoy2pmbuild

install-exec-local:
	for dist in $(y2pmbuildconfdists_DATA); do \
		rm -f $(DESTDIR)$(bindir)/y2pmbuild-$$dist; \
		ln -s sudoy2pmbuild $(DESTDIR)$(bindir)/y2pmbuild-$$dist; \
		mkdir -p $(DESTDIR)$(y2pmbuildconffilesdir)/$$dist/etc/rpm; \
		mkdir -p $(DESTDIR)$(y2pmbuildconfdistsdir); \
		for arch in i386 x86_64; do \
			file=$(DESTDIR)$(y2pmbuildconfdistsdir)/$$dist-$$arch; \
			rm -f $$file; \
			ln -s $$dist $$file; \
			file=$(DESTDIR)$(y2pmbuildconffilesdir)/$$dist-$$arch; \
			rm -f $$file; \
			ln -s $$dist $$file; \
			echo "%distribution SuSE Linux $$dist (%_arch)" > $(DESTDIR)$(y2pmbuildconffilesdir)/$$dist/etc/rpm/macros; \
		done; \
	done

EXTRA_DIST = $(bin_SCRIPTS) $(y2pmbuildconf_DATA) $(y2pmbuildconfdists_DATA)
