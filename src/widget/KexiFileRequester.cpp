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

#include <KFileFilterCombo>
#include <KFileWidget>
#include <KLineEdit>
#include <KLocalizedString>
#include <KRecentDirs>
#include <KUrlComboBox>
#include <KUrlCompletion>
#include <KMessageBox>

#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QLabel>
#include <QMimeDatabase>
#include <QPushButton>
#include <QRegExp>
#include <QSettings>
#include <QStandardPaths>
#include <QTreeView>
#include <QVBoxLayout>

#include <functional>

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
    explicit KexiFileSystemModel(QObject *parent = nullptr)
        : QFileSystemModel(parent)
    {
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return LastModifiedColumnId + 1;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        const int col = index.column();
        if (col == NameColumnId) {
            switch (role) {
            case Qt::DecorationRole: {
                if (isDir(index)) {
                    return koIcon("folder");
                } else {
                    return QIcon::fromTheme(m_mimeDb.mimeTypeForFile(filePath(index)).iconName());
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

private:
    QMimeDatabase m_mimeDb;
};

//! @internal
class KexiUrlCompletion : public KUrlCompletion
{
public:
    explicit KexiUrlCompletion(QList<QRegExp *> *filterRegExps, QList<QMimeType> *filterMimeTypes)
        : KUrlCompletion(KUrlCompletion::FileCompletion)
        , m_filterRegExps(filterRegExps)
        , m_filterMimeTypes(filterMimeTypes)
    {
    }

    using KUrlCompletion::postProcessMatches;

    //! Reimplemented to match the filter
    void postProcessMatches(QStringList *matches) const override
    {
        for (QStringList::Iterator matchIt = matches->begin();
             matchIt != matches->end();)
        {
            if (fileMatchesFilter(*matchIt)) {
                ++matchIt;
            } else {
                matchIt = matches->erase(matchIt);
            }
        }
    }

private:
    /**
     * @return @c true if @a fileName matches the current regular expression as well as the mime types
     *
     * The mime type matching allows to overcome issues with patterns such as *.doc being used
     * for text/plain mime types but really belonging to application/msword mime type.
     */
    bool fileMatchesFilter(const QString &fileName) const
    {
        bool found = false;
        for (QRegExp *regexp : *m_filterRegExps) {
            if (regexp->exactMatch(fileName)) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
        const QMimeType mimeType(m_mimeDb.mimeTypeForFile(fileName));
        qDebug() << mimeType;
        int i = m_filterMimeTypes->indexOf(mimeType);
        return i >= 0;
    }

    const QList<QRegExp*> * const m_filterRegExps;
    const QList<QMimeType> * const m_filterMimeTypes;
    QMimeDatabase m_mimeDb;
};

//! @internal
class Q_DECL_HIDDEN KexiFileRequester::Private : public QObject
{
    Q_OBJECT
public:
    Private(KexiFileRequester *r) : q(r)
    {
    }

    ~Private()
    {
        qDeleteAll(filterRegExps);
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
        if (model->rootPath() != dirPath) {
            model->setRootPath(dirPath);
            list->setRootIndex(model->index(filePath));
            list->resizeColumnToContents(LastModifiedColumnId);
            urlCompletion->setDir(QUrl::fromLocalFile(dirPath));
        }
        const QModelIndex fileIndex = model->index(filePath);
        list->scrollTo(fileIndex);
        list->selectionModel()->select(fileIndex, QItemSelectionModel::ClearAndSelect);
    }

    void itemClicked(const QModelIndex &index)
    {
        handleItem(index, std::bind(&KexiFileRequester::fileHighlighted, q, std::placeholders::_1),
                   true);
        if (activateItemsOnSingleClick) {
            handleItem(index, std::bind(&KexiFileRequester::fileSelected, q, std::placeholders::_1),
                       false);
        }
    }

    void itemActivated(const QModelIndex &index)
    {
        if (!activateItemsOnSingleClick) {
            handleItem(index, std::bind(&KexiFileRequester::fileSelected, q, std::placeholders::_1),
                       true);
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

    void locationEditTextChanged(const QString &text)
    {
        locationEdit->lineEdit()->setModified(true);
        if (text.isEmpty()) {
            list->clearSelection();
        }
        QFileInfo info(model->rootPath() + '/' + text);
        if (info.isFile() && model->rootDirectory().exists(text)) {
            updateFileName(model->rootDirectory().absoluteFilePath(text)); // select file
        } else {
            updateFileName(model->rootPath()); // only dir, unselect file
        }
    }

    void locationEditReturnPressed()
    {
        QString text(locationEdit->lineEdit()->text());
        if (text.isEmpty()) {
            return;
        }
        if (text == QStringLiteral("~")) {
            text = QDir::homePath();
        } else if (text.startsWith(QStringLiteral("~/"))) {
            text = QDir::home().absoluteFilePath(text.mid(2));
        }
        if (QDir::isAbsolutePath(text)) {
            QFileInfo info(text);
            if (!info.isReadable()) {
                return;
            }
            if (info.isDir()) { // jump to absolute dir and clear the editor
                updateFileName(info.canonicalFilePath());
                locationEdit->lineEdit()->clear();
            } else { // jump to absolute dir and select the file in it
                updateFileName(info.dir().canonicalPath());
                locationEdit->lineEdit()->setText(info.fileName());
                locationEditReturnPressed();
            }
        } else { // relative path
            QFileInfo info(model->rootPath() + '/' + text);
            if (info.isReadable() && info.isDir()) { // jump to relative dir and clear the editor
                updateFileName(info.canonicalFilePath());
                locationEdit->lineEdit()->clear();
            } else { // emit the file selection
                //not needed - preselected: updateFileName(text);
                emit q->fileSelected(q->selectedFile());
            }
        }
    }

    void slotFilterComboChanged()
    {
        const QStringList patterns = filterCombo->currentFilter().split(' ');
        //qDebug() << patterns;
        model->setNameFilters(patterns);
        qDeleteAll(filterRegExps);
        filterRegExps.clear();
        for (const QString &pattern : patterns) {
            filterRegExps.append(new QRegExp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard));
        }
    }

private:
    void handleItem(const QModelIndex &index, std::function<void(const QString&)> sig, bool silent)
    {
        const QString filePath(model->filePath(index));
        if (model->isDir(index)) {
            QFileInfo info(filePath);
            if (info.isReadable()) {
                updateFileName(filePath);
            } else {
                if (silent) {
                    KMessageBox::error(q,
                                       xi18n("Could not enter directory <filename>%1</filename>.",
                                             QDir::toNativeSeparators(info.absoluteFilePath())));
                }
            }
        } else {
            emit sig(filePath);
        }
    }

public:
    KexiFileRequester* const q;
    QPushButton *upButton;
    QLabel *folderIcon;
    QLabel *urlLabel;
    QPushButton *selectUrlButton;
    KexiFileSystemModel *model;
    QTreeView *list;
    bool activateItemsOnSingleClick;
    KUrlComboBox *locationEdit;
    KexiUrlCompletion *urlCompletion;
    KFileFilterCombo *filterCombo;
    QList<QRegExp*> filterRegExps; //!< Regular expression for the completer in the URL box
    QList<QMimeType> filterMimeTypes;
};

KexiFileRequester::KexiFileRequester(const QUrl &fileOrVariable, KexiFileFilters::Mode mode,
                                     QWidget *parent)
    : QWidget(parent), KexiFileWidgetInterface(fileOrVariable), d(new Private(this))
{
    init();
    const QString fileName = Private::urlToPath(startUrl());
    d->updateFileName(fileName);
    setMode(mode);
}

KexiFileRequester::KexiFileRequester(const QString &selectedFileName, KexiFileFilters::Mode mode,
                                     QWidget *parent)
    : QWidget(parent), KexiFileWidgetInterface(QUrl(selectedFileName)), d(new Private(this))
{
    init();
    d->updateFileName(selectedFileName);
    setMode(mode);
}

KexiFileRequester::~KexiFileRequester()
{
    const QString startDir(d->urlLabel->text());
    addRecentDir(startDir);
    delete d;
}

void KexiFileRequester::init()
{
    // [^] [Dir    ][..]
    // [ files list    ]
    // [ location      ]
    // [ filter combo  ]
    QVBoxLayout *lyr = new QVBoxLayout(this);
    setContentsMargins(QMargins());
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
    d->activateItemsOnSingleClick = KexiUtils::activateItemsOnSingleClick(d->list);
    connect(d->list, &QTreeView::clicked, d, &KexiFileRequester::Private::itemClicked);
    connect(d->list, &QTreeView::activated, d, &KexiFileRequester::Private::itemActivated);
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

    QGridLayout *bottomLyr = new QGridLayout;
    lyr->addLayout(bottomLyr);

    QLabel *locationLabel = new QLabel(xi18n("Name:"));
    bottomLyr->addWidget(locationLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
    d->locationEdit = new KUrlComboBox(KUrlComboBox::Files, true);
    d->locationEdit->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    connect(d->locationEdit, &KUrlComboBox::editTextChanged, d,
            &KexiFileRequester::Private::locationEditTextChanged);
    connect(d->locationEdit, QOverload<>::of(&KUrlComboBox::returnPressed),
            d, &Private::locationEditReturnPressed);
    d->urlCompletion = new KexiUrlCompletion(&d->filterRegExps, &d->filterMimeTypes);
    d->locationEdit->setCompletionObject(d->urlCompletion);
    d->locationEdit->setAutoDeleteCompletionObject(true);
    d->locationEdit->lineEdit()->setClearButtonEnabled(true);
    locationLabel->setBuddy(d->locationEdit);
    bottomLyr->addWidget(d->locationEdit, 0, 1, Qt::AlignVCenter);

    QLabel *filterLabel = new QLabel(xi18n("Filter:"));
    bottomLyr->addWidget(filterLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
    d->filterCombo = new KFileFilterCombo;
    connect(d->filterCombo, &KFileFilterCombo::filterChanged,
            d, &Private::slotFilterComboChanged);
    filterLabel->setBuddy(d->filterCombo);
    bottomLyr->addWidget(d->filterCombo, 1, 1, Qt::AlignVCenter);
}

QString KexiFileRequester::selectedFile() const
{
    const QModelIndexList list(d->list->selectionModel()->selectedIndexes());
    if (list.isEmpty()) {
        return QString();
    }
    if (d->model->isDir(list.first())) {
        return QString();
    }
    return d->model->filePath(list.first());
}

QString KexiFileRequester::highlightedFile() const
{
    return selectedFile();
}

QString KexiFileRequester::currentDir() const
{
    return d->model->rootPath();
}

void KexiFileRequester::setSelectedFile(const QString &name)
{
    d->updateFileName(name);
}

void KexiFileRequester::updateFilters()
{
    // Update filters for the file model, filename completion and the filter combo
    const QStringList patterns = filters()->allGlobPatterns();
    if (patterns != d->model->nameFilters()) {
        d->model->setNameFilters(patterns);
        qDeleteAll(d->filterRegExps);
        d->filterRegExps.clear();
        for (const QString &pattern : patterns) {
            d->filterRegExps.append(new QRegExp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard));
        }
        d->filterMimeTypes = filters()->mimeTypes();
        d->filterCombo->setFilter(filters()->toString(KexiFileFilters::KDEFormat));
    }
}

void KexiFileRequester::setWidgetFrame(bool set)
{
    d->list->setFrameShape(set ? QFrame::StyledPanel : QFrame::NoFrame);
    d->list->setLineWidth(set ? 1 : 0);
}

void KexiFileRequester::applyEnteredFileName()
{
}

QStringList KexiFileRequester::currentFilters() const
{
    return QStringList();
}

void KexiFileRequester::showEvent(QShowEvent *event)
{
    setFiltersUpdated(false);
    updateFilters();
    QWidget::showEvent(event);
}

#include "KexiFileRequester.moc"
