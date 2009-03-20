/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2007 by OpenMFG, LLC
 * Copyright (C) 2007-2008 by Adam Pigg (adam@piggz.co.uk)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact info@openmfg.com with any questions on this license.
 */

#ifndef __REPORTSCENEVIEW_H__
#define __REPORTSCENEVIEW_H__

#include <QGraphicsView>
class ReportDesigner;

class ReportSceneView : public QGraphicsView
{
    Q_OBJECT
public:
    ReportSceneView(ReportDesigner *, QGraphicsScene * scene, QWidget * parent = 0, const char * name = 0);
    virtual ~ReportSceneView();

    ReportDesigner * designer();
    virtual QSize sizeHint() const;
public slots:
    void resizeContents(QSize);

protected:
    void mouseReleaseEvent(QMouseEvent * e);

private:
    ReportDesigner* m_reportDesigner;
};

#endif
