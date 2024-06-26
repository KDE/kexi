/**  This file is part of the KDE project
 *
 *  Copyright (C) 2011 Adam Pigg <adam@piggz.co.uk>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */



#ifndef KEXIMOBILENAVIGATOR_H
#define KEXIMOBILENAVIGATOR_H

#include <QWidget>
#include <QHBoxLayout>
#include <widget/navigator/KexiProjectNavigator.h>

class KexiProjectModel;
class KexiProject;

class KexiMobileNavigator : public QWidget
{
    Q_OBJECT
public:
    explicit KexiMobileNavigator(QWidget* parent = nullptr, Qt::WindowFlags f = {});
    virtual ~KexiMobileNavigator();

    void setProject(KexiProject*);

private Q_SLOTS:
    void slotOpenItem(KexiPart::Item *item);

Q_SIGNALS:
    void openItem(KexiPart::Item *item);

private:
    KexiProject *m_project;
    KexiProjectModel* m_reportModel;
    KexiProjectNavigator *m_formNavigator;
    KexiProjectNavigator *m_reportNavigator;

    QHBoxLayout *m_layout;
};

#endif // KEXIMOBILENAVIGATOR_H
