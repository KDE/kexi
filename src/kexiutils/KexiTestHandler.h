/* This file is part of the KDE project
   Copyright (C) 2012-2017 Jarosław Staniek <staniek@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KEXITESTHANDLER_H
#define KEXITESTHANDLER_H

#include <kexiutils_export.h>

#include <QtTest>
#include <QtTest/qtestkeyboard.h>
#include <QtTest/qtestmouse.h>
#include <QCommandLineOption>
#include <QObject>

//! A handler for Kexi test objects
class KEXIUTILS_EXPORT KexiTestHandler : public QObject
{
    Q_OBJECT
public:
    KexiTestHandler();

    ~KexiTestHandler();

    void addExtraOption(const QCommandLineOption &option);

    QList<QCommandLineOption> extraOptions() const;

    void removeOwnOptions(QStringList *args);

private:
    class Private;
    Private * const d;
};

// Override but still use the same macro so Qt Creator lists the test(s)
// Note: don't link agains QtTest lib, otherwise Creator will mark KexiUtils as tests
#undef QTEST_MAIN
#define QTEST_MAIN(TestObject) \
    QT_BEGIN_NAMESPACE \
    QTEST_ADD_GPU_BLACKLIST_SUPPORT_DEFS \
    QT_END_NAMESPACE \
    int main(int argc, char *argv[]) \
    { \
        QApplication app(argc, argv); \
        app.setAttribute(Qt::AA_Use96Dpi, true); \
        QTEST_DISABLE_KEYPAD_NAVIGATION \
        QTEST_ADD_GPU_BLACKLIST_SUPPORT \
        TestObject tc; \
        QStringList args(QCoreApplication::arguments()); \
        tc.removeOwnOptions(&args); \
        return QTest::qExec(&tc, args); \
    }

#endif
