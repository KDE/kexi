/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2006 Jaroslaw Staniek <js@iidea.pl>

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

#include "widgetfactory.h"

#include <qcursor.h>
#include <qobjectlist.h>
#include <qdict.h>
#include <qmetaobject.h>

#include <kdebug.h>
#include <klocale.h>
//#ifdef KEXI_KTEXTEDIT
#include <ktextedit.h>
//#else
#include <klineedit.h>
//#endif
#include <kdialogbase.h>
#include <keditlistbox.h>
#include <kxmlguiclient.h>
#include <kactioncollection.h>

#include "richtextdialog.h"
#include "editlistviewdialog.h"
#include "resizehandle.h"
#include "formmanager.h"
#include "form.h"
#include "container.h"
#include "objecttree.h"
#include "widgetlibrary.h"
#include "utils.h"
#include "widgetpropertyset.h"
#include "widgetwithsubpropertiesinterface.h"
#include <koproperty/property.h>

using namespace KFormDesigner;

///// Widget Info //////////////////////////

WidgetInfo::WidgetInfo(WidgetFactory *f)
 : m_inheritedClass(0)
 , m_overriddenAlternateNames(0)
 , m_factory(f)
 , m_propertiesWithDisabledAutoSync(0)
 , m_customTypesForProperty(0)
{
}

WidgetInfo::WidgetInfo(WidgetFactory *f, const char* parentFactoryName,
	const char* inheritedClassName)
 : m_parentFactoryName( QCString("kformdesigner_")+parentFactoryName )
 , m_inheritedClassName(inheritedClassName)
 , m_inheritedClass(0)
 , m_overriddenAlternateNames(0)
 , m_factory(f)
 , m_propertiesWithDisabledAutoSync(0)
 , m_customTypesForProperty(0)
{
	m_class = inheritedClassName;
}

WidgetInfo::~WidgetInfo()
{
	delete m_overriddenAlternateNames;
	delete m_propertiesWithDisabledAutoSync;
	delete m_customTypesForProperty;
}

void WidgetInfo::addAlternateClassName(const QCString& alternateName, bool override)
{
	m_alternateNames += alternateName;
	if (override) {
		if (!m_overriddenAlternateNames)
			m_overriddenAlternateNames = new QAsciiDict<char>(101);
		m_overriddenAlternateNames->insert(alternateName, (char*)1);
	}
	else {
		if (m_overriddenAlternateNames)
			m_overriddenAlternateNames->take(alternateName);
	}
}

bool WidgetInfo::isOverriddenClassName(const QCString& alternateName) const
{
	return m_overriddenAlternateNames && (m_overriddenAlternateNames->find(alternateName) != 0);
}

void WidgetInfo::setAutoSyncForProperty(const char *propertyName, tristate flag)
{
	if (!m_propertiesWithDisabledAutoSync) {
		if (~flag)
			return;
		m_propertiesWithDisabledAutoSync = new QAsciiDict<char>(101);
	}

	if (~flag) {
		m_propertiesWithDisabledAutoSync->remove(propertyName);
	}
	else {
		m_propertiesWithDisabledAutoSync->insert(propertyName, flag==true ? (char*)1 : (char*)2);
	}
}

tristate WidgetInfo::autoSyncForProperty(const char *propertyName) const
{
	char* flag = m_propertiesWithDisabledAutoSync ? m_propertiesWithDisabledAutoSync->find(propertyName) : 0;
	if (!flag)
		return cancelled;
	return flag==(char*)1 ? true : false;
}

void WidgetInfo::setCustomTypeForProperty(const char *propertyName, int type)
{
	if (!propertyName || type==KoProperty::Auto)
		return;
	if (!m_customTypesForProperty) {
		m_customTypesForProperty = new QMap<QCString,int>();
	}
	m_customTypesForProperty->replace(propertyName, type);
}

int WidgetInfo::customTypeForProperty(const char *propertyName) const
{
	if (!m_customTypesForProperty || !m_customTypesForProperty->contains(propertyName))
		return KoProperty::Auto;
	return (*m_customTypesForProperty)[propertyName];
}


///// Widget Factory //////////////////////////

WidgetFactory::WidgetFactory(QObject *parent, const char *name)
 : QObject(parent, (const char*)(QCString("kformdesigner_")+name))
{
	m_showAdvancedProperties = true;
	m_classesByName.setAutoDelete(true);
	m_hiddenClasses = 0;
	m_guiClient = 0;
}

WidgetFactory::~WidgetFactory()
{
	delete m_hiddenClasses;
}

void WidgetFactory::addClass(WidgetInfo *w)
{
	WidgetInfo *oldw = m_classesByName[w->className()];
	if (oldw==w)
		return;
	if (oldw) {
		kdWarning() << "WidgetFactory::addClass(): class with name '" << w->className()
			<< "' already exists for factory '" << name() << "'" << endl;
		return;
	}
	m_classesByName.insert( w->className(), w );
}

void WidgetFactory::hideClass(const char *classname)
{
	if (!m_hiddenClasses)
		m_hiddenClasses = new QAsciiDict<char>(101, false);
	m_hiddenClasses->insert(classname, (char*)1);
}

void
WidgetFactory::createEditor(const QCString &classname, const QString &text,
	QWidget *w, Container *container, QRect geometry,
	int align, bool useFrame, bool multiLine, BackgroundMode background)
{
//#ifdef KEXI_KTEXTEDIT
	if (multiLine) {
		KTextEdit *textedit = new KTextEdit(text, QString::null, w->parentWidget());
		textedit->setTextFormat(Qt::PlainText);
		textedit->setAlignment(align);
		if (dynamic_cast<QTextEdit*>(w)) {
			textedit->setWordWrap(dynamic_cast<QTextEdit*>(w)->wordWrap());
			textedit->setWrapPolicy(dynamic_cast<QTextEdit*>(w)->wrapPolicy());
		}
		textedit->setPalette(w->palette());
		textedit->setFont(w->font());
		textedit->setResizePolicy(QScrollView::Manual);
		textedit->setGeometry(geometry);
		if(background == Qt::NoBackground)
			textedit->setBackgroundMode(w->backgroundMode());
		else
			textedit->setBackgroundMode(background);
//		textedit->setPaletteBackgroundColor(textedit->colorGroup().color( QColorGroup::Base ));
		textedit->setPaletteBackgroundColor(w->paletteBackgroundColor());
		for(int i =0; i <= textedit->paragraphs(); i++)
			textedit->setParagraphBackgroundColor(i, w->paletteBackgroundColor());
		textedit->selectAll(true);
		textedit->setColor(w->paletteForegroundColor());
		textedit->selectAll(false);
		textedit->moveCursor(QTextEdit::MoveEnd, false);
		textedit->setParagraphBackgroundColor(0, w->paletteBackgroundColor());
		textedit->setVScrollBarMode(QScrollView::AlwaysOff); //ok?
		textedit->setHScrollBarMode(QScrollView::AlwaysOff); //ok?
		textedit->installEventFilter(this);
		textedit->setFrameShape(useFrame ? QFrame::LineEditPanel : QFrame::NoFrame);
		textedit->setMargin(2); //to move away from resize handle
		textedit->show();
		textedit->setFocus();
		textedit->selectAll();
		setEditor(w, textedit);

		connect(textedit, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));
		connect(w, SIGNAL(destroyed()), this, SLOT(widgetDestroyed()));
		connect(textedit, SIGNAL(destroyed()), this, SLOT(editorDeleted()));
//#else
	}
	else {
		KLineEdit *editor = new KLineEdit(text, w->parentWidget());
		editor->setAlignment(align);
		editor->setPalette(w->palette());
		editor->setFont(w->font());
		editor->setGeometry(geometry);
		if(background == Qt::NoBackground)
			editor->setBackgroundMode(w->backgroundMode());
		else
			editor->setBackgroundMode(background);
		editor->installEventFilter(this);
		editor->setFrame(useFrame);
		editor->setMargin(2); //to move away from resize handle
		editor->show();
		editor->setFocus();
		editor->selectAll();
		connect(editor, SIGNAL(textChanged(const QString&)), this, SLOT(changeTextInternal(const QString&)));
		connect(w, SIGNAL(destroyed()), this, SLOT(widgetDestroyed()));
		connect(editor, SIGNAL(destroyed()), this, SLOT(editorDeleted()));

		setEditor(w, editor);
//		m_editor = editor;
	}
	//copy properties if available
	WidgetWithSubpropertiesInterface* subpropIface = dynamic_cast<WidgetWithSubpropertiesInterface*>(w);
	QWidget *subwidget = (subpropIface && subpropIface->subwidget()) ? subpropIface->subwidget() : w;
	if (-1!=m_editor->metaObject()->findProperty("margin", true) && -1!=subwidget->metaObject()->findProperty("margin", true))
		m_editor->setProperty("margin", subwidget->property("margin"));
//#endif
//js	m_handles = new ResizeHandleSet(w, container->form(), true);
	m_handles = container->form()->resizeHandlesForWidget(w);
	if (m_handles) {
		m_handles->setEditingMode(true);
		m_handles->raise();
	}

	ObjectTreeItem *tree = container->form()->objectTree()->lookup(w->name());
	if(!tree)
		return;
	tree->eventEater()->setContainer(this);

	//m_widget = w;
	setWidget(w, container);
	m_editedWidgetClass = classname;
	m_firstText = text;
//	m_container = container;

	changeTextInternal(text); // to update size of the widget
}

void
WidgetFactory::disableFilter(QWidget *w, Container *container)
{
	ObjectTreeItem *tree = container->form()->objectTree()->lookup(w->name());
	if(!tree)
		return;
	tree->eventEater()->setContainer(this);

	w->setFocus();
//js	m_handles = new ResizeHandleSet(w, container->form(), true);
	m_handles = container->form()->resizeHandlesForWidget(w);
	if (m_handles) {
		m_handles->setEditingMode(true);
		m_handles->raise();
	}

	//m_widget = w;
	setWidget(w, container);
//	m_container = container;
	setEditor(w, 0);
//	m_editor = 0;

	// widget is disabled, so we re-enable it while editing
	if(!tree->isEnabled()) {
		QPalette p = w->palette();
		QColorGroup cg = p.active();
		p.setActive(p.disabled());
		p.setDisabled(cg);
		w->setPalette(p);
	}

	connect(w, SIGNAL(destroyed()), this, SLOT(widgetDestroyed()));
}

bool
WidgetFactory::editList(QWidget *w, QStringList &list)
{
	KDialogBase dialog(w->topLevelWidget(), "stringlist_dialog", true, i18n("Edit List of Items"),
	    KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, false);

	KEditListBox *edit = new KEditListBox(i18n("Contents of %1").arg(w->name()), &dialog, "editlist");
	dialog.setMainWidget(edit);
	edit->insertStringList(list);
//	edit->show();

	if(dialog.exec() == QDialog::Accepted)
	{
		list = edit->items();
		return true;
	}
	return false;
}

bool
WidgetFactory::editRichText(QWidget *w, QString &text)
{
	RichTextDialog dlg(w, text);
	if(dlg.exec()== QDialog::Accepted)
	{
		text = dlg.text();
		return true;
	}
	return false;
}

void
WidgetFactory::editListView(QListView *listview)
{
	EditListViewDialog dlg(((QWidget*)listview)->topLevelWidget());
	//dlg.exec(listview);
}

bool
WidgetFactory::eventFilter(QObject *obj, QEvent *ev)
{
	if( ((ev->type() == QEvent::Resize) || (ev->type() == QEvent::Move) ) && (obj == m_widget) && editor(m_widget)) {
		// resize widget using resize handles
		QWidget *ed = editor(m_widget);
		resizeEditor(ed, m_widget, m_widget->className());
	}
	else if((ev->type() == QEvent::Paint) && (obj == m_widget) && editor(m_widget)) {
		// paint event for container edited (eg button group)
		return m_container->eventFilter(obj, ev);
	}
	else if((ev->type() == QEvent::MouseButtonPress) && (obj == m_widget) && editor(m_widget)) {
		// click outside editor --> cancel editing
		Container *cont = m_container;
		resetEditor();
		return cont->eventFilter(obj, ev);
	}

	if(ev->type() == QEvent::FocusOut)
	{
		QWidget *w = editor(m_widget);
		if (!w)
			w = (QWidget *)m_widget;
		if(obj != (QObject *)w)
			return false;

		QWidget *focus = w->topLevelWidget()->focusWidget();
		if(focus && w != focus && !w->child(focus->name(), focus->className()))
			resetEditor();
	}
	else if(ev->type() == QEvent::KeyPress)
	{
		QWidget *w = editor(m_widget);
		if (!w)
			w = (QWidget *)m_widget;
		if(obj != (QObject *)w)
			return false;

		QKeyEvent *e = static_cast<QKeyEvent*>(ev);
		if(((e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter)) && (e->state() != AltButton))
			resetEditor();
		if(e->key() == Qt::Key_Escape)
		{
			setEditorText(m_firstText);
			//changeText(m_firstText);
			resetEditor();
		}
	}
	else if(ev->type() == QEvent::ContextMenu) {
		QWidget *w = editor(m_widget);
		if (!w)
			w = (QWidget *)m_widget;
		if(obj != (QObject *)w)
			return false;

		return true;
	}
//	if(obj == m_widget)
//		return m_container->eventFilter(obj, ev);
//	else
	return false;
}

void
WidgetFactory::resetEditor()
{
	if (m_container)
		m_container->stopInlineEditing();

	QWidget *ed = editor(m_widget);
	if(m_widget)
	{
		ObjectTreeItem *tree = m_container ? m_container->form()->objectTree()->lookup(m_widget->name()) : 0;
		if(!tree)
		{
			kdDebug() << "WidgetFactory::resetEditor() : error cannot found a tree item " << endl;
			return;
		}
		tree->eventEater()->setContainer(m_container);
		if(m_widget) {// && !ed)
			setRecursiveCursor(m_widget, m_container->form());
			if (m_widget->inherits("QLineEdit") || m_widget->inherits("QTextEdit")) { //fix weird behaviour
				m_widget->unsetCursor();
				m_widget->setCursor(Qt::ArrowCursor);
			}
		}

		// disable again the widget
		if(!ed && !tree->isEnabled()) {
			QPalette p = m_widget->palette();
			QColorGroup cg = p.active();
			p.setActive(p.disabled());
			p.setDisabled(cg);
			m_widget->setPalette(p);
		}
	}
	if(ed)
	{
		changeTextInternal(editorText());
		disconnect(ed, 0, this, 0);
		ed->deleteLater();
	}

	if(m_widget)
	{
		disconnect(m_widget, 0, this, 0);
		m_widget->repaint();
	}

//js	delete m_handles;
	if (m_handles) {
		m_handles->setEditingMode(false);
	}
	setEditor(m_widget, 0);
//	m_editor = 0;
	setWidget(0, 0);
	//m_widget = 0;
	m_handles = 0;
//	m_container = 0;
}

void
WidgetFactory::widgetDestroyed()
{
	if(m_editor)
	{
		m_editor->deleteLater();
		m_editor = 0;
	}

//js	delete m_handles;
	if (m_handles) {
		m_handles->setEditingMode(false);

	}
	m_widget = 0;
	m_handles = 0;
	m_container = 0;
}

void
WidgetFactory::editorDeleted()
{
//js	delete m_handles;
	if (m_handles) {
		m_handles->setEditingMode(false);
	}
	setEditor(m_widget, 0);
	setWidget(0, 0);
//	m_widget = 0;
	m_handles = 0;
//	m_container = 0;
}

void
WidgetFactory::changeProperty(const char *name, const QVariant &value, Form *form)
//WidgetFactory::changeProperty(const char *name, const QVariant &value, Container *container)
{
//	if (!form->manager())
//		return;
	if(form->selectedWidgets()->count() > 1)
	{ // If eg multiple labels are selected, we only want to change the text of one of them (the one the user cliked on)
		if(m_widget)
			m_widget->setProperty(name, value);
		else
			form->selectedWidgets()->first()->setProperty(name, value);
	}
	else
	{
		WidgetPropertySet *set = KFormDesigner::FormManager::self()->propertySet();
		if(set->contains(name))
			(*set)[name] = value;
	}
}

/*
void
WidgetFactory::addPropertyDescription(Container *container, const char *prop, const QString &desc)
{
	WidgetPropertySet *buff = container->form()->manager()->buffer();
	buff->addPropertyDescription(prop, desc);
}

void
WidgetFactory::addValueDescription(Container *container, const char *value, const QString &desc)
{
	WidgetPropertySet *buff = container->form()->manager()->buffer();
	buff->addValueDescription(value, desc);
}*/

bool
WidgetFactory::isPropertyVisible(const QCString &classname, QWidget *w, 
	const QCString &property, bool multiple, bool isTopLevel)
{
	if (multiple)
	{
		return property=="font" || property=="paletteBackgroundColor" || property=="enabled" 
			|| property=="paletteForegroundColor" || property=="cursor" || property=="paletteBackgroundPixmap";
	}

//	if(d->properties.isEmpty() && !isTopLevel)
//		d->properties << "caption" << "icon" << "sizeIncrement" << "iconText";
//	if(! (d->properties.grep(property)).isEmpty() )
//		return false;

	return isPropertyVisibleInternal(classname, w, property, isTopLevel);
//	return !multiple && isPropertyVisibleInternal(classname, w, property);
}

bool
WidgetFactory::isPropertyVisibleInternal(const QCString &, QWidget *w,
	const QCString &property, bool isTopLevel)
{
	Q_UNUSED( w );

#ifdef KEXI_NO_CURSOR_PROPERTY
//! @todo temporary unless cursor works properly in the Designer
	if (property=="cursor")
		return false;
#endif

	if (!isTopLevel 
		&& (property=="caption" || property=="icon" || property=="sizeIncrement" || property=="iconText")) {
		// don't show these properties for a non-toplevel widget
		return false;
	}
	return true;
}

bool
WidgetFactory::propertySetShouldBeReloadedAfterPropertyChange(const QCString& classname, QWidget *w, 
	const QCString& property)
{
	Q_UNUSED(classname);
	Q_UNUSED(w);
	Q_UNUSED(property);
	return false;
}

void
WidgetFactory::resizeEditor(QWidget *, QWidget *, const QCString&)
{
}

void
WidgetFactory::slotTextChanged()
{
	changeTextInternal(editorText());
}

bool
WidgetFactory::clearWidgetContent(const QCString &, QWidget *)
{
	return false;
}

void
WidgetFactory::changeTextInternal(const QString& text)
{
	if (changeText( text ))
		return;
	//try in inherited
	if (!m_editedWidgetClass.isEmpty()) {
		WidgetInfo *wi = m_classesByName[ m_editedWidgetClass ];
		if (wi && wi->inheritedClass()) {
//			wi->inheritedClass()->factory()->m_container = m_container;
			wi->inheritedClass()->factory()->changeText( text );
		}
	}
}

bool
WidgetFactory::changeText(const QString& text)
{
	changeProperty( "text", text, m_container->form() );
	return true;
}

bool
WidgetFactory::readSpecialProperty(const QCString &, QDomElement &, QWidget *, ObjectTreeItem *)
{
	return false;
}

bool
WidgetFactory::saveSpecialProperty(const QCString &, const QString &, const QVariant&, QWidget *, QDomElement &,  QDomDocument &)
{
	return false;
}

bool WidgetFactory::inheritsFactories()
{
	for (QAsciiDictIterator<WidgetInfo> it(m_classesByName); it.current(); ++it) {
		if (!it.current()->parentFactoryName().isEmpty())
			return true;
	}
	return false;
}

QString WidgetFactory::editorText() const {
	QWidget *ed = editor(m_widget);
	return dynamic_cast<KTextEdit*>(ed) ? dynamic_cast<KTextEdit*>(ed)->text() : dynamic_cast<KLineEdit*>(ed)->text();
}

void WidgetFactory::setEditorText(const QString& text) {
	QWidget *ed = editor(m_widget);
	if (dynamic_cast<KTextEdit*>(ed))
		dynamic_cast<KTextEdit*>(ed)->setText(text);
	else
		dynamic_cast<KLineEdit*>(ed)->setText(text);
}

void WidgetFactory::setEditor(QWidget *widget, QWidget *editor)
{
	if (!widget)
		return;
	WidgetInfo *winfo = m_classesByName[widget->className()];
	if (!winfo || winfo->parentFactoryName().isEmpty()) {
		m_editor = editor;
	}
	else {
		WidgetFactory *f = m_library->factory(winfo->parentFactoryName());
		if (f!=this)
			f->setEditor(widget, editor);
		m_editor = editor; //keep a copy
	}
}

QWidget *WidgetFactory::editor(QWidget *widget) const
{
	if (!widget)
		return 0;
	WidgetInfo *winfo = m_classesByName[widget->className()];
	if (!winfo || winfo->parentFactoryName().isEmpty()) {
		return m_editor;
	}
	else {
		WidgetFactory *f = m_library->factoryForClassName(widget->className());
		if (f!=this)
			return f->editor(widget);
		return m_editor;
	}
}

void WidgetFactory::setWidget(QWidget *widget, Container* container)
{
	WidgetInfo *winfo = widget ? m_classesByName[widget->className()] : 0;
	if (winfo && !winfo->parentFactoryName().isEmpty()) {
		WidgetFactory *f = m_library->factory(winfo->parentFactoryName());
		if (f!=this)
			f->setWidget(widget, container);
	}
	m_widget = widget; //keep a copy
	m_container = container;
}

QWidget *WidgetFactory::widget() const
{
	return m_widget;
}

void WidgetFactory::setInternalProperty(const QCString& classname, const QCString& property,
	const QString& value)
{
	m_internalProp[classname+":"+property]=value;
}

void WidgetFactory::setPropertyOptions( WidgetPropertySet& /*buf*/, const WidgetInfo& /*info*/, QWidget * /*w*/ )
{
	//nothing
}

#include "widgetfactory.moc"
