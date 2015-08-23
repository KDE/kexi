/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2011  Radosław Wicik <radoslaw@wicik.pl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "MapBrowserWidget.h"
#include <kexiutils/utils.h>

#include <QDebug>

MapBrowserWidget::MapBrowserWidget(QWidget *parent)
  : Marble::MarbleWidget(parent),
    KFormDesigner::FormWidgetInterface(),
    KexiFormDataItemInterface(),
    m_slotMapChanged_enabled(true),
    m_internalReadOnly(false)
{
#ifndef Q_CC_MSVC
#warning this id could be invalid; try to use Marble::MapThemeManager::mapThemes() and get proper Marble::GeoSceneDocument::head()->mapThemeId()
#endif
  setMapThemeId("earth/srtm/srtm.dgml");
  m_defaultZoom = 1200; // with this value "more or less" entire earth fits the window. Please check the MarbleWidget::setZoom documentation for more info.
  setAnimationsEnabled(true);
  connect( this, SIGNAL(visibleLatLonAltBoxChanged(GeoDataLatLonAltBox)), this , SLOT(slotMapChanged()));
}

MapBrowserWidget::~MapBrowserWidget()
{

}

QVariant MapBrowserWidget::value()
{
    if (dataSource().isEmpty()){
        return serializeData(0.0, 0.0, m_defaultZoom);
    }
    return serializeData(centerLatitude(), centerLongitude(), zoom());
}

void MapBrowserWidget::setValueInternal(const QVariant& add, bool removeOld)
{
    Q_UNUSED(add);
    Q_UNUSED(removeOld);
    m_slotMapChanged_enabled = false;
    //qDebug() << "add:" << add;
    //qDebug() << "originalValue():" << originalValue();
    deserializeData(originalValue());
    m_slotMapChanged_enabled = true;
}

bool MapBrowserWidget::valueIsNull()
{
    return false;
}

bool MapBrowserWidget::valueIsEmpty()
{
    return false;
}

void MapBrowserWidget::setReadOnly(bool readOnly)
{
    m_internalReadOnly = readOnly;
}

bool MapBrowserWidget::isReadOnly() const
{
    return m_internalReadOnly;
}

void MapBrowserWidget::clear()
{
    setCenterLatitude(0.0);
    setCenterLongitude(0.0);
    setZoom(m_defaultZoom);
}

bool MapBrowserWidget::cursorAtEnd()
{
   return true;
}

bool MapBrowserWidget::cursorAtStart()
{
    return true;
}

void MapBrowserWidget::setInvalidState(const QString& )
{
    setEnabled(false);
    setReadOnly(true);
}

QVariant MapBrowserWidget::serializeData(qreal lat, qreal lon, int zoomLevel)
{
    return QString("%1;%2;%3").arg(lat).arg(lon).arg(zoomLevel);
}

void MapBrowserWidget::deserializeData(const QVariant& serialized)
{
    //qDebug() << "seting new data";
    QString serializedData = serialized.toString();
    //qDebug() << "serializedData:" << serializedData << ";" << serialized;
    QStringList dataList = serializedData.split(';');
    //qDebug() << "splited:" << dataList;
    if (dataList.length()>=3) {
        setAnimationsEnabled(false);
        setCenterLatitude(dataList[0].toDouble());
        setCenterLongitude(dataList[1].toDouble());
        zoomView(dataList[2].toInt());
        setAnimationsEnabled(true);
    }
}

void MapBrowserWidget::slotMapChanged()
{
    if(!m_slotMapChanged_enabled)
        return;
    signalValueChanged();
}

void MapBrowserWidget::resizeEvent(QResizeEvent *event)
{
    KexiUtils::BoolBlocker guard(m_slotMapChanged_enabled, false);
    Marble::MarbleWidget::resizeEvent(event);
}

