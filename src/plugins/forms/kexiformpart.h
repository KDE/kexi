/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005-2017 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIFORMPART_H
#define KEXIFORMPART_H

#include <kexi.h>
#include <kexipart.h>
#include <kexiblobbuffer.h>
#include <KexiWindowData.h>
#include <kexiformview.h>

#include <KDbTableSchemaChangeListener>

class QDomDocument;
namespace KFormDesigner
{
class Form;
class WidgetTreeWidget;
}
class KDbFieldList;
class KexiDataSourcePage;

class KexiFormPartTempData : public KexiWindowData, public KDbTableSchemaChangeListener
{
    Q_OBJECT
public:
    KexiFormPartTempData(KexiWindow* parent, KDbConnection *conn);
    ~KexiFormPartTempData();

    //! Sets data source used for this data.
    //! If the previous data source is different and is not empty, listener for it will be unregistered.
    //! If the new data source is empty this temp-data object will be registered as a listener for it.
    void setDataSource(const QString &pluginId, const QString &dataSource);

    QPointer<KFormDesigner::Form> form;
    QPointer<KFormDesigner::Form> previewForm;
    QString tempForm;
    QPoint scrollViewContentsPos; //!< to preserve contents pos after switching
    //! Used in KexiFormView::setUnsavedLocalBLOBs()
    QHash<QWidget*, KexiBLOBBuffer::Id_t> unsavedLocalBLOBs;
    //! Used when loading a form from (temporary) XML in Data View
    //! to get unsaved blobs collected at design mode.
    QHash<QByteArray, KexiBLOBBuffer::Id_t> unsavedLocalBLOBsByName;

protected:
    //! This temp-data acts as a listener for tracking changes in table schema
    //! used by the form. This method closes the form on request.
    tristate closeListener() override;

private:
    Q_DISABLE_COPY(KexiFormPartTempData)
    class Private;
    Private * const d;
};

//! Kexi Form Plugin
/*! It just creates a \ref KexiFormView. See there for most of code. */
class KEXIFORMUTILS_EXPORT KexiFormPart : public KexiPart::Part
{
    Q_OBJECT

public:
    KexiFormPart(QObject *parent, const QVariantList &);
    virtual ~KexiFormPart();

    KexiDataSourcePage* dataSourcePage() const;

    KFormDesigner::WidgetTreeWidget* widgetTreePage() const;

#ifndef KEXI_NO_FORM_DATASOURCE_WIZARD
    void generateForm(KDbFieldList *list, QDomDocument &domDoc);
#endif

    virtual KLocalizedString i18nMessage(const QString& englishMessage,
                                         KexiWindow* window) const;

protected:
    KexiWindowData* createWindowData(KexiWindow* window) override Q_REQUIRED_RESULT;

    KexiView *createView(QWidget *parent, KexiWindow *window, KexiPart::Item *item,
                         Kexi::ViewMode viewMode = Kexi::DataViewMode,
                         QMap<QString, QVariant> *staticObjectArgs = nullptr) override Q_REQUIRED_RESULT;

    virtual void initPartActions();
    virtual void initInstanceActions();
    virtual void setupPropertyPane(KexiPropertyPaneWidget *pane);

private:
    class Private;
    Private* d;
};

#endif

