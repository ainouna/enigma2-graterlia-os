SUBDIRS = countries fonts extensions keymaps skin_default rc_models

dist_pkgdata_DATA = \
	encoding.conf \
	keymap.xml \
	menu.xml \
        menu_d.xml \
        menu_nd.xml \
	prev.png \
	radio.mvi \
	black.mvi \
	rcpositions.xml \
	setup.xml \
	skin_default.xml \
	skin_subtitles.xml \
	skin.xml \
	startwizard.xml \
	freesat.t1 \
	freesat.t2 \
	timezone.xml \
	unicable.xml

if HAVE_TEXTLCD
dist_pkgdata_DATA += skin_text.xml
else
dist_pkgdata_DATA += skin_display.xml
endif

skin_display.xml: $(srcdir)/skin_display_default.xml
	test -f $(srcdir)/skin_display_$(BOXTYPE).xml && cp $(srcdir)/skin_display_$(BOXTYPE).xml skin_display.xml || cp $(srcdir)/skin_display_default.xml skin_display.xml

skin_text.xml: $(srcdir)/skin_text_default.xml
	test -f $(srcdir)/skin_text_$(BOXTYPE).xml && cp $(srcdir)/skin_text_$(BOXTYPE).xml skin_text.xml || cp $(srcdir)/skin_text_default.xml skin_text.xml

