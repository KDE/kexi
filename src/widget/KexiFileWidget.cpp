/* This file is part of the KDE project
   Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>

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
//! @todo KEXI3 #include <migration/migratemanager.h>

#include <KDbDriver>
#include <KDbUtils>

#include <KActionCollection>
#include <KFile>
#include <KFileFilterCombo>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRecentDirs>
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

    KexiFileFilters filters;
    QString defaultExtension;
    bool confirmOverwrites = true;
    bool filtersUpdated = false;
    QUrl highlightedUrl;
    QString recentDirClass;
};

//------------------

KexiFileWidget::KexiFileWidget(
    const QUrl &startDirOrVariable, KexiFileFilters::Mode mode, QWidget *parent)
        :  KFileWidget(startDirOrVariable, parent)
        , d(new Private)
{
    qDebug() << startDirOrVariable.scheme();
    if (startDirOrVariable.scheme() == "kfiledialog") {
//! @todo KEXI3 test it
        KFileWidget::getStartUrl(startDirOrVariable, d->recentDirClass);
    }
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setMode(mode);
    QAction *previewAction = actionCollection()->action("preview");
    if (previewAction)
        previewAction->setChecked(false);
    setFocusProxy(locationEdit());
    connect(this, SIGNAL(fileHighlighted(QUrl)),
            this, SLOT(slotExistingFileHighlighted(QUrl)));
}

KexiFileWidget::~KexiFileWidget()
{
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
        qDebug() << dir;
        qDebug() << highlightedFile();
        if (!dir.isEmpty())
            KRecentDirs::add(d->recentDirClass, dir.url());
    }
    delete d;
}

void KexiFileWidget::slotExistingFileHighlighted(const QUrl& url)
{
    qDebug() << url;
    d->highlightedUrl = url;
    emit fileHighlighted();
}

QString KexiFileWidget::highlightedFile() const
{
    return d->highlightedUrl.toLocalFile();
}

KexiFileFilters::Mode KexiFileWidget::mode() const
{
    return d->filters.mode();
}

void KexiFileWidget::setMode(KexiFileFilters::Mode mode)
{
    //delayed
    d->filters.setMode(mode);
    d->filtersUpdated = false;
    updateFilters();
}

QStringList KexiFileWidget::additionalMimeTypes() const
{
    return d->filters.additionalMimeTypes();
}

void KexiFileWidget::setAdditionalMimeTypes(const QStringList &mimeTypes)
{
    d->filters.setAdditionalMimeTypes(mimeTypes);
    d->filtersUpdated = false;
}

QStringList KexiFileWidget::excludedMimeTypes() const
{
    return d->filters.excludedMimeTypes();
}

void KexiFileWidget::setExcludedMimeTypes(const QStringList &mimeTypes)
{
    d->filters.setExcludedMimeTypes(mimeTypes);
    d->filtersUpdated = false;
}

void KexiFileWidget::updateFilters()
{
    if (d->filtersUpdated)
        return;
    d->filtersUpdated = true;
    clearFilter();
    d->filters.setDefaultFilter(filterWidget()->defaultFilter());
    setFilter(d->filters.toString(KexiFileFilters::KDEFormat));

    if (d->filters.mode() == KexiFileFilters::Opening || d->filters.mode() == KexiFileFilters::CustomOpening) {
        KFileWidget::setMode(KFile::ExistingOnly | KFile::LocalOnly | KFile::File);
        setOperationMode(KFileWidget::Opening);
    } else {
        KFileWidget::setMode(KFile::LocalOnly | KFile::File);
        setOperationMode(KFileWidget::Saving);
    }
}

void KexiFileWidget::showEvent(QShowEvent * event)
{
    d->filtersUpdated = false;
    updateFilters();
    KFileWidget::showEvent(event);
}

/*TODO
QString KexiFileWidget::selectedFile() const
{
#ifdef Q_OS_WIN
// QString path = selectedFile();
  //js @todo
// qDebug() << "selectedFile() == " << path << " '" << url().fileName() << "' " << m_lineEdit->text();
  QString path = dir()->absolutePath();
  if (!path.endsWith('/') && !path.endsWith("\\"))
    path.append("/");
  path += m_lineEdit->text();
// QString path = QFileInfo(selectedFile()).dirPath(true) + "/" + m_lineEdit->text();
#else
// QString path = locationEdit->currentText().trimmed(); //url.path().trimmed(); that does not work, if the full path is not in the location edit !!!!!
  QString path( KFileWidget::selectedFile() );
  qDebug() << "prev selectedFile() == " << path;
  qDebug() << "locationEdit == " << locationEdit()->currentText().trimmed();
  //make sure user-entered path is acceped:
//! @todo KEXI3 setSelection( locationEdit()->currentText().trimmed() );
// path = KFileWidget::selectedFile();
  path = locationEdit()->currentText().trimmed();
  qDebug() << "selectedFile() == " << path;

#endif

  if (!currentFilter().isEmpty()) {
    if (d->mode & SavingFileBasedDB) {
      const QStringList filters( currentFilter().split(' ') );
      qDebug()<< " filter == " << filters;
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
        qDebug() << "KexiFileWidget::checkURL(): append extension, " << path;
      }
    }
  }
  qDebug() << "KexiFileWidget::currentFileName() == " << path;
  return path;
}
*/

bool KexiFileWidget::checkSelectedFile()
{
    qDebug() << "d->highlightedUrl: " << d->highlightedUrl;

    if (!locationEdit()->lineEdit()->text().isEmpty()) {
        qDebug() << locationEdit()->lineEdit()->text();
        qDebug() << locationEdit()->urls();
        qDebug() << baseUrl();

        d->highlightedUrl = baseUrl();
        const QString firstUrl(locationEdit()->lineEdit()->text());   // FIXME: find first...
        if (QDir::isAbsolutePath(firstUrl)) {
            d->highlightedUrl = QUrl::fromLocalFile(firstUrl);
        } else {
            d->highlightedUrl = d->highlightedUrl.adjusted(QUrl::StripTrailingSlash);
            d->highlightedUrl.setPath(d->highlightedUrl.path() + '/' + firstUrl);
        }
    }

    qDebug() << "d->highlightedUrl: " << d->highlightedUrl;
    if (d->highlightedUrl.isEmpty()) {
        KMessageBox::error(this, xi18n("Enter a filename."));
        return false;
    }

    if (!currentFilter().isEmpty()) {
        if (d->filters.mode() == KexiFileFilters::SavingFileBasedDB || d->filters.mode() == KexiFileFilters::CustomSavingFileBasedDB) {
            const QStringList filters( currentFilter().split(' ') );
            QString path = highlightedFile();
            qDebug()<< "filter:" << filters << "path:" << path;
            QString ext( QFileInfo(path).suffix() );
            bool hasExtension = false;
            foreach (const QString& filter, filters) {
                const QString f( filter.trimmed() );
                hasExtension = !f.midRef(2).isEmpty() && ext==f.midRef(2);
                if (hasExtension) {
                    break;
                }
            }
            if (!hasExtension) {
                //no extension: add one
                QString defaultExtension( d->defaultExtension );
                if (defaultExtension.isEmpty()) {
                    defaultExtension = filters.first().trimmed().mid(2); //first one
                }
                path += (QLatin1String(".")+defaultExtension);
                qDebug() << "appended extension" << path;
                setSelection( path );
                d->highlightedUrl = QUrl(path);
            }
        }
    }

    qDebug() << "KexiFileWidget::checkURL() path: " << d->highlightedUrl;
// qDebug() << "KexiFileWidget::checkURL() fname: " << url.fileName();
//! @todo if ( url.isLocalFile() ) {
    QFileInfo fi(d->highlightedUrl.toLocalFile());
    if (KFileWidget::mode() & KFile::ExistingOnly) {
        if (!fi.exists()) {
            KMessageBox::error(this,
                               xi18nc("@info", "The file <filename>%1</filename> does not exist.",
                                      QDir::toNativeSeparators(d->highlightedUrl.toLocalFile())));
            return false;
        } else if (KFileWidget::mode() & KFile::File) {
            if (!fi.isFile()) {
                KMessageBox::error(this, xi18nc("@info", "Enter a filename."));
                return false;
            } else if (!fi.isReadable()) {
                KMessageBox::error(this,
                                   xi18nc("@info", "The file <filename>%1</filename> is not readable.",
                                          QDir::toNativeSeparators(d->highlightedUrl.path())));
                return false;
            }
        }
    } else if (d->confirmOverwrites && !KexiUtils::askForFileOverwriting(d->highlightedUrl.path(), this)) {
        return false;
    }
    return true;
}

void KexiFileWidget::accept()
{
    qDebug() << "KexiFileWidget::accept()...";

    KFileWidget::accept();
// qDebug() << selectedFile();

// locationEdit->setFocus();
// QKeyEvent ev(QEvent::KeyPress, Qt::Key_Enter, '\n', 0);
// QApplication::sendEvent(locationEdit, &ev);
// QApplication::postEvent(locationEdit, &ev);

// qDebug() << "KexiFileWidget::accept() m_lastUrl == " << m_lastUrl.path();
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
      qDebug() << "KexiFileWidget::accept(): path = " << selectedFile();
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
    qDebug() << "KexiFileWidget: reject!";
    emit rejected();
}

void KexiFileWidget::setLocationText(const QString& fn)
{
    locationEdit()->setUrl(QUrl(fn));
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

void KexiFileWidget::focusInEvent(QFocusEvent *)
{
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

void KexiFileWidget::setDefaultExtension(const QString& ext)
{
    d->defaultExtension = ext;
}

void KexiFileWidget::setConfirmOverwrites(bool set)
{
    d->confirmOverwrites = set;
}
