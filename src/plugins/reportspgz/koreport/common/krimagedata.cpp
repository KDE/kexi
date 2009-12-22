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
#include "krimagedata.h"
#include <koproperty/Property.h>
#include <koproperty/Set.h>
#include <KoGlobal.h>
#include <kdebug.h>
#include <klocalizedstring.h>
#include <kglobalsettings.h>
#include <QBuffer>
#include <kcodecs.h>

KRImageData::KRImageData(QDomNode & element)
{
    createProperties();
    QDomNodeList nl = element.childNodes();
    QString n;
    QDomNode node;

    m_name->setValue(element.toElement().attribute("report:name"));
    m_controlSource->setValue(element.toElement().attribute("report:control-source"));
    m_resizeMode->setValue(element.toElement().attribute("report:resize-mode", "stretch"));
    Z = element.toElement().attribute("report:z-index").toDouble();

    for (int i = 0; i < nl.count(); i++) {
        node = nl.item(i);
        n = node.nodeName();

        if (n == "report:rect") {
            parseReportRect(node.toElement(), &m_pos, &m_size);
        } else if (n == "report:inline-image-data") {

            setInlineImageData(node.firstChild().nodeValue().toLatin1());
        } else {
            kDebug() << "while parsing image element encountered unknow element: " << n;
        }
    }

}

bool KRImageData::isInline()
{
    return !(inlineImageData().isEmpty());
}

QString KRImageData::inlineImageData()
{
    QPixmap pixmap = m_staticImage->value().value<QPixmap>();
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(IO_ReadWrite);
    pixmap.save(&buffer, "PNG");   // writes pixmap into ba in PNG format,
    //TODO should i remember the format used, or save as PNG as its lossless?

    QByteArray imageEncoded(KCodecs::base64Encode(buffer.buffer(), true));
    return imageEncoded;
}

void KRImageData::setInlineImageData(QByteArray dat, const QString &fn)
{


    if (!fn.isEmpty()) {
        QPixmap pix(fn);
        if (!pix.isNull())
            m_staticImage->setValue(pix);
        else {
            QPixmap blank(1, 1);
            blank.fill();
            m_staticImage->setValue(blank);
        }
    } else {
        const QByteArray binaryStream(KCodecs::base64Decode(dat));
        const QPixmap pix(QPixmap::fromImage(QImage::fromData(binaryStream), Qt::ColorOnly));
        m_staticImage->setValue(pix);
    }

}

QString KRImageData::mode()
{
    return m_resizeMode->value().toString();
}
void KRImageData::setMode(QString m)
{
    if (mode() != m) {
        m_resizeMode->setValue(m);
    }
}

void KRImageData::createProperties()
{
    m_set = new KoProperty::Set(0, "Image");

    m_controlSource = new KoProperty::Property("control-source", QStringList(), QStringList(), "", "Control Source");

    QStringList keys, strings;
    keys << "clip" << "stretch";
    strings << i18n("Clip") << i18n("Stretch");
    m_resizeMode = new KoProperty::Property("resize-mode", keys, strings, "clip", "Resize Mode");

    m_staticImage = new KoProperty::Property("static-image", QPixmap(), "Static Image", "Static Image");

    m_set->addProperty(m_name);
    m_set->addProperty(m_controlSource);
    m_set->addProperty(m_resizeMode);
    m_set->addProperty(m_pos.property());
    m_set->addProperty(m_size.property());
    m_set->addProperty(m_staticImage);
}


void KRImageData::setColumn(QString c)
{
    m_controlSource->setValue(c);
}

QString KRImageData::column()
{
    return m_controlSource->value().toString();
}

int KRImageData::type() const
{
    return RTTI;
}
int KRImageData::RTTI = KRObjectData::EntityImage;
KRImageData* KRImageData::toImage()
{
    return this;
}
