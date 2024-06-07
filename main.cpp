// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
// Copyright (C) 2023 Rodrigo Mendez.

#include "mainwindow.h"
#include "scale.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QUrl>

#define APP_VERSION_STR "1.0.1"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("Linamp");
    QCoreApplication::setOrganizationName("Rod");
    QCoreApplication::setApplicationVersion(APP_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription("Linamp");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("url", "The URL(s) to open.");
    parser.process(app);

    MainWindow window;
    if (!parser.positionalArguments().isEmpty()) {
        QList<QUrl> urls;
        for (auto &a : parser.positionalArguments())
            urls.append(QUrl::fromUserInput(a, QDir::currentPath()));
        //window.player->addToPlaylist(urls);
    }

    #ifdef IS_EMBEDDED
    window.setWindowState(Qt::WindowFullScreen);
    #endif
    window.show();

    return app.exec();
}
