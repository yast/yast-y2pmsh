y2pmbuildconf_DATA = dists_common
y2pmbuildconfdir = $(sysconfdir)/y2pmbuild

y2pmbuildconfdists_DATA = 10.1 10.0 9.3 9.2 9.1 9.0 8.2 8.1
y2pmbuildconfdistsdir = $(y2pmbuildconfdir)/dists

y2pmbuildconffilesdir = $(y2pmbuildconfdir)/files

y2pmbuildconfmacrosdir = $(y2pmbuildconfdir)/macros

bin_SCRIPTS = sudoy2pmbuild

EXTRA_DIST = $(bin_SCRIPTS) $(y2pmbuildconf_DATA) $(y2pmbuildconfdists_DATA) macros

install-exec-local:
	for dist in $(y2pmbuildconfdists_DATA); do \
		rm -f $(DESTDIR)$(bindir)/y2pmbuild-$$dist; \
		ln -s sudoy2pmbuild $(DESTDIR)$(bindir)/y2pmbuild-$$dist; \
		mkdir -p $(DESTDIR)$(y2pmbuildconffilesdir)/$$dist/etc/rpm; \
		mkdir -p $(DESTDIR)$(y2pmbuildconfdistsdir); \
		archs=`uname -m`; \
		case $$archs in \
			i?86) archs=i386 ;; \
			x86_64) archs="i386 x86_64" ;; \
			ppc64) archs="ppc ppc64" ;; \
		esac; \
		for arch in $$archs; do \
			file=$(DESTDIR)$(y2pmbuildconfdistsdir)/$$dist-$$arch; \
			rm -f $$file; \
			ln -s $$dist $$file; \
			file=$(DESTDIR)$(y2pmbuildconffilesdir)/$$dist-$$arch; \
			rm -f $$file; \
			ln -s $$dist $$file; \
			mkdir -p $(DESTDIR)$(y2pmbuildconfmacrosdir)/$$dist-$$arch; \
			install -m 644 macros $(DESTDIR)$(y2pmbuildconfmacrosdir)/$$dist-$$arch/macros; \
			ver=$$(($${dist//./}*10));\
			echo "%suse_version $$ver" >> $(DESTDIR)$(y2pmbuildconfmacrosdir)/$$dist-$$arch/macros;\
			cpu=$$arch; \
			case $$cpu in \
				i?86) cpu=i586 ;; \
				axp) cpu=alpha ;; \
			esac; \
			echo "%_target_cpu $$cpu" >> $(DESTDIR)$(y2pmbuildconfmacrosdir)/$$dist-$$arch/macros;\
			echo "%distribution SuSE Linux $$dist (%_arch)" > $(DESTDIR)$(y2pmbuildconffilesdir)/$$dist/etc/rpm/macros; \
		done; \
	done
