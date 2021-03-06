/* This file is part of the KDE project
   Copyright (C) 2004 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXINAMEWIDGET_H
#define KEXINAMEWIDGET_H

#include "kexiextwidgets_export.h"

#include <QLabel>

class QLineEdit;
class KDbValidator;

//! A widget displaying object's name and caption and allowing editing.
//! @see KexiNameDialog
class KEXIEXTWIDGETS_EXPORT KexiNameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KexiNameWidget(const QString& message, QWidget * parent = 0);

    KexiNameWidget(const QString& message,
                   const QString& nameLabel, const QString& nameText,
                   const QString& captionLabel, const QString& captionText,
                   QWidget * parent = 0);

    virtual ~KexiNameWidget();

    QLabel* captionLabel() const;
    QLabel* nameLabel() const;
    QLineEdit* captionLineEdit() const;
    QLineEdit* nameLineEdit() const;

    QString messageText() const;
    void setMessageText(const QString& msg);

    //! \return entered caption text
    QString captionText() const;

    void setCaptionText(const QString& capt);
    //! \return entered name text, always in lower case

    QString nameText() const;

    QString originalNameText() const;

    void setNameText(const QString& name);

    /*! Sets i18n'ed warning message displayed when user leaves 'name' field
     without filling it (if acceptsEmptyValue() is false).
     By default the message is equal "Please enter the name.". */
    void setWarningForName(const QString& txt);

    /*! Sets i18n'ed warning message displayed when user leaves 'name' field
     without filling it (if acceptsEmptyValue() is false).
     By default the message is equal "Please enter the caption." */
    void setWarningForCaption(const QString& txt);

    /*! \return true if name or caption is empty. */
    bool empty() const;

    KDbValidator *nameValidator() const;

    /*! Adds subvalidator for name field. In fact it is added to internal
     multivalidator. If \a owned is true, \a validator will be owned by the object.
     \sa KDbMultiValidator::addSubvalidator(). */
    void addNameSubvalidator(KDbValidator* validator, bool owned = true);

    /*! \return true if name text cannot be empty (true by default). */
    bool isNameRequired() const;

    void setNameRequired(bool set);

    /*! \return true if caption text cannot be empty (false by default). */
    bool isCaptionRequired() const;

    void setCaptionRequired(bool set);

public Q_SLOTS:
    /*! Clears both name and caption. */
    virtual void clear();

    /*! Checks if both fields have valid values
     (i.e. not empty if acceptsEmptyValue() is false).
     If not, warning message is shown and false is returned. */
    bool checkValidity();

Q_SIGNALS:
    /*! Emitted whenever return key is pressed on name or caption label. */
    void returnPressed();

    /*! Emitted whenever the caption or the name text changes */
    void textChanged();

    /*! Emitted whenever the message changes */
    void messageChanged();

protected Q_SLOTS:
    void slotNameTextChanged(const QString&);
    void slotCaptionTextChanged(const QString&);

protected:
    void init(
        const QString& message,
        const QString& nameLabel, const QString& nameText,
        const QString& captionLabel, const QString& captionText);

    QLabel *messageLabel() const;

private:
    class Private;
    Private * const d;

    friend class KexiNameDialog;
};

#endif
