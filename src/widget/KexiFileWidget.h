/* This file is part of the KDE project
   Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIFILEWIDGET_H
#define KEXIFILEWIDGET_H

#include <config-kexi.h>
#include "KexiStartupFileHandler.h"

#ifdef KEXI_USE_KFILEWIDGET

#include <KFileWidget>
#include <KexiFileFilters.h>

//! @short Widget for opening/saving files supported by Kexi
/*! For simplicity, initially the widget has hidden the preview pane. */
class KEXIEXTWIDGETS_EXPORT KexiFileWidget : public KFileWidget
{
    Q_OBJECT

public:
    //! @todo KEXI3 add equivalent of kfiledialog:/// for startDirOrVariable
    KexiFileWidget(
        const QUrl &startDirOrVariable, KexiFileFilters::Mode mode, QWidget *parent);

    virtual ~KexiFileWidget();

    using KFileWidget::setMode;

    KexiFileFilters::Mode mode() const;

    void setMode(KexiFileFilters::Mode mode);

    //! @return additional mime types
    QStringList additionalMimeTypes() const;

    //! Sets additional mime types, e.g. "text/x-csv"
    void setAdditionalMimeTypes(const QStringList &mimeTypes);

    //! @return excluded mime types
    QStringList excludedMimeTypes() const;

    //! Set excluded mime types
    void setExcludedMimeTypes(const QStringList &mimeTypes);

    //! @return selected file.
    //! @note Call checkSelectedFile() first
    virtual QString highlightedFile() const;

    //! just sets locationWidget()->setCurrentText(fn)
    //! (and something similar on win32)
    void setLocationText(const QString& fn);

    //! Sets default extension which will be added after accepting
    //! if user didn't provided one. This method is usable when there is
    //! more than one filter so there is no rule what extension should be selected
    //! (by default first one is selected).
    void setDefaultExtension(const QString& ext);

    /*! \return true if the current URL meets requies constraints
    (i.e. exists or doesn't exist);
    shows appropriate message box if needed. */
    bool checkSelectedFile();

    /*! If true, user will be asked to accept overwriting existing file.
    This is true by default. */
    void setConfirmOverwrites(bool set);

public Q_SLOTS:
    virtual void showEvent(QShowEvent * event);
    virtual void focusInEvent(QFocusEvent *);

    //! Typing a file that doesn't exist closes the file dialog, we have to
    //! handle this case better here.
    virtual void accept();

Q_SIGNALS:
    void fileHighlighted();
    void rejected();

protected Q_SLOTS:
    virtual void reject();
    void slotExistingFileHighlighted(const QUrl& url);

private:
    void updateFilters();

    class Private;
    Private * const d;
};

#endif // KEXI_USE_KFILEWIDGET
#endif // KEXIFILEWIDGET_H
