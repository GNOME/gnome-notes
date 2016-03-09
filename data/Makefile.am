
SUBDIRS = icons

@APPSTREAM_XML_RULES@

xml_in_files = org.gnome.bijiben.xml.in
xml_files = $(xml_in_files:.xml.in=.xml)

app_data_in_files = org.gnome.bijiben.appdata.xml.in
app_data_files = $(app_data_in_files:.xml.in=.xml)

appstream_XML = $(app_data_files)

check-local: org.gnome.bijiben.desktop
	$(DESKTOP_FILE_VALIDATE) org.gnome.bijiben.desktop



@INTLTOOL_XML_RULE@

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = org.gnome.bijiben.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

desktopdir   = $(datadir)/applications
desktop_DATA = $(desktop_files)

appdatadir = $(datadir)/appdata
appdata_DATA = $(app_data_files)


servicedir = $(datadir)/dbus-1/services
service_DATA = $(service_in_files:.service.in=.service)
service_in_files = 				\
	org.gnome.bijiben.SearchProvider.service.in

mimedir = $(datadir)/mime/packages
mime_DATA = $(xml_files)

org.gnome.bijiben.SearchProvider.service: org.gnome.bijiben.SearchProvider.service.in Makefile
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

searchproviderdir = $(datadir)/gnome-shell/search-providers
searchprovider_DATA = org.gnome.bijiben-search-provider.ini

web_files = Default.css bijiben.js

webdir = $(pkgdatadir)
web_DATA = $(web_files)

gsettings_SCHEMAS = org.gnome.bijiben.gschema.xml
@GSETTINGS_RULES@

EXTRA_DIST =                            \
	$(desktop_in_files)     	\
	$(web_files)			\
	$(gsettings_SCHEMAS) \
	shell-search-provider-dbus-interfaces.xml \
	$(service_in_files)		\
	$(searchprovider_DATA) \
	$(xml_in_files) \
	$(app_data_in_files)

DISTCLEANFILES =                        \
	$(desktop_files)                \
	$(service_DATA)   \
	$(xml_files) \
	$(app_data_files)

install-data-hook:
if ENABLE_UPDATE_MIMEDB
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"
endif

uninstall-hook:
if ENABLE_UPDATE_MIMEDB
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"
endif

-include $(top_srcdir)/git.mk
