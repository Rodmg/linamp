cmake_minimum_required(VERSION 3.16)
project(player VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Concurrent DBus Gui Multimedia MultimediaWidgets Network Widgets)

qt_standard_project_setup()

qt_add_executable(player WIN32 MACOSX_BUNDLE
    audiosource.cpp audiosource.h
    audiosourcebluetooth.cpp audiosourcebluetooth.h
    audiosourcecd.cpp audiosourcecd.h
    audiosourcecoordinator.cpp audiosourcecoordinator.h
    audiosourcefile.cpp audiosourcefile.h
    audiosourcewspectrumcapture.cpp audiosourcewspectrumcapture.h
    controlbuttonswidget.cpp controlbuttonswidget.h controlbuttonswidget.ui
    desktopbasewindow.cpp desktopbasewindow.h desktopbasewindow.ui
    desktopplayerwindow.cpp desktopplayerwindow.h desktopplayerwindow.ui
    embeddedbasewindow.cpp embeddedbasewindow.h embeddedbasewindow.ui
    fft.cpp fft.h
    filebrowsericonprovider.cpp filebrowsericonprovider.h
    main.cpp
    mainmenuview.cpp mainmenuview.h mainmenuview.ui
    mainwindow.cpp mainwindow.h
    mediaplayer.cpp mediaplayer.h
    playerview.cpp playerview.h playerview.ui
    playlistmodel.cpp playlistmodel.h
    playlistview.cpp playlistview.h playlistview.ui
    qmediaplaylist.cpp qmediaplaylist.h qmediaplaylist_p.cpp qmediaplaylist_p.h
    qplaylistfileparser.cpp qplaylistfileparser.h
    scale.cpp scale.h
    scrolltext.cpp scrolltext.h
    spectrumwidget.cpp spectrumwidget.h
    systemaudiocontrol.cpp systemaudiocontrol.h
    titlebar.cpp titlebar.h titlebar.ui
    util.cpp util.h
    uiassets.qrc
)

target_include_directories(player PRIVATE
    /usr/include/pipewire-0.3
    /usr/include/python3.11
    /usr/include/spa-0.2
)

target_link_libraries(player PRIVATE
    # Remove: L/usr/lib/python3.11/config-3.11-x86_64-linux-gnu/
    Qt::Concurrent
    Qt::Core
    Qt::DBus
    Qt::Gui
    Qt::Multimedia
    Qt::MultimediaWidgets
    Qt::Network
    Qt::Widgets
    asound
    pipewire-0.3
    pulse
    pulse-simple
    python3.11
    tag
)

install(TARGETS player
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET player
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
