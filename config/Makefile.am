y2pmbuildconfdists_DATA = 9.3 9.2 9.1 9.0 8.2
y2pmbuildconfdistsdir = $(sysconfdir)/y2pmbuild/dists

bin_SCRIPTS = sudoy2pmbuild

install-exec-local:
	for i in 9.3 9.2 9.1 9.0 8.2; do \
		rm -f $(DESTDIR)$(bindir)/y2pmbuild-$$i; \
		ln -s sudoy2pmbuild $(DESTDIR)$(bindir)/y2pmbuild-$$i; \
	done

EXTRA_DIST = $(bin_SCRIPTS) $(y2pmbuildconfdists_DATA)
