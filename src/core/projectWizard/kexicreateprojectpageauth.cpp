/* This file is part of the KDE project
Copyright (C) 2002   Peter Simonsson <psn@linux.se>

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
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qcheckbox.h>

#include <klocale.h>
#include <klineedit.h>
#include <kdialog.h>

#include "kexicreateprojectpageauth.h"

KexiCreateProjectPageAuth::KexiCreateProjectPageAuth(KexiCreateProject *parent, QPixmap *wpic, const char *name)
 : KexiCreateProjectPage(parent, wpic, name)
{
	//page settings
	setProperty("section", QVariant("RemoteDB"));
	setProperty("caption", QVariant(i18n("Authentication")));

	//labels
	QLabel *lUser = new QLabel(i18n("User:"), m_contents);
	QLabel *lPassword = new QLabel(i18n("Password:"), m_contents);

	//inputfields
	m_userEdit = new KLineEdit(m_contents);
	m_passwordEdit = new KLineEdit(m_contents);
	m_passwordEdit->setEchoMode(QLineEdit::Password);

	m_savePassChk = new QCheckBox(i18n("Save password"), m_contents);
	m_savePassChk->setChecked(true);
#ifdef KEXI_NO_UNFINISHED
	m_savePassChk->hide();
#endif
	setProperty("savePassword", QVariant(m_savePassChk->isChecked()));

	//layout
	QGridLayout *g = new QGridLayout(m_contents);
	g->addWidget(lUser,		0,      0);
	g->addWidget(lPassword,		1,      0);
	g->addWidget(m_userEdit,	0,      1);
	g->addWidget(m_passwordEdit,	1,      1);
	g->addWidget(m_savePassChk,	2,	1);
	g->setSpacing(KDialog::spacingHint());
	g->setRowStretch(3, 1);
	g->setColStretch(2, 1);

	connect(m_userEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotUserChanged(const QString &)));
	connect(m_passwordEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotPassChanged(const QString &)));
	connect(m_savePassChk, SIGNAL(toggled(bool)), this, SLOT(slotSavePassChanged(bool)));
}

void
KexiCreateProjectPageAuth::slotUserChanged(const QString &user)
{
	setProperty("user", QVariant(user));
}

void
KexiCreateProjectPageAuth::slotPassChanged(const QString &password)
{
	setProperty("password", QVariant(password));
}

void
KexiCreateProjectPageAuth::slotSavePassChanged(bool state)
{
	setProperty("savePassword", QVariant(state));
}

#include "kexicreateprojectpageauth.moc"
