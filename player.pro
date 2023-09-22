TEMPLATE = app
TARGET = player

QT += network \
      multimedia \
      multimediawidgets \
      widgets

HEADERS = \
    mainwindow.h \
    playlistmodel.h \
    scrolltext.h \
    qmediaplaylist.h \
    qmediaplaylist_p.h \
    qplaylistfileparser.h

SOURCES = main.cpp \
    mainwindow.cpp \
    playlistmodel.cpp \
    scrolltext.cpp \
    qmediaplaylist.cpp \
    qmediaplaylist_p.cpp \
    qplaylistfileparser.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/player
INSTALLS += target

FORMS += \
    mainwindow.ui

RESOURCES += \
    uiassets.qrc

DISTFILES += \
    scale-skin.sh
