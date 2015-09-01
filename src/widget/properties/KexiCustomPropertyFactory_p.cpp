/* This file is part of the KDE project
   Copyright (C) 2005 Jarosław Staniek <staniek@kde.org>

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

#include "KexiCustomPropertyFactory_p.h"

#include <KProperty>

#include <KDb>
#include <KDbIdentifierValidator>

#include <QDebug>

//! @todo
#if 0 //TODO
KexiImagePropertyEdit::KexiImagePropertyEdit(
    KProperty *property, QWidget *parent)
        : KPropertyPixmapEditor(property, parent)
        , m_id(0)
{
}

KexiImagePropertyEdit::~KexiImagePropertyEdit()
{
}

void KexiImagePropertyEdit::selectPixmap()
{
    QString fileName(KPropertyPixmapEditor::selectPixmapFileName());
    if (fileName.isEmpty())
        return;
    KexiBLOBBuffer::Handle h(KexiBLOBBuffer::self()->insertPixmap(QUrl(fileName)));
    setValue(int(/*! @todo unsafe*/h.id()));
#if 0 //will be reenabled for new image collection
    if (!m_manager->activeForm() || !property())
        return;

    ObjectTreeItem *item = m_manager->activeForm()->objectTree()->lookup(m_manager->activeForm()->selectedWidget()->name());
    QString name = item ? item->pixmapName(property()->name()) : "";
    PixmapCollectionChooser dialog(m_manager->activeForm()->pixmapCollection(), name, topLevelWidget());
    if (dialog.exec() == QDialog::Accepted) {
        setValue(dialog.pixmap(), true);
        item->setPixmapName(property()->name(), dialog.pixmapName());
    }
#endif
}

QVariant KexiImagePropertyEdit::value() const
{
    return int(/*! @todo unsafe*/m_id);
}

void KexiImagePropertyEdit::setValue(const QVariant &value, bool emitChange)
{
    m_id = value.toInt();
    PixmapEdit::setValue(KexiBLOBBuffer::self()->objectForId(m_id).pixmap(), emitChange);
}

void KexiImagePropertyEdit::drawViewer(QPainter *p, cg, const QRect &r,
                                       const QVariant &value)
{
    KexiBLOBBuffer::Handle h(KexiBLOBBuffer::self()->objectForId(value.toInt()));
    PixmapEdit::drawViewer(p, cg, r, h.pixmap());
}
#endif

//----------------------------------------------------------------

KexiIdentifierPropertyEditor::KexiIdentifierPropertyEditor(QWidget *parent)
        : KPropertyStringEditor(parent)
{
    KDbIdentifierValidator *val = new KDbIdentifierValidator(this);
    setValidator(val);
    val->setObjectName("KexiIdentifierPropertyEdit Validator");
}

KexiIdentifierPropertyEditor::~KexiIdentifierPropertyEditor()
{
}

void KexiIdentifierPropertyEditor::setValue(const QString &value)
{
    if (value.isEmpty()) {
        qWarning() << "Value cannot be empty. This call has no effect.";
        return;
    }
    const QString identifier(KDb::stringToIdentifier(value));
    if (identifier != value)
        qDebug() << QString("String \"%1\" converted to identifier \"%2\".").arg(value).arg(identifier);
    KPropertyStringEditor::setValue(identifier);
}

