/* This file is part of the KDE project
   Copyright (C) 2003-2017 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2012 Dimitrios T. Tanis <dimitrios.tanis@kdemail.net>

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

#include "KexiConnectionSelectorWidget.h"
#include "ui_KexiConnectionSelector.h"
#include "kexiprjtypeselector.h"
#include "kexidbconnectionwidget.h"
#include "KexiFileWidgetInterface.h"
#include <kexiutils/utils.h>
#include <core/kexi.h>
#include <KexiIcon.h>
#include <KexiServerDriverNotFoundMessage.h>

#include <KDbDriverManager>
#include <KDbDriverMetaData>
#include <KDbConnectionData>
#include <KDbUtils>
#include <KDbMessageHandler>

#include <KMessageBox>
#include <KUrlComboBox>

#include <QDebug>
#include <QDialog>
#include <QPushButton>
#include <QLayout>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QPixmap>
#include <QFrame>
#include <QStackedWidget>
#include <QKeyEvent>

class KexiConnectionSelector : public QWidget, public Ui_KexiConnectionSelector
{
    Q_OBJECT
public:
    explicit KexiConnectionSelector(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
        setObjectName("conn_sel");
        lblIcon->setPixmap(koDesktopIconCStr(Kexi::serverIconName()));
        lblIcon->setFixedSize(lblIcon->pixmap()->size());
        btn_add->setToolTip(xi18n("Add a new database connection"));
        btn_edit->setToolTip(xi18n("Edit selected database connection"));
        btn_remove->setToolTip(xi18n("Delete selected database connections"));
    }
    ~KexiConnectionSelector()
    {
    }
};

/*================================================================*/

ConnectionDataLVItem::ConnectionDataLVItem(KDbConnectionData *data,
                                           const KDbDriverMetaData &driverMetaData,
                                           QTreeWidget* list)
        : QTreeWidgetItem(list)
        , m_data(data)
{
    update(driverMetaData);
}

ConnectionDataLVItem::~ConnectionDataLVItem()
{
}

void ConnectionDataLVItem::update(const KDbDriverMetaData &driverMetaData)
{
    setText(0, m_data->caption() + "  ");
    const QString sfile = xi18n("File");
    QString driverName = driverMetaData.name();
    QString column1;
    if (driverMetaData.isFileBased()) {
        column1 = xi18nc("file (driver name)", "%1 (%2)", sfile, driverName);
    } else {
        column1 = driverName;
    }
    setText(1, column1 + "  ");
    setText(2, (driverMetaData.isFileBased() ? QString("<%1>").arg(sfile.toLower())
                                             : m_data->toUserVisibleString()) + "  ");
}

/*================================================================*/

//! @internal
class Q_DECL_HIDDEN KexiConnectionSelectorWidget::Private
{
public:
    Private()
            : conn_sel_shown(false)
            , confirmOverwrites(true)
    {
    }

    void updateRemoteListColumns()
    {
        remote->list->resizeColumnToContents(0); // name
        remote->list->resizeColumnToContents(1); // type
    }

    QWidget *fileWidget()
    {
        return fileIface ? fileIface->widget() : nullptr;
    }

    KexiFileWidgetInterface *fileIface = nullptr;
    KexiConnectionSelector *remote;
    QWidget* openExistingWidget;
    KexiPrjTypeSelector* prjTypeSelector;
    QUrl startDirOrVariable;
    KexiConnectionSelectorWidget::OperationMode operationMode;
    QStackedWidget *stack;
    QPointer<KexiDBConnectionSet> conn_set;
    KDbDriverManager manager;
    bool conn_sel_shown; //!< helper
    bool confirmOverwrites;
    KexiUtils::PaintBlocker* descGroupBoxPaintBlocker;
    bool isConnectionSelected;
    bool fileWidgetFrameVisible = true;
    QPointer<KexiServerDriverNotFoundMessage> errorMessagePopup;
};

/*================================================================*/

KexiConnectionSelectorWidget::KexiConnectionSelectorWidget(
    KexiDBConnectionSet *conn_set,
    const QUrl& startDirOrVariable, OperationMode mode, QWidget* parent)
    : QWidget(parent)
    , d(new Private())
{
    Q_ASSERT(conn_set);
    d->conn_set = conn_set;
    d->startDirOrVariable = startDirOrVariable;
    d->operationMode = mode;
    setWindowIcon(Kexi::defaultFileBasedDriverIcon());

    QBoxLayout* globalLyr = new QVBoxLayout(this);
    globalLyr->setContentsMargins(QMargins());

    //create header with radio buttons
    d->openExistingWidget = new QWidget(this);
    d->openExistingWidget->setObjectName("openExistingWidget");
    QVBoxLayout* openExistingWidgetLyr = new QVBoxLayout(d->openExistingWidget);
    openExistingWidgetLyr->setContentsMargins(0, 0, 0, 0);
    d->prjTypeSelector = new KexiPrjTypeSelector(d->openExistingWidget);
    connect(d->prjTypeSelector->buttonGroup, SIGNAL(buttonClicked(QAbstractButton*)),
            this, SLOT(slotPrjTypeSelected(QAbstractButton*)));
    openExistingWidgetLyr->addWidget(d->prjTypeSelector);
    d->prjTypeSelector->setContentsMargins(0, 0, 0, KexiUtils::spacingHint());
    //openExistingWidgetLyr->addSpacing(KexiUtils::spacingHint());
    QFrame* line = new QFrame(d->openExistingWidget);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    openExistingWidgetLyr->addWidget(line);
    globalLyr->addWidget(d->openExistingWidget);

    d->stack = new QStackedWidget(this);
    d->stack->setObjectName("stack");
    globalLyr->addWidget(d->stack, 1);

    d->remote = new KexiConnectionSelector(d->stack);
    connect(d->remote->btn_add, SIGNAL(clicked()), this, SLOT(slotRemoteAddBtnClicked()));
    connect(d->remote->btn_edit, SIGNAL(clicked()), this, SLOT(slotRemoteEditBtnClicked()));
    connect(d->remote->btn_remove, SIGNAL(clicked()), this, SLOT(slotRemoteRemoveBtnClicked()));
    d->stack->addWidget(d->remote);
    if (d->remote->layout())
        d->remote->layout()->setMargin(0);
    connect(d->remote->list, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(slotConnectionItemExecuted(QTreeWidgetItem*)));
    connect(d->remote->list, SIGNAL(itemSelectionChanged()),
            this, SLOT(slotConnectionSelectionChanged()));
    d->remote->list->installEventFilter(this);
    d->descGroupBoxPaintBlocker = new KexiUtils::PaintBlocker(d->remote->descGroupBox);
    d->descGroupBoxPaintBlocker->setEnabled(false);
    d->isConnectionSelected = false;
}

KexiConnectionSelectorWidget::~KexiConnectionSelectorWidget()
{
    delete d;
}

void KexiConnectionSelectorWidget::showAdvancedConnection()
{
    d->prjTypeSelector->option_server->setChecked(true);
    slotPrjTypeSelected(d->prjTypeSelector->option_server);
}

void KexiConnectionSelectorWidget::slotPrjTypeSelected(QAbstractButton *btn)
{
    if (btn == d->prjTypeSelector->option_file) { //file-based prj type
        showSimpleConnection();
    } else if (btn == d->prjTypeSelector->option_server) { //server-based prj type
        if (KDbDriverManager().hasDatabaseServerDrivers()) {
            if (!d->conn_sel_shown) {
                d->conn_sel_shown = true;
                //show connections (on demand):
                foreach(KDbConnectionData* connData, d->conn_set->list()) {
                    addConnectionData(connData);
                    //   else {
                    //this error should be more verbose:
                    //    qWarning() << "no driver found for '" << it.current()->driverName << "'!";
                    //   }
                }
                if (d->remote->list->topLevelItemCount() > 0) {
                    d->updateRemoteListColumns();
                    d->remote->list->sortByColumn(0, Qt::AscendingOrder);
                    d->remote->list->topLevelItem(0)->setSelected(true);
                }
                d->remote->descGroupBox->layout()->setMargin(2);
                d->remote->list->setFocus();
                slotConnectionSelectionChanged();
            }
            d->stack->setCurrentWidget(d->remote);
        }
        else {
            if (!d->errorMessagePopup) {
                QWidget *errorMessagePopupParent = new QWidget(this);
                QVBoxLayout *vbox = new QVBoxLayout(errorMessagePopupParent);
                d->errorMessagePopup = new KexiServerDriverNotFoundMessage(errorMessagePopupParent);
                vbox->addWidget(d->errorMessagePopup);
                vbox->addStretch(0);
                d->stack->addWidget(errorMessagePopupParent);
                d->errorMessagePopup->setAutoDelete(false);
                d->stack->setCurrentWidget(d->errorMessagePopup->parentWidget());
                d->errorMessagePopup->animatedShow();
            }
            else {
                d->stack->setCurrentWidget(d->errorMessagePopup->parentWidget());
            }
        }
    }
}

ConnectionDataLVItem* KexiConnectionSelectorWidget::addConnectionData(KDbConnectionData* data)
{
    const KDbDriverMetaData* driverMetaData = d->manager.driverMetaData(data->driverId());
    return driverMetaData ?
                new ConnectionDataLVItem(data, *driverMetaData, d->remote->list) : 0;
}

void KexiConnectionSelectorWidget::showSimpleConnection()
{
    d->prjTypeSelector->option_file->setChecked(true);
    if (!d->fileIface) {
        d->fileIface = KexiFileWidgetInterface::createWidget(
            d->startDirOrVariable, d->operationMode == Opening ? KexiFileFilters::Opening
                                                               : KexiFileFilters::SavingFileBasedDB,
            d->stack);
        d->fileIface->setWidgetFrame(d->fileWidgetFrameVisible);
        d->fileIface->setConfirmOverwrites(d->confirmOverwrites);
        d->stack->addWidget(d->fileIface->widget());
        d->fileIface->connectFileSelectedSignal(this, SLOT(slotFileConnectionSelected(QString)));
    }
    d->stack->setCurrentWidget(d->fileIface->widget());
}

void KexiConnectionSelectorWidget::setFileWidgetFrameVisible(bool set)
{
    d->fileWidgetFrameVisible = set;
    if (d->fileIface) {
        d->fileIface->setWidgetFrame(d->fileWidgetFrameVisible);
    }
}

KexiConnectionSelectorWidget::ConnectionType KexiConnectionSelectorWidget::selectedConnectionType() const
{
    return (d->stack->currentWidget() == d->fileWidget()) ? FileBased : ServerBased;
}

KDbConnectionData* KexiConnectionSelectorWidget::selectedConnectionData() const
{
    QList<QTreeWidgetItem *> items = d->remote->list->selectedItems();
    if (items.isEmpty())
        return 0;
    ConnectionDataLVItem *item = static_cast<ConnectionDataLVItem*>(items.first());
    if (!item)
        return 0;
    return item->data();
}

QString KexiConnectionSelectorWidget::selectedFile() const
{
    if (selectedConnectionType() != KexiConnectionSelectorWidget::FileBased) {
        return QString();
    }
    return d->fileIface->selectedFile();
}

void KexiConnectionSelectorWidget::setSelectedFile(const QString& name)
{
    if (selectedConnectionType() != KexiConnectionSelectorWidget::FileBased) {
        return;
    }
    return d->fileIface->setSelectedFile(name);
}

void KexiConnectionSelectorWidget::slotConnectionItemExecuted(QTreeWidgetItem* item)
{
    emit connectionItemExecuted(static_cast<ConnectionDataLVItem*>(item));
    slotConnectionSelected();
}

void KexiConnectionSelectorWidget::slotConnectionItemExecuted()
{
    QList<QTreeWidgetItem *> items = d->remote->list->selectedItems();
    if (items.isEmpty())
        return;
    slotConnectionItemExecuted(items.first());
    slotConnectionSelected();
}

void KexiConnectionSelectorWidget::slotConnectionSelectionChanged()
{
    QList<QTreeWidgetItem *> items = d->remote->list->selectedItems();
    if (items.isEmpty())
        return;
    ConnectionDataLVItem* item = static_cast<ConnectionDataLVItem*>(items.first());
    d->remote->btn_edit->setEnabled(item);
    d->remote->btn_remove->setEnabled(item);
    QString desc;
    if (item) {
        desc = item->data()->description();
    }
    d->descGroupBoxPaintBlocker->setEnabled(desc.isEmpty());
    d->remote->descriptionLabel->setText(desc);
    emit connectionSelected(d->isConnectionSelected);
    emit connectionItemHighlighted(item);
}

QTreeWidget* KexiConnectionSelectorWidget::connectionsList() const
{
    return d->remote->list;
}

void KexiConnectionSelectorWidget::setFocus()
{
    QWidget::setFocus();
    if (d->stack->currentWidget() == d->fileWidget()) {
        d->fileWidget()->setFocus();
    } else {
        d->remote->list->setFocus();
    }
}

void KexiConnectionSelectorWidget::hideHelpers()
{
    d->openExistingWidget->hide();
}

void KexiConnectionSelectorWidget::setConfirmOverwrites(bool set)
{
    d->confirmOverwrites = set;
    if (d->fileIface) {
        d->fileIface->setConfirmOverwrites(d->confirmOverwrites);
    }
}

bool KexiConnectionSelectorWidget::confirmOverwrites() const
{
    return d->confirmOverwrites;
}

void KexiConnectionSelectorWidget::slotRemoteAddBtnClicked()
{
    KDbConnectionData data;
    KexiDBConnectionDialog dlg(this, data, QString(),
                               KGuiItem(xi18nc("@action:button Add Database Connection", "&Add"), koIconName("dialog-ok"), xi18n("Add database connection")));
    dlg.setWindowTitle(xi18nc("@title:window", "Add a New Database Connection"));
    if (QDialog::Accepted != dlg.exec())
        return;

    //store this conn. data
    KDbConnectionData *newData
        = new KDbConnectionData(*dlg.currentProjectData().connectionData());
    KDbMessageGuard mg(d->conn_set);
    if (!d->conn_set->addConnectionData(newData)) {
        delete newData;
        return;
    }

    ConnectionDataLVItem* item = addConnectionData(newData);
    if (item) {
        d->remote->list->clearSelection();
        d->updateRemoteListColumns();
        item->setSelected(true);
        slotConnectionSelectionChanged();
    }
}

void KexiConnectionSelectorWidget::slotRemoteEditBtnClicked()
{
    QList<QTreeWidgetItem *> items = d->remote->list->selectedItems();
    if (items.isEmpty())
        return;
    ConnectionDataLVItem* item = static_cast<ConnectionDataLVItem*>(items.first());
    if (!item)
        return;
    KexiDBConnectionDialog dlg(this, *item->data(), QString(),
                               KGuiItem(xi18nc("@action:button Save Database Connection", "&Save"), koIconName("document-save"),
                                        xi18n("Save changes made to this database connection")));
    dlg.setWindowTitle(xi18nc("@title:window", "Edit Database Connection"));
    if (QDialog::Accepted != dlg.exec())
        return;

    KDbMessageGuard mg(d->conn_set);
    if (!d->conn_set->saveConnectionData(item->data(), *dlg.currentProjectData().connectionData())) {
        return;
    }
    const KDbDriverMetaData *driverMetaData = d->manager.driverMetaData(item->data()->driverId());
    if (driverMetaData) {
        item->update(*driverMetaData);
        d->updateRemoteListColumns();
        slotConnectionSelectionChanged(); //to update descr. edit
    }
}

void KexiConnectionSelectorWidget::slotRemoteRemoveBtnClicked()
{
    QList<QTreeWidgetItem *> items = d->remote->list->selectedItems();
    if (items.isEmpty())
        return;
    ConnectionDataLVItem* item = static_cast<ConnectionDataLVItem*>(items.first());
    if (!item)
        return;
    if (KMessageBox::PrimaryAction != KMessageBox::questionTwoActions(this,
            xi18nc("@info",
                "Do you want to delete database connection <resource>%1</resource> from "
                "the list of available connections?",
                item->data()->toUserVisibleString()),
            QString(), //caption
            KStandardGuiItem::del(), KStandardGuiItem::cancel(),
            QString(), //dont'ask name
            KMessageBox::Notify | KMessageBox::Dangerous)) {
        return;
    }

    QTreeWidgetItem* nextItem = d->remote->list->itemBelow(item);
    if (!nextItem)
        nextItem = d->remote->list->itemAbove(item);
    KDbMessageGuard mg(d->conn_set);
    if (!d->conn_set->removeConnectionData(item->data()))
        return;

    delete item->data();
    delete item;

    if (nextItem)
        nextItem->setSelected(true);
    d->updateRemoteListColumns();
}

void KexiConnectionSelectorWidget::hideConnectonIcon()
{
    d->remote->lblIcon->setFixedWidth(0);
    d->remote->lblIcon->setPixmap(QPixmap());
}

void KexiConnectionSelectorWidget::hideDescription()
{
    d->remote->lblIcon->hide();
    d->remote->label->hide();
}

void KexiConnectionSelectorWidget::setExcludedMimeTypes(const QStringList &mimeTypes)
{
    d->fileIface->setExcludedMimeTypes(mimeTypes);
}

bool KexiConnectionSelectorWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if ((ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return)
            && ke->modifiers() == Qt::NoModifier)
        {
            slotConnectionItemExecuted();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void KexiConnectionSelectorWidget::slotFileConnectionSelected(const QString &name)
{
    Q_UNUSED(name)
    d->isConnectionSelected = !d->fileIface->selectedFile().isEmpty();
    emit connectionSelected(d->isConnectionSelected);
    emit fileSelected(name);
}

void KexiConnectionSelectorWidget::slotConnectionSelected()
{
    d->isConnectionSelected = !d->remote->list->selectedItems().isEmpty();
    emit connectionSelected(d->isConnectionSelected);
}

bool KexiConnectionSelectorWidget::hasSelectedConnection() const
{
    return d->isConnectionSelected;
}

void KexiConnectionSelectorWidget::setFileMode(KexiFileFilters::Mode mode)
{
    if (d->fileIface) {
        d->fileIface->setMode(mode);
    }
}

void KexiConnectionSelectorWidget::setAdditionalMimeTypes(const QStringList &mimeTypes)
{
    if (d->fileIface) {
        d->fileIface->setAdditionalMimeTypes(mimeTypes);
    }
}

bool KexiConnectionSelectorWidget::checkSelectedFile()
{
    if (d->fileIface) {
        return d->fileIface->checkSelectedFile();
    }
    return false;
}

QString KexiConnectionSelectorWidget::highlightedFile() const
{
    if (d->fileIface) {
        return d->fileIface->highlightedFile();
    }
    return QString();
}

#include "KexiConnectionSelectorWidget.moc"
