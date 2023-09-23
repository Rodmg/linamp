TEMPLATE = app
TARGET = player

QT += network \
      multimedia \
      multimediawidgets \
      widgets

HEADERS = \
    mainwindow.h \
    playerview.h \
    playlistmodel.h \
    playlistview.h \
    scrolltext.h \
    qmediaplaylist.h \
    qmediaplaylist_p.h \
    qplaylistfileparser.h

SOURCES = main.cpp \
    mainwindow.cpp \
    playerview.cpp \
    playlistmodel.cpp \
    playlistview.cpp \
    scrolltext.cpp \
    qmediaplaylist.cpp \
    qmediaplaylist_p.cpp \
    qplaylistfileparser.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/player
INSTALLS += target

FORMS += \
    playerview.ui \
    playlistview.ui

RESOURCES += \
    uiassets.qrc

DISTFILES += \
    scale-skin.sh
