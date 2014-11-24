/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
 *  Copyright (c) 2014 Mohit Goyal <mohit.bits2011@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef KUNDO2STACK_H
#define KUNDO2STACK_H

#include <QObject>
#include <QString>
#include <QList>
#include <QAction>
#include <QTime>
#include <QVector>


#include "kundo2_export.h"

class QAction;
class KUndo2CommandPrivate;
class KUndo2Group;
class KActionCollection;

#ifndef QT_NO_UNDOCOMMAND

#include "kundo2magicstring.h"

class KUNDO2_EXPORT KUndo2Command
{
    KUndo2CommandPrivate *d;
    int timedID;

public:
    explicit KUndo2Command(KUndo2Command *parent = 0);
    explicit KUndo2Command(const KUndo2MagicString &text, KUndo2Command *parent = 0);
    virtual ~KUndo2Command();

    virtual void undo();
    virtual void redo();

    QString actionText() const;
    KUndo2MagicString text() const;
    void setText(const KUndo2MagicString &text);

    virtual int id() const;
    virtual int timedId();
    virtual void setTimedID(int timedID);
    virtual bool mergeWith(const KUndo2Command *other);
    virtual bool timedMergeWith(KUndo2Command *other);

    int childCount() const;
    const KUndo2Command *child(int index) const;

    bool hasParent();
    virtual void setTime();
    virtual QTime time();
    virtual void setEndTime();
    virtual QTime endTime();

    virtual QVector<KUndo2Command*> mergeCommandsVector();
    virtual bool isMerged();
    virtual void undoMergedCommands();
    virtual void redoMergedCommands();



private:
    Q_DISABLE_COPY(KUndo2Command)
    friend class KUndo2QStack;


    bool m_hasParent;
    int m_timedID;

    QTime m_timeOfCreation;
    QTime m_endOfCommand;
    QVector<KUndo2Command*> m_mergeCommandsVector;
};

#endif // QT_NO_UNDOCOMMAND

#ifndef QT_NO_UNDOSTACK

class KUNDO2_EXPORT KUndo2QStack : public QObject
{
    Q_OBJECT
//    Q_DECLARE_PRIVATE(KUndo2QStack)
    Q_PROPERTY(bool active READ isActive WRITE setActive)
    Q_PROPERTY(int undoLimit READ undoLimit WRITE setUndoLimit)

public:
    explicit KUndo2QStack(QObject *parent = 0);
    virtual ~KUndo2QStack();
    void clear();

    void push(KUndo2Command *cmd);

    bool canUndo() const;
    bool canRedo() const;
    QString undoText() const;
    QString redoText() const;

    int count() const;
    int index() const;
    QString actionText(int idx) const;
    QString text(int idx) const;

#ifndef QT_NO_ACTION
    QAction *createUndoAction(QObject *parent) const;
    QAction *createRedoAction(QObject *parent) const;
#endif // QT_NO_ACTION

    bool isActive() const;
    bool isClean() const;
    int cleanIndex() const;

    void beginMacro(const KUndo2MagicString &text);
    void endMacro();

    void setUndoLimit(int limit);
    int undoLimit() const;

    const KUndo2Command *command(int index) const;

    void setUseCumulativeUndoRedo(bool value);
    bool useCumulativeUndoRedo();
    void setTimeT1(double value);
    double timeT1();
    void setTimeT2(double value);
    double timeT2();
    int strokesN();
    void setStrokesN(int value);


public Q_SLOTS:
    void setClean();
    virtual void setIndex(int idx);
    virtual void undo();
    virtual void redo();
    void setActive(bool active = true);

Q_SIGNALS:
    void indexChanged(int idx);
    void cleanChanged(bool clean);
    void canUndoChanged(bool canUndo);
    void canRedoChanged(bool canRedo);
    void undoTextChanged(const QString &undoActionText);
    void redoTextChanged(const QString &redoActionText);

private:
    // from QUndoStackPrivate
    QList<KUndo2Command*> m_command_list;
    QList<KUndo2Command*> m_macro_stack;
    int m_index;
    int m_clean_index;
    KUndo2Group *m_group;
    int m_undo_limit;
    bool m_useCumulativeUndoRedo;
    double m_timeT1;
    double m_timeT2;
    int m_strokesN;
    int m_lastMergedSetCount;
    int m_lastMergedIndex;

    // also from QUndoStackPrivate
    void setIndex(int idx, bool clean);
    bool checkUndoLimit();

    Q_DISABLE_COPY(KUndo2QStack)
    friend class KUndo2Group;
};

class KUNDO2_EXPORT KUndo2Stack : public KUndo2QStack
{
public:
    explicit KUndo2Stack(QObject *parent = 0);

    // functions from KUndoStack
    QAction* createRedoAction(KActionCollection* actionCollection, const QString& actionName = QString());
    QAction* createUndoAction(KActionCollection* actionCollection, const QString& actionName = QString());
};

#endif // QT_NO_UNDOSTACK

#endif // KUNDO2STACK_H
