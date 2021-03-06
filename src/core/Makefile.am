VERSION_INFO = 1:0:0

include_HEADERS = \
	remote-control.h

lib_LTLIBRARIES = \
	libremote-control.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = remote-control.pc

libremote_control_la_CFLAGS = \
	-DSYSCONF_DIR=\"$(sysconfdir)\" \
	-I$(top_srcdir)/src/common \
	@X11_CFLAGS@ \
	@LIBNL_CFLAGS@ \
	@GUDEV_CFLAGS@ \
	@GLIB_CFLAGS@ \
	@GTK_CFLAGS@ \
	@WATCHDOG_CFLAGS@

libremote_control_la_LDFLAGS = -version-info $(VERSION_INFO)

libremote_control_la_LIBADD = \
	../common/libcommon.la \
	@X11_CFLAGS@ \
	@LIBNL_LIBS@ \
	@GUDEV_LIBS@ \
	@GLIB_LIBS@ \
	@GTK_LIBS@ \
	@WATCHDOG_LIBS@

libremote_control_la_SOURCES = \
	cursor-movement.c \
	event-manager.c \
	net-udp.c \
	remote-control.c \
	remote-control.h \
	task-manager.c \
	usb-handset.c \
	utils.c

if ENABLE_LLDPCTL
libremote_control_la_CFLAGS += @LLDPCTL_CFLAGS@
libremote_control_la_LIBADD += @LLDPCTL_LIBS@
libremote_control_la_SOURCES += lldp-monitor-lldpctl.c
else
libremote_control_la_SOURCES += lldp-monitor.c
endif

if ENABLE_AUDIO_ALSA
libremote_control_la_CFLAGS += @ALSA_CFLAGS@
libremote_control_la_LIBADD += @ALSA_LIBS@
libremote_control_la_SOURCES += audio-alsa.c
else
if ENABLE_AUDIO_PULSE
libremote_control_la_CFLAGS += @PULSE_CFLAGS@
libremote_control_la_LIBADD += @PULSE_LIBS@
libremote_control_la_SOURCES += audio-pulse.c
else
libremote_control_la_SOURCES += audio-null.c
endif # ENABLE_AUDIO_PULSE
endif #ENABLE_AUDIO_ALSA

if ENABLE_MIXER_ALSA
libremote_control_la_CFLAGS += @ALSA_CFLAGS@
libremote_control_la_SOURCES += mixer-alsa.c
libremote_control_la_LIBADD += @ALSA_LIBS@
else
libremote_control_la_SOURCES += mixer-null.c
endif

if ENABLE_BACKLIGHT_DPMS
libremote_control_la_SOURCES += backlight-dpms.c
else
if ENABLE_BACKLIGHT_MEDATOM
libremote_control_la_SOURCES += backlight-medatom.c smbus.h
else
if ENABLE_BACKLIGHT_SYSFS
libremote_control_la_SOURCES += backlight-sysfs.c
else
libremote_control_la_SOURCES += backlight-null.c
endif # ENABLE_BACKLIGHT_SYSFS
endif # ENABLE_BACKLIGHT_MEDATOM
endif # ENABLE_BACKLIGHT_DPMS

if ENABLE_LINPHONE
libremote_control_la_CFLAGS += \
	@LINPHONE_CFLAGS@

libremote_control_la_LIBADD += \
	@LINPHONE_LIBS@

libremote_control_la_SOURCES += \
	voip-linphone.c
else
libremote_control_la_SOURCES += \
	voip-null.c
endif # ENABLE_LINPHONE

if ENABLE_LIBVLC
libremote_control_la_CFLAGS += \
	@LIBVLC_CFLAGS@

libremote_control_la_LIBADD += \
	@LIBVLC_LIBS@

libremote_control_la_SOURCES += \
	media-player-gtk-vlc.c \
	sound-manager-vlc.c
else
if ENABLE_GST
libremote_control_la_CFLAGS += \
	@GST_CFLAGS@ \
	@XRANDR_CFLAGS@

libremote_control_la_LIBADD += \
	@GST_LIBS@ \
	@XRANDR_LIBS@

libremote_control_la_SOURCES += \
	media-player-gtk-gst.c \
	sound-manager-gst.c
else
libremote_control_la_SOURCES += \
	media-player-null.c \
	sound-manager-null.c
endif # ENABLE_GST
endif # ENABLE_LIBVLC

if ENABLE_LIBSMARTCARD
libremote_control_la_CFLAGS += @LIBSMARTCARD_CFLAGS@
libremote_control_la_LIBADD += @LIBSMARTCARD_LIBS@
libremote_control_la_SOURCES += smartcard-i2c.c
endif # ENABLE_LIBSMARTCARD

if ENABLE_LIBPCSCLITE
libremote_control_la_CFLAGS += @LIBPCSCLITE_CFLAGS@
libremote_control_la_LIBADD += @LIBPCSCLITE_LIBS@
libremote_control_la_SOURCES += smartcard-pcsc.c
endif # ENABLE_LIBPCSCLITE

libremote_control_la_SOURCES += smartcard-nxp.c
libremote_control_la_SOURCES += smartcard-generic.c

if ENABLE_SMARTCARD_INFO
libremote_control_la_SOURCES += smartcard-info.c
libremote_control_la_CFLAGS += @ZLIB_CFLAGS@
libremote_control_la_LIBADD += @ZLIB_LIBS@
libremote_control_la_CFLAGS += @LIBXML2_CFLAGS@
libremote_control_la_LIBADD += @LIBXML2_LIBS@
else
libremote_control_la_SOURCES += smartcard-info-null.c
endif # ENABLE_SMARTCARD_INFO

if ENABLE_LIBMODEM
libremote_control_la_CFLAGS += @LIBMODEM_CFLAGS@
libremote_control_la_LIBADD += @LIBMODEM_LIBS@
libremote_control_la_SOURCES += modem-manager-libmodem.c
else
libremote_control_la_SOURCES += modem-manager-null.c
endif # ENABLE_LIBMODEM

if ENABLE_LIBPTIP
libremote_control_la_CFLAGS += @LIBPTIP_CFLAGS@
libremote_control_la_LIBADD += @LIBPTIP_LIBS@
libremote_control_la_SOURCES += handset-ptip.c
else
libremote_control_la_SOURCES += handset-null.c
endif # ENABLE_LIBPTIP

if ENABLE_GPIO_GPIODEV
libremote_control_la_SOURCES += gpio-gpiodev.c
else
if ENABLE_GPIO_SYSFS
libremote_control_la_CFLAGS += @LIBGPIO_CFLAGS@
libremote_control_la_LIBADD += @LIBGPIO_LIBS@
libremote_control_la_SOURCES += gpio-sysfs.c
else
libremote_control_la_SOURCES += gpio-null.c
endif # ENABLE_GPIO_SYSFS
endif # ENABLE_GPIO_GPIODEV

if ENABLE_WATCHDOG
libremote_control_la_SOURCES += app-watchdog-dbus.c
else
libremote_control_la_SOURCES += app-watchdog-null.c
endif # ENABLE_WATCHDOG

libremote_control_la_SOURCES += gtkosk-dbus.c

gtkosk-dbus.c: gtkosk-dbus.h
	touch $@

gtkosk-dbus.h:
	gdbus-codegen \
		--generate-c-code gtkosk-dbus \
		--c-namespace GtkOsk \
		--interface-prefix com.avionicdesign.gtkosk. \
		@GTKOSK_DATADIR@/com.avionicdesign.gtkosk.control.xml

BUILT_SOURCES = \
	gtkosk-dbus.c \
	gtkosk-dbus.h

CLEANFILES = \
	$(BUILT_SOURCES)
