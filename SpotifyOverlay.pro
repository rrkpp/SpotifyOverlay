QT       += core gui network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET          = SpotifyOverlay
TEMPLATE        = app

SOURCES         += main.cpp\
                spotifyoverlay.cpp \
                openvroverlaycontroller.cpp

HEADERS         += spotifyoverlay.h \
                openvr.h \
                openvroverlaycontroller.h

FORMS           += spotifyoverlay.ui

LIBS            += -L$$PWD/lib/win64 -lopenvr_api \
                -luser32

RESOURCES       += res.qrc

win32:RC_ICONS  += img/spotify_overlay.ico

VERSION = 1.0
