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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

qt_add_executable(player WIN32 MACOSX_BUNDLE
    src/audiosource.cpp src/audiosource.h
    src/audiosourcebluetooth.cpp src/audiosourcebluetooth.h
    src/audiosourcecd.cpp src/audiosourcecd.h
    src/audiosourcecoordinator.cpp src/audiosourcecoordinator.h
    src/audiosourcefile.cpp src/audiosourcefile.h
    src/audiosourcewspectrumcapture.cpp src/audiosourcewspectrumcapture.h
    src/controlbuttonswidget.cpp src/controlbuttonswidget.h src/controlbuttonswidget.ui
    src/desktopbasewindow.cpp src/desktopbasewindow.h src/desktopbasewindow.ui
    src/desktopplayerwindow.cpp src/desktopplayerwindow.h src/desktopplayerwindow.ui
    src/embeddedbasewindow.cpp src/embeddedbasewindow.h src/embeddedbasewindow.ui
    src/fft.cpp src/fft.h
    src/filebrowsericonprovider.cpp src/filebrowsericonprovider.h
    src/main.cpp
    src/mainmenuview.cpp src/mainmenuview.h src/mainmenuview.ui
    src/mainwindow.cpp src/mainwindow.h
    src/mediaplayer.cpp src/mediaplayer.h
    src/playerview.cpp src/playerview.h src/playerview.ui
    src/playlistmodel.cpp src/playlistmodel.h
    src/playlistview.cpp src/playlistview.h src/playlistview.ui
    src/qmediaplaylist.cpp src/qmediaplaylist.h src/qmediaplaylist_p.cpp src/qmediaplaylist_p.h
    src/qplaylistfileparser.cpp src/qplaylistfileparser.h
    src/scale.cpp src/scale.h
    src/scrolltext.cpp src/scrolltext.h
    src/spectrumwidget.cpp src/spectrumwidget.h
    src/systemaudiocontrol.cpp src/systemaudiocontrol.h
    src/titlebar.cpp src/titlebar.h src/titlebar.ui
    src/util.cpp src/util.h
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
