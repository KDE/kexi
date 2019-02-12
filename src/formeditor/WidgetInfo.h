/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2017 Jarosław Staniek <staniek@kde.org>

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

#ifndef KFORMDESIGNERWIDGETINFO_H
#define KFORMDESIGNERWIDGETINFO_H

#include "kformdesigner_export.h"

#include <KDbTristate>

#include <QHash>

namespace KFormDesigner
{

class WidgetFactory;
class WidgetLibrary;

//! A class providing properties of a single widget class offered by a factory
class KFORMDESIGNER_EXPORT WidgetInfo
{
public:
    explicit WidgetInfo(WidgetFactory *f);

    virtual ~WidgetInfo();

    //! \return the name of the icon associated with the widget
    QString iconName() const;

    void setIconName(const QString &iconName);

    //! @return the class name of a widget e.g. "LineEdit"
    QByteArray className() const;

    void setClassName(const QByteArray& className);

    QByteArray inheritedClassName() const;

    void setInheritedClassName(const QByteArray& inheritedClassName);

    /**
     * @return Untranslated name prefix used to generate unique names for widget instances
     *
     * Widget names appear in the property editor or in scripts and can be references there.
     * Names and prefixes must be valid identifiers, as defined by KDb::isIdentifier().
     *
     * @see translatedNamePrefix()
     */
    QString namePrefix() const;

    /**
     * Sets untranslated name prefix used to generate unique names for widget instances
     *
     * @a prefix must be a valid identifier, as defined by KDb::isIdentifier(). If it is not then
     * real name prefix will be reset to "widget".
     * Parameters of setNamePrefix() should be enclosed with I18N_NOOP2() to enable translation
     * text extraction without actual translation of the argument. @a context parameter is unused;
     * it's provided only to make I18N_NOOP2() work.
     *
     * Example use for a label class:
     * @code
     * setNamePrefix(
     *      I18N_NOOP2("A prefix for identifiers of label widgets. Based on that, identifiers such as "
     *          "label1, label2 are generated. "
     *          "This string can be used to refer the widget object as variables in programming "
     *          "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
     *          "should start with lower case letter and if there are subsequent words, these should "
     *          "start with upper case letter. Example: smallCamelCase. "
     *          "Moreover, try to make this prefix as short as possible.",
     *          "label"));
     * @endcode
     *
     * If translation of @a prefix for given locale is not a valid identifier, as defined by
     * KDb::isIdentifier() then a warning is issued to the error channel and untranslated prefix is
     * used, e.g. "label".
     */
    void setNamePrefix(const char *context, const char *prefix);

    /**
     * @return Translated name prefix used to generate unique names for widget instances
     *
     * This string is created using i18n(namePrefix()). See setNamePrefix() for exceptional cases
     * of invalid translations.
     */
    QString translatedNamePrefix() const;

    //! \return the real name e.g. 'Line Edit', showed eg in ObjectTreeView
    QString name() const;

    void setName(const QString &n);

    QString description() const;

    void setDescription(const QString &desc);

    QString includeFileName() const;

    /*! Sets the C++ include file corresponding to this class,
     that uic will need to add when creating the file. You don't have to set this for Qt std widgets.*/
    void setIncludeFileName(const QString &name);

    QList<QByteArray> alternateClassNames() const;

    QByteArray savingName() const;

    WidgetFactory *factory() const;

    /*! Sets alternate names for this class.
     If this name is found when loading a .ui file, the className() will be used instead.
     It allows to support both KDE and Qt versions of widget, without duplicating code.
     As a rule, className() should always return a class name which is inherited from
     alternate class. For exampleQPushButton class has alternate KexiPushButton class.

     \a override parameter overrides class name of a widget,
     even if it was implemented in other factory.
     By default it's set to false, what means that no other class is overridden
     by this widget class if there is already a class implementing it
     (no matter in which factory).
     By forced overriding existing class with other - custom, user
     will be able to see more or less properties and experience different behaviour.
     For example, in Kexi application, KexiDBPushButton class contains additional
     properties.
    */
    void addAlternateClassName(const QByteArray& alternateName, bool override = false);

    /*! \return true if a class \a alternateName is defined as alternate name with
     'override' flag set to true, using addAlternateClassName().
     If this flag is set to false (the default) or there's no such alternate class
     name defined. */
    bool isOverriddenClassName(const QByteArray& alternateName) const;

    /*! Sets the name that will be written in the .ui file when saving.
     This name must be one of alternate names (or loading will be impossible).

     On form data saving to XML .ui format, saveName is used instead,
     so .ui format is not broken and still usable with other software as Qt Designer.
     Custom properties are saved as well with 'stdset' attribute set to 0. */
    void setSavingName(const QByteArray &saveName);

    /*! Sets autoSync flag for property \a propertyName.
     This allows to override autoSync flag for certain widget's property, because
     e.g. KPropertyEditorView can have autoSync flag set to false or true, but
     not all properties have to comply with that.
     \a flag equal to cancelled value means there is no overriding (the default). */
    void setAutoSyncForProperty(const QByteArray& propertyName, tristate flag);

    /*! \return autoSync override value (true or false) for \a propertyName.
     If cancelled value is returned, there is no overriding (the default). */
    tristate autoSyncForProperty(const QByteArray& propertyName) const;

    QByteArray parentFactoryName() const;

    void setParentFactoryName(const QByteArray& factoryName);

    WidgetInfo* inheritedClass() const;

    /*! Sets custom type \a type for property \a propertyName.
     This allows to override default type, especially when custom property
     and custom property editor item has to be used. */
    void setCustomTypeForProperty(const QByteArray& propertyName, int type);

    /*! \return custom type for property \a propertyName. If no specific custom type has been assigned,
     KProperty::Auto is returned.
     @see setCustomTypeForProperty() */
    int customTypeForProperty(const QByteArray& propertyName) const;

    /*! @return list of the properties that should automatically be saved
    for a widget of @a classname class.
    Examples are: custom properties "text" for label or button, "contents" for combobox...
    If there is inherited class (see @ref inheritedClass()), autosave properties
    from that class are prepended to the list. */
    QList<QByteArray> autoSaveProperties() const;

    /*! Sets list of the properties that should automatically be saved for a widget of @a classname class. */
    void setAutoSaveProperties(const QList<QByteArray>& properties);

    /*! @return internal property @a property.
     Internal properties are not stored within objects, but can be just provided
     to describe class' details. */
    QVariant internalProperty(const QByteArray& property) const;

    /*! Assigns @a value for internal property @a property.
     Internal properties are not stored within objects, but can be provided
     to describe class' details. */
    void setInternalProperty(const QByteArray& property, const QVariant& value);

    /**
     * @brief Returns @c alignment flags supports by the widget
     *
     * By default returns all possible flags minus Qt::AlignAbsolute.
     * @see setSupportedAlignmentFlags
     */
    Qt::Alignment supportedAlignmentFlags() const;

    /**
     * @brief Sets alignment flags supported by the widget
     *
     * Used for example by an image box that does not support justified, absolute and baseline
     * alignment.
     * @see supportedAlignmentFlags
     */
    void setSupportedAlignmentFlags(Qt::Alignment flags);

protected:
    void setInheritedClass(WidgetInfo *inheritedClass);

private:
    class Private;
    Private * const d;

    friend class WidgetLibrary;
};

} // namespace KFormDesigner

#endif
