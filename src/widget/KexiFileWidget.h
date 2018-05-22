/* This file is part of the KDE project
   Copyright (C) 2003-2018 Jarosław Staniek <staniek@kde.org>

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

#ifdef KEXI_USE_KFILEWIDGET

#include "KexiFileWidgetInterface.h"
#include <KFileWidget>

//! @short Widget for opening/saving files supported by Kexi
/*! For simplicity, initially the widget has hidden the preview pane. */
class KEXIEXTWIDGETS_EXPORT KexiFileWidget : public KFileWidget, public KexiFileWidgetInterface
{
    Q_OBJECT

public:
    //! @todo KEXI3 add equivalent of kfiledialog:/// for startDirOrVariable
    KexiFileWidget(const QUrl &startDirOrVariable, KexiFileFilters::Mode mode,
                   const QString &fileName, QWidget *parent = nullptr);

    KexiFileWidget(const QUrl &startDirOrVariable, KexiFileFilters::Mode mode,
                   QWidget *parent = nullptr);

    ~KexiFileWidget() override;

    using KFileWidget::setMode;

    /**
     * Returns the full path of the selected file in the local filesystem.
     * (Local files only)
     */
    QString selectedFile() const override;

    /**
     * Returns the full path of the highlighted file in the local filesystem.
     * (Local files only)
     */
    QString highlightedFile() const override;

    /**
     * @return the currently shown directory.
     */
    QString currentDir() const override;

public Q_SLOTS:
    void setMode(KexiFileFilters::Mode mode);

    //! Just sets locationWidget()->setCurrentText(text)
    //void setLocationText(const QString& text) override;

    /**
     * Sets the file name to preselect to @p name
     *
     * This takes absolute URLs and relative file names.
     */
    void setSelectedFile(const QString &name) override;

    //! Typing a file that doesn't exist closes the file dialog, we have to
    //! handle this case better here.
    virtual void accept();

    /**
     * Sets whether the line edit draws itself with a frame.
     */
    void setWidgetFrame(bool set) override;

Q_SIGNALS:
    void fileHighlighted(const QString &name);
    void fileSelected(const QString &name);
    void rejected();

protected Q_SLOTS:
    virtual void reject();
    void slotFileHighlighted(const QUrl& url);
    void slotFileSelected(const QUrl& url);

protected:
    virtual void showEvent(QShowEvent *event);

    virtual void focusInEvent(QFocusEvent *event);

    /**
     * Updates filters in the widget based on current filter selection.
     */
    void updateFilters() override;

    void applyEnteredFileName() override;

    QStringList currentFilters() const override;

private:
    class Private;
    Private * const d;
};

#endif // KEXI_USE_KFILEWIDGET
#endif // KEXIFILEWIDGET_H
