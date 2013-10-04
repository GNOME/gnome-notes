SUBDIRS = \
	libbiji

bin_PROGRAMS = bijiben

libexec_PROGRAMS=				\
	bijiben-shell-search-provider		\
	$(NULL)

AM_CPPFLAGS = \
	-DDATADIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-I$(top_srcdir)/libgd \
	$(BIJIBEN_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g






LDADD = libbiji/libbiji.la \
	-lm \
	$(top_builddir)/libgd/libgd.la \
	$(BIJIBEN_LIBS)

bijiben_LDFLAGS = \
	-Wl,--export-dynamic

resource_files = \
	resources/app-menu.ui \
	resources/thumbnail-frame.png

bjb-resources.c: bjb.gresource.xml $(resource_files)
	glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name bjb $(srcdir)/bjb.gresource.xml

bjb-resources.h: bjb.gresource.xml
	glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name bjb $(srcdir)/bjb.gresource.xml

bijiben_built_sources = \
	bjb-resources.c \
	bjb-resources.h

bijiben_SOURCES = \
	$(bijiben_built_sources) \
	bjb-app-menu.c \
	bjb-app-menu.h \
	bjb-bijiben.c \
	bjb-bijiben.h \
	bjb-color-button.c \
	bjb-color-button.h \
	bjb-controller.c \
	bjb-controller.h \
	bjb-debug.c \
	bjb-debug.h \
	bjb-editor-toolbar.c \
	bjb-editor-toolbar.h \
	bjb-empty-results-box.c \
	bjb-empty-results-box.h \
	bjb-import-dialog.h \
	bjb-import-dialog.c \
	bjb-load-more-button.c \
	bjb-load-more-button.h \
	bjb-main.c \
	bjb-main-toolbar.c \
	bjb-main-toolbar.h \
	bjb-main-view.c \
	bjb-main-view.h \
	bjb-note-tag-dialog.c \
	bjb-note-tag-dialog.h \
	bjb-note-view.c \
	bjb-note-view.h \
	bjb-rename-note.c \
	bjb-rename-note.h \
	bjb-search-toolbar.c \
	bjb-search-toolbar.h \
	bjb-selection-toolbar.c \
	bjb-selection-toolbar.h \
	bjb-settings.c \
	bjb-settings.h \
	bjb-settings-dialog.c \
	bjb-settings-dialog.h \
	bjb-share.c \
	bjb-share.h \
	bjb-window-base.c \
	bjb-window-base.h

dbus_shell_search_provider_built_sources =		\
	bijiben-shell-search-provider-generated.c	\
	bijiben-shell-search-provider-generated.h

$(dbus_shell_search_provider_built_sources) : Makefile.am $(top_srcdir)/data/shell-search-provider-dbus-interfaces.xml
	gdbus-codegen									\
		--interface-prefix org.gnome.						\
		--c-namespace Bijiben							\
		--generate-c-code bijiben-shell-search-provider-generated		\
		$(top_srcdir)/data/shell-search-provider-dbus-interfaces.xml		\
		$(NULL)

bijiben_shell_search_provider_SOURCES = 	\
	bijiben-shell-search-provider.c		\
	$(NULL)

nodist_bijiben_shell_search_provider_SOURCES =		\
	$(dbus_shell_search_provider_built_sources)	\
	$(NULL)

BUILT_SOURCES = \
	$(bijiben_built_sources) \
	$(dbus_shell_search_provider_built_sources) 	\
	$(NULL)

EXTRA_DIST = \
	bjb.gresource.xml \
	resources/app-menu.ui \
	resources/thumbnail-frame.png \
	$(NULL)

CLEANFILES = \
	$(BUILT_SOURCES)	\
	$(NULL)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

