/* This file is part of the KDE project
   Copyright (C) 2005-2013 Jarosław Staniek <staniek@kde.org>

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

#include "kexicsvwidgets.h"
#include <KexiIcon.h>

#include <QDir>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>
#include <QVector>
#include <QLineEdit>

#include <KIconLoader>
#include <KIO/Global>
#include <KLocalizedString>

#include <kexi_global.h>
#include <kexiutils/utils.h>

#define KEXICSV_OTHER_DELIMITER_INDEX 4
#define KEXICSV_OTHER_NUMBER_OF_SIGNS 2
#define KEXICSV_OTHER_commentSymbol_INDEX 1

class Q_DECL_HIDDEN KexiCSVInfoLabel::Private
{
public:
    Private() {}

    QLabel *leftLabel, *iconLbl, *fnameLbl, *commentLbl;
    QFrame* separator;
};

class Q_DECL_HIDDEN KexiCSVDelimiterWidget::Private
{
public:
    Private() : availableDelimiters(KEXICSV_OTHER_DELIMITER_INDEX) {
        availableDelimiters[0] = KEXICSV_DEFAULT_FILE_DELIMITER;
        availableDelimiters[1] = ";";
        availableDelimiters[2] = "\t";
        availableDelimiters[3] = " ";
    }
    QString delimiter;
    QVector<QString> availableDelimiters;
    KComboBox* combo;
    QLineEdit* delimiterEdit;
};

class Q_DECL_HIDDEN KexiCSVCommentWidget::Private
{
public:
    Private() : availablecommentSymbols(KEXICSV_OTHER_NUMBER_OF_SIGNS) {
        availablecommentSymbols[0] = KEXICSV_DEFAULT_COMMENT_START;
        availablecommentSymbols[1] = KEXICSV_DEFAULT_HASHSIGN;
    }
    QString commentSymbol;
    QVector<QString> availablecommentSymbols;
    KComboBox* combo;
};

KexiCSVDelimiterWidget::KexiCSVDelimiterWidget(bool lineEditOnBottom, QWidget * parent)
        : QWidget(parent)
        , d(new Private())
{
    QBoxLayout *lyr = new QBoxLayout(lineEditOnBottom ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    setLayout(lyr);
    KexiUtils::setMargins(lyr, 0);
    lyr->setSpacing(KexiUtils::spacingHint());

    d->combo = new KComboBox(this);
    d->combo->setObjectName("KexiCSVDelimiterComboBox");
    d->combo->addItem(xi18n("Comma \",\""));    //<-- KEXICSV_DEFAULT_FILE_DELIMITER
    d->combo->addItem(xi18n("Semicolon \";\""));
    d->combo->addItem(xi18n("Tabulator"));
    d->combo->addItem(xi18n("Space \" \""));
    d->combo->addItem(xi18n("Other"));
    lyr->addWidget(d->combo);
    setFocusProxy(d->combo);

    d->delimiterEdit = new QLineEdit(this);
    d->delimiterEdit->setObjectName("d->delimiterEdit");
    d->delimiterEdit->setMaximumSize(QSize(30, 32767));
    d->delimiterEdit->setMaxLength(1);
    d->delimiterEdit->setVisible(false);
    lyr->addWidget(d->delimiterEdit);
    if (!lineEditOnBottom)
        lyr->addStretch(2);

    slotDelimiterChangedInternal(KEXICSV_DEFAULT_FILE_DELIMITER_INDEX); //this will init d->delimiter
    connect(d->combo, SIGNAL(activated(int)),
            this, SLOT(slotDelimiterChanged(int)));
    connect(d->delimiterEdit, SIGNAL(returnPressed()),
            this, SLOT(slotDelimiterLineEditReturnPressed()));
    connect(d->delimiterEdit, SIGNAL(textChanged(QString)),
            this, SLOT(slotDelimiterLineEditTextChanged(QString)));
    slotDelimiterChangedInternal(KEXICSV_DEFAULT_FILE_DELIMITER_INDEX); //this will init d->delimiter
    connect(d->combo, SIGNAL(activated(int)),
            this, SLOT(slotDelimiterChanged(int)));
}

KexiCSVDelimiterWidget::~KexiCSVDelimiterWidget()
{
    delete d;
}

void KexiCSVDelimiterWidget::slotDelimiterChanged(int index)
{
    slotDelimiterChangedInternal(index);
    if (index == KEXICSV_OTHER_DELIMITER_INDEX)
        d->delimiterEdit->setFocus();
}

void KexiCSVDelimiterWidget::slotDelimiterChangedInternal(int index)
{
    bool changed = false;
    if (index > KEXICSV_OTHER_DELIMITER_INDEX)
        return;
    else if (index == KEXICSV_OTHER_DELIMITER_INDEX) {
        changed = d->delimiter != d->delimiterEdit->text();
        d->delimiter = d->delimiterEdit->text();
    } else {
        changed = d->delimiter != d->availableDelimiters[index];
        d->delimiter = d->availableDelimiters[index];
    }
    d->delimiterEdit->setEnabled(index == KEXICSV_OTHER_DELIMITER_INDEX);
    if (changed)
        emit delimiterChanged(d->delimiter);
}

void KexiCSVDelimiterWidget::slotDelimiterLineEditReturnPressed()
{
    if (d->combo->currentIndex() != KEXICSV_OTHER_DELIMITER_INDEX)
        return;
    slotDelimiterChangedInternal(KEXICSV_OTHER_DELIMITER_INDEX);
}

void KexiCSVDelimiterWidget::slotDelimiterLineEditTextChanged(const QString &)
{
    slotDelimiterChangedInternal(KEXICSV_OTHER_DELIMITER_INDEX);
}

QString KexiCSVDelimiterWidget::delimiter() const
{
    return d->delimiter;
}

void KexiCSVDelimiterWidget::setDelimiter(const QString& delimiter)
{
    for (int i=0; i < d->availableDelimiters.size(); ++i) { // we cannot use iterator here because of msvc
        if (d->availableDelimiters[i] == delimiter) {       // (see https://bugreports.qt.io/browse/QTBUG-45368)
            d->combo->setCurrentIndex(i);
            slotDelimiterChangedInternal(i);
            return;
        }
    }
    //else: set other (custom) delimiter
    d->delimiterEdit->setText(delimiter);
    d->combo->setCurrentIndex(KEXICSV_OTHER_DELIMITER_INDEX);
    slotDelimiterChangedInternal(KEXICSV_OTHER_DELIMITER_INDEX);
}

//----------------------------------------------------

KexiCSVCommentWidget::KexiCSVCommentWidget(bool lineEditOnBottom, QWidget *parent) : QWidget(parent)
  , d(new Private())
{
    QBoxLayout *lyr = new QBoxLayout(lineEditOnBottom ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    setLayout(lyr);
    KexiUtils::setMargins(lyr, 0);
    lyr->setSpacing(KexiUtils::spacingHint());

    d->combo = new KComboBox(this);
    d->combo->setObjectName("KexiCSVcommentSymbolComboBox");
    d->combo->addItem(xi18n("None"));
    d->combo->addItem(xi18n("Hash \"#\""));
    lyr->addWidget(d->combo);

    setFocusProxy(d->combo);
    const int numberOfNonePosition = 0;
    slotcommentSymbolChangedInternal(numberOfNonePosition);
    connect(d->combo, SIGNAL(activated(int)),
            this, SLOT(slotcommentSymbolChanged(int)));
}

KexiCSVCommentWidget::~KexiCSVCommentWidget()
{
    delete d;
}

void KexiCSVCommentWidget::slotcommentSymbolChanged(int index)
{
    slotcommentSymbolChangedInternal(index);
}

void KexiCSVCommentWidget::slotcommentSymbolChangedInternal(int index)
{
    bool changed = d->commentSymbol != d->availablecommentSymbols[index];
    d->commentSymbol = d->availablecommentSymbols[index];
    if (changed)
      emit commentSymbolChanged(d->commentSymbol);
}

QString KexiCSVCommentWidget::commentSymbol() const {
  return d->commentSymbol;
}

void KexiCSVCommentWidget::setcommentSymbol(const QString& commentSymbol)
{
    for (int i=0; i < d->availablecommentSymbols.size(); ++i) { // we cannot use iterator here because of msvc
        if (d->availablecommentSymbols[i] == commentSymbol) {   // (see https://bugreports.qt.io/browse/QTBUG-45368)
            d->combo->setCurrentIndex(i);
            slotcommentSymbolChangedInternal(i);
            return;
        }
    }
}

KexiCSVTextQuoteComboBox::KexiCSVTextQuoteComboBox(QWidget * parent)
        : KComboBox(parent)
{
    addItem("\"");
    addItem("'");
    addItem(xi18n("None"));
}

QString KexiCSVTextQuoteComboBox::textQuote() const
{
    if (currentIndex() == 2)
        return QString();
    return currentText();
}

void KexiCSVTextQuoteComboBox::setTextQuote(const QString& textQuote)
{
    QString q(textQuote.isEmpty() ? xi18n("None") : textQuote);
    setCurrentIndex(findText(q));
}

//----------------------------------------------------

KexiCSVInfoLabel::KexiCSVInfoLabel(const QString& labelText, QWidget* parent, bool showFnameLine)
        : QWidget(parent)
        , d(new Private)
{
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);
    KexiUtils::setMargins(vbox, 0);
    vbox->setSpacing(KexiUtils::spacingHint());
    QGridLayout *topbox = new QGridLayout;
    vbox->addLayout(topbox);

    d->iconLbl = new QLabel(this);
    d->iconLbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    d->iconLbl->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    topbox->addWidget(d->iconLbl, 0, 0, 2, 1);
    topbox->addItem(new QSpacerItem(
        KexiUtils::spacingHint(), KexiUtils::spacingHint(), QSizePolicy::Fixed, QSizePolicy::Fixed),
        0, 1, 2, 1
    );

    d->leftLabel = new QLabel(labelText, this);
    d->leftLabel->setMinimumWidth(130);
    d->leftLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    QSizePolicy fnameLblSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    fnameLblSizePolicy.setHorizontalStretch(1);
    d->leftLabel->setSizePolicy(fnameLblSizePolicy);
    d->leftLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    topbox->addWidget(d->leftLabel, 0, 2, showFnameLine ? 1 : 2, 1);

    if (showFnameLine) {
        d->fnameLbl = new QLabel(this);
        d->fnameLbl->setOpenExternalLinks(true);
        d->fnameLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        d->fnameLbl->setTextFormat(Qt::PlainText);
        QSizePolicy fnameLblSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        fnameLblSizePolicy.setHorizontalStretch(1);
        d->fnameLbl->setSizePolicy(fnameLblSizePolicy);
        d->fnameLbl->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        d->fnameLbl->setWordWrap(true);
        topbox->addWidget(d->fnameLbl, 1, 2); //Qt::AlignVCenter | Qt::AlignLeft);
    }
    else {
        d->fnameLbl = 0;
    }

    d->commentLbl = new QLabel(this);
    d->commentLbl->setOpenExternalLinks(true);
    d->commentLbl->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    d->commentLbl->setFocusPolicy(Qt::NoFocus);
    d->commentLbl->setTextFormat(Qt::PlainText);
    d->commentLbl->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    d->commentLbl->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    d->commentLbl->setWordWrap(true);
    topbox->addWidget(d->commentLbl, 0, 3, 2, 1);

    d->separator = new QFrame(this);
    d->separator->setFrameShape(QFrame::HLine);
    d->separator->setFrameShadow(QFrame::Sunken);
    vbox->addWidget(d->separator);
}

KexiCSVInfoLabel::~KexiCSVInfoLabel()
{
    delete d;
}

void KexiCSVInfoLabel::setFileName(const QString& fileName)
{
    if (!d->fnameLbl)
        return;
    d->fnameLbl->setText(QDir::toNativeSeparators(fileName));
    if (!fileName.isEmpty()) {
        d->iconLbl->setPixmap(KIconLoader::global()->loadMimeTypeIcon(
            KIO::iconNameForUrl(QUrl::fromLocalFile(fileName)), KIconLoader::Desktop));
    }
}

void KexiCSVInfoLabel::setFileNameText(const QString& text)
{
    if (!d->fnameLbl)
        return;
    d->fnameLbl->setText(text);
}

void KexiCSVInfoLabel::setLabelText(const QString& text)
{
    d->leftLabel->setText(text);
}

void KexiCSVInfoLabel::setIcon(const QString& iconName)
{
    d->iconLbl->setPixmap(koDesktopIconCStr(iconName));
}


QLabel* KexiCSVInfoLabel::leftLabel() const
{
    return d->leftLabel;
}

QLabel* KexiCSVInfoLabel::fileNameLabel() const
{
    return d->fnameLbl;
}

QLabel* KexiCSVInfoLabel::commentLabel() const
{
    return d->commentLbl;
}

QFrame* KexiCSVInfoLabel::separator() const {
    return d->separator;
}


void KexiCSVInfoLabel::setCommentText(const QString& text)
{
    d->commentLbl->setText(text);
}

//----------------------------------------------------

QStringList csvMimeTypes()
{
    return QStringList()
              << "text/csv"
              << "text/tab-separated-value"
              << "text/plain"; // use application/octet-stream if you want
                               // all files, but then the others are not necessary
}

