/* This file is part of the KDE project
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2005 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef KexiDBDateEdit_H
#define KexiDBDateEdit_H

#include "kexiformdataiteminterface.h"
#include <qdatetimeedit.h>

class KPopupMenu;
class KDatePicker;
class QDateTimeEditor;

//! @short A db-aware date editor
class KEXIFORMUTILS_EXPORT KexiDBDateEdit : public QWidget, public KexiFormDataItemInterface
{
	Q_OBJECT
	Q_PROPERTY(QString dataSource READ dataSource WRITE setDataSource DESIGNABLE true)
	Q_PROPERTY(QCString dataSourceMimeType READ dataSourceMimeType WRITE setDataSourceMimeType DESIGNABLE true)
	// properties copied from QDateEdit
	Q_ENUMS( Order )
	Q_PROPERTY( Order order READ order WRITE setOrder DESIGNABLE true)
	Q_PROPERTY( QDate date READ date WRITE setDate DESIGNABLE true)
	Q_PROPERTY( bool autoAdvance READ autoAdvance WRITE setAutoAdvance DESIGNABLE true)
	Q_PROPERTY( QDate maxValue READ maxValue WRITE setMaxValue DESIGNABLE true)
	Q_PROPERTY( QDate minValue READ minValue WRITE setMinValue DESIGNABLE true)
	Q_PROPERTY( bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE true )

	public:
		enum Order { DMY = QDateEdit::DMY, MDY = QDateEdit::MDY, YMD = QDateEdit::YMD,  YDM = QDateEdit::YDM };

		KexiDBDateEdit(const QDate &date, QWidget *parent, const char *name=0);
		virtual ~KexiDBDateEdit();

		inline QString dataSource() const { return KexiFormDataItemInterface::dataSource(); }
		inline QCString dataSourceMimeType() const { return KexiFormDataItemInterface::dataSourceMimeType(); }
		virtual QVariant value();
		virtual void setInvalidState( const QString& displayText );

		//! \return true if editor's value is null (not empty)
		//! Used for checking if a given constraint within table of form is met.
		virtual bool valueIsNull();

		//! \return true if editor's value is empty (not necessary null).
		//! Only few data types can accept "EMPTY" property
		//! (use KexiDB::Field::hasEmptyProperty() to check this).
		//! Used for checking if a given constraint within table or form is met.
		virtual bool valueIsEmpty();

		/*! \return 'readOnly' flag for this widget. */
		virtual bool isReadOnly() const;

		/*! \return the view widget of this item, e.g. line edit widget. */
		virtual QWidget* widget();

		virtual bool cursorAtStart();
		virtual bool cursorAtEnd();
		virtual void clear();

		virtual void  setEnabled(bool enabled);

		// property functions
		inline QDate date() const { return m_edit->date(); }
		inline void setOrder(Order order) { m_edit->setOrder( (QDateEdit::Order) order); }
		inline Order order() const { return (Order)m_edit->order(); }
		inline void setAutoAdvance( bool advance ) { m_edit->setAutoAdvance(advance); }
		inline bool autoAdvance() const { return m_edit->autoAdvance(); }
		inline void setMinValue(const QDate& d) { m_edit->setMinValue(d); }
		inline QDate minValue() const { return m_edit->minValue(); }
		inline void setMaxValue(const QDate& d) { m_edit->setMaxValue(d); }
		inline QDate maxValue() const { return m_edit->maxValue(); }

	signals:
		void  dateChanged(const QDate &date);

	public slots:
		inline void setDataSource(const QString &ds) { KexiFormDataItemInterface::setDataSource(ds); }
		inline void setDataSourceMimeType(const QCString &ds) { KexiFormDataItemInterface::setDataSourceMimeType(ds); }
		inline void setDate(const QDate& date)  { m_edit->setDate(date); }
		virtual void setReadOnly(bool set);

	protected slots:
		void slotValueChanged(const QDate&);
		void  slotShowDatePicker();
		void  acceptDate();

	protected:
		virtual void setValueInternal(const QVariant& add, bool removeOld);
		virtual bool  eventFilter(QObject *o, QEvent *e);

	private:
		KDatePicker *m_datePicker;
		QDateEdit *m_edit;
		KPopupMenu *m_datePickerPopupMenu;
		QDateTimeEditor *m_dte_date;
		bool m_invalidState : 1;
		bool m_cleared : 1;
		bool m_readOnly : 1;
};

#endif
