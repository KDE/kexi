/* This file is part of the KDE project
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2011 Jarosław Staniek <staniek@kde.org>

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

#include "kexidbimagebox.h"
#include <KexiIcon.h>
#include <widget/utils/kexidropdownbutton.h>
#include <kexiutils/utils.h>
#include <formeditor/widgetlibrary.h>
#include <formeditor/utils.h>
#include <kexi_global.h>
#include "kexidbutils.h"
#include "kexiformpart.h"
#include "kexiformmanager.h"

#include <KDbField>
#include <KDbUtils>
#include <KDbQuerySchema>

#include <KIconLoader>
#include <KLocalizedString>
#include <KIconEffect>

#include <QApplication>
#include <QPixmap>
#include <QStyle>
#include <QStyleOptionFocusRect>
#include <QStyleOptionFrame>
#include <QClipboard>
#include <QFile>
#include <QBuffer>
#include <QPainter>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QMimeType>
#include <QImageReader>
#include <QDebug>

//! @internal
struct KexiDBImageBox_Static {
    KexiDBImageBox_Static() : pixmap(0), small(0) {}
    ~KexiDBImageBox_Static() { delete pixmap; delete small; }
    QPixmap *pixmap;
    QPixmap *small;
};

Q_GLOBAL_STATIC(KexiDBImageBox_Static, KexiDBImageBox_static)

KexiDBImageBox::KexiDBImageBox(bool designMode, QWidget *parent)
        : KexiFrame(parent)
        , KexiFormDataItemInterface()
        , m_alignment(Qt::AlignLeft | Qt::AlignTop)
        , m_readOnly(false)
        , m_scaledContents(false)
        , m_smoothTransformation(true)
        , m_keepAspectRatio(true)
        , m_insideSetData(false)
        , m_setFocusOnButtonAfterClosingPopup(false)
        , m_paintEventEnabled(true)
        , m_dropDownButtonVisible(true)
        , m_insideSetPalette(false)
{
    setDesignMode(designMode);
    installEventFilter(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QPalette pal(palette());
    pal.setBrush(backgroundRole(), QBrush(Qt::transparent));
    KexiFrame::setPalette(pal);

    m_contextMenu = new KexiImageContextMenu(this);
    m_contextMenu->installEventFilter(this);

    if (designMode) {
        m_chooser = 0;
    } else {
        m_chooser = new KexiDropDownButton(this);
        m_chooser->setFocusPolicy(Qt::StrongFocus);
        m_chooser->setMenu(m_contextMenu);
        setFocusProxy(m_chooser);
        m_chooser->installEventFilter(this);
    }

    setFrameShape(QFrame::Box);
    setFrameShadow(QFrame::Plain);
    setFrameColor(palette().color(QPalette::Foreground));

    m_paletteBackgroundColorChanged = false; //set this here, not before

    connect(m_contextMenu, SIGNAL(updateActionsAvailabilityRequested(bool*,bool*)),
            this, SLOT(slotUpdateActionsAvailabilityRequested(bool*,bool*)));
    connect(m_contextMenu, SIGNAL(insertFromFileRequested(QUrl)),
            this, SLOT(handleInsertFromFileAction(QUrl)));
    connect(m_contextMenu, SIGNAL(saveAsRequested(QUrl)),
            this, SLOT(handleSaveAsAction(QUrl)));
    connect(m_contextMenu, SIGNAL(cutRequested()),
            this, SLOT(handleCutAction()));
    connect(m_contextMenu, SIGNAL(copyRequested()),
            this, SLOT(handleCopyAction()));
    connect(m_contextMenu, SIGNAL(pasteRequested()),
            this, SLOT(handlePasteAction()));
    connect(m_contextMenu, SIGNAL(clearRequested()),
            this, SLOT(clear()));
    connect(m_contextMenu, SIGNAL(showPropertiesRequested()),
            this, SLOT(handleShowPropertiesAction()));

    KexiFrame::setLineWidth(0);
    setDataSource(QString());   //to initialize popup menu and actions availability
}

KexiDBImageBox::~KexiDBImageBox()
{
}

KexiImageContextMenu* KexiDBImageBox::contextMenu() const
{
    return m_contextMenu;
}

QVariant KexiDBImageBox::value()
{
    if (dataSource().isEmpty()) {
        //not db-aware
        return QVariant();
    }
    //db-aware mode
    return m_value; //!< @todo
}

void KexiDBImageBox::setValueInternal(const QVariant& add, bool removeOld, bool loadPixmap)
{
    if (isReadOnly())
        return;
    m_contextMenu->hide();
    if (removeOld)
        m_value = add.toByteArray();
    else //do not add "m_origValue" to "add" as this is QByteArray
        m_value = KexiDataItemInterface::originalValue().toByteArray();
    bool ok = !m_value.isEmpty();
    if (ok) {
        if (loadPixmap) {
            ok = KexiUtils::loadPixmapFromData(&m_pixmap, m_value);
            m_currentScaledPixmap = QPixmap(); // clear cache
        }
        if (!ok) {
            //! @todo inform about error?
        }
    }
    if (!ok) {
        m_valueMimeType.clear();
        m_pixmap = QPixmap();
        m_currentScaledPixmap = QPixmap(); // clear cache
    }
    repaint();
}

void KexiDBImageBox::setInvalidState(const QString& displayText)
{
    Q_UNUSED(displayText);
    if (!dataSource().isEmpty()) {
        m_value = QByteArray();
    }

//! @todo m_pixmapLabel->setText( displayText );

    if (m_chooser)
        m_chooser->hide();
    setReadOnly(true);
}

bool KexiDBImageBox::valueIsNull()
{
    return m_value.isEmpty();
}

bool KexiDBImageBox::valueIsEmpty()
{
    return false;
}

bool KexiDBImageBox::isReadOnly() const
{
    return m_readOnly;
}

void KexiDBImageBox::setReadOnly(bool set)
{
    m_readOnly = set;
}

QPixmap KexiDBImageBox::pixmap() const
{
    if (dataSource().isEmpty()) {
        //not db-aware
        return m_data.pixmap();
    }
    //db-aware mode
    return m_pixmap;
}

int KexiDBImageBox::pixmapId() const
{
    if (dataSource().isEmpty()) {
        //not db-aware
        return m_data.id();
    }
    return 0;
}

void KexiDBImageBox::setPixmapId(int id)
{
    if (m_insideSetData) //avoid recursion
        return;
    setData(KexiBLOBBuffer::self()->objectForId(id, /*unstored*/false));
    repaint();
}

int KexiDBImageBox::storedPixmapId() const
{
    if (dataSource().isEmpty() && m_data.stored()) {
        //not db-aware
        return m_data.id();
    }
    return 0;
}

void KexiDBImageBox::setStoredPixmapId(int id)
{
    setData(KexiBLOBBuffer::self()->objectForId(id, /*stored*/true));
    repaint();
}

bool KexiDBImageBox::hasScaledContents() const
{
    return m_scaledContents;
}

void KexiDBImageBox::setScaledContents(bool set)
{
//! @todo m_pixmapLabel->setScaledContents(set);
    m_scaledContents = set;
    m_currentScaledPixmap = QPixmap();
    repaint();
}

bool KexiDBImageBox::smoothTransformation() const
{
    return m_smoothTransformation;
}

Qt::Alignment KexiDBImageBox::alignment() const
{
    return m_alignment;
}

bool KexiDBImageBox::keepAspectRatio() const
{
    return m_keepAspectRatio;
}

void KexiDBImageBox::setSmoothTransformation(bool set)
{
    m_smoothTransformation = set;
    m_currentScaledPixmap = QPixmap();
    repaint();
}

void KexiDBImageBox::setKeepAspectRatio(bool set)
{
    m_keepAspectRatio = set;
    m_currentScaledPixmap = QPixmap();
    if (m_scaledContents) {
        repaint();
    }
}

QWidget* KexiDBImageBox::widget()
{
    //! @todo return m_pixmapLabel;
    return this;
}

bool KexiDBImageBox::cursorAtStart()
{
    return true;
}

bool KexiDBImageBox::cursorAtEnd()
{
    return true;
}

QByteArray KexiDBImageBox::data() const
{
    if (dataSource().isEmpty()) {
        //static mode
        return m_data.data();
    } else {
        //db-aware mode
        return m_value;
    }
}

void KexiDBImageBox::insertFromFile()
{
    m_contextMenu->insertFromFile();
}

void KexiDBImageBox::handleInsertFromFileAction(const QUrl &url)
{
    if (!dataSource().isEmpty() && isReadOnly())
        return;

    if (dataSource().isEmpty()) {
        //static mode
        KexiBLOBBuffer::Handle h = KexiBLOBBuffer::self()->insertPixmap(url);
        if (!h)
            return;
        setData(h);
        repaint();
    } else {
        //db-aware
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
        QMimeDatabase db;
        m_valueMimeType = db.mimeTypeForFile(fileName, QMimeDatabase::MatchExtension).name();
        setValueInternal(ba, true);
    }

//! @todo emit signal for setting "dirty" flag within the design
    if (!dataSource().isEmpty()) {
        signalValueChanged();
    }
}

void KexiDBImageBox::handleAboutToSaveAsAction(
    QString* origFilename, QString* mimeType, bool *dataIsEmpty)
{
    Q_ASSERT(origFilename);
    Q_ASSERT(mimeType);
    Q_ASSERT(dataIsEmpty);
    if (data().isEmpty()) {
        qWarning() << "no pixmap!";
        *dataIsEmpty = false;
        return;
    }
    if (dataSource().isEmpty()) { //for static images filename and mimetype can be available
        *origFilename = m_data.originalFileName();
        if (!origFilename->isEmpty()) {
            *origFilename = QLatin1String("/") + *origFilename;
        }
        const QMimeDatabase db;
        const QMimeType mime(db.mimeTypeForName(m_data.mimeType()));
        if (!m_data.mimeType().isEmpty() && QImageReader::supportedMimeTypes().contains(mime.name().toLatin1())) {
            *mimeType = mime.name();
        }
    }
}

bool KexiDBImageBox::handleSaveAsAction(const QUrl &url)
{
    //! @todo handle remote URLs
    QFile f(url.toLocalFile());
    if (!f.open(QIODevice::WriteOnly)) {
        //! @todo err msg
        return false;
    }
    f.write(data());
    if (f.error() != QFile::NoError) {
        //! @todo err msg
        f.close();
        return false;
    }
    f.close();
    return true;
}

void KexiDBImageBox::handleCutAction()
{
    if (!dataSource().isEmpty() && isReadOnly())
        return;
    handleCopyAction();
    clear();
}

void KexiDBImageBox::handleCopyAction()
{
    qApp->clipboard()->setPixmap(pixmap(), QClipboard::Clipboard);
}

void KexiDBImageBox::handlePasteAction()
{
    if (isReadOnly() || (!designMode() && !hasFocus()))
        return;
    QPixmap pm(qApp->clipboard()->pixmap(QClipboard::Clipboard));
    if (dataSource().isEmpty()) {
        //static mode
        KexiBLOBBuffer::Handle h = KexiBLOBBuffer::self()->insertPixmap(pm);
        if (!h)
            return;
        setData(h);
    } else {
        //db-aware mode
        m_pixmap = pm;
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        if (m_pixmap.save(&buffer, "PNG")) {  // write pixmap into ba in PNG format
            setValueInternal(ba, true, false/* !loadPixmap */);
            m_currentScaledPixmap = QPixmap(); // clear cache
        } else {
            setValueInternal(QByteArray(), true);
        }
    }

    repaint();
    if (!dataSource().isEmpty()) {
        signalValueChanged();
    }
}

void KexiDBImageBox::clear()
{
    if (dataSource().isEmpty()) {
        //static mode
        setData(KexiBLOBBuffer::Handle());
    } else {
        if (isReadOnly())
            return;
        //db-aware mode
        setValueInternal(QByteArray(), true);
    }

    //! @todo emit signal for setting "dirty" flag within the design

    repaint();
    if (!dataSource().isEmpty()) {
        signalValueChanged();
    }
}

void KexiDBImageBox::handleShowPropertiesAction()
{
    //! @todo
}

void KexiDBImageBox::slotUpdateActionsAvailabilityRequested(bool* valueIsNull, bool* valueIsReadOnly)
{
    Q_ASSERT(valueIsNull);
    Q_ASSERT(valueIsReadOnly);
    *valueIsNull = !(
                         (dataSource().isEmpty() && !pixmap().isNull()) /*static pixmap available*/
                      || (!dataSource().isEmpty() && !this->valueIsNull())  /*db-aware pixmap available*/
                  );
    // read-only if static pixmap or db-aware pixmap for read-only widget:
    *valueIsReadOnly =
           (!designMode() && dataSource().isEmpty())
        || (!dataSource().isEmpty() && isReadOnly())
        || (designMode() && !dataSource().isEmpty());
}

void KexiDBImageBox::contextMenuEvent(QContextMenuEvent * e)
{
    if (popupMenuAvailable())
        m_contextMenu->exec(e->globalPos());
}

void KexiDBImageBox::updateActionStrings()
{
    if (!m_contextMenu)
        return;
    if (designMode()) {
    }
    else {
        //update title in data view mode, based on the data source
        if (columnInfo()) {
            KexiImageContextMenu::updateTitle(m_contextMenu, columnInfo()->captionOrAliasOrName(),
                                              KexiFormManager::self()->library()->iconName(metaObject()->className()));
        }
    }

    if (m_chooser) {
        if (popupMenuAvailable() && dataSource().isEmpty()) { //this may work in the future (see @todo below)
            m_chooser->setToolTip(xi18n("Click to show actions for this image box"));
        } else {
            QString beautifiedImageBoxName;
            if (designMode()) {
                beautifiedImageBoxName = dataSource();
            } else {
                beautifiedImageBoxName = columnInfo() ? columnInfo()->captionOrAliasOrName() : QString();
                /*! @todo look at makeFirstCharacterUpperCaseInCaptions setting [bool]
                 (see doc/dev/settings.txt) */
                beautifiedImageBoxName = beautifiedImageBoxName[0].toUpper() + beautifiedImageBoxName.mid(1);
            }
            m_chooser->setToolTip(
                xi18n("Click to show actions for <interface>%1</interface> image box", beautifiedImageBoxName));
        }
    }
}

bool KexiDBImageBox::popupMenuAvailable()
{
    /*! @todo add kexi-global setting which anyway, allows to show this button
              (read-only actions like copy/save as/print can be available) */
    //chooser button can be only visible when data source is specified
    return !dataSource().isEmpty();
}

void KexiDBImageBox::setDataSource(const QString &ds)
{
    KexiFormDataItemInterface::setDataSource(ds);
    setData(KexiBLOBBuffer::Handle());
    updateActionStrings();
    KexiFrame::setFocusPolicy(focusPolicy());   //set modified policy

    if (m_chooser) {
        m_chooser->setEnabled(popupMenuAvailable());
        if (m_dropDownButtonVisible && popupMenuAvailable()) {
            m_chooser->show();
        } else {
            m_chooser->hide();
        }
    }

    // update some properties not changed by user
//! @todo get default line width from global style settings
//    if (!m_lineWidthChanged) {
//        KexiFrame::setLineWidth(ds.isEmpty() ? 0 : 1);
//    }
    if (!m_paletteBackgroundColorChanged && parentWidget()) {
        QPalette p = palette();
        p.setColor(backgroundRole(),
                   dataSource().isEmpty()
                     ? parentWidget()->palette().color(parentWidget()->backgroundRole())
                     : palette().color(QPalette::Active, QPalette::Base)
        );
        KexiFrame::setPalette(p);
    }
}

QSize KexiDBImageBox::sizeHint() const
{
    if (pixmap().isNull())
        return QSize(80, 80);
    return pixmap().size();
}

int KexiDBImageBox::realLineWidth() const
{
    switch (frameShape()) {
    case QFrame::NoFrame:
        // shadow, line, midline unused
        return 0;
    case QFrame::Box:
        switch (frameShadow()) {
        case QFrame::Plain:
            // midline unused
            return lineWidth();
        default: // sunken, raised:
            return 2 * lineWidth() + midLineWidth();
        }
        break;
    case QFrame::Panel:
        // shadow, midline unused
        return lineWidth();
    case QFrame::WinPanel:
        // shadow, line, midline unused
        return 2;
    case QFrame::StyledPanel: {
        // shadow, line, midline unused
        QStyleOptionFrame option;
        option.initFrom(this);
        return style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, this);
    }
    default:
        return lineWidth();
    }
}

static QPixmap *scaledImageBoxIcon(const QMargins& margins, const QSize& size)
{
    const int realHeight = size.height() - margins.top() - margins.bottom();
    const int realWidth = size.width() - margins.left() - margins.right();
    if (   realHeight <= KexiDBImageBox_static->pixmap->height()
        || realWidth <= KexiDBImageBox_static->pixmap->width())
    {
        if (   realHeight <= KexiDBImageBox_static->small->height()
            || realWidth <= KexiDBImageBox_static->small->width())
        {
            return 0;
        }
        return KexiDBImageBox_static->small;
    }
    return KexiDBImageBox_static->pixmap;
}

void KexiDBImageBox::paintEvent(QPaintEvent *pe)
{
    if (!m_paintEventEnabled)
        return;
    QPainter p(this);
    p.setClipRect(pe->rect());
    QMargins margins(contentsMargins());
    margins += realLineWidth();
    if (designMode() && pixmap().isNull()) {
        QRect r(
            QPoint(margins.left(), margins.top()),
            size() - QSize(margins.left() + margins.right(), margins.top() + margins.bottom()));

        updatePixmap();
        QPixmap *imagBoxPm = scaledImageBoxIcon(margins, size());
        if (imagBoxPm) {
            p.drawPixmap(2, height() - margins.top() - margins.bottom() - imagBoxPm->height() - 2,
                         *imagBoxPm);
        }
        QFont f(qApp->font());
        p.setFont(f);
        const QFontMetrics fm(fontMetrics());
        QString text;
        if (dataSource().isEmpty()) {
            if ((fm.height() * 2) > height()) {
                text = xi18nc("Unbound Image Box", "%1 (unbound)", objectName());
            } else {
                text = xi18nc("Unbound Image Box", "%1\n(unbound)", objectName());
            }
        }
        else {
            text = dataSource();
            const QPixmap dataSourceTagIcon(KexiFormUtils::dataSourceTagIcon());
            if (width() >= (dataSourceTagIcon.width() + 2 + fm.boundingRect(r, Qt::AlignCenter, text).width())) {
                r.setLeft( r.left() + dataSourceTagIcon.width() + 2 ); // make some room for the [>] icon
                QRect bounding = fm.boundingRect(r, Qt::AlignCenter, text);
                p.drawPixmap(
                    bounding.left() - dataSourceTagIcon.width() - 2,
                    bounding.top() + bounding.height() / 2 - dataSourceTagIcon.height() / 2,
                    dataSourceTagIcon);
            }
        }
        p.drawText(r, Qt::AlignCenter, text);
    }
    else {
        QSize internalSize(size());
        if (m_chooser && m_dropDownButtonVisible && !dataSource().isEmpty())
            internalSize.setWidth(internalSize.width() - m_chooser->width());

        const QRect internalRect(QPoint(0, 0), internalSize);
        if (m_currentScaledPixmap.isNull() || internalRect != m_currentRect) {
            m_currentRect = internalRect;
            m_currentPixmapPos = QPoint(0, 0);
            m_currentScaledPixmap = KexiUtils::scaledPixmap(
                margins, m_currentRect, pixmap(), &m_currentPixmapPos, m_alignment,
                m_scaledContents, m_keepAspectRatio,
                m_smoothTransformation ? Qt::SmoothTransformation : Qt::FastTransformation);
        }
        p.drawPixmap(m_currentPixmapPos, m_currentScaledPixmap);
    }
    KexiFrame::drawFrame(&p);

    if (designMode()) {
        const bool hasFrame = frameWidth() >= 1 && frameShape() != QFrame::NoFrame;
        if (!hasFrame) {
            KFormDesigner::paintWidgetFrame(p, rect());
        }
    }
    else { // data mode
        // if the widget is focused, draw focus indicator rect _if_ there is no chooser button
        if (   !dataSource().isEmpty()
            && hasFocus()
            && (!m_chooser || !m_chooser->isVisible()))
        {
            QStyleOptionFocusRect option;
            option.initFrom(this);
            style()->drawPrimitive(
                QStyle::PE_FrameFocusRect, &option, &p, this);
        }
    }
}

void KexiDBImageBox::updatePixmap()
{
    if (!(designMode() && pixmap().isNull()))
        return;

    if (!KexiDBImageBox_static->pixmap) {
        const QIcon icon(KexiIcon("imagebox"));
        KexiDBImageBox_static->pixmap = new QPixmap(
            icon.pixmap(KIconLoader::SizeLarge, KIconLoader::SizeLarge, QIcon::Disabled));
        if (!KexiDBImageBox_static->pixmap->isNull()) {
            KIconEffect::semiTransparent(*KexiDBImageBox_static->pixmap);
            KIconEffect::semiTransparent(*KexiDBImageBox_static->pixmap);
        }
        KexiDBImageBox_static->small = new QPixmap(
            icon.pixmap(KIconLoader::SizeSmall, KIconLoader::SizeSmall, QIcon::Disabled));
        if (!KexiDBImageBox_static->small->isNull()) {
            KIconEffect::semiTransparent(*KexiDBImageBox_static->small); // once is enough for small
        }
    }
}

void KexiDBImageBox::setAlignment(Qt::Alignment alignment)
{
    m_alignment = alignment;
    m_currentScaledPixmap = QPixmap(); // clear cache
    repaint();
}

void KexiDBImageBox::setData(const KexiBLOBBuffer::Handle& handle)
{
    if (m_insideSetData) //avoid recursion
        return;
    m_insideSetData = true;
    m_data = handle;
    m_currentScaledPixmap = QPixmap(); // clear cache
    emit idChanged(handle.id());
    m_insideSetData = false;
    update();
}

void KexiDBImageBox::resizeEvent(QResizeEvent * e)
{
    KexiFrame::resizeEvent(e);
    if (m_chooser) {
        QSize s(m_chooser->sizeHint());
        const int _realLineWidth = realLineWidth();
        QSize margin(_realLineWidth, _realLineWidth);
        s.setHeight(height() - 2*margin.height());
        s = s.boundedTo(size() - 2 * margin);
        m_chooser->resize(s);
        m_chooser->move(QRect(QPoint(0, 0), e->size() - m_chooser->size() - margin + QSize(1, 1)).bottomRight());
    }
}

void KexiDBImageBox::setColumnInfo(KDbConnection *conn,KDbQueryColumnInfo* cinfo)
{
    KexiFormDataItemInterface::setColumnInfo(conn, cinfo);
    //updating strings and title is needed
    updateActionStrings();
}

bool KexiDBImageBox::keyPressed(QKeyEvent *ke)
{
    // Esc key should close the popup
    if (ke->modifiers() == Qt::NoModifier && ke->key() == Qt::Key_Escape) {
        if (m_contextMenu->isVisible()) {
            m_setFocusOnButtonAfterClosingPopup = true;
            return true;
        }
    }
    return false;
}

void KexiDBImageBox::setPalette(const QPalette &pal)
{
    KexiFrame::setPalette(pal);
    if (m_insideSetPalette)
        return;
    m_insideSetPalette = true;
    setPaletteBackgroundColor(pal.color(QPalette::Active, QPalette::Base));
    QPalette p(palette());
    p.setColor(foregroundRole(), pal.color(foregroundRole()));
    setPalette(p);
    m_insideSetPalette = false;
}

void KexiDBImageBox::setPaletteBackgroundColor(const QColor & color)
{
    m_paletteBackgroundColorChanged = true;
    QPalette pal(palette());
    pal.setColor(backgroundRole(), color);
    setPalette(pal);
    if (m_chooser)
        m_chooser->setPalette(qApp->palette());
}

bool KexiDBImageBox::dropDownButtonVisible() const
{
    return m_dropDownButtonVisible;
}

int KexiDBImageBox::lineWidth() const
{
    return KexiFrame::lineWidth();
}

void KexiDBImageBox::setDropDownButtonVisible(bool set)
{
//! @todo use global default setting for this property
    if (m_dropDownButtonVisible == set)
        return;
    m_dropDownButtonVisible = set;
    if (m_chooser) {
        if (m_dropDownButtonVisible)
            m_chooser->show();
        else
            m_chooser->hide();
    }
}

bool KexiDBImageBox::subwidgetStretchRequired(KexiDBAutoField* autoField) const
{
    Q_UNUSED(autoField);
    return true;
}

bool KexiDBImageBox::eventFilter(QObject * watched, QEvent * e)
{
    if (watched == this || watched == m_chooser) { //we're watching chooser as well because it's a focus proxy even if invisible
        if (e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut || e->type() == QEvent::MouseButtonPress) {
            update(); //to repaint focus rect
        }
    }
    // hide popup menu as soon as it loses focus
    if (watched == m_contextMenu && e->type() == QEvent::FocusOut) {
        m_contextMenu->hide();
    }
    return KexiFrame::eventFilter(watched, e);
}

void KexiDBImageBox::setValueInternal(const QVariant& add, bool removeOld)
{
    setValueInternal(add, removeOld, true /*loadPixmap*/);
}

Qt::FocusPolicy KexiDBImageBox::focusPolicy() const
{
    if (dataSource().isEmpty())
        return Qt::NoFocus;
    return m_focusPolicyInternal;
}

Qt::FocusPolicy KexiDBImageBox::focusPolicyInternal() const
{
    return m_focusPolicyInternal;
}

void KexiDBImageBox::setFocusPolicy(Qt::FocusPolicy policy)
{
    m_focusPolicyInternal = policy;
    KexiFrame::setFocusPolicy(focusPolicy());   //set modified policy
}

void KexiDBImageBox::setFrameShape(QFrame::Shape s)
{
    KexiFrame::setFrameShape(s);
    m_currentScaledPixmap = QPixmap(); // clear cache
    update();
}

void KexiDBImageBox::setFrameShadow(QFrame::Shadow s)
{
    KexiFrame::setFrameShadow(s);
    m_currentScaledPixmap = QPixmap(); // clear cache
    update();
}

void KexiDBImageBox::setLineWidth(int w)
{
    KexiFrame::setLineWidth(w);
    m_currentScaledPixmap = QPixmap(); // clear cache
    update();
}

void KexiDBImageBox::setMidLineWidth(int w)
{
    KexiFrame::setMidLineWidth(w);
    m_currentScaledPixmap = QPixmap(); // clear cache
    update();
}

