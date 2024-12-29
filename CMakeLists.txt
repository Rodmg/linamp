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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/audiosource-base)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/audiosource-coordinator)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/audiosourcebluetooth)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/audiosourcepython)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/audiosourcecd)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/audiosourcefile)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/shared)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/view-basewindow)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/view-menu)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/view-player)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/view-playlist)

qt_add_executable(player WIN32 MACOSX_BUNDLE
    src/audiosource-base/audiosource.cpp
    src/audiosource-base/audiosource.h
    src/audiosource-base/audiosourcewspectrumcapture.cpp
    src/audiosource-base/audiosourcewspectrumcapture.h
    src/audiosourcebluetooth/audiosourcebluetooth.cpp
    src/audiosourcebluetooth/audiosourcebluetooth.h
    src/audiosourcecd/audiosourcecd.cpp
    src/audiosourcecd/audiosourcecd.h
    src/audiosourcepython/audiosourcepython.cpp
    src/audiosourcepython/audiosourcepython.h
    src/audiosource-coordinator/audiosourcecoordinator.cpp
    src/audiosource-coordinator/audiosourcecoordinator.h
    src/audiosourcefile/audiosourcefile.cpp
    src/audiosourcefile/audiosourcefile.h
    src/audiosourcefile/mediaplayer.cpp
    src/audiosourcefile/mediaplayer.h
    src/view-player/controlbuttonswidget.cpp
    src/view-player/controlbuttonswidget.h
    src/view-player/controlbuttonswidget.ui
    src/view-player/scrolltext.cpp
    src/view-player/scrolltext.h
    src/view-player/spectrumwidget.cpp
    src/view-player/spectrumwidget.h
    src/view-player/playerview.cpp
    src/view-player/playerview.h
    src/view-player/playerview.ui
    src/view-basewindow/desktopbasewindow.cpp
    src/view-basewindow/desktopbasewindow.h
    src/view-basewindow/desktopbasewindow.ui
    src/view-basewindow/desktopplayerwindow.cpp
    src/view-basewindow/desktopplayerwindow.h
    src/view-basewindow/desktopplayerwindow.ui
    src/view-basewindow/embeddedbasewindow.cpp
    src/view-basewindow/embeddedbasewindow.h
    src/view-basewindow/embeddedbasewindow.ui
    src/view-basewindow/mainwindow.cpp
    src/view-basewindow/mainwindow.h
    src/view-basewindow/titlebar.cpp
    src/view-basewindow/titlebar.h
    src/view-basewindow/titlebar.ui
    src/view-playlist/filebrowsericonprovider.cpp
    src/view-playlist/filebrowsericonprovider.h
    src/view-playlist/playlistmodel.cpp
    src/view-playlist/playlistmodel.h
    src/view-playlist/playlistview.cpp
    src/view-playlist/playlistview.h
    src/view-playlist/playlistview.ui
    src/view-playlist/qmediaplaylist.cpp
    src/view-playlist/qmediaplaylist.h
    src/view-playlist/qmediaplaylist_p.cpp
    src/view-playlist/qmediaplaylist_p.h
    src/view-playlist/qplaylistfileparser.cpp
    src/view-playlist/qplaylistfileparser.h
    src/view-menu/mainmenuview.cpp
    src/view-menu/mainmenuview.h
    src/view-menu/mainmenuview.ui
    src/shared/scale.cpp
    src/shared/scale.h
    src/shared/systemaudiocontrol.cpp
    src/shared/systemaudiocontrol.h
    src/shared/fft.cpp
    src/shared/fft.h
    src/shared/util.cpp
    src/shared/util.h
    src/shared/linampslider.h
    src/shared/linampslider.cpp
    src/main.cpp
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
