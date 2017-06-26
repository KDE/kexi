/* This file is part of the KDE project
   Copyright (C) 2003-2014 Jarosław Staniek <staniek@kde.org>

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

#include "KexiStartupFileHandler.h"
#include <kexi_global.h>
#include <core/kexi.h>
#include <KexiFileFilters.h>
#include <kexiutils/utils.h>
#include <kexiutils/KexiContextMessage.h>

#include <KDbDriver>
#include <KDbUtils>

#include <KMessageBox>
#include <KFileWidget>
#include <KFile>
#include <KUrlComboBox>
#include <KActionCollection>
//removed in KEXI3 #include <KFileDialog>
#include <KUrlRequester>
#include <KLocalizedString>
#include <KRecentDirs>

#include <QDebug>
#include <QEvent>
#include <QAction>
#include <QEventLoop>
#include <QMimeDatabase>
#include <QMimeType>
#include <QUrl>

//! @internal
class Q_DECL_HIDDEN KexiStartupFileHandler::Private
{
public:
    Private()
            : confirmOverwrites(true)
            //, filtersUpdated(false)
    {
    }
    ~Private() {
        if (messageWidgetLoop) {
            messageWidgetLoop->exit(0);
            messageWidgetLoop->processEvents(); // for safe exit
            messageWidgetLoop->exit(0);
            delete messageWidgetLoop;
        }
    }

    void setUrl(const QUrl &url)
    {
        if (requester) {
            requester->setUrl(url);
        }
/*removed in KEXI3
        else {
            dialog->setUrl(url);
        }*/
    }

// removed in KEXI3    QPointer<KFileDialog> dialog;
    QPointer<KUrlRequester> requester;
    QString lastFileName;
    KexiFileFilters::Mode mode;
    QSet<QString> additionalMimeTypes, excludedMimeTypes;
    QString defaultExtension;
    bool confirmOverwrites;
    QString recentDirClass;

    QPointer<QEventLoop> messageWidgetLoop;
    //! Used in KexiStartupFileHandler::askForOverwriting() to remember path that
    //! was recently accepted for overwrite by the user.
    QString recentFilePathConfirmed;
};

//------------------

/* removed in KEXI3
KexiStartupFileHandler::KexiStartupFileHandler(
    const QUrl &startDirOrVariable, Mode mode, KFileDialog *dialog)
    :  QObject(dialog->parent())
    , d(new Private)
{
    d->dialog = dialog;
    init(startDirOrVariable, mode);
}*/

KexiStartupFileHandler::KexiStartupFileHandler(
    const QUrl &startDirOrVariable, KexiFileFilters::Mode mode, KUrlRequester *requester)
    :  QObject(requester->parent())
    , d(new Private)
{
    d->requester = requester;
//removed in KEXI3    d->dialog = d->requester->fileDialog();
    init(startDirOrVariable, mode);
}

void KexiStartupFileHandler::init(const QUrl &startDirOrVariable, KexiFileFilters::Mode mode)
{
//removed in KEXI3    connect(d->dialog, SIGNAL(accepted()), this, SLOT(slotAccepted()));
    QUrl url;
    if (startDirOrVariable.scheme() == "kfiledialog") {
        url = KFileWidget::getStartUrl(startDirOrVariable, d->recentDirClass);
    }
    else {
        url = startDirOrVariable;
    }
    d->setUrl(url);
    setMode(mode);
/*removed in KEXI3
    QAction *previewAction = d->dialog->actionCollection()->action("preview");
    if (previewAction)
        previewAction->setChecked(false);*/
}

KexiStartupFileHandler::~KexiStartupFileHandler()
{
    saveRecentDir();
    delete d;
}

void KexiStartupFileHandler::saveRecentDir()
{
    if (!d->recentDirClass.isEmpty()) {
        qDebug() << d->recentDirClass;

        QUrl dirUrl;
        if (d->requester)
            dirUrl = d->requester->url();
//removed in KEXI3        else if (d->dialog)
//removed in KEXI3            dirUrl = d->dialog->selectedUrl();
        qDebug() << dirUrl;
        if (dirUrl.isValid() && dirUrl.isLocalFile()) {
            dirUrl = dirUrl.adjusted(QUrl::RemoveFilename);
            dirUrl.setPath(dirUrl.path() + QString());
            qDebug() << "Added" << dirUrl.url() << "to recent dirs class" << d->recentDirClass;
            KRecentDirs::add(d->recentDirClass, dirUrl.url());
        }
    }
}

KexiFileFilters::Mode KexiStartupFileHandler::mode() const
{
    return d->mode;
}

void KexiStartupFileHandler::setMode(KexiFileFilters::Mode mode)
{
    //delayed
    d->mode = mode;
    updateFilters();
}

QStringList KexiStartupFileHandler::additionalMimeTypes() const
{
    return d->additionalMimeTypes.toList();
}

void KexiStartupFileHandler::setAdditionalMimeTypes(const QStringList &mimeTypes)
{
    //delayed
    d->additionalMimeTypes = mimeTypes.toSet();
    updateFilters();
}

QStringList KexiStartupFileHandler::excludedMimeTypes() const
{
    return d->excludedMimeTypes.toList();
}

void KexiStartupFileHandler::setExcludedMimeTypes(const QStringList &mimeTypes)
{
    //delayed
    d->excludedMimeTypes.clear();
    //convert to lowercase
    for(const QString& mimeType : mimeTypes) {
        d->excludedMimeTypes.insert(mimeType.toLower());
    }
    updateFilters();
}

void KexiStartupFileHandler::updateFilters()
{
    d->lastFileName.clear();
//removed in KEXI3    d->dialog->clearFilter();

    QString filter;
    QMimeDatabase db;
    QMimeType mime;
    QStringList allfilters;

    const QString separator(KexiFileFilters::separator(KexiFileFilters::KDEFormat));
    if (d->mode == KexiFileFilters::Opening || d->mode == KexiFileFilters::SavingFileBasedDB) {
        mime = db.mimeTypeForName(KDb::defaultFileBasedDriverMimeType());
        if (mime.isValid() && !d->excludedMimeTypes.contains(mime.name().toLower())) {
            if (!filter.isEmpty()) {
                filter += separator;
            }
            filter += KexiFileFilters::toString(mime, KexiFileFilters::KDEFormat);
            allfilters += mime.globPatterns();
        }
    }
    if (d->mode == KexiFileFilters::Opening || d->mode == KexiFileFilters::SavingServerBasedDB) {
        mime = db.mimeTypeForName("application/x-kexiproject-shortcut");
        if (mime.isValid() && !d->excludedMimeTypes.contains(mime.name().toLower())) {
            if (!filter.isEmpty()) {
                filter += separator;
            }
            filter += KexiFileFilters::toString(mime, KexiFileFilters::KDEFormat);
            allfilters += mime.globPatterns();
        }
    }
    if (d->mode == KexiFileFilters::Opening || d->mode == KexiFileFilters::SavingServerBasedDB) {
        mime = db.mimeTypeForName("application/x-kexi-connectiondata");
        if (mime.isValid() && !d->excludedMimeTypes.contains(mime.name().toLower())) {
            if (!filter.isEmpty()) {
                filter += separator;
            }
            filter += KexiFileFilters::toString(mime, KexiFileFilters::KDEFormat);
            allfilters += mime.globPatterns();
        }
    }

//! @todo hardcoded for MSA:
    if (d->mode == KexiFileFilters::Opening || d->mode == KexiFileFilters::CustomOpening) {
        mime = db.mimeTypeForName("application/vnd.ms-access");
        if (mime.isValid() && !d->excludedMimeTypes.contains(mime.name().toLower())) {
            if (!filter.isEmpty()) {
                filter += separator;
            }
            filter += KexiFileFilters::toString(mime, KexiFileFilters::KDEFormat);
            allfilters += mime.globPatterns();
        }
    }

    foreach(const QString& mimeName, d->additionalMimeTypes) {
        if (mimeName == "all/allfiles") {
            continue;
        }
        if (d->excludedMimeTypes.contains(mimeName.toLower())) {
            continue;
        }
        if (!filter.isEmpty()) {
            filter += separator;
        }
        filter += KexiFileFilters::toString(mimeName, KexiFileFilters::KDEFormat);
        mime = db.mimeTypeForName(mimeName);
        allfilters += mime.globPatterns();
    }

    if (!d->excludedMimeTypes.contains("all/allfiles")) {
        if (!filter.isEmpty()) {
            filter += separator;
        }
        filter += KexiFileFilters::toString("all/allfiles", KexiFileFilters::KDEFormat);
    }
    //remove duplicates made because upper- and lower-case extenstions are used:
    QStringList allfiltersUnique = allfilters.toSet().toList();
    qSort(allfiltersUnique);

    if (allfiltersUnique.count() > 1) {//prepend "all supoported files" entry
        if (!filter.isEmpty()) {
            filter += separator;
        }
        filter.prepend(KexiFileFilters::toString(allfiltersUnique,
            xi18n("All Supported Files"), KexiFileFilters::KDEFormat));
    }

    d->requester->setFilter(filter);

    if (d->mode == KexiFileFilters::Opening || d->mode == KexiFileFilters::CustomOpening) {
        d->requester->setMode(KFile::ExistingOnly | KFile::LocalOnly | KFile::File);
//removed in KEXI3        d->dialog->setOperationMode(KFileDialog::Opening);
    } else {
        d->requester->setMode(KFile::LocalOnly | KFile::File);
//removed in KEXI3        d->dialog->setOperationMode(KFileDialog::Saving);
    }
}

//! @todo
/*TODO
QString KexiStartupFileDialog::selectedFile() const
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
        qDebug() << "KexiStartupFileDialog::checkURL(): append extension, " << path;
      }
    }
  }
  qDebug() << "KexiStartupFileDialog::currentFileName() == " << path;
  return path;
}
*/

bool KexiStartupFileHandler::checkSelectedUrl()
{
    //qDebug() << "d->highlightedUrl: " << d->highlightedUrl;

    QUrl url;
    if (d->requester)
        url = d->requester->url();
//removed in KEXI3    else
//removed in KEXI3       url = d->dialog->selectedUrl();
    qDebug() << url;
#if 0
    if (/*d->highlightedUrl.isEmpty() &&*/ !locationEdit()->lineEdit()->text().isEmpty()) {
        qDebug() << locationEdit()->lineEdit()->text();
        //qDebug() << locationEdit()->urls();
        qDebug() << baseUrl();

        d->highlightedUrl = baseUrl();
        const QString firstUrl(locationEdit()->lineEdit()->text());   // FIXME: find first...
        if (QDir::isAbsolutePath(firstUrl))
            d->highlightedUrl = QUrl::fromLocalFile(firstUrl);
        else
            d->highlightedUrl.addPath(firstUrl);
    }
#endif
    //qDebug() << "d->highlightedUrl: " << d->highlightedUrl;
    if (!url.isValid() || QFileInfo(url.path()).isDir()) {
        KMessageBox::error(d->requester->parentWidget(), xi18n("Enter a filename."));
        return false;
    }

    if (!d->requester->filter().isEmpty()) {
        if (d->mode == KexiFileFilters::SavingFileBasedDB) {
            const QStringList filters( d->requester->filter().split('\n') );
            QString path = url.toLocalFile();
            qDebug()<< "filters:" << filters << "path:" << path;
            QString ext( QFileInfo(path).suffix() );
            bool hasExtension = false;
            for (const QString &filter : filters) {
                QStringList filterPatterns = filter.split('|').first().split(' ');
                for (const QString &filterPattern : filterPatterns) {
                    const QString f( filterPattern.trimmed() );
                    if (!f.midRef(2).isEmpty() && ext == f.midRef(2)) {
                        hasExtension = true;
                        break;
                    }
                }
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
                path += (QLatin1String(".") + defaultExtension);
                qDebug() << "appended extension, result:" << path;
                url = QUrl(path);
                d->setUrl(url);
            }
        }
    }

// qDebug() << "KexiStartupFileDialog::checkURL() path: " << d->highlightedUrl;
// qDebug() << "KexiStartupFileDialog::checkURL() fname: " << url.fileName();
//! @todo if ( url.isLocalFile() ) {
    QFileInfo fi(url.toLocalFile());
    if (d->mode & KFile::ExistingOnly) {
        if (!fi.exists()) {
            KMessageBox::error(d->requester->parentWidget(),
                               xi18nc("@info", "The file <filename>%1</filename> does not exist.",
                                      QDir::toNativeSeparators(url.toLocalFile())));
            return false;
        } else if (mode() & KFile::File) {
            if (!fi.isFile()) {
                KMessageBox::error(d->requester->parentWidget(),
                                   xi18nc("@info", "Enter a filename."));
                return false;
            } else if (!fi.isReadable()) {
                KMessageBox::error(d->requester->parentWidget(),
                                   xi18nc("@info", "The file <filename>%1</filename> is not readable.",
                                          QDir::toNativeSeparators(url.toLocalFile())));
                return false;
            }
        }
    }
    else if (d->confirmOverwrites && !askForOverwriting(url.toLocalFile()))
    {
        return false;
    }
    return true;
}

void KexiStartupFileHandler::messageWidgetActionYesTriggered()
{
    d->messageWidgetLoop->exit(1);
}

void KexiStartupFileHandler::messageWidgetActionNoTriggered()
{
    d->messageWidgetLoop->exit(0);
}

void KexiStartupFileHandler::updateUrl(const QString &name)
{
    QUrl url = d->requester->url();
    QString fn = KDbUtils::stringToFileName(name);
    if (!fn.isEmpty() && !fn.endsWith(".kexi"))
        fn += ".kexi";
    url = url.adjusted(QUrl::RemoveFilename);
    qDebug() << url.toLocalFile();
    url.setPath(url.toLocalFile() + fn);
    d->requester->setUrl(url);
}

bool KexiStartupFileHandler::askForOverwriting(const QString& filePath)
{
    QFileInfo fi(filePath);
    if (d->recentFilePathConfirmed == filePath) {
        return true;
    }
    d->recentFilePathConfirmed.clear();
    if (!fi.exists())
        return true;
    KexiContextMessage message(
        xi18n("This file already exists. Do you want to overwrite it?"));
    QScopedPointer<QAction> messageWidgetActionYes(new QAction(xi18n("Overwrite"), 0));
    connect(messageWidgetActionYes.data(), SIGNAL(triggered()),
            this, SLOT(messageWidgetActionYesTriggered()));
    message.addAction(messageWidgetActionYes.data());
    QScopedPointer<QAction> messageWidgetActionNo(new QAction(KStandardGuiItem::no().text(), 0));
    connect(messageWidgetActionNo.data(), SIGNAL(triggered()),
            this, SLOT(messageWidgetActionNoTriggered()));
    message.addAction(messageWidgetActionNo.data());
    message.setDefaultAction(messageWidgetActionNo.data());
    emit askForOverwriting(message);
    if (!d->messageWidgetLoop) {
        d->messageWidgetLoop = new QEventLoop;
    }
    bool ok = d->messageWidgetLoop->exec();
    if (ok) {
        d->recentFilePathConfirmed = filePath;
    }
    return ok;
}

/*removed in KEXI3
void KexiStartupFileHandler::setLocationText(const QString& fn)
{
    d->dialog->locationEdit()->setUrl(QUrl(fn));
}*/

void KexiStartupFileHandler::setDefaultExtension(const QString& ext)
{
    d->defaultExtension = ext;
}

void KexiStartupFileHandler::setConfirmOverwrites(bool set)
{
    d->confirmOverwrites = set;
}

void KexiStartupFileHandler::slotAccepted()
{
    checkSelectedUrl();
}

