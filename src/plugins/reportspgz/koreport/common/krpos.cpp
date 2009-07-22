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
#include "krpos.h"
#include <kdebug.h>

KRPos::KRPos(const KoUnit& unit)
{
    m_unit = unit;
    //TODO When KoProperty can handle QPointF
    m_property = new KoProperty::Property("Position", toScene().toPoint(), "Position", "Position");
}

void KRPos::setName(const QString& n)
{
    m_property->setName(n.toLatin1());
    m_property->setCaption(n);
}

KRPos::~KRPos()
{
}

void KRPos::setScenePos(const QPointF& pos, bool update)
{
    qreal x, y;

    x = INCH_TO_POINT(pos.x() / KoGlobal::dpiX());
    y = INCH_TO_POINT(pos.y() / KoGlobal::dpiY());

    m_pointPos.setX(x);
    m_pointPos.setY(y);

    if (update)
        m_property->setValue(toUnit());
}

void KRPos::setUnitPos(const QPointF& pos, bool update)
{
    kDebug() << pos;
    qreal x, y;
    x = m_unit.fromUserValue(pos.x());
    y = m_unit.fromUserValue(pos.y());

    m_pointPos.setX(x);
    m_pointPos.setY(y);

    if (update)
        m_property->setValue(toUnit().toPoint());
}

void KRPos::setPointPos(const QPointF& pos, bool update)
{
    m_pointPos.setX(pos.x());
    m_pointPos.setY(pos.y());

    if (update)
        m_property->setValue(toUnit().toPoint());

}

void KRPos::setUnit(KoUnit u)
{
    m_unit = u;
    m_property->setValue(toUnit().toPoint());
}

QPointF KRPos::toPoint()
{
    return m_pointPos;
}

QPointF KRPos::toScene()
{
    qreal x, y;
    x = POINT_TO_INCH(m_pointPos.x()) * KoGlobal::dpiX();
    y = POINT_TO_INCH(m_pointPos.y()) * KoGlobal::dpiY();

    return QPointF(x, y);
}

QPointF KRPos::toUnit()
{
    qreal x, y;
    x = m_unit.toUserValue(m_pointPos.x());
    y = m_unit.toUserValue(m_pointPos.y());

    return QPointF(x, y);
}

