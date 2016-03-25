/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

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

#include "KexiStyle.h"
#include "utils.h"

#include <QFrame>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QIconEngine>
#include <QFile>
#include <QSvgRenderer>

namespace KexiStyle {

KEXIUTILS_EXPORT void setupFrame(QFrame *frame)
{
    if (frame) {
        frame->setFrameStyle(QFrame::NoFrame);
    }
}

KEXIUTILS_EXPORT void setupModeSelector(QFrame *selector)
{
    KexiStyle::setupFrame(selector);
    selector->setFont(KexiUtils::smallestReadableFont());
    QPalette p(selector->palette());
    p.setColor(QPalette::Window, KexiUtils::shadeBlack());
    p.setColor(QPalette::Base, KexiUtils::shadeBlack());
    p.setColor(QPalette::Button, KexiUtils::shadeBlack());
    p.setColor(QPalette::AlternateBase, KexiUtils::shadeBlackLighter());
    p.setColor(QPalette::WindowText, KexiUtils::cardboardGrey());
    p.setColor(QPalette::ButtonText, KexiUtils::cardboardGrey());
    p.setColor(QPalette::Text, KexiUtils::cardboardGrey());
    p.setColor(QPalette::Highlight, KexiUtils::cardboardGrey());
    p.setColor(QPalette::Active, QPalette::Highlight, KexiUtils::plasmaBlue()); // unused anyway because mode selector has no focus
    p.setColor(QPalette::HighlightedText, KexiUtils::charcoalGrey());
    selector->setPalette(p);
}

KEXIUTILS_EXPORT void overpaintModeSelector(QWidget *widget, QPainter *painter,
                                            const QRect &selectedRect)
{
    // draw gradient
    painter->save();
    int w = widget->fontMetrics().height() * 3 / 2;
    painter->translate(widget->width() - w, 0);
    QLinearGradient grad(0, 0, w, 0);
    QColor c(widget->palette().base().color());
    c.setAlpha(0);
    grad.setColorAt(0, c);
    c.setAlpha(15);
    grad.setColorAt(0.5, c);
    grad.setColorAt(1.0, QColor(0, 0, 0, 50));
    painter->fillRect(0, 0, w, widget->height(), QBrush(grad));
    painter->restore();

    // draw: /|
    //       \|
    if (!selectedRect.isNull()) {
        painter->save();
        w = selectedRect.height() / 10;
        if (w % 2 == 0) {
            ++w;
        }
        painter->translate(selectedRect.x() + selectedRect.width() - w,
                           selectedRect.y() + (selectedRect.height() - w * 2) / 2 - 0.5);
        QPolygon polygon;
        polygon << QPoint(w, 0) << QPoint(w, w * 2) << QPoint(0, w);
        painter->setPen(QPen(Qt::NoPen));
        painter->setBrush(KexiUtils::charcoalGrey());
        painter->drawPolygon(polygon);
        painter->restore();
    }
}

KEXIUTILS_EXPORT void overpaintModeSelectorItem(QPainter *painter,
                                                const QStyleOptionViewItem &option,
                                                const QModelIndex &index)
{
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

KEXIUTILS_EXPORT QPalette alternativePalette(const QPalette &palette)
{
    QPalette p(palette);
    p.setColor(QPalette::Window, KexiUtils::charcoalGrey());
    p.setColor(QPalette::Base, KexiUtils::shadeBlack());
    p.setColor(QPalette::Button, KexiUtils::shadeBlack());
    p.setColor(QPalette::AlternateBase, KexiUtils::shadeBlackLighter());
    p.setColor(QPalette::WindowText, KexiUtils::paperWhite());
    p.setColor(QPalette::ButtonText, KexiUtils::paperWhite());
    p.setColor(QPalette::Text, KexiUtils::paperWhite());
    p.setColor(QPalette::Highlight, KexiUtils::cardboardGrey());
    p.setColor(QPalette::Active, QPalette::Highlight, KexiUtils::plasmaBlue());
    p.setColor(QPalette::HighlightedText, KexiUtils::charcoalGrey());
    p.setColor(QPalette::Active, QPalette::HighlightedText, KexiUtils::cardboardGrey());
    return p;
}

KEXIUTILS_EXPORT QPalette sidebarsPalette(const QPalette &palette)
{
    return alternativePalette(palette);
}

KEXIUTILS_EXPORT void setSidebarsPalette(QWidget *widget)
{
    widget->setPalette(sidebarsPalette(widget->palette()));
    widget->setAutoFillBackground(true);
}

KEXIUTILS_EXPORT QFont titleFont(const QFont &font)
{
    QFont newFont(font);
    newFont.setCapitalization(QFont::AllUppercase);
    return newFont;
}

static const QString g_contexts[] = {
    QLatin1String("actions"), // Any
    QLatin1String("actions"),
    QLatin1String("apps"),
    QLatin1String("devices"),
    QLatin1String("filesystems"),
    QLatin1String("mimetypes"),
    QLatin1String("animations"),
    QLatin1String("categories"),
    QLatin1String("emblems"),
    QLatin1String("emotes"),
    QLatin1String("intl"),
    QLatin1String("places"),
    QLatin1String("status")
};

KEXIUTILS_EXPORT QIcon darkIcon(const QString &iconName, KIconLoader::Context iconContext)
{
    Q_ASSERT(iconContext < (sizeof(g_contexts) / sizeof(g_contexts[0])));
    static const QIcon::Mode modes[] = { QIcon::Normal }; //can be supported too: , QIcon::Selected };
    const QString prefix(QLatin1String(":/icons/breeze/")
                         + g_contexts[iconContext] + QLatin1Char('/'));
    const QString suffixes[] = {
        iconName + QLatin1String("@dark.svg"),
        iconName + QLatin1String(".svg") };
    static const QString sizesStr[] = {
        QString::fromLatin1("32/"), // important: opposite direction
        QString::fromLatin1("22/"),
        QString::fromLatin1("16/") };
    static const QSize sizes[] = { QSize(32, 32), QSize(22, 22), QSize(16, 16) }; // important: opposite direction
    QIcon icon;
    for (int mode = 0; mode < int(sizeof(modes) / sizeof(modes[0])); ++mode) {
        for (int size = 0; size < int(sizeof(sizes) / sizeof(sizes[0])); ++size) {
            //qDebug() << prefix + sizesStr[size] + suffixes[mode] << sizes[size] << modes[mode];
            icon.addFile(prefix + sizesStr[size] + suffixes[mode], sizes[size], modes[mode], QIcon::Off);
            icon.addFile(prefix + sizesStr[size] + suffixes[mode], sizes[size], modes[mode], QIcon::On);
        }
    }
    return icon;
}

class IconEngine : public QIconEngine
{
public:
    IconEngine(const KexiStyledIconParameters &parameters)
     : m_parameters(parameters)
    {
    }
    inline QIconEngine *clone() const Q_DECL_OVERRIDE {
        return new IconEngine(*this);
    }

    //! @todo add caching?
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE {
        Q_UNUSED(state)
        QFile f(QLatin1String(":/icons/breeze/") + g_contexts[m_parameters.context] + QChar('/') + QString::number(size.width())
                + QChar('/') + m_parameters.name + ".svg");
        if (!f.open(QIODevice::ReadOnly)) {
            return QPixmap();
        }
        QByteArray svg(f.readAll());
        QColor color;
        if (mode == QIcon::Selected && m_parameters.selectedColor.isValid()) {
            color = m_parameters.selectedColor;
        } else if (mode == QIcon::Disabled && m_parameters.disabledColor.isValid()) {
            color = m_parameters.disabledColor;
            qDebug() << m_parameters.disabledColor;
        } else if (m_parameters.color.isValid()) {
            color = m_parameters.color;
        }
        if (color.isValid()) {
            svg.replace(KexiUtils::iconGrey().name().toLatin1(), color.name().toLatin1());
        }
        QSvgRenderer renderer(svg);
        QPixmap pm(size);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        renderer.render(&p, pm.rect());
        return pm;
    }

    //! Nothing to paint extra here
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE {
        Q_UNUSED(painter)
        Q_UNUSED(rect)
        Q_UNUSED(mode)
        Q_UNUSED(state)
    }

private:
    //! Needed for clone()
    IconEngine(const IconEngine &other) : m_parameters(other.m_parameters) {}

    const KexiStyledIconParameters m_parameters;
};

KEXIUTILS_EXPORT QIcon icon(const KexiStyledIconParameters &parameters)
{
    return QIcon(new IconEngine(parameters));
}

}
