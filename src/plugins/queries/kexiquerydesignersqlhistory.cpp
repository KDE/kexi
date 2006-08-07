/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004 Jaroslaw Staniek <js@iidea.pl>

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

#include <qpainter.h>
#include <qclipboard.h>
#include <qregexp.h>

#include <kpopupmenu.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kapplication.h>

#include "kexiquerydesignersqlhistory.h"

KexiQueryDesignerSQLHistory::KexiQueryDesignerSQLHistory(QWidget *parent, const char *name)
 : QScrollView(parent, name)
{
	viewport()->setPaletteBackgroundColor(white);

	m_selected = 0;
	m_history = new History();
	m_history->setAutoDelete(true);

	m_popup = new KPopupMenu(this);
	m_popup->insertItem(SmallIcon("editcopy"), i18n("Copy to Clipboard"), this, SLOT(slotToClipboard()));
}

KexiQueryDesignerSQLHistory::~KexiQueryDesignerSQLHistory()
{
}

void
KexiQueryDesignerSQLHistory::drawContents(QPainter *p, int cx, int cy, int cw, int ch)
{
	QRect clipping(cx, cy, cw, ch);

	int y = 0;
	for(HistoryEntry *it = m_history->first(); it; it = m_history->next())
	{
//		it->drawItem(p, visibleWidth());
		if(clipping.intersects(it->geometry(y, visibleWidth(), fontMetrics())))
		{
			p->saveWorldMatrix();
			p->translate(0, y);
			it->drawItem(p, visibleWidth(), colorGroup());
			p->restoreWorldMatrix();
		}
		y += it->geometry(y, visibleWidth(), fontMetrics()).height() + 5;
	}
}

void
KexiQueryDesignerSQLHistory::contentsMousePressEvent(QMouseEvent * e)
{
	int y = 0;
	HistoryEntry *popupHistory = 0;
	int pos;
	for(QPtrListIterator<HistoryEntry> it(*m_history); it.current(); ++it)
	{
		if(it.current()->isSelected())
		{
			//clear
			it.current()->setSelected(false, colorGroup());
			updateContents(it.current()->geometry(y, visibleWidth(), fontMetrics()));
		}

		if(it.current()->geometry(y, visibleWidth(), fontMetrics()).contains(e->pos()))
		{
			popupHistory = it.current();
			pos = y;
		}
		y += it.current()->geometry(y, visibleWidth(), fontMetrics()).height() + 5;
	}

	//now do update
	if (popupHistory) {
			if (m_selected && m_selected != popupHistory) {
				m_selected->setSelected(false, colorGroup());
				updateContents(m_selected->geometry(pos, visibleWidth(), fontMetrics()));
			}
			m_selected = popupHistory;
			m_selected->setSelected(true, colorGroup());
			updateContents(m_selected->geometry(pos, visibleWidth(), fontMetrics()));
			if(e->button() == RightButton) {
				m_popup->exec(e->globalPos());
			}
	}
}

void
KexiQueryDesignerSQLHistory::contentsMouseDoubleClickEvent(QMouseEvent * e)
{
	contentsMousePressEvent(e);
	if (m_selected)
		emit currentItemDoubleClicked();
}

void
KexiQueryDesignerSQLHistory::addEvent(const QString& q, bool s, const QString &error)
{
	HistoryEntry *he=m_history->last();
	if (he) {
		if (he->statement()==q) {
			he->updateTime(QTime::currentTime());
			repaint();
			return;
		}
	}
	addEntry(new HistoryEntry(s, QTime::currentTime(), q, error));
}

void
KexiQueryDesignerSQLHistory::addEntry(HistoryEntry *e)
{
	m_history->append(e);
//	m_history->prepend(e);

	int y = 0;
	for(HistoryEntry *it = m_history->first(); it; it = m_history->next())
	{
		y += it->geometry(y, visibleWidth(), fontMetrics()).height() + 5;
	}

	resizeContents(visibleWidth() - 1, y);
	if (m_selected) {
		m_selected->setSelected(false, colorGroup());
	}
	m_selected = e;
	m_selected->setSelected(true, colorGroup());
	ensureVisible(0,y+5);
	updateContents();
/*	ensureVisible(0, 0);
	if (m_selected) {
		m_selected->setSelected(false, colorGroup());
	}
	m_selected = e;
	m_selected->setSelected(true, colorGroup());
//	updateContents();
	updateContents(m_selected->geometry(0, visibleWidth(), fontMetrics()));*/
}

/*void
KexiQueryDesignerSQLHistory::contextMenu(const QPoint &pos, HistoryEntry *)
{
	KPopupMenu p(this);
	p.insertItem(SmallIcon("editcopy"), i18n("Copy to Clipboard"), this, SLOT(slotToClipboard()));
	

#ifndef KEXI_NO_UNFINISHED
	p.insertSeparator();
	p.insertItem(SmallIcon("edit"), i18n("Edit"), this, SLOT(slotEdit()));
	p.insertItem(SmallIcon("reload"), i18n("Requery"));
#endif

	p.exec(pos);
}*/

void
KexiQueryDesignerSQLHistory::slotToClipboard()
{
	if(!m_selected)
		return;

	QApplication::clipboard()->setText(m_selected->statement(), QClipboard::Clipboard);
}

void
KexiQueryDesignerSQLHistory::slotEdit()
{
	emit editRequested(m_selected->statement());
}

QString
KexiQueryDesignerSQLHistory::selectedStatement() const
{
	return m_selected ? m_selected->statement() : QString::null;
}

void
KexiQueryDesignerSQLHistory::setHistory(History *h)
{
	m_history = h;
	update();
}

void KexiQueryDesignerSQLHistory::clear()
{
	m_selected = 0;
	m_history->clear();
	updateContents();
}

KPopupMenu* KexiQueryDesignerSQLHistory::popupMenu() const
{
	return m_popup;
}

//==================================

HistoryEntry::HistoryEntry(bool succeed, const QTime &execTime, const QString &statement, /*int ,*/ const QString &err)
{
	m_succeed = succeed;
	m_execTime = execTime;
	m_statement = statement;
	m_error = err;
	m_selected = false;
	highlight(QColorGroup());
}

void
HistoryEntry::drawItem(QPainter *p, int width, const QColorGroup &cg)
{
	p->setPen(QColor(200, 200, 200));
	p->setBrush(QColor(200, 200, 200));
	p->drawRect(2, 2, 200, 20);
	p->setPen(QColor(0, 0, 0));

	if(m_succeed)
		p->drawPixmap(4, 4, SmallIcon("button_ok"));
	else
		p->drawPixmap(4, 4, SmallIcon("button_cancel"));

	p->drawText(22, 2, 180, 20, Qt::AlignLeft | Qt::AlignVCenter, m_execTime.toString());
	p->setPen(QColor(200, 200, 200));
	p->setBrush(QColor(255, 255, 255));
	m_formated->setWidth(width - 2);
	QRect content(2, 21, width - 2, m_formated->height());
//	QRect content = p->fontMetrics().boundingRect(2, 21, width - 2, 0, Qt::WordBreak | Qt::AlignLeft | Qt::AlignVCenter, m_statement);
//	QRect content(2, 21, width - 2, p->fontMetrics().height() + 4);
//	content = QRect(2, 21, width - 2, m_for.height());

	if(m_selected)
		p->setBrush(cg.highlight());

	p->drawRect(content);

	if(!m_selected)
		p->setPen(cg.text());
	else
		p->setPen(cg.highlightedText());

	content.setX(content.x() + 2);
	content.setWidth(content.width() - 2);
//	p->drawText(content, Qt::WordBreak | Qt::AlignLeft | Qt::AlignVCenter, m_statement);
	m_formated->draw(p, content.x(), content.y(), content, cg);
}

void
HistoryEntry::highlight(const QColorGroup &cg)
{
	QString statement;
	QString text;
	bool quote = false;
	bool dblquote = false;

	statement = m_statement;
	statement.replace("<", "&lt;");
	statement.replace(">", "&gt;");
	statement.replace("\r\n", "<br>"); //(js) first win32 specific pair
	statement.replace("\n", "<br>"); // now single \n
	statement.replace(" ", "&nbsp;");
	statement.replace("\t", "&nbsp;&nbsp;&nbsp;");

	// getting quoting...
	if(!m_selected)
	{
		for(int i=0; i < (int)statement.length(); i++)
		{
			QString beginTag;
			QString endTag;
			QChar curr = QChar(statement[i]);

			if(curr == "'" && !dblquote && QChar(statement[i-1]) != "\\")
			{
				if(!quote)
				{
					quote = true;
					beginTag += "<font color=\"#ff0000\">";
				}
				else
				{
					quote = false;
					endTag += "</font>";
				}
			}
			if(curr == "\"" && !quote && QChar(statement[i-1]) != "\\")
			{
				if(!dblquote)
				{
					dblquote = true;
					beginTag += "<font color=\"#ff0000\">";
				}
				else
				{
					dblquote = false;
					endTag += "</font>";
				}
			}
			if(QRegExp("[0-9]").exactMatch(QString(curr)) && !quote && !dblquote)
			{
				beginTag += "<font color=\"#0000ff\">";
				endTag += "</font>";
			}

			text += beginTag + curr + endTag;
		}
	}
	else
	{
		text = QString("<font color=\"%1\">%2").arg(cg.highlightedText().name()).arg(statement);
	}

	QRegExp keywords("\\b(SELECT|UPDATE|INSERT|DELETE|DROP|FROM|WHERE|AND|OR|NOT|NULL|JOIN|LEFT|RIGHT|ON|INTO|TABLE)\\b");
	keywords.setCaseSensitive(false);
	text = text.replace(keywords, "<b>\\1</b>");

	if(!m_error.isEmpty())
//		text += ("<br>"+i18n("Error: %1").arg(m_error));
//		text += QString("<br><font face=\"") + KGlobalSettings::generalFont().family() + QString("\" size=\"-1\">") + i18n("Error: %1").arg(m_error) + "</font>";
		text += QString("<br><font face=\"") + KGlobalSettings::generalFont().family() + QString("\">") + i18n("Error: %1").arg(m_error) + "</font>";

	kdDebug() << "HistoryEntry::highlight() text:" << text << endl;
//	m_formated = new QSimpleRichText(text, QFont("courier", 8));
	m_formated = new QSimpleRichText(text, KGlobalSettings::fixedFont());

}

void
HistoryEntry::setSelected(bool selected, const QColorGroup &cg)
{
	m_selected = selected;
	highlight(cg);
}

QRect
HistoryEntry::geometry(int y, int width, QFontMetrics f)
{
	Q_UNUSED( f );

//	int h = 21 + f.boundingRect(2, 21, width - 2, 0, Qt::WordBreak | Qt::AlignLeft | Qt::AlignVCenter, m_statement).height();
//	return QRect(0, y, width, h);
	m_formated->setWidth(width - 2);
	return QRect(0, y, width, m_formated->height() + 21);
}

void HistoryEntry::updateTime(const QTime &execTime) {
	m_execTime=execTime;
}

HistoryEntry::~HistoryEntry()
{
}

#include "kexiquerydesignersqlhistory.moc"
