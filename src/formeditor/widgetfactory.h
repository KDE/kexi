/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2009 Jarosław Staniek <staniek@kde.org>

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

#ifndef KFORMDESIGNERWIDGETFACTORY_H
#define KFORMDESIGNERWIDGETFACTORY_H

#include <QObject>
#include <QPointer>
#include <QPixmap>
#include <QHash>

#include <kexi_export.h>
#include <kexiutils/tristate.h>

//! Disable list widget because we need to replace it with QTreeWidget 
//! which uses very different API compared to Q3ListView.
//! @todo re-add QTreeWidget
#define KEXI_FORMS_NO_LIST_WIDGET

class QWidget;
class QMenu;
class QDomElement;
class QDomDocument;
class QVariant;
class KActionCollection;
class KXMLGUIClient;

namespace KoProperty
{
class Set;
}

namespace KFormDesigner
{

class WidgetFactory;
class WidgetLibrary;
class Container;
class ResizeHandleSet;
class ObjectTreeItem;
class Form;

/**
 * This class holds properties of widget classes provided by a factory.
 */
class KFORMEDITOR_EXPORT WidgetInfo
{
public:
    typedef QList<WidgetInfo*> List;
    typedef QHash<QByteArray, WidgetInfo*> Hash;

    WidgetInfo(WidgetFactory *f);

    WidgetInfo(WidgetFactory *f, const char* parentFactoryName, const char* inheritedClassName = 0);

    virtual ~WidgetInfo();

    //! \return a pixmap associated with the widget
    QString pixmap() const {
        return m_pixmap;
    }

    //! \return the class name of a widget e.g. 'QLineEdit'
    QByteArray className() const {
        return m_class;
    }

    /*! \return the name used to name widget, that will appear eg in scripts (must not contain spaces
      nor non-latin1 characters) */
    QString namePrefix() const {
        return m_prefixName;
    }

    //! \return the real name e.g. 'Line Edit', showed eg in ObjectTreeView
    QString name() const {
        return m_name;
    }

    QString description() const {
        return m_desc;
    }
    QString includeFileName() const {
        return m_include;
    }
    QList<QByteArray> alternateClassNames() const {
        return m_alternateNames;
    }
    QString savingName() const {
        return m_saveName;
    }
    WidgetFactory *factory() const {
        return m_factory;
    }

    void setPixmap(const QString &p) {
        m_pixmap = p;
    }
    void setClassName(const QByteArray &s) {
        m_class = s;
    }
    void setName(const QString &n) {
        m_name = n;
    }
    void setNamePrefix(const QString &n) {
        m_prefixName = n;
    }
    void setDescription(const QString &desc) {
        m_desc = desc;
    }

    /*! Sets the C++ include file corresponding to this class,
     that uic will need to add when creating the file. You don't have to set this for Qt std widgets.*/
    void setIncludeFileName(const QString &name) {
        m_include = name;
    }

    /*! Sets alternate names for this class.
     If this name is found when loading a .ui file, the className() will be used instead.
     It allows to support both KDE and Qt versions of widget, without duplicating code.
     As a rule, className() should always return a class name which is inherited from
     alternate class. For example KLineEdit class has alternate QLineEdit class.

     \a override parameter overrides class name of a widget,
     even if it was implemented in other factory.
     By default it's set to false, what means that no other class is overridden
     by this widget class if there is already a class implementing it
     (no matter in which factory).
     By forced overriding existing class with other - custom, user
     will be able to see more or less properties and experience different behaviour.
     For example, in Kexi application, KLineEdit class contains additional
     "datasource" property for binding to database sources.
    */
    void addAlternateClassName(const QByteArray& alternateName, bool override = false);

    /*! \return true is a class \a alternateName is defined as alternate name with
     'override' flag set to true, using addAlternateClassName().
     If this flag is set to false (the default) or there's no such alternate class
     name defined. */
    bool isOverriddenClassName(const QByteArray& alternateName) const;

    /*! Sets the name that will be written in the .ui file when saving.
     This name must be one of alternate names (or loading will be impossible).

     On form data saving to XML .ui format, saveName is used instead,
     so .ui format is not broken and still usable with other software as Qt Designer.
     Custom properties are saved as well with 'stdset' attribute set to 0. */
    void setSavingName(const QString &saveName) {
        m_saveName = saveName;
    }

    /*! Sets autoSync flag for property \a propertyName.
     This allows to override autoSync flag for certain widget's property, because
     e.g. KoProperty::EditorView can have autoSync flag set to false or true, but
     not all properties have to comply with that.
     \a flag equal to cancelled value means there is no overriding (the default). */
    void setAutoSyncForProperty(const char *propertyName, tristate flag);

    /*! \return autoSync override value (true or false) for \a propertyName.
     If cancelled value is returned, there is no overriding (the default). */
    tristate autoSyncForProperty(const char *propertyName) const;

    QByteArray parentFactoryName() const {
        return m_parentFactoryName;
    }

    WidgetInfo* inheritedClass() const {
        return m_inheritedClass;
    }

    /*! Sets custom type \a type for property \a propertyName.
     This allows to override default type, especially when custom property
     and custom property editor item has to be used. */
    void setCustomTypeForProperty(const char *propertyName, int type);

    /*! \return custom type for property \a propertyName. If no specific custom type has been assigned,
     KoProperty::Auto is returned.
     @see setCustomTypeForProperty() */
    int customTypeForProperty(const char *propertyName) const;

protected:
    QByteArray m_parentFactoryName, m_inheritedClassName; //!< Used for inheriting widgets between factories
    WidgetInfo* m_inheritedClass;

private:
    QString m_pixmap;
    QByteArray m_class;
    QString m_name;
    QString m_prefixName;
    QString m_desc;
    QString m_include;
    QList<QByteArray> m_alternateNames;
    QSet<QByteArray> *m_overriddenAlternateNames;
    QString m_saveName;
    QPointer<WidgetFactory> m_factory;
    QHash<QByteArray, tristate> *m_propertiesWithDisabledAutoSync;
    QHash<QByteArray, int> *m_customTypesForProperty;

    friend class WidgetLibrary;
};

//! The base class for all widget Factories
/*! This is the class you need to inherit to create a new Factory. There are few
 virtuals you need to implement, and some other functions
 to implement if you want more features.\n \n

  <b>Widget Creation</b>\n
  To be able to create widgets, you need to implement the create() function, an classes(),
  which should return all the widgets supported by this factory.\n \n

  <b>GUI Integration</b>\n
  The following functions allow you to customize even more the look-n-feel of your widgets inside KFormDesigner.
  You can use createMenuActions() to add custom items in widget's context menu. The previewWidget()
  is called when the Form gets in Preview mode, and you have a last opportunity to remove all editing-related
  stuff (see eg \ref Spring class).\n
  You can also choose which properties to show in the Property Editor.
  By default, most all properties are shown (see implementation for details),
  but you can hide some reimplementing isPropertyVisibleInternal() (don't forget to call superclass' method)
  To add new properties, just define new Q_PROPERTY in widget class definition.\n \n

  <b>Inline editing</b>\n
  KFormDesigner allow you to edit the widget's contents inside Form, without using a dialog.
  You can of course customize the behaviour of your widgets, using startInlineEditing(). There are some editing
  modes already implemented in WidgetFactroy, but you can create your own if you want:
  \li Editing using a line edit (createInlineEditor()): a line edit is created on top of widget,
  where the user inputs text. As the text changes, changeInlineText() is called
  (where you should set your widget's text and resize widget to fit the text if needed) and resizeEditor()
  to update editor's position when widget is moved/resized.\n
  \li Editing by disabling event filter: if you call disableFilter(), the event filter
   on the object is temporarily disabled, so the widget behaves as usual. This
  can be used for more complex widgets, such as spinbox, date/time edit, etc.
  \li Other modes: there are 3 other modes, to edit a string list: editList()
  (for combo box, listbox), to edit rich text: editRichText() (for labels, etc.)\n \n

  <b>Widget saving/loading</b>\n
  You can also control how your widget are saved/loaded. You can choose which properties to save
   (see autoSaveProperties()), and save/load custom properties, ie
  properties that are not Q_PROPERTY but you want to save in the UI file. This is used eg to
   save combo box or list widget contents (see saveSpecialProperty() and
  readSpecialProperty()). \n \n

  <b>Special internal properties</b>\n
  Use void setInternalProperty(const QByteArray& classname, const QByteArray& property, const QString& value);
  to set values of special internal properties.
  Currently these properties are used for customizing popup menu items used for orientation selection.
  Customization for class ClassName should look like:
  <code> void setInternalProperty("ClassName", "orientationSelectionPopup", "myicon"); </code>
  Available internal properties:
  * "orientationSelectionPopup" - set it to "1" if you want a given class to offer orientation selection,
     so orientation selection popup will be displayed when needed.
  * "orientationSelectionPopup:horizontalIcon" - sets a name of icon for "Horizontal" item
    for objects of class 'ClassName'. Set this property only for classes supporting orientations.
  * "orientationSelectionPopup:verticalIcon" - the same for "Vertical" item.
    Set this property only for classes supporting orientations.
  * "orientationSelectionPopup:horizontalText" - sets a i18n'd text for "Horizontal" item
    for objects of class 'ClassName', e.g. i18n("Insert Horizontal Line").
    Set this property only for classes supporting orientations.
  * "orientationSelectionPopup:verticalText" - the same for "Vertical" item,
    e.g. i18n("Insert Vertical Line"). Set this property only for classes supporting orientations.
  * "dontStartEditingOnInserting" - if not empty, WidgetFactory::startInlineEditing() will not be executed upon
    widget inseting by a user.
  * "forceShowAdvancedProperty:{propertyname}" - set it to "1" for "{propertyname}" advanced property
    if you want to force it to be visible even if WidgetLibrary::setAdvancedPropertiesVisible(false)
    has been called. For example, setting "forceShowAdvancedProperty:pixmap" to "1"
    unhides "pixmap" property for a given class.

  See StdWidgetFactory::StdWidgetFactory() for properties like
  "Line:orientationSelectionPopup:horizontalIcon".

  \n\n
  See the standard factories in formeditor/factories for an example of factories,
  and how to deal with complex widgets (eg tabwidget).
  */
class KFORMEDITOR_EXPORT WidgetFactory : public QObject
{
    Q_OBJECT
public:
    //! Options used in createWidget()
    enum CreateWidgetOption {
        NoCreateWidgetOptions = 0,
        AnyOrientation = 1,        //!< any orientation hint
        HorizontalOrientation = 2, //!< horizontal orientation hint
        VerticalOrientation = 4,   //!< vertical orientation hint
        DesignViewMode = 8,        //!< create widget in design view mode, otherwise preview mode
        DefaultOptions = AnyOrientation | DesignViewMode
    };
    Q_DECLARE_FLAGS(CreateWidgetOptions, CreateWidgetOption)

    WidgetFactory(QObject *parent = 0, const char *name = 0);
    virtual ~WidgetFactory();

    /*! Adds a new class described by \a w. */
    void addClass(WidgetInfo *w);

    /*! This method allows to force a class \a classname to hidden.
     It is useful if you do not want a class to be available
     (e.g. because it is not implemented well yet for our purposes).
     All widget libraries are affected by this setting. */
    void hideClass(const char *classname);

    /**
     * \return all classes which are provided by this factory
     */
    const WidgetInfo::Hash& classes() const {
        return m_classesByName;
    }

    /**
     * Creates a widget (and if needed a KFormDesigner::Container)
     * \return the created widget
     * \param classname the classname of the widget, which should get created
     * \param parent the parent for the created widget
     * \param name the name of the created widget
     * \param container the toplevel Container (if a container should get created)
     * \param options options for the created widget: orientation and view mode (see CreateWidgetOptions)
     */
    virtual QWidget* createWidget(const QByteArray &classname, QWidget *parent, const char *name,
                                  KFormDesigner::Container *container,
                                  CreateWidgetOptions options = DefaultOptions) = 0;

    /*! Creates custom actions. Reimplement this if you need to add some
     actions coming from the factory. */
    virtual void createCustomActions(KActionCollection *col) {
        Q_UNUSED(col);
    }

    /*! This function can be used to add custom items in widget \a w context
    menu \a menu. */
    virtual bool createMenuActions(const QByteArray &classname, QWidget *w, QMenu *menu,
                                   KFormDesigner::Container *container) = 0;

    //! Arguments used by Form::createInlineEditor() and startInlineEditing()
    /*! @a text is the text to display by default in the line edit.
       @a widget is the edited widget, @a geometry is the geometry the new line
       edit should have, and @a alignment is Qt::Alignment of the new line edit. 
       If @a useFrame is false (the default), the line edit has no frame. 
       if @a multiLine is false (the default), the line edit has single line. 
       @a background describes line edit's background. 
       If @a execute is true (the default), createInlineEditor() will be executed. */
    class KFORMEDITOR_EXPORT InlineEditorCreationArguments {
    public:
        InlineEditorCreationArguments(
            const QByteArray& _classname, QWidget *_widget, Container *_container);
        QByteArray classname;
        QString text;
        QWidget *widget;
        Container *container;
        QRect geometry;
        Qt::Alignment alignment;
        Qt::BackgroundMode backgroundMode;
        bool useFrame : 1;
        bool multiLine : 1;
        bool execute : 1;
    };

    /*! Sets up (if necessary) aguments for the inline editor used to edit the contents 
       of the widget directly within the Form,
       e.g. creates a line edit to change the text of a label. @a args is
       used to pass the arguments back to the caller.
     */
    virtual bool startInlineEditing(InlineEditorCreationArguments& args) = 0;

    /*! This function is called just before the Form is previewed. It allows widgets
     to make changes before switching (ie for a Spring, hiding the cross) */
    virtual bool previewWidget(const QByteArray &classname, QWidget *widget, Container *container) = 0;

    virtual bool clearWidgetContent(const QByteArray &classname, QWidget *w);

    /*! This function is called when FormIO finds a property, at save time,
     that it cannot handle (ie not a normal property).
    This way you can save special properties, for example the contents of a listbox.
      \sa readSpecialProperty()
     */
    virtual bool saveSpecialProperty(const QByteArray &classname, const QString &name,
                                     const QVariant &value, QWidget *w,
                                     QDomElement &parentNode, QDomDocument &parent);

    /*! This function is called when FormIO finds a property or an unknown
    element in a .ui file. You can this way load a special property, for
      example the contents of a listbox.
       \sa saveSpecialProperty()
    */
    virtual bool readSpecialProperty(const QByteArray &classname, QDomElement &node,
                                     QWidget *w, ObjectTreeItem *item);

    /*! This function is used to know whether the \a property for the widget \a w
    should be shown or not in the PropertyEditor. If \a multiple is true,
    then multiple widgets of the same class are selected, and you should
    only show properties shared by widgets (eg font, color). By default,
    all properties are shown if multiple == true, and none if multiple == false. */
    bool isPropertyVisible(const QByteArray &classname, QWidget *w,
                           const QByteArray &property, bool multiple, bool isTopLevel);

    /*! You need to return here a list of the properties that should automatically be saved
    for a widget belonging to \a classname, and your custom properties (eg "text"
    for label or button, "contents" for combobox...). */
    virtual QList<QByteArray> autoSaveProperties(const QByteArray &classname) = 0;

    /*! \return The i18n'ed name of the property whose name is \a name,
     that will be displayed in PropertyEditor. */
    inline QString propertyDescForName(const QByteArray &name) {
        return m_propDesc[name];
    }

    /*! \return The i18n'ed name of the property's value whose name is \a name. */
    inline QString propertyDescForValue(const QByteArray &name) {
        return m_propValDesc[name];
    }

    /*! This method is called after form's property set was filled with properties
     of a widget \a w, of class defined by \a info.
     Default implementation does nothing.
     Implement this if you need to set options for properties within the set \a set. */
    virtual void setPropertyOptions(KoProperty::Set& set, const WidgetInfo& info, QWidget *w);

    /*! \return internal property \a property for a class \a classname.
     Internal properties are not stored within objects, but can be just provided
     to describe classes' details. */
    inline QString internalProperty(const QByteArray& classname, const QByteArray& property) const {
        return m_internalProp[classname+":"+property];
    }

    /*! This function is called when the widget is resized,
     and the @a editor size needs to be updated. */
    virtual void resizeEditor(QWidget *editor, QWidget *widget, const QByteArray &classname);

protected:
    /*! This function is called when we want to know whether the property should be visible.
     Implement it in the factory; don't forget to call implementation in the superclass.
     Default implementation hides "windowTitle", "windowIcon", "sizeIncrement" and "windowIconText" properties. */
    virtual bool isPropertyVisibleInternal(const QByteArray &classname, QWidget *w,
                                           const QByteArray &property, bool isTopLevel);

    /*! Sometimes property sets should be reloaded when a given property value changed.
     Implement it in the factory. Default implementation always returns false. */
    virtual bool propertySetShouldBeReloadedAfterPropertyChange(const QByteArray& classname, QWidget *w,
            const QByteArray& property);

    /*! This function provides a simple editing mode: it just disables event filtering
     for the widget, and it install it again when
     the widget loose focus or Enter is pressed.
    */
// contents moved to Form
    void disableFilter(QWidget *w, Container *container);

    /*! This function creates a little dialog (a KEditListBox) to modify the contents
     of a list (of strings). It can be used to modify the contents
     of a combo box for instance. The modified list is copied
     into \a list if the user presses "Ok" and true is returned. 
     When user presses "Cancel" false is returned. */
    bool editList(QWidget *w, QStringList &list) const;

    /*! This function creates a little editor to modify rich text. It supports alignment,
     subscript and superscript and all basic formatting properties.
     If the user presses "Ok", the edited text is put into @a text and true is returned. 
     If he presses "Cancel" false is returned. */
    bool editRichText(QWidget *w, QString &text) const;

#ifndef KEXI_FORMS_NO_LIST_WIDGET
    /*! This function creates a dialog to modify the contents of a list widget. You can modify both
    columns and list items. The list widget is automatically  updated if the user presses "Ok".*/
    void editListWidget(QListWidget *listwidget) const;
#endif

    /*! This function destroys the editor when it loses focus or Enter is pressed. */
// moved to Form
//    virtual bool  eventFilter(QObject *obj, QEvent *ev);

    /*! This function is used to modify a property of a widget (eg after editing it).
    Please use it instead of w->setProperty() to allow sync inside PropertyEditor.
    */
    void changeProperty(Form *form, QWidget *widget, const char *name, const QVariant &value);

//  /*! Adds the i18n'ed description of a property, which will be shown in PropertyEditor. */
//  void  addPropertyDescription(Container *container, const char *prop, const QString &desc);

//  /*! Adds the i18n'ed description of a property value, which will be shown in PropertyEditor. */
//  void  addValueDescription(Container *container, const char *value, const QString &desc);

    /*! \return true if at least one class defined by this factory inherits
     a class from other factory. Used in WidgetLibrary::loadFactories()
     to load factories in proper order. */
    bool inheritsFactories();

public slots:

// moved to Form::resetInlineEditor()
    /*! @internal. This slot is called when the editor has lost focus or the user pressed Enter.
    It destroys the editor or installs again the event filter on the widget. */
//    void resetEditor();

    //! Changes inline text for widget @a widget to @a text
    /*! Default implementation changes "text" property of the widget.
    You have to reimplement this function for inline editing inside the form @a form if your widget's
    property you want to change is not named "text".
    This slot is called when the line edit text changes, and you have to make
    it really change property of the widget using changeProperty() (text, title, etc.). */
    virtual bool changeInlineText(Form *form, QWidget *widget,
                                  const QString& text, QString &oldText);

protected slots:

// Moved to Form::changeInlineTextInternal()
//    void changeTextInternal(const QString& text);

// Moved to Form::slotInlineTextChanged()
//    void slotTextChanged();

// Moved to Form()
    /*! This slot is called when the editor is destroyed.*/
//    void editorDeleted();
// Moved to Form()
//    void widgetDestroyed();

protected:
// Moved to Form::inlineEditorText()
//    QString editorText() const;
// Moved to Form::setInlineEditorText()
//    void setEditorText(const QString& text);
#if 0 //2.0
    void setEditor(QWidget *widget, QWidget *editor);
    QWidget *editor(QWidget *widget) const;
    void setWidget(QWidget *widget, Container *container);
    QWidget *widget() const;
#endif
    /*! Assigns \a value for internal property \a property for a class \a classname.
     Internal properties are not stored within objects, but can be provided
     to describe classes' details. */
    void setInternalProperty(const QByteArray& classname, const QByteArray& property, const QString& value);

    WidgetLibrary *m_library;
// moved to Form
//    QByteArray m_editedWidgetClass;
// moved to Form as originalInlineText
//    QString m_firstText;
//2.0: removed
//    QPointer<ResizeHandleSet> m_handles;
//2.0: moved to Form::Private::inlineEditorContainer
//    QPointer<Container> m_container;
//  WidgetInfo::List m_classes;
    WidgetInfo::Hash m_classesByName;
    QSet<QByteArray>* m_hiddenClasses;

    //! i18n stuff
    QHash<QByteArray, QString> m_propDesc;
    QHash<QByteArray, QString> m_propValDesc;
    //! internal properties
    QHash<QByteArray, QString> m_internalProp;

    /*! flag useful to decide whether to hide some properties.
     It's value is inherited from WidgetLibrary. */
    bool m_showAdvancedProperties;

    /*! Contains name of an XMLGUI file providing toolbar buttons
     (and menu items in the future?) for the factory.
     Can be empty, e.g. for the main factory which has XMLGUI defined in the shell window itself
     (e.g. kexiformpartinstui.rc for Kexi Forms). This name is set in WidgetLibrary::loadFactories() */
    QString m_xmlGUIFileName;

    KXMLGUIClient *m_guiClient;

//2.0    QPointer<QWidget> m_widget;
// moved to Form as Private::inlineEditor
//    QPointer<QWidget> m_editor;

    friend class WidgetLibrary;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(WidgetFactory::CreateWidgetOptions)

//! macro to declare KFormDesigner-compatible widget factory as a KDE Component factory
#define KFORMDESIGNER_WIDGET_FACTORY(factoryClassName, libraryName) \
    K_EXPORT_COMPONENT_FACTORY(kformdesigner_ ## libraryName, KGenericFactory<factoryClassName>("kformdesigner_" # libraryName))

}
#endif
