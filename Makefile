
OLDPORTNAME	= httping
PORTNAME	= multihttping
PORTVERSION = 2.3.4
PORTDIR		= /usr/ports/net/multihttping
SRCDIR		= src

.PHONY: port

build:
	$(MAKE) -C $(SRCDIR)

build-clean:
	$(MAKE) -C $(SRCDIR) clean

build-distclean:
	$(MAKE) -C $(SRCDIR) distclean

port:
	@echo Recreating port folder...
	rm -rf $(PORTDIR)
	cp -r port $(PORTDIR)
	@echo
	@echo Fetching and extracting original port...
	cd $(PORTDIR); make fetch extract
	@echo
	@echo Renaming original files to *.orig
	@for f in `find $(PORTDIR)/work/ -type f ! -name *.orig`; do \
		echo mv $$f $$f.orig; \
		mv $$f $$f.orig; \
	done;
	@echo
	@echo Cleaning project files...
	gmake -C $(SRCDIR) distclean
	@echo
	@echo Copying all project files to the port directory...
	cp $(SRCDIR)/* $(PORTDIR)/work/$(PORTNAME)-$(PORTVERSION)
	@echo
	@echo Generating patches...
	make -C $(PORTDIR) makepatch
	@echo
	@echo Adding new files...
	@for f in `find $(PORTDIR)/work/ -type f ! -name *.orig`; do \
		if [ ! -f "$$f.orig" ]; then \
			echo cp $$f $(PORTDIR)/files; \
			cp $$f $(PORTDIR)/files; \
		fi; \
	done;

