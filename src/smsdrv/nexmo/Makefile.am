DRIVER = nexmo

pkglib_LTLIBRARIES = nexmo.la
nexmo_la_SOURCES = main.cpp
nexmo_la_CPPFLAGS=-I@top_srcdir@/include
nexmo_la_LDFLAGS = -module -avoid-version
nexmo_la_LIBADD = ../../libnetxms/libnetxms.la -lcurl
if USE_INTERNAL_JANSSON
nexmo_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nexmo_la_LIBADD += -ljansson
endif

EXTRA_DIST = \
	nexmo.vcproj

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/smsdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/smsdrv/$(DRIVER).sms
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
	rm -f $(DESTDIR)$(libdir)/libnxsms_$(DRIVER).so*
	ln -s netxms/smsdrv/$(DRIVER).sms $(DESTDIR)$(libdir)/libnxsms_$(DRIVER).so
