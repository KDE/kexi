/***************************************************************************
 *   Copyright (C) 2003 by Lucijan Busch                                   *
 *   lucijan@kde.org                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 ***************************************************************************/

#include <qlabel.h>
#include <qlineedit.h>

#include <kiconloader.h>
#include <kgenericfactory.h>
#include <kdebug.h>

#include "stdwidgetfactory.h"

StdWidgetFactory::StdWidgetFactory(QObject *parent, const char *name, const QStringList &)
 : KFormDesigner::WidgetFactory(parent, name)
{
	KFormDesigner::Widget *wLabel = new KFormDesigner::Widget(this);
	wLabel->setPixmap(SmallIcon("label"));
	wLabel->setClassName("QLabel");
	wLabel->setName("Text Label");
	m_classes.append(wLabel);

	KFormDesigner::Widget *wLineEdit = new KFormDesigner::Widget(this);
	wLineEdit->setPixmap(SmallIcon("lineedit"));
	wLineEdit->setClassName("QLineEdit");
	wLineEdit->setName("Line Edit");
	m_classes.append(wLineEdit);
}

QString
StdWidgetFactory::name()
{
	return("stdwidgets");
}

KFormDesigner::WidgetList
StdWidgetFactory::classes()
{
	return m_classes;
}

QWidget*
StdWidgetFactory::create(const QString &c, QWidget *p, const char *n, KFormDesigner::Container *container)
{
	kdDebug() << "StdWidgetFactory::create() " << this << endl;

	if(c == "QLabel")
	{
		QWidget *w = new QLabel("Label", p, "textlabel");
		w->installEventFilter(container);

		return w;
	}
	else if(c == "QLineEdit")
	{
		QWidget *w = new QLineEdit(p, n);
		w->installEventFilter(container);

		return w;
	}

	return 0;
}

StdWidgetFactory::~StdWidgetFactory()
{
}

K_EXPORT_COMPONENT_FACTORY(stdwidgets, KGenericFactory<StdWidgetFactory>)

#include "stdwidgetfactory.moc"

