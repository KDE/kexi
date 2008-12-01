/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2007 by OpenMFG, LLC
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


#ifndef __REPORTENTITYFIELD_H__
#define __REPORTENTITYFIELD_H__

#include <QGraphicsRectItem>
#include <koproperty/Property.h>
#include <koproperty/Set.h>
#include <krfielddata.h>
#include "reportrectentity.h"
//
// ReportEntityField
//
class ReportEntityField : public QObject, public ReportRectEntity, public KRFieldData
{
    Q_OBJECT
public:
    //Used when creating new basic field
    ReportEntityField(ReportDesigner *, QGraphicsScene * scene);
    //Used when loading from file
    ReportEntityField(QDomNode & element, ReportDesigner *, QGraphicsScene * scene);
    virtual ~ReportEntityField();

    virtual void buildXML(QDomDocument & doc, QDomElement & parent);

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget *widget = 0);
    virtual ReportEntityField* clone();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);

private:
    void init(QGraphicsScene*);
    QRect getTextRect();

private slots:
    void propertyChanged(KoProperty::Set &, KoProperty::Property &);
};

#endif
