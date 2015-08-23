/* This file is part of the KDE project
   Copyright (C) 2002, 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003-2007 Jarosław Staniek <staniek@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KexiRelationsTableContainer_P_H
#define KexiRelationsTableContainer_P_H

#include <QLabel>
#include <QMouseEvent>
#include <QEvent>
#include <QDropEvent>

#include <widget/fields/KexiFieldListView.h>

class KexiRelationsScrollArea;
class KexiRelationViewTable;
class KexiRelationViewTableContainerHeader;
class KDbTableOrQuerySchema;

//! @internal A field list widget used in table container to show fields
class KexiRelationsTableFieldList : public KexiFieldListView
{
    Q_OBJECT

public:
    KexiRelationsTableFieldList(KDbTableOrQuerySchema* tableOrQuerySchema,
                                KexiRelationsScrollArea *scrollArea, QWidget *parent = 0);
    virtual ~KexiRelationsTableFieldList();

    int globalY(const QString &item);

    virtual QSize sizeHint() const;

Q_SIGNALS:
    void tableScrolling();

protected Q_SLOTS:
    void slotContentsMoving(int, int);

protected:
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* e);
    virtual void dropEvent(QDropEvent *e);
    virtual void contentsMousePressEvent(QMouseEvent * e);
    virtual bool eventFilter(QObject *o, QEvent *ev);

private:
    KexiRelationsScrollArea *m_scrollArea;
};

//! @internal A header widget used in table container
class KexiRelationViewTableContainerHeader : public QLabel
{
    Q_OBJECT
public:
    explicit KexiRelationViewTableContainerHeader(const QString& text, QWidget *parent = 0);
    virtual ~KexiRelationViewTableContainerHeader();

    virtual void setFocus();
    virtual void unsetFocus();

Q_SIGNALS:
    void moved();
    void endDrag();

protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    void mousePressEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);

    bool m_dragging;
    int m_grabX;
    int m_grabY;
    int m_offsetX;
    int m_offsetY;

    QColor m_activeBG, m_activeFG, m_inactiveBG, m_inactiveFG;
};

#endif
