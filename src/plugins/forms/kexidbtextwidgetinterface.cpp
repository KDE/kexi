/* This file is part of the KDE project
   Copyright (C) 2005 Jaroslaw Staniek <js@iidea.pl>

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

#include "kexidbtextwidgetinterface.h"
#include "kexiformdataiteminterface.h"
#include <kexidb/queryschema.h>
#include <kexiutils/utils.h>
#include <qframe.h>
#include <qpainter.h>

void KexiDBTextWidgetInterface::setColumnInfo(KexiDB::QueryColumnInfo* cinfo, QWidget *w)
{
	if (cinfo->field->isAutoIncrement()) {
		if (!m_autonumberDisplayParameters)
			m_autonumberDisplayParameters = new KexiDisplayUtils::DisplayParameters();
		KexiDisplayUtils::initDisplayForAutonumberSign(*m_autonumberDisplayParameters, w);
	}
}

void KexiDBTextWidgetInterface::paintEvent( QFrame *w, bool textIsEmpty, int alignment, bool hasFocus  )
{
	KexiFormDataItemInterface *dataItemIface = dynamic_cast<KexiFormDataItemInterface*>(w);
	if (dataItemIface && dataItemIface->columnInfo() && dataItemIface->columnInfo()->field->isAutoIncrement()
		&& m_autonumberDisplayParameters && dataItemIface->cursorAtNewRow() && textIsEmpty)
	{
		QPainter p(w);
		if (w->hasFocus()) {
			p.setPen(KexiUtils::blendedColors(m_autonumberDisplayParameters->textColor, w->palette().active().base(), 1, 3));
		}
		const int m = w->lineWidth()+w->midLineWidth();
		KexiDisplayUtils::drawAutonumberSign(*m_autonumberDisplayParameters, &p,
			2+m+w->margin(), m, w->width()-m*2 -2-2, w->height()-m*2 -2, alignment, hasFocus);
	}
}

void KexiDBTextWidgetInterface::event( QEvent * e, QWidget *w, bool textIsEmpty )
{
	if (e->type()==QEvent::FocusIn || e->type()==QEvent::FocusOut) {
		if (m_autonumberDisplayParameters && textIsEmpty)
			w->repaint();
	}
}
