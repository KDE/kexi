/* This file is part of the KDE project
   Copyright (C) 2005 Jaroslaw Staniek <js@iidea.pl>

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

#include "importoptionsdlg.h"
#include <widget/kexicharencodingcombobox.h>

#include <qdir.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtextcodec.h>
#include <qcheckbox.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kcombobox.h>
#include <klocale.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kiconloader.h>

using namespace KexiMigration;

OptionsDialog::OptionsDialog( const QString& databaseFile, const QString& selectedEncoding, QWidget* parent )
 : KDialogBase( 
	KDialogBase::Plain, 
	i18n( "Advanced Import Options" ),
	Ok|Cancel, 
	Ok,
	parent, 
	"KexiMigration::OptionsDialog", 
	true, 
	false
 )
{
	setIcon(DesktopIcon("configure"));
	QGridLayout *lyr = new QGridLayout( plainPage(), 4, 3, KDialogBase::marginHint(), 
		KDialogBase::spacingHint());

	m_encodingComboBox = new KexiCharacterEncodingComboBox(plainPage(), selectedEncoding);
	m_encodingComboBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	lyr->addWidget( m_encodingComboBox, 1, 1 );
	QLabel* lbl = new QLabel( 
		i18n("<h3>Text encoding for Microsoft Access database</h3>\n"
		"<p>Database file \"%1\" appears to be created by a version of Microsoft Access older than 2000.</p>"
		"<p>In order to properly import national characters, you may need to choose a proper text encoding "
		"if the database was created on a computer with a different character set.</p>")
		.arg(QDir::convertSeparators(databaseFile)), plainPage());
	lbl->setAlignment( Qt::AlignAuto | Qt::WordBreak );
	lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	lyr->addMultiCellWidget( lbl, 0, 0, 0, 2 );

	QLabel* lbl2 = new QLabel( m_encodingComboBox, i18n("Text encoding:"), plainPage());
	lyr->addWidget( lbl2, 1, 0 );

	m_chkAlwaysUseThisEncoding = new QCheckBox(
		i18n("Always use this encoding in similar situations"), plainPage());
	lyr->addMultiCellWidget( m_chkAlwaysUseThisEncoding, 2, 2, 1,2 );

	lyr->addItem( new QSpacerItem( 20, 111, QSizePolicy::Minimum, QSizePolicy::Expanding ), 3, 1 );
	lyr->addItem( new QSpacerItem( 121, 20, QSizePolicy::Expanding, QSizePolicy::Minimum ), 1, 2 );

	//read config
	kapp->config()->setGroup("ImportExport");
	QString defaultEncodingForMSAccessFiles = kapp->config()->readEntry("DefaultEncodingForMSAccessFiles");
	if (!defaultEncodingForMSAccessFiles.isEmpty()) {
		m_encodingComboBox->setSelectedEncoding(defaultEncodingForMSAccessFiles);
		m_chkAlwaysUseThisEncoding->setChecked(true);
	}

	adjustSize();
	m_encodingComboBox->setFocus();
}

OptionsDialog::~OptionsDialog()
{
}

KexiCharacterEncodingComboBox* OptionsDialog::encodingComboBox() const
{
	return m_encodingComboBox;
}

void OptionsDialog::accept()
{
	kapp->config()->setGroup("ImportExport");
	if (m_chkAlwaysUseThisEncoding->isChecked())
		kapp->config()->writeEntry("defaultEncodingForMSAccessFiles", 
			m_encodingComboBox->selectedEncoding());
	else
		kapp->config()->deleteEntry("defaultEncodingForMSAccessFiles");

	KDialogBase::accept();
}

#include "importoptionsdlg.moc"
