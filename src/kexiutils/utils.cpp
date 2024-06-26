/* This file is part of the KDE project
   Copyright (C) 2003-2018 Jarosław Staniek <staniek@kde.org>

   Contains code from kglobalsettings.cpp:
   Copyright (C) 2000, 2006 David Faure <faure@kde.org>
   Copyright (C) 2008 Friedrich W. H. Kossebau <kossebau@kde.org>

   Contains code from kdialog.cpp:
   Copyright (C) 1998 Thomas Tanghus (tanghus@earthling.net)
   Additions 1999-2000 by Espen Sand (espen@kde.org)
                       and Holger Freyther <freyther@kde.org>
             2005-2009 Olivier Goffart <ogoffart @ kde.org>
             2006      Tobias Koenig <tokoe@kde.org>

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

#include "utils.h"
#include "utils_p.h"
#include "FontSettings_p.h"
#include "kexiutils_global.h"
#include <KexiIcon.h>

#include <QDomNode>
#include <QPainter>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QIcon>
#include <QMetaProperty>
#include <QFocusEvent>
#include <QFile>
#include <QStyle>
#include <QLayout>
#include <KMessageBox>
#include <QFileInfo>
#include <QClipboard>
#include <QMimeDatabase>
#include <QMimeType>
#include <QUrl>
#include <QApplication>
#include <QDesktopWidget>
#include <QFontDatabase>
#include <QTextCodec>
#include <QDebug>
#include <QFileDialog>
#include <QDesktopServices>
#include <QStyleHints>
#include <QLineEdit>
#include <QProcess>

#ifndef KEXI_MOBILE
#include <KFileWidget>
#include <KIO/JobUiDelegate>
#include <KIO/OpenUrlJob>
#include <KRecentDirs>
#include <KRun>
#include <kio_version.h>
#endif
#include <KAboutData>
#include <KColorScheme>
#include <KConfigGroup>
#include <KIconEffect>

#if HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#ifdef Q_OS_WIN
#include <windows.h>

static QRgb qt_colorref2qrgb(COLORREF col)
{
    return qRgb(GetRValue(col), GetGValue(col), GetBValue(col));
}
#endif

using namespace KexiUtils;

DelayedCursorHandler::DelayedCursorHandler(QWidget *widget)
        : startedOrActive(false), m_widget(widget), m_handleWidget(widget)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(show()));
}
void DelayedCursorHandler::start(bool noDelay)
{
    startedOrActive = true;
    m_timer.start(noDelay ? 0 : 1000);
}
void DelayedCursorHandler::stop()
{
    startedOrActive = false;
    m_timer.stop();
    if (m_handleWidget && m_widget) {
        m_widget->unsetCursor();
    } else {
        QApplication::restoreOverrideCursor();
    }
}

void DelayedCursorHandler::show()
{
    const QCursor waitCursor(Qt::WaitCursor);
    if (m_handleWidget && m_widget) {
        m_widget->unsetCursor();
        m_widget->setCursor(waitCursor);
    } else {
        QApplication::restoreOverrideCursor();
        QApplication::setOverrideCursor(waitCursor);
    }
}

Q_GLOBAL_STATIC(DelayedCursorHandler, _delayedCursorHandler)

void KexiUtils::setWaitCursor(bool noDelay)
{
    if (qobject_cast<QApplication*>(qApp)) {
        _delayedCursorHandler->start(noDelay);
    }
}

void KexiUtils::removeWaitCursor()
{
    if (qobject_cast<QApplication*>(qApp)) {
        _delayedCursorHandler->stop();
    }
}

WaitCursor::WaitCursor(bool noDelay)
    : m_handler(nullptr)
{
    setWaitCursor(noDelay);
}

WaitCursor::WaitCursor(QWidget *widget, bool noDelay)
{
    DelayedCursorHandler *handler = new DelayedCursorHandler(widget);
    handler->start(noDelay);
    m_handler = handler;
}

WaitCursor::~WaitCursor()
{
    if (m_handler) {
        qobject_cast<DelayedCursorHandler*>(m_handler)->stop();
        delete m_handler;
    } else {
        removeWaitCursor();
    }
}

WaitCursorRemover::WaitCursorRemover()
{
    m_reactivateCursor = _delayedCursorHandler->startedOrActive;
    _delayedCursorHandler->stop();
}

WaitCursorRemover::~WaitCursorRemover()
{
    if (m_reactivateCursor)
        _delayedCursorHandler->start(true);
}

//--------------------------------------------------------------------------------

QObject* KexiUtils::findFirstQObjectChild(QObject *o, const char* className, const char* objName)
{
    if (!o)
        return 0;
    const QObjectList list(o->children());
    foreach(QObject *child, list) {
        if (child->inherits(className) && (!objName || child->objectName() == objName))
            return child;
    }
    //try children
    foreach(QObject *child, list) {
        child = findFirstQObjectChild(child, className, objName);
        if (child)
            return child;
    }
    return 0;
}

QMetaProperty KexiUtils::findPropertyWithSuperclasses(const QObject* object,
        const char* name)
{
    const int index = object->metaObject()->indexOfProperty(name);
    if (index == -1)
        return QMetaProperty();
    return object->metaObject()->property(index);
}

bool KexiUtils::objectIsA(QObject* object, const QList<QByteArray>& classNames)
{
    foreach(const QByteArray& ba, classNames) {
        if (objectIsA(object, ba.constData()))
            return true;
    }
    return false;
}

QList<QMetaMethod> KexiUtils::methodsForMetaObject(
    const QMetaObject *metaObject, QFlags<QMetaMethod::MethodType> types,
    QFlags<QMetaMethod::Access> access)
{
    const int count = metaObject ? metaObject->methodCount() : 0;
    QList<QMetaMethod> result;
    for (int i = 0; i < count; i++) {
        QMetaMethod method(metaObject->method(i));
        if (types & method.methodType() && access & method.access())
            result += method;
    }
    return result;
}

QList<QMetaMethod> KexiUtils::methodsForMetaObjectWithParents(
    const QMetaObject *metaObject, QFlags<QMetaMethod::MethodType> types,
    QFlags<QMetaMethod::Access> access)
{
    QList<QMetaMethod> result;
    while (metaObject) {
        const int count = metaObject->methodCount();
        for (int i = 0; i < count; i++) {
            QMetaMethod method(metaObject->method(i));
            if (types & method.methodType() && access & method.access())
                result += method;
        }
        metaObject = metaObject->superClass();
    }
    return result;
}

QList<QMetaProperty> KexiUtils::propertiesForMetaObject(
    const QMetaObject *metaObject)
{
    const int count = metaObject ? metaObject->propertyCount() : 0;
    QList<QMetaProperty> result;
    for (int i = 0; i < count; i++)
        result += metaObject->property(i);
    return result;
}

QList<QMetaProperty> KexiUtils::propertiesForMetaObjectWithInherited(
    const QMetaObject *metaObject)
{
    QList<QMetaProperty> result;
    while (metaObject) {
        const int count = metaObject->propertyCount();
        for (int i = 0; i < count; i++)
            result += metaObject->property(i);
        metaObject = metaObject->superClass();
    }
    return result;
}

QStringList KexiUtils::enumKeysForProperty(const QMetaProperty& metaProperty, int filter)
{
    QStringList result;
    const QMetaEnum enumerator(metaProperty.enumerator());
    const int count = enumerator.keyCount();
    int total = 0;
    for (int i = 0; i < count; i++) {
        if (filter == INT_MIN) {
            result.append(QString::fromLatin1(enumerator.key(i)));
        } else {
            const int v = enumerator.value(i);
            if ((v & filter) && !(total & v)) { // !(total & v) is a protection adding against masks
                result.append(QString::fromLatin1(enumerator.key(i)));
                total |= v;
            }
        }
    }
    return result;
}

//! @internal
static QFileDialog* getImageDialog(QWidget *parent, const QString &caption, const QUrl &directory,
                                   const QList<QByteArray> &supportedMimeTypes)
{
    QFileDialog *dialog = new QFileDialog(parent, caption);
    dialog->setDirectoryUrl(directory);
    const QStringList mimeTypeFilters
        = KexiUtils::convertTypesUsingFunction<QByteArray, QString, &QString::fromLatin1>(supportedMimeTypes);
    dialog->setMimeTypeFilters(mimeTypeFilters);
    return dialog;
}

QUrl KexiUtils::getOpenImageUrl(QWidget *parent, const QString &caption, const QUrl &directory)
{
    QScopedPointer<QFileDialog> dialog(
        getImageDialog(parent, caption.isEmpty() ? i18n("Open") : caption, directory,
                       QImageReader::supportedMimeTypes()));
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    if (QDialog::Accepted == dialog->exec()) {
        return dialog->selectedUrls().value(0);
    } else {
        return QUrl();
    }
}

QUrl KexiUtils::getSaveImageUrl(QWidget *parent, const QString &caption, const QUrl &directory)
{
    QScopedPointer<QFileDialog> dialog(
        getImageDialog(parent, caption.isEmpty() ? i18n("Save") : caption, directory,
                       QImageWriter::supportedMimeTypes()));
    dialog->setAcceptMode(QFileDialog::AcceptSave);
    if (QDialog::Accepted == dialog->exec()) {
        return dialog->selectedUrls().value(0);
    } else {
        return QUrl();
    }
}

#ifndef KEXI_MOBILE
QUrl KexiUtils::getStartUrl(const QUrl &startDirOrVariable, QString *recentDirClass,
                            const QString &fileName)
{
    QUrl result;
    if (recentDirClass) {
        result = KFileWidget::getStartUrl(startDirOrVariable, *recentDirClass);
        // Fix bug introduced by Kexi 3.0.x in KexiFileWidget: remove file protocol from path
        // (the KRecentDirs::add(.., dir.url()) call was invalid because of prepended protocol)
        const QString protocol("file:/");
        if (result.path().startsWith(protocol) && !result.path().startsWith(protocol + '/')) {
            result.setPath(result.path().mid(protocol.length() - 1));
        }
        if (!fileName.isEmpty()) {
            result.setPath(result.path() + '/' + fileName);
        }
    } else {
        qWarning() << "Missing recentDirClass";
    }
    return result;
}

void KexiUtils::addRecentDir(const QString &fileClass, const QString &directory)
{
    KRecentDirs::add(fileClass, directory);
}
#endif

bool KexiUtils::askForFileOverwriting(const QString& filePath, QWidget *parent)
{
    QFileInfo fi(filePath);
    if (!fi.exists()) {
        return true;
    }
    const KMessageBox::ButtonCode res = KMessageBox::warningTwoActions(parent,
                    xi18nc("@info", "<para>The file <filename>%1</filename> already exists.</para>"
                           "<para>Do you want to overwrite it?</para>",
                           QDir::toNativeSeparators(filePath)),
                    QString(),
                    KStandardGuiItem::overwrite(), KStandardGuiItem::cancel());
    return res == KMessageBox::PrimaryAction;
}

QColor KexiUtils::blendedColors(const QColor& c1, const QColor& c2, int factor1, int factor2)
{
    return QColor(
               int((c1.red()*factor1 + c2.red()*factor2) / (factor1 + factor2)),
               int((c1.green()*factor1 + c2.green()*factor2) / (factor1 + factor2)),
               int((c1.blue()*factor1 + c2.blue()*factor2) / (factor1 + factor2)));
}

QColor KexiUtils::contrastColor(const QColor& c)
{
    int g = qGray(c.rgb());
    if (g > 110)
        return c.dark(200);
    else if (g > 80)
        return c.light(150);
    else if (g > 20)
        return c.light(300);
    return Qt::gray;
}

QColor KexiUtils::bleachedColor(const QColor& c, int factor)
{
    int h, s, v;
    c.getHsv(&h, &s, &v);
    QColor c2;
    if (factor < 100)
        factor = 100;
    if (s >= 250 && v >= 250) //for colors like cyan or red, make the result more white
        s = qMax(0, s - factor - 50);
    else if (s <= 5 && v <= 5)
        v += factor - 50;
    c2.setHsv(h, s, qMin(255, v + factor - 100));
    return c2;
}

QIcon KexiUtils::colorizeIconToTextColor(const QPixmap& icon, const QPalette& palette,
                                         QPalette::ColorRole role)
{
    QPixmap pm(
        KIconEffect().apply(icon, KIconEffect::Colorize, 1.0f,
                            palette.color(role), false));
    KIconEffect::semiTransparent(pm);
    return QIcon(pm);
}

QPixmap KexiUtils::emptyIcon(KIconLoader::Group iconGroup)
{
    const int size = KIconLoader::global()->currentSize(iconGroup);
    QPixmap noIcon(size, size);
    noIcon.fill(Qt::transparent);
    return noIcon;
}

static void drawOrScalePixmapInternal(QPainter* p, const QMargins& margins, const QRect& rect,
                                      QPixmap* pixmap, QPoint* pos, Qt::Alignment alignment,
                                      bool scaledContents, bool keepAspectRatio,
                                      Qt::TransformationMode transformMode = Qt::FastTransformation)
{
    Q_ASSERT(pos);
    if (pixmap->isNull())
        return;

    const bool fast = false;
    const int w = rect.width() - margins.left() - margins.right();
    const int h = rect.height() - margins.top() - margins.bottom();
//! @todo we can optimize painting by drawing rescaled pixmap here
//! and performing detailed painting later (using QTimer)
//    QPixmap pixmapBuffer;
//    QPainter p2;
//    QPainter *target;
//    if (fast) {
//       target = p;
//    } else {
//        target = &p2;
//    }
//! @todo only create buffered pixmap of the minimum size and then do not fillRect()
// target->fillRect(0,0,rect.width(),rect.height(), backgroundColor);

    *pos = rect.topLeft() + QPoint(margins.left(), margins.top());
    if (scaledContents) {
        if (keepAspectRatio) {
            QImage img(pixmap->toImage());
            img = img.scaled(w, h, Qt::KeepAspectRatio, transformMode);
            if (img.width() < w) {
                if (alignment & Qt::AlignRight)
                    pos->setX(pos->x() + w - img.width());
                else if (alignment & Qt::AlignHCenter)
                    pos->setX(pos->x() + w / 2 - img.width() / 2);
            }
            else if (img.height() < h) {
                if (alignment & Qt::AlignBottom)
                    pos->setY(pos->y() + h - img.height());
                else if (alignment & Qt::AlignVCenter)
                    pos->setY(pos->y() + h / 2 - img.height() / 2);
            }
            if (p) {
                p->drawImage(*pos, img);
            }
            else {
                *pixmap = QPixmap::fromImage(img);
            }
        } else {
            if (!fast) {
                *pixmap = pixmap->scaled(w, h, Qt::IgnoreAspectRatio, transformMode);
                if (p) {
                    p->drawPixmap(*pos, *pixmap);
                }
            }
        }
    }
    else {
        if (alignment & Qt::AlignRight)
            pos->setX(pos->x() + w - pixmap->width());
        else if (alignment & Qt::AlignHCenter)
            pos->setX(pos->x() + w / 2 - pixmap->width() / 2);
        else //left, etc.
            pos->setX(pos->x());

        if (alignment & Qt::AlignBottom)
            pos->setY(pos->y() + h - pixmap->height());
        else if (alignment & Qt::AlignVCenter)
            pos->setY(pos->y() + h / 2 - pixmap->height() / 2);
        else //top, etc.
            pos->setY(pos->y());
        *pos += QPoint(margins.left(), margins.top());
        if (p) {
            p->drawPixmap(*pos, *pixmap);
        }
    }
}

void KexiUtils::drawPixmap(QPainter* p, const QMargins& margins, const QRect& rect,
                           const QPixmap& pixmap, Qt::Alignment alignment, bool scaledContents,
                           bool keepAspectRatio, Qt::TransformationMode transformMode)
{
    QPixmap px(pixmap);
    QPoint pos;
    drawOrScalePixmapInternal(p, margins, rect, &px, &pos, alignment, scaledContents,
                              keepAspectRatio, transformMode);
}

QPixmap KexiUtils::scaledPixmap(const QMargins& margins, const QRect& rect,
                                const QPixmap& pixmap, QPoint* pos, Qt::Alignment alignment,
                                bool scaledContents, bool keepAspectRatio,
                                Qt::TransformationMode transformMode)
{
    QPixmap px(pixmap);
    drawOrScalePixmapInternal(0, margins, rect, &px, pos, alignment, scaledContents, keepAspectRatio, transformMode);
    return px;
}

bool KexiUtils::loadPixmapFromData(QPixmap *pixmap, const QByteArray &data, const char *format)
{
    bool ok = pixmap->loadFromData(data, format);
    if (ok) {
        return true;
    }
    if (format) {
        return false;
    }
    const QList<QByteArray> commonFormats({"png", "jpg", "bmp", "tif"});
    QList<QByteArray> formats(commonFormats);
    for(int i=0; ;) {
        ok = pixmap->loadFromData(data, formats[i]);
        if (ok) {
            return true;
        }
        ++i;
        if (i == formats.count()) {// try harder
            if (i == commonFormats.count()) {
                formats += QImageReader::supportedImageFormats();
                if (formats.count() == commonFormats.count()) {
                    break; // sanity check
                }
            } else {
                break;
            }
        }
    }
    return false;
}

void KexiUtils::setFocusWithReason(QWidget* widget, Qt::FocusReason reason)
{
    if (!widget)
        return;
    QFocusEvent fe(QEvent::FocusIn, reason);
    QCoreApplication::sendEvent(widget, &fe);
}

void KexiUtils::unsetFocusWithReason(QWidget* widget, Qt::FocusReason reason)
{
    if (!widget)
        return;
    QFocusEvent fe(QEvent::FocusOut, reason);
    QCoreApplication::sendEvent(widget, &fe);
}

//--------

void KexiUtils::adjustIfRtl(QMargins *margins)
{
    if (margins && QGuiApplication::isRightToLeft()) {
        const int left = margins->left();
        margins->setLeft(margins->right());
        margins->setRight(left);
    }
}

//---------

Q_GLOBAL_STATIC(FontSettingsData, g_fontSettings)

QFont KexiUtils::smallestReadableFont()
{
    return g_fontSettings->font(FontSettingsData::SmallestReadableFont);
}

//---------------------

KTextEditorFrame::KTextEditorFrame(QWidget * parent, Qt::WindowFlags f)
        : QFrame(parent, f)
{
    QEvent dummy(QEvent::StyleChange);
    changeEvent(&dummy);
}

void KTextEditorFrame::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::StyleChange) {
        if (style()->objectName() != "oxygen") // oxygen already nicely paints the frame
            setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
        else
            setFrameStyle(QFrame::NoFrame);
    }
}

//---------------------

int KexiUtils::marginHint()
{
    return QApplication::style()->pixelMetric(QStyle::PM_DefaultChildMargin);
}

int KexiUtils::spacingHint()
{
    return QApplication::style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing);
}

void KexiUtils::setStandardMarginsAndSpacing(QLayout *layout)
{
    setMargins(layout, KexiUtils::marginHint());
    layout->setSpacing(KexiUtils::spacingHint());
}

void KexiUtils::setMargins(QLayout *layout, int value)
{
    layout->setContentsMargins(value, value, value, value);
}

void KexiUtils::replaceColors(QPixmap* original, const QColor& color)
{
    Q_ASSERT(original);
    QImage dest(original->toImage());
    replaceColors(&dest, color);
    *original = QPixmap::fromImage(dest);
}

void KexiUtils::replaceColors(QImage* original, const QColor& color)
{
    Q_ASSERT(original);
    *original = original->convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter p(original);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(original->rect(), color);
}

bool KexiUtils::isLightColorScheme()
{
    return KColorScheme(QPalette::Active, KColorScheme::Window).background().color().lightness() >= 128;
}

int KexiUtils::dimmedAlpha()
{
    return 150;
}

QPalette KexiUtils::paletteWithDimmedColor(const QPalette &pal, QPalette::ColorGroup group,
                                           QPalette::ColorRole role)
{
    QPalette result(pal);
    QColor color(result.color(group, role));
    color.setAlpha(dimmedAlpha());
    result.setColor(group, role, color);
    return result;
}

QPalette KexiUtils::paletteWithDimmedColor(const QPalette &pal, QPalette::ColorRole role)
{
    QPalette result(pal);
    QColor color(result.color(role));
    color.setAlpha(dimmedAlpha());
    result.setColor(role, color);
    return result;
}

QPalette KexiUtils::paletteForReadOnly(const QPalette &palette)
{
    QPalette p(palette);
    p.setBrush(QPalette::Base, palette.brush(QPalette::Disabled, QPalette::Base));
    p.setBrush(QPalette::Text, palette.brush(QPalette::Disabled, QPalette::Text));
    p.setBrush(QPalette::Highlight, palette.brush(QPalette::Disabled, QPalette::Highlight));
    p.setBrush(QPalette::HighlightedText, palette.brush(QPalette::Disabled, QPalette::HighlightedText));
    return p;
}

void KexiUtils::setBackgroundColor(QWidget *widget, const QColor &color)
{
    widget->setAutoFillBackground(true);
    QPalette pal(widget->palette());
    pal.setColor(widget->backgroundRole(), color);
    widget->setPalette(pal);
}

//---------------------

void KexiUtils::installRecursiveEventFilter(QObject *object, QObject *filter)
{
    if (!object || !filter || !object->isWidgetType())
        return;

//    qDebug() << "Installing event filter on widget:" << object
//        << "directed to" << filter->objectName();
    object->installEventFilter(filter);

    const QObjectList list(object->children());
    foreach(QObject *obj, list) {
        installRecursiveEventFilter(obj, filter);
    }
}

void KexiUtils::removeRecursiveEventFilter(QObject *object, QObject *filter)
{
    object->removeEventFilter(filter);
    if (!object->isWidgetType())
        return;

    const QObjectList list(object->children());
    foreach(QObject *obj, list) {
        removeRecursiveEventFilter(obj, filter);
    }
}

PaintBlocker::PaintBlocker(QWidget* parent)
 : QObject(parent)
 , m_enabled(true)
{
    parent->installEventFilter(this);
}

void PaintBlocker::setEnabled(bool set)
{
    m_enabled = set;
}

bool PaintBlocker::enabled() const
{
    return m_enabled;
}

bool PaintBlocker::eventFilter(QObject* watched, QEvent* event)
{
    if (m_enabled && watched == parent() && event->type() == QEvent::Paint) {
        return true;
    }
    return false;
}

tristate KexiUtils::openHyperLink(const QUrl &url, QWidget *parent, const OpenHyperlinkOptions &options)
{
#ifdef KEXI_MOBILE
    //! @todo
    Q_UNUSED(url)
    Q_UNUSED(parent)
    Q_UNUSED(options)
#else
    if (url.isLocalFile()) {
        QFileInfo fileInfo(url.toLocalFile());
        if (!fileInfo.exists()) {
            KMessageBox::error(parent, xi18nc("@info", "The file or directory <filename>%1</filename> does not exist.", fileInfo.absoluteFilePath()));
            return false;
        }
    }

    if (!url.isValid()) {
        KMessageBox::error(parent, xi18nc("@info", "Invalid hyperlink <link>%1</link>.",
                                          url.url(QUrl::PreferLocalFile)));
        return false;
    }

    QMimeDatabase db;
    QString type = db.mimeTypeForUrl(url).name();

    if (!options.allowExecutable && KRun::isExecutableFile(url, type)) {
        KMessageBox::error(parent, xi18nc("@info", "Executable <link>%1</link> not allowed.",
                                          url.url(QUrl::PreferLocalFile)));
        return false;
    }

    if (!options.allowRemote && !url.isLocalFile()) {
        KMessageBox::error(parent, xi18nc("@info", "Remote hyperlink <link>%1</link> not allowed.",
                                          url.url(QUrl::PreferLocalFile)));
        return false;
    }

    if (KRun::isExecutableFile(url, type)) {
        int ret = KMessageBox::questionTwoActions(parent
                    , xi18nc("@info", "Do you want to run this file?"
                            "<warning>Running executables can be dangerous.</warning>")
                    , QString()
                    , KGuiItem(xi18nc("@action:button Run script file", "Run"), koIconName("system-run"))
                    , KStandardGuiItem::cancel()
                    , "AllowRunExecutable", KMessageBox::Notify | KMessageBox::Dangerous);

        if (ret != KMessageBox::PrimaryAction) {
            return cancelled;
        }
    }

    switch(options.tool) {
        case OpenHyperlinkOptions::DefaultHyperlinkTool:
#if KIO_VERSION >= QT_VERSION_CHECK(5, 71, 0)
        {
            auto *job = new KIO::OpenUrlJob(url, type);
            job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parent));
            job->setRunExecutables(true);
            job->start();
            return true;
        }
#elif KIO_VERSION >= QT_VERSION_CHECK(5, 31, 0)
            return KRun::runUrl(url, type, parent, KRun::RunFlags(KRun::RunExecutables));
#else
            return KRun::runUrl(url, type, parent);
#endif
        case OpenHyperlinkOptions::BrowserHyperlinkTool:
            return QDesktopServices::openUrl(url);
        case OpenHyperlinkOptions::MailerHyperlinkTool:
            return QDesktopServices::openUrl(url);
        default:;
    }
#endif
    return false;
}

// ----

KexiDBDebugTreeWidget::KexiDBDebugTreeWidget(QWidget *parent)
 : QTreeWidget(parent)
{
}

void KexiDBDebugTreeWidget::copy()
{
    if (currentItem()) {
        qApp->clipboard()->setText(currentItem()->text(0));
    }
}

// ----

DebugWindow::DebugWindow(QWidget * parent)
    : QWidget(parent, Qt::Window)
{
}

// ----

QSize KexiUtils::comboBoxArrowSize(QStyle *style)
{
    if (!style) {
        style = QApplication::style();
    }
    QStyleOptionComboBox cbOption;
    return style->subControlRect(QStyle::CC_ComboBox, &cbOption, QStyle::SC_ComboBoxArrow).size();
}

void KexiUtils::addDirtyFlag(QString *text)
{
    Q_ASSERT(text);
    *text = xi18nc("'Dirty (modified) object' flag", "%1*", *text);
}

//! From klocale_kde.cpp
//! @todo KEXI3 support other OS-es (use from klocale_*.cpp)
static QByteArray systemCodeset()
{
    QByteArray codeset;
#if HAVE_LANGINFO_H
    // Qt since 4.2 always returns 'System' as codecForLocale and KDE (for example
    // KEncodingFileDialog) expects real encoding name. So on systems that have langinfo.h use
    // nl_langinfo instead, just like Qt compiled without iconv does. Windows already has its own
    // workaround

    codeset = nl_langinfo(CODESET);

    if ((codeset == "ANSI_X3.4-1968") || (codeset == "US-ASCII")) {
        // means ascii, "C"; QTextCodec doesn't know, so avoid warning
        codeset = "ISO-8859-1";
    }
#endif
    return codeset;
}

QTextCodec* g_codecForEncoding = 0;

bool setEncoding(int mibEnum)
{
    QTextCodec *codec = QTextCodec::codecForMib(mibEnum);
    if (codec) {
        g_codecForEncoding = codec;
    }

    return codec != 0;
}

//! From klocale_kde.cpp
static void initEncoding()
{
    if (!g_codecForEncoding) {
        // This all made more sense when we still had the EncodingEnum config key.

        QByteArray codeset = systemCodeset();

        if (!codeset.isEmpty()) {
            QTextCodec *codec = QTextCodec::codecForName(codeset);
            if (codec) {
                setEncoding(codec->mibEnum());
            }
        } else {
            setEncoding(QTextCodec::codecForLocale()->mibEnum());
        }

        if (!g_codecForEncoding) {
            qWarning() << "Cannot resolve system encoding, defaulting to ISO 8859-1.";
            const int mibDefault = 4; // ISO 8859-1
            setEncoding(mibDefault);
        }
        Q_ASSERT(g_codecForEncoding);
    }
}

QByteArray KexiUtils::encoding()
{
    initEncoding();
    return g_codecForEncoding->name();
}

namespace {

//! @internal for graphicEffectsLevel()
class GraphicEffectsLevel
{
public:
    GraphicEffectsLevel() {
        KConfigGroup g(KSharedConfig::openConfig(), "KDE-Global GUI Settings");

        // Asking for hasKey we do not ask for graphicEffectsLevelDefault() that can
        // contain some very slow code. If we can save that time, do it. (ereslibre)
        if (g.hasKey("GraphicEffectsLevel")) {
            value = ((GraphicEffects) g.readEntry("GraphicEffectsLevel", QVariant((int) NoEffects)).toInt());
            return;
        }

        // For now, let always enable animations by default. The plan is to make
        // this code a bit smarter. (ereslibre)
        value = ComplexAnimationEffects;
    }
    GraphicEffects value;
};
}

Q_GLOBAL_STATIC(GraphicEffectsLevel, g_graphicEffectsLevel)

GraphicEffects KexiUtils::graphicEffectsLevel()
{
    return g_graphicEffectsLevel->value;
}

#if defined Q_OS_UNIX && !defined Q_OS_MACOS
//! For detectedDesktopSession()
class DetectedDesktopSession
{
public:
    DetectedDesktopSession() : name(detect()), isKDE(name == QStringLiteral("KDE"))
    {
    }
    const QByteArray name;
    const bool isKDE;

private:
    static QByteArray detect() {
        // https://www.freedesktop.org/software/systemd/man/pam_systemd.html#%24XDG_SESSION_DESKTOP
        // KDE, GNOME, UNITY, LXDE, MATE, XFCE...
        const QString xdgSessionDesktop = qgetenv("XDG_SESSION_DESKTOP").trimmed();
        if (!xdgSessionDesktop.isEmpty()) {
            return xdgSessionDesktop.toLatin1().toUpper();
        }
        // Similar to detectDesktopEnvironment() from qgenericunixservices.cpp
        const QString xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP").trimmed();
        if (!xdgCurrentDesktop.isEmpty()) {
            return xdgCurrentDesktop.toLatin1().toUpper();
        }
        // fallbacks
        if (!qEnvironmentVariableIsEmpty("KDE_FULL_SESSION")) {
            return QByteArrayLiteral("KDE");
        }
        if (!qEnvironmentVariableIsEmpty("GNOME_DESKTOP_SESSION_ID")) {
            return QByteArrayLiteral("GNOME");
        }
        const QString desktopSession = qgetenv("DESKTOP_SESSION").trimmed();
        if (desktopSession.compare("gnome", Qt::CaseInsensitive) == 0) {
            return QByteArrayLiteral("GNOME");
        } else if (desktopSession.compare("xfce", Qt::CaseInsensitive) == 0) {
            return QByteArrayLiteral("XFCE");
        }
        return QByteArray();
    }
};

Q_GLOBAL_STATIC(DetectedDesktopSession, s_detectedDesktopSession)

QByteArray KexiUtils::detectedDesktopSession()
{
    return s_detectedDesktopSession->name;
}

bool KexiUtils::isKDEDesktopSession()
{
    return s_detectedDesktopSession->isKDE;
}

bool KexiUtils::shouldUseNativeDialogs()
{
#if defined Q_OS_UNIX && !defined Q_OS_MACOS
    return isKDEDesktopSession() || detectedDesktopSession().isEmpty();
#else
    return true;
#endif
}


//! @return value of XFCE property @a property for channel @a channel
//! Sets the value pointed by @a ok to status.
//! @todo Should be part of desktop integration or KF
static QByteArray xfceSettingValue(const QByteArray &channel, const QByteArray &property,
                                   bool *ok = nullptr)
{
    if (ok) {
        *ok = false;
    }
    QByteArray result;
    const QString program = QString::fromLatin1("xfconf-query");
    const QString programPath = QStandardPaths::findExecutable(program);
    const QStringList arguments{ program, "-c", channel, "-p", property };
    QProcess process;
    process.start(programPath, arguments);//, QIODevice::ReadOnly | QIODevice::Text);
    if (!process.waitForStarted()) {
        qWarning() << "Count not execute command" << programPath << arguments
                   << "error:" << process.error();
        return QByteArray();
    }
    const int exitCode = process.exitCode(); // !=0 e.g. for "no such property or channel"
    if (exitCode != 0 || !process.waitForFinished() || process.exitStatus() != QProcess::NormalExit) {
        qWarning() << "Count not finish command" << programPath << arguments
                   << "error:" << process.error() << "exit code:" << exitCode
                   << "exit status:" << process.exitStatus();
        return QByteArray();
    }
    if (ok) {
        *ok = true;
    }
    result = process.readAll();
    result.chop(1);
    return result;
}

#else

QByteArray KexiUtils::detectedDesktopSession()
{
    return QByteArray();
}
#endif

bool KexiUtils::activateItemsOnSingleClick(QWidget *widget)
{
    const KConfigGroup mainWindowGroup = KSharedConfig::openConfig()->group("MainWindow");
#ifdef Q_OS_WIN
    return mainWindowGroup.readEntry("SingleClickOpensItem", true);
#else
    if (mainWindowGroup.hasKey("SingleClickOpensItem")) {
        return mainWindowGroup.readEntry("SingleClickOpensItem", true);
    }
    const QByteArray desktopSession = detectedDesktopSession();
    if (desktopSession == "XFCE") {
        /* To test:
           Set to true: fconf-query -c xfce4-desktop -p /desktop-icons/single-click -n -t bool -s true
           Get value: xfconf-query -c xfce4-desktop -p /desktop-icons/single-click
           Reset: xfconf-query -c xfce4-desktop -p /desktop-icons/single-click -r
        */
        return xfceSettingValue("xfce4-desktop", "/desktop-icons/single-click") == "true";
    }
# if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    Q_UNUSED(widget)
    return QApplication::styleHints()->singleClickActivation();
# else
    QStyle *style = widget ? widget->style() : QApplication::style();
    return style->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, 0, widget);
# endif
#endif
}

// NOTE: keep this in sync with kdebase/workspace/kcontrol/colors/colorscm.cpp
QColor KexiUtils::inactiveTitleColor()
{
#ifdef Q_OS_WIN
    return qt_colorref2qrgb(GetSysColor(COLOR_INACTIVECAPTION));
#else
    KConfigGroup g(KSharedConfig::openConfig(), "WM");
    return g.readEntry("inactiveBackground", QColor(224, 223, 222));
#endif
}

// NOTE: keep this in sync with kdebase/workspace/kcontrol/colors/colorscm.cpp
QColor KexiUtils::inactiveTextColor()
{
#ifdef Q_OS_WIN
    return qt_colorref2qrgb(GetSysColor(COLOR_INACTIVECAPTIONTEXT));
#else
    KConfigGroup g(KSharedConfig::openConfig(), "WM");
    return g.readEntry("inactiveForeground", QColor(75, 71, 67));
#endif
}

// NOTE: keep this in sync with kdebase/workspace/kcontrol/colors/colorscm.cpp
QColor KexiUtils::activeTitleColor()
{
#ifdef Q_OS_WIN
    return qt_colorref2qrgb(GetSysColor(COLOR_ACTIVECAPTION));
#else
    KConfigGroup g(KSharedConfig::openConfig(), "WM");
    return g.readEntry("activeBackground", QColor(48, 174, 232));
#endif
}

// NOTE: keep this in sync with kdebase/workspace/kcontrol/colors/colorscm.cpp
QColor KexiUtils::activeTextColor()
{
#ifdef Q_OS_WIN
    return qt_colorref2qrgb(GetSysColor(COLOR_CAPTIONTEXT));
#else
    KConfigGroup g(KSharedConfig::openConfig(), "WM");
    return g.readEntry("activeForeground", QColor(255, 255, 255));
#endif
}

QString KexiUtils::makeStandardCaption(const QString &userCaption, CaptionFlags flags)
{
    QString caption = KAboutData::applicationData().displayName();
    if (caption.isEmpty()) {
        return QCoreApplication::instance()->applicationName();
    }
    QString captionString = userCaption.isEmpty() ? caption : userCaption;

    // If the document is modified, add '[modified]'.
    if (flags & ModifiedCaption) {
        captionString += QString::fromUtf8(" [") + xi18n("modified") + QString::fromUtf8("]");
    }

    if (!userCaption.isEmpty()) {
        // Add the application name if:
        // User asked for it, it's not a duplication  and the app name (caption()) is not empty
        if (flags & AppNameCaption &&
                !caption.isEmpty() &&
                !userCaption.endsWith(caption)) {
            // TODO: check to see if this is a transient/secondary window before trying to add the app name
            //       on platforms that need this
            captionString += xi18nc("Document/application separator in titlebar", " – ") + caption;
        }
    }
    return captionString;
}

QString themedIconName(const QString &name)
{
    static bool firstUse = true;
    if (firstUse) {
        // workaround for some kde-related crash
        const bool _unused = KIconLoader::global()->iconPath(name, KIconLoader::NoGroup, true).isEmpty();
        Q_UNUSED(_unused);
        firstUse = false;
    }

    // try load themed icon
    const QColor background = qApp->palette().background().color();
    const bool useDarkIcons = background.value() > 100;
    return QLatin1String(useDarkIcons ? "dark_" : "light_") + name;
}

QIcon themedIcon(const QString &name)
{
    const QString realName(themedIconName(name));
    const QIcon icon = QIcon::fromTheme(realName);

    // fallback
    if (icon.isNull()) {
        return QIcon::fromTheme(name);
    }
    return icon;
}

QString KexiUtils::localizedStringToHtmlSubstring(const KLocalizedString &string)
{
    return string.isEmpty() ? QString()
                            : string.toString(Kuit::RichText)
                                  .remove(QLatin1String("<html>"))
                                  .remove(QLatin1String("</html>"));
}

QString KexiUtils::localizedSentencesToHtml(const KLocalizedString &part1, const KLocalizedString &part2,
                                    const KLocalizedString &part3, const KLocalizedString &part4,
                                    const KLocalizedString &part5, const KLocalizedString &part6)
{
    return xi18nc("@info/plain Concatenated sentence1 sentence2 ...", "<html>%1%2%3%4%5%6</html>",
                  KexiUtils::localizedStringToHtmlSubstring(part1),
                  KexiUtils::localizedStringToHtmlSubstring(part2),
                  KexiUtils::localizedStringToHtmlSubstring(part3),
                  KexiUtils::localizedStringToHtmlSubstring(part4),
                  KexiUtils::localizedStringToHtmlSubstring(part5),
                  KexiUtils::localizedStringToHtmlSubstring(part6));
}

bool KexiUtils::cursorAtEnd(const QLineEdit *lineEdit)
{
    if (!lineEdit) {
        return false;
    }
    if (lineEdit->inputMask().isEmpty()) {
        return lineEdit->cursorPosition() >= lineEdit->displayText().length();
    } else {
        return lineEdit->cursorPosition() >= (lineEdit->displayText().length() - 1);
    }
}

QDebug operator<<(QDebug dbg, const QDomNode &node)
{
  QString s;
  QTextStream str(&s, QIODevice::WriteOnly);
  node.save(str, 2);
  dbg << qPrintable(s);
  return dbg;
}
