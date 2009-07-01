/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2006-2009 Jarosław Staniek <staniek@kde.org>

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

#ifndef CONTAINERFACTORY_H
#define CONTAINERFACTORY_H

#include "commands.h"
#include "widgetfactory.h"
#include "utils.h"
#include <QGroupBox>
#include <QMenu>
#include <QPaintEvent>

namespace KFormDesigner
{
class Form;
class Container;
}

class InsertPageCommand : public KFormDesigner::Command
{
public:
    InsertPageCommand(KFormDesigner::Container *container, QWidget *widget);

    virtual void execute();
    virtual void undo();

protected:
    KFormDesigner::Form *m_form;
    QString  m_containername;
    QString  m_name;
    QString  m_parentname;
};

//! Helper widget (used when using 'Lay out horizontally')
class HBox : public QFrame
{
    Q_OBJECT

public:
    HBox(QWidget *parent);
    virtual ~HBox();
    void setPreviewMode() {
        m_preview = true;
    }
    virtual void paintEvent(QPaintEvent *ev);

protected:
    bool  m_preview;
};

//! Helper widget (used when using 'Lay out vertically')
class VBox : public QFrame
{
    Q_OBJECT

public:
    VBox(QWidget *parent);
    virtual ~VBox();
    void setPreviewMode() {
        m_preview = true;
    }
    virtual void paintEvent(QPaintEvent *ev);

protected:
    bool  m_preview;
};

//! Helper widget (used when using 'Lay out in a grid')
class Grid : public QFrame
{
    Q_OBJECT

public:
    Grid(QWidget *parent);
    virtual ~Grid();
    void setPreviewMode() {
        m_preview = true;
    }
    virtual void paintEvent(QPaintEvent *ev);

protected:
    bool  m_preview;
};

//! Helper widget (used when using 'Lay out with horizontal flow')
class HFlow : public QFrame
{
    Q_OBJECT

public:
    HFlow(QWidget *parent);
    virtual ~HFlow();
    void setPreviewMode() {
        m_preview = true;
    }
    virtual void paintEvent(QPaintEvent *ev);

protected:
    bool  m_preview;
};

//! Helper widget (used when using 'Lay out with horizontal flow')
class VFlow : public QFrame
{
    Q_OBJECT

public:
    VFlow(QWidget *parent);
    virtual ~VFlow();
    void setPreviewMode() {
        m_preview = true;
    }
    virtual void paintEvent(QPaintEvent *ev);
    virtual QSize  sizeHint() const;

protected:
    bool  m_preview;
};

//! A simple container widget
class ContainerWidget : public QWidget
{
    Q_OBJECT

    friend class KFDTabWidget;

public:
    ContainerWidget(QWidget *parent);
    virtual ~ContainerWidget();

    virtual QSize sizeHint() const;

    //! Used to emit handleDragMoveEvent() signal needed to control dragging over the container's surface
    virtual void dragMoveEvent(QDragMoveEvent *e);

    //! Used to emit handleDropEvent() signal needed to control dropping on the container's surface
    virtual void dropEvent(QDropEvent *e);

signals:
    //! Needed to control dragging over the container's surface
    void handleDragMoveEvent(QDragMoveEvent *e);

    //! Needed to control dropping on the container's surface
    void handleDropEvent(QDropEvent *e);
};

//! A tab widget
class KFDTabWidget : public KFormDesigner::TabWidget
{
    Q_OBJECT

public:
    KFDTabWidget(KFormDesigner::Container *container, QWidget *parent);
    virtual ~KFDTabWidget();

    virtual QSize sizeHint() const;

    //! Used to emit handleDragMoveEvent() signal needed to control dragging over the container's surface
    virtual void dragMoveEvent(QDragMoveEvent *e);

    //! Used to emit handleDropEvent() signal needed to control dropping on the container's surface
    virtual void dropEvent(QDropEvent *e);

    KFormDesigner::Container *container() const { return m_container; }
    
signals:
    //! Needed to control dragging over the container's surface
    void handleDragMoveEvent(QDragMoveEvent *e);

    //! Needed to control dropping on the container's surface
    void handleDropEvent(QDropEvent *e);

private:
    KFormDesigner::Container *m_container;
};

//! A group box widget
class GroupBox : public QGroupBox
{
    Q_OBJECT

public:
    GroupBox(const QString & title, QWidget *parent);
    virtual ~GroupBox();

    //! Used to emit handleDragMoveEvent() signal needed to control dragging over the container's surface
    virtual void dragMoveEvent(QDragMoveEvent *e);

    //! Used to emit handleDropEvent() signal needed to control dropping on the container's surface
    virtual void dropEvent(QDropEvent *e);

signals:
    //! Needed to control dragging over the container's surface
    void handleDragMoveEvent(QDragMoveEvent *e);

    //! Needed to control dropping on the container's surface
    void handleDropEvent(QDropEvent *e);
};

//! @todo SubForm
#if 0
//! A form embedded as a widget inside other form
class SubForm : public Q3ScrollView
{
    Q_OBJECT
    Q_PROPERTY(QString formName READ formName WRITE setFormName)

public:
    SubForm(KFormDesigner::Form *parentForm, QWidget *parent);
    ~SubForm();

    //! \return the name of the subform inside the db
    QString   formName() const {
        return m_formName;
    }
    void      setFormName(const QString &name);

private:
    KFormDesigner::Form   *m_form;
    KFormDesigner::Form   *m_parentForm;
    QWidget  *m_widget;
    QString   m_formName;
};
#endif //0

//! Standard Factory for all container widgets
class ContainerFactory : public KFormDesigner::WidgetFactory
{
    Q_OBJECT

public:
    ContainerFactory(QObject *parent, const QStringList &args);
    virtual ~ContainerFactory();

    virtual QWidget* createWidget(const QByteArray &classname, QWidget *parent, const char *name,
                                  KFormDesigner::Container *container,
                                  CreateWidgetOptions options = DefaultOptions);
    virtual bool createMenuActions(const QByteArray& classname, QWidget *w,
                                   QMenu *menu, KFormDesigner::Container *container);
    virtual bool startInlineEditing(InlineEditorCreationArguments& args);
    virtual bool previewWidget(const QByteArray& classname, QWidget *widget,
                               KFormDesigner::Container *container);
    virtual bool saveSpecialProperty(const QByteArray& classname, const QString &name,
                                     const QVariant &value, QWidget *w, QDomElement &parentNode, QDomDocument &parent);
    virtual bool readSpecialProperty(const QByteArray& classname, QDomElement &node, QWidget *w,
                                     KFormDesigner::ObjectTreeItem *item);
    virtual QList<QByteArray> autoSaveProperties(const QByteArray &classname);

protected:
    virtual bool isPropertyVisibleInternal(const QByteArray &classname, QWidget *w,
                                           const QByteArray &property, bool isTopLevel);
    virtual bool changeInlineText(KFormDesigner::Form *form, QWidget *widget,
                                  const QString &text, QString &oldText);
    virtual void resizeEditor(QWidget *editor, QWidget *widget, const QByteArray &classname);

public slots:
//moved to internal AddTabAction       void addTabPage();
//moved to internal RenameTabAction    void renameTabPage();
//moved to internal RemoveTabAction    void removeTabPage();
//moved to internal AddStackPageAction void addStackPage();
//moved to internal RemoveStackPageAction void removeStackPage();
//moved to internal GoToStackPageAction   void prevStackPage();
//moved to internal GoToStackPageAction   void nextStackPage();
    void reorderTabs(int oldpos, int newpos);
};

#endif
