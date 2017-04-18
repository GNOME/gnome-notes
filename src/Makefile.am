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
	-I$(top_builddir)/src/libbiji \
	$(BIJIBEN_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g






LDADD = libbiji/libbiji.la \
	$(LIBM) \
	$(top_builddir)/libgd/libgd.la \
	$(BIJIBEN_LIBS)

bijiben_LDFLAGS = \
	-Wl,--export-dynamic

bjb_gresource_xml_deps = \
	$(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir=$(srcdir) $(srcdir)/bjb.gresource.xml)
bjb-resources.c: bjb.gresource.xml $(bjb_gresource_xml_deps)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source --c-name bjb $<
bjb-resources.h: bjb.gresource.xml $(bjb_gresource_xml_deps)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-header --c-name bjb $<

bijiben_resources_sources = \
	bjb-resources.c \
	bjb-resources.h \
	$(NULL)

bijiben_SOURCES = \
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
	bjb-note-view.c \
	bjb-note-view.h \
	bjb-organize-dialog.c \
	bjb-organize-dialog.h \
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

nodist_bijiben_SOURCES = \
	$(bjb_resources_sources) \
	$(NULL)

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
	$(bijiben_resources_sources) \
	$(dbus_shell_search_provider_built_sources)	\
	$(NULL)

BUILT_SOURCES = \
	$(bijiben_resources_sources) \
	$(dbus_shell_search_provider_built_sources) 	\
	$(NULL)

EXTRA_DIST = \
	bjb.gresource.xml \
	$(bjb_gresource_xml_deps) \
	$(NULL)

CLEANFILES = \
	$(BUILT_SOURCES)	\
	$(NULL)

-include $(top_srcdir)/git.mk
