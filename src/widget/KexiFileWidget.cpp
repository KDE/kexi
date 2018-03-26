/* This file is part of the KDE project
   Copyright (C) 2003-2017 Jarosław Staniek <staniek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "KexiFileWidget.h"
#include <kexi_global.h>
#include <core/kexi.h>
#include <core/KexiMainWindowIface.h>
#include <core/KexiMigrateManagerInterface.h>
#include <kexiutils/utils.h>

#include <KDbDriver>
#include <KDbUtils>

#include <KActionCollection>
#include <KFile>
#include <KFileFilterCombo>
#include <KLocalizedString>
#include <KMessageBox>
#include <KUrlComboBox>

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QFileDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLocale>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPushButton>

//! @internal
class Q_DECL_HIDDEN KexiFileWidget::Private
{
public:
    Private()
    {
    }
    QUrl selectedUrl;
};

//------------------

KexiFileWidget::KexiFileWidget(const QUrl &startDirOrVariable, KexiFileFilters::Mode mode,
                               QWidget *parent)
    : KFileWidget(startDirOrVariable, parent)
    , KexiFileWidgetInterface(startDirOrVariable)
    , d(new Private)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    QAction *previewAction = actionCollection()->action("preview");
    if (previewAction) {
        previewAction->setChecked(false);
    }
    setFocusProxy(locationEdit());
    connect(this, &KFileWidget::fileHighlighted, this, &KexiFileWidget::slotFileHighlighted);
    setMode(mode);
}

KexiFileWidget::~KexiFileWidget()
{
    done();
#if 0
    qDebug() << d->recentDirClass;
    if (!d->recentDirClass.isEmpty()) {
        QString hf = highlightedFile();
        QUrl dir;
        if (hf.isEmpty()) {
            dir = baseUrl();
        }
        else {
            QFileInfo fi(hf);
            QString dirStr = fi.isDir() ? fi.absoluteFilePath() : fi.dir().absolutePath();
            dir = QUrl::fromLocalFile(dirStr);
        }
        //qDebug() << dir;
        //qDebug() << highlightedFile();
        if (!dir.isEmpty())
            KRecentDirs::add(d->recentDirClass, dir.path());
    }
#endif
    delete d;
}

void KexiFileWidget::slotFileHighlighted(const QUrl& url)
{
    //qDebug() << url;
    d->selectedUrl = url;
    emit fileSelected(selectedFile());
}

void KexiFileWidget::slotFileSelected(const QUrl& url)
{
    Q_UNUSED(url)
    //qDebug() << url;
    emit fileSelected(selectedFile());
}

void KexiFileWidget::setMode(KexiFileFilters::Mode mode)
{
    KexiFileWidgetInterface::setMode(mode);
}

void KexiFileWidget::updateFilters()
{
    if (filtersUpdated())
        return;
    setFiltersUpdated(true);
    clearFilter();
    filters()->setDefaultFilter(filterWidget()->defaultFilter());
    setFilter(filters()->toString(KexiFileFilters::KDEFormat));

    if (filters()->mode() == KexiFileFilters::Opening || filters()->mode() == KexiFileFilters::CustomOpening) {
        KFileWidget::setMode(KFile::ExistingOnly | KFile::LocalOnly | KFile::File);
        setOperationMode(KFileWidget::Opening);
    } else {
        KFileWidget::setMode(KFile::LocalOnly | KFile::File);
        setOperationMode(KFileWidget::Saving);
    }
}

void KexiFileWidget::showEvent(QShowEvent * event)
{
    setFiltersUpdated(false);
    updateFilters();
    KFileWidget::showEvent(event);
}

/*TODO
QString KexiFileWidget::selectedFile() const
{
#ifdef Q_OS_WIN
// QString path = selectedFile();
  //js @todo
// qDebug() << "selectedFile() ==" << path << "'" << url().fileName() << "'" << m_lineEdit->text();
  QString path = dir()->absolutePath();
  if (!path.endsWith('/') && !path.endsWith("\\"))
    path.append("/");
  path += m_lineEdit->text();
// QString path = QFileInfo(selectedFile()).dirPath(true) + "/" + m_lineEdit->text();
#else
// QString path = locationEdit->currentText().trimmed(); //url.path().trimmed(); that does not work, if the full path is not in the location edit !!!!!
  QString path( KFileWidget::selectedFile() );
  qDebug() << "prev selectedFile() ==" << path;
  qDebug() << "locationEdit ==" << locationEdit()->currentText().trimmed();
  //make sure user-entered path is acceped:
//! @todo KEXI3 setSelection( locationEdit()->currentText().trimmed() );
// path = KFileWidget::selectedFile();
  path = locationEdit()->currentText().trimmed();
  qDebug() << "selectedFile() ==" << path;

#endif

  if (!currentFilter().isEmpty()) {
    if (d->mode & SavingFileBasedDB) {
      const QStringList filters( currentFilter().split(' ') );
      qDebug()<< "filter ==" << filters;
      QString ext( QFileInfo(path).suffix() );
      bool hasExtension = false;
      foreach (const QString& filter, filters) {
        const QString f( filter.trimmed() );
        hasExtension = !f.mid(2).isEmpty() && ext==f.mid(2);
        if (hasExtension)
          break;
      }
      if (!hasExtension) {
        //no extension: add one
        QString defaultExtension( d->defaultExtension );
        if (defaultExtension.isEmpty())
          defaultExtension = filters.first().trimmed().mid(2); //first one
        path += (QString(".")+defaultExtension);
        qDebug() << "KexiFileWidget::checkURL(): append extension," << path;
      }
    }
  }
  qDebug() << "KexiFileWidget::currentFileName() ==" << path;
  return path;
}
*/

QString KexiFileWidget::selectedFile() const
{
    return d->selectedUrl.toLocalFile();
}

QString KexiFileWidget::highlightedFile() const
{
    return d->selectedUrl.toLocalFile();
}

void KexiFileWidget::setSelectedFile(const QString &name)
{
    d->selectedUrl = QUrl::fromLocalFile(name);
}

QString KexiFileWidget::currentDir() const
{
    return KFileWidget::baseUrl().toLocalFile();
}

void KexiFileWidget::applyEnteredFileName()
{
    const QString enteredFileName(locationEdit()->lineEdit()->text());
    if (enteredFileName.isEmpty()) {
        return;
    }
    //qDebug() << enteredFileName;
    //qDebug() << locationEdit()->urls();
    //qDebug() << currentDir();

    setSelectedFile(currentDir());
    // FIXME: find first...
    if (QDir::isAbsolutePath(enteredFileName)) {
        setSelectedFile(enteredFileName);
    } else {
        setSelectedFile(currentDir() + enteredFileName);
    }
}

QStringList KexiFileWidget::currentFilters() const
{
    return currentFilter().split(QLatin1Char(' '));
}

void KexiFileWidget::accept()
{
    //qDebug();
    KFileWidget::accept();
// qDebug() << selectedFile();

// locationEdit->setFocus();
// QKeyEvent ev(QEvent::KeyPress, Qt::Key_Enter, '\n', 0);
// QApplication::sendEvent(locationEdit, &ev);
// QApplication::postEvent(locationEdit, &ev);

// qDebug() << "KexiFileWidget::accept() m_lastUrl ==" << m_lastUrl.path();
// if (m_lastUrl.path()==currentURL().path()) {//(js) to prevent more multiple kjob signals (I do not know why this is)
    /*
      if (d->lastFileName==selectedFile()) {//(js) to prevent more multiple kjob signals (I do not know why this is)
    //  m_lastUrl=QUrl();
        d->lastFileName.clear();
        qDebug() << "d->lastFileName==selectedFile()";
    #ifdef Q_OS_WIN
        return;
    #endif
      }
      qDebug() << "KexiFileWidget::accept(): path =" << selectedFile();
      if ( checkSelectedFile() ) {
        emit accepted();
      }
      d->lastFileName = selectedFile();

    #ifdef Q_OS_WIN
      saveLastVisitedPath(d->lastFileName);
    #endif*/
}

void KexiFileWidget::reject()
{
    //qDebug();
    emit rejected();
}

#if 0 // TODO?
void KexiFileWidget::setLocationText(const QString& text)
{
    locationEdit()->setUrl(QUrl(text));
    /*
    #ifdef Q_OS_WIN
      //js @todo
      setSelection(fn);
    #else
      setSelection(fn);
    // locationEdit->setCurrentText(fn);
    // locationEdit->lineEdit()->setEdited( true );
    // setSelection(fn);
    #endif*/
}
#endif

void KexiFileWidget::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    locationEdit()->setFocus();
}

/*bool KexiFileWidget::eventFilter ( QObject * watched, QEvent * e )
{
  //filter-out ESC key
  if (e->type()==QEvent::KeyPress && static_cast<QKeyEvent*>(e)->key()==Qt::Key_Escape
   && static_cast<QKeyEvent*>(e)->state()==Qt::NoButton) {
    static_cast<QKeyEvent*>(e)->accept();
    emit rejected();
    return true;
  }
  return KexiFileWidgetBase::eventFilter(watched,e);
} */

void KexiFileWidget::setWidgetFrame(bool set)
{
    Q_UNUSED(set)
}
