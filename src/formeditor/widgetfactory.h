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

#include <QVariant>
#include <QRect>

#include <KPluginFactory>

#include <config-kexi.h>
#include "kformdesigner_export.h"

class QWidget;
class QListWidget;
class QMenu;
class QDomElement;
class QDomDocument;
class QVariant;
class KActionCollection;
class KPropertySet;

namespace KFormDesigner
{

class Container;
class ObjectTreeItem;
class Form;
class WidgetLibrary;
class WidgetInfo;

//! Used by WidgetFactory
class KFORMDESIGNER_EXPORT InternalPropertyHandlerInterface
{
protected:
    InternalPropertyHandlerInterface();

    virtual ~InternalPropertyHandlerInterface();

    /*! Assigns \a value for internal property \a property for a class \a classname.
     Internal properties are not stored within objects, but can be provided
     to describe class' details. */
    virtual void setInternalProperty(const QByteArray& classname, const QByteArray& property, const QVariant& value) = 0;

    friend class WidgetInfo;
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
  stuff (see eg Spring class).\n
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
  Use void WidgetInfo::setInternalProperty(const QByteArray& property, const QVariant& value)
  to set values of special internal properties.
  Currently these properties are used for customizing popup menu items used for orientation selection.
  Customization for class ClassName should look like:
  <code>
   WidgetInfo *wi = ...
   wi->setInternalProperty("orientationSelectionPopup", true);
   wi->setInternalProperty("orientationSelectionPopup:horizontalIcon", "myicon");
  </code>
  Available internal properties:
  * "orientationSelectionPopup" - set it to true if you want a given class to offer orientation selection,
     so orientation selection popup will be displayed when needed.
  * "orientationSelectionPopup:horizontalIcon" - sets a name of icon for "Horizontal" item
    for objects of class 'ClassName'. Set this property only for classes supporting orientations.
  * "orientationSelectionPopup:verticalIcon" - the same for "Vertical" item.
    Set this property only for classes supporting orientations.
  * "orientationSelectionPopup:horizontalText" - sets a i18n'd text for "Horizontal" item
    for objects of class 'ClassName', e.g. xi18n("Insert Horizontal Line").
    Set this property only for classes supporting orientations.
  * "orientationSelectionPopup:verticalText" - the same for "Vertical" item,
    e.g. xi18n("Insert Vertical Line"). Set this property only for classes supporting orientations.
  * "dontStartEditingOnInserting" - if true, WidgetFactory::startInlineEditing() will not be executed upon
    widget inseting by a user.
  * "forceShowAdvancedProperty:{propertyname}" - set it to true for "{propertyname}" advanced property
    if you want to force it to be visible even if WidgetLibrary::setAdvancedPropertiesVisible(false)
    has been called. For example, setting "forceShowAdvancedProperty:pixmap" to true
    unhides "pixmap" property for a given class.

  See KexiStandardFormWidgetsFactory::KexiStandardFormWidgetsFactory() for properties like "Line:orientationSelectionPopup:horizontalIcon".

  \n\n
  See the standard factories in formeditor/factories for an example of factories,
  and how to deal with complex widgets (eg tabwidget).
  */
class KFORMDESIGNER_EXPORT WidgetFactory : public QObject,
                                           public InternalPropertyHandlerInterface
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

    explicit WidgetFactory(QObject *parent);

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
    QHash<QByteArray, WidgetInfo*> classes() const;

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
                                  CreateWidgetOptions options = DefaultOptions) /*Q_REQUIRED_RESULT*/ = 0;

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
    class KFORMDESIGNER_EXPORT InlineEditorCreationArguments {
    public:
        InlineEditorCreationArguments(
            const QByteArray& _classname, QWidget *_widget, Container *_container);
        QByteArray classname;
        QString text;
        QWidget *widget;
        Container *container;
        QRect geometry;
        Qt::Alignment alignment;
        bool useFrame;
        bool multiLine;
        bool execute;
        //! true if the inline editor's bakground should be transparent (false by default)
        bool transparentBackground;
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

    /*! \return The i18n'ed name of the property whose name is \a name,
     that will be displayed in PropertyEditor. */
    QString propertyDescription(const char* name) const;

    /*! \return The i18n'ed name of the property's value whose name is \a name. */
    QString valueDescription(const char* name) const;

    /*! This method is called after form's property set was filled with properties
     of a widget \a w, of class defined by \a info.
     Default implementation does nothing.
     Implement this if you need to set options for properties within the set \a set. */
    virtual void setPropertyOptions(KPropertySet& set, const WidgetInfo& info, QWidget *w);

    /*! \return internal property \a property for a class \a classname.
     Internal properties are not stored within objects, but can be just provided
     to describe class' details. */
    QVariant internalProperty(const QByteArray& classname, const QByteArray& property) const;

    /*! This function is called when the widget is resized,
     and the @a editor size needs to be updated. */
    virtual void resizeEditor(QWidget *editor, QWidget *widget, const QByteArray &classname);

    /*! @return selectable item for @a item item.
     By default it is equal to @a item but e.g. for pages of QTabWidget,
     item for the widget itself is returned.
     Used when user clicks on the Widget Tree item or when parent of the current
     widget should to be selected. Defaults can be overridden by reimplementing this method. */
    virtual ObjectTreeItem* selectableItem(ObjectTreeItem* item);

    /*! Sets the i18n'ed description of a property, which will be shown in PropertyEditor. */
    void setPropertyDescription(const char *property, const QString &description);

    /*! Sets the i18n'ed description of a property value, which will be shown in PropertyEditor. */
    void setValueDescription(const char *valueName, const QString &description);


    void setAdvancedPropertiesVisible(bool set);
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
     the widget loose focus or Enter is pressed. */
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
     If the user presses "Cancel" false is returned. */
    bool editRichText(QWidget *w, QString &text) const;

#ifdef KEXI_LIST_FORM_WIDGET_SUPPORT
    /*! This function creates a dialog to modify the contents of a list widget. You can modify both
    columns and list items. The list widget is automatically  updated if the user presses "Ok".*/
    void editListWidget(QListWidget *listwidget) const;
#endif

    /*! This function is used to modify a property of a widget (eg after editing it).
    Please use it instead of w->setProperty() to allow sync inside PropertyEditor.
    */
    void changeProperty(Form *form, QWidget *widget, const char *name, const QVariant &value);

    /*! \return true if at least one class defined by this factory inherits
     a class from other factory. Used in WidgetLibrary::loadFactories()
     to load factories in proper order. */
    bool inheritsFactories();

    /*! Assigns \a value for internal property \a property for a class \a classname.
     Internal properties are not stored within objects, but can be provided
     to describe class' details. */
    void setInternalProperty(const QByteArray& classname, const QByteArray& property, const QVariant& value) override;

    WidgetInfo* widgetInfoForClassName(const char* classname);

    const QSet<QByteArray>* hiddenClasses() const;

    WidgetLibrary* library();

    bool advancedPropertiesVisible() const;

    void setLibrary(WidgetLibrary* library);

public Q_SLOTS:
    /*! @internal. This slot is called when the editor has lost focus or the user pressed Enter.
    It destroys the editor or installs again the event filter on the widget. */

    //! Changes inline text for widget @a widget to @a text
    /*! Default implementation changes "text" property of the widget.
    You have to reimplement this function for inline editing inside the form @a form if your widget's
    property you want to change is not named "text".
    This slot is called when the line edit text changes, and you have to make
    it really change property of the widget using changeProperty() (text, title, etc.). */
    virtual bool changeInlineText(Form *form, QWidget *widget,
                                  const QString& text, QString *oldText = nullptr);
private:
    class Private;
    Private* const d;

    friend class WidgetLibrary;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(WidgetFactory::CreateWidgetOptions)

}
#endif
