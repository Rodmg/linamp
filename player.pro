TEMPLATE = app
TARGET = player

QT += network \
      multimedia \
      multimediawidgets \
      widgets \
      concurrent \
      dbus

LIBS += -ltag -lasound -lpulse -lpulse-simple

HEADERS = \
    audiosource.h \
    audiosourcebluetooth.h \
    audiosourcecoordinator.h \
    audiosourcefile.h \
    controlbuttonswidget.h \
    desktopbasewindow.h \
    desktopplayerwindow.h \
    embeddedbasewindow.h \
    fft.h \
    filebrowsericonprovider.h \
    mainmenuview.h \
    mainwindow.h \
    mediaplayer.h \
    playerview.h \
    playlistmodel.h \
    playlistview.h \
    scale.h \
    scrolltext.h \
    qmediaplaylist.h \
    qmediaplaylist_p.h \
    qplaylistfileparser.h \
    spectrumwidget.h \
    systemaudiocontrol.h \
    titlebar.h \
    util.h

SOURCES = main.cpp \
    audiosource.cpp \
    audiosourcebluetooth.cpp \
    audiosourcecoordinator.cpp \
    audiosourcefile.cpp \
    controlbuttonswidget.cpp \
    desktopbasewindow.cpp \
    desktopplayerwindow.cpp \
    embeddedbasewindow.cpp \
    fft.cpp \
    filebrowsericonprovider.cpp \
    mainmenuview.cpp \
    mainwindow.cpp \
    mediaplayer.cpp \
    playerview.cpp \
    playlistmodel.cpp \
    playlistview.cpp \
    scale.cpp \
    scrolltext.cpp \
    qmediaplaylist.cpp \
    qmediaplaylist_p.cpp \
    qplaylistfileparser.cpp \
    spectrumwidget.cpp \
    systemaudiocontrol.cpp \
    titlebar.cpp \
    util.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/player
INSTALLS += target

FORMS += \
    controlbuttonswidget.ui \
    desktopbasewindow.ui \
    desktopplayerwindow.ui \
    embeddedbasewindow.ui \
    mainmenuview.ui \
    playerview.ui \
    playlistview.ui \
    titlebar.ui

RESOURCES += \
    uiassets.qrc

DISTFILES += \
    README.md \
    scale-skin.sh \
    shutdown.sh \
    styles/controlbuttonswidget.shuffleButton.4x.qss
