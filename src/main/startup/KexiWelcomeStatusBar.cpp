/* This file is part of the KDE project
   Copyright (C) 2011-2012 Jarosław Staniek <staniek@kde.org>

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

#include "KexiWelcomeStatusBar.h"
#include "KexiWelcomeStatusBar_p.h"

#include <core/KexiMainWindowIface.h>
#include <KexiVersion.h>
#include <kexiutils/utils.h>
#include <kexiutils/KexiContextMessage.h>
#include <kexiutils/KexiFadeWidgetEffect.h>
#include "KexiUserFeedbackAgent.h"
#define KEXI_SKIP_SETUPPRIVATEICONSRESOURCE
#define KEXI_SKIP_SETUPBREEZEICONTHEME
#define KEXI_SKIP_REGISTERICONSRESOURCE
#define KEXI_SKIP_REGISTERRESOURCE
#include "KexiRegisterResource_p.h"

#include <KColorScheme>
#include <KStandardGuiItem>
#include <KConfigGroup>
#include <KIO/Job>
#include <KCodecs>
#include <KSharedConfig>
#include <KLocalizedString>
#include <KMessageBox>
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
#  include <QNetworkAccessManager>
#  include <QNetworkRequest>
#  include <QNetworkReply>
#  include <QSaveFile>
#else
#  define USE_KIO_COPY
#  include <KIO/CopyJob>
#endif

#include <QDebug>
#include <QEvent>
#include <QLayout>
#include <qmath.h>
#include <QFile>
#include <QDesktopServices>
#include <QUiLoader>
#include <QScrollArea>
#include <QLabel>
#include <QResource>
#include <QTimer>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QDir>
#include <QFontDatabase>
#include <QAction>
#include <QLocale>
#include <QRegularExpression>

#include <stdio.h>

class QNetworkReply;
class KJob;

static const int GUI_UPDATE_INTERVAL = 60; // update interval for GUI, in minutes
static const int DONATION_INTERVAL = 10; // donation interval, in days
static const int UPDATE_FILES_LIST_SIZE_LIMIT = 1024 * 128;
static const int UPDATE_FILES_COUNT_LIMIT = 128;

//! @return x.y.0
static QString stableVersionStringDot0()
{
    return QString::number(Kexi::stableVersionMajor()) + '.'
           + QString::number(Kexi::stableVersionMinor()) + ".0";
}

static QString uiPath(const QString &fname)
{
    KexiUserFeedbackAgent *f = KexiMainWindowIface::global()->userFeedbackAgent();
    return f->serviceUrl() + QString("/ui/%1/").arg(stableVersionStringDot0())
        + fname;
}

static QString basePath()
{
    return QString(KEXI_BASE_PATH "/status");
}

static QString findFileName(const QString &guiFileName)
{
    QStringList triedLocations;
    QString result = locateFile(QString(), basePath() + '/' + guiFileName,
                                QStandardPaths::GenericDataLocation, QString(), &triedLocations);
    if (result.isEmpty()) { // last chance: file from the source tree
        result = QFileInfo(QFile::decodeName(CMAKE_CURRENT_SOURCE_DIR "/status/") + guiFileName)
                    .canonicalFilePath();
    }
    //qDebug() << result;
    return result;
}

// ---

class Q_DECL_HIDDEN KexiWelcomeStatusBarGuiUpdater::Private : public QObject
{
    Q_OBJECT
public:
    Private()
     : configGroup(KConfigGroup(KSharedConfig::openConfig()->group("User Feedback")))
    {
    }

    KConfigGroup configGroup;

public Q_SLOTS:
    void sendRequestListFilesFinished(KJob* job)
    {
        if (job->error()) {
            qWarning() << "Error while receiving .list file - no files will be updated";
            //! @todo error...
            return;
        }
        KIO::StoredTransferJob* sendJob = qobject_cast<KIO::StoredTransferJob*>(job);
        QString result = sendJob->data();
        if (result.length() > UPDATE_FILES_LIST_SIZE_LIMIT) { // anti-DOS protection
            qWarning() << "Too large .list file (" << result.length()
                << "); the limit is" << UPDATE_FILES_LIST_SIZE_LIMIT
                << "- no files will be updated";
            return;
        }
        //qDebug() << result;
        QStringList data = result.split('\n', QString::SkipEmptyParts);
        result.clear();
        m_fileNamesToUpdate.clear();
        if (data.count() > UPDATE_FILES_COUNT_LIMIT) { // anti-DOS protection
            qWarning() << "Too many files to update (" << data.count()
                << "); the limit is" << UPDATE_FILES_COUNT_LIMIT
                << "- no files will be updated";
            return;
        }
        // OK, try to update (stage 1: check, stage 2: checking)
        for (int stage = 1; stage <= 2; stage++) {
            int i = 0;
            for (QStringList::ConstIterator it(data.constBegin()); it!=data.constEnd(); ++it, i++) {
                const QByteArray hash((*it).left(32).toLatin1());
                const QString remoteFname((*it).mid(32 + 2));
                if (stage == 1) {
                    if (hash.length() != 32) {
                        qWarning() << "Invalid hash" << hash << "in line" << i+1 << "- no files will be updated";
                        return;
                    }
                    if ((*it).mid(32, 2) != "  ") {
                        qWarning() << "Two spaces expected but found" << (*it).mid(32, 2)
                            << "in line" << i+1 << "- no files will be updated";
                        return;
                    }
                    if (remoteFname.contains(QRegularExpression("\\s"))) {
                        qWarning() << "Filename expected without whitespace but found" << remoteFname
                            << "in line" << i+1 << "- no files will be updated";
                        return;
                    }
                }
                else if (stage == 2) {
                    checkFile(hash, remoteFname, &m_fileNamesToUpdate);
                }
            }
        }
        if (m_fileNamesToUpdate.isEmpty()) {
            //qDebug() << "No files to update.";
            return;
        }
        // update files
        QList<QUrl> sourceFiles;
        foreach (const QString &fname, m_fileNamesToUpdate) {
            sourceFiles.append(QUrl(uiPath(fname)));
        }
        m_tempDir.reset(new QTemporaryDir(QDir::tempPath() + "/kexi-status"));
        //qDebug() << m_tempDir->path();
#ifdef USE_KIO_COPY
        KIO::CopyJob *copyJob = KIO::copy(sourceFiles,
                                          QUrl::fromLocalFile(m_tempDir->path()),
                                          KIO::HideProgressInfo | KIO::Overwrite);
        connect(copyJob, &KIO::CopyJob::result, this, &Private::filesCopyFinished);
#else
        if (!m_downloadManager) {
            m_downloadManager = new QNetworkAccessManager(this);
            connect(m_downloadManager.data(), &QNetworkAccessManager::finished,
                    this, &Private::fileDownloadFinished);
        }
        m_sourceFilesToDownload = sourceFiles;
        downloadNextFile();
#endif
        //qDebug() << "copying from" << QUrl(uiPath(fname)) << "to"
        //         << (dir + fname);
    }

private Q_SLOTS:
    void filesCopyFinished(KJob* job)
    {
#ifdef USE_KIO_COPY
        if (job->error()) {
            //! @todo error...
            qDebug() << "ERROR:" << job->errorString();
            m_tempDir.reset();
            return;
        }
        KIO::CopyJob* copyJob = qobject_cast<KIO::CopyJob*>(job);
        Q_UNUSED(copyJob)
        //qDebug() << "DONE" << copyJob->destUrl();
        (void)copyFilesToDestinationDir();
#else
        Q_UNUSED(job)
#endif
    }


    void fileDownloadFinished(QNetworkReply* reply)
    {
#ifdef USE_KIO_COPY
        Q_UNUSED(reply)
#else
        const bool ok = copyFile(reply);
        reply->deleteLater();
        if (!ok) {
            qWarning() << "Error downloading file" << m_sourceFilesToDownload.first();
            delete m_downloadManager;
            m_sourceFilesToDownload.clear();
            m_tempDir.reset();
        }
        m_sourceFilesToDownload.removeFirst();
        downloadNextFile();
#endif
    }

    bool copyFile(QNetworkReply* reply)
    {
#ifdef USE_KIO_COPY
        Q_UNUSED(reply)
#else
#define DOWNLOAD_BUFFER_SIZE 1024 * 50
        if (reply->error() != QNetworkReply::NoError) {
            return false;
        }
        const QString filename(m_sourceFilesToDownload.first().fileName());
        QString path(m_tempDir->path() + '/' + filename);
        QSaveFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            return false;
        }
        QByteArray buf(DOWNLOAD_BUFFER_SIZE, Qt::Uninitialized);
        while (!reply->atEnd()) {
            const qint64 size = reply->read(buf.data(), buf.size());
            if (size < 0) {
                return false;
            }
            if (f.write(buf.data(), size) != size) {
                return false;
            }
        }
        if (!f.commit()) {
            return false;
        }
#endif
        return true;
    }

private:
#ifndef USE_KIO_COPY
    void downloadNextFile() {
        if (m_sourceFilesToDownload.isEmpty()) {
            // success
            (void)copyFilesToDestinationDir();
            return;
        }
        m_downloadManager->get(QNetworkRequest(m_sourceFilesToDownload.first()));
    }
#endif

private:
    bool copyFilesToDestinationDir()
    {
        const QString dir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                          + QLatin1Char('/') + basePath() + '/');
        bool ok = true;
        if (!QDir(dir).exists()) {
            if (!QDir().mkpath(dir)) {
                ok = false;
                qWarning() << "Could not create" << dir;
            }
        }
        if (ok) {
            foreach (const QString &fname, m_fileNamesToUpdate) {
                const QByteArray oldName(QFile::encodeName(m_tempDir->path() + '/' + fname)),
                                 newName(QFile::encodeName(dir + fname));
                if (0 != ::rename(oldName.constData(), newName.constData())) {
                    qWarning() << "cannot move" << (m_tempDir->path() + '/' + fname) << "to" << (dir + fname);
                }
            }
        }
        QDir(m_tempDir->path()).removeRecursively();
        m_tempDir.reset();
        m_fileNamesToUpdate.clear();
        return ok;
    }

    void checkFile(const QByteArray &hash, const QString &remoteFname, QStringList *fileNamesToUpdate)
    {
        QString localFname = findFileName(remoteFname);
        if (localFname.isEmpty()) {
            fileNamesToUpdate->append(remoteFname);
            qDebug() << "missing filename" << remoteFname << "- download it";
            return;
        }
        QFile file(localFname);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "could not open file" << localFname << "- update it";
            fileNamesToUpdate->append(remoteFname);
            return;
        }
        QCryptographicHash md5(QCryptographicHash::Md5);
        if (!md5.addData(&file)) {
            qWarning() << "could not check MD5 for file" << localFname << "- update it";
            fileNamesToUpdate->append(remoteFname);
            return;
        }
        if (md5.result().toHex() != hash) {
            qDebug() << "not matching file" << localFname << "- update it";
            fileNamesToUpdate->append(remoteFname);
        }
    }

    QStringList m_fileNamesToUpdate;
    QScopedPointer<QTemporaryDir> m_tempDir;
#ifndef USE_KIO_COPY
    QList<QUrl> m_sourceFilesToDownload;
    QPointer<QNetworkAccessManager> m_downloadManager;
#endif
};

KexiWelcomeStatusBarGuiUpdater::KexiWelcomeStatusBarGuiUpdater()
 : QObject()
 , d(new Private)
{
}

KexiWelcomeStatusBarGuiUpdater::~KexiWelcomeStatusBarGuiUpdater()
{
    delete d;
}

void KexiWelcomeStatusBarGuiUpdater::update()
{
    QDateTime lastStatusBarUpdate = d->configGroup.readEntry("LastStatusBarUpdate", QDateTime());
    if (lastStatusBarUpdate.isValid()) {
        int minutes = lastStatusBarUpdate.secsTo(QDateTime::currentDateTime()) / 60;

        if (minutes < GUI_UPDATE_INTERVAL) {
            qDebug() << "gui updated" << minutes << "min. ago, next auto-update in"
                << (GUI_UPDATE_INTERVAL - minutes) << "min.";
            return;
        }
    }

    d->configGroup.writeEntry("LastStatusBarUpdate", QDateTime::currentDateTime());

    KexiUserFeedbackAgent *f = KexiMainWindowIface::global()->userFeedbackAgent();
    f->waitForRedirect(this, SLOT(slotRedirectLoaded()));
}

void KexiWelcomeStatusBarGuiUpdater::slotRedirectLoaded()
{
    QByteArray postData = stableVersionStringDot0().toLatin1();
    KIO::Job* sendJob = KIO::storedHttpPost(postData,
                                            QUrl(uiPath(".list")),
                                            KIO::HideProgressInfo);
    connect(sendJob, SIGNAL(result(KJob*)), d, SLOT(sendRequestListFilesFinished(KJob*)));
    sendJob->addMetaData("content-type", "Content-Type: application/x-www-form-urlencoded");
}

// ---

//! @internal
class ScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    explicit ScrollArea(QWidget *parent = 0) : QScrollArea(parent)
    {
        setFrameShape(QFrame::NoFrame);
        setBackgroundRole(QPalette::Base);
        setWidgetResizable(true);
    }

    void setEnabled(bool set) {
        if (set != isEnabled()) {
            QScrollArea::setEnabled(set);
            updateColors();
        }
    }

protected:
    virtual void changeEvent(QEvent* event)
    {
        switch (event->type()) {
        case QEvent::EnabledChange:
        case QEvent::PaletteChange:
            updateColors();
            break;
        default:;
        }
        QScrollArea::changeEvent(event);
    }

    void updateColors() {
        if (!widget())
            return;
        KColorScheme scheme(palette().currentColorGroup());
        QColor linkColor = scheme.foreground(KColorScheme::LinkText).color();
        //qDebug() << "_____________" << isEnabled();

        foreach(QLabel* lbl, widget()->findChildren<QLabel*>()) {
            QString t = lbl->text();
            QRegularExpression re("<a.*>", QRegularExpression::InvertedGreedinessOption);
            int pos = 0;
            int oldPos = 0;
            QString newText;
            QRegularExpressionMatch match = re.match(t);
            //qDebug() << "t:" << t;
            while ((pos = match.capturedStart(pos)) != -1) {
                //qDebug() << "pos:" << pos;
                //qDebug() << "newText += t.mid(oldPos, pos - oldPos)"
                //    << t.mid(oldPos, pos - oldPos);
                newText += t.midRef(oldPos, pos - oldPos);
                //qDebug() << "newText1:" << newText;
                //qDebug() << lbl->objectName() << "~~~~" << t.mid(pos, re.matchedLength());
                QString a = t.mid(pos, match.capturedLength());
                //qDebug() << "a:" << a;
                int colPos = a.indexOf("color:");
                if (colPos == -1) { // add color
                    a.insert(a.length() - 1, " style=\"color:" + linkColor.name() + ";\"");
                }
                else { // replace color
                    colPos += qstrlen("color:");
                    for (;colPos < a.length() && a[colPos] == ' '; colPos++) {
                    }
                    if (colPos < a.length() && a[colPos] == '#') {
                        colPos++;
                        int i = colPos;
                        for (;i < a.length(); i++) {
                            if (a[i] == ';' || a[i] == ' ' || a[i] == '"' || a[i] == '\'')
                                break;
                        }
                        //qDebug() << "******" << a.mid(colPos, i - colPos);
                        a.replace(colPos, i - colPos, linkColor.name().mid(1));
                    }
                }
                //qDebug() << "a2:" << a;
                newText += a;
                //qDebug() << "newText2:" << newText;
                pos += match.capturedLength();
                oldPos = pos;
                //qDebug() << "pos2:" << pos;
            }
            //qDebug() << "oldPos:" << oldPos;
            newText += t.midRef(oldPos);
            //qDebug() << "newText3:" << newText;
            lbl->setText(newText);
        }

#if 0
        QString text;
        text = QString("<a href=\"%1\" style=\"color:%2;\">%3</a>")
            .arg(link).arg(linkColor.name()).arg(linkText);
        if (!format.isEmpty()) {
            text = QString(format).replace("%L", text);
        }
        q->setText(text);
#endif
    }
};

// ---

class Q_DECL_HIDDEN KexiWelcomeStatusBar::Private
{
public:
    explicit Private(KexiWelcomeStatusBar* _q)
     : statusWidget(0),
       q(_q)
    {
        rccFname = findFileName("status.rcc");
        if (!rccFname.isEmpty())  {
            QResource::registerResource(rccFname);
        }
    }

    ~Private() {
        delete msgWidget;
        if (!rccFname.isEmpty())  {
            QResource::unregisterResource(rccFname);
        }
    }

    template<typename T>
    T widgetOfClass(T parent, const char *widgetName) const
    {
        T w = parent->template findChild<T>(widgetName);
        if (!w) {
            qWarning() << "NO SUCH widget" << widgetName << "in" << parent;
        }
        return w;
    }

    QWidget* widget(QWidget *parent, const char *widgetName) const
    {
        return widgetOfClass<QWidget*>(parent, widgetName);
    }

    QObject* object(QObject *parent, const char *objectName) const
    {
        QObject *o = parent->findChild<QObject*>(objectName);
        if (!o) {
            qWarning() << "NO SUCH object" << objectName << "in" << parent;
        }
        return o;
    }

    void setProperty(QWidget *parent, const char *widgetName,
                     const char *propertyName, const QVariant &value)
    {
        QWidget *w = widget(parent, widgetName);
        if (w) {
            w->setProperty(propertyName, value);
        }
    }

    QVariant property(QWidget *parent, const char *widgetName, const char *propertyName) const
    {
        QWidget *w = widget(parent, widgetName);
        return w ? w->property(propertyName) : QVariant();
    }

    void connect(QWidget *parent, const char *widgetName, const char *signalName,
                 QObject *receiver, const char *slotName)
    {
        QWidget *w = widget(parent, widgetName);
        if (w) {
            QObject::connect(w, signalName, receiver, slotName);
        }
    }

    void animatedHide(QWidget *parent, const char *widgetName)
    {
        QWidget *w = widget(parent, widgetName);
        if (!w)
            return;
        KexiFadeWidgetEffect *animation = new KexiFadeWidgetEffect(w);
        QObject::connect(animation, SIGNAL(destroyed()), w, SLOT(hide()));
        animation->start();
    }

    QWidget* loadGui(const QString &guiFileName, QWidget *parentWidget = 0)
    {
        QString fname = findFileName(guiFileName);
        if (fname.isEmpty()) {
            qWarning() << "filename" << fname << "not found";
            return 0;
        }
        QFile file(fname);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "could not open file" << fname;
            return 0;
        }
        QUiLoader loader;
        QWidget* widget = loader.load(&file, parentWidget);
        if (!widget) {
            qWarning() << "could load ui from file" << fname;
        }
        file.close();
        return widget;
    }

    void updateStatusWidget()
    {
        QWidget *widget = loadGui("status.ui", statusScrollArea);
        if (!widget) {
            return;
        }
        int smallFontSize = qFloor((QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont).pointSizeF()
                                   + q->font().pointSizeF())
                                   / 2.0);
        smallFont = q->font();
        smallFont.setPointSizeF(smallFontSize);
        widget->setFont(smallFont);
        //delete statusWidget;
        statusWidget = widget;
        statusScrollArea->setWidget(statusWidget);

        QString donationText = property(statusWidget, "link_donate", "text").toString();
        donationText.remove(QStringLiteral("(+%1%)"));
        setProperty(statusWidget, "link_donate", "text", donationText);

        // do not alter background palette
        QPalette pal(widget->palette());
        pal.setColor(QPalette::Disabled, QPalette::Base,
                     pal.color(QPalette::Normal, QPalette::Base));
        widget->setPalette(pal);

        setProperty(statusWidget, "donation_url", "visible", false);
        connect(statusWidget, "link_donate", SIGNAL(linkActivated(QString)),
                q, SLOT(showDonation()));
    }

    enum CalloutAlignment {
        AlignToBar,
        AlignToWidget
    };

    //! Aligns callout pointer position of msgWidget to widget named @a alignToWidgetName
    void setMessageWidgetCalloutPointerPosition(
        const QString& alignToWidgetName,
        CalloutAlignment calloutAlignment = AlignToBar)
    {
            //qDebug() << q->pos() << q->mapToGlobal(QPoint(0, 100));
            QPoint p(q->mapToGlobal(QPoint(0, 100)));
            QWidget *alignToWidget = this->widget(statusWidget, alignToWidgetName.toLatin1());
            if (alignToWidget) {
                p.setY(
                    alignToWidget->mapToGlobal(
                        QPoint(-5, alignToWidget->height() / 2)).y());
                if (calloutAlignment == AlignToWidget) {
                    p.setX(alignToWidget->mapToGlobal(QPoint(-5, 0)).x());
                    //qDebug() << p;
                }
            }
            else {
                qWarning() << alignToWidgetName << "not found!";
            }
            msgWidget->setCalloutPointerPosition(p, alignToWidget);
    }

    //! Shows message widget taking maximum space within the welcome page
    //! Returns created layout for further use into @a layout.
    //! Created widge is assigned to msgWidget.
    //! Calls slot @a slotToCallAfterShow after animated showing, if provided.
    //! Call msgWidget->animatedShow() afterwards.
    void showMaximizedMessageWidget(const QString &alignToWidgetName,
                                    QPointer<QGridLayout> *layout,
                                    const char* slotToCallAfterShow,
                                    CalloutAlignment calloutAlignment = AlignToBar)
    {
        QWidget *alignToWidget = this->widget(statusWidget, alignToWidgetName.toLatin1());
        int msgWidth;
        if (alignToWidget && calloutAlignment == AlignToWidget) {
            msgWidth = q->parentWidget()->width() - alignToWidget->width() - 10;
        }
        else {
            msgWidth = q->parentWidget()->width() - q->width();
        }
        QWidget *widget = new QWidget;
        *layout = new QGridLayout(widget);
        if (msgWidth > 100) { // nice text margin
            (*layout)->setColumnMinimumWidth(0, 50);
        }
        //qDebug() << (q->parentWidget()->width() - q->width()) << "***";
        KexiContextMessage msg(widget);
        if (msgWidget) {
            delete static_cast<KexiContextMessageWidget*>(msgWidget);
        }
        msgWidget
            = new KexiContextMessageWidget(q->parentWidget()->parentWidget(), 0, 0, msg);
        msgWidget->setCalloutPointerDirection(KMessageWidget::Right);
        msgWidget->setMessageType(KMessageWidget::Information);
        msgWidget->setCloseButtonVisible(true);
        int offset_y = 0;
        if (alignToWidget) {
            offset_y = alignToWidget->mapToGlobal(QPoint(0, 0)).y()
                       - q->parentWidget()->mapToGlobal(QPoint(0, 0)).y();
        }
        else {
            qWarning() << alignToWidgetName << "not found!";
        }
        msgWidget->resize(msgWidth, q->parentWidget()->height() - offset_y);
        setMessageWidgetCalloutPointerPosition(alignToWidgetName, calloutAlignment);
        msgWidget->setResizeTrackingPolicy(Qt::Horizontal | Qt::Vertical);
        statusScrollArea->setEnabled(false);
        // async show to for speed up
        if (slotToCallAfterShow) {
            QObject::connect(msgWidget, SIGNAL(animatedShowFinished()),
                            q, slotToCallAfterShow);
        }
        QObject::connect(msgWidget, SIGNAL(animatedHideFinished()),
                         q, SLOT(slotMessageWidgetClosed()));
    }

    ScrollArea *statusScrollArea;
    QWidget *statusWidget;
    QVBoxLayout *lyr;
    QPointer<KexiContextMessageWidget> msgWidget;
    QFont smallFont;
    bool detailsDataVisible = false;

    KexiWelcomeStatusBarGuiUpdater guiUpdater;
private:
    QString rccFname;
    KexiWelcomeStatusBar *q;
    QMap<QString, QString> dict;
};

KexiWelcomeStatusBar::KexiWelcomeStatusBar(QWidget* parent)
 : QWidget(parent), d(new Private(this))
{
    d->lyr = new QVBoxLayout(this);

    init();
}

KexiWelcomeStatusBar::~KexiWelcomeStatusBar()
{
    delete d;
}

void KexiWelcomeStatusBar::init()
{
    d->statusScrollArea = new ScrollArea(this);
    d->lyr->addWidget(d->statusScrollArea);

    d->updateStatusWidget();
    QTimer::singleShot(10, &d->guiUpdater, SLOT(update()));
}

void KexiWelcomeStatusBar::showDonation()
{
    if (!sender()) {
        return;
    }
    if (KMessageBox::Yes != KMessageBox::questionYesNo(this,
       xi18nc("@info donate to the project", "<title>Kexi may be totally free, but its development is costly.</title>"
            "<para>Power, hardware, office space, internet access, traveling for meetings - everything costs.</para>"
            "<para>Direct donation is the easiest and fastest way to efficiently support the Kexi Project. "
            "Everyone, regardless of any degree of involvement can do so.</para>"
            "<para>What do you receive for your donation? Kexi will become more feature-full and stable as "
            "contributors will be able to devote more time to Kexi. Not only you can "
            "expect new features, but you can also have an influence on what features are added!</para>"
            "<para>Currently we are accepting donations through <emphasis>BountySource</emphasis> (a funding platform "
            "for open-source software) using secure PayPal, Bitcoin and Google Wallet transfers.</para>"
            "<para>Contact us at <link url='https://community.kde.org/Kexi/Contact'>https://community.kde.org/Kexi/Contact</link> "
            "for more information.</para>"
            "<para>Thanks for your support!</para>"),
       xi18n("Donate to the Project"),
       KGuiItem(xi18nc("@action:button Go to Donation", "Proceed to the Donation Web Page"), QIcon(":/icons/heart.png")),
       KGuiItem(xi18nc("Do not donate now", "Not Now")),
       QString(),
       KMessageBox::Notify | KMessageBox::AllowLink))
    {
        return;
    }
    QUrl donationUrl(d->property(this, "donation_url", "text").toString());
    if (donationUrl.isValid()) {
        QDesktopServices::openUrl(donationUrl);
        KConfigGroup configGroup(KSharedConfig::openConfig()->group("User Feedback"));
        configGroup.writeEntry("LastDonation", QDateTime::currentDateTime());
    }
    else {
        qWarning() << "Invalid donation URL" << donationUrl;
    }
}

#include "KexiWelcomeStatusBar.moc"
