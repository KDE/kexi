/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005-2007 Jaroslaw Staniek <js@iidea.pl>

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

#include <kdebug.h>

#include <qmetaobject.h>
#include <qdom.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qcursor.h>
#include <qbuffer.h>
#include <qimage.h>
#include <qlayout.h>
#include <qobjectlist.h>
#include <qdatetime.h>
#include <qlabel.h>
#include <qpainter.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kcommand.h>
#include <kaccelmanager.h>

#include "form.h"
#include "container.h"
#include "objecttree.h"
#include "formmanager.h"
#include "widgetlibrary.h"
#include "spring.h"
#include "pixmapcollection.h"
#include "events.h"
#include "utils.h"
#include "kexiflowlayout.h"
#include "widgetwithsubpropertiesinterface.h"
#include "formIO.h"

/// A blank widget used when the class name is not supported
CustomWidget::CustomWidget(const QCString &className, QWidget *parent, const char *name)
: QWidget(parent, name), m_className(className)
{
	setBackgroundMode(Qt::PaletteDark);
}

CustomWidget::~CustomWidget()
{
}

void
CustomWidget::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setPen(palette().active().text());
	QRect r(rect());
	r.setX(r.x()+2);
	p.drawText(r, Qt::AlignTop, m_className);
}

using namespace KFormDesigner;

QDict<QLabel> *FormIO::m_buddies = 0;
ObjectTreeItem *FormIO::m_currentItem = 0;
Form *FormIO::m_currentForm = 0;
bool FormIO::m_savePixmapsInline = false;

// FormIO itself

KFORMEDITOR_EXPORT uint KFormDesigner::version()
{
	return KFORMDESIGNER_VERSION;
}

/////////////////////////////////////////////////////////////////////////////
///////////// Saving/loading functions //////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

FormIO::FormIO()
{
}

FormIO::~FormIO()
{
}

bool
FormIO::saveFormToFile(Form *form, const QString &filename)
{
	QString m_filename;
	if(!form->filename().isNull() && filename.isNull())
		m_filename = form->filename();

	if(filename.isNull())
	{
		m_filename = KFileDialog::getSaveFileName(QString::null, i18n("*.ui|Qt Designer UI Files"));
		if(m_filename.isNull())
			return false;
	}
	else
		m_filename = filename;
	form->setFilename(m_filename);

	QDomDocument domDoc;
	if (!saveFormToDom(form, domDoc))
		return false;

	QFile file(m_filename);
	if (!file.open(IO_WriteOnly))
		return false;

	QTextStream stream(&file);
	stream << domDoc.toString(3) << endl;
	file.close();

	return true;
}

bool
FormIO::saveFormToByteArray(Form *form, QByteArray &dest)
{
	QDomDocument domDoc;
	if (!saveFormToDom(form, domDoc))
		return false;
	dest = domDoc.toCString();
	return true;
}

bool
FormIO::saveFormToString(Form *form, QString &dest, int indent)
{
	QDomDocument domDoc;
	if (!saveFormToDom(form, domDoc))
		return false;
	dest = domDoc.toString(indent);
	return true;
}

bool
FormIO::saveFormToDom(Form *form, QDomDocument &domDoc)
{
	m_currentForm = form;

	domDoc = QDomDocument("UI");
	QDomElement uiElement = domDoc.createElement("UI");
	domDoc.appendChild(uiElement);
	uiElement.setAttribute("version", "3.1");
	uiElement.setAttribute("stdsetdef", 1);

	//update format version information
	form->headerProperties()->insert("version", QString::number(form->formatVersion()));
	//custom properties
	QDomElement headerPropertiesEl = domDoc.createElement("kfd:customHeader");
	for (QMapConstIterator<QCString,QString> it=form->headerProperties()->constBegin(); it!=form->headerProperties()->constEnd(); ++it) {
		headerPropertiesEl.setAttribute(it.key(), it.data());
	}
	uiElement.appendChild(headerPropertiesEl);

	/// We save the savePixmapsInline property in the Form
	QDomElement inlinePix = domDoc.createElement("pixmapinproject");
	uiElement.appendChild(inlinePix);

	// We create the top class element
	QDomElement baseClass = domDoc.createElement("class");
	uiElement.appendChild(baseClass);
	QDomText baseClassV = domDoc.createTextNode("QWidget");
	baseClass.appendChild(baseClassV);

	// Save the toplevel widgets, and so the whole Form
	saveWidget(form->objectTree(), uiElement, domDoc);

	// We then save the layoutdefaults element
	QDomElement layoutDefaults = domDoc.createElement("layoutDefaults");
	layoutDefaults.setAttribute("spacing", QString::number(form->defaultSpacing()));
	layoutDefaults.setAttribute("margin", QString::number(form->defaultMargin()));
	uiElement.appendChild(layoutDefaults);

	/// Save tab Stops
	if(form->autoTabStops())
		form->autoAssignTabStops();
	QDomElement tabStops = domDoc.createElement("tabstops");
	uiElement.appendChild(tabStops);
	for(ObjectTreeListIterator it( form->tabStopsIterator() ); it.current(); ++it)
	{
		QDomElement tabstop = domDoc.createElement("tabstop");
		tabStops.appendChild(tabstop);
		QDomText tabStopText = domDoc.createTextNode(it.current()->name());
		tabstop.appendChild(tabStopText);
	}

	// Save the Form 's PixmapCollection
	form->pixmapCollection()->save(uiElement);
	// Save the Form connections
	form->connectionBuffer()->save(uiElement);

	form->commandHistory()->documentSaved();

	m_currentForm = 0;
	m_currentItem = 0;
	//m_currentWidget = 0;

	return true;
}

bool
FormIO::loadFormFromByteArray(Form *form, QWidget *container, QByteArray &src, bool preview)
{
	QString errMsg;
	int errLine;
	int errCol;

	QDomDocument inBuf;
	bool parsed = inBuf.setContent(src, false, &errMsg, &errLine, &errCol);

	if(!parsed)
	{
		kdDebug() << "WidgetWatcher::load(): " << errMsg << endl;
		kdDebug() << "WidgetWatcher::load(): line: " << errLine << " col: " << errCol << endl;
		return false;
	}

	if (!loadFormFromDom(form, container, inBuf))
		return false;
	if(preview)
		form->setDesignMode(false);
	return true;
}

bool
FormIO::loadFormFromString(Form *form, QWidget *container, QString &src, bool preview)
{
	QString errMsg;
	int errLine;
	int errCol;

#ifdef KEXI_DEBUG_GUI
	form->m_recentlyLoadedUICode = src;
#endif

	QDomDocument inBuf;
	bool parsed = inBuf.setContent(src, false, &errMsg, &errLine, &errCol);

	if(!parsed)
	{
		kdDebug() << "WidgetWatcher::load(): " << errMsg << endl;
		kdDebug() << "WidgetWatcher::load(): line: " << errLine << " col: " << errCol << endl;
		return false;
	}

	if (!loadFormFromDom(form, container, inBuf))
		return false;
	if(preview)
		form->setDesignMode(false);
	return true;
}

bool
FormIO::loadFormFromFile(Form *form, QWidget *container, const QString &filename)
{
	QString errMsg;
	int errLine;
	int errCol;
	QString m_filename;

	if(filename.isNull())
	{
		m_filename = KFileDialog::getOpenFileName(QString::null, i18n("*.ui|Qt Designer UI Files"));
		if(m_filename.isNull())
			return false;
	}
	else
		m_filename = filename;

	QFile file(m_filename);
	if(!file.open(IO_ReadOnly))
	{
		kdDebug() << "Cannot open the file " << filename << endl;
		return false;
	}
	QTextStream stream(&file);
	QString text = stream.read();

	QDomDocument inBuf;
	bool parsed = inBuf.setContent(text, false, &errMsg, &errLine, &errCol);

	if(!parsed)
	{
		kdDebug() << "WidgetWatcher::load(): " << errMsg << endl;
		kdDebug() << "WidgetWatcher::load(): line: " << errLine << " col: " << errCol << endl;
		return false;
	}

	return loadFormFromDom(form, container, inBuf);
}

bool
FormIO::loadFormFromDom(Form *form, QWidget *container, QDomDocument &inBuf)
{
	m_currentForm = form;

	QDomElement ui = inBuf.namedItem("UI").toElement();

	//custom properties
	form->headerProperties()->clear();
	QDomElement headerPropertiesEl = ui.namedItem("kfd:customHeader").toElement();
	QDomAttr attr = headerPropertiesEl.firstChild().toAttr();
	while (!attr.isNull() && attr.isAttr()) {
		form->headerProperties()->insert(attr.name().latin1(), attr.value());
		attr = attr.nextSibling().toAttr();
	}
	//update format version information
	uint ver = 1; //the default
	if (form->headerProperties()->contains("version")) {
		bool ok;
		uint v = (*form->headerProperties())["version"].toUInt(&ok);
		if (ok)
			ver = v;
	}
	kdDebug() << "FormIO::loadFormFromDom(): original format version: " << ver << endl;
	form->setOriginalFormatVersion( ver );
	if (ver < KFormDesigner::version()) {
//! @todo We can either 1) convert from old format and later save in a new one or 2) keep old format.
//!     To do this we may need to look at the original format version number.
		kdDebug() << "FormIO::loadFormFromDom(): original format is older than current: " << KFormDesigner::version() << endl;
		form->setFormatVersion( KFormDesigner::version() );
	}
	else
		form->setFormatVersion( ver );

	if (ver > KFormDesigner::version()) {
//! @todo display information about too new format and that "some information will not be available".
		kdDebug() << "FormIO::loadFormFromDom(): original format is newer than current: " << KFormDesigner::version() << endl;
	}

	// Load the pixmap collection
	m_savePixmapsInline = ( (ui.namedItem("pixmapinproject").isNull()) || (!ui.namedItem("images").isNull()) );
	form->pixmapCollection()->load(ui.namedItem("collection"));

	QDomElement element = ui.namedItem("widget").toElement();
	createToplevelWidget(form, container, element);

	// Loading the tabstops
	QDomElement tabStops = ui.namedItem("tabstops").toElement();
//	if(tabStops.isNull())
//		return 1;
	if(!tabStops.isNull()) {
		int i = 0;
		uint itemsNotFound = 0;
		for(QDomNode n = tabStops.firstChild(); !n.isNull(); n = n.nextSibling(), i++)
		{
			QString name = n.toElement().text();
			ObjectTreeItem *item = form->objectTree()->lookup(name);
			if(!item)
			{
				kdDebug() << "FormIO::loadFormFromDom ERROR : no ObjectTreeItem " << endl;
				continue;
			}
			const int index = form->tabStops()->findRef(item);
			/* Compute a real destination index: "a number of not found items so far". */
			const int realIndex = i - itemsNotFound;
			if((index != -1) && (index != realIndex)) // the widget is not in the same place, so we move it
			{
				form->tabStops()->remove(item);
				form->tabStops()->insert(realIndex, item);
			}
			if(index == -1) {
				itemsNotFound++;
				kdDebug() << "FormIO: item '" << name << "' not in list" << endl;
			}
		}
	}

	// Load the form connections
	form->connectionBuffer()->load(ui.namedItem("connections"));

	m_currentForm = 0;
	m_currentItem = 0;

	return true;
}

/////////////////////////////////////////////////////////////////////////////
///////////// Functions to save/load properties /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
FormIO::savePropertyValue(QDomElement &parentNode, QDomDocument &parent, const char *name, 
	const QVariant &value, QWidget *w, WidgetLibrary *lib)
{
	// Widget specific properties and attributes ///////////////
//	kdDebug() << "FormIO::savePropertyValue()  Saving the property: " << name << endl;
	WidgetWithSubpropertiesInterface* subpropIface = dynamic_cast<WidgetWithSubpropertiesInterface*>(w);
	QWidget *subwidget = w;
	bool addSubwidgetFlag = false;
	int propertyId = w->metaObject()->findProperty(name, true);
	if (propertyId == -1 && subpropIface && subpropIface->subwidget()) { // try property from subwidget
		subwidget = subpropIface->subwidget();
		propertyId = subpropIface->subwidget()->metaObject()->findProperty(name, true);
		addSubwidgetFlag = true;
	}
	if(propertyId == -1)
	{
		kdDebug() << "FormIO::savePropertyValue()  The object doesn't have this property. Let's try the WidgetLibrary." << endl;
		if(lib)
			lib->saveSpecialProperty(w->className(), name, value, w, parentNode, parent);
		return;
	}

	const QMetaProperty *meta = subwidget->metaObject()->property(propertyId, true);
	if (!meta->stored( subwidget )) //not storable
		return;
	QDomElement propertyE = parent.createElement("property");
	propertyE.setAttribute("name", name);
	if (addSubwidgetFlag)
		propertyE.setAttribute("subwidget", "true");

	if(meta && meta->isEnumType())
	{
		// this property is enum or set type
		QDomElement type;
		QDomText valueE;

		if(meta->isSetType())
		{
			QStringList list = QStringList::fromStrList(meta->valueToKeys(value.toInt()));
			type = parent.createElement("set");
			valueE = parent.createTextNode(list.join("|"));
			type.appendChild(valueE);
		}
		else
		{
			QString s = meta->valueToKey(value.toInt());
			type = parent.createElement("enum");
			valueE = parent.createTextNode(s);
			type.appendChild(valueE);
		}
		propertyE.appendChild(type);
		parentNode.appendChild(propertyE);
		return;
	}

	if(value.type() == QVariant::Pixmap) {
		QDomText valueE;
		QDomElement type = parent.createElement("pixmap");
		QCString property = propertyE.attribute("name").latin1();
//todo		QCString pixmapName = m_currentItem->widget()->property("pixmapName").toCString();
		if(m_savePixmapsInline /* (js)too risky: || m_currentItem->pixmapName(property).isNull() */)
			valueE = parent.createTextNode(saveImage(parent, value.toPixmap()));
		else
			valueE = parent.createTextNode(m_currentItem->pixmapName(property));
		type.appendChild(valueE);
		propertyE.appendChild(type);
		parentNode.appendChild(propertyE);
		return;
	}

	// Saving a "normal" property
	writeVariant(parent, propertyE, value);
	parentNode.appendChild(propertyE);
}

void
FormIO::writeVariant(QDomDocument &parent, QDomElement &parentNode, QVariant value)
{
	QDomElement type;
	QDomText valueE;

	switch(value.type())
	{
		case QVariant::String:
		{
			type = parent.createElement("string");
			valueE = parent.createTextNode(value.toString());
			type.appendChild(valueE);
			break;
		}
		case QVariant::CString:
		{
			type = parent.createElement("cstring");
			valueE = parent.createTextNode(value.toString());
			type.appendChild(valueE);
			break;
		}
		case QVariant::Rect:
		{
			type = parent.createElement("rect");
			QDomElement x = parent.createElement("x");
			QDomElement y = parent.createElement("y");
			QDomElement w = parent.createElement("width");
			QDomElement h = parent.createElement("height");
			QDomText valueX = parent.createTextNode(QString::number(value.toRect().x()));
			QDomText valueY = parent.createTextNode(QString::number(value.toRect().y()));
			QDomText valueW = parent.createTextNode(QString::number(value.toRect().width()));
			QDomText valueH = parent.createTextNode(QString::number(value.toRect().height()));

			x.appendChild(valueX);
			y.appendChild(valueY);
			w.appendChild(valueW);
			h.appendChild(valueH);

			type.appendChild(x);
			type.appendChild(y);
			type.appendChild(w);
			type.appendChild(h);
			break;
		}
		case QVariant::Color:
		{
			type = parent.createElement("color");
			QDomElement r = parent.createElement("red");
			QDomElement g = parent.createElement("green");
			QDomElement b = parent.createElement("blue");
			QDomText valueR = parent.createTextNode(QString::number(value.toColor().red()));
			QDomText valueG = parent.createTextNode(QString::number(value.toColor().green()));
			QDomText valueB = parent.createTextNode(QString::number(value.toColor().blue()));

			r.appendChild(valueR);
			g.appendChild(valueG);
			b.appendChild(valueB);

			type.appendChild(r);
			type.appendChild(g);
			type.appendChild(b);
			break;
		}
		case QVariant::Bool:
		{
			type = parent.createElement("bool");
			//valueE = parent.createTextNode(QString::number(value.toBool()));
			valueE = parent.createTextNode(value.toBool() ? "true" : "false");
			type.appendChild(valueE);
			break;
		}
		case QVariant::Int:
		case QVariant::UInt:
		{
			type = parent.createElement("number");
			valueE = parent.createTextNode(QString::number(value.toInt()));
			type.appendChild(valueE);
			break;
		}
		case QVariant::Size:
		{
			type = parent.createElement("size");
			QDomElement w = parent.createElement("width");
			QDomElement h = parent.createElement("height");
			QDomText valueW = parent.createTextNode(QString::number(value.toSize().width()));
			QDomText valueH = parent.createTextNode(QString::number(value.toSize().height()));

			w.appendChild(valueW);
			h.appendChild(valueH);

			type.appendChild(w);
			type.appendChild(h);
			break;
		}
		case QVariant::Point:
		{
			type = parent.createElement("point");
			QDomElement x = parent.createElement("x");
			QDomElement y = parent.createElement("y");
			QDomText valueX = parent.createTextNode(QString::number(value.toPoint().x()));
			QDomText valueY = parent.createTextNode(QString::number(value.toPoint().y()));

			x.appendChild(valueX);
			y.appendChild(valueY);

			type.appendChild(x);
			type.appendChild(y);
			break;
		}
		case QVariant::Font:
		{
			type = parent.createElement("font");
			QDomElement f = parent.createElement("family");
			QDomElement p = parent.createElement("pointsize");
			QDomElement w = parent.createElement("weight");
			QDomElement b = parent.createElement("bold");
			QDomElement i = parent.createElement("italic");
			QDomElement u = parent.createElement("underline");
			QDomElement s = parent.createElement("strikeout");
			QDomText valueF = parent.createTextNode(value.toFont().family());
			QDomText valueP = parent.createTextNode(QString::number(value.toFont().pointSize()));
			QDomText valueW = parent.createTextNode(QString::number(value.toFont().weight()));
			QDomText valueB = parent.createTextNode(QString::number(value.toFont().bold()));
			QDomText valueI = parent.createTextNode(QString::number(value.toFont().italic()));
			QDomText valueU = parent.createTextNode(QString::number(value.toFont().underline()));
			QDomText valueS = parent.createTextNode(QString::number(value.toFont().strikeOut()));

			f.appendChild(valueF);
			p.appendChild(valueP);
			w.appendChild(valueW);
			b.appendChild(valueB);
			i.appendChild(valueI);
			u.appendChild(valueU);
			s.appendChild(valueS);

			type.appendChild(f);
			type.appendChild(p);
			type.appendChild(w);
			type.appendChild(b);
			type.appendChild(i);
			type.appendChild(u);
			type.appendChild(s);
			break;
		}
		case QVariant::Cursor:
		{
			type = parent.createElement("cursor");
			valueE = parent.createTextNode(QString::number(value.toCursor().shape()));
			type.appendChild(valueE);
			break;
		}
		case QVariant::SizePolicy:
		{
			type = parent.createElement("sizepolicy");
			QDomElement h = parent.createElement("hsizetype");
			QDomElement v = parent.createElement("vsizetype");
			QDomElement hs = parent.createElement("horstretch");
			QDomElement vs = parent.createElement("verstretch");
			QDomText valueH = parent.createTextNode(QString::number(value.toSizePolicy().horData()));
			QDomText valueV = parent.createTextNode(QString::number(value.toSizePolicy().verData()));
			QDomText valueHS = parent.createTextNode(QString::number(value.toSizePolicy().horStretch()));
			QDomText valueVS = parent.createTextNode(QString::number(value.toSizePolicy().verStretch()));

			h.appendChild(valueH);
			v.appendChild(valueV);
			hs.appendChild(valueHS);
			vs.appendChild(valueVS);

			type.appendChild(h);
			type.appendChild(v);
			type.appendChild(hs);
			type.appendChild(vs);
			break;
		}
		case QVariant::Time:
		{
			type = parent.createElement("time");
			QDomElement h = parent.createElement("hour");
			QDomElement m = parent.createElement("minute");
			QDomElement s = parent.createElement("second");
			QDomText valueH = parent.createTextNode(QString::number(value.toTime().hour()));
			QDomText valueM = parent.createTextNode(QString::number(value.toTime().minute()));
			QDomText valueS = parent.createTextNode(QString::number(value.toTime().second()));

			h.appendChild(valueH);
			m.appendChild(valueM);
			s.appendChild(valueS);

			type.appendChild(h);
			type.appendChild(m);
			type.appendChild(s);
			break;
		}
		case QVariant::Date:
		{
			type = parent.createElement("date");
			QDomElement y = parent.createElement("year");
			QDomElement m = parent.createElement("month");
			QDomElement d = parent.createElement("day");
			QDomText valueY = parent.createTextNode(QString::number(value.toDate().year()));
			QDomText valueM = parent.createTextNode(QString::number(value.toDate().month()));
			QDomText valueD = parent.createTextNode(QString::number(value.toDate().day()));

			y.appendChild(valueY);
			m.appendChild(valueM);
			d.appendChild(valueD);

			type.appendChild(y);
			type.appendChild(m);
			type.appendChild(d);
			break;
		}
		case QVariant::DateTime:
		{
			type = parent.createElement("datetime");
			QDomElement h = parent.createElement("hour");
			QDomElement m = parent.createElement("minute");
			QDomElement s = parent.createElement("second");
			QDomElement y = parent.createElement("year");
			QDomElement mo = parent.createElement("month");
			QDomElement d = parent.createElement("day");
			QDomText valueH = parent.createTextNode(QString::number(value.toDateTime().time().hour()));
			QDomText valueM = parent.createTextNode(QString::number(value.toDateTime().time().minute()));
			QDomText valueS = parent.createTextNode(QString::number(value.toDateTime().time().second()));
			QDomText valueY = parent.createTextNode(QString::number(value.toDateTime().date().year()));
			QDomText valueMo = parent.createTextNode(QString::number(value.toDateTime().date().month()));
			QDomText valueD = parent.createTextNode(QString::number(value.toDateTime().date().day()));

			h.appendChild(valueH);
			m.appendChild(valueM);
			s.appendChild(valueS);
			y.appendChild(valueY);
			mo.appendChild(valueMo);
			d.appendChild(valueD);

			type.appendChild(h);
			type.appendChild(m);
			type.appendChild(s);
			type.appendChild(y);
			type.appendChild(mo);
			type.appendChild(d);
			break;
		}
		default:
			break;
	}

	parentNode.appendChild(type);
}

void
FormIO::savePropertyElement(QDomElement &parentNode, QDomDocument &domDoc, const QString &tagName, const QString &property, const QVariant &value)
{
	QDomElement propertyE = domDoc.createElement(tagName);
	propertyE.setAttribute("name", property);
	writeVariant(domDoc, propertyE, value);
	parentNode.appendChild(propertyE);
}

QVariant
FormIO::readPropertyValue(QDomNode node, QObject *obj, const QString &name)
{
	QDomElement tag = node.toElement();
	QString text = tag.text();
	QString type = tag.tagName();

	if(type == "string" || type == "cstring")
		return text;
	else if(type == "rect")
	{
		QDomElement x = node.namedItem("x").toElement();
		QDomElement y = node.namedItem("y").toElement();
		QDomElement w = node.namedItem("width").toElement();
		QDomElement h = node.namedItem("height").toElement();

		int rx = x.text().toInt();
		int ry = y.text().toInt();
		int rw = w.text().toInt();
		int rh = h.text().toInt();

		return QRect(rx, ry, rw, rh);
	}
	else if(type == "color")
	{
		QDomElement r = node.namedItem("red").toElement();
		QDomElement g = node.namedItem("green").toElement();
		QDomElement b = node.namedItem("blue").toElement();

		int red = r.text().toInt();
		int green = g.text().toInt();
		int blue = b.text().toInt();

		return QColor(red, green, blue);
	}
	else if(type == "bool")
	{
		if(text == "true")
			return QVariant(true, 3);
		else if(text == "false")
			return QVariant(false, 3);
		return QVariant(text.toInt(), 3);
	}
	else if(type == "number")
	{
		return text.toInt();
	}
	else if(type == "size")
	{
		QDomElement w = node.namedItem("width").toElement();
		QDomElement h = node.namedItem("height").toElement();

		return QSize(w.text().toInt(), h.text().toInt());
	}
	else if(type == "point")
	{
		QDomElement x = node.namedItem("x").toElement();
		QDomElement y = node.namedItem("y").toElement();

		return QPoint(x.text().toInt(), y.text().toInt());
	}
	else if(type == "font")
	{
		QDomElement fa = node.namedItem("family").toElement();
		QDomElement p = node.namedItem("pointsize").toElement();
		QDomElement w = node.namedItem("weight").toElement();
		QDomElement b = node.namedItem("bold").toElement();
		QDomElement i = node.namedItem("italic").toElement();
		QDomElement u = node.namedItem("underline").toElement();
		QDomElement s = node.namedItem("strikeout").toElement();

		QFont f;
		f.setFamily(fa.text());
		f.setPointSize(p.text().toInt());
		f.setWeight(w.text().toInt());
		f.setBold(b.text().toInt());
		f.setItalic(i.text().toInt());
		f.setUnderline(u.text().toInt());
		f.setStrikeOut(s.text().toInt());

		return f;
	}
	else if(type == "cursor")
	{
		return QCursor(text.toInt());
	}
	else if(type == "time")
	{
		QDomElement h = node.namedItem("hour").toElement();
		QDomElement m = node.namedItem("minute").toElement();
		QDomElement s = node.namedItem("second").toElement();

		return QTime(h.text().toInt(), m.text().toInt(), s.text().toInt());
	}
	else if(type == "date")
	{
		QDomElement y = node.namedItem("year").toElement();
		QDomElement m = node.namedItem("month").toElement();
		QDomElement d = node.namedItem("day").toElement();

		return QDate(y.text().toInt(), m.text().toInt(), d.text().toInt());
	}
	else if(type == "datetime")
	{
		QDomElement h = node.namedItem("hour").toElement();
		QDomElement m = node.namedItem("minute").toElement();
		QDomElement s = node.namedItem("second").toElement();
		QDomElement y = node.namedItem("year").toElement();
		QDomElement mo = node.namedItem("month").toElement();
		QDomElement d = node.namedItem("day").toElement();

		QTime t(h.text().toInt(), m.text().toInt(), s.text().toInt());
		QDate da(y.text().toInt(), mo.text().toInt(), d.text().toInt());

		return QDateTime(da, t);
	}
	else if(type == "sizepolicy")
	{
		QDomElement h = node.namedItem("hsizetype").toElement();
		QDomElement v = node.namedItem("vsizetype").toElement();
		QDomElement hs = node.namedItem("horstretch").toElement();
		QDomElement vs = node.namedItem("verstretch").toElement();

		QSizePolicy s;
		s.setHorData((QSizePolicy::SizeType)h.text().toInt());
		s.setVerData((QSizePolicy::SizeType)v.text().toInt());
		s.setHorStretch(hs.text().toInt());
		s.setVerStretch(vs.text().toInt());
		return s;
	}
	else if(type == "pixmap")
	{
		if(m_savePixmapsInline || !m_currentForm || !m_currentItem || !m_currentForm->pixmapCollection()->contains(text))
			return loadImage(tag.ownerDocument(), text);
		else
		{
			m_currentItem->setPixmapName(name.latin1(), text);
			return m_currentForm->pixmapCollection()->getPixmap(text);
		}
		return QVariant(QPixmap());
	}
	else if(type == "enum")
		return text;
	else if(type == "set")
	{
		WidgetWithSubpropertiesInterface* subpropIface = dynamic_cast<WidgetWithSubpropertiesInterface*>(obj);
		QObject *subobject = (subpropIface && subpropIface->subwidget()) ? subpropIface->subwidget() : obj;
		const int count = subobject->metaObject()->findProperty(name.latin1(), true);
		const QMetaProperty *meta = count!=-1 ? subobject->metaObject()->property(count, true) : 0;

		if (meta) {
			if (meta->isSetType()) {
				QStrList keys;
				const QStringList list( QStringList::split("|", text) );
				for (QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it)
					keys.append((*it).latin1());

				return meta->keysToValue(keys);
			}
		}
		else {
			// Metaproperty not found, probably because subwidget is not created.
			// We will return a string list here with hope that names will 
			// be resolved and translated into an integer value later when subwidget is created,
			// e.g. near KexiFormView::updateValuesForSubproperties()
			return QStringList::split("|", text);
		}
	}
	return QVariant();
}

/////////////////////////////////////////////////////////////////////////////
///////////// Functions to save/load widgets ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
FormIO::saveWidget(ObjectTreeItem *item, QDomElement &parent, QDomDocument &domDoc, bool insideGridLayout)
{
	if (!item)
		return;
	bool savedAlignment = false;
	// we let Spring class handle saving itself
	if(item->className() == "Spring")
	{
		Spring::saveSpring(item, parent, domDoc, insideGridLayout);
		return;
	}

	bool resetCurrentForm = false;
	m_currentItem = item;
	if(!m_currentForm) // copying widget
	{
		resetCurrentForm = true;
		m_currentForm = item->container() ? item->container()->form() : item->parent()->container()->form();
	}


	WidgetLibrary *lib = m_currentForm->library();
//	if(item->container())
//		lib = item->container()->form()->manager()->lib();
//	else
//		lib = item->parent()->container()->form()->manager()->lib();

	// We create the "widget" element
	QDomElement tclass = domDoc.createElement("widget");
	parent.appendChild(tclass);

	if(insideGridLayout)
	{
		tclass.setAttribute("row", item->gridRow());
		tclass.setAttribute("column", item->gridCol());
		if(item->spanMultipleCells())
		{
			tclass.setAttribute("rowspan", item->gridRowSpan());
			tclass.setAttribute("colspan", item->gridColSpan());
		}
	}

	if(!item->parent()) // Toplevel widget
		tclass.setAttribute("class", "QWidget");
	// For compatibility, HBox, VBox and Grid are saved as "QLayoutWidget"
	else if(item->widget()->isA("HBox") || item->widget()->isA("VBox") || item->widget()->isA("Grid")
			|| item->widget()->isA("HFlow") || item->widget()->isA("VFlow"))
		tclass.setAttribute("class", "QLayoutWidget");
	else if(item->widget()->isA("CustomWidget"))
		tclass.setAttribute("class", item->className());
	else // Normal widgets
		tclass.setAttribute("class", lib->savingName(item->widget()->className()) );

	savePropertyValue(tclass, domDoc, "name", item->widget()->property("name"), item->widget());

	// Important: save dataSource property FIRST before properties like "alignment"
	// - needed when subproperties are defined after subwidget creation, and subwidget is created after setting "dataSource"
	//   (this is the case for KexiDBAutoField)
//! @todo more properties like "dataSource" may needed here...
//	if (-1 != item->widget()->metaObject()->findProperty("dataSource"))
	//	savePropertyValue(tclass, domDoc, "dataSource", item->widget()->property("dataSource"), item->widget());

	// We don't want to save the geometry if the widget is inside a layout (so parent.tagName() == "grid" for example)
	if(item && !item->parent()) {
		// save form widget size, but not its position
		savePropertyValue(tclass, domDoc, "geometry",
			QRect( QPoint(0,0), item->widget()->size()),
			item->widget());
	}
	// normal widget (if == "UI', it means we're copying widget)
	else if(parent.tagName() == "widget" || parent.tagName() == "UI")
		savePropertyValue(tclass, domDoc, "geometry", item->widget()->property("geometry"), item->widget());

	// Save the buddy widget for a label
	if(item->widget()->inherits("QLabel") && ((QLabel*)item->widget())->buddy())
		savePropertyElement(tclass, domDoc, "property", "buddy", ((QLabel*)item->widget())->buddy()->name());

	// We save every property in the modifProp list of the ObjectTreeItem
	QVariantMap *map = new QVariantMap( *(item->modifiedProperties()) );
	QMap<QString,QVariant>::ConstIterator endIt = map->constEnd();
	for(QMap<QString,QVariant>::ConstIterator it = map->constBegin(); it != endIt; ++it)
	{
		const QCString name( it.key().latin1() );
		if(name == "hAlign" || name == "vAlign" || name == "wordbreak" || name == "alignment") {
			if(!savedAlignment) // not to save it twice
			{
				savePropertyValue(tclass, domDoc, "alignment", item->widget()->property("alignment"), item->widget());
				savedAlignment = true;
			}
		}
		else if(name == "name" || name == "geometry" || name == "layout") {
			// these have already been saved
		}
		else {
			savePropertyValue(tclass, domDoc, it.key().latin1(), item->widget()->property(it.key().latin1()), 
				item->widget(), lib);
		}
	}
	delete map;

	if(item->widget()->isA("CustomWidget")) {
		QDomDocument doc("TEMP");
		doc.setContent(item->m_unknownProps);
		for(QDomNode n = doc.firstChild(); !n.isNull(); n = n.nextSibling()) {
			tclass.appendChild(n.cloneNode());
		}

	}
	// Saving container 's layout if there is one
	QDomElement layout;
	if(item->container() && item->container()->layoutType() != Container::NoLayout)
	{
		if(item->container()->layout()) // there is a layout
		{
			layout = domDoc.createElement("temp");
			savePropertyValue(layout, domDoc, "name", "unnamed", item->widget());
			if(item->modifiedProperties()->contains("layoutMargin"))
				savePropertyElement(layout, domDoc, "property", "margin", item->container()->layoutMargin());
			if(item->modifiedProperties()->contains("layoutSpacing"))
				savePropertyElement(layout, domDoc, "property", "spacing", item->container()->layoutSpacing());
			tclass.appendChild(layout);
		}
	}

	int layoutType = item->container() ? item->container()->layoutType() : Container::NoLayout;
	switch(layoutType) {
		case Container::Grid: // grid layout
		{
			layout.setTagName("grid");
			for(ObjectTreeItem *objIt = item->children()->first(); objIt; objIt = item->children()->next())
				saveWidget(objIt, layout, domDoc, true);
			break;
		}
		case Container::HBox: case Container::VBox:
		{
			// as we don't save geometry, we need to sort widgets in the right order, not creation order
			WidgetList *list;
			if(layout.tagName() == "hbox") {
				list = new HorWidgetList(item->container()->form()->toplevelContainer()->widget());
				layout.setTagName("hbox");
			}
			else {
				list = new VerWidgetList(item->container()->form()->toplevelContainer()->widget());
				layout.setTagName("vbox");
			}

			for(ObjectTreeItem *objTree = item->children()->first(); objTree; objTree = item->children()->next())
				list->append(objTree->widget());
			list->sort();

			for(QWidget *obj = list->first(); obj; obj = list->next()) {
				ObjectTreeItem *titem = item->container()->form()->objectTree()->lookup(obj->name());
				if(item)
					saveWidget(titem, layout, domDoc);
			}
			delete list;
			break;
		}
		case Container::HFlow: case Container::VFlow:
		{
			layout.setTagName("grid");
			KexiFlowLayout *flow = static_cast<KexiFlowLayout*>(item->container()->layout());
			if(!flow)  break;
			WidgetList *list = (WidgetList*)flow->widgetList();

			// save some special properties
			savePropertyElement(layout, domDoc, "property", "customLayout", Container::layoutTypeToString(item->container()->layoutType()) );
			savePropertyElement(layout, domDoc, "property", "justify", QVariant(static_cast<KexiFlowLayout*>(item->container()->layout())->isJustified(), 3) );

			// fill the widget's grid info, ie just simulate grid layout
			item->container()->createGridLayout(true);
			for(QWidget *obj = list->first(); obj; obj = list->next()) {
				ObjectTreeItem *titem = item->container()->form()->objectTree()->lookup(obj->name());
				if(item)
					saveWidget(titem, layout, domDoc, true); // save grid info for compatibility with QtDesigner
			}
			delete list;
			break;
		}
		default:
		{
			for(ObjectTreeItem *objIt = item->children()->first(); objIt; objIt = item->children()->next())
				saveWidget(objIt, tclass, domDoc);
		}
	}

	addIncludeFileName(lib->includeFileName(item->widget()->className()), domDoc);

	if(resetCurrentForm)
		m_currentForm = 0;
	m_currentItem = 0;
}

void
FormIO::cleanClipboard(QDomElement &uiElement)
{
	// remove includehints element not needed
	if(!uiElement.namedItem("includehints").isNull())
		uiElement.removeChild(uiElement.namedItem("includehints"));
	// and ensure images and connection are at the end
	if(!uiElement.namedItem("connections").isNull())
		uiElement.insertAfter(uiElement.namedItem("connections"), QDomNode());
	if(!uiElement.namedItem("images").isNull())
		uiElement.insertAfter(uiElement.namedItem("images"), QDomNode());
}

void
FormIO::loadWidget(Container *container, const QDomElement &el, QWidget *parent)
{
	bool resetCurrentForm = false;
	if(!m_currentForm) // pasting widget
	{
		resetCurrentForm = true;
		m_currentForm = container->form();
	}

	// We first look for the widget's name
	QString wname;
	for(QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if((n.toElement().tagName() == "property") && (n.toElement().attribute("name") == "name"))
		{
			wname = n.toElement().text();
			break;
		}
	}

	QWidget *w;
	QCString classname, alternate;
	// We translate some name (for compatibility)
	if(el.tagName() == "spacer")
		classname = "Spring";
	else if(el.attribute("class") == "QLayoutWidget")
	{
		for(QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling())
		{
			QString tagName = n.toElement().tagName();
			if(tagName == "property")
				continue;
			if(tagName == "hbox")
				classname = "HBox";
			else if(tagName == "vbox")
				classname = "VBox";
			else if(tagName == "grid") {
				// first, see if it is flow layout
				for(QDomNode child = n.firstChild(); !child.isNull(); child = child.nextSibling())  {
					if((child.toElement().tagName() == "property") 
						&& (child.toElement().attribute("name") == "customLayout"))
					{
						classname = child.toElement().text().latin1();
						break;
					}
				}

				if(classname.isEmpty()) // normal grid
					classname = "Grid";
			}
		}
	}
	else
	// We check if this classname is an alternate one, and replace it if necessary
	{
		classname = el.attribute("class").latin1();
		alternate = container->form()->library()->classNameForAlternate(classname);
	}

	if(alternate == "CustomWidget")
		w = new CustomWidget(classname, container->widget(), wname.latin1());
	else
	{
		if(!alternate.isNull())
			classname = alternate;

		int widgetOptions = WidgetFactory::DefaultOptions;
		if (!container->form()->designMode()) {
			widgetOptions ^= WidgetFactory::DesignViewMode;
		}

		if(!parent)
			w = container->form()->library()->createWidget(classname, container->widget(), 
				wname.latin1(), container, widgetOptions);
		else
			w = container->form()->library()->createWidget(classname, parent, wname.latin1(), 
				container, widgetOptions);
	}

	if(!w)
		return;
#if KDE_VERSION >= KDE_MAKE_VERSION(3,4,0) 
//! @todo allow setting this for data view mode as well
	if (m_currentForm->designMode()) {
		//don't generate accelerators for widgets in design mode
		KAcceleratorManager::setNoAccel(w);
	}
#endif
	w->setStyle(&(container->widget()->style()));
	w->show();

	// We create and insert the ObjectTreeItem at the good place in the ObjectTree
	ObjectTreeItem *item = container->form()->objectTree()->lookup(wname);
	if (!item)  {
		// not yet created
		item =  new ObjectTreeItem(container->form()->library()->displayName(classname), 
			wname, w, container);
		if(parent)  {
			ObjectTreeItem *titem = container->form()->objectTree()->lookup(parent->name());
			if(titem)
				container->form()->objectTree()->addItem(titem, item);
			else
				kdDebug() << "FORMIO :: ERROR no parent widget "  << endl;
		}
		else
			container->form()->objectTree()->addItem(container->objectTree(), item);
	}
	//assign item for its widget if it supports DesignTimeDynamicChildWidgetHandler interface
	//(e.g. KexiDBAutoField)
	if (container->form()->designMode() && dynamic_cast<DesignTimeDynamicChildWidgetHandler*>(w)) {
		dynamic_cast<DesignTimeDynamicChildWidgetHandler*>(w)->assignItem(item);
	}

	m_currentItem = item;
	// if we are inside a Grid, we need to insert the widget in the good cell
	if(container->layoutType() == Container::Grid)  {
		QGridLayout *layout = (QGridLayout*)container->layout();
		if(el.hasAttribute("rowspan")) { // widget spans multiple cells
			if(layout)
				layout->addMultiCellWidget(w, el.attribute("row").toInt(), el.attribute("row").toInt() + el.attribute("rowspan").toInt()-1,
					 el.attribute("column").toInt(),  el.attribute("column").toInt() + el.attribute("colspan").toInt()-1);
			 item->setGridPos(el.attribute("row").toInt(),  el.attribute("column").toInt(), el.attribute("rowspan").toInt(),
			   el.attribute("colspan").toInt());
		}
		else  {
			if(layout)
				layout->addWidget(w, el.attribute("row").toInt(), el.attribute("column").toInt());
			item->setGridPos(el.attribute("row").toInt(),  el.attribute("column").toInt(), 0, 0);
		}
	}
	else if(container->layout())
		container->layout()->add(w);

	readChildNodes(item, container, el, w);

	if(item->container() && item->container()->layout())
		item->container()->layout()->activate();

	// We add the autoSaveProperties in the modifProp list of the ObjectTreeItem, so that they are saved later
	QValueList<QCString> list(container->form()->library()->autoSaveProperties(w->className()));
	QValueList<QCString>::ConstIterator endIt = list.constEnd();
	KFormDesigner::WidgetWithSubpropertiesInterface* subpropIface 
		= dynamic_cast<KFormDesigner::WidgetWithSubpropertiesInterface*>(w);
	QWidget *subwidget = (subpropIface && subpropIface->subwidget()) ? subpropIface->subwidget() : w;
	for(QValueList<QCString>::ConstIterator it = list.constBegin(); it != endIt; ++it) {
		if(subwidget->metaObject()->findProperty(*it, true) != -1)
			item->addModifiedProperty(*it, subwidget->property(*it));
	}

	if(resetCurrentForm)
		m_currentForm = 0;
	m_currentItem = 0;
}

void
FormIO::createToplevelWidget(Form *form, QWidget *container, QDomElement &el)
{
	// We first look for the widget's name
	QString wname;
	for(QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if((n.toElement().tagName() == "property") && (n.toElement().attribute("name") == "name"))
		{
			wname = n.toElement().text();
			break;
		}

	}
	// And rename the widget and its ObjectTreeItem
	container->setName(wname.latin1());
	if(form->objectTree())
		form->objectTree()->rename(form->objectTree()->name(), wname);
	form->setInteractiveMode(false);

	QDict<QLabel>  *oldBuddies = 0;
	if(m_buddies)  // save old buddies (for subforms)
		oldBuddies = m_buddies;
	m_buddies = new QDict<QLabel>();
	m_currentItem = form->objectTree();

	readChildNodes(form->objectTree(), form->toplevelContainer(), el, container);

	// Now the Form is fully loaded, we can assign the buddies
	QDictIterator<QLabel> it(*m_buddies);
	for(; it.current(); ++it)
	{
		ObjectTreeItem *item = form->objectTree()->lookup(it.currentKey());
		if(!item || !item->widget())
		{
			kdDebug() << "Cannot assign buddy for widget " << it.current()->name() << " to " << it.currentKey() << endl;
			continue;
		}
		it.current()->setBuddy(item->widget());
	}
	delete m_buddies;
	m_buddies = oldBuddies; // and restore it

	m_currentItem = 0;

	form->setInteractiveMode(true);
}

void
FormIO::readChildNodes(ObjectTreeItem *item, Container *container, const QDomElement &el, QWidget *w)
{
	QString eltag = el.tagName();

	WidgetWithSubpropertiesInterface* subpropIface = dynamic_cast<WidgetWithSubpropertiesInterface*>(w);
	QWidget *subwidget = (subpropIface && subpropIface->subwidget()) ? subpropIface->subwidget() : w;

	for(QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		QString tag = n.toElement().tagName();
		QDomElement node = n.toElement();

		if((tag == "property") || (tag == "attribute"))
		{
			QString name = node.attribute("name");
			//if(name == "geometry")
			//	hasGeometryProp = true;
			if( ((eltag == "grid") || (eltag == "hbox") || (eltag == "vbox")) &&
			      (name == "name")) // we don't care about layout names
				continue;

			if (node.attribute("subwidget")=="true") {
				//this is property for subwidget: remember it for delayed setting
				//because now the subwidget could be not created yet (true e.g. for KexiDBAutoField)
				const QVariant val( readPropertyValue(node.firstChild(), w, name) );
				kdDebug() << val.toStringList() << endl;
				item->addSubproperty( name.latin1(), val );
				//subwidget->setProperty(name.latin1(), val);
				item->addModifiedProperty( name.latin1(), val );
				continue;
			}

			// We cannot assign the buddy now as the buddy widget may not be created yet
			if(name == "buddy")
				m_buddies->insert(readPropertyValue(node.firstChild(), w, name).toString(), (QLabel*)w);
			else if(((eltag == "grid") || (eltag == "hbox") || (eltag == "vbox")) &&
			  item->container() && item->container()->layout()) {
			  // We load the margin of a Layout
				if(name == "margin")  {
					int margin = readPropertyValue(node.firstChild(), w, name).toInt();
					item->container()->setLayoutMargin(margin);
					item->container()->layout()->setMargin(margin);
				}
				// We load the spacing of a Layout
				else if(name == "spacing")  {
					int spacing = readPropertyValue(node.firstChild(), w, name).toInt();
					item->container()->setLayoutSpacing(spacing);
					item->container()->layout()->setSpacing(spacing);
				}
				else if((name == "justify")){
					bool justify = readPropertyValue(node.firstChild(), w, name).toBool();
					KexiFlowLayout *flow = static_cast<KexiFlowLayout*>(item->container()->layout());
					if(flow)
						flow->setJustified(justify);
				}
			}
			// If the object doesn't have this property, we let the Factory handle it (maybe a special property)
			else if(subwidget->metaObject()->findProperty(name.latin1(), true) == -1)
			{
				if(w->className() == QString::fromLatin1("CustomWidget"))
					item->storeUnknownProperty(node);
				else {
					bool read = container->form()->library()->readSpecialProperty(
						w->className(), node, w, item);
					if(!read) // the factory doesn't support this property neither
						item->storeUnknownProperty(node);
				}
			}
			else // we have a normal property, let's load it
			{
				QVariant val( readPropertyValue(node.firstChild(), w, name) );
				if(name == "geometry" && dynamic_cast<FormWidget*>(w)) {
					//fix geometry if needed - this is top level form widget
					QRect r( val.toRect() );
					if (r.left()<0) //negative X!
						r.moveLeft(0);
					if (r.top()<0) //negative Y!
						r.moveTop(0);
					val = r;
				}
				subwidget->setProperty(name.latin1(), val);
//				int count = w->metaObject()->findProperty(name, true);
//				const QMetaProperty *meta = w->metaObject()->property(count, true);
//				if(meta && meta->isEnumType()) {
//					val = w->property(name.latin1()); //update: we want a numeric value of enum
//				}
				item->addModifiedProperty(name.latin1(), val);
			}
		}
		else if(tag == "widget") // a child widget
		{
			if(item->container()) // we are a Container
				loadWidget(item->container(), node);
			else
				loadWidget(container, node, w);
		}
		else if(tag == "spacer")  {
			loadWidget(container, node, w);
		}
		else if(tag == "grid") {
			// first, see if it is flow layout
			QString layoutName;
			for(QDomNode child = node.firstChild(); !child.isNull(); child = child.nextSibling())  {
				if((child.toElement().tagName() == "property") && (child.toElement().attribute("name") == "customLayout"))  {
					layoutName = child.toElement().text();
					break;
				}
			}

			 if(layoutName == "HFlow") {
				item->container()->m_layType = Container::HFlow;
				KexiFlowLayout *layout = new KexiFlowLayout(item->widget());
				layout->setOrientation(Horizontal);
				item->container()->m_layout = (QLayout*)layout;
			}
			else if(layoutName == "VFlow") {
				item->container()->m_layType = Container::VFlow;
				KexiFlowLayout *layout = new KexiFlowLayout(item->widget());
				layout->setOrientation(Vertical);
				item->container()->m_layout = (QLayout*)layout;
			}
			else { // grid layout
				item->container()->m_layType = Container::Grid;
				QGridLayout *layout = new QGridLayout(item->widget(), 1, 1);
				item->container()->m_layout = (QLayout*)layout;
			}
			readChildNodes(item, container, node, w);
		}
		else if(tag == "vbox")  {
			item->container()->m_layType = Container::VBox;
			QVBoxLayout *layout = new QVBoxLayout(item->widget());
			item->container()->m_layout = (QLayout*)layout;
			readChildNodes(item, container, node, w);
		}
		else if(tag == "hbox") {
			item->container()->m_layType = Container::HBox;
			QHBoxLayout *layout = new QHBoxLayout(item->widget());
			item->container()->m_layout = (QLayout*)layout;
			readChildNodes(item, container, node, w);
		}
		else {// unknown tag, we let the Factory handle it
			if(w->className() == QString::fromLatin1("CustomWidget"))
				item->storeUnknownProperty(node);
			else {
				bool read = container->form()->library()->readSpecialProperty(
					w->className(), node, w, item);
				if(!read) // the factory doesn't suport this property neither
					item->storeUnknownProperty(node);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
///////////// Helper functions //////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
FormIO::addIncludeFileName(const QString &include, QDomDocument &domDoc)
{
	if(include.isEmpty())
		return;

	QDomElement includes;
	QDomElement uiEl = domDoc.namedItem("UI").toElement();
	if(uiEl.namedItem("includehints").isNull())
	{
		includes = domDoc.createElement("includehints");
		uiEl.appendChild(includes);
	}
	else
		includes = uiEl.namedItem("includehints").toElement();

	// Check if this include has already been saved, and return if it is the case
	for(QDomNode n = includes.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if(n.toElement().text() == include)
			return;
	}

	QDomElement includeHint = domDoc.createElement("includehint");
	includes.appendChild(includeHint);
	QDomText includeText = domDoc.createTextNode(include);
	includeHint.appendChild(includeText);
}

////////  Qt Designer code: these two functions were copied (and adapted) from Qt Designer for compatibility ////////

QString
FormIO::saveImage(QDomDocument &domDoc, const QPixmap &pixmap)
{
	QDomNode node = domDoc.namedItem("images");
	QDomElement images;
	if(node.isNull())
	{
		images = domDoc.createElement("images");
		QDomElement ui = domDoc.namedItem("UI").toElement();
		ui.appendChild(images);
	}
	else
		images = node.toElement();

	int count = images.childNodes().count();
	QDomElement image = domDoc.createElement("image");
	QString name = "image" + QString::number(count);
	image.setAttribute("name", name);

	QImage img = pixmap.convertToImage();
	QByteArray ba;
	QBuffer buf(ba);
	buf.open( IO_WriteOnly | IO_Translate );
	QString format = img.depth() > 1 ? "XPM" : "XBM";
	QImageIO iio( &buf, format.latin1() );
	iio.setImage( img );
	iio.write();
	buf.close();
	QByteArray bazip = qCompress( ba );
	ulong len = bazip.size();

	QDomElement data = domDoc.createElement("data");
	data.setAttribute("format", format + ".GZ");
	data.setAttribute("length", ba.size());

	static const char hexchars[] = "0123456789abcdef";
	QString content;
	for(int i = 4; i < (int)len; ++i)
	{
	uchar s = (uchar) bazip[i];
	content += hexchars[s >> 4];
	content += hexchars[s & 0x0f];
	}

	QDomText text = domDoc.createTextNode(content);
	data.appendChild(text);
	image.appendChild(data);
	images.appendChild(image);

	return name;
}

QPixmap
FormIO::loadImage(QDomDocument domDoc, const QString& name)
{
	QDomElement images = domDoc.namedItem("UI").namedItem("images").toElement();
	if(images.isNull())
		return 0;

	QDomElement image;
	for(QDomNode n = images.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if((n.toElement().tagName() == "image") && (n.toElement().attribute("name") == name))
		{
			image = n.toElement();
			break;
		}
	}

	QPixmap pix;
	QString data = image.namedItem("data").toElement().text();
	const int lengthOffset = 4;
	int baSize = data.length() / 2 + lengthOffset;
	uchar *ba = new uchar[baSize];
	for(int i = lengthOffset; i < baSize; ++i)
	{
		char h = data[2 * (i-lengthOffset)].latin1();
		char l = data[2 * (i-lengthOffset) + 1].latin1();
		uchar r = 0;
		if(h <= '9')
		    r += h - '0';
		else
		    r += h - 'a' + 10;
		r = r << 4;
		if(l <= '9')
		    r += l - '0';
		else
		    r += l - 'a' + 10;
		ba[i] = r;
	}

	QString format = image.namedItem("data").toElement().attribute("format", "PNG");
	if((format == "XPM.GZ") || (format == "XBM.GZ"))
	{
		ulong len = image.attribute("length").toULong();
		if(len < data.length() * 5)
		    len = data.length() * 5;
		// qUncompress() expects the first 4 bytes to be the expected length of
		// the uncompressed data
		ba[0] = ( len & 0xff000000 ) >> 24;
		ba[1] = ( len & 0x00ff0000 ) >> 16;
		ba[2] = ( len & 0x0000ff00 ) >> 8;
		ba[3] = ( len & 0x000000ff );
		QByteArray baunzip = qUncompress(ba, baSize);
		pix.loadFromData( (const uchar*)baunzip.data(), baunzip.size(), format.left(format.find('.')).latin1() );
	}
	else
		pix.loadFromData( (const uchar*)ba+lengthOffset, baSize-lengthOffset, format.latin1() );

	delete[] ba;

	return pix;
}

//////// End of Qt Designer code ////////////////////////////////////////////////////////

#include "formIO.moc"
