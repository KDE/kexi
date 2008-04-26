//
// C++ Implementation: krscriptsection
//
// Description:
//
//
// Author: Adam Pigg <adam@piggz.co.uk>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "krscriptsection.h"
#include "krscriptlabel.h"
#include "krscriptfield.h"
#include <kdebug.h>

namespace Scripting
{
	Section::Section ( KRSectionData* sec )
	{
		_section = sec;
	}


	Section::~Section()
	{
	}

	QColor Section::backgroundColor()
	{
		return _section->_bgColor->value().value<QColor>();
	}

	void   Section::setBackgroundColor ( const QColor &c)
	{
		kDebug() << c.name();
		_section->_bgColor->setValue(c);
	}

	qreal Section::height()
	{
		return _section->_height->value().toDouble();
	}

	void Section::setHeight ( qreal h)
	{
		_section->_height->setValue(h);
	}

	QString Section::name()
	{
		return _section->_name;
	}

	QObject* Section::objectByNumber(int i)
	{
		switch (_section->_objects[i]->type())
		{
			case KRObjectData::EntityLabel:
				return new Scripting::Label(_section->_objects[i]->toLabel());
				break;
			case KRObjectData::EntityField:
				return new Scripting::Field(_section->_objects[i]->toField());
				break;
			default:
				return new QObject();
		}
	
	}

	QObject* Section::objectByName(const QString& n)
	{
		for (int i = 0; i < _section->objects().count(); ++i)
		{
			if (_section->_objects[i]->entityName() == n)
			{
				return objectByNumber(i);
			}
		}
		return 0;
	}

}
