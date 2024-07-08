cmake_minimum_required(VERSION 3.16)
project(player VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
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


# Resources:
set(uiassets_resource_files
    "assets/LED_LCD_123.ttf"
    "assets/Minecraft.ttf"
    "assets/Winamp.ttf"
    "assets/background.png"
    "assets/balanceHandle.png"
    "assets/balanceHandle_p.png"
    "assets/bignumbers.ttf"
    "assets/eq_off.png"
    "assets/eq_off_p.png"
    "assets/eq_on.png"
    "assets/eq_on_p.png"
    "assets/fb_folderIcon.png"
    "assets/fb_folderIcon_selected.png"
    "assets/fb_musicIcon.png"
    "assets/fb_musicIcon_selected.png"
    "assets/filesIcon.png"
    "assets/logoButton.png"
    "assets/next.png"
    "assets/next_p.png"
    "assets/open.png"
    "assets/open_p.png"
    "assets/pause.png"
    "assets/pause_p.png"
    "assets/pl_add.png"
    "assets/pl_addIcon.png"
    "assets/pl_add_p.png"
    "assets/pl_close.png"
    "assets/pl_close_p.png"
    "assets/pl_homeIcon.png"
    "assets/pl_off.png"
    "assets/pl_off_p.png"
    "assets/pl_on.png"
    "assets/pl_on_p.png"
    "assets/pl_playerIcon.png"
    "assets/pl_upIcon.png"
    "assets/play.png"
    "assets/play_p.png"
    "assets/playlistsIcon.png"
    "assets/posHandle.png"
    "assets/posHandle_p.png"
    "assets/prev.png"
    "assets/prev_p.png"
    "assets/repeat_off.png"
    "assets/repeat_off_p.png"
    "assets/repeat_on.png"
    "assets/repeat_on_p.png"
    "assets/scroll_handle.png"
    "assets/scroll_handle_p.png"
    "assets/shuffle_off.png"
    "assets/shuffle_off_p.png"
    "assets/shuffle_on.png"
    "assets/shuffle_on_p.png"
    "assets/spotifyIcon.png"
    "assets/status_paused.png"
    "assets/status_playing.png"
    "assets/status_stopped.png"
    "assets/stop.png"
    "assets/stop_p.png"
    "assets/visualizationBackground.png"
    "assets/volumeHandle.png"
    "assets/volumeHandle_p.png"
    "assets/windowClose.png"
    "assets/windowMaximize.png"
    "assets/windowMinimize.png"
    "styles/controlbuttonswidget.repeatButton.1x.qss"
    "styles/controlbuttonswidget.repeatButton.2x.qss"
    "styles/controlbuttonswidget.repeatButton.3x.qss"
    "styles/controlbuttonswidget.repeatButton.4x.qss"
    "styles/controlbuttonswidget.shuffleButton.1x.qss"
    "styles/controlbuttonswidget.shuffleButton.2x.qss"
    "styles/controlbuttonswidget.shuffleButton.3x.qss"
    "styles/controlbuttonswidget.shuffleButton.4x.qss"
    "styles/desktopbasewindow.1x.qss"
    "styles/desktopbasewindow.2x.qss"
    "styles/desktopbasewindow.3x.qss"
    "styles/desktopbasewindow.4x.qss"
    "styles/playerview.balanceSlider.1x.qss"
    "styles/playerview.balanceSlider.2x.qss"
    "styles/playerview.balanceSlider.3x.qss"
    "styles/playerview.balanceSlider.4x.qss"
    "styles/playerview.codecDetailsContainer.1x.qss"
    "styles/playerview.codecDetailsContainer.2x.qss"
    "styles/playerview.codecDetailsContainer.3x.qss"
    "styles/playerview.codecDetailsContainer.4x.qss"
    "styles/playerview.eqButton.1x.qss"
    "styles/playerview.eqButton.2x.qss"
    "styles/playerview.eqButton.3x.qss"
    "styles/playerview.eqButton.4x.qss"
    "styles/playerview.kbpsFrame.1x.qss"
    "styles/playerview.kbpsFrame.2x.qss"
    "styles/playerview.kbpsFrame.3x.qss"
    "styles/playerview.kbpsFrame.4x.qss"
    "styles/playerview.khzFrame.1x.qss"
    "styles/playerview.khzFrame.2x.qss"
    "styles/playerview.khzFrame.3x.qss"
    "styles/playerview.khzFrame.4x.qss"
    "styles/playerview.playlistButton.1x.qss"
    "styles/playerview.playlistButton.2x.qss"
    "styles/playerview.playlistButton.3x.qss"
    "styles/playerview.playlistButton.4x.qss"
    "styles/playerview.posBar.1x.qss"
    "styles/playerview.posBar.2x.qss"
    "styles/playerview.posBar.3x.qss"
    "styles/playerview.posBar.4x.qss"
    "styles/playerview.songInfoContainer.1x.qss"
    "styles/playerview.songInfoContainer.2x.qss"
    "styles/playerview.songInfoContainer.3x.qss"
    "styles/playerview.songInfoContainer.4x.qss"
    "styles/playerview.visualizationFrame.1x.qss"
    "styles/playerview.visualizationFrame.2x.qss"
    "styles/playerview.visualizationFrame.3x.qss"
    "styles/playerview.visualizationFrame.4x.qss"
    "styles/playerview.volumeSlider.1x.qss"
    "styles/playerview.volumeSlider.2x.qss"
    "styles/playerview.volumeSlider.3x.qss"
    "styles/playerview.volumeSlider.4x.qss"
)

qt_add_resources(player "uiassets"
    PREFIX
        "/"
    FILES
        ${uiassets_resource_files}
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
