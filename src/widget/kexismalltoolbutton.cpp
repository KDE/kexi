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

#include "kexismalltoolbutton.h"

#include <QStyle>
#include <QStyleOption>
#include <QPainter>

#include <kiconloader.h>
#include <kglobalsettings.h>

#include <core/kexi.h>

KexiSmallToolButton::KexiSmallToolButton(QWidget* parent, const QString& text,
	const QString& icon)
 : QToolButton(parent)
{
	init();
	update(text, KIcon(icon));
}

KexiSmallToolButton::KexiSmallToolButton(QWidget* parent, const QString& text,
	const QIcon& icon)
 : QToolButton(parent)
{
	init();
	update(text, icon);
}

KexiSmallToolButton::KexiSmallToolButton(QWidget* parent, KAction* action)
 : QToolButton(parent)
 , m_action(action)
{
	setText(action->objectName());
	init();
	connect(this, SIGNAL(clicked()), action, SLOT(activate()));
	connect(action, SIGNAL(enabled(bool)), this, SLOT(setEnabled(bool)));
	updateAction();
}

KexiSmallToolButton::~KexiSmallToolButton()
{
}

void KexiSmallToolButton::updateAction()
{
	if (!m_action)
		return;
	removeAction(m_action);
	addAction(m_action);
#ifdef __GNUC__
#warning TODO KexiSmallToolButton::updateAction() OK?
#endif
#if 0
	update(m_action->text(), m_action->icon());
	setShortcut(m_action->shortcut());
	setToolTip( m_action->toolTip());
	setWhatsThis( m_action->whatsThis());
#endif
}

void KexiSmallToolButton::init()
{
	setPaletteBackgroundColor(palette().active().background());
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	QFont f(KGlobalSettings::toolBarFont());
	f.setPixelSize(Kexi::smallFont().pixelSize());
	setFont(f);
	setAutoRaise(true);
}

void KexiSmallToolButton::update(const QString& text, const QIcon& icon, bool tipToo)
{
	int width = 0;
	if (text.isEmpty()) {
		width = 10;
		setToolButtonStyle(Qt::ToolButtonIconOnly);
	}
	else {
		width += QFontMetrics(font()).width(text+" ");
		setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		QToolButton::setText(text);
		if (tipToo)
			setToolTip(text);
	}
	if (!icon.isNull()) {
		width += IconSize(K3Icon::Small);
		QToolButton::setIcon(icon);
	}
	setFixedWidth( width );
}

void KexiSmallToolButton::setIcon( const QIcon& icon )
{
	update(text(), icon);
}

void KexiSmallToolButton::setIcon( const QString& icon )
{
	setIcon( KIcon(icon) );
}

void KexiSmallToolButton::setText( const QString& text )
{
	update(text, icon());
}

void KexiSmallToolButton::paintEvent(QPaintEvent *pe)
{
#ifdef __GNUC__
#warning TODO KexiSmallToolButton::drawButton() - painting OK?
#endif
	QToolButton::paintEvent(pe);
	QPainter painter(this);
	if (QToolButton::menu()) {
		QStyle::State arrowFlags = QStyle::State_None;
		QStyleOption option;
		option.initFrom(this);
		if (isDown())
			option.state |= QStyle::State_DownArrow;
		if (isEnabled())
			option.state |= QStyle::State_Enabled;
		style()->drawPrimitive(QStyle::PE_IndicatorButtonDropDown, &option, &painter, this);
	}
}

#include "kexismalltoolbutton.moc"
