/*
 * Kexi Report Plugin
 * Copyright (C) 2007-2008 by Adam Pigg (adam@piggz.co.uk)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact info@openmfg.com with any questions on this license.
 */
#include "krscripttext.h"

namespace Scripting
{

Text::Text(KRTextData* t)
{
    m_text = t;
}


Text::~Text()
{
}

QString Text::source()
{
    return m_text->column();
}

void Text::setSource(const QString& s)
{
    m_text->m_controlSource->setValue(s);
}

int Text::horizontalAlignment()
{
    QString a = m_text->m_horizontalAlignment->value().toString();

    if (a.toLower() == "left") {
        return -1;
    } else if (a.toLower() == "center") {
        return 0;
    } else if (a.toLower() == "right") {
        return 1;
    }
    return -1;
}
void Text::setHorizonalAlignment(int a)
{
    switch (a) {
    case -1:
        m_text->m_horizontalAlignment->setValue("Left");
        break;
    case 0:
        m_text->m_horizontalAlignment->setValue("Center");
        break;
    case 1:
        m_text->m_horizontalAlignment->setValue("Right");
        break;
    default:
        m_text->m_horizontalAlignment->setValue("Left");
        break;
    }
}

int Text::verticalAlignment()
{
    QString a = m_text->m_horizontalAlignment->value().toString();

    if (a.toLower() == "top") {
        return -1;
    } else if (a.toLower() == "middle") {
        return 0;
    } else if (a.toLower() == "bottom") {
        return 1;
    }
    return -1;
}
void Text::setVerticalAlignment(int a)
{
    switch (a) {
    case -1:
        m_text->m_horizontalAlignment->setValue("Top");
        break;
    case 0:
        m_text->m_horizontalAlignment->setValue("Middle");
        break;
    case 1:
        m_text->m_horizontalAlignment->setValue("Bottom");
        break;
    default:
        m_text->m_horizontalAlignment->setValue("Top");
        break;
    }
}

QColor Text::backgroundColor()
{
    return m_text->m_backgroundColor->value().value<QColor>();
}
void Text::setBackgroundColor(const QColor& c)
{
    m_text->m_backgroundColor->setValue(QColor(c));
}

QColor Text::foregroundColor()
{
    return m_text->m_foregroundColor->value().value<QColor>();
}
void Text::setForegroundColor(const QColor& c)
{
    m_text->m_foregroundColor->setValue(QColor(c));
}

int Text::backgroundOpacity()
{
    return m_text->m_backgroundOpacity->value().toInt();
}
void Text::setBackgroundOpacity(int o)
{
    m_text->m_backgroundOpacity->setValue(o);
}

QColor Text::lineColor()
{
    return m_text->m_lineColor->value().value<QColor>();
}
void Text::setLineColor(const QColor& c)
{
    m_text->m_lineColor->setValue(QColor(c));
}

int Text::lineWeight()
{
    return m_text->m_lineWeight->value().toInt();
}
void Text::setLineWeight(int w)
{
    m_text->m_lineWeight->setValue(w);
}

int Text::lineStyle()
{
    return m_text->m_lineStyle->value().toInt();
}
void Text::setLineStyle(int s)
{
    if (s < 0 || s > 5) {
        s = 1;
    }
    m_text->m_lineStyle->setValue(s);
}

QPointF Text::position()
{
    return m_text->m_pos.toPoint();
}
void Text::setPosition(const QPointF& p)
{
    m_text->m_pos.setPointPos(p);
}

QSizeF Text::size()
{
    return m_text->m_size.toPoint();
}
void Text::setSize(const QSizeF& s)
{
    m_text->m_size.setPointSize(s);
}
}
