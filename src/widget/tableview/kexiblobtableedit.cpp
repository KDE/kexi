/* This file is part of the KDE project
   Copyright (C) 2002 Peter Simonsson <psn@linux.se>
   Copyright (C) 2004, 2006 Jarosław Staniek <staniek@kde.org>

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

#include "kexiblobtableedit.h"
#include "KexiTableScrollArea.h"
#include "KexiTableScrollAreaWidget.h"
#include <kexiutils/utils.h>
#include <widget/utils/kexidropdownbutton.h>
#include <widget/utils/kexicontextmenuutils.h>
#include <KexiIcon.h>

#include <KDbTableViewColumn>
#include <KDbQueryColumnInfo>

#include <KIconLoader>
#include <KLocalizedString>

#include <QDebug>
#include <QDataStream>
#include <QFile>
#include <QPixmap>
#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QBuffer>
#include <QCache>
#include <QScrollBar>
#include <QTemporaryFile>
#include <QUrl>

#include <stdlib.h>

struct PixmapAndPos {
    QPixmap pixmap;
    QPoint pos;
};

//! @internal
class Q_DECL_HIDDEN KexiBlobTableEdit::Private
{
public:
    Private()
            : menu(0)
            , readOnly(false)
            , setValueInternalEnabled(true) {
    }

    QByteArray value;
    KexiDropDownButton *button;
    QSize totalSize;
    KexiImageContextMenu *menu;
    bool readOnly; //!< cached for slotUpdateActionsAvailabilityRequested()
    bool setValueInternalEnabled; //!< used to disable KexiBlobTableEdit::setValueInternal()
    QCache<qulonglong, PixmapAndPos> cachedPixmaps;
};

//======================================================

KexiBlobTableEdit::KexiBlobTableEdit(KDbTableViewColumn *column, QWidget *parent)
        : KexiTableEdit(column, parent)
        , d(new Private())
{
    KexiDataItemInterface::setHasFocusableWidget(false);
    d->button = new KexiDropDownButton(parentWidget() /*usually a viewport*/);
    d->button->hide();
    d->button->setToolTip(xi18n("Click to show available actions for this cell"));

    d->menu = new KexiImageContextMenu(this);
    d->menu->installEventFilter(this);
    if (column->columnInfo())
        KexiImageContextMenu::updateTitle(d->menu, column->columnInfo()->captionOrAliasOrName(),
//! @todo pixmaplabel icon is hardcoded...
                                          KexiIconName("imagebox"));
    d->button->setMenu(d->menu);

    connect(d->menu, SIGNAL(updateActionsAvailabilityRequested(bool*,bool*)),
            this, SLOT(slotUpdateActionsAvailabilityRequested(bool*,bool*)));
    connect(d->menu, SIGNAL(insertFromFileRequested(QUrl)),
            this, SLOT(handleInsertFromFileAction(QUrl)));
    connect(d->menu, SIGNAL(saveAsRequested(QString)),
            this, SLOT(handleSaveAsAction(QString)));
    connect(d->menu, SIGNAL(cutRequested()),
            this, SLOT(handleCutAction()));
    connect(d->menu, SIGNAL(copyRequested()),
            this, SLOT(handleCopyAction()));
    connect(d->menu, SIGNAL(pasteRequested()),
            this, SLOT(handlePasteAction()));
    connect(d->menu, SIGNAL(clearRequested()),
            this, SLOT(clear()));
    connect(d->menu, SIGNAL(showPropertiesRequested()),
            this, SLOT(handleShowPropertiesAction()));
}

KexiBlobTableEdit::~KexiBlobTableEdit()
{
    delete d;
#if 0
    qDebug() << "Cleaning up...";
    if (m_tempFile) {
        m_tempFile->unlink();
        //! @todo
    }
    delete m_proc;
    m_proc = 0;
    qDebug() << "Ready.";
#endif
}

//! initializes this editor with \a add value
void KexiBlobTableEdit::setValueInternal(const QVariant& add, bool removeOld)
{
    if (!d->setValueInternalEnabled)
        return;
    if (removeOld)
        d->value = add.toByteArray();
    else //do not add "m_origValue" to "add" as this is QByteArray
        d->value = KexiDataItemInterface::originalValue().toByteArray();

//! @todo
#if 0 //todo?
    QByteArray val = m_origValue.toByteArray();
    qDebug() << "Size of BLOB:" << val.size();
    m_tempFile = new QTemporaryFile();
    m_tempFile->open();
    qDebug() << "Creating temporary file:" << m_tempFile->fileName();
    QDataStream stream(m_tempFile);
    stream->writeRawBytes(val.data(), val.size());
    delete m_tempFile;
    m_tempFile = 0;

    KMimeMagicResult* mmr = KMimeMagic::self()->findFileType(m_tempFile->fileName());
    qDebug() << "Mimetype=" << mmr->mimeType();

    setViewWidget(new QWidget(this));
#endif
}

bool KexiBlobTableEdit::valueIsNull()
{
//! @todo
    return d->value.isEmpty();
}

bool KexiBlobTableEdit::valueIsEmpty()
{
//! @todo
    return d->value.isEmpty();
}

QVariant
KexiBlobTableEdit::value()
{
    return d->value;
//! @todo
#if 0
    if (m_content && m_content->isModified()) {
        return QVariant(m_content->text());
    }
    QByteArray value;
    QFile f(m_tempFile->fileName());
    f.open(QIODevice::ReadOnly);
    QDataStream stream(&f);
    char* data = (char*) malloc(f.size());
    value.resize(f.size());
    stream.readRawBytes(data, f.size());
    value.duplicate(data, f.size());
    free(data);
    qDebug() << "Size of BLOB:" << value.size();
    return QVariant(value);
#endif
}

void KexiBlobTableEdit::paintFocusBorders(QPainter *p, QVariant &, int x, int y, int w, int h)
{
    p->drawRect(x, y, w, h);
}

void
KexiBlobTableEdit::setupContents(QPainter *p, bool focused, const QVariant& val,
                                 QString &txt, int &align, int &x, int &y_offset, int &w, int &h)
{
    Q_UNUSED(focused);
    Q_UNUSED(txt);
    Q_UNUSED(align);

    x = 0;
    w -= 1; //a place for border
    h -= 1; //a place for border
    if (p) {
        const QByteArray array(val.toByteArray());
//! @todo optimize: keep this checksum in context of data
//! @todo optimize: for now 100 items are cached; set proper cache size, e.g. based on the number of blob items visible on screen
        // the key is unique for this tuple: (checksum, w, h)
        qulonglong sum((((qulonglong(qChecksum(array.constData(), array.length())) << 32) + w) << 16) + h);
        bool insertToCache = false;
        PixmapAndPos *pp = d->cachedPixmaps.object(sum);
        if (!pp) {
            QPixmap pixmap;
            if (val.canConvert(QVariant::ByteArray) && KexiUtils::loadPixmapFromData(&pixmap, val.toByteArray())) {
#if 0
        KexiUtils::drawPixmap(*p, QMargins()/*lineWidth*/, QRect(x, y_offset, w, h),
                              pixmap, Qt::AlignCenter, true/*scaledContents*/, true/*keepAspectRatio*/);
#endif
                QPoint pos;
                pixmap = KexiUtils::scaledPixmap(QMargins()/*lineWidth*/, QRect(x, y_offset, w, h),
                             pixmap, &pos, Qt::AlignCenter, true/*scaledContents*/, true/*keepAspectRatio*/,
                             Qt::SmoothTransformation);
                if (!pixmap.isNull()) {
                    pp = new PixmapAndPos;
                    pp->pixmap = pixmap;
                    pp->pos = pos;
                    insertToCache = true;
                }
            }
        }
        if (pp) {
            p->drawPixmap(pp->pos, pp->pixmap);
        }
        if (insertToCache) {
            // Do that afterwards because in theory QCache can delete pp immediately (COVERITY CID #1354236)
            //! @todo use cost other than 1
            d->cachedPixmaps.insert(sum, pp);
        }
    }
}

bool KexiBlobTableEdit::cursorAtStart()
{
    return true;
}

bool KexiBlobTableEdit::cursorAtEnd()
{
    return true;
}

void KexiBlobTableEdit::handleInsertFromFileAction(const QUrl &url)
{
    if (isReadOnly())
        return;

    QString fileName(url.isLocalFile() ? url.toLocalFile() : url.toDisplayString());

    //! @todo download the file if remote, then set fileName properly
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        //! @todo err msg
        return;
    }
    QByteArray ba = f.readAll();
    if (f.error() != QFile::NoError) {
        //! @todo err msg
        f.close();
        return;
    }
    f.close();
    setValueInternal(ba, true);
    signalEditRequested();
    //emit acceptRequested();
}

void KexiBlobTableEdit::handleAboutToSaveAsAction(QString *origFilename, QString *mimeType, bool *dataIsEmpty)
{
    Q_ASSERT(origFilename);
    Q_ASSERT(mimeType);
    Q_ASSERT(dataIsEmpty);
    Q_UNUSED(origFilename);
    Q_UNUSED(mimeType);
    *dataIsEmpty = valueIsEmpty();
//! @todo no fname stored for now
}

void KexiBlobTableEdit::handleSaveAsAction(const QString& fileName)
{
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly)) {
        //! @todo err msg
        return;
    }
    f.write(d->value);
    if (f.error() != QFile::NoError) {
        //! @todo err msg
        f.close();
        return;
    }
    f.close();
}

void KexiBlobTableEdit::handleCutAction()
{
    if (isReadOnly())
        return;
    handleCopyAction();
    clear();
}

void KexiBlobTableEdit::handleCopyAction()
{
    executeCopyAction(d->value);
}

void KexiBlobTableEdit::executeCopyAction(const QByteArray& data)
{
    QPixmap pixmap;
    if (!KexiUtils::loadPixmapFromData(&pixmap, data)) {
        return;
    }
    qApp->clipboard()->setPixmap(pixmap, QClipboard::Clipboard);
}

void KexiBlobTableEdit::handlePasteAction()
{
    if (isReadOnly())
        return;
    QPixmap pm(qApp->clipboard()->pixmap(QClipboard::Clipboard));
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    if (pm.save(&buffer, "PNG")) {  // write pixmap into ba in PNG format
        setValueInternal(ba, true);
    } else {
        setValueInternal(QByteArray(), true);
    }
    signalEditRequested();
    repaintRelatedCell();
}

void KexiBlobTableEdit::clear()
{
    setValueInternal(QByteArray(), true);
    signalEditRequested();
    //emit acceptRequested();
    repaintRelatedCell();
}

void KexiBlobTableEdit::handleShowPropertiesAction()
{
    //! @todo
}

void KexiBlobTableEdit::showFocus(const QRect& r, bool readOnly)
{
    d->readOnly = readOnly; //cache for slotUpdateActionsAvailabilityRequested()
    updateFocus(r);
    if (d->readOnly)
        d->button->hide();
    else
        d->button->show();
}

void KexiBlobTableEdit::resize(int w, int h)
{
    d->totalSize = QSize(w, h);
    const int addWidth = d->readOnly ? 0 : d->button->width();
    QWidget::resize(w - addWidth, h);
    if (!d->readOnly)
        d->button->resize(h, h);
    m_rightMarginWhenFocused = m_rightMargin + addWidth;
    QRect r(pos().x(), pos().y(), w + 1, h + 1);
    r.translate(qobject_cast<KexiTableScrollAreaWidget*>(parentWidget())->scrollArea->horizontalScrollBar()->value(),
                qobject_cast<KexiTableScrollAreaWidget*>(parentWidget())->scrollArea->verticalScrollBar()->value());
    updateFocus(r);
/*! @todo
    if (d->menu) {
        d->menu->updateSize();
    }*/
}

void KexiBlobTableEdit::updateFocus(const QRect& r)
{
    if (!d->readOnly) {
        if (d->button->width() > r.width())
            moveChild(d->button, r.right() + 1, r.top());
        else
            moveChild(d->button, r.right() - d->button->width(), r.top());
    }
}

void KexiBlobTableEdit::hideFocus()
{
    d->button->hide();
}

QSize KexiBlobTableEdit::totalSize() const
{
    return d->totalSize;
}

void KexiBlobTableEdit::slotUpdateActionsAvailabilityRequested(bool *valueIsNull, bool *valueIsReadOnly)
{
    Q_ASSERT(valueIsNull);
    Q_ASSERT(valueIsReadOnly);
    emit editRequested();
    *valueIsNull = this->valueIsNull();
    *valueIsReadOnly = d->readOnly || isReadOnly();
}

void KexiBlobTableEdit::signalEditRequested()
{
    d->setValueInternalEnabled = false;
    emit editRequested();
    d->setValueInternalEnabled = true;
}

bool KexiBlobTableEdit::handleKeyPress(QKeyEvent* ke, bool editorActive)
{
    Q_UNUSED(editorActive);

    const int k = ke->key();
    if (!d->readOnly) {
        if ((ke->modifiers() == Qt::NoButton && k == Qt::Key_F4)
                || (ke->modifiers() == Qt::AltModifier && k == Qt::Key_Down)) {
            d->button->animateClick();
            QMouseEvent me(QEvent::MouseButtonPress, QPoint(2, 2), Qt::LeftButton, Qt::NoButton,
                           Qt::NoModifier);
            QApplication::sendEvent(d->button, &me);
        } else if (ke->modifiers() == Qt::NoModifier
                   && (k == Qt::Key_F2 || k == Qt::Key_Space || k == Qt::Key_Enter || k == Qt::Key_Return)) {
            d->menu->insertFromFile();
        } else
            return false;
    } else
        return false;
    return true;
}

bool KexiBlobTableEdit::handleDoubleClick()
{
    d->menu->insertFromFile();
    return true;
}

void KexiBlobTableEdit::handleCopyAction(const QVariant& value, const QVariant& visibleValue)
{
    Q_UNUSED(visibleValue);
    executeCopyAction(value.toByteArray());
}

void KexiBlobTableEdit::handleAction(const QString& actionName)
{
    if (actionName == "edit_paste") {
        d->menu->paste();
    } else if (actionName == "edit_cut") {
        emit editRequested();
        d->menu->cut();
    }
}

bool KexiBlobTableEdit::eventFilter(QObject *o, QEvent *e)
{
    if (o == d->menu && e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);
        const Qt::KeyboardModifiers mods = ke->modifiers();
        const int k = ke->key();
        if ((mods == Qt::NoModifier && (k == Qt::Key_Tab || k == Qt::Key_Left || k == Qt::Key_Right))
                || (mods == Qt::ShiftModifier && k == Qt::Key_Backtab)
           ) {
            d->menu->hide();
            QApplication::sendEvent(this, ke);   //re-send to move cursor
            return true;
        }
    }
    return false;
}

KEXI_CELLEDITOR_FACTORY_ITEM_IMPL(KexiBlobEditorFactoryItem, KexiBlobTableEdit)

//=======================
// KexiKIconTableEdit class is temporarily here:

//! @internal
class Q_DECL_HIDDEN KexiKIconTableEdit::Private
{
public:
    Private()
            : pixmapCache(17) {
    }
    //! We've no editor widget that would store current value, so we do this here
    QVariant currentValue;

    QCache<QString, QPixmap> pixmapCache;
};

KexiKIconTableEdit::KexiKIconTableEdit(KDbTableViewColumn *column, QWidget *parent)
        : KexiTableEdit(column, parent)
        , d(new Private())
{
    init();
}

KexiKIconTableEdit::~KexiKIconTableEdit()
{
    delete d;
}

void KexiKIconTableEdit::init()
{
    KexiDataItemInterface::setHasFocusableWidget(false);
}

void KexiKIconTableEdit::setValueInternal(const QVariant& /*add*/, bool /*removeOld*/)
{
    d->currentValue = KexiDataItemInterface::originalValue();
}

bool KexiKIconTableEdit::valueIsNull()
{
    return d->currentValue.isNull();
}

bool KexiKIconTableEdit::valueIsEmpty()
{
    return d->currentValue.isNull();
}

QVariant KexiKIconTableEdit::value()
{
    return d->currentValue;
}

void KexiKIconTableEdit::clear()
{
    d->currentValue = QVariant();
}

bool KexiKIconTableEdit::cursorAtStart()
{
    return true;
}

bool KexiKIconTableEdit::cursorAtEnd()
{
    return true;
}

void KexiKIconTableEdit::setupContents(QPainter *p, bool /*focused*/, const QVariant& val,
                                       QString &/*txt*/, int &/*align*/, int &/*x*/, int &y_offset, int &w, int &h)
{
    Q_UNUSED(y_offset);

#if 0
#ifdef Q_OS_WIN
    y_offset = -1;
#else
    y_offset = 0;
#endif
    int s = qMax(h - 5, 12);
    s = qMin(h - 3, s);
    s = qMin(w - 3, s);//avoid too large box
    QRect r(qMax(w / 2 - s / 2, 0) , h / 2 - s / 2 /*- 1*/, s, s);
    p->setPen(QPen(colorGroup().text(), 1));
    p->drawRect(r);
    if (val.asBool()) {
        p->drawLine(r.x(), r.y(), r.right(), r.bottom());
        p->drawLine(r.x(), r.bottom(), r.right(), r.y());
    }
#endif

    QString key(val.toString());
    QPixmap pm;
    if (!key.isEmpty()) {
        QPixmap *cached = d->pixmapCache[ key ];
        if (cached)
            pm = *cached;
        if (pm.isNull()) {
            //cache pixmap
            pm = QIcon::fromTheme(key).pixmap(KIconLoader::global()->currentSize(KIconLoader::Small));
            if (!pm.isNull())
                d->pixmapCache.insert(key, new QPixmap(pm));
        }
    }

    if (p && !pm.isNull())
        p->drawPixmap((w - pm.width()) / 2, (h - pm.height()) / 2, pm);
}

void KexiKIconTableEdit::handleCopyAction(const QVariant& value, const QVariant& visibleValue)
{
    Q_UNUSED(value);
    Q_UNUSED(visibleValue);
}

KEXI_CELLEDITOR_FACTORY_ITEM_IMPL(KexiKIconTableEditorFactoryItem, KexiKIconTableEdit)

