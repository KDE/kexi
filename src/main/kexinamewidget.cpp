/* This file is part of the KDE project
   Copyright (C) 2004 Jaroslaw Staniek <js@iidea.pl>

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

#include "kexinamewidget.h"

#include <qlabel.h>
#include <qlayout.h>

#include <klineedit.h>
#include <kmessagebox.h>
#include <klocale.h>

#include <kexiutils/validator.h>
#include <kexiutils/identifier.h>
#include <core/kexi.h>

using namespace KexiUtils;

KexiNameWidget::KexiNameWidget( const QString& message, 
	QWidget* parent, const char* name, WFlags fl )
 : QWidget(parent, name, fl)
{
	init(message, QString::null, QString::null, QString::null, QString::null);
}

KexiNameWidget::KexiNameWidget(const QString& message, 
	const QString& nameLabel, const QString& nameText, 
	const QString& captionLabel, const QString& captionText, 
	QWidget * parent, const char * name, WFlags fl)
{
	Q_UNUSED( parent );
	Q_UNUSED( name );
	Q_UNUSED( fl );

	init(message, nameLabel, nameText, captionLabel, captionText);
}

void KexiNameWidget::init(
	const QString& message, 
	const QString& nameLabel, const QString& nameText, 
	const QString& captionLabel, const QString& captionText)
{
	Q_UNUSED( captionText );

	m_le_name_txtchanged_disable = false;
	m_le_name_autofill = true;
	m_caption_required = false;

	lyr = new QGridLayout( this, 1, 1, 0, 6, "lyr");

	lbl_message = new QLabel( this, "message" );
	setMessageText( message );
	lbl_message->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	lbl_message->setAlignment( QLabel::AlignTop | QLabel::WordBreak );
	lyr->addMultiCellWidget( lbl_message, 0, 0, 0, 1 );

	lbl_caption = new QLabel( captionLabel.isEmpty() ? i18n( "Caption:" ) : captionLabel,
		this, "lbl_caption" );
	lyr->addWidget( lbl_caption, 1, 0 );

	lbl_name = new QLabel( nameLabel.isEmpty() ? tr( "Name:" ) : nameLabel,
		this, "lbl_name" );
	lyr->addWidget( lbl_name, 2, 0 );

	le_caption = new KLineEdit( nameText, this, "le_caption" );
	le_caption->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed, 1, 0));
	lyr->addWidget( le_caption, 1, 1 );

	le_name = new KLineEdit( nameText, this, "le_name" );
	le_name->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed,1,0));
	Validator *idValidator = new IdentifierValidator(0, "id_val");
	le_name->setValidator( m_validator = new MultiValidator(idValidator, this, "val") );
	lyr->addWidget( le_name, 2, 1 );

	setFocusProxy(le_caption);
	resize( QSize(342, 123).expandedTo(minimumSizeHint()) );

	m_nameWarning = i18n("Please enter the name.");
	m_captionWarning = i18n("Please enter the caption.");

	connect(le_caption, SIGNAL(textChanged(const QString&)),
		this,SLOT(slotCaptionTxtChanged(const QString&)));
	connect(le_name, SIGNAL(textChanged(const QString&)),
		this,SLOT(slotNameTxtChanged(const QString&)));
	connect(le_caption, SIGNAL(returnPressed()),
		this,SIGNAL(returnPressed()));
	connect(le_name, SIGNAL(returnPressed()),
		this,SIGNAL(returnPressed()));
}

KexiNameWidget::~KexiNameWidget()
{
}

void KexiNameWidget::slotCaptionTxtChanged(const QString &capt)
{
	emit textChanged();
	if (le_name->text().isEmpty())
		m_le_name_autofill=true;
	if (m_le_name_autofill) {
		m_le_name_txtchanged_disable = true;
		le_name->setText( string2Identifier(capt).lower() );
		m_le_name_txtchanged_disable = false;
	}
}

void KexiNameWidget::slotNameTxtChanged(const QString &)
{
	emit textChanged();
	if (m_le_name_txtchanged_disable)
		return;
	m_le_name_autofill = false;
}

void KexiNameWidget::clear()
{
	le_name->clear();
	le_caption->clear();
}

bool KexiNameWidget::empty() const
{
	return le_name->text().isEmpty() || le_caption->text().stripWhiteSpace().isEmpty();
}

void KexiNameWidget::setNameRequired( bool set ) 
{ m_validator->setAcceptsEmptyValue(!set); }

bool KexiNameWidget::isNameRequired() const 
{ return !m_validator->acceptsEmptyValue(); }

void KexiNameWidget::setCaptionText(const QString& capt)
{
	le_caption->setText(capt);
	m_le_name_autofill = true;
}

void KexiNameWidget::setNameText(const QString& name)
{
	le_name->setText(name);
	m_le_name_autofill = true;
}

void KexiNameWidget::setMessageText(const QString& msg)
{
	if (msg.stripWhiteSpace().isEmpty()) {
		lbl_message->setText("");
		lbl_message->hide();
	} else {
		lbl_message->setText(msg.stripWhiteSpace()+"<br>");
		lbl_message->show();
	}
	messageChanged();
}

QString KexiNameWidget::captionText() const
{
	return le_caption->text();
}

QString KexiNameWidget::nameText() const
{
	return le_name->text().lower();
}

bool KexiNameWidget::checkValidity()
{
	if (isNameRequired() && le_name->text().stripWhiteSpace().isEmpty()) {
		KMessageBox::sorry(0, m_nameWarning);
		le_name->setFocus();
		return false;
	}
	if (isCaptionRequired() && le_caption->text().stripWhiteSpace().isEmpty()) {
		KMessageBox::sorry(0, m_captionWarning);
		le_caption->setFocus();
		return false;
	}
	QString dummy, message, details;
	if (m_validator->check(dummy, le_name->text(), message, details)
		==Validator::Error) {
		KMessageBox::detailedSorry(0, message, details);
		le_name->setFocus();
		return false;
	}
	return true;
}

Validator *KexiNameWidget::nameValidator() const 
{
	return m_validator;
}

void KexiNameWidget::addNameSubvalidator( Validator* validator, bool owned )
{
	m_validator->addSubvalidator( validator, owned );
}

/*bool KexiNameWidget::eventFilter( QObject *obj, QEvent *ev )
{
	if (ev->type()==QEvent::FocusOut && !acceptsEmptyValue()) {
		if (obj==le_name) {
			if (le_name->text().isEmpty()) {
				KMessageBox::information(0, m_nameWarning);
				le_name->setFocus();
				return true;
			}
		}
		else if (obj==le_caption) {
			if (le_caption->text().isEmpty()) {
				KMessageBox::information(0, m_captionWarning);
				le_caption->setFocus();
				return true;
			}
		}
	}
	return false;
}*/

#include "kexinamewidget.moc"

