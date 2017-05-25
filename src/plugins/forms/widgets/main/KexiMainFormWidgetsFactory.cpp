/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2016 Jarosław Staniek <staniek@kde.org>

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

#include "KexiMainFormWidgetsFactory.h"
#include <KexiIcon.h>
#include <formeditor/container.h>
#include <formeditor/form.h>
#include <formeditor/formIO.h>
#include <formeditor/objecttree.h>
#include <formeditor/utils.h>
#include <formeditor/widgetlibrary.h>
#include <core/kexi.h>
#include <core/kexipart.h>
#include <core/KexiMainWindowIface.h>
#include <kexiutils/utils.h>
#include <kexiutils/KexiCommandLinkButton.h>
#include <widget/properties/KexiCustomPropertyFactory.h>
#include <widget/utils/kexicontextmenuutils.h>
#include <kexi_global.h>
#include "kexiformview.h"
#include "KexiStandardContainerFormWidgets.h"
#include "KexiStandardFormWidgets.h"
#include "kexidbcheckbox.h"
#include "kexidbimagebox.h"
#include "kexiframe.h"
#include "kexidblabel.h"
#include "kexidblineedit.h"
#include "kexidbtextedit.h"
#include "kexidbcombobox.h"
#include "KexiDBPushButton.h"
#include "kexidbform.h"
#include "kexidbcommandlinkbutton.h"
#include "kexidbslider.h"
#include "kexidbprogressbar.h"
#include "kexidbdatepicker.h"
#include "kexidataawarewidgetinfo.h"
#include <widget/dataviewcommon/kexiformdataiteminterface.h>

#include <KDbConnection>

#include <KPropertySet>

#include <KActionCollection>
#include <KLocalizedString>

#include <QDomElement>
#include <QStyle>
#include <QFontMetrics>

KEXI_PLUGIN_FACTORY(KexiMainFormWidgetsFactory, "kexiforms_mainwidgetsplugin.json")

KexiMainFormWidgetsFactory::KexiMainFormWidgetsFactory(QObject *parent, const QVariantList &)
        : KexiDBFactoryBase(parent)
        , m_assignAction(0)
{
    {
        KexiDataAwareWidgetInfo *wi = new KexiDataAwareWidgetInfo(this);
        wi->setIconName(KexiIconName("form"));
        wi->setClassName("KexiDBForm");
        wi->setName(xi18nc("Form widget", "Form"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of forms. Based on that, identifiers such as "
                "form1, form2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "form"));
        wi->setDescription(xi18n("A form widget"));
        addClass(wi);
    }

    {
        KFormDesigner::WidgetInfo *wi = new KFormDesigner::WidgetInfo(this);
        wi->setIconName(KexiIconName("unknown-widget"));
        wi->setClassName("CustomWidget");
        wi->setName(/* no i18n needed */ "Custom Widget");
        wi->setNamePrefix(/* no i18n needed */ "customWidget");
        wi->setDescription(/* no i18n needed */ "A custom or non-supported widget");
        addClass(wi);
    }

    {
        KexiDataAwareWidgetInfo* wi = new KexiDataAwareWidgetInfo(this);
        wi->setIconName(KexiIconName("lineedit"));
        wi->setClassName("KexiDBLineEdit");
        wi->addAlternateClassName("QLineEdit", true/*override*/);
        wi->addAlternateClassName("KLineEdit", true/*override*/);
        wi->setName(xi18nc("Text Box widget", "Text Box"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of text box widgets. Based on that, identifiers such as "
                "textBox1, textBox2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "textBox"));
        wi->setDescription(xi18n("A widget for entering and displaying line of text text"));
        wi->setInternalProperty("dontStartEditingOnInserting", true); // because we are most probably assign data source to this widget
        wi->setInlineEditingEnabledWhenDataSourceSet(false);
        addClass(wi);
    }
    {
        KexiDataAwareWidgetInfo* wi = new KexiDataAwareWidgetInfo(this);
        wi->setIconName(KexiIconName("textedit"));
        wi->setClassName("KexiDBTextEdit");
        wi->addAlternateClassName("QTextEdit", true/*override*/);
        wi->addAlternateClassName("KTextEdit", true/*override*/);
        wi->setName(xi18nc("Text Editor widget", "Text Editor"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of text editor widgets. Based on that, identifiers such as "
                "textEditor1, textEditor2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "textEditor"));
        wi->setDescription(xi18n("A multiline text editor"));
        wi->setInternalProperty("dontStartEditingOnInserting", true); // because we are most probably assign data source to this widget
        wi->setInlineEditingEnabledWhenDataSourceSet(false);
        addClass(wi);
    }
    {
        KexiDataAwareWidgetInfo* wi = new KexiDataAwareWidgetInfo(this);
        wi->setIconName(KexiIconName("label"));
        wi->setClassName("KexiDBLabel");
        wi->addAlternateClassName("QLabel", true/*override*/);
        wi->addAlternateClassName("KexiLabel", true/*override*/); //older
        wi->setName(xi18nc("Text Label widget", "Label"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of label widgets. Based on that, identifiers such as "
                "label1, label2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "label"));
        wi->setDescription(xi18n("A widget for displaying text"));
        wi->setInlineEditingEnabledWhenDataSourceSet(false);
        addClass(wi);
    }

    {
        KexiDataAwareWidgetInfo* wi = new KexiDataAwareWidgetInfo(this);
        wi->setIconName(KexiIconName("imagebox"));
        wi->setClassName("KexiDBImageBox");
        wi->addAlternateClassName("KexiPictureLabel", true/*override*/);
        wi->addAlternateClassName("KexiImageBox", true/*override*/); //older
        wi->setName(xi18nc("Image Box widget", "Image Box"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of image box widgets. Based on that, identifiers such as "
                "image1, image2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "image"));
        wi->setDescription(xi18n("A widget for displaying images"));
    // wi->setCustomTypeForProperty("pixmapData", KexiCustomPropertyFactory::PixmapData);
        wi->setCustomTypeForProperty("pixmapId", KexiCustomPropertyFactory::PixmapId);
        wi->setInternalProperty("dontStartEditingOnInserting", true);
        wi->setAutoSaveProperties(QList<QByteArray>() << "pixmap");
        addClass(wi);
    }

    {
        KexiDataAwareWidgetInfo* wi = new KexiDataAwareWidgetInfo(this);
        wi->setIconName(KexiIconName("combobox"));
        wi->setClassName("KexiDBComboBox");
        wi->addAlternateClassName("QComboBox", true/*override*/);
        wi->addAlternateClassName("KComboBox", true/*override*/);
        wi->setName(xi18nc("Combo Box widget", "Combo Box"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of combo box widgets. Based on that, identifiers such as "
                "comboBox1, comboBox2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "comboBox"));
        wi->setDescription(xi18n("A combo box widget"));
        addClass(wi);
    }
    {
        KexiDataAwareWidgetInfo* wi = new KexiDataAwareWidgetInfo(this);
        wi->setIconName(KexiIconName("checkbox"));
        wi->setClassName("KexiDBCheckBox");
        wi->addAlternateClassName("QCheckBox", true/*override*/);
        wi->setName(xi18nc("Check Box widget", "Check Box"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of combo box widgets. Based on that, identifiers such as "
                "checkBox1, checkBox2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "checkBox"));
        wi->setDescription(xi18n("A check box with text label"));
        addClass(wi);
    }
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    {
// Unused, commented-out in Kexi 2.9 to avoid unnecessary translations:
//         KexiDataAwareWidgetInfo* wi = new KexiDataAwareWidgetInfo(this);
//         wi->setIconName(KexiIconName("autofield"));
//         wi->setClassName("KexiDBAutoField");
//         wi->addAlternateClassName("KexiDBFieldEdit", true/*override*/); //older
//         wi->setName(xi18n("Auto Field"));
//         wi->setNamePrefix(
//             i18nc("Widget name. This string will be used to name widgets of this class. "
//                   "It must _not_ contain white spaces and non latin1 characters", "autoField"));
//         wi->setDescription(xi18n("A widget containing an automatically selected editor "
//                                 "and a label to edit the value of a database field of any type."));
//         addClass(wi);
    }
#endif

    {
        KFormDesigner::WidgetInfo* wi = new KFormDesigner::WidgetInfo(this);
        wi->setClassName("KexiDBPushButton");
        wi->setIconName(KexiIconName("button"));
        wi->addAlternateClassName("KexiPushButton", true/*override*/);
        wi->addAlternateClassName("QPushButton", true/*override*/);
        wi->setName(xi18nc("Button widget", "Button"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of button widgets. Based on that, identifiers such as "
                "button1, button2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "button"));
        wi->setDescription(xi18n("A button for executing actions"));
        addClass(wi);
    }
    {
        KFormDesigner::WidgetInfo* wi = new KFormDesigner::WidgetInfo(this);
        wi->setClassName("KexiDBCommandLinkButton");
        wi->setIconName(KexiIconName("button"));
        wi->setName(xi18nc("Link Button widget", "Link Button"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of link button widgets. Based on that, identifiers such as "
                "linkButton1, linkButton2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "linkButton"));
        wi->setDescription(xi18n("A Link button for executing actions"));
        addClass(wi);
    }
    //! @todo radio button
#ifdef KEXI_SHOW_UNFINISHED
    {
        KFormDesigner::WidgetInfo *wi = new KFormDesigner::WidgetInfo(this);
        wi->setIconName(KexiIconName("radiobutton"));
        wi->setClassName("QRadioButton");
        wi->setName(/* no i18n needed */ "Option Button");
        wi->setNamePrefix(/* no i18n needed */ "option");
        wi->setDescription(/* no i18n needed */ "An option button with text or pixmap label");
        addClass(wi);
    }
    {
        KFormDesigner::WidgetInfo *wi = new KFormDesigner::WidgetInfo(this);
        wi->setIconName(KexiIconName("spinbox"));
        wi->setClassName("QSpinBox");
        wi->addAlternateClassName("KIntSpinBox");
        wi->setName(/* no i18n needed */ "Spin Box");
        wi->setNamePrefix(/* no i18n needed */ "spinBox");
        wi->setDescription(/* no i18n needed */ "A spin box widget");
        addClass(wi);
    }
#endif
    {
        KexiDataAwareWidgetInfo* wi = new KexiDataAwareWidgetInfo(this);
        wi->setClassName("KexiDBSlider");
        wi->setIconName(KexiIconName("slider"));
        wi->addAlternateClassName("QSlider", true/*override*/);
        wi->setName(xi18nc("Slider widget", "Slider"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of slider widgets. Based on that, identifiers such as "
                "slider1, slider2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "slider"));
        wi->setDescription(xi18n("A Slider widget"));
        addClass(wi);
    }
    {
        KexiDataAwareWidgetInfo* wi = new KexiDataAwareWidgetInfo(this);
        wi->setClassName("KexiDBProgressBar");
        wi->setIconName(KexiIconName("progressbar"));
        wi->addAlternateClassName("QProgressBar", true/*override*/);
        wi->setName(xi18nc("Progress Bar widget", "Progress Bar"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of progress bar widgets. Based on that, identifiers such as "
                "progressBar1, progressBar2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "progressBar"));
        wi->setDescription(xi18n("A Progress Bar widget"));
        addClass(wi);
    }
    {
        KFormDesigner::WidgetInfo *wi = new KFormDesigner::WidgetInfo(this);
        wi->setIconName(KexiIconName("line-horizontal"));
        wi->setClassName("KexiLineWidget");
        wi->addAlternateClassName("Line", true/*override*/);
        wi->setName(xi18n("Line"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of line widgets. Based on that, identifiers such as "
                "line1, line2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "line"));
        wi->setDescription(xi18n("A line to be used as a separator"));
        wi->setAutoSaveProperties(QList<QByteArray>() << "orientation");
        wi->setInternalProperty("orientationSelectionPopup", true);
        wi->setInternalProperty("orientationSelectionPopup:horizontalIcon", KexiIconName("line-horizontal"));
        wi->setInternalProperty("orientationSelectionPopup:verticalIcon", KexiIconName("line-vertical"));
        wi->setInternalProperty("orientationSelectionPopup:horizontalText", xi18n("Insert &Horizontal Line"));
        wi->setInternalProperty("orientationSelectionPopup:verticalText", xi18n("Insert &Vertical Line"));
        addClass(wi);
    }
    {
        KexiDataAwareWidgetInfo* wi = new KexiDataAwareWidgetInfo(this);
        wi->setClassName("KexiDBDatePicker");
        wi->setIconName(KexiIconName("dateedit"));
        wi->addAlternateClassName("QDateEdit", true/*override*/);
        wi->addAlternateClassName("KDateWidget", true/*override*/);
        wi->setName(xi18nc("Date Picker widget", "Date Picker"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of date picker widgets. Based on that, identifiers such as "
                "datePicker1, datePicker2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "datePicker"));
        wi->setDescription(xi18n("A Date Picker widget"));
        addClass(wi);
    }
    //! @todo time edit
#ifdef KEXI_SHOW_UNFINISHED
    {
        KFormDesigner::WidgetInfo *wi = new KFormDesigner::WidgetInfo(this);
        wi->setIconName(KexiIconName("timeedit"));
        wi->setClassName("QTimeEdit");
        wi->addAlternateClassName("KTimeWidget");
        wi->setName(/* no i18n needed */ "Time Widget");
        wi->setNamePrefix(/* no i18n needed */ "timeWidget");
        wi->setDescription(/* no i18n needed */ "A widget to input and display a time");
        wi->setAutoSaveProperties(QList<QByteArray>() << "time");
        addClass(wi);
    }
#endif
    //! @todo datetime edit
#ifdef KEXI_SHOW_UNFINISHED
    {
        KFormDesigner::WidgetInfo *wi = new KFormDesigner::WidgetInfo(this);
        wi->setIconName(KexiIconName("datetimeedit"));
        wi->setClassName("QDateTimeEdit");
        wi->addAlternateClassName("KDateTimeWidget");
        wi->setName(/* no i18n needed */ "Date/Time Widget");
        wi->setNamePrefix(/* no i18n needed */ "dateTimeWidget");
        wi->setDescription(/* no i18n needed */ "A widget to input and display a time and a date");
        wi->setAutoSaveProperties(QList<QByteArray>() << "dateTime");
        addClass(wi);
    }
#endif

    setPropertyDescription("echoMode",
        xi18nc("Property: Echo mode for Line Edit widget eg. Normal, NoEcho, Password", "Echo Mode"));
    setPropertyDescription("indent", xi18n("Indent"));

    setPropertyDescription("invertedAppearance", xi18n("Inverted"));
    setPropertyDescription("minimum", xi18n("Minimum"));
    setPropertyDescription("maximum", xi18n("Maximum"));
    setPropertyDescription("format", xi18n("Format"));
    setPropertyDescription("orientation", xi18n("Orientation"));
    setPropertyDescription("textDirection", xi18n("Text Direction"));
    setPropertyDescription("textVisible", xi18n("Text Visible"));
    setPropertyDescription("value", xi18n("Value"));
    setPropertyDescription("date", xi18n("Date"));
    setPropertyDescription("arrowVisible", xi18n("Arrow Visible"));
    setPropertyDescription("description", xi18n("Description"));
    setPropertyDescription("pageStep", xi18nc("Property of slider widgets", "Page Step"));
    setPropertyDescription("singleStep", xi18nc("Property of slider widgets", "Single Step"));
    setPropertyDescription("tickInterval", xi18nc("Property of slider widgets", "Tick Interval"));
    setPropertyDescription("tickPosition", xi18nc("Property of slider widgets", "Tick Position"));
    setPropertyDescription("showEditor", xi18n("Show Editor"));
    setPropertyDescription("formName", xi18n("Form Name"));
    setPropertyDescription("onClickAction", xi18n("On Click"));
    setPropertyDescription("onClickActionOption", xi18n("On Click Option"));
    setPropertyDescription("autoTabStops", xi18n("Auto Tab Order"));
    setPropertyDescription("checkSpellingEnabled", xi18n("Spell Checking"));
    setPropertyDescription("html", xi18nc("Widget Property", "HTML"));
    setPropertyDescription("lineWrapColumnOrWidth", xi18n("Line Wrap At"));
    setPropertyDescription("lineWrapMode", xi18n("Line Wrap Mode"));
    setPropertyDescription("spellCheckingLanguage", xi18n("Spell Checking Language"));

    setPropertyDescription("widgetType", xi18n("Editor Type"));
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    //for autofield's type: inherit i18n from KDb
    setValueDescription("Auto", futureI18nc("AutoField editor's type", "Auto"));
    setValueDescription("Text", KDbField::typeName(KDbField::Text));
    setValueDescription("Integer", KDbField::typeName(KDbField::Integer));
    setValueDescription("Double", KDbField::typeName(KDbField::Double));
    setValueDescription("Boolean", KDbField::typeName(KDbField::Boolean));
    setValueDescription("Date", KDbField::typeName(KDbField::Date));
    setValueDescription("Time", KDbField::typeName(KDbField::Time));
    setValueDescription("DateTime", KDbField::typeName(KDbField::DateTime));
    setValueDescription("MultiLineText", xi18nc("AutoField editor's type", "Multiline Text"));
    setValueDescription("ComboBox", xi18nc("AutoField editor's type", "Drop-Down List"));
    setValueDescription("Image", xi18nc("AutoField editor's type", "Image"));
#endif

    setValueDescription("NoTicks", xi18nc("Possible value of slider widget's \"Tick position\" property", "No Ticks"));
    setValueDescription("TicksAbove", xi18nc("Possible value of slider widget's \"Tick position\" property", "Above"));
    setValueDescription("TicksLeft", xi18nc("Possible value of slider widget's \"Tick position\" property", "Left"));
    setValueDescription("TicksBelow", xi18nc("Possible value of slider widget's \"Tick position\" property", "Below"));
    setValueDescription("TicksRight", xi18nc("Possible value of slider widget's \"Tick position\" property", "Right"));
    setValueDescription("TicksBothSides", xi18nc("Possible value of slider widget's \"Tick position\" property", "Both Sides"));

// auto field:
//    setPropertyDescription("autoCaption", futureI18n("Auto Label"));
//    setPropertyDescription("foregroundLabelColor", futureI18n("Label Text Color"));
//    setPropertyDescription("backgroundLabelColor", futureI18nc("(a property name, keep the text narrow!)",
//                                         "Label Background\nColor"));

//    setPropertyDescription("labelPosition", futureI18n("Label Position"));
//    setValueDescription("Left", futureI18nc("Label Position", "Left"));
//    setValueDescription("Top", futureI18nc("Label Position", "Top"));
//    setValueDescription("NoLabel", futureI18nc("Label Position", "No Label"));

    setPropertyDescription("sizeInternal", xi18n("Size"));
    setPropertyDescription("pixmapId", xi18n("Image"));
    setPropertyDescription("scaledContents", xi18n("Scaled Contents"));
    setPropertyDescription("smoothTransformation", xi18nc("Property: Smoothing when contents are scaled", "Smoothing"));
    setPropertyDescription("keepAspectRatio", xi18nc("Property: Keep Aspect Ratio (keep short)", "Keep Ratio"));

    //used in labels, frames...
    setPropertyDescription("dropDownButtonVisible",
        xi18nc("Drop-Down Button for Image Box Visible (a property name, keep the text narrow!)",
              "Drop-Down\nButton Visible"));

    //for checkbox
    setPropertyDescription("checked", xi18nc("Property: Checked checkbox", "Checked"));
    setPropertyDescription("tristate", xi18nc("Property: Tristate checkbox", "Tristate"));
    setValueDescription("TristateDefault", xi18nc("Value of \"Tristate\" property in checkbox: default", "Default"));
    setValueDescription("TristateOn", xi18nc("Value of \"Tristate\" property in checkbox: yes", "Yes"));
    setValueDescription("TristateOff", xi18nc("Value of \"Tristate\" property in checkbox: no", "No"));

    //for combobox
    setPropertyDescription("editable", xi18nc("Editable combobox", "Editable"));

    //for button
    setPropertyDescription("checkable", xi18nc("Property: Button is checkable", "On/Off"));
    setPropertyDescription("autoRepeat", xi18nc("Property: Button", "Auto Repeat"));
    setPropertyDescription("autoRepeatDelay", xi18nc("Property: Auto Repeat Button's Delay", "Auto Rep. Delay"));
    setPropertyDescription("autoRepeatInterval", xi18nc("Property: Auto Repeat Button's Interval", "Auto Rep. Interval"));
    // unused (too advanced) setPropertyDescription("autoDefault", xi18n("Auto Default"));
    // unused (too advanced) setPropertyDescription("default", xi18nc("Property: Button is default", "Default"));
    setPropertyDescription("flat", xi18nc("Property: Button is flat", "Flat"));
    setPropertyDescription("hyperlink" , xi18nc("Hyperlink address", "Hyperlink"));
    setPropertyDescription("hyperlinkType", xi18nc("Type of hyperlink", "Hyperlink Type"));
    setPropertyDescription("hyperlinkTool", xi18nc("Tool used for opening a hyperlink", "Hyperlink Tool"));
    setPropertyDescription("remoteHyperlink", xi18nc("Allow to open remote hyperlinks", "Remote Hyperlink"));
    setPropertyDescription("hyperlinkExecutable", xi18nc("Allow to open executables", "Executable Hyperlink"));

    setValueDescription("NoHyperlink", xi18nc("Hyperlink type, NoHyperlink", "No Hyperlink"));
    setValueDescription("StaticHyperlink", xi18nc("Hyperlink type, StaticHyperlink", "Static"));
    setValueDescription("DynamicHyperlink", xi18nc("Hyperlink type, DynamicHyperlink", "Dynamic"));

    setValueDescription("DefaultHyperlinkTool", xi18nc("Hyperlink tool, DefaultTool", "Default"));
    setValueDescription("BrowserHyperlinkTool", xi18nc("Hyperlink tool, BrowserTool", "Browser"));
    setValueDescription("MailerHyperlinkTool", xi18nc("Hyperlink tool, MailerTool", "Mailer"));

    //for label
    setPropertyDescription("textFormat", xi18n("Text Format"));
    setValueDescription("PlainText", xi18nc("For Text Format", "Plain"));
    setValueDescription("RichText", xi18nc("For Text Format", "Hypertext"));
    setValueDescription("AutoText", xi18nc("For Text Format", "Auto"));
    setValueDescription("LogText", xi18nc("For Text Format", "Log"));
    setPropertyDescription("openExternalLinks", xi18nc("property: Can open external links in label", "Open Ext. Links"));

    //for line edit
    setPropertyDescription("placeholderText", xi18nc("Property: line edit's placeholder text", "Placeholder Text"));
    setPropertyDescription("clearButtonEnabled", xi18nc("Property: Clear Button Enabled", "Clear Button"));
    //for EchoMode
    setPropertyDescription("passwordMode", xi18nc("Property: Password Mode for line edit", "Password Mode"));
    setPropertyDescription("squeezedTextEnabled", xi18nc("Property: Squeezed Text Mode for line edit", "Squeezed Text"));

    // text edit
    setPropertyDescription("tabStopWidth", xi18n("Tab Stop Width"));
    setPropertyDescription("tabChangesFocus", xi18n("Tab Changes Focus"));
    setPropertyDescription("wrapPolicy", xi18n("Word Wrap Policy"));
    setValueDescription("AtWordBoundary", xi18nc("Property: For Word Wrap Policy", "At Word Boundary"));
    setValueDescription("Anywhere", xi18nc("Property: For Word Wrap Policy", "Anywhere"));
    setValueDescription("AtWordOrDocumentBoundary", xi18nc("Property: For Word Wrap Policy", "At Word Boundary If Possible"));
    setPropertyDescription("wordWrap", xi18n("Word Wrapping"));
    setPropertyDescription("wrapColumnOrWidth", xi18n("Word Wrap Position"));
    setValueDescription("NoWrap", xi18nc("Property: For Word Wrap Position", "None"));
    setValueDescription("WidgetWidth", xi18nc("Property: For Word Wrap Position", "Widget's Width"));
    setValueDescription("FixedPixelWidth", xi18nc("Property: For Word Wrap Position", "In Pixels"));
    setValueDescription("FixedColumnWidth", xi18nc("Property: For Word Wrap Position", "In Columns"));
    setPropertyDescription("linkUnderline", xi18n("Links Underlined"));
    setPropertyDescription("horizontalScrollBarPolicy", xi18n("Horizontal Scroll Bar"));
    setPropertyDescription("verticalScrollBarPolicy", xi18n("Vertical Scroll Bar"));
    //ScrollBarPolicy
    setValueDescription("ScrollBarAsNeeded", xi18nc("Property: Show Scroll Bar As Needed", "As Needed"));
    setValueDescription("ScrollBarAlwaysOff", xi18nc("Property: Scroll Bar Always Off", "Always Off"));
    setValueDescription("ScrollBarAlwaysOn", xi18nc("Property: Scroll Bar Always On", "Always On"));
    setPropertyDescription("acceptRichText", xi18nc("Property: Text Edit accepts rich text", "Rich Text"));
    setPropertyDescription("HTML", xi18nc("Property: HTML value of text edit", "HTML"));

    // --- containers ---
    {
        KFormDesigner::WidgetInfo *wi = new KFormDesigner::WidgetInfo(this);
        wi->setIconName(KexiIconName("tabwidget"));
        wi->setClassName("KFDTabWidget");
        wi->addAlternateClassName("KTabWidget");
        wi->addAlternateClassName("QTabWidget");
        wi->setSavingName("QTabWidget");
        wi->setName(xi18n("Tab Widget"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of tab widgets. Based on that, identifiers such as "
                  "tab1, tab2 are generated. "
                  "This string can be used to refer the widget object as variables in programming "
                  "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                  "should start with lower case letter and if there are subsequent words, these should "
                  "start with upper case letter. Example: smallCamelCase. "
                  "Moreover, try to make this prefix as short as possible.",
                  "tabWidget"));
        wi->setDescription(xi18n("A widget to display multiple pages using tabs"));
        addClass(wi);
    }
    {
        KFormDesigner::WidgetInfo *wi = new KFormDesigner::WidgetInfo(this);
        wi->setIconName(KexiIconName("frame"));
        wi->setClassName("QWidget");
        wi->addAlternateClassName("ContainerWidget");
        wi->setName(/* no i18n needed */ "Basic container");
        wi->setNamePrefix(/* no i18n needed */ "container");
        wi->setDescription(/* no i18n needed */ "An empty container with no frame");
        addClass(wi);
    }
    {
        KFormDesigner::WidgetInfo* wi = new KFormDesigner::WidgetInfo(this);
        wi->setIconName(KexiIconName("frame"));
        wi->setClassName("KexiFrame");
        wi->addAlternateClassName("QFrame", true/*override*/);
        wi->addAlternateClassName("Q3Frame", true/*override*/);
        wi->setName(xi18nc("Frame widget", "Frame"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of frame widgets. Based on that, identifiers such as "
                "frame1, frame2 are generated. "
                "This string can be used to refer the widget object as variables in programming "
                "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                "should start with lower case letter and if there are subsequent words, these should "
                "start with upper case letter. Example: smallCamelCase. "
                "Moreover, try to make this prefix as short as possible.",
                "frame"));
        wi->setDescription(xi18n("A frame widget"));
        addClass(wi);
    }
    {
        KFormDesigner::WidgetInfo *wi = new KFormDesigner::WidgetInfo(this);
        wi->setIconName(KexiIconName("groupbox"));
        wi->setClassName("QGroupBox");
        wi->addAlternateClassName("GroupBox");
        wi->setName(xi18n("Group Box"));
        wi->setNamePrefix(
            xi18nc("A prefix for identifiers of group box widgets. Based on that, identifiers such as "
                  "groupBox1, groupBox2 are generated. "
                  "This string can be used to refer the widget object as variables in programming "
                  "languages or macros so it must _not_ contain white spaces and non latin1 characters, "
                  "should start with lower case letter and if there are subsequent words, these should "
                  "start with upper case letter. Example: smallCamelCase. "
                  "Moreover, try to make this prefix as short as possible.",
                  "groupBox"));
        wi->setDescription(xi18n("A container to group some widgets"));
        addClass(wi);
    }

    //groupbox
    setPropertyDescription("title", xi18nc("'Title' property for group box", "Title"));
    setPropertyDescription("flat", xi18nc("'Flat' property for group box", "Flat"));

    //tab widget
    setPropertyDescription("tabBarAutoHide", xi18n("Auto-hide Tabs"));
    setPropertyDescription("tabPosition", xi18n("Tab Position"));
    setPropertyDescription("currentIndex", xi18nc("'Current page' property for tab widget", "Current Page"));
    setPropertyDescription("tabShape", xi18n("Tab Shape"));
    setPropertyDescription("elideMode", xi18nc("Tab Widget's Elide Mode property", "Elide Mode"));
    setPropertyDescription("usesScrollButtons",
                           xi18nc("Tab Widget's property: true if can use scroll buttons", "Scroll Buttons"));

    setPropertyDescription("tabsClosable", xi18n("Closable Tabs"));
    setPropertyDescription("movable", xi18n("Movable Tabs"));
    setPropertyDescription("documentMode", xi18n("Document Mode"));

    setValueDescription("Rounded", xi18nc("Property value for Tab Shape", "Rounded"));
    setValueDescription("Triangular", xi18nc("Property value for Tab Shape", "Triangular"));
}

KexiMainFormWidgetsFactory::~KexiMainFormWidgetsFactory()
{
}

QWidget*
KexiMainFormWidgetsFactory::createWidget(const QByteArray &c, QWidget *p, const char *n,
                            KFormDesigner::Container *container,
                            CreateWidgetOptions options)
{
    QWidget *w = 0;
    const QString text(container->form()->library()->textForWidgetName(n, c));
    const bool designMode = options & KFormDesigner::WidgetFactory::DesignViewMode;
    bool createContainer = false;

    if (c == "KexiDBLineEdit" || c == "QLineEdit") {
        w = new KexiDBLineEdit(p);
    }
    else if (c == "KexiDBTextEdit" || c == "KTextEdit") {
        w = new KexiDBTextEdit(p);
    }
    else if (c == "Q3Frame" || c == "QFrame" || c == "KexiFrame") {
        w = new KexiFrame(p);
        createContainer = true;
    } else if (c == "KexiDBLabel" || c == "QLabel") {
        w = new KexiDBLabel(text, p);
    }
    else if (c == "KexiDBImageBox" || c == "KexiPictureLabel") {
        w = new KexiDBImageBox(designMode, p);
        connect(w, SIGNAL(idChanged(long)), this, SLOT(slotImageBoxIdChanged(long)));
    }
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    else if (c == "KexiDBAutoField") {
        w = new KexiDBAutoField(p);
    }
#endif
    else if (c == "KexiDBCheckBox" || c == "QCheckBox") {
        w = new KexiDBCheckBox(text, p);
    }
    else if (c == "KexiDBSlider" || c == "QSlider") {
        w = new KexiDBSlider(p);
    } else if (c == "KexiDBProgressBar" || c == "QProgressBar") {
        w = new KexiDBProgressBar(p);
    } else if (c == "KexiDBDatePicker" || c == "KDateWidget" || c == "QDateEdit") {
        w = new KexiDBDatePicker(p);
    }
    else if (c == "KexiDBComboBox" || c == "KComboBox") {
        w = new KexiDBComboBox(p);
    }
    else if (c == "QPushButton" || c == "KPushButton" || c == "KexiDBPushButton" || c == "KexiPushButton") {
        w = new KexiDBPushButton(text, p);
    }
    else if (c == "KexiDBCommandLinkButton" || c == "KexiCommandLinkButton") {
        w = new KexiDBCommandLinkButton(text, QString(), p);
    }
#ifdef KEXI_SHOW_UNFINISHED
    else if (c == "QRadioButton") {
        w = new QRadioButton(text, p);
    } else if (c == "KIntSpinBox" || c == "QSpinBox") {
        w = new QSpinBox(p);
    }
#endif
#ifdef KEXI_LIST_FORM_WIDGET_SUPPORT
    else if (c == "QTreeWidget") {
            QTreeWidget *tw = new QTreeWidget(p);
            w = tw;
            if (container->form()->interactiveMode()) {
                tw->setColumnCount(1);
                tw->setHeaderItem(new QTreeWidetItem(tw));
                tw->headerItem()->setText(1, futureI18n("Column 1"));
            }
            lw->setRootIsDecorated(true);
    } else if (c == "KTimeWidget" || c == "QTimeEdit") {
       w = new QTimeEdit(QTime::currentTime(), p);
    } else if (c == "KDateTimeWidget" || c == "QDateTimeEdit") {
        w = new QDateTimeEdit(QDateTime::currentDateTime(), p);
    }
#endif
    else if (c == "KexiLineWidget" || c == "Line") {
        w = new KexiLineWidget(options & WidgetFactory::VerticalOrientation
                ? Qt::Vertical : Qt::Horizontal, p);
    } // --- containers ---
    else if (c == "KFDTabWidget") {
        KFDTabWidget *tab = new KFDTabWidget(container, p);
        w = tab;
#if defined(USE_KTabWidget)
        tab->setTabReorderingEnabled(true);
        connect(tab, SIGNAL(movedTab(int,int)), this, SLOT(reorderTabs(int,int)));
#endif
        //qDebug() << "Creating ObjectTreeItem:";
        container->form()->objectTree()->addItem(container->objectTree(),
            new KFormDesigner::ObjectTreeItem(
                container->form()->library()->displayName(c), n, tab, container));
    } else if (c == "QWidget") {
        w = new ContainerWidget(p);
        w->setObjectName(n);
        (void)new KFormDesigner::Container(container, w, p);
        return w;
    } else if (c == "QGroupBox") {
        w = new GroupBox(text, p);
        createContainer = true;
    }

    if (w)
        w->setObjectName(n);
    if (createContainer) {
        (void)new KFormDesigner::Container(container, w, container);
    }
    if (c == "KFDTabWidget") {
        // if we are loading, don't add this tab
        if (container->form()->interactiveMode()) {
            TabWidgetBase *tab = qobject_cast<TabWidgetBase*>(w);
            AddTabAction(container, tab, 0).slotTriggered();
        }
    }
    return w;
}

bool KexiMainFormWidgetsFactory::createMenuActions(const QByteArray &classname, QWidget *w,
        QMenu *menu, KFormDesigner::Container *container)
{
    QWidget *pw = w->parentWidget();
    if (m_assignAction->isEnabled()) {
        /*! @todo also call createMenuActions() for inherited factory! */
        menu->addAction(m_assignAction);
        return true;
    } else if (classname == "KexiDBImageBox") {
        KexiDBImageBox *imageBox = static_cast<KexiDBImageBox*>(w);
        imageBox->contextMenu()->updateActionsAvailability();
        KActionCollection *ac = imageBox->contextMenu()->actionCollection();
        QMenu *subMenu = menu->addMenu(xi18n("&Image"));
//! @todo make these actions undoable/redoable
        subMenu->addAction(ac->action("insert"));
        subMenu->addAction(ac->action("file_save_as"));
        subMenu->addSeparator();
        subMenu->addAction(ac->action("edit_cut"));
        subMenu->addAction(ac->action("edit_copy"));
        subMenu->addAction(ac->action("edit_paste"));
        subMenu->addAction(ac->action("delete"));
        if (ac->action("properties")) {
            subMenu->addSeparator();
            subMenu->addAction(ac->action("properties"));
        }
    } else if (classname == "KexiDBLabel" || classname == "KexiDBTextEdit") {
        menu->addAction( new EditRichTextAction(container, w, menu, this) );
        return true;
    }
#ifdef KEXI_LIST_FORM_WIDGET_SUPPORT
    else if (classname == "QTreeWidget") {
        menu->addAction(koIcon("document-properties"), futureI18n("Edit Contents of List Widget"),
            this, SLOT(editListContents()));
        return true;
    }
#endif
    else if (classname == "KFDTabWidget" || pw->parentWidget()->inherits("QTabWidget")) {
//! @todo KEXI3 port this: setWidget(pw->parentWidget(), m_container->toplevel());
#if 0
        if (pw->parentWidget()->inherits("QTabWidget")) {
            setWidget(pw->parentWidget(), m_container->toplevel());
        }
#endif
        TabWidgetBase *tab = qobject_cast<TabWidgetBase*>(w);
        if (tab) {
            menu->addAction( new AddTabAction(container, tab, menu) );
            menu->addAction( new RenameTabAction(container, tab, menu) );
            menu->addAction( new RemoveTabAction(container, tab, menu) );
        }
        return true;
    }
    return false;
}

void
KexiMainFormWidgetsFactory::createCustomActions(KActionCollection* col)
{
    //this will create shared instance action for design mode (special collection is provided)
    col->addAction("widget_assign_action",
                   m_assignAction = new QAction(KexiIcon("form-action"), xi18n("&Assign Action..."), this));
}

bool
KexiMainFormWidgetsFactory::startInlineEditing(InlineEditorCreationArguments& args)
{
    const KFormDesigner::WidgetInfo* wclass = args.container->form()->library()->widgetInfoForClassName(args.classname);
    const KexiDataAwareWidgetInfo* wDataAwareClass = dynamic_cast<const KexiDataAwareWidgetInfo*>(wclass);
    if (wDataAwareClass && !wDataAwareClass->inlineEditingEnabledWhenDataSourceSet()) {
        KexiFormDataItemInterface* iface = dynamic_cast<KexiFormDataItemInterface*>(args.widget);
        if (iface && !iface->dataSource().isEmpty()) {
//! @todo reimplement inline editing for KexiDBLineEdit using combobox with data sources
            return false;
        }
    }

    if (args.classname == "KexiDBLineEdit") {
        QLineEdit *lineedit = static_cast<QLineEdit*>(args.widget);
        args.text = lineedit->text();
        args.alignment = lineedit->alignment();
        args.useFrame = true;
        return true;
    }
    else if (args.classname == "KexiDBTextEdit") {
        KTextEdit *textedit = static_cast<KTextEdit*>(args.widget);
//! @todo rich text?
        args.text = textedit->toPlainText();
        args.alignment = textedit->alignment();
        args.useFrame = true;
        args.multiLine = true;
//! @todo
#if 0
        //copy a few properties
        KTextEdit *ed = dynamic_cast<KTextEdit *>(editor(w));
        ed->setLineWrapMode(textedit->lineWrapMode());
        ed->setLineWrapColumnOrWidth(textedit->lineWrapColumnOrWidth());
        ed->setWordWrapMode(textedit->wordWrapMode());
        ed->setTabStopWidth(textedit->tabStopWidth());
        ed->setTextFormat(textedit->textFormat());
        ed->setHorizontalScrollBarPolicy(textedit->horizontalScrollBarPolicy());
        ed->setVerticalScrollBarPolicy(textedit->verticalScrollBarPolicy());
#endif
        return true;
    }
    else if (args.classname == "KexiDBPushButton") {
        KexiDBPushButton *push = static_cast<KexiDBPushButton*>(args.widget);
        QStyleOptionButton option;
        option.initFrom(push);
        args.text = push->text();
        const QRect r(push->style()->subElementRect(
                          QStyle::SE_PushButtonContents, &option, push));
        args.geometry = QRect(push->x() + r.x(), push->y() + r.y(), r.width(), r.height());
//! @todo this is typical alignment, can we get actual from the style?
        args.alignment = Qt::AlignCenter;
        //args.transparentBackground = true;
        return true;
    }
    else if (args.classname == "KexiDBCommandLinkButton" ){
        KexiDBCommandLinkButton *linkButton = static_cast<KexiDBCommandLinkButton*>(args.widget);
        QStyleOptionButton option;
        option.initFrom(linkButton);
        args.text = linkButton->text();
        const QRect r(linkButton->style()->subElementRect(
                        QStyle::SE_PushButtonContents, &option, linkButton));

        QFontMetrics fm(linkButton->font());
        args.geometry = QRect(linkButton->x() + linkButton->iconSize().width() + 6,
                              linkButton->y() + r.y(),
                              r.width()  - 6 - linkButton->iconSize().width(),
                              std::min(fm.height() + 14, linkButton->height() - 4));
        return true;
    }
#ifdef KEXI_SHOW_UNFINISHED
    else if (args.classname == "QRadioButton") {
        QRadioButton *radio = static_cast<QRadioButton*>(args.widget);
        QStyleOptionButton option;
        option.initFrom(radio);
        args.text = radio->text();
        const QRect r(radio->style()->subElementRect(
                          QStyle::SE_RadioButtonContents, &option, radio));
        args.geometry = QRect(
            radio->x() + r.x(), radio->y() + r.y(), r.width(), r.height());
        return true;
    }
#endif
    else if (args.classname == "KexiDBLabel") {
        KexiDBLabel *label = static_cast<KexiDBLabel*>(args.widget);
        if (label->textFormat() == Qt::RichText) {
            args.execute = false;
            EditRichTextAction(args.container, label, nullptr, this).trigger();
//! @todo
        } else {
            args.text = label->text();
            args.alignment = label->alignment();
            args.multiLine = label->wordWrap();
        }
        return true;
    }
    else if (   args.classname == "KexiDBDateEdit" || args.classname == "KexiDBDateTimeEdit"
             /*|| args.classname == "KexiDBTimeEdit" || classname == "KexiDBIntSpinBox" || classname == "KexiDBDoubleSpinBox"*/)
    {
        disableFilter(args.widget, args.container);
        return true;
    }
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    else if (args.classname == "KexiDBAutoField") {
        if (static_cast<KexiDBAutoField*>(args.widget)->hasAutoCaption())
            return false; // caption is auto, abort editing
        QLabel *label = static_cast<KexiDBAutoField*>(args.widget)->label();
        args.text = label->text();
        args.widget = label;
        args.geometry = label->geometry();
        args.alignment = label->alignment();
        return true;
    }
#endif
    else if (args.classname == "KexiDBCheckBox") {
        KexiDBCheckBox *cb = static_cast<KexiDBCheckBox*>(args.widget);
        QStyleOptionButton option;
        option.initFrom(cb);
        QRect r(cb->geometry());
        r.setLeft(
            r.left() + 2
            + cb->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, cb).width());
        args.text = cb->text();
        args.geometry = r;
        return true;
    }
    else if (args.classname == "KexiDBImageBox") {
        KexiDBImageBox *image = static_cast<KexiDBImageBox*>(args.widget);
        image->insertFromFile();
        args.execute = false;
        return true;
    }
    return false;
}

bool
KexiMainFormWidgetsFactory::previewWidget(const QByteArray &, QWidget *, KFormDesigner::Container *)
{
    return false;
}

bool
KexiMainFormWidgetsFactory::clearWidgetContent(const QByteArray & /*classname*/, QWidget *w)
{
    KexiFormDataItemInterface *iface = dynamic_cast<KexiFormDataItemInterface*>(w);
    if (iface) {
        iface->clear();
    }
    return true;
}

bool
KexiMainFormWidgetsFactory::isPropertyVisibleInternal(const QByteArray& classname, QWidget *w,
        const QByteArray& property, bool isTopLevel)
{
    //general
    bool ok = true;
    if (classname == "KexiDBPushButton" || classname == "KexiPushButton") {
        ok = property != "isDragEnabled"
#ifndef KEXI_SHOW_UNFINISHED
             && property != "onClickAction" /*! @todo reenable */
             && property != "onClickActionOption" /*! @todo reenable */
             && property != "iconSet" /*! @todo reenable */
             && property != "iconSize" /*! @todo reenable */
             && property != "stdItem" /*! @todo reenable stdItem */
#endif
             ;
        ok = KFormDesigner::WidgetFactory::advancedPropertiesVisible()
             || (ok && property != "autoDefault" && property != "default");
     } else if (classname == "KexiDBCommandLinkButton") {
        ok = property != "isDragEnabled"
             && property != "default"
             && property != "checkable"
             && property != "autoDefault"
             && property != "autoRepeat"
             && property != "autoRepeatDelay"
             && property != "autoRepeatInterval"
#ifndef KEXI_SHOW_UNFINISHED
             && property != "onClickAction" /*! @todo reenable */
             && property != "onClickActionOption" /*! @todo reenable */
             && property != "iconSet" /*! @todo reenable */
             && property != "iconSize" /*! @todo reenable */
             && property != "stdItem" /*! @todo reenable stdItem */
#endif
             ;
     } else if (classname == "KexiDBSlider") {
        ok = property != "sliderPosition"
             && property != "tracking";
     } else if (classname == "KexiDBProgressBar") {
        ok = property != "focusPolicy"
             && property != "value";
     } else if (classname == "KexiDBLineEdit" || classname == "QLineEdit")
        ok = property != "urlDropsEnabled"
             && property != "echoMode"
             && property != "clickMessage" // Replaced by placeholderText in 2.9,
                                           // kept for backward compatibility Kexi projects created with Qt < 4.7.
             && property != "showClearButton" // Replaced by clearButtonEnabled in 3.0,
                                              // kept for backward compatibility Kexi projects created with Qt 4.
#ifndef KEXI_SHOW_UNFINISHED
             && property != "inputMask"
             && property != "maxLength" //!< we may want to integrate this with db schema
#endif
             ;
    else if (classname == "KexiDBComboBox")
        ok = property != "autoCaption"
             && property != "labelPosition"
             && property != "widgetType"
             && property != "fieldTypeInternal"
             && property != "fieldCaptionInternal" //hide properties that come with KexiDBAutoField
#ifndef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
             && property != "foregroundLabelColor"
             && property != "backgroundLabelColor"
#endif
             ;
    else if (classname == "KexiDBTextEdit" || classname == "KTextEdit")
        ok = KFormDesigner::WidgetFactory::advancedPropertiesVisible()
             || (property != "undoDepth"
                 && property != "undoRedoEnabled" //always true!
                 && property != "dragAutoScroll" //always true!
                 && property != "overwriteMode" //always false!
                 && property != "resizePolicy"
                 && property != "autoFormatting" //too complex
                 && property != "documentTitle"
                 && property != "cursorWidth"
#ifndef KEXI_SHOW_UNFINISHED
                 && property != "paper"
#endif
                 && property != "textInteractionFlags"
//! @todo support textInteractionFlags property of QLabel and QTextEdit
             );
    else if (classname == "KexiDBForm")
        ok = property != "iconText"
             && property != "geometry" /*nonsense for toplevel widget; for size, "size" property is used*/;
    else if (classname == "KexiDBLabel" || classname == "QLabel")
        ok = property != "focusPolicy"
             && property != "textInteractionFlags"
             && property != "pixmap";
//! @todo support textInteractionFlags property of QLabel
    else if (classname == "KexiLineWidget" || classname == "Line") {
        ok = property != "frameShape" && property != "font" && property != "margin";
    }
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    else if (classname == "KexiDBAutoField") {
        if (!isTopLevel && property == "caption")
            return true; //force
        if (property == "fieldTypeInternal" || property == "fieldCaptionInternal"
//! @todo unhide in 2.0
                || property == "widgetType")
            return false;
        ok = property != "text"; /* "text" is not needed as "caption" is used instead */
    }
#endif
    else if (classname == "KexiDBImageBox" || classname == "KexiPictureLabel") {
        ok = property != "font" && property != "pixmapId"
             && property != "text" && property != "indent" && property != "textFormat";
    }
    else if (classname == "KexiDBCheckBox" || classname == "QCheckBox") {
        //hide text property if the widget is a child of an autofield beause there's already "caption" for this purpose
        if (property == "text" && w && dynamic_cast<KFormDesigner::WidgetWithSubpropertiesInterface*>(w->parentWidget()))
            return false;
        ok = KFormDesigner::WidgetFactory::advancedPropertiesVisible() || property != "autoRepeat";
    }
#ifdef KEXI_SHOW_UNFINISHED
    else if (classname == "QRadioButton") {
        ok = KFormDesigner::WidgetFactory::advancedPropertiesVisible() || (property != "autoRepeat");
    }
#endif
    else if (classname == "KexiDBDatePicker") {
        ok = property != "closeButton"
             && property != "fontSize";
    } else if (classname == "QGroupBox") {
        ok =
#ifndef KEXI_SHOW_UNFINISHED
            /*! @todo Hidden for now in Kexi. "checkable" and "checked" props need adding
            a fake properties which will allow to properly work in design mode, otherwise
            child widgets become frozen when checked==true */
            (KFormDesigner::WidgetFactory::advancedPropertiesVisible() || (property != "checkable" && property != "checked")) &&
#endif
            true;
    } else if (classname == "KFDTabWidget") {
        ok = (KFormDesigner::WidgetFactory::advancedPropertiesVisible()
             || (property != "tabReorderingEnabled"
                 && property != "hoverCloseButton"
                 && property != "hoverCloseButtonDelayed"));
    }
    return ok && KexiDBFactoryBase::isPropertyVisibleInternal(classname, w, property, isTopLevel);
}

bool
KexiMainFormWidgetsFactory::propertySetShouldBeReloadedAfterPropertyChange(const QByteArray& classname,
        QWidget *w, const QByteArray& property)
{
    Q_UNUSED(classname);
    Q_UNUSED(w);
    return property == "fieldTypeInternal" || property == "widgetType"
           || property == "paletteBackgroundColor" || property == "autoFillBackground";
}

bool KexiMainFormWidgetsFactory::changeInlineText(KFormDesigner::Form *form, QWidget *widget,
    const QString &text, QString &oldText)
{
    const QByteArray n(widget->metaObject()->className());
    if (false) {
    }
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    else if (n == "KexiDBAutoField") {
        oldText = widget->property("caption").toString();
        changeProperty(form, widget, "caption", text);
        return true;
    }
#endif
#ifdef KEXI_SHOW_UNFINISHED
    else if (n == "KIntSpinBox") {
        oldText = QString::number(qobject_cast<QSpinBox*>(widget)->value());
        qobject_cast<QSpinBox*>(widget)->setValue(text.toInt());
    }
#endif
    else {
        oldText = widget->property("text").toString();
        changeProperty(form, widget, "text", text);
        return true;
    }
//! @todo check field's geometry
    return false;
}

void
KexiMainFormWidgetsFactory::resizeEditor(QWidget *editor, QWidget *w, const QByteArray &classname)
{
    QSize s = w->size();
    QPoint p = w->pos();
    QRect r;

    if (classname == "KexiDBCheckBox") {
        QStyleOptionButton option;
        option.initFrom(w);
        r = w->style()->subElementRect(QStyle::SE_CheckBoxContents, &option, w);
        p += r.topLeft();
        s.setWidth(r.width());
    } else if (classname == "KexiDBPushButton") {
        QStyleOptionButton option;
        option.initFrom(w);
        r = w->style()->subElementRect(QStyle::SE_PushButtonContents, &option, w);
        p += r.topLeft();
        s = r.size();
    }
#ifdef KEXI_SHOW_UNFINISHED
    else if (classname == "QRadioButton") {
        QStyleOptionButton option;
        option.initFrom(w);
        r = w->style()->subElementRect(
                QStyle::SE_RadioButtonContents, &option, w);
        p += r.topLeft();
        s.setWidth(r.width());
#endif

    editor->resize(s);
    editor->move(p);

    //! @todo KEXI3
    /* from ContainerFactory::resizeEditor(QWidget *editor, QWidget *widget, const QByteArray &):
        QSize s = w->size();
        editor->move(w->x() + 2, w->y() - 5);
        editor->resize(s.width() - 20, w->fontMetrics().height() + 10); */
    
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    if (classname == "KexiDBAutoField") {
        editor->setGeometry(static_cast<KexiDBAutoField*>(w)->label()->geometry());
    }
#endif
}

bool KexiMainFormWidgetsFactory::saveSpecialProperty(const QByteArray &classname,
    const QString &name, const QVariant &, QWidget *w, QDomElement &parentNode,
    QDomDocument &domDoc)
{
    Q_UNUSED(classname)
    if (false) {
    }
/* TODO
    else if (name == "list_items" && classname == "KexiDBComboBox") {
        KexiDBComboBox *combo = qobject_cast<KexiDBComboBox*>(w);
        for (int i = 0; i < combo->row; i++) {
            QDomElement item = domDoc.createElement("item");
            KFormDesigner::FormIO::savePropertyElement(item, domDoc, "property", "text", combo->itemText(i));
            parentNode.appendChild(item);
        }
        return true;
    }
*/
#ifdef KEXI_LIST_FORM_WIDGET_SUPPORT
    else if (name == "list_contents" && classname == "QTreeWidget") {
        QTreeWidget *treewidget = qobject_cast<QTreeWidget*>(w);
        // First we save the columns
        QTreeWidgetItem *headerItem = treewidget->headerItem();
        if (headerItem) {
            for (int i = 0; i < treewidget->columnCount(); i++) {
                QDomElement item = domDoc.createElement("column");
                KFormDesigner::FormIO::savePropertyElement(
                    item, domDoc, "property", "text", headerItem->text(i));
                KFormDesigner::FormIO::savePropertyElement(
                    item, domDoc, "property", "width", treewidget->columnWidth(i));
                KFormDesigner::FormIO::savePropertyElement(
                    item, domDoc, "property", "resizable", treewidget->header()->isResizeEnabled(i));
                KFormDesigner::FormIO::savePropertyElement(
                    item, domDoc, "property", "clickable", treewidget->header()->isClickEnabled(i));
                KFormDesigner::FormIO::savePropertyElement(
                    item, domDoc, "property", "fullwidth", treewidget->header()->isStretchEnabled(i));
                parentNode.appendChild(item);
            }
        }
        // Then we save the list view items
        QTreeWidgetItem *item = listwidget->firstChild();
        while (item) {
            saveListItem(item, parentNode, domDoc);
            item = item->nextSibling();
        }
        return true;
    }
#endif
    else if (name == "title" && w->parentWidget()->parentWidget()->inherits("QTabWidget")) {
        TabWidgetBase *tab = qobject_cast<TabWidgetBase*>(w->parentWidget()->parentWidget());
        KFormDesigner::FormIO::savePropertyElement(
            parentNode, domDoc, "attribute", "title", tab->tabText(tab->indexOf(w)));
    }
    return true;
}

#ifdef KEXI_LIST_FORM_WIDGET_SUPPORT
void KexiMainFormWidgetsFactory::saveListItem(QListWidgetItem *item,
                                              QDomNode &parentNode, QDomDocument &domDoc)
{
    QDomElement element = domDoc.createElement("item");
    parentNode.appendChild(element);

    // We save the text of each column
    for (int i = 0; i < item->listWidget()->columns(); i++) {
        KFormDesigner::FormIO::savePropertyElement(
            element, domDoc, "property", "text", item->text(i));
    }

    // Then we save every sub items
    QListWidgetItem *child = item->firstChild();
    while (child) {
        saveListItem(child, element, domDoc);
        child = child->nextSibling();
    }
}

void KexiMainFormWidgetsFactory::readTreeItem(
    QDomElement &node, QTreeWidgetItem *parent, QTreeWidget *treewidget)
{
    QTreeWidgetItem *item;
    if (parent)
        item = new QTreeWidgetItem(parent);
    else
        item = new QTreeWidgetItem(treewidget);

    // We need to move the item at the end of the list
    QTreeWidgetItem *last;
    if (parent)
        last = parent->firstChild();
    else
        last = treewidget->firstChild();

    while (last->nextSibling())
        last = last->nextSibling();
    item->moveItem(last);

    int i = 0;
    for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
        QDomElement childEl = n.toElement();
        QString prop = childEl.attribute("name");
        QString tag = childEl.tagName();

        // We read sub items
        if (tag == "item") {
            item->setOpen(true);
            readListItem(childEl, item, treewidget);
        }
        // and column texts
        else if (tag == "property" && prop == "text") {
            QVariant val = KFormDesigner::FormIO::readPropertyValue(
                n.firstChild(), treewidget, "item");
            item->setText(i, val.toString());
            i++;
        }
    }
}

void KexiMainFormWidgetsFactory::editListContents()
{
    if (widget()->inherits("QTreeWidget"))
        editTreeWidget(qobject_cast<QTreeWidget*>(widget()));
}
#endif

bool KexiMainFormWidgetsFactory::readSpecialProperty(const QByteArray &classname,
                                                     QDomElement &node, QWidget *w,
                                                     KFormDesigner::ObjectTreeItem *item)
{
    Q_UNUSED(classname)
    const QString tag(node.tagName());
    const QString name(node.attribute("name"));
//    KFormDesigner::Form *form = item->container()
//            ? item->container()->form() : item->parent()->container()->form();

    if (false) {
    }
/* TODO
    else if (tag == "item" && classname == "KComboBox") {
        KComboBox *combo = qobject_cast<KComboBox*>(w);
        QVariant val = KFormDesigner::FormIO::readPropertyValue(
                    form, node.firstChild().firstChild(), w, name);
        if (val.canConvert(QVariant::Pixmap))
            combo->addItem(val.value<QPixmap>(), QString());
        else
            combo->addItem(val.toString());
        return true;
    }*/
#ifdef KEXI_LIST_FORM_WIDGET_SUPPORT
    else if (tag == "column" && classname == "QTreeWidget") {
        QTreeWidget *tw = qobject_cast<QTreeWidget*>(w);
        int id = 0;
        for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
            QString prop = n.toElement().attribute("name");
            QVariant val = KFormDesigner::FormIO::readPropertyValue(n.firstChild(), w, name);
            if (prop == "text")
                id = tw->addColumn(val.toString());
            else if (prop == "width")
                tw->setColumnWidth(id, val.toInt());
            else if (prop == "resizable")
                tw->header()->setResizeEnabled(val.toBool(), id);
            else if (prop == "clickable")
                tw->header()->setClickEnabled(val.toBool(), id);
            else if (prop == "fullwidth")
                tw->header()->setStretchEnabled(val.toBool(), id);
        }
        return true;
    }
    else if (tag == "item" && classname == "QTreeWidget") {
        QTreeWidget *tw = qobject_cast<QTreeWidget*>(w);
        readListItem(node, 0, tw);
        return true;
    }
#endif
    else if (name == "title" && item->parent()->widget()->inherits("QTabWidget")) {
        TabWidgetBase *tab = qobject_cast<TabWidgetBase*>(w->parentWidget());
        tab->addTab(w, node.firstChild().toElement().text());
        item->addModifiedProperty("title", node.firstChild().toElement().text());
        return true;
    }
    return false;
}

void KexiMainFormWidgetsFactory::setPropertyOptions(KPropertySet& set,
                                                    const KFormDesigner::WidgetInfo& info,
                                                    QWidget *w)
{
    Q_UNUSED(info);
    Q_UNUSED(w);
    if (set.contains("indent")) {
        set["indent"].setOption("min", -1);
        set["indent"].setOption("minValueText", xi18nc("default indent value", "default"));
    }
}

void KexiMainFormWidgetsFactory::reorderTabs(int oldpos, int newpos)
{
    KFDTabWidget *tabWidget = qobject_cast<KFDTabWidget*>(sender());
    KFormDesigner::ObjectTreeItem *tab
            = tabWidget->container()->form()->objectTree()->lookup(tabWidget->objectName());
    if (!tab)
        return;

    tab->children()->move(oldpos, newpos);
}

KFormDesigner::ObjectTreeItem* KexiMainFormWidgetsFactory::selectableItem(
                                                KFormDesigner::ObjectTreeItem* item)
{
    if (item->parent() && item->parent()->widget()) {
        if (qobject_cast<QTabWidget*>(item->parent()->widget())) {
            // tab widget's page
            return item->parent();
        }
    }
    return item;
}

void KexiMainFormWidgetsFactory::slotImageBoxIdChanged(KexiBLOBBuffer::Id_t id)
{
    KexiFormView *formView = KDbUtils::findParent<KexiFormView*>((QWidget*)sender());
    if (formView) {
        changeProperty(formView->form(), formView, "pixmapId", int(/*! @todo unsafe */id));
        formView->setUnsavedLocalBLOB(formView->form()->selectedWidget(), id);
    }
}

#include "KexiMainFormWidgetsFactory.moc"
