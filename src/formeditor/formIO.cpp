/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005-2009 Jarosław Staniek <staniek@kde.org>

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
#include <qobject.h>
#include <qdatetime.h>
#include <qlabel.h>
#include <qpainter.h>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QImageWriter>

#include <kfiledialog.h>
#include <klocale.h>
#include <k3command.h>
#include <kacceleratormanager.h>

#include <kexiutils/utils.h>
#include "FormWidget.h"
#include "form.h"
#include "container.h"
#include "objecttree.h"
//unused #include "formmanager.h"
#include "widgetlibrary.h"
#include "spring.h"
#include "events.h"
#include "utils.h"
#include "widgetwithsubpropertiesinterface.h"
#include "formIO.h"

#define KEXI_NO_FLOWLAYOUT
#ifdef __GNUC__
#warning "Port Kexi flow layout!"
#endif
#ifndef KEXI_NO_FLOWLAYOUT
#include <kexiutils/FlowLayout.h>
#endif

#define KEXI_NO_PIXMAPCOLLECTION
#ifdef __GNUC__
#warning pixmapcollection
#endif
#ifndef KEXI_NO_PIXMAPCOLLECTION
#include "pixmapcollection.h"
#endif

//! @return Value of attribute "name", or "objectName" in absence of "name".
//!         This is for compatibility with Kexi 1.x.
static QString nameAttribute(const QDomElement& el)
{
    QString res( el.attribute("name") );
    if (res.isEmpty()) {
        res = el.attribute("objectName");
    }
    return res;
}

//! A blank widget used when the class name is not supported
CustomWidget::CustomWidget(const QByteArray &className, QWidget *parent)
        : QWidget(parent), m_className(className)
{
    setBackgroundRole(QPalette::Dark);
}

CustomWidget::~CustomWidget()
{
}

void
CustomWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setBrush(palette().text());
    QRect r(rect());
    r.setX(r.x() + 2);
    p.drawText(r, Qt::AlignTop, m_className);
}

using namespace KFormDesigner;

QHash<QString, QLabel*> *FormIO::m_buddies = 0;
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
    QString _filename;
    if (!form->filename().isEmpty() && filename.isEmpty()) {
        _filename = form->filename();
    }

    if (filename.isEmpty()) {
        _filename = KFileDialog::getSaveFileName(KUrl(), i18n("*.ui|Qt Designer UI Files"));
        if (_filename.isEmpty()) {
            return false;
        }
    }
    else {
        _filename = filename;
    }
    form->setFilename(_filename);

    QDomDocument domDoc;
    if (!saveFormToDom(form, domDoc))
        return false;

    QFile file(_filename);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QTextStream stream(&file);
    stream << domDoc.toString(3);
    file.close();

    return true;
}

bool
FormIO::saveFormToByteArray(Form *form, QByteArray &dest)
{
    QDomDocument domDoc;
    if (!saveFormToDom(form, domDoc))
        return false;
    dest = domDoc.toByteArray();
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
    QHash<QByteArray, QString>::ConstIterator itEnd = form->headerProperties()->constEnd();
    for (QHash<QByteArray, QString>::ConstIterator it = form->headerProperties()->constBegin(); 
        it != itEnd; ++it)
    {
        headerPropertiesEl.setAttribute(it.key(), it.value());
    }
    uiElement.appendChild(headerPropertiesEl);

    // We save the savePixmapsInline property in the Form
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

    // Save tab Stops
    if (form->autoTabStops())
        form->autoAssignTabStops();
    QDomElement tabStops = domDoc.createElement("tabstops");
    uiElement.appendChild(tabStops);
    foreach (ObjectTreeItem *item, *form->tabStops()) {
        QDomElement tabstop = domDoc.createElement("tabstop");
        tabStops.appendChild(tabstop);
        QDomText tabStopText = domDoc.createTextNode(item->name());
        tabstop.appendChild(tabStopText);
    }

#ifdef __GNUC__
#warning pixmapcollection
#endif
#ifndef KEXI_NO_PIXMAPCOLLECTION
    // Save the Form 's PixmapCollection
    form->pixmapCollection()->save(uiElement);
#endif
#ifdef KFD_SIGSLOTS
    // Save the Form connections
    form->connectionBuffer()->save(uiElement);
#endif

    form->setUndoStackClean();

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

    if (!parsed) {
        kDebug() << errMsg;
        kDebug() << "line:" << errLine << "col:" << errCol;
        return false;
    }

    if (!loadFormFromDom(form, container, inBuf)) {
        return false;
    }
    if (preview) {
        form->setMode(Form::DataMode);
    }
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

    if (!parsed) {
        kDebug() << errMsg;
        kDebug() << "line:" << errLine << "col: " << errCol;
        return false;
    }

    if (!loadFormFromDom(form, container, inBuf)) {
        return false;
    }
    if (preview) {
        form->setMode(Form::DataMode);
    }
    return true;
}

bool
FormIO::loadFormFromFile(Form *form, QWidget *container, const QString &filename)
{
    QString errMsg;
    int errLine;
    int errCol;
    QString _filename;

    if (filename.isEmpty()) {
        _filename = KFileDialog::getOpenFileName(KUrl(), i18n("*.ui|Qt Designer UI Files"));
        if (_filename.isEmpty()) {
            return false;
        }
    }
    else {
        _filename = filename;
    }

    QFile file(_filename);
    if (!file.open(QIODevice::ReadOnly)) {
//! @todo proved err msg to the user
        kDebug() << "Cannot open the file " << _filename;
        return false;
    }
    QDomDocument doc;
    if (!doc.setContent(&file, false/* !namespaceProcessing*/,
                        &errMsg, &errLine, &errCol)) {
//! @todo proved err msg to the user
        kDebug() << errMsg;
        kDebug() << errLine << "col:" << errCol;
        return false;
    }

    return loadFormFromDom(form, container, doc);
}

bool
FormIO::loadFormFromDom(Form *form, QWidget *container, QDomDocument &inBuf)
{
    m_currentForm = form;

    QDomElement ui = inBuf.firstChildElement("UI");

    //custom properties
    form->headerProperties()->clear();
    QDomElement headerPropertiesEl = ui.firstChildElement("kfd:customHeader");
    QDomAttr attr = headerPropertiesEl.firstChild().toAttr();
    while (!attr.isNull() && attr.isAttr()) {
        form->headerProperties()->insert(attr.name().toLatin1(), attr.value());
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
    kDebug() << "original format version: " << ver;
    form->setOriginalFormatVersion(ver);
    if (ver < KFormDesigner::version()) {
//! @todo We can either 1) convert from old format and later save in a new one or 2) keep old format.
//!     To do this we may need to look at the original format version number.
        kWarning() << "original format is older than current: " << KFormDesigner::version();
        form->setFormatVersion(KFormDesigner::version());
    } else
        form->setFormatVersion(ver);

    if (ver > KFormDesigner::version()) {
//! @todo display information about too new format and that "some information will not be available".
        kWarning() << "original format is newer than current: " << KFormDesigner::version();
    }

    // Load the pixmap collection
    m_savePixmapsInline = ((ui.firstChildElement("pixmapinproject").isNull()) || (!ui.firstChildElement("images").isNull()));
#ifdef __GNUC__
#warning pixmapcollection
#endif
#ifndef KEXI_NO_PIXMAPCOLLECTION
    form->pixmapCollection()->load(ui.namedItem("collection"));
#endif

    QDomElement element = ui.firstChildElement("widget");
    createToplevelWidget(form, container, element);

    // Loading the tabstops
    QDomElement tabStops = ui.firstChildElement("tabstops");
    if (!tabStops.isNull()) {
        int i = 0;
        uint itemsNotFound = 0;
        for (QDomNode n = tabStops.firstChild(); !n.isNull(); n = n.nextSibling(), i++) {
            QString name = n.toElement().text();
            ObjectTreeItem *item = form->objectTree()->lookup(name);
            if (!item) {
                kWarning() << "ERROR : no ObjectTreeItem ";
                continue;
            }
            const int index = form->tabStops()->indexOf(item);
            /* Compute a real destination index: "a number of not found items so far". */
            const int realIndex = i - itemsNotFound;
            if ((index != -1) && (index != realIndex)) { // the widget is not in the same place, so we move it
                form->tabStops()->removeOne(item);
                form->tabStops()->insert(realIndex, item);
            }
            if (index == -1) {
                itemsNotFound++;
                kDebug() << "FormIO: item '" << name << "' not in list";
            }
        }
    }

#ifdef KFD_SIGSLOTS
    // Load the form connections
    form->connectionBuffer()->load(ui.namedItem("connections"));
#endif

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
// kDebug() << "Saving the property: " << name;
    WidgetWithSubpropertiesInterface* subpropIface = dynamic_cast<WidgetWithSubpropertiesInterface*>(w);
    QWidget *subwidget = w;
    bool addSubwidgetFlag = false;
    int propertyId = KexiUtils::indexOfPropertyWithSuperclasses(w, name);
    const bool propertyIsName = qstrcmp(name, "objectName") == 0 || qstrcmp(name, "name") == 0;
    if (!propertyIsName && propertyId == -1 && subpropIface && subpropIface->subwidget()) { // try property from subwidget
        subwidget = subpropIface->subwidget();
        propertyId = KexiUtils::indexOfPropertyWithSuperclasses(
                         subpropIface->subwidget(), name);
        addSubwidgetFlag = true;
    }
    if (!propertyIsName && propertyId == -1) {
        kDebug() << "The object doesn't have this property. Let's try the WidgetLibrary.";
        if (lib)
            lib->saveSpecialProperty(w->metaObject()->className(), name, value, w, parentNode, parent);
        return;
    }

    QMetaProperty meta;
    if (!propertyIsName) {
        meta = KexiUtils::findPropertyWithSuperclasses(subwidget, propertyId);
    }
    if (!propertyIsName && (!meta.isValid() || !meta.isStored(subwidget)))   //not storable
        return;
    QDomElement propertyE = parent.createElement("property");
    propertyE.setAttribute("name", propertyIsName ? "name" /* compat with 1.x */ : name);
    if (addSubwidgetFlag)
        propertyE.setAttribute("subwidget", "true");

    if (meta.isValid() && meta.isEnumType()) {
        // this property is enum or set type
        QDomElement type;
        QDomText valueE;

        if (meta.isFlagType()) {
            /* js: it's simpler in Qt4:
            QStringList list = QStringList::fromStrList(meta.enumerator().valueToKeys(value.toInt()));
            type = parent.createElement("set");
            valueE = parent.createTextNode(list.join("|"));*/
            type = parent.createElement("set");
            valueE = parent.createTextNode(
                         meta.enumerator().valueToKeys(value.toInt()));
            type.appendChild(valueE);
        } else {
            QString s = meta.enumerator().valueToKey(value.toInt());
            type = parent.createElement("enum");
            valueE = parent.createTextNode(s);
            type.appendChild(valueE);
        }
        propertyE.appendChild(type);
        parentNode.appendChild(propertyE);
        return;
    }

    if (value.type() == QVariant::Pixmap) {
        QDomText valueE;
        QDomElement type = parent.createElement("pixmap");
        QByteArray property = propertyE.attribute("name").toLatin1();
//todo  QCString pixmapName = m_currentItem->widget()->property("pixmapName").toCString();
        if (m_savePixmapsInline /* (js)too risky: || m_currentItem->pixmapName(property).isNull() */)
            valueE = parent.createTextNode(saveImage(parent, value.value<QPixmap>()));
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
FormIO::writeVariant(QDomDocument &parent, QDomElement &parentNode, const QVariant& value)
{
    QDomElement type;
    QDomText valueE;

    switch (value.type()) {
    case QVariant::String: {
        type = parent.createElement("string");
        valueE = parent.createTextNode(value.toString());
        type.appendChild(valueE);
        break;
    }
    case QVariant::CString: {
        type = parent.createElement("cstring");
        valueE = parent.createTextNode(value.toString());
        type.appendChild(valueE);
        break;
    }
    case QVariant::Rect: {
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
    case QVariant::Color: {
        type = parent.createElement("color");
        QDomElement r = parent.createElement("red");
        QDomElement g = parent.createElement("green");
        QDomElement b = parent.createElement("blue");

        const QColor color(value.value<QColor>());
        QDomText valueR = parent.createTextNode(QString::number(color.red()));
        QDomText valueG = parent.createTextNode(QString::number(color.green()));
        QDomText valueB = parent.createTextNode(QString::number(color.blue()));

        r.appendChild(valueR);
        g.appendChild(valueG);
        b.appendChild(valueB);

        type.appendChild(r);
        type.appendChild(g);
        type.appendChild(b);
        break;
    }
    case QVariant::Bool: {
        type = parent.createElement("bool");
        //valueE = parent.createTextNode(QString::number(value.toBool()));
        valueE = parent.createTextNode(value.toBool() ? "true" : "false");
        type.appendChild(valueE);
        break;
    }
    case QVariant::Int:
    case QVariant::UInt: {
        type = parent.createElement("number");
        valueE = parent.createTextNode(QString::number(value.toInt()));
        type.appendChild(valueE);
        break;
    }
    case QVariant::Size: {
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
    case QVariant::Point: {
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
    case QVariant::Font: {
        type = parent.createElement("font");
        QDomElement f = parent.createElement("family");
        QDomElement p = parent.createElement("pointsize");
        QDomElement w = parent.createElement("weight");
        QDomElement b = parent.createElement("bold");
        QDomElement i = parent.createElement("italic");
        QDomElement u = parent.createElement("underline");
        QDomElement s = parent.createElement("strikeout");

        const QFont font(value.value<QFont>());
        QDomText valueF = parent.createTextNode(font.family());
        QDomText valueP = parent.createTextNode(QString::number(font.pointSize()));
        QDomText valueW = parent.createTextNode(QString::number(font.weight()));
        QDomText valueB = parent.createTextNode(QString::number(font.bold()));
        QDomText valueI = parent.createTextNode(QString::number(font.italic()));
        QDomText valueU = parent.createTextNode(QString::number(font.underline()));
        QDomText valueS = parent.createTextNode(QString::number(font.strikeOut()));

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
    case QVariant::Cursor: {
        type = parent.createElement("cursor");
        valueE = parent.createTextNode(QString::number(value.value<QCursor>().shape()));
        type.appendChild(valueE);
        break;
    }
    case QVariant::SizePolicy: {
        type = parent.createElement("sizepolicy");
        QDomElement h(parent.createElement("hsizetype"));
        QDomElement v(parent.createElement("vsizetype"));
        QDomElement hs(parent.createElement("horstretch"));
        QDomElement vs(parent.createElement("verstretch"));

        const QSizePolicy sizePolicy(value.value<QSizePolicy>());
        const QDomText valueH(parent.createTextNode(QString::number(sizePolicy.horizontalPolicy())));
        const QDomText valueV(parent.createTextNode(QString::number(sizePolicy.verticalPolicy())));
        const QDomText valueHS(parent.createTextNode(QString::number(sizePolicy.horizontalStretch())));
        const QDomText valueVS(parent.createTextNode(QString::number(sizePolicy.verticalStretch())));

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
    case QVariant::Time: {
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
    case QVariant::Date: {
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
    case QVariant::DateTime: {
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
FormIO::savePropertyElement(QDomElement &parentNode, QDomDocument &domDoc, const QString &tagName, 
    const QString &property, const QVariant &value)
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

    if (type == "string" || type == "cstring")
        return text;
    else if (type == "rect") {
        QDomElement x = node.firstChildElement("x");
        QDomElement y = node.firstChildElement("y");
        QDomElement w = node.firstChildElement("width");
        QDomElement h = node.firstChildElement("height");

        int rx = x.text().toInt();
        int ry = y.text().toInt();
        int rw = w.text().toInt();
        int rh = h.text().toInt();

        return QRect(rx, ry, rw, rh);
    } else if (type == "color") {
        const QDomElement r(node.firstChildElement("red"));
        const QDomElement g(node.firstChildElement("green"));
        const QDomElement b(node.firstChildElement("blue"));

        return QColor(r.text().toInt(), g.text().toInt(), b.text().toInt());
    } else if (type == "bool") {
        if (text == "true")
            return true;
        else if (text == "false")
            return false;
        return text.toInt() != 0;
    } else if (type == "number") {
        return text.toInt();
    } else if (type == "size") {
        QDomElement w = node.firstChildElement("width");
        QDomElement h = node.firstChildElement("height");

        return QSize(w.text().toInt(), h.text().toInt());
    } else if (type == "point") {
        QDomElement x = node.firstChildElement("x");
        QDomElement y = node.firstChildElement("y");

        return QPoint(x.text().toInt(), y.text().toInt());
    } else if (type == "font") {
        QDomElement fa = node.firstChildElement("family");
        QDomElement p = node.firstChildElement("pointsize");
        QDomElement w = node.firstChildElement("weight");
        QDomElement b = node.firstChildElement("bold");
        QDomElement i = node.firstChildElement("italic");
        QDomElement u = node.firstChildElement("underline");
        QDomElement s = node.firstChildElement("strikeout");

        QFont f;
        f.setFamily(fa.text());
        f.setPointSize(p.text().toInt());
        f.setWeight(w.text().toInt());
        f.setBold(b.text().toInt());
        f.setItalic(i.text().toInt());
        f.setUnderline(u.text().toInt());
        f.setStrikeOut(s.text().toInt());

        return f;
    } else if (type == "cursor") {
        return QCursor((Qt::CursorShape) text.toInt());
    } else if (type == "time") {
        QDomElement h = node.firstChildElement("hour");
        QDomElement m = node.firstChildElement("minute");
        QDomElement s = node.firstChildElement("second");

        return QTime(h.text().toInt(), m.text().toInt(), s.text().toInt());
    } else if (type == "date") {
        QDomElement y = node.firstChildElement("year");
        QDomElement m = node.firstChildElement("month");
        QDomElement d = node.firstChildElement("day");

        return QDate(y.text().toInt(), m.text().toInt(), d.text().toInt());
    } else if (type == "datetime") {
        QDomElement h = node.firstChildElement("hour");
        QDomElement m = node.firstChildElement("minute");
        QDomElement s = node.firstChildElement("second");
        QDomElement y = node.firstChildElement("year");
        QDomElement mo = node.firstChildElement("month");
        QDomElement d = node.firstChildElement("day");

        QTime t(h.text().toInt(), m.text().toInt(), s.text().toInt());
        QDate da(y.text().toInt(), mo.text().toInt(), d.text().toInt());

        return QDateTime(da, t);
    } else if (type == "sizepolicy") {
        QDomElement h = node.firstChildElement("hsizetype");
        QDomElement v = node.firstChildElement("vsizetype");
        QDomElement hs = node.firstChildElement("horstretch");
        QDomElement vs = node.firstChildElement("verstretch");

        QSizePolicy s;
        s.setHorizontalPolicy((QSizePolicy::SizeType)h.text().toInt());
        s.setVerticalPolicy((QSizePolicy::SizeType)v.text().toInt());
        s.setHorizontalStretch(hs.text().toInt());
        s.setVerticalStretch(vs.text().toInt());
        return s;
    } else if (type == "pixmap") {
#ifdef __GNUC__
#warning pixmapcollection
#endif
#ifndef KEXI_NO_PIXMAPCOLLECTION
        if (m_savePixmapsInline || !m_currentForm || !m_currentItem || !m_currentForm->pixmapCollection()->contains(text))
            return loadImage(tag.ownerDocument(), text);
        else {
            m_currentItem->setPixmapName(name.toLatin1(), text);
            return m_currentForm->pixmapCollection()->getPixmap(text);
        }
#endif
        return QPixmap();
    }
    else if (type == "enum") {
        return text;
    }
    else if (type == "set") {
        WidgetWithSubpropertiesInterface* subpropIface
        = dynamic_cast<WidgetWithSubpropertiesInterface*>(obj);
        QObject *subobject = (subpropIface && subpropIface->subwidget())
                             ? subpropIface->subwidget() : obj;
        const QMetaProperty meta(KexiUtils::findPropertyWithSuperclasses(subobject, name.toLatin1().constData()));
        if (meta.isValid()) {
            if (meta.isFlagType()) {
                /*qt4   Q3StrList keys;
                      QStringList list = QStringList::split("|", text);
                      QStringList::ConstIterator it, end( list.constEnd() );
                      for( it = list.constBegin(); it != end; ++it)
                        keys.append((*it).toLatin1());*/
                return meta.enumerator().keysToValue(text.toLatin1());
            } else {
                // Metaproperty not found, probably because subwidget is not created.
                // We will return a string list here with hope that names will
                // be resolved and translated into an integer value later when subwidget is created,
                // e.g. near KexiFormView::updateValuesForSubproperties()
                return text.split("|");
            }
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
    if (item->className() == "Spring") {
        Spring::saveSpring(item, parent, domDoc, insideGridLayout);
        return;
    }

    bool resetCurrentForm = false;
    m_currentItem = item;
    if (!m_currentForm) { // copying widget
        resetCurrentForm = true;
        m_currentForm = item->container() ? item->container()->form() : item->parent()->container()->form();
    }


    WidgetLibrary *lib = m_currentForm->library();
// if(item->container())
//  lib = item->container()->form()->manager()->lib();
// else
//  lib = item->parent()->container()->form()->manager()->lib();

    // We create the "widget" element
    QDomElement tclass = domDoc.createElement("widget");
    parent.appendChild(tclass);

    if (insideGridLayout) {
        tclass.setAttribute("row", item->gridRow());
        tclass.setAttribute("column", item->gridCol());
        if (item->spanMultipleCells()) {
            tclass.setAttribute("rowspan", item->gridRowSpan());
            tclass.setAttribute("colspan", item->gridColSpan());
        }
    }

    if (!item->parent()) // Toplevel widget
        tclass.setAttribute("class", "QWidget");
    // For compatibility, HBox, VBox and Grid are saved as "QLayoutWidget"
    else if (KexiUtils::objectIsA(item->widget(),
                                  QList<QByteArray>() << "HBox" << "VBox" << "Grid" << "HFlow" << "VFlow"))
        tclass.setAttribute("class", "QLayoutWidget");
    else if (KexiUtils::objectIsA(item->widget(), "CustomWidget"))
        tclass.setAttribute("class", item->className());
    else // Normal widgets
        tclass.setAttribute("class", lib->savingName(item->widget()->metaObject()->className()));

    savePropertyValue(tclass, domDoc, "objectName", item->widget()->objectName(), item->widget());

    // Important: save dataSource property FIRST before properties like "alignment"
    // - needed when subproperties are defined after subwidget creation, and subwidget is created after setting "dataSource"
    //   (this is the case for KexiDBAutoField)
//! @todo more properties like "dataSource" may needed here...
// if (-1 != item->widget()->metaObject()->findProperty("dataSource"))
    // savePropertyValue(tclass, domDoc, "dataSource", item->widget()->property("dataSource"), item->widget());

    // We don't want to save the geometry if the widget is inside a layout (so parent.tagName() == "grid" for example)
    if (item && !item->parent()) {
        // save form widget size, but not its position
        savePropertyValue(tclass, domDoc, "geometry",
                          QRect(QPoint(0, 0), item->widget()->size()),
                          item->widget());
    }
    // normal widget (if == "UI', it means we're copying widget)
    else if (parent.tagName() == "widget" || parent.tagName() == "UI")
        savePropertyValue(tclass, domDoc, "geometry", item->widget()->property("geometry"), item->widget());

    // Save the buddy widget for a label
    if (item->widget()->inherits("QLabel") && ((QLabel*)item->widget())->buddy())
        savePropertyElement(
            tclass, domDoc, "property", "buddy", ((QLabel*)item->widget())->buddy()->objectName());

    // We save every property in the modifProp list of the ObjectTreeItem
    QHash<QString, QVariant> hash(*(item->modifiedProperties()));
    foreach (const QString& name, hash.keys()) {
        if (name == "hAlign" || name == "vAlign" || name == "wordbreak" || name == "alignment") {
            if (!savedAlignment) { // not to save it twice
                savePropertyValue(
                    tclass, domDoc, "alignment", 
                    item->widget()->property("alignment"), item->widget());
                savedAlignment = true;
            }
        } else if (name == "objectName" || name == "geometry" || name == "layout") {
            // these have already been saved
        } else {
            savePropertyValue(tclass, domDoc, name.toLatin1(), item->widget()->property(name.toLatin1()),
                              item->widget(), lib);
        }
    }
    hash.clear();

    if (KexiUtils::objectIsA(item->widget(), "CustomWidget")) {
        QDomDocument doc("TEMP");
        doc.setContent(item->m_unknownProps);
        for (QDomNode n = doc.firstChild(); !n.isNull(); n = n.nextSibling()) {
            tclass.appendChild(n.cloneNode());
        }

    }
    // Saving container 's layout if there is one
    QDomElement layout;
    if (item->container() && item->container()->layoutType() != Form::NoLayout) {
        if (item->container()->layout()) { // there is a layout
            layout = domDoc.createElement("temp");
            savePropertyValue(layout, domDoc, "objectName", "unnamed", item->widget());
            if (item->modifiedProperties()->contains("layoutMargin"))
                savePropertyElement(layout, domDoc, "property", "margin", item->container()->layoutMargin());
            if (item->modifiedProperties()->contains("layoutSpacing"))
                savePropertyElement(layout, domDoc, "property", "spacing", item->container()->layoutSpacing());
            tclass.appendChild(layout);
        }
    }

    Form::LayoutType layoutType = item->container() ? item->container()->layoutType() : Form::NoLayout;
    switch (layoutType) {
    case Form::Grid: { // grid layout
        layout.setTagName("grid");
        foreach (ObjectTreeItem *titem, *item->children()) {
            saveWidget(titem, layout, domDoc, true);
        }
        break;
    }
    case Form::HBox:
    case Form::VBox: {
        // as we don't save geometry, we need to sort widgets in the right order, not creation order
        CustomSortableWidgetList *list;
        if (layout.tagName() == "hbox") {
            list = new HorizontalWidgetList(item->container()->form()->toplevelContainer()->widget());
            layout.setTagName("hbox");
        } else {
            list = new HorizontalWidgetList(item->container()->form()->toplevelContainer()->widget());
            layout.setTagName("vbox");
        }

        foreach (ObjectTreeItem *titem, *item->children()) {
            list->append(titem->widget());
        }
        list->sort();

        foreach (QWidget *w, *list) {
            ObjectTreeItem *titem = item->container()->form()->objectTree()->lookup(
                w->objectName()
            );
            if (item)
                saveWidget(titem, layout, domDoc);
        }
        delete list;
        break;
    }
    case Form::HFlow:
    case Form::VFlow: {
#ifdef KEXI_NO_FLOWLAYOUT
#ifdef __GNUC__
#warning "Port Kexi flow layout!"
#endif
#else
        layout.setTagName("grid");
        KexiFlowLayout *flow = static_cast<KexiFlowLayout*>(item->container()->layout());
        if (!flow)
            break;
        QWidgetList *list = (QWidgetList*)flow->widgetList();

        // save some special properties
        savePropertyElement(layout, domDoc, "property", "customLayout",
            Container::layoutTypeToString(item->container()->layoutType()));
        savePropertyElement(layout, domDoc, "property", "justify",
            static_cast<KexiFlowLayout*>(item->container()->layout())->isJustified());

        // fill the widget's grid info, ie just simulate grid layout
        item->container()->createGridLayout(true);
        foreach (QWidget *w, *list) {
            ObjectTreeItem *titem = item->container()->form()->objectTree()->lookup(
                w->name()
            );
            if (item)
                saveWidget(titem, layout, domDoc, true); // save grid info for compatibility with QtDesigner
        }
        delete list;
#endif
        break;
    }
    default: {
        foreach (ObjectTreeItem *titem, *item->children()) {
            saveWidget(titem, tclass, domDoc);
        }
    }
    }

    addIncludeFileName(lib->includeFileName(
                           item->widget()->metaObject()->className()), domDoc);

    if (resetCurrentForm)
        m_currentForm = 0;
    m_currentItem = 0;
}

void
FormIO::cleanClipboard(QDomElement &uiElement)
{
    // remove includehints element not needed
    if (!uiElement.firstChildElement("includehints").isNull())
        uiElement.removeChild(uiElement.firstChildElement("includehints"));
    // and ensure images and connection are at the end
    if (!uiElement.firstChildElement("connections").isNull())
        uiElement.insertAfter(uiElement.firstChildElement("connections"), QDomNode());
    if (!uiElement.firstChildElement("images").isNull())
        uiElement.insertAfter(uiElement.firstChildElement("images"), QDomNode());
}

void
FormIO::loadWidget(Container *container, const QDomElement &el, QWidget *parent)
{
    bool resetCurrentForm = false;
    if (!m_currentForm) { // pasting widget
        resetCurrentForm = true;
        m_currentForm = container->form();
    }

    // We first look for the widget's name
    QString wname;
    for (QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (   n.toElement().tagName() == "property"
            && nameAttribute(n.toElement()) == "name" /* compat with 1.x */)
        {
            wname = n.toElement().text();
            break;
        }
    }

    QWidget *w;
    QByteArray classname, alternate;
    // We translate some name (for compatibility)
    if (el.tagName() == "spacer")
        classname = "Spring";
    else if (el.attribute("class") == "QLayoutWidget") {
        for (QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling())
        {
            QString tagName = n.toElement().tagName();
            if (tagName == "property")
                continue;
            if (tagName == "hbox")
                classname = "HBox";
            else if (tagName == "vbox")
                classname = "VBox";
            else if (tagName == "grid") {
                // first, see if it is flow layout
                for (QDomNode child = n.firstChild(); !child.isNull(); child = child.nextSibling())  {
                    if (    child.toElement().tagName() == "property"
                         && nameAttribute(child.toElement()) == "customLayout")
                    {
                        classname = child.toElement().text().toLatin1();
                        break;
                    }
                }

                if (classname.isEmpty()) // normal grid
                    classname = "Grid";
            }
        }
    } else
        // We check if this classname is an alternate one, and replace it if necessary
    {
        classname = el.attribute("class").toLatin1();
        alternate = container->form()->library()->classNameForAlternate(classname);
    }

    if (alternate == "CustomWidget") {
        w = new CustomWidget(classname, container->widget());
        w->setObjectName(wname);
    } else {
        if (!alternate.isEmpty()) {
            classname = alternate;
        }

        WidgetFactory::CreateWidgetOptions widgetOptions = WidgetFactory::DefaultOptions;
        if (container->form()->mode() != Form::DesignMode) {
            widgetOptions ^= WidgetFactory::DesignViewMode;
        }

        if (!parent)
            w = container->form()->library()->createWidget(classname, container->widget(),
                    wname.toLatin1(), container, widgetOptions);
        else
            w = container->form()->library()->createWidget(classname, parent, wname.toLatin1(),
                    container, widgetOptions);
    }

    if (!w)
        return;
//! @todo allow setting this for data view mode as well
    if (m_currentForm->mode() == Form::DesignMode) {
        //don't generate accelerators for widgets in design mode
        KAcceleratorManager::setNoAccel(w);
    }
    w->setStyle(container->widget()->style());
    w->show();

    // We create and insert the ObjectTreeItem at the good place in the ObjectTree
    ObjectTreeItem *item = container->form()->objectTree()->lookup(wname);
    if (!item)  {
        // not yet created
        item =  new ObjectTreeItem(container->form()->library()->displayName(classname),
                                   wname, w, container);
        if (parent)  {
            ObjectTreeItem *titem = container->form()->objectTree()->lookup(parent->objectName());
            if (titem)
                container->form()->objectTree()->addItem(titem, item);
            else
                kDebug() << "FORMIO :: ERROR no parent widget ";
        } else
            container->form()->objectTree()->addItem(container->objectTree(), item);
    }
    //assign item for its widget if it supports DesignTimeDynamicChildWidgetHandler interface
    //(e.g. KexiDBAutoField)
    if (container->form()->mode() == Form::DesignMode && dynamic_cast<DesignTimeDynamicChildWidgetHandler*>(w)) {
        dynamic_cast<DesignTimeDynamicChildWidgetHandler*>(w)->assignItem(item);
    }

    m_currentItem = item;
    // if we are inside a Grid, we need to insert the widget in the good cell
    if (container->layoutType() == Form::Grid)  {
        QGridLayout *layout = (QGridLayout*)container->layout();
        if (el.hasAttribute("rowspan")) { // widget spans multiple cells
            if (layout) {
                layout->addWidget(
                    w, 
                    el.attribute("row").toInt(), el.attribute("column").toInt(),
                    el.attribute("rowspan").toInt(), el.attribute("colspan").toInt());
//! @todo alignment attribute?
            }
            item->setGridPos(el.attribute("row").toInt(),  el.attribute("column").toInt(), el.attribute("rowspan").toInt(),
                             el.attribute("colspan").toInt());
        } else  {
            if (layout) {
                layout->addWidget(w, el.attribute("row").toInt(), el.attribute("column").toInt());
            }
            item->setGridPos(el.attribute("row").toInt(),  el.attribute("column").toInt(), 0, 0);
        }
    } else if (container->layout())
        container->layout()->add(w);

    readChildNodes(item, container, el, w);

    if (item->container() && item->container()->layout())
        item->container()->layout()->activate();

    // We add the autoSaveProperties in the modifProp list of the ObjectTreeItem, so that they are saved later
    const QList<QByteArray> list(container->form()->library()->autoSaveProperties(w->metaObject()->className()));
    KFormDesigner::WidgetWithSubpropertiesInterface* subpropIface
        = dynamic_cast<KFormDesigner::WidgetWithSubpropertiesInterface*>(w);
    QWidget *subwidget = (subpropIface && subpropIface->subwidget()) ? subpropIface->subwidget() : w;
    foreach (const QByteArray &propName, list) {
        if (-1 != KexiUtils::indexOfPropertyWithSuperclasses(subwidget, propName)) {
            item->addModifiedProperty(propName, subwidget->property(propName));
        }
    }

    if (resetCurrentForm)
        m_currentForm = 0;
    m_currentItem = 0;
}

void
FormIO::createToplevelWidget(Form *form, QWidget *container, QDomElement &el)
{
    // We first look for the widget's name
    QString wname;
    for (QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (    n.toElement().tagName() == "property"
             && nameAttribute(n.toElement()) == "name" /* compat with 1.x */)
        {
            wname = n.toElement().text();
            break;
        }

    }
    // And rename the widget and its ObjectTreeItem
    container->setObjectName(wname);
    if (form->objectTree())
        form->objectTree()->rename(form->objectTree()->name(), wname);
    form->setInteractiveMode(false);

    QHash<QString, QLabel*> *oldBuddies = 0;
    if (m_buddies) // save old buddies (for subforms)
        oldBuddies = m_buddies;
    m_buddies = new QHash<QString, QLabel*>();
    m_currentItem = form->objectTree();

    readChildNodes(form->objectTree(), form->toplevelContainer(), el, container);

    // Now the Form is fully loaded, we can assign the buddies
    for (QHash<QString, QLabel*>::ConstIterator it(m_buddies->constBegin());
        it!=m_buddies->constEnd(); ++it)
    {
        ObjectTreeItem *item = form->objectTree()->lookup(it.key());
        if (!item || !item->widget()) {
            kDebug() << "Cannot assign buddy for widget "
                << it.value()->objectName() << " to " << it.key();
            continue;
        }
        it.value()->setBuddy(item->widget());
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

    for (QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling()) {
        QString tag = n.toElement().tagName();
        QDomElement node = n.toElement();

        if ((tag == "property") || (tag == "attribute")) {
            const QString name( node.attribute("name") );
            const bool isQt3NameProperty = name == QLatin1String("name");
            //if(name == "geometry")
            // hasGeometryProp = true;
            if (   (eltag == "grid" || eltag == "hbox" || eltag == "vbox")
                && (isQt3NameProperty || name == "objectName")
               )
            {
                // we don't care about layout names
                continue;
            }

            if (node.attribute("subwidget") == "true") {
                //this is property for subwidget: remember it for delayed setting
                //because now the subwidget could be not created yet (true e.g. for KexiDBAutoField)
                item->addSubproperty(name.toLatin1(), readPropertyValue(node.firstChild(), w, name));
                const QVariant val(readPropertyValue(node.firstChild(), w, name));
                kDebug() << val.toStringList();
                item->addSubproperty(name.toLatin1(), val);
                //subwidget->setProperty(name.toLatin1(), val);
                item->addModifiedProperty(name.toLatin1(), val);
                continue;
            }

            // We cannot assign the buddy now as the buddy widget may not be created yet
            if (name == "buddy")
                m_buddies->insert(readPropertyValue(node.firstChild(), w, name).toString(), (QLabel*)w);
            else if (    (eltag == "grid" || eltag == "hbox" || eltag == "vbox")
                      && item->container()
                      && item->container()->layout() )
            {
                // We load the margin of a Layout
                if (name == "margin")  {
                    int margin = readPropertyValue(node.firstChild(), w, name).toInt();
                    item->container()->setLayoutMargin(margin);
                    item->container()->layout()->setMargin(margin);
                }
                // We load the spacing of a Layout
                else if (name == "spacing")  {
                    int spacing = readPropertyValue(node.firstChild(), w, name).toInt();
                    item->container()->setLayoutSpacing(spacing);
                    item->container()->layout()->setSpacing(spacing);
                } else if ((name == "justify")) {
#ifdef KEXI_NO_FLOWLAYOUT
#ifdef __GNUC__
#warning "Port Kexi flow layout!"
#endif
#else
                    bool justify = readPropertyValue(node.firstChild(), w, name).toBool();
                    KexiFlowLayout *flow = static_cast<KexiFlowLayout*>(item->container()->layout());
                    if (flow)
                        flow->setJustified(justify);
#endif
                }
            }
            // If the object doesn't have this property, we let the Factory handle it (maybe a special property)
            else if (!isQt3NameProperty && -1 == KexiUtils::indexOfPropertyWithSuperclasses(subwidget, name.toLatin1())) {
                if (w->metaObject()->className() == QString::fromLatin1("CustomWidget"))
                    item->storeUnknownProperty(node);
                else {
                    bool read = container->form()->library()->readSpecialProperty(
                                    w->metaObject()->className(), node, w, item);
                    if (!read) // the factory doesn't support this property neither
                        item->storeUnknownProperty(node);
                }
            }
            else { // we have a normal property, let's load it
                QVariant val(readPropertyValue(node.firstChild(), w, name));
                if (name == "geometry" && dynamic_cast<FormWidget*>(w)) {
                    //fix geometry if needed - this is top level form widget
                    QRect r(val.toRect());
                    if (r.left() < 0) //negative X!
                        r.moveLeft(0);
                    if (r.top() < 0) //negative Y!
                        r.moveTop(0);
                    val = r;
                }
                QByteArray realName;
                if (isQt3NameProperty) {
                    realName = "objectName";
                }
                else {
                    realName = name.toLatin1();
                }
                subwidget->setProperty(realName, val);
//    int count = w->metaObject()->findProperty(name, true);
//    const QMetaProperty *meta = w->metaObject()->property(count, true);
//    if(meta && meta->isEnumType()) {
//     val = w->property(name.toLatin1()); //update: we want a numeric value of enum
//    }
                item->addModifiedProperty(realName, val);
            }
        }
        else if (tag == "widget") { // a child widget
            if (item->container()) // we are a Container
                loadWidget(item->container(), node);
            else
                loadWidget(container, node, w);
        }
        else if (tag == "spacer")  {
            loadWidget(container, node, w);
        }
        else if (tag == "grid") {
            // first, see if it is flow layout
            QString layoutName;
            for (QDomNode child = node.firstChild(); !child.isNull(); child = child.nextSibling())  {
                if (   child.toElement().tagName() == "property"
                    && child.toElement().attribute("name") == "customLayout")
                {
                    layoutName = child.toElement().text();
                    break;
                }
            }

            if (layoutName == "HFlow") {
#ifdef KEXI_NO_FLOWLAYOUT
#ifdef __GNUC__
#warning "Port Kexi flow layout!"
#endif
#else
                item->container()->m_layType = Form::HFlow;
                KexiFlowLayout *layout = new KexiFlowLayout(item->widget());
                layout->setOrientation(Qt::Horizontal);
                item->container()->m_layout = (QLayout*)layout;
#endif
            } else if (layoutName == "VFlow") {
#ifdef KEXI_NO_FLOWLAYOUT
#ifdef __GNUC__
#warning "Port Kexi flow layout!"
#endif
#else
                item->container()->m_layType = Form::VFlow;
                KexiFlowLayout *layout = new KexiFlowLayout(item->widget());
                layout->setOrientation(Qt::Vertical);
                item->container()->m_layout = (QLayout*)layout;
#endif
            } else { // grid layout
                item->container()->m_layType = Form::Grid;
                QGridLayout *layout = new QGridLayout(item->widget());
                item->container()->m_layout = (QLayout*)layout;
            }
            readChildNodes(item, container, node, w);
        } else if (tag == "vbox")  {
            item->container()->m_layType = Form::VBox;
            QVBoxLayout *layout = new QVBoxLayout(item->widget());
            item->container()->m_layout = (QLayout*)layout;
            readChildNodes(item, container, node, w);
        } else if (tag == "hbox") {
            item->container()->m_layType = Form::HBox;
            QHBoxLayout *layout = new QHBoxLayout(item->widget());
            item->container()->m_layout = (QLayout*)layout;
            readChildNodes(item, container, node, w);
        } else {// unknown tag, we let the Factory handle it
            if (w->metaObject()->className() == QString::fromLatin1("CustomWidget"))
                item->storeUnknownProperty(node);
            else {
                bool read = container->form()->library()->readSpecialProperty(
                                w->metaObject()->className(), node, w, item);
                if (!read) // the factory doesn't suport this property neither
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
    if (include.isEmpty())
        return;

    QDomElement includes;
    QDomElement uiEl = domDoc.firstChildElement("UI");
    if (uiEl.firstChildElement("includehints").isNull()) {
        includes = domDoc.createElement("includehints");
        uiEl.appendChild(includes);
    } else {
        includes = uiEl.firstChildElement("includehints");
    }

    // Check if this include has already been saved, and return if it is the case
    for (QDomNode n = includes.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.toElement().text() == include)
            return;
    }

    QDomElement includeHint = domDoc.createElement("includehint");
    includes.appendChild(includeHint);
    QDomText includeText = domDoc.createTextNode(include);
    includeHint.appendChild(includeText);
}

QString
FormIO::saveImage(QDomDocument &domDoc, const QPixmap &pixmap)
{
    QDomElement images = domDoc.firstChildElement("images");
    if (images.isNull()) {
        images = domDoc.createElement("images");
        QDomElement ui = domDoc.firstChildElement("UI");
        ui.appendChild(images);
    }

    int count = images.childNodes().count();
    QDomElement image = domDoc.createElement("image");
    QString name = "image" + QString::number(count);
    image.setAttribute("name", name);

    const QImage img(pixmap.toImage());
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly | QIODevice::Text);
    const QByteArray format(img.depth() > 1 ? "XPM" : "XBM");
    QImageWriter imageWriter(&buf, format);
    imageWriter.write(img);
    buf.close();
    const QByteArray bazip = qCompress(ba);
    const int len = bazip.size();

    QDomElement data = domDoc.createElement("data");
    data.setAttribute("format", QString(format + ".GZ"));
    data.setAttribute("length", ba.size());

    static const char hexchars[] = "0123456789abcdef";
    QString content;
    for (int i = 4; i < len; i++) {
        uchar s = (uchar) bazip[i];
        content += hexchars[s >> 4];
        content += hexchars[s & 0x0f];
    }

    data.appendChild(domDoc.createTextNode(content));
    image.appendChild(data);
    images.appendChild(image);

    return name;
}

QPixmap
FormIO::loadImage(QDomDocument domDoc, const QString& name)
{
    QDomElement images = domDoc.firstChildElement("UI").firstChildElement("images");
    if (images.isNull())
        return 0;

    QDomElement image;
    for (QDomNode n = images.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if ((n.toElement().tagName() == "image") && (n.toElement().attribute("name") == name)) {
            image = n.toElement();
            break;
        }
    }

    QPixmap pix;
    QString data(image.firstChildElement("data").text());
    const int lengthOffset = 4;
    int baSize = data.length() / 2 + lengthOffset;
    uchar *ba = new uchar[baSize];
    for (int i = lengthOffset; i < baSize; ++i) {
        char h = data[2 * (i-lengthOffset)].toLatin1();
        char l = data[2 * (i-lengthOffset) + 1].toLatin1();
        uchar r = 0;
        if (h <= '9')
            r += h - '0';
        else
            r += h - 'a' + 10;
        r = r << 4;
        if (l <= '9')
            r += l - '0';
        else
            r += l - 'a' + 10;
        ba[i] = r;
    }

    QString format = image.firstChildElement("data").attribute("format", "PNG");
    if ((format == "XPM.GZ") || (format == "XBM.GZ")) {
        int len = image.attribute("length").toInt();
        if (len < data.length() * 5)
            len = data.length() * 5;
        // qUncompress() expects the first 4 bytes to be the expected length of
        // the uncompressed data
        ba[0] = (len & 0xff000000) >> 24;
        ba[1] = (len & 0x00ff0000) >> 16;
        ba[2] = (len & 0x0000ff00) >> 8;
        ba[3] = (len & 0x000000ff);
        QByteArray baunzip = qUncompress(ba, baSize);
        pix.loadFromData((const uchar*)baunzip.data(), baunzip.size(), format.left(format.indexOf('.')).toLatin1());
    } else
        pix.loadFromData((const uchar*)ba + lengthOffset, baSize - lengthOffset, format.toLatin1());

    delete[] ba;

    return pix;
}

#include "formIO.moc"
