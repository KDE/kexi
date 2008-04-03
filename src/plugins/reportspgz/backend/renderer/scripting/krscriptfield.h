//
// C++ Interface: krscriptfield
//
// Description:
//
//
// Author: Adam Pigg <adam@piggz.co.uk>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KRSCRIPTFIELD_H
#define KRSCRIPTFIELD_H

#include <QObject>
#include <krfielddata.h>

/**
	@author Adam Pigg <adam@piggz.co.uk>
*/
namespace Scripting
{
	class Field : public QObject
	{
		Q_OBJECT
		public:
			Field ( KRFieldData* );

			~Field();
		
		public slots:
			/**Returns the source (column) that the field gets its data from*/
			QString source();
			/**Sets the source (column) for the field*/
			void setSource(const QString&);
			
			int horizontalAlignment();
			void setHorizonalAlignment(int);
			
			int verticalAlignment();
			void setVerticalAlignment(int);
		
			QColor backgroundColor();
			void setBackgroundColor(QString);
		
			QColor foregroundColor();
			void setForegroundColor(QString);
		
			int backgroundOpacity();
			void setBackgroundOpacity(int);
		
			QColor lineColor();
			void setLineColor(QString);
			
			int lineWeight();
			void setLineWeight(int);
			
			int lineStyle();
			void setLineStyle(int);
			
			QPointF position();
			void setPosition(qreal, qreal);
			
			QSizeF size();
			void setSize(qreal,qreal);
		private:
			KRFieldData *_field;

	};
}
#endif
