noinst_LTLIBRARIES = \
	libbiji.la

libbiji_la_LIBADD = $(NULL)


AM_CPPFLAGS = \
	-DDATADIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-I$(top_srcdir)/libgd \
	-I$(top_srcdir)/src/ \
	-I$(top_srcdir)/src/libiji \
	$(BIJIBEN_CFLAGS) \
	$(ZEITGEIST_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g


if BUILD_ZEITGEIST
AM_CPPFLAGS += -DBUILD_ZEITGEIST
endif

libbiji_la_built_sources = \
	biji-marshalers.c \
	biji-marshalers.h \
	$(NULL)

libbiji_la_SOURCES =  \
	$(libbiji_la_built_sources) \
	libbiji.h \
	biji-date-time.c \
	biji-date-time.h \
	biji-error.c \
	biji-error.h \
	biji-info-set.c \
	biji-info-set.h \
	biji-item.c \
	biji-item.h \
	biji-manager.c \
	biji-manager.h \
	biji-notebook.c \
	biji-notebook.h \
	biji-note-id.c \
	biji-note-id.h \
	biji-note-obj.c \
	biji-note-obj.h \
	biji-string.c \
	biji-string.h \
	biji-timeout.c \
	biji-timeout.h \
	biji-tracker.c \
	biji-tracker.h \
	biji-zeitgeist.c \
	biji-zeitgeist.h \
	deserializer/biji-lazy-deserializer.c \
	deserializer/biji-lazy-deserializer.h \
	deserializer/biji-tomboy-reader.c \
	deserializer/biji-tomboy-reader.h \
	editor/biji-editor-selection.c \
	editor/biji-editor-selection.h \
	editor/biji-webkit-editor.c \
	editor/biji-webkit-editor.h \
	provider/biji-import-provider.c \
	provider/biji-import-provider.h \
	provider/biji-local-note.c \
	provider/biji-local-note.h \
	provider/biji-local-provider.c \
	provider/biji-local-provider.h \
	provider/biji-memo-note.c \
	provider/biji-memo-note.h \
	provider/biji-memo-provider.c \
	provider/biji-memo-provider.h \
	provider/biji-own-cloud-note.c \
	provider/biji-own-cloud-note.h \
	provider/biji-own-cloud-provider.c \
	provider/biji-own-cloud-provider.h \
	provider/biji-provider.c \
	provider/biji-provider.h \
	serializer/biji-lazy-serializer.c \
	serializer/biji-lazy-serializer.h




BUILT_SOURCES = \
	$(libbiji_la_built_sources) \
	$(NULL)

EXTRA_DIST = \
	biji-marshalers.list \
	$(NULL)

CLEANFILES = \
	$(libbiji_la_built_sources) \
	stamp-biji-marshalers.h \
	$(NULL)



if BUILD_ZEITGEIST
  libbiji_la_LIBADD += \
  $(ZEITGEIST_LIBS) \
  $(NULL)
endif



biji-marshalers.h: stamp-biji-marshalers.h
	@true

stamp-biji-marshalers.h: biji-marshalers.list Makefile
	$(AM_V_GEN) ( \
		( glib-genmarshal \
			--prefix=_biji_marshal \
			--header $(srcdir)/biji-marshalers.list \
		) >> xgen-tmh \
		&& ( cmp -s xgen-tmh biji-marshalers.h || cp xgen-tmh biji-marshalers.h ) \
		&& rm -f xgen-tmh \
		&& echo timestamp > $(@F) \
	)

biji-marshalers.c: biji-marshalers.list biji-marshalers.h Makefile
	$(AM_V_GEN) ( \
		echo "#include \"biji-marshalers.h\"" > xgen-tmc \
		&& ( glib-genmarshal \
			--prefix=_biji_marshal \
			--body $(srcdir)/biji-marshalers.list \
		   ) >> xgen-tmc \
		&& cp xgen-tmc biji-marshalers.c \
		&& rm -f xgen-tmc \
	)

-include $(top_srcdir)/git.mk
