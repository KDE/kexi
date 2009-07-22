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
#ifndef KROBJECTDATA_H
#define KROBJECTDATA_H
#include <koproperty/Property.h>
#include "krpos.h"

class KRLineData;
class KRLabelData;
class KRFieldData;
class KRTextData;
class KRBarcodeData;
class KRImageData;
class KRChartData;
class KRShapeData;
class KRCheckData;

namespace KoProperty
{
class Set;
class Property;
}
/**
 @author
*/
class KRObjectData
{
public:
    enum EntityTypes {
        EntityNone = 0,
        EntityLine  = 65537,
        EntityLabel = 65550,
        EntityField = 65551,
        EntityText  = 65552,
        EntityBarcode = 65553,
        EntityImage = 65554,
        EntityChart = 65555,
        EntityShape = 65556,
        EntityCheck = 65557
    };

    KRObjectData();
    virtual ~KRObjectData();

    virtual int type() const = 0;
    virtual KRLineData * toLine();
    virtual KRLabelData * toLabel();
    virtual KRFieldData * toField();
    virtual KRTextData * toText();
    virtual KRBarcodeData * toBarcode();
    virtual KRImageData * toImage();
    virtual KRChartData * toChart();
    virtual KRShapeData * toShape();
    virtual KRCheckData * toCheck();

    KoProperty::Set* properties() {
        return m_set;
    }
    virtual void createProperties() = 0;

    qreal Z;
    KRPos position() {
        return m_pos;
    }

    void setEntityName(const QString& n) {
        m_name->setValue(n);
    }
    QString entityName() {
        return m_name->value().toString();
    }
protected:
    KoProperty::Set *m_set;
    KoProperty::Property *m_name;
    KRPos m_pos;

    QString m_oldName;

};

#endif
