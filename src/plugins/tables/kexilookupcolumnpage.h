/* This file is part of the KDE project
   Copyright (C) 2006-2016 Jarosław Staniek <staniek@kde.org>

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
#ifndef KEXILOOKUPCOLUMNPAGE_H
#define KEXILOOKUPCOLUMNPAGE_H

#include <QWidget>

#include <KDbField>
#include <KDbUtils>

#include <KPropertySet>

class KexiProject;

//! @short A page within table designer's property pane, providing lookup column editor.
/*! It's data model is basically KDbLookupFieldSchema class, but the page does
 not create it directly but instead updates a property set that defines
 the field currently selected in the designer.

 @todo not all features of KDbLookupFieldSchema class are displayed on this page yet
 */
class KexiLookupColumnPage : public QWidget
{
    Q_OBJECT

public:
    explicit KexiLookupColumnPage(QWidget *parent = 0);
    virtual ~KexiLookupColumnPage();

public Q_SLOTS:
    void setProject(KexiProject *prj);
    void clearRowSourceSelection(bool alsoClearComboBox = true);
    void clearBoundColumnSelection();
    void clearVisibleColumnSelection();

    //! Receives a pointer to a new property \a set (from KexiFormView::managerPropertyChanged())
    void assignPropertySet(KPropertySet* propertySet);

Q_SIGNALS:
    //! Signal emitted when helper button 'Go to selected record sourcesource' is clicked.
    void jumpToObjectRequested(const QString& mime, const QString& name);

protected Q_SLOTS:
    void slotRowSourceTextChanged(const QString &text);
    void slotRowSourceChanged();
    void slotGotoSelectedRowSource();
    void slotBoundColumnTextChanged(const QString &text);
    void slotBoundColumnSelected();
    void slotVisibleColumnTextChanged(const QString &text);
    void slotVisibleColumnSelected();

protected:
    void updateBoundColumnWidgetsAvailability();

    //! Used instead of m_propertySet->changeProperty() to honor m_propertySetEnabled
    void changeProperty(const QByteArray &property, const QVariant &value);

    QWidget* addWidgetSpacer();

private:
    class Private;
    Private* const d;
};

#endif
