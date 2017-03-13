/* This file is part of the KDE project
   Copyright (C) 2016-2017 Jarosław Staniek <staniek@kde.org>

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

#include "KexiFileRequester.h"
#include <KexiFileFilters.h>
#include <kexiutils/utils.h>
#include <KexiIcon.h>

#include <KFileWidget>
#include <KLineEdit>
#include <KLocalizedString>
#include <KRecentDirs>

#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QLabel>
#include <QMimeDatabase>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTreeView>
#include <QVBoxLayout>

namespace {
    enum KexiFileSystemModelColumnIds {
        NameColumnId,
        LastModifiedColumnId
    };
}

//! A model for KexiFileRequester
class KexiFileSystemModel : public QFileSystemModel
{
    Q_OBJECT
public:
    explicit KexiFileSystemModel(QWidget *parent = nullptr)
        : QFileSystemModel(parent)
    {
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent)
        return 2;
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        const int col = index.column();
        if (col == NameColumnId) {
            switch (role) {
            case Qt::DecorationRole: {
                if (isDir(index)) {
                    return koIcon("folder");
                } else {
                    QMimeDatabase db;
                    return QIcon::fromTheme(db.mimeTypeForFile(filePath(index)).iconName());
                }
            }
            default: break;
            }
            return QFileSystemModel::data(index, role);
        } else if (col == LastModifiedColumnId) {
            const QWidget *parentWidget = qobject_cast<QWidget*>(QObject::parent());
            switch (role) {
            case Qt::DisplayRole:
                return parentWidget->locale().toString(QFileSystemModel::lastModified(index),
                                                       QLocale::ShortFormat);
            default:
                break;
            }
        }
        return QVariant();
    }
    Qt::ItemFlags flags(const QModelIndex& index) const override {
        Q_UNUSED(index)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
};

//! @internal
class Q_DECL_HIDDEN KexiFileRequester::Private : public QObject
{
    Q_OBJECT
public:
    Private(KexiFileRequester *r) : q(r)
    {
    }
    void updateFilter()
    {
        model->setNameFilters(filters.allGlobPatterns());
    }

    static QString urlToPath(const QUrl &url)
    {
        QString filePath = QDir::toNativeSeparators(url.path(QUrl::RemoveScheme | QUrl::PreferLocalFile | QUrl::StripTrailingSlash));
#ifdef Q_OS_WIN
        if (filePath.startsWith('\\')) {
            filePath = filePath.mid(1);
        } else if (filePath.startsWith("file:\\")) {
            filePath = filePath.mid(6);
        }
#endif
        return filePath;
    }

public Q_SLOTS:
    void updateUrl(const QUrl &url)
    {
        updateFileName(urlToPath(url));
    }

    void updateFileName(const QString &filePath)
    {
        const QFileInfo fileInfo(filePath);
        QString dirPath;
        if (fileInfo.isDir()) {
            dirPath = fileInfo.absoluteFilePath();
        } else {
            dirPath = fileInfo.absolutePath();
        }
        dirPath = QDir::toNativeSeparators(dirPath);
        if (filePath.isEmpty()) { // display Windows Explorer's "Computer" folder name for the top level
#ifdef Q_OS_WIN
            QString computerNameString = QSettings(
                "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
                "CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}",
                        QSettings::NativeFormat).value("Default").toString();
            if (computerNameString.isEmpty()) {
                 computerNameString = xi18n("Computer");
            }
            urlLabel->setText(computerNameString);
            folderIcon->setPixmap(koSmallIcon("computer"));
            upButton->setEnabled(false);
#else
            urlLabel->setText("/");
            folderIcon->setPixmap(koSmallIcon("folder"));
            upButton->setEnabled(false);
#endif
        } else {
            urlLabel->setText(dirPath);
            folderIcon->setPixmap(koSmallIcon("folder"));
            upButton->setEnabled(filePath != "/");
        }
        model->setRootPath(dirPath);
        list->setRootIndex(model->index(filePath));
        list->resizeColumnToContents(LastModifiedColumnId);
    }

    void itemActivated(const QModelIndex & index)
    {
        const QString filePath(model->filePath(index));
        if (model->isDir(index)) {
            updateFileName(filePath);
        } else {
            emit q->fileSelected(filePath);
        }
    }

    void upButtonClicked()
    {
        QString dirPath(urlLabel->text());
        QDir dir(dirPath);
        if (dirPath.isEmpty() || !dir.cdUp()) {
            updateFileName(QString());
        } else {
            updateFileName(dir.absolutePath());
        }
        //! @todo update button enable flag
    }

    void selectUrlButtonClicked()
    {
        QUrl dirUrl;
#ifdef Q_OS_WIN
        if (!upButton->isEnabled()) { // Computer folder, see http://doc.qt.io/qt-5/qfiledialog.html#setDirectoryUrl
            dirUrl = QUrl("clsid:0AC0837C-BBF8-452A-850D-79D08E667CA7");
        }
#else
        if (false) {
        }
#endif
        else {
            dirUrl = QUrl::fromLocalFile(urlLabel->text());
        }
        QUrl selectedUrl = QFileDialog::getExistingDirectoryUrl(q, QString(), dirUrl);
        if (selectedUrl.isLocalFile()) {
            updateFileName(selectedUrl.toLocalFile());
        }
    }

public:
    KexiFileRequester* const q;
    QString recentDirClass;
    QPushButton *upButton;
    QLabel *folderIcon;
    QLabel *urlLabel;
    QPushButton *selectUrlButton;
    KexiFileSystemModel *model;
    QTreeView *list;
    KexiFileFilters filters;
};

KexiFileRequester::KexiFileRequester(const QUrl &fileOrVariable, QWidget *parent)
    : QWidget(parent), d(new Private(this))
{
    init();
    QUrl url;
    if (fileOrVariable.scheme() == "kfiledialog") {
        url = KFileWidget::getStartUrl(fileOrVariable, d->recentDirClass);
    } else {
        url = fileOrVariable;
    }
    const QString fileName = Private::urlToPath(url);
    d->updateFileName(fileName);
}

KexiFileRequester::KexiFileRequester(const QString &selectedFileName, QWidget *parent)
    : QWidget(parent), d(new Private(this))
{
    init();
    d->updateFileName(selectedFileName);
}

KexiFileRequester::~KexiFileRequester()
{
    const QString startDir(d->urlLabel->text());
    if (QDir(startDir).exists()) {
        KRecentDirs::add(d->recentDirClass, startDir);
    }
    delete d;
}

void KexiFileRequester::init()
{
    // [^] [Dir ][..]
    // [ files      ]
    QVBoxLayout *lyr = new QVBoxLayout(this);
    setContentsMargins(lyr->contentsMargins());
    lyr->setContentsMargins(QMargins());
    QHBoxLayout *urlLyr = new QHBoxLayout;
    urlLyr->setContentsMargins(QMargins());
    lyr->addLayout(urlLyr);

    d->upButton = new QPushButton;
    d->upButton->setFocusPolicy(Qt::NoFocus);
    d->upButton->setIcon(koIcon("go-up"));
    d->upButton->setToolTip(xi18n("Go to parent directory"));
    d->upButton->setFlat(true);
    connect(d->upButton, &QPushButton::clicked, d, &KexiFileRequester::Private::upButtonClicked);
    urlLyr->addWidget(d->upButton);

    d->folderIcon = new QLabel;
    urlLyr->addWidget(d->folderIcon);

    d->urlLabel = new QLabel;
    d->urlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    d->urlLabel->setWordWrap(true);
    d->urlLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    urlLyr->addWidget(d->urlLabel, 1);

    d->selectUrlButton = new QPushButton;
    d->selectUrlButton->setFocusPolicy(Qt::NoFocus);
    d->selectUrlButton->setIcon(koIcon("folder"));
    d->selectUrlButton->setToolTip(xi18n("Select directory"));
    d->selectUrlButton->setFlat(true);
    connect(d->selectUrlButton, &QPushButton::clicked, d, &KexiFileRequester::Private::selectUrlButtonClicked);
    urlLyr->addWidget(d->selectUrlButton);

    d->list = new QTreeView;
    connect(d->list,
            KexiUtils::activateItemsOnSingleClick(d->list) ? &QTreeView::clicked : &QTreeView::activated,
            d, &KexiFileRequester::Private::itemActivated);
    d->list->setRootIsDecorated(false);
    d->list->setItemsExpandable(false);
    d->list->header()->hide();
    lyr->addWidget(d->list);
    d->model = new KexiFileSystemModel(d->list);
    d->model->setNameFilterDisables(false);

    d->list->setModel(d->model);
    d->list->header()->setStretchLastSection(false);
    d->list->header()->setSectionResizeMode(NameColumnId, QHeaderView::Stretch);
    d->list->header()->setSectionResizeMode(LastModifiedColumnId, QHeaderView::ResizeToContents);
}

QString KexiFileRequester::selectedFileName() const
{
    const QModelIndexList list(d->list->selectionModel()->selectedIndexes());
    if (list.isEmpty()) {
        return QString();
    }
    const QModelIndex index(list.first());
    if (d->model->isDir(index)) {
        return QString();
    }
    return d->model->filePath(index);
}

void KexiFileRequester::setSelectedFileName(const QString &selectedFileName)
{
    d->updateFileName(selectedFileName);
}

void KexiFileRequester::setFileMode(KexiFileFilters::Mode mode)
{
    KFile::Modes fileModes = KFile::File | KFile::LocalOnly;
    if (mode == KexiFileFilters::Opening || mode == KexiFileFilters::CustomOpening) {
        fileModes |= KFile::ExistingOnly;
    }
    d->filters.setMode(mode);
    d->updateFilter();
}

QStringList KexiFileRequester::additionalMimeTypes() const
{
    return d->filters.additionalMimeTypes();
}

QStringList KexiFileRequester::excludedMimeTypes() const
{
    return d->filters.excludedMimeTypes();
}

QString KexiFileRequester::defaultFilter() const
{
    return d->filters.defaultFilter();
}

void KexiFileRequester::setExcludedMimeTypes(const QStringList &mimeTypes)
{
    d->filters.setExcludedMimeTypes(mimeTypes);
    d->updateFilter();
}

void KexiFileRequester::setAdditionalMimeTypes(const QStringList &mimeTypes)
{
    d->filters.setAdditionalMimeTypes(mimeTypes);
    d->updateFilter();
}

void KexiFileRequester::setDefaultFilter(const QString &filter)
{
    d->filters.setDefaultFilter(filter);
    d->updateFilter();
}

void KexiFileRequester::setFrame(bool frame)
{
    d->list->setFrameShape(frame ? QFrame::StyledPanel : QFrame::NoFrame);
}

#include "KexiFileRequester.moc"
