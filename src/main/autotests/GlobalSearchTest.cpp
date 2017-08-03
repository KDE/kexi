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

#include <core/kexipartitem.h>
#include <core/kexiaboutdata.h>
#include <main/KexiMainWindow.h>
#include <kexiutils/KexiTester.h>
#include <widget/navigator/KexiProjectNavigator.h>
#include <KexiTestHandler.h>
#include <kexistartupdata.h>

#include <KActionCollection>

#include <QApplication>
#include <QFile>
#include <QTreeView>
#include <QLineEdit>
#include <QDebug>

const int GUI_DELAY = 10;
const char *FILES_DATA_DIR = CURRENT_SOURCE_DIR "/data";

class GlobalSearchTest : public KexiTestHandler
{
    Q_OBJECT
public:
    GlobalSearchTest();

private Q_SLOTS:
    void testGlobalSearch();
private:
    QCommandLineOption m_loopOption;
};

GlobalSearchTest::GlobalSearchTest()
 : m_loopOption("loop", "Do not exit after successful test (stay in event loop)")
{
    addExtraOption(m_loopOption);
}

void GlobalSearchTest::testGlobalSearch()
{
    const QString filename(QFile::decodeName(FILES_DATA_DIR) + "/GlobalSearchTest.kexi");
    QStringList args(QApplication::arguments());
    args.append(filename);
    const int result = KexiMainWindow::create(args, metaObject()->className(), extraOptions());
    const KexiStartupData *h = KexiStartupData::global();
    QCOMPARE(result, 0);

    QLineEdit *lineEdit = kexiTester().widget<QLineEdit*>("globalSearch.lineEdit");
    QVERIFY(lineEdit);
    QTreeView *treeView = kexiTester().widget<QTreeView*>("globalSearch.treeView");
    QVERIFY(treeView);

    lineEdit->setFocus();
    // enter "cars", expect 4 completion items
    QTest::keyClicks(lineEdit, "cars");
    QVERIFY(treeView->isVisible());
    int globalSearchCompletionListRows = treeView->model()->rowCount();
    QCOMPARE(globalSearchCompletionListRows, 4);

    // add "x", expect no completion items and hidden list
    QTest::keyClicks(lineEdit, "x");
    QVERIFY(!treeView->isVisible());
    globalSearchCompletionListRows = treeView->model()->rowCount();
    QCOMPARE(globalSearchCompletionListRows, 0);

    // Escape should clear
    QTest::keyClick(lineEdit, Qt::Key_Escape,  Qt::NoModifier, GUI_DELAY);
    QVERIFY(lineEdit->text().isEmpty());

    QTest::keyClicks(lineEdit, "cars", Qt::NoModifier, GUI_DELAY);
    QVERIFY(treeView->isVisible());
    treeView->setFocus();
    // no highlight initially
    KexiProjectNavigator *projectNavigator = kexiTester().widget<KexiProjectNavigator*>("KexiProjectNavigator");
    QVERIFY(projectNavigator);
    QVERIFY(!projectNavigator->partItemWithSearchHighlight());

    QTest::keyPress(treeView, Qt::Key_Down, Qt::NoModifier, GUI_DELAY);

    // selecting 1st row should highlight "cars" table
    KexiPart::Item* highlightedPartItem = projectNavigator->partItemWithSearchHighlight();
    QVERIFY(highlightedPartItem);
    QCOMPARE(highlightedPartItem->name(), QLatin1String("cars"));
    QCOMPARE(highlightedPartItem->pluginId(), QLatin1String("org.kexi-project.table"));

    QTest::keyPress(treeView, Qt::Key_Down, Qt::NoModifier, GUI_DELAY);
    QTest::keyPress(treeView, Qt::Key_Down, Qt::NoModifier, GUI_DELAY);

    // 3rd row should be "cars" form
    QModelIndexList selectedIndices = treeView->selectionModel()->selectedRows();
    QCOMPARE(selectedIndices.count(), 1);
    QCOMPARE(treeView->model()->data(selectedIndices.first(), Qt::DisplayRole).toString(), QLatin1String("cars"));

    // check if proper entry of Project Navigator is selected
    QTest::keyPress(treeView, Qt::Key_Enter, Qt::NoModifier, GUI_DELAY);

    KexiPart::Item* selectedPartItem = projectNavigator->selectedPartItem();
    QVERIFY(selectedPartItem);
    QCOMPARE(selectedPartItem->name(), QLatin1String("cars"));
    QCOMPARE(selectedPartItem->pluginId(), QLatin1String("org.kexi-project.form"));

    if (h->isSet(m_loopOption)) {
        const int result = qApp->exec();
        QCOMPARE(result, 0);
    }
}

QTEST_MAIN(GlobalSearchTest)

#include "GlobalSearchTest.moc"
