TEMPLATE = app
TARGET = player

QT += network \
      multimedia \
      multimediawidgets \
      widgets \
      concurrent \
      dbus

LIBS += -ltag -lasound -lpulse -lpulse-simple -lpipewire-0.3 -L/usr/lib/python3.11/config-3.11-x86_64-linux-gnu/ -lpython3.11
INCLUDEPATH += /usr/include/python3.11 /usr/include/pipewire-0.3 /usr/include/spa-0.2

HEADERS = \
    audiosource.h \
    audiosourcebluetooth.h \
    audiosourcecd.h \
    audiosourcecoordinator.h \
    audiosourcefile.h \
    audiosourcewspectrumcapture.h \
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
    audiosourcecd.cpp \
    audiosourcecoordinator.cpp \
    audiosourcefile.cpp \
    audiosourcewspectrumcapture.cpp \
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
    install.sh \
    python/linamp/__init__.py \
    python/linamp/cdplayer.py \
    python/linamp-mock/__init__.py \
    python/linamp-mock/mock_cdplayer.py \
    python/requirements.txt \
    scale-skin.sh \
    setup.sh \
    shutdown.sh \
    start.sh \
    styles/controlbuttonswidget.shuffleButton.4x.qss
