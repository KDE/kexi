/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2014 Jarosław Staniek <staniek@kde.org>

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

#include "form_p.h"
#include "formIO.h"

#include <KLocalizedString>

#include <QStyleOption>

using namespace KFormDesigner;

DesignModeStyle::DesignModeStyle(QStyle* parentStyle)
    : QProxyStyle(parentStyle)
{
}

void DesignModeStyle::drawControl(ControlElement element, const QStyleOption *option,
                                  QPainter *p, const QWidget *w) const
{
    QStyleOption *so = alterOption(element, option);
    QProxyStyle::drawControl(element, so ? so : option, p, w);
    delete so;
}

//! Used in alterOption()
template <class StyleOptionClass>
static StyleOptionClass *cloneStyleOption(const QStyleOption *option)
{
    return new StyleOptionClass( *qstyleoption_cast<const StyleOptionClass*>(option) );
}

QStyleOption* DesignModeStyle::alterOption(ControlElement element, const QStyleOption *option) const
{
    Q_UNUSED(element)
    if (!option)
        return 0;
    QStyleOption* res = 0;
    switch (option->type) {
    case QStyleOption::SO_Button:
        res = cloneStyleOption<QStyleOptionButton>(option);
        break;
    case QStyleOption::SO_ComboBox:
        res = cloneStyleOption<QStyleOptionComboBox>(option);
        break;
    case QStyleOption::SO_Complex:
        res = cloneStyleOption<QStyleOptionComplex>(option);
        break;
    case QStyleOption::SO_DockWidget:
        res = cloneStyleOption<QStyleOptionDockWidget>(option);
        break;
    case QStyleOption::SO_FocusRect:
        res = cloneStyleOption<QStyleOptionFocusRect>(option);
        break;
    case QStyleOption::SO_Frame:
        res = cloneStyleOption<QStyleOptionFrame>(option);
        break;
    case QStyleOption::SO_GraphicsItem:
        res = cloneStyleOption<QStyleOptionGraphicsItem>(option);
        break;
    case QStyleOption::SO_GroupBox:
        res = cloneStyleOption<QStyleOptionGroupBox>(option);
        break;
    case QStyleOption::SO_Header:
        res = cloneStyleOption<QStyleOptionHeader>(option);
        break;
    case QStyleOption::SO_MenuItem:
        res = cloneStyleOption<QStyleOptionMenuItem>(option);
        break;
    case QStyleOption::SO_ProgressBar:
        res = cloneStyleOption<QStyleOptionProgressBar>(option);
        break;
    case QStyleOption::SO_RubberBand:
        res = cloneStyleOption<QStyleOptionRubberBand>(option);
        break;
    case QStyleOption::SO_SizeGrip:
        res = cloneStyleOption<QStyleOptionSizeGrip>(option);
        break;
    case QStyleOption::SO_Slider:
        res = cloneStyleOption<QStyleOptionSlider>(option);
        break;
    case QStyleOption::SO_SpinBox:
        res = cloneStyleOption<QStyleOptionSpinBox>(option);
        break;
    case QStyleOption::SO_Tab:
        res = cloneStyleOption<QStyleOptionTab>(option);
        break;
    case QStyleOption::SO_TabBarBase:
        res = cloneStyleOption<QStyleOptionTabBarBase>(option);
        break;
    case QStyleOption::SO_TabWidgetFrame:
        res = cloneStyleOption<QStyleOptionTabWidgetFrame>(option);
        break;
    case QStyleOption::SO_TitleBar:
        res = cloneStyleOption<QStyleOptionTitleBar>(option);
        break;
    case QStyleOption::SO_ToolBar:
        res = cloneStyleOption<QStyleOptionToolBar>(option);
        break;
    case QStyleOption::SO_ToolBox:
        res = cloneStyleOption<QStyleOptionToolBox>(option);
        break;
    case QStyleOption::SO_ToolButton:
        res = cloneStyleOption<QStyleOptionToolButton>(option);
        break;
    case QStyleOption::SO_ViewItem:
        res = cloneStyleOption<QStyleOptionViewItem>(option);
        break;
    default:
        return 0;
    }

    const QStyle::State statesToRemove( QStyle::State_MouseOver | State_HasFocus );
    res->state |= statesToRemove;
    res->state ^= statesToRemove;
    return res;
}

//--------------

FormPrivate::FormPrivate(Form *form, WidgetLibrary* _library)
 : state(Form::WidgetSelecting)
 , internalCollection(static_cast<QObject*>(0))
 , library(_library)
 , q(form)
{
    toplevel = 0;
    topTree = 0;
    widget = 0;
    modified = false;
    interactive = true;
    autoTabstops = true;
    isRedoing = false;
//! @todo get the default from globals...
    snapToGrid = true;
    gridSize = 10;
#ifdef KFD_SIGSLOTS
    connBuffer = new ConnectionBuffer();
#endif
    formatVersion = KFormDesigner::version();
    originalFormatVersion = KFormDesigner::version();
    lastCommand = 0;
    lastCommandGroup = 0;
    isUndoing = false;
    slotPropertyChangedEnabled = true;
    slotPropertyChanged_addCommandEnabled = true;
    insideAddPropertyCommand = false;
    initPropertiesDescription();
    designModeStyle = 0;
    idOfPropertyCommand = 0;
    selectWidgetEnabled = true;
    executingCommand = 0;
    pixmapsStoredInline = false;
}

FormPrivate::~FormPrivate()
{
    delete topTree;
#ifdef KFD_SIGSLOTS
    delete connBuffer;
    connBuffer = 0;
#endif
}

QString FormPrivate::propertyCaption(const QByteArray &name)
{
    return propCaption.value(name);
}

QString FormPrivate::valueCaption(const QByteArray &name)
{
    return propValCaption.value(name);
}

void FormPrivate::addPropertyCaption(const QByteArray &property, const QString &caption)
{
    if (!propCaption.contains(property))
        propCaption.insert(property, caption);
}

void FormPrivate::addValueCaption(const QByteArray &value, const QString &caption)
{
    if (!propValCaption.contains(value))
        propValCaption.insert(value, caption);
}

void FormPrivate::setColorProperty(KProperty& p,
                                   QPalette::ColorRole (QWidget::*roleMethod)() const,
                                   const QVariant& value)
{
    bool nullColor = value.isNull() || !value.value<QColor>().isValid();
    foreach(QWidget* widget, selected) {
        ObjectTreeItem *titem = q->objectTree()->lookup(widget->objectName());
        QColor color;
        if (nullColor && roleMethod == &QWidget::backgroundRole) {
            color = widget->parentWidget()->palette().color((widget->*roleMethod)());
        }
        else {
            color = value.value<QColor>();
        }
        if (titem && p.isModified())
            titem->addModifiedProperty(p.name(), color);
        QPalette widgetPalette(widget->palette());
        QColor oldColor(widgetPalette.color((widget->*roleMethod)()));
        widgetPalette.setColor((widget->*roleMethod)(), color);
        widget->setPalette(widgetPalette);
        if (!isRedoing && !isUndoing) {
            q->addPropertyCommand(widget->objectName().toLatin1(),
                oldColor, color, p.name(), Form::DontExecuteCommand);
        }
        if (roleMethod == &QWidget::backgroundRole) {
            widget->setAutoFillBackground(!nullColor);
            if (nullColor) { // make background inherited
                widget->setBackgroundRole(QPalette::NoRole);
            }
        }
    }
}

void FormPrivate::enableAction(const char *name, bool enable)
{
    QAction *a = collection->action(QLatin1String(name));
    if (a) {
        a->setEnabled(enable);
    }
}

////////////////////////////////////////// i18n related functions ////////

void FormPrivate::initPropertiesDescription()
{
//! \todo perhaps a few of them shouldn't be translated within KFD mode,
//!       to be more Qt Designer friendly?
    propCaption["name"] = xi18n("Name"); // for backward compatibility with Qt 3
    propCaption["objectName"] = xi18n("Name");
    propCaption["caption"] = xi18n("Caption");
    propCaption["windowTitle"] = xi18n("Window title");
    propCaption["text"] = xi18n("Text");
    propCaption["paletteBackgroundPixmap"] = xi18n("Background Pixmap");
    propCaption["enabled"] = xi18nc("Propery: enabled widget", "Enabled");
    propCaption["geometry"] = xi18n("Geometry");
    propCaption["sizePolicy"] = xi18n("Size Policy");
    propCaption["minimumSize"] = xi18n("Minimum Size");
    propCaption["maximumSize"] = xi18n("Maximum Size");
    propCaption["font"] = xi18n("Font");
    propCaption["cursor"] = xi18n("Cursor");
    propCaption["paletteForegroundColor"] = xi18n("Foreground Color");
    propCaption["paletteBackgroundColor"] = xi18n("Background Color");
    propCaption["autoFillBackground"] = xi18n("Fill Background");
    propCaption["focusPolicy"] = xi18n("Focus Policy");
    propCaption["margin"] = xi18n("Margin");
    propCaption["readOnly"] = xi18n("Read Only");
    propCaption["styleSheet"] = xi18n("Style Sheet");
    propCaption["toolTip"] = xi18nc("Widget's Tooltip", "Tooltip");
    propCaption["whatsThis"] = xi18nc("Widget's Whats This", "What's This");
    propCaption["iconSize"] = xi18n("Icon Size");

    //any QFrame
    propCaption["frame"] = xi18n("Frame");
    propCaption["lineWidth"] = xi18n("Frame Width");
    propCaption["midLineWidth"] = xi18n("Mid Frame Width");
    propCaption["frameShape"] = xi18n("Frame Shape");
    propCaption["frameShadow"] = xi18n("Frame Shadow");
    //any QScrollBar
    propCaption["vScrollBarMode"] = xi18n("Vertical Scrollbar");
    propCaption["hScrollBarMode"] = xi18n("Horizontal Scrollbar");

    propValCaption["NoBackground"] = xi18n("No Background");
    propValCaption["PaletteForeground"] = xi18n("Palette Foreground");
    propValCaption["AutoText"] = xi18nc("Auto (HINT: for AutoText)", "Auto");

    propValCaption["AlignAuto"] = xi18nc("Auto (HINT: for Align)", "Auto");
    propValCaption["AlignLeft"] = xi18nc("Left (HINT: for Align)", "Left");
    propValCaption["AlignRight"] = xi18nc("Right (HINT: for Align)", "Right");
    propValCaption["AlignHCenter"] = xi18nc("Center (HINT: for Align)", "Center");
    propValCaption["AlignJustify"] = xi18nc("Justify (HINT: for Align)", "Justify");
    propValCaption["AlignVCenter"] = xi18nc("Center (HINT: for Align)", "Center");
    propValCaption["AlignTop"] = xi18nc("Top (HINT: for Align)", "Top");
    propValCaption["AlignBottom"] = xi18nc("Bottom (HINT: for Align)", "Bottom");

    propValCaption["NoFrame"] = xi18nc("No Frame (HINT: for Frame Shape)", "No Frame");
    propValCaption["Box"] = xi18nc("Box (HINT: for Frame Shape)", "Box");
    propValCaption["Panel"] = xi18nc("Panel (HINT: for Frame Shape)", "Panel");
    propValCaption["WinPanel"] = xi18nc("Windows Panel (HINT: for Frame Shape)", "Windows Panel");
    propValCaption["HLine"] = xi18nc("Horiz. Line (HINT: for Frame Shape)", "Horiz. Line");
    propValCaption["VLine"] = xi18nc("Vertical Line (HINT: for Frame Shape)", "Vertical Line");
    propValCaption["StyledPanel"] = xi18nc("Styled (HINT: for Frame Shape)", "Styled");
    propValCaption["PopupPanel"] = xi18nc("Popup (HINT: for Frame Shape)", "Popup");
    propValCaption["MenuBarPanel"] = xi18nc("Menu Bar (HINT: for Frame Shape)", "Menu Bar");
    propValCaption["ToolBarPanel"] = xi18nc("Toolbar (HINT: for Frame Shape)", "Toolbar");
    propValCaption["LineEditPanel"] = xi18nc("Text Box (HINT: for Frame Shape)", "Text Box");
    propValCaption["TabWidgetPanel"] = xi18nc("Tab Widget (HINT: for Frame Shape)", "Tab Widget");
    propValCaption["GroupBoxPanel"] = xi18nc("Group Box (HINT: for Frame Shape)", "Group Box");

    propValCaption["Plain"] = xi18nc("Plain (HINT: for Frame Shadow)", "Plain");
    propValCaption["Raised"] = xi18nc("Raised (HINT: for Frame Shadow)", "Raised");
    propValCaption["Sunken"] = xi18nc("Sunken (HINT: for Frame Shadow)", "Sunken");
    propValCaption["MShadow"] = xi18nc("for Frame Shadow", "Internal");

    propValCaption["NoFocus"] = xi18nc("No Focus (HINT: for Focus)", "No Focus");
    propValCaption["TabFocus"] = xi18nc("Tab (HINT: for Focus)", "Tab");
    propValCaption["ClickFocus"] = xi18nc("Click (HINT: for Focus)", "Click");
    propValCaption["StrongFocus"] = xi18nc("Tab/Click (HINT: for Focus)", "Tab/Click");
    propValCaption["WheelFocus"] = xi18nc("Tab/Click/MouseWheel (HINT: for Focus)", "Tab/Click/Mouse Wheel");

    propValCaption["Auto"] = xi18n("Auto");
    propValCaption["AlwaysOff"] = xi18n("Always Off");
    propValCaption["AlwaysOn"] = xi18n("Always On");

    //orientation
    propValCaption["Horizontal"] = xi18n("Horizontal");
    propValCaption["Vertical"] = xi18n("Vertical");

    //layout direction
    propValCaption["LeftToRight"] = xi18n("Left to Right");
    propValCaption["RightToLeft"] = xi18n("Right to Left");
}

KPropertyListData* FormPrivate::createValueList(WidgetInfo *winfo, const QStringList &list)
{
    QStringList names;
    foreach (const QString& name, list) {
        QString n(propValCaption.value(name.toLatin1()));
        if (n.isEmpty()) { //try within factory and (maybe) parent factory
            if (winfo) {
                n = q->library()->propertyDescForValue(winfo, name.toLatin1());
            }
            if (n.isEmpty()) {
                names.append(name);   //untranslated
            }
            else {
                names.append(n);
            }
        } else {
            names.append(n);
        }
    }
    return new KPropertyListData(list, names);
}
