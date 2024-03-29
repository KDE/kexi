/* This file is part of the KDE project
   Copyright (C) 2011-2016 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).

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

#include "KexiSearchLineEdit.h"
#include <KexiSearchableModel.h>

#include <KLocalizedString>
#include <KActionCollection>

#include <kexiutils/completer/KexiCompleter.h>
#include <kexiutils/KexiTester.h>
#include <KexiMainWindowIface.h>

#include <QDebug>
#include <QShortcut>
#include <QKeySequence>
#include <QTreeView>
#include <QAbstractProxyModel>
#include <QInputMethodEvent>
#include <QStyledItemDelegate>
#include <QTextLayout>
#include <QPainter>
#include <QFontMetrics>

class SearchableObject
{
public:
    KexiSearchableModel *model;
    int index;
};

class KexiSearchLineEditCompleterPopupModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit KexiSearchLineEditCompleterPopupModel(QObject *parent = 0);
    ~KexiSearchLineEditCompleterPopupModel();
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

public Q_SLOTS:
    //! Adds a new model or updates information (model items) about existing one
    void addSearchableModel(KexiSearchableModel *model);

    //! Removes existing model
    void removeSearchableModel(KexiSearchableModel *model);

private:
    class Private;
    Private * const d;
};

class Q_DECL_HIDDEN KexiSearchLineEditCompleterPopupModel::Private
{
public:
    Private()
     : cachedCount(-1)
    {
    }
    ~Private() {
        qDeleteAll(searchableObjects);
    }
    void removeSearchableModel(KexiSearchableModel *model) {
        if (searchableModels.removeAll(model) == 0) {
            return;
        }
        QMutableMapIterator<int, SearchableObject *> it(searchableObjects);
        while (it.hasNext()) {
            it.next();
            if (it.value()->model == model) {
                it.remove();
            }
        }
    }
    void updateCachedCount() {
        cachedCount = 0;
        foreach (KexiSearchableModel* searchableModel, searchableModels) {
            cachedCount += searchableModel->searchableObjectCount();
        }
    }
    int cachedCount;
    QList<KexiSearchableModel*> searchableModels;
    QMap<int, SearchableObject*> searchableObjects;
};

KexiSearchLineEditCompleterPopupModel::KexiSearchLineEditCompleterPopupModel(QObject *parent)
 : QAbstractListModel(parent), d(new Private)
{
}

KexiSearchLineEditCompleterPopupModel::~KexiSearchLineEditCompleterPopupModel()
{
    delete d;
}

int KexiSearchLineEditCompleterPopupModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (d->cachedCount < 0) {
        d->updateCachedCount();
    }
    return d->cachedCount;
}

QVariant KexiSearchLineEditCompleterPopupModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (d->cachedCount <= row) {
        return QVariant();
    }
    SearchableObject *object = static_cast<SearchableObject*>(index.internalPointer());
    QModelIndex sourceIndex = object->model->sourceIndexForSearchableObject(object->index);
    return object->model->searchableData(sourceIndex, role);
}

QModelIndex KexiSearchLineEditCompleterPopupModel::index(int row, int column,
                                                         const QModelIndex &parent) const
{
    //qDebug() << row;
    if (!hasIndex(row, column, parent)) {
        qDebug() << "!hasIndex";
        return QModelIndex();
    }

    int r = row;
    SearchableObject *sobject = d->searchableObjects.value(row);
    if (!sobject) {
        foreach (KexiSearchableModel* searchableModel, d->searchableModels) {
            const int count = searchableModel->searchableObjectCount();
            if (r < count) {
                sobject = new SearchableObject;
                sobject->model = searchableModel;
                sobject->index = r;
                d->searchableObjects.insert(row, sobject);
                break;
            }
            else {
                r -= count;
            }
        }
    }
    if (!sobject) {
        return QModelIndex();
    }
    return createIndex(row, column, sobject);
}

void KexiSearchLineEditCompleterPopupModel::addSearchableModel(KexiSearchableModel *model)
{
    if (!model) {
        return;
    }
    beginResetModel();
    d->removeSearchableModel(model);
    d->searchableModels.append(model);
    connect(model->deleteNotifier(), &KexiSearchableModelDeleteNotifier::aboutToDelete, this,
            &KexiSearchLineEditCompleterPopupModel::removeSearchableModel, Qt::UniqueConnection);
    d->updateCachedCount();
    endResetModel();
}

void KexiSearchLineEditCompleterPopupModel::removeSearchableModel(KexiSearchableModel *model)
{
    if (!model || !d->searchableModels.contains(model)) {
        return;
    }
    beginResetModel();
    d->removeSearchableModel(model);
    d->updateCachedCount();
    endResetModel();
}

// ----

class KexiSearchLineEditCompleter : public KexiCompleter
{
    Q_OBJECT
public:
    explicit KexiSearchLineEditCompleter(QObject *parent = 0) : KexiCompleter(parent) {
        setCompletionRole(Qt::DisplayRole);
    }

    virtual QString pathFromIndex(const QModelIndex &index) const override {
        if (!index.isValid())
            return QString();
        SearchableObject *object = static_cast<SearchableObject*>(index.internalPointer());
        QModelIndex sourceIndex = object->model->sourceIndexForSearchableObject(object->index);
        return object->model->pathFromIndex(sourceIndex);
    }
};

// ----

class KexiSearchLineEditPopupItemDelegate;

class Q_DECL_HIDDEN KexiSearchLineEdit::Private
{
public:
    explicit Private(KexiSearchLineEdit *_q)
     : q(_q), clearShortcut(QKeySequence(Qt::Key_Escape), _q),
       recentlyHighlightedModel(0)
    {
        // make Escape key clear the search box
        QObject::connect(&clearShortcut, SIGNAL(activated()),
                         q, SLOT(slotClearShortcutActivated()));
    }

    void highlightSearchableObject(const QPair<QModelIndex, KexiSearchableModel*> &source)
    {
        source.second->highlightSearchableObject(source.first);
        recentlyHighlightedModel = source.second;
    }

    void removeHighlightingForSearchableObject()
    {
        if (recentlyHighlightedModel) {
            recentlyHighlightedModel->highlightSearchableObject(QModelIndex());
            recentlyHighlightedModel = 0;
        }
    }

    KexiSearchLineEditCompleter *completer;
    QTreeView *popupTreeView;
    KexiSearchLineEditCompleterPopupModel *model;
    KexiSearchLineEditPopupItemDelegate *delegate;
    QPointer<QWidget> previouslyFocusedWidget;

private:
    KexiSearchLineEdit *q;
    QShortcut clearShortcut;
    KexiSearchableModel *recentlyHighlightedModel;
};

// ----

static QSizeF viewItemTextLayout(QTextLayout &textLayout, int lineWidth)
{
    qreal height = 0;
    qreal widthUsed = 0;
    textLayout.beginLayout();
    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout.endLayout();
    return QSizeF(widthUsed, height);
}

class KexiSearchLineEditPopupItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    KexiSearchLineEditPopupItemDelegate(QObject *parent, KexiCompleter *completer)
     : QStyledItemDelegate(parent), highlightMatchingSubstrings(true), m_completer(completer)
    {
    }

    //! Implemented to improve width hint
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize size(QStyledItemDelegate::sizeHint(option, index));
        QStyleOptionViewItem v4 = option;
        QStyledItemDelegate::initStyleOption(&v4, index);
        const QSize s = v4.widget->style()->sizeFromContents(QStyle::CT_ItemViewItem, &v4, size, v4.widget);
        size.setWidth(s.width());
        return size;
    }

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);
        QStyleOptionViewItem v4 = option;
        QStyledItemDelegate::initStyleOption(&v4, index);
        // like in QCommonStyle::paint():
        if (!v4.text.isEmpty()) {
            painter->save();
            painter->setClipRect(v4.rect);
            QPalette::ColorGroup cg = (v4.state & QStyle::State_Enabled)
                                    ? QPalette::Normal : QPalette::Disabled;
            if (cg == QPalette::Normal && !(v4.state & QStyle::State_Active)) {
                cg = QPalette::Inactive;
            }
            if (v4.state & QStyle::State_Selected) {
                painter->setPen(v4.palette.color(cg, QPalette::HighlightedText));
            }
            else {
                painter->setPen(v4.palette.color(cg, QPalette::Text));
            }
            QRect textRect = v4.widget->style()->subElementRect(QStyle::SE_ItemViewItemText,
                                                                &v4, v4.widget);
            viewItemDrawText(painter, &v4, textRect);
            painter->restore();
        }
    }
    bool highlightMatchingSubstrings;

protected:
    // bits from qcommonstyle.cpp
    void viewItemDrawText(QPainter *p, const QStyleOptionViewItem *option, const QRect &rect) const
    {
        const QWidget *widget = option->widget;
        const int textMargin = widget->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, widget) + 1;

        QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
        const bool wrapText = option->features & QStyleOptionViewItem::WrapText;
        QTextOption textOption;
        textOption.setWrapMode(wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap);
        textOption.setTextDirection(option->direction);
        textOption.setAlignment(QStyle::visualAlignment(option->direction, option->displayAlignment));
        QTextLayout textLayout;
        textLayout.setTextOption(textOption);
        textLayout.setFont(option->font);
        QString text = option->text;
        textLayout.setText(text);

        if (highlightMatchingSubstrings) {
            QList<QTextLayout::FormatRange> formats;
            QString substring = m_completer->completionPrefix();
            QColor underLineColor(p->pen().color());
            underLineColor.setAlpha(128);
            QTextLayout::FormatRange formatRange;
            formatRange.format.setFontUnderline(true);
            formatRange.format.setUnderlineColor(underLineColor);

            for (int i = 0; i < text.length();) {
                i = text.indexOf(substring, i, Qt::CaseInsensitive);
                if (i == -1)
                    break;
                formatRange.length = substring.length();
                formatRange.start = i;
                formats.append(formatRange);
                i += formatRange.length;
            }
            textLayout.setAdditionalFormats(formats);
        }
        viewItemTextLayout(textLayout, textRect.width());

        const int lineCount = textLayout.lineCount();
        QPointF position = textRect.topLeft();
        for (int i = 0; i < lineCount; ++i) {
            const QTextLine line = textLayout.lineAt(i);
            const QPointF adjustPos(0, qreal(textRect.height() - line.rect().height()) / 2.0);
            line.draw(p, position + adjustPos);
            position.setY(position.y() + line.y() + line.ascent());
        }
    }

    virtual void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        QStyleOptionViewItem *v4 = qstyleoption_cast<QStyleOptionViewItem*>(option);
        if (v4) {
            v4->text.clear();
        }
    }
    KexiCompleter *m_completer;
};

// ----

//! @internal Style-dependent fixes for the left margin, probably needed because of the limited
//! width of the line edit - it's placed in tab bar's corner widget.
static void fixLeftMargin(QLineEdit *lineEdit)
{
    int add = 0;
    const QByteArray st(lineEdit->style()->objectName().toLatin1());
    if (st == "breeze" || st == "gtk+") {
        add = 4; // like QLineEditIconButton::IconMargin
    }
    else if (st == "fusion") {
        add = 2;
    }
    if (add != 0) {
        QMargins margins(lineEdit->textMargins());
        margins.setLeft(margins.left() + add);
        lineEdit->setTextMargins(margins);
    }
}

// ----

KexiSearchLineEdit::KexiSearchLineEdit(QWidget *parent)
 : QLineEdit(parent), d(new Private(this))
{
    d->completer = new KexiSearchLineEditCompleter(this);
    d->popupTreeView = new QTreeView;
    kexiTester() << KexiTestObject(d->popupTreeView, "globalSearch.treeView");

    d->completer->setPopup(d->popupTreeView);
    d->completer->setModel(d->model = new KexiSearchLineEditCompleterPopupModel(d->completer));
    d->completer->setCaseSensitivity(Qt::CaseInsensitive);
    d->completer->setSubstringCompletion(true);
    d->completer->setMaxVisibleItems(12);
    // Use unsorted model, sorting is handled in the source model itself.
    // Moreover, sorting KexiCompleter::CaseInsensitivelySortedModel breaks
    // filtering so only table names are displayed.
    d->completer->setModelSorting(KexiCompleter::UnsortedModel);

    d->popupTreeView->setHeaderHidden(true);
    d->popupTreeView->setRootIsDecorated(false);
    d->popupTreeView->setItemDelegate(
        d->delegate = new KexiSearchLineEditPopupItemDelegate(d->popupTreeView, d->completer));

    // forked initialization like in QLineEdit::setCompleter:
    d->completer->setWidget(this);
    if (hasFocus()) {
        connectCompleter();
    }

    setFocusPolicy(Qt::NoFocus); // We cannot focus set any policy here.
                                 // Qt::ClickFocus would make it impossible to find
                                 // previously focus widget in KexiSearchLineEdit::setFocus().
                                 // We need this information to focus back when pressing Escape key.
    setClearButtonEnabled(true);
    QAction *action_tools_locate = KexiMainWindowIface::global()->actionCollection()->action("tools_locate");
    setPlaceholderText(xi18nc("Tools->Locate textbox' placeholder text with shortcut", "Locate (%1)",
                       action_tools_locate->shortcut().toString(QKeySequence::NativeText)));
    fixLeftMargin(this);
    setMinimumWidth(fontMetrics().width("  " + placeholderText() + "    " + "    " + "      "));
}

KexiSearchLineEdit::~KexiSearchLineEdit()
{
    delete d;
}

void KexiSearchLineEdit::connectCompleter()
{
    connect(d->completer, SIGNAL(activated(QString)),
            this, SLOT(setText(QString)));
    connect(d->completer, SIGNAL(activated(QModelIndex)),
            this, SLOT(slotCompletionActivated(QModelIndex)));
    connect(d->completer, SIGNAL(highlighted(QString)),
            this, SLOT(slotCompletionHighlighted(QString)));
    connect(d->completer, SIGNAL(highlighted(QModelIndex)),
            this, SLOT(slotCompletionHighlighted(QModelIndex)));
}

void KexiSearchLineEdit::disconnectCompleter()
{
    disconnect(d->completer, 0, this, 0);
}

void KexiSearchLineEdit::slotClearShortcutActivated()
{
    //qDebug() << (QWidget*)d->previouslyFocusedWidget << text();
    d->removeHighlightingForSearchableObject();
    if (text().isEmpty() && d->previouslyFocusedWidget) {
        // after second Escape, go back to previously focused widget
        d->previouslyFocusedWidget->setFocus();
        d->previouslyFocusedWidget = 0;
    }
    else {
        clear();
    }
}

void KexiSearchLineEdit::addSearchableModel(KexiSearchableModel *model)
{
    d->model->addSearchableModel(model);
}

void KexiSearchLineEdit::removeSearchableModel(KexiSearchableModel *model)
{
    d->model->removeSearchableModel(model);
}

QPair<QModelIndex, KexiSearchableModel*> KexiSearchLineEdit::mapCompletionIndexToSource(const QModelIndex &index) const
{
    QModelIndex realIndex
        = qobject_cast<QAbstractProxyModel*>(d->completer->completionModel())->mapToSource(index);
    if (!realIndex.isValid()) {
        return qMakePair(QModelIndex(), static_cast<KexiSearchableModel*>(0));
    }
    SearchableObject *object = static_cast<SearchableObject*>(realIndex.internalPointer());
    if (!object) {
        return qMakePair(QModelIndex(), static_cast<KexiSearchableModel*>(0));
    }
    return qMakePair(object->model->sourceIndexForSearchableObject(object->index), object->model);
}

void KexiSearchLineEdit::slotCompletionHighlighted(const QString &newText)
{
    if (d->completer->completionMode() != KexiCompleter::InlineCompletion) {
        setText(newText);
    }
    else {
        int p = cursorPosition();
        QString t = text();
        setText(t.left(p) + newText.mid(p));
        end(false);
        cursorBackward(text().length() - p, true);
    }
}

void KexiSearchLineEdit::slotCompletionHighlighted(const QModelIndex &index)
{
    QPair<QModelIndex, KexiSearchableModel*> source = mapCompletionIndexToSource(index);
    if (!source.first.isValid())
        return;
    //qDebug() << source.second->searchableData(source.first, Qt::EditRole);
    d->highlightSearchableObject(source);
}

void KexiSearchLineEdit::slotCompletionActivated(const QModelIndex &index)
{
    QPair<QModelIndex, KexiSearchableModel*> source = mapCompletionIndexToSource(index);
    if (!source.first.isValid())
        return;
    //qDebug() << source.second->searchableData(source.first, Qt::EditRole);

    d->highlightSearchableObject(source);
    d->removeHighlightingForSearchableObject();
    if (source.second->activateSearchableObject(source.first)) {
        clear();
    }
}

// forked bits from QLineEdit::inputMethodEvent()
void KexiSearchLineEdit::inputMethodEvent(QInputMethodEvent *e)
{
    QLineEdit::inputMethodEvent(e);
    if (isReadOnly() || !e->isAccepted())
        return;
    if (!e->commitString().isEmpty()) {
        complete(Qt::Key_unknown);
    }
}

void KexiSearchLineEdit::setFocus()
{
    //qDebug() << "d->previouslyFocusedWidget:" << (QWidget*)d->previouslyFocusedWidget
    //         << "window()->focusWidget():" << window()->focusWidget();
    if (!d->previouslyFocusedWidget && window()->focusWidget() != this) {
        d->previouslyFocusedWidget = window()->focusWidget();
    }
    QLineEdit::setFocus();
}

// forked bits from QLineEdit::focusInEvent()
void KexiSearchLineEdit::focusInEvent(QFocusEvent *e)
{
    //qDebug() << "d->previouslyFocusedWidget:" << (QWidget*)d->previouslyFocusedWidget
    //         << "window()->focusWidget():" << window()->focusWidget();
    if (!d->previouslyFocusedWidget && window()->focusWidget() != this) {
        d->previouslyFocusedWidget = window()->focusWidget();
    }
    QLineEdit::focusInEvent(e);
    d->completer->setWidget(this);
    connectCompleter();
    update();
}

// forked bits from QLineEdit::focusOutEvent()
void KexiSearchLineEdit::focusOutEvent(QFocusEvent *e)
{
    QLineEdit::focusOutEvent(e);
    disconnectCompleter();
    update();
    if (e->reason() == Qt::TabFocusReason || e->reason() == Qt::BacktabFocusReason) {
        // go back to previously focused widget
        if (d->previouslyFocusedWidget) {
            d->previouslyFocusedWidget->setFocus();
        }
        e->accept();
    }
    d->previouslyFocusedWidget = 0;
    d->removeHighlightingForSearchableObject();
}

// forked bits from QLineControl::processKeyEvent()
void KexiSearchLineEdit::keyPressEvent(QKeyEvent *event)
{
    bool inlineCompletionAccepted = false;

    //qDebug() << event->key() << (QWidget*)d->previouslyFocusedWidget;

    KexiCompleter::CompletionMode completionMode = d->completer->completionMode();
    if ((completionMode == KexiCompleter::PopupCompletion
            || completionMode == KexiCompleter::UnfilteredPopupCompletion)
        && d->completer->popup()
        && d->completer->popup()->isVisible()) {
        // The following keys are forwarded by the completer to the widget
        // Ignoring the events lets the completer provide suitable default behavior
        switch (event->key()) {
        case Qt::Key_Escape:
            event->ignore();
            return;
#ifdef QT_KEYPAD_NAVIGATION
        case Qt::Key_Select:
            if (!QApplication::keypadNavigationEnabled())
                break;
            d->completer->popup()->hide(); // just hide. will end up propagating to parent
#endif
        default:
            break; // normal key processing
        }
    } else if (completionMode == KexiCompleter::InlineCompletion) {
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_F4:
#ifdef QT_KEYPAD_NAVIGATION
        case Qt::Key_Select:
            if (!QApplication::keypadNavigationEnabled())
                break;
#endif
            if (!d->completer->currentCompletion().isEmpty() && hasSelectedText()
                && textAfterSelection().isEmpty())
            {
                setText(d->completer->currentCompletion());
                inlineCompletionAccepted = true;
            }
        default:
            break; // normal key processing
        }
    }

    if (d->completer->popup() && !d->completer->popup()->isVisible()
        && (event->key() == Qt::Key_F4 || event->key() == Qt::Key_Down))
    {
        // go back to completing when popup is closed and F4/Down pressed
        d->completer->complete();
    }
    else if (d->completer->popup() && d->completer->popup()->isVisible()
        && event->key() == Qt::Key_F4)
    {
        // hide popup if F4 pressed
        d->completer->popup()->hide();
    }

    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        if (d->completer->popup() && !d->completer->popup()->isVisible()) {
            d->completer->setCompletionPrefix(text());
        }
        if (d->completer->completionCount() == 1) {
            // single item on the completion list, select it automatically
            d->completer->setCurrentRow(0);
            slotCompletionActivated(d->completer->currentIndex());
            event->accept();
            if (d->completer->popup()) {
                d->completer->popup()->hide();
            }
            return;
        }
        //qDebug() << "currentRow:" << d->completer->currentRow();
        //qDebug() << "currentIndex:" << d->completer->currentIndex().isValid();
        //qDebug() << "currentCompletion:" << d->completer->currentCompletion();
        if (d->completer->popup() && d->completer->completionCount() > 1) {
            //qDebug() << "11111" << d->completer->completionPrefix()
            //          << d->completer->completionCount();

            // more than one item on completion list, find exact match, if found, accept
            for (int i = 0; i < d->completer->completionCount(); i++) {
                //qDebug() << d->completer->completionModel()->index(i, 0, QModelIndex()).data(Qt::EditRole).toString();
                if (d->completer->completionPrefix()
                    == d->completer->completionModel()->index(i, 0, QModelIndex()).data(Qt::EditRole).toString())
                {
                    d->completer->setCurrentRow(i);
                    slotCompletionActivated(d->completer->currentIndex());
                    event->accept();
                    d->completer->popup()->hide();
                    return;
                }
            }
            // exactly matching item not found
            bool selectedItem = !d->completer->popup()->selectionModel()->selectedIndexes().isEmpty();
            if (!selectedItem || !d->completer->popup()->isVisible()) {
                if (!d->completer->popup()->isVisible()) {
                    // there is no matching text, go back to completing
                    d->completer->complete();
                }
                // do not hide
                event->accept();
                return;
            }
        }
        // applying completion since there is item selected
        d->completer->popup()->hide();
        connectCompleter();
        QLineEdit::keyPressEvent(event); /* executes this:
                                            if (hasAcceptableInput() || fixup()) {
                                                emit returnPressed();
                                                emit editingFinished();
                                            } */
        if (inlineCompletionAccepted)
            event->accept();
        else
            event->ignore();
        return;
    }

    if (event == QKeySequence::MoveToNextChar) {
#if defined(Q_OS_WIN)
        if (hasSelectedText()
            && d->completer->completionMode() == KexiCompleter::InlineCompletion)
        {
            int selEnd = selectionEnd();
            if (selEnd >= 0) {
                setCursorPosition(selEnd);
            }
            event->accept();
            return;
        }
#endif
    }
    else if (event == QKeySequence::MoveToPreviousChar) {
#if defined(Q_OS_WIN)
        if (hasSelectedText()
            && d->completer->completionMode() == KexiCompleter::InlineCompletion)
        {
            int selStart = selectionStart();
            if (selStart >= 0) {
                setCursorPosition(selStart);
            }
            event->accept();
            return;
        }
#endif
    }
    else {
        if (event->modifiers() & Qt::ControlModifier) {
            switch (event->key()) {
            case Qt::Key_Up:
            case Qt::Key_Down:
                complete(event->key());
                return;
            default:;
            }
        } else { // ### check for *no* modifier
            switch (event->key()) {
            case Qt::Key_Backspace:
                if (!isReadOnly()) {
                    backspace();
                    complete(Qt::Key_Backspace);
                    return;
                }
                break;
            case Qt::Key_Delete:
                if (!isReadOnly()) {
                    QLineEdit::keyPressEvent(event);
                    complete(Qt::Key_Delete);
                    return;
                }
                break;
            default:;
            }
        }
    }

    if (!isReadOnly()) {
        QString t = event->text();
        if (!t.isEmpty() && t.at(0).isPrint()) {
            QLineEdit::keyPressEvent(event);
            complete(event->key());
            return;
        }
    }

    QLineEdit::keyPressEvent(event);
}

void KexiSearchLineEdit::changeEvent(QEvent *event)
{
    QLineEdit::changeEvent(event);
    if (event->type() == QEvent::StyleChange) {
        fixLeftMargin(this);
    }
}

// forked bits from QLineControl::advanceToEnabledItem()
// iterating forward(dir=1)/backward(dir=-1) from the
// current row based. dir=0 indicates a new completion prefix was set.
bool KexiSearchLineEdit::advanceToEnabledItem(int dir)
{
    int start = d->completer->currentRow();
    if (start == -1)
        return false;
    int i = start + dir;
    if (dir == 0)
        dir = 1;
    do {
        if (!d->completer->setCurrentRow(i)) {
            if (!d->completer->wrapAround())
                break;
            i = i > 0 ? 0 : d->completer->completionCount() - 1;
        } else {
            QModelIndex currentIndex = d->completer->currentIndex();
            if (d->completer->completionModel()->flags(currentIndex) & Qt::ItemIsEnabled)
                return true;
            i += dir;
        }
    } while (i != start);

    d->completer->setCurrentRow(start); // restore
    return false;
}

QString KexiSearchLineEdit::textBeforeSelection() const
{
    return hasSelectedText() ? text().left(selectionStart()) : QString();
}

QString KexiSearchLineEdit::textAfterSelection() const
{
    return hasSelectedText() ? text().mid(selectionEnd()) : QString();
}

int KexiSearchLineEdit::selectionEnd() const
{
    return hasSelectedText() ?
        (selectionStart() + selectedText().length()) : -1;
}

// forked bits from QLineControl::complete()
void KexiSearchLineEdit::complete(int key)
{
    if (isReadOnly() || echoMode() != QLineEdit::Normal)
        return;

    QString text = this->text();
    if (d->completer->completionMode() == KexiCompleter::InlineCompletion) {
        if (key == Qt::Key_Backspace)
            return;
        int n = 0;
        if (key == Qt::Key_Up || key == Qt::Key_Down) {
            if (textAfterSelection().length())
                return;
            QString prefix = hasSelectedText() ? textBeforeSelection() : text;
            if (text.compare(d->completer->currentCompletion(), d->completer->caseSensitivity()) != 0
                || prefix.compare(d->completer->completionPrefix(), d->completer->caseSensitivity()) != 0) {
                d->completer->setCompletionPrefix(prefix);
            } else {
                n = (key == Qt::Key_Up) ? -1 : +1;
            }
        } else {
            d->completer->setCompletionPrefix(text);
        }
        if (!advanceToEnabledItem(n))
            return;
    } else {
#ifndef QT_KEYPAD_NAVIGATION
        if (text.isEmpty()) {
            d->completer->popup()->hide();
            return;
        }
#endif
        d->completer->setCompletionPrefix(text);
    }

    d->popupTreeView->resizeColumnToContents(0);
    d->completer->complete();
}

bool KexiSearchLineEdit::highlightMatchingSubstrings() const
{
    return d->delegate->highlightMatchingSubstrings;
}

void KexiSearchLineEdit::setHighlightMatchingSubstrings(bool highlight)
{
    d->delegate->highlightMatchingSubstrings = highlight;
}

#include "KexiSearchLineEdit.moc"
