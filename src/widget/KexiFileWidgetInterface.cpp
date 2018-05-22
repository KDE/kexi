/* This file is part of the KDE project
   Copyright (C) 2003-2018 Jarosław Staniek <staniek@kde.org>

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

#include "KexiFileWidgetInterface.h"
#include <kexiutils/utils.h>
#include "KexiFileRequester.h"

#include <KLocalizedString>
#include <KMessageBox>

#ifdef KEXI_USE_KFILEWIDGET
#include "KexiFileWidget.h"

#include <KConfigGroup>
#include <KSharedConfig>
#endif

#include <QDir>

//! @internal
class Q_DECL_HIDDEN KexiFileWidgetInterface::Private
{
public:
    Private()
    {
    }

    QUrl startUrl;
    KexiFileFilters filters;
    QString defaultExtension;
    bool confirmOverwrites = true;
    bool filtersUpdated = false;
    QString highlightedName;
    QString recentDirClass;
};

//------------------

KexiFileWidgetInterface::KexiFileWidgetInterface(const QUrl &startDirOrVariable,
                                                 const QString &fileName)
    : d(new Private)
{
    if (startDirOrVariable.scheme() == "kfiledialog") {
        d->startUrl = KexiUtils::getStartUrl(startDirOrVariable, &d->recentDirClass, fileName);
    } else if (fileName.isEmpty()) {
        d->startUrl = startDirOrVariable;
    } else {
        d->startUrl = startDirOrVariable;
        d->startUrl.setPath(startDirOrVariable.path() + '/' + fileName);
    }
}

KexiFileWidgetInterface::~KexiFileWidgetInterface()
{
    delete d;
}

QUrl KexiFileWidgetInterface::startUrl() const
{
    return d->startUrl;
}

void KexiFileWidgetInterface::addRecentDir(const QString &name)
{
    if (!d->recentDirClass.isEmpty() && QDir(name).exists()) {
        KexiUtils::addRecentDir(d->recentDirClass, name);
    }
}

void KexiFileWidgetInterface::done()
{
    qDebug() << d->recentDirClass;
    if (!d->recentDirClass.isEmpty()) {
        QString f = selectedFile();
        QString dir;
        if (f.isEmpty()) {
            dir = currentDir();
        }
        else {
            QFileInfo fi(f);
            QString dirStr = fi.isDir() ? fi.absoluteFilePath() : fi.dir().absolutePath();
            dir = dirStr;
        }
        qDebug() << dir;
        qDebug() << selectedFile();
        addRecentDir(dir);
    }
}

// static
KexiFileWidgetInterface *KexiFileWidgetInterface::createWidget(const QUrl &startDirOrVariable,
                                                               KexiFileFilters::Mode mode,
                                                               const QString &fileName,
                                                               QWidget *parent)
{
#ifdef KEXI_USE_KFILEWIDGET
    bool useKFileWidget;
    KConfigGroup group =  KSharedConfig::openConfig()->group("File Dialogs");
    if (group.hasKey("UseKFileWidget")) { // allow to override
        useKFileWidget = group.readEntry("UseKFileWidget", false);
    } else {
        useKFileWidget = KexiUtils::isKDEDesktopSession();
    }
    if (useKFileWidget) {
        return new KexiFileWidget(startDirOrVariable, mode, fileName, parent);
    }
#endif
    return new KexiFileRequester(startDirOrVariable, mode, fileName, parent);
}

// static
KexiFileWidgetInterface *KexiFileWidgetInterface::createWidget(const QUrl &startDirOrVariable,
                                                               KexiFileFilters::Mode mode,
                                                               QWidget *parent)
{
    return KexiFileWidgetInterface::createWidget(startDirOrVariable, mode, QString(), parent);
}

KexiFileFilters::Mode KexiFileWidgetInterface::mode() const
{
    return d->filters.mode();
}

void KexiFileWidgetInterface::setMode(KexiFileFilters::Mode mode)
{
    //delayed
    d->filters.setMode(mode);
    d->filtersUpdated = false;
    updateFilters();
}

QStringList KexiFileWidgetInterface::additionalMimeTypes() const
{
    return d->filters.additionalMimeTypes();
}

void KexiFileWidgetInterface::setAdditionalMimeTypes(const QStringList &mimeTypes)
{
    d->filters.setAdditionalMimeTypes(mimeTypes);
    d->filtersUpdated = false;
}

QStringList KexiFileWidgetInterface::excludedMimeTypes() const
{
    return d->filters.excludedMimeTypes();
}

void KexiFileWidgetInterface::setExcludedMimeTypes(const QStringList &mimeTypes)
{
    d->filters.setExcludedMimeTypes(mimeTypes);
    d->filtersUpdated = false;
}

QString KexiFileWidgetInterface::defaultExtension() const
{
    return d->defaultExtension;
}

void KexiFileWidgetInterface::setDefaultExtension(const QString& ext)
{
    d->defaultExtension = ext;
}

bool KexiFileWidgetInterface::confirmOverwrites() const
{
    return d->confirmOverwrites;
}

void KexiFileWidgetInterface::setConfirmOverwrites(bool set)
{
    d->confirmOverwrites = set;
}

QString KexiFileWidgetInterface::currentDir() const
{
    qFatal("Implement it");
    return QString();
}

void KexiFileWidgetInterface::setFiltersUpdated(bool set)
{
    d->filtersUpdated = set;
}

bool KexiFileWidgetInterface::filtersUpdated() const
{
    return d->filtersUpdated;
}

KexiFileFilters* KexiFileWidgetInterface::filters()
{
    return &d->filters;
}

const KexiFileFilters* KexiFileWidgetInterface::filters() const
{
    return &d->filters;
}

void KexiFileWidgetInterface::connectFileHighlightedSignal(QObject *receiver, const char *slot)
{
    QObject::connect(widget(), SIGNAL(fileHighlighted(QString)), receiver, slot);
}

void KexiFileWidgetInterface::connectFileSelectedSignal(QObject *receiver, const char *slot)
{
    QObject::connect(widget(), SIGNAL(fileSelected(QString)), receiver, slot);
}

bool KexiFileWidgetInterface::checkSelectedFile()
{
    qDebug() << "selectedFile:" << selectedFile();

    applyEnteredFileName();

    qDebug() << "selectedFile after applyEnteredFileName():" << selectedFile();
    
    if (selectedFile().isEmpty()) {
        KMessageBox::error(widget(), xi18n("Enter a filename."));
        return false;
    }

    if (filters()->mode() == KexiFileFilters::SavingFileBasedDB || filters()->mode() == KexiFileFilters::CustomSavingFileBasedDB) {
        const QStringList currentFilters(this->currentFilters());
        if (!currentFilters.isEmpty()) {
            QString path = selectedFile();
            qDebug()<< "filter:" << currentFilters << "path:" << path;
            QString ext(QFileInfo(path).suffix());
            bool hasExtension = false;
            for (const QString &filter : currentFilters) {
                const QString f(filter.trimmed());
                hasExtension = !f.midRef(2).isEmpty() && ext == f.midRef(2);
                if (hasExtension) {
                    break;
                }
            }
            if (!hasExtension) {
                //no extension: add one
                QString ext(defaultExtension());
                if (ext.isEmpty()) {
                    ext = currentFilters.first().trimmed().mid(2); //first one
                }
                path += (QLatin1String(".") + ext);
                qDebug() << "appended extension:" << path;
                setSelectedFile(path);
            }
            qDebug() << "selectedFile after applying extension:" << selectedFile();
        }
    }

    if (filters()->isExistingFileRequired()) {
        QFileInfo fi(selectedFile());
        if (!fi.exists()) {
            KMessageBox::error(widget(),
                               xi18nc("@info", "The file <filename>%1</filename> does not exist.",
                                      QDir::toNativeSeparators(fi.absoluteFilePath())));
            return false;
        }
        if (!fi.isFile()) {
            KMessageBox::error(widget(), xi18nc("@info", "Enter a filename."));
            return false;
        }
        if (!fi.isReadable()) {
            KMessageBox::error(widget(),
                               xi18nc("@info", "The file <filename>%1</filename> is not readable.",
                                      QDir::toNativeSeparators(fi.absoluteFilePath())));
            return false;
        }
    } else if (confirmOverwrites() && !KexiUtils::askForFileOverwriting(selectedFile(), widget())) {
        return false;
    }
    return true;
}
