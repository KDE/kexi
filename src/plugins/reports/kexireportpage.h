/*
 * Kexi Report Plugin
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
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef KEXIREPORTPAGE_H
#define KEXIREPORTPAGE_H

#include <qwidget.h>
#include <KoReportRendererBase.h>

class QPixmap;
class ORODocument;

/**
 @author Adam Pigg <adam@piggz.co.uk>
*/
class KexiReportPage : public QWidget
{
    Q_OBJECT
public:
    KexiReportPage(QWidget *parent, ORODocument *document);

    ~KexiReportPage();

    void renderPage(int page);

public slots:
    virtual void paintEvent(QPaintEvent*);

private:
    ORODocument *m_reportDocument;
    int m_page;
    bool m_repaint;
    QPixmap *m_pixmap;
    KoReportRendererFactory m_factory;
    KoReportRendererBase *m_renderer;
};

#endif
