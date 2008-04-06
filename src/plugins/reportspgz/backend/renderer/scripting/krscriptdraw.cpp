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
#include "krscriptdraw.h"
#include <renderobjects.h>
#include <krpos.h>
#include <krsize.h>

KRScriptDraw::KRScriptDraw(QObject *parent)
 : QObject(parent)
{
	_curPage = 0;
}


KRScriptDraw::~KRScriptDraw()
{
}

void KRScriptDraw::setPage(OROPage *p)
{
	_curPage = p;
}

void KRScriptDraw::setOffset(QPointF off)
{
	_curOffset = off;
}

void KRScriptDraw::rectangle(qreal x, qreal y, qreal w, qreal h, const QString& lc, const QString& fc, qreal lw, int a)
{
	if (_curPage)
	{
		ORORect *r = new ORORect();
		KRPos p;
		KRSize s;
		
		p.setPointPos(QPointF(x, y));
		s.setPointSize(QSizeF(w, h));
		r->setRect(QRectF(p.toScene() + _curOffset, s.toScene()));
		
		QPen pen(QColor(lc), lw);
		QColor c(fc);
		c.setAlpha(a);
		QBrush bru(c);
		
		r->setBrush(bru);
		r->setPen(pen);
		_curPage->addPrimitive(r);
	}
}

void KRScriptDraw::ellipse(qreal x, qreal y, qreal w, qreal h, const QString& lc, const QString& fc, qreal lw, int a)
{
	if (_curPage)
	{
		OROEllipse *e = new OROEllipse();
		KRPos p;
		KRSize s;
		
		p.setPointPos(QPointF(x, y));
		s.setPointSize(QSizeF(w, h));
		e->setRect(QRectF(p.toScene() + _curOffset, s.toScene()));
		
		QPen pen(QColor(lc), lw);
		QColor c(fc);
		c.setAlpha(a);
		QBrush bru(c);
		
		e->setBrush(bru);
		e->setPen(pen);
		_curPage->addPrimitive(e);
	}
}

void KRScriptDraw::line(qreal x1, qreal y1, qreal x2, qreal y2, const QString& lc)
{
	if (_curPage)
	{
		OROLine *ln = new OROLine();
		KRPos s;
		KRPos e;
		
		s.setPointPos(QPointF(x1, y1));
		e.setPointPos(QPointF(x2, y2));
		
		ln->setStartPoint(s.toScene());
		ln->setEndPoint(e.toScene());
		_curPage->addPrimitive(ln);
	}
}
