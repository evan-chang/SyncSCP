/*
 * Syncex - For cross platform exchanger syncronization your data on multi-platform PC
 *
 * Support WinXP/Vista/7, Mac OS X Leopard/Snow Leopard, Ubunto
 * Author : Evan Chang
 *
 * Reserved Copyrightc Evan Chang
 */

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>
#include "syncexserver.h"

CSyncexServer::CSyncexServer(QWidget *parent) :
    QDialog(parent),
    m_pSyncexServer(NULL)
{
    m_pLabelPort = new QLabel(tr("port"), this);
    m_pEditServerPort = new QLineEdit(tr("%1") .arg(DEFAULT_SERVER_PORT), this);
    m_pEditServerPort->setValidator(new QIntValidator(1, 65535, this));
    m_pLabelPort->setBuddy(m_pEditServerPort);

    m_pLabelSyncDir = new QLabel(tr("SyncDirectory:"), this);
    m_pEditSyncDir = new QLineEdit(this);
    m_pLabelSyncDir->setBuddy(m_pEditSyncDir);
    m_pButtonSyncDir = new QPushButton(tr("Browse"), this);
    connect(m_pButtonSyncDir, SIGNAL(clicked()), this, SLOT(slotOpenSyncDirectory()));

    m_pLabelScanPeriod = new QLabel(tr("Scan period (min):"), this);
    m_pEditScanPeriod = new QLineEdit(tr("%1") .arg(DEFAULT_SCAN_IN_MIN), this);
    m_pTimer = new QTimer(this);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(slotOnTimer()));
    m_pFileListMutex = new QMutex(QMutex::Recursive);

    m_pButtonRun = new QPushButton(tr("Run"), this);
    m_pButtonRun->setDefault(true);
    m_pButtonRun->setDisabled(true);
    connect(m_pButtonRun, SIGNAL(clicked()), this, SLOT(slotCreateServer()));
    m_pButtonStop = new QPushButton(tr("Stop"));
    m_pButtonStop->setDisabled(true);
    connect(m_pButtonStop, SIGNAL(clicked()), this, SLOT(slotStopServer()));

    m_pTEditLog = new QTextEdit(this);
    m_pTEditLog->setFixedSize(500, 200);

    QGridLayout *pLayout = new QGridLayout(this);
    pLayout->addWidget(m_pLabelPort, 0, 0);
    pLayout->addWidget(m_pEditServerPort, 0, 1);
    pLayout->addWidget(m_pLabelSyncDir, 1, 0);
    pLayout->addWidget(m_pEditSyncDir, 1, 1);
    pLayout->addWidget(m_pButtonSyncDir, 1, 2);
    pLayout->addWidget(m_pLabelScanPeriod, 2, 0);
    pLayout->addWidget(m_pEditScanPeriod, 2, 1);
    pLayout->addWidget(m_pButtonRun, 3, 3);
    pLayout->addWidget(m_pButtonStop, 3, 4);
    pLayout->addWidget(m_pTEditLog, 5, 0, 1, 5);

    setWindowTitle(tr("Syncex Server"));

    connect(m_pEditServerPort, SIGNAL(textChanged(QString)), this, SLOT(slotEnableButtonRun()));
    connect(m_pEditSyncDir, SIGNAL(textChanged(QString)), this, SLOT(slotEnableButtonRun()));
    connect(m_pEditScanPeriod, SIGNAL(textChanged(QString)), this, SLOT(slotEnableButtonRun()));
}

CSyncexServer::~CSyncexServer()
{
    if (m_pFileListMutex)
        delete m_pFileListMutex;

    if (m_pSyncexServer)
        qDeleteAll(m_SocketList);
}

void CSyncexServer::slotOpenSyncDirectory()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::Directory);

    if (dialog.exec())
    {
        m_szSyncDir = dialog.selectedFiles().at(0);
        m_pEditSyncDir->setText(m_szSyncDir);
    }
}

bool CSyncexServer::hashSyncFiles()
{
    if (m_szSyncDir.isEmpty())
        return false;

    // protect this critical section
    m_pFileListMutex->lock();

    displayLog(tr("Scanning syncronization files"));

    // --- Critical Section Start ---

    // reserver old list
    m_orgFileNameList = m_FileNameList;
    m_orgFileMd5List = m_FileMd5List;
    m_orgFileModifyTimeList = m_FileModifyTimeList;
    m_FileNameList.clear();
    m_FileMd5List.clear();
    m_FileModifyTimeList.clear();

    QString szCurDir = m_szSyncDir;
    QDirIterator it(szCurDir, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString szCurFilePath = it.next();
        if (it.fileInfo().isDir() || it.fileInfo().isSymLink() || it.fileInfo().isRelative() || szCurFilePath.endsWith(".md5", Qt::CaseInsensitive))
            continue;

        // append to file tree list
        QString szCurFileName(it.filePath().remove(szCurDir));
        QByteArray qbMd5Hash;
        QFile qfCurFile(szCurFilePath);
        QFileInfo qfInfo(qfCurFile);

        uint uiLastModifyTime = qfInfo.lastModified().toTime_t();
        int index = m_orgFileNameList.indexOf(szCurFileName);
        if ((index > -1) && (uiLastModifyTime == m_orgFileModifyTimeList[index].toUInt()))
        {
            // Find this file in original file name list,
            // and the last modify time is the same,
            // it means this file has not been changed.
            m_FileNameList.append(m_orgFileNameList.at(index));
            m_FileMd5List.append(m_orgFileMd5List.at(index));
            m_FileModifyTimeList.append(m_orgFileModifyTimeList.at(index));

            continue;
        }

        // This file is new added/modified.
        if (qfCurFile.open(QIODevice::ReadOnly))
        {
            qbMd5Hash = QCryptographicHash::hash(qfCurFile.readAll(), QCryptographicHash::Md5);

            if ((index >= 0) && (QString(qbMd5Hash) == m_orgFileMd5List[index]))
            {
                // This file is not modified, update m_orgFileModifyTimeList,
                // or it'll cause sync files compare different
                Q_ASSERT(index > -1);
                m_orgFileModifyTimeList[index] = tr("%1") .arg(uiLastModifyTime);
            }

            m_FileNameList.append(szCurFileName);
            m_FileMd5List.append(QString(qbMd5Hash));
            m_FileModifyTimeList.append(tr("%1") .arg(uiLastModifyTime));

            qfCurFile.close();
        }
        else
        {
            displayLog(tr("Hash file failed, name = %1") .arg(szCurFilePath));
        }
    }

    displayLog(tr("Scan process complete"));

    // --- Critical Section End ---
    m_pFileListMutex->unlock();
    return true;
}

void CSyncexServer::slotCreateServer()
{
    // exit if server is already created
    if (m_pSyncexServer)
        return;

    // check for syncronization directory setting
    if (m_szSyncDir.isEmpty())
        m_szSyncDir = m_pEditSyncDir->text();

    if (!hashSyncFiles())
    {
        displayLog(tr("Hash failed : Please set up directory to syncronization !"));
        return;
    }

    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    int i;
    for (i = 0; i < ipAddressesList.size(); ++i)
    {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost && ipAddressesList.at(i).toIPv4Address())
        {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }

    if (i == ipAddressesList.size())
    {
        displayLog(tr("Find no ip address to listen"));
        return;
    }

    m_pSyncexServer = new QTcpServer(this);
    if (!m_pSyncexServer->listen(ipAddressesList.at(i), m_pEditServerPort->text().toInt()))
    {
        displayLog(tr("Initializing server failed"));
        return;
    }

    connect(m_pSyncexServer, SIGNAL(newConnection()), this, SLOT(slotConnections()));
    m_pTimer->start(m_pEditScanPeriod->text().toInt()*DEFAULT_SCAN_PERIOD);

    displayLog(tr("Server creation complete ip=%1, port=%2") .arg(ipAddress, m_pEditServerPort->text()));
    displayLog(tr("Waiting for connections"));

    m_pButtonRun->setEnabled(false);
    m_pButtonStop->setEnabled(true);
}

void CSyncexServer::slotStopServer()
{
    qDeleteAll(m_SocketList);
    m_pSyncexServer->deleteLater();
    m_pSyncexServer = NULL;

    slotEnableButtonRun();
    m_pButtonStop->setEnabled(false);
}

void CSyncexServer::slotConnections()
{
    while(m_pSyncexServer->hasPendingConnections())
    {
        displayLog(tr("One client connection received"));
        QTcpSocket *pClient = m_pSyncexServer->nextPendingConnection();
        connect(pClient, SIGNAL(readyRead()), this, SLOT(slotDataReceived()));
        connect(pClient, SIGNAL(disconnected()), this, SLOT(slotDisconnections()));
        m_SocketList.append(pClient);
        displayLog(tr("Connection completed from %1") .arg(pClient->peerAddress().toString()));
    }
}

void CSyncexServer::slotDisconnections()
{
    QTcpSocket *pClient = qobject_cast<QTcpSocket*>(sender());
    if (!pClient)
        return;

    pClient->deleteLater();
    displayLog(tr("One client disconnected from %1") .arg(pClient->peerAddress().toString()));
    m_SocketList.removeAll(pClient);
}

void CSyncexServer::slotDataReceived()
{
    QTcpSocket *pClient = qobject_cast<QTcpSocket*>(sender());
    if (!pClient)
        return;

    QByteArray inblock;
    QDataStream in(&inblock, QIODevice::ReadOnly);
    inblock = pClient->readAll();

    qint64 socketAvailable = 0;
    while ((socketAvailable = in.device()->bytesAvailable()) > 0) // size SYNC_STARTCODE(4) + AtomSocketSize(8)
    {
        quint32 startcode = 0, endcode = 0;
        quint64 ui64SocketSize = 0;

        if (socketAvailable < 12)
        { // do data copy with uncertain socket size
            m_qbBuffer.clear();
            in.device()->seek(in.device()->pos()-socketAvailable);
            m_qbBuffer = in.device()->readAll();
            break;
        }
        else
        {
            in >> startcode >> ui64SocketSize;
        }

        if (startcode == SYNCACTION::SYNC_STARTCODE)
        {
            if (ui64SocketSize > socketAvailable)
            { // means there will have remaining data in next packet. Hence, save it in buffer
                m_qbBuffer.clear();
                in.device()->seek(in.device()->pos()-12);
                m_qbBuffer = in.device()->readAll();
                break;
            }
        }
        else //maybe remain data or not an acceptted packet
        {
            inblock.prepend(m_qbBuffer);
            in.device()->waitForBytesWritten(1000);
            in.device()->seek(0);
            continue;
        }

        if (startcode == SYNCACTION::SYNC_STARTCODE)
        {
            quint8 mainAction = 0;
            in >> mainAction;
            switch (mainAction)
            {
            case SYNCACTION::SYNC_GET:
                {
                    quint8 subAction = 0;
                    in >> subAction;
                    switch (subAction)
                    {
                    case SYNCACTION::SYNC_SUB_MD5_LIST:
                        {
                            hashSyncFiles();

                            displayLog(tr("Receive from client: request file lists"));
                            QByteArray outblock;
                            QDataStream out(&outblock, QIODevice::ReadWrite);
                            out << SYNCACTION::SYNC_STARTCODE << (quint64)0 << (quint8)SYNCACTION::SYNC_REPLY << (quint8)SYNCACTION::SYNC_SUB_MD5_LIST;

                            out << m_FileNameList;
                            out << m_FileMd5List;
                            out << m_FileModifyTimeList;

                            out << SYNCACTION::SYNC_ENDCODE;

                            out.device()->seek(4);
                            out << (quint64)outblock.size();
                            pClient->write(outblock);
                            pClient->flush();
                            break;
                        }
                    case SYNCACTION::SYNC_SUB_FILE:
                        {
                            QString szRequestFileName;
                            in >> szRequestFileName;
                            displayLog(tr("Receive from client: request file : %1, sending ...") .arg(szRequestFileName));
                            QString szRequestFilePath(tr("%1%2") .arg(m_szSyncDir, szRequestFileName));

                            QFile qfRequestFile(szRequestFilePath);
                            qfRequestFile.open(QIODevice::ReadOnly);

                            QByteArray outblock;
                            QDataStream out(&outblock, QIODevice::WriteOnly);

                            out << SYNCACTION::SYNC_STARTCODE << (quint64)0 << (quint8)SYNCACTION::SYNC_REPLY << (quint8)SYNCACTION::SYNC_SUB_FILE;
                            out << szRequestFileName;
                            out.writeBytes(qfRequestFile.readAll().data(), qfRequestFile.size());
                            out << SYNCACTION::SYNC_ENDCODE;

                            out.device()->seek(4);
                            out << (quint64)outblock.size();
                            pClient->write(outblock);
                            pClient->flush();
                            qfRequestFile.close();
                            break;
                        }
                    default:
                        displayLog(tr("Receive from client: GET command, and there is No relative operation"));
                    }

                    break;
                }
            case SYNCACTION::SYNC_POST:
                {
                    quint8 subAction = 0;
                    in >> subAction;

                    switch (subAction)
                    {
                    case SYNCACTION::SYNC_SUB_MD5_LIST:
                        {
                            break;
                        }
                    case SYNCACTION::SYNC_SUB_FILE:
                        {
                            QString szReadin;
                            in >> szReadin;
                            displayLog(tr("Receive from client: POST file : %1, writing ...") .arg(szReadin));
                            quint32 dataLength = 0;
                            in >> dataLength;
                            char *pData = (char*)malloc(sizeof(char)*dataLength);
                            int iRet = in.readRawData(pData, dataLength);

                            QString szFileName(tr("%1%2") .arg(m_szSyncDir) .arg(szReadin));
                            QString szTmp = szFileName.left(szFileName.lastIndexOf("/"));
                            QDir qdTmp(szTmp);
                            qdTmp.mkpath(szTmp);
                            QFile qfFile(szFileName);

                            qfFile.open(QIODevice::WriteOnly);
                            qfFile.write(pData, iRet);
                            qfFile.waitForBytesWritten(-1);
                            qfFile.close();
                            free(pData);
                            break;
                        }
                    default:
                        displayLog(tr("Receive from client: GET command, and there is No relative operation"));
                    }

                    break;
                }
            case SYNCACTION::SYNC_REPLY:
                {
                    quint8 subAction = 0;
                    in >> subAction;

                    switch (subAction)
                    {
                    case SYNCACTION::SYNC_SUB_MD5_LIST:
                        {
                            hashSyncFiles();

                            QStringList szRemoteFileNameList;
                            QStringList szRemoteFileMd5List;
                            QStringList szRemoteFileModifyTimeList;

                            in >> szRemoteFileNameList;
                            in >> szRemoteFileMd5List;
                            in >> szRemoteFileModifyTimeList;

                            // --- Critical Section Start ---
                            m_pFileListMutex->lock();

                            displayLog(tr("Receive from server: request file lists"));

                            // Check for status of local files, include add/modify/rename/remove.
                            for (int i=0; i<m_orgFileNameList.size(); i++)
                            {
                                int index = m_FileNameList.indexOf(m_orgFileNameList[i]);
                                if (index > -1)
                                {
                                    if (m_orgFileModifyTimeList.at(i) != m_FileModifyTimeList.at(index))
                                    {
                                        QString szModifyFileName = m_FileNameList.at(index);
                                        QString szModifyFileMd5 = m_FileMd5List.at(index);
                                        QString szModifyFileLastModifyTime = m_FileModifyTimeList.at(index);
                                        index = szRemoteFileNameList.indexOf(szModifyFileName);
                                        if (index > -1)
                                        {
                                        // This file has been modified.
                                            displayLog(tr("Detect one file has been modified, name = \"%1\"") .arg(szModifyFileName));

                                            QByteArray outblock;
                                            QDataStream out(&outblock, QIODevice::WriteOnly);

                                            out << SYNCACTION::SYNC_STARTCODE << (quint64)0 << (quint8)SYNCACTION::SYNC_OPERATION << (quint8)SYNCACTION::SYNC_SUB_FILE_MODIFY;
                                            out << m_FileNameList.at(index);

                                            QFile qfLocal(tr("%1%2") .arg(m_szSyncDir, szModifyFileName));
                                            qfLocal.open(QIODevice::ReadOnly);

                                            out.writeBytes(qfLocal.readAll().data(), qfLocal.size());
                                            out << SYNCACTION::SYNC_ENDCODE;

                                            out.device()->seek(4);
                                            out << (quint64)outblock.size();
                                            pClient->write(outblock);
                                            pClient->flush();

                                            // Update remote lists
                                            szRemoteFileNameList.replace(index, szModifyFileName);
                                            szRemoteFileMd5List.replace(index, szModifyFileMd5);
                                            szRemoteFileModifyTimeList.replace(index, szModifyFileLastModifyTime);
                                        }
                                        else
                                        {
                                            // Treat it as a new add file, and deal with it in following instructions below; do nothing here.
                                        }
                                    }
                                    else
                                    {
                                        // This file hasn't been modified, do nothing here.
                                    }
                                }
                                else
                                {
                                    // Can't find in current lists, may be renamed/modified.
                                    index = m_FileMd5List.indexOf(m_orgFileMd5List.at(i));
                                    if (index > -1)
                                    {
                                        // This file has been renamed.
                                        QString szOrgFileName = m_orgFileNameList.at(i);
                                        QString szRenFileName = m_FileNameList.at(index);
                                        QString szRenFileMd5 = m_FileMd5List.at(index);
                                        QString szRenFileModifyTime = m_FileModifyTimeList.at(index);

                                        displayLog(tr("Detect one file \"%1\" has been renamed to \"%2\"") .arg(szOrgFileName, szRenFileName));

                                        index = szRemoteFileNameList.indexOf(szOrgFileName);
                                        if (index > -1)
                                        {
                                            // Find on remote, tell remote to rename this file.
                                            QByteArray outblock;
                                            QDataStream out(&outblock, QIODevice::WriteOnly);

                                            out << SYNCACTION::SYNC_STARTCODE << (quint64)0 << (quint8)SYNCACTION::SYNC_OPERATION << (quint8)SYNCACTION::SYNC_SUB_FILE_RENAME;
                                            out << szOrgFileName << szRenFileName << szRenFileMd5 << szRenFileModifyTime;
                                            out << SYNCACTION::SYNC_ENDCODE;

                                            out.device()->seek(4);
                                            out << (quint64)outblock.size();
                                            pClient->write(outblock);
                                            pClient->flush();

                                            // Update remote lists
                                            szRemoteFileNameList.replace(index, szRenFileName);
                                            szRemoteFileMd5List.replace(index, szRenFileMd5);
                                            szRemoteFileModifyTimeList.replace(index, szRenFileModifyTime);
                                        }
                                        else
                                        {
                                            // Can't find on remote, do nothing
                                        }
                                    }
                                    else
                                    {
                                    // This file has been removed.
                                    QString removedFileName = m_orgFileNameList[i];

                                        displayLog(tr("Detect one file has been removed, name = \"%1\"") .arg(removedFileName));

                                    index = szRemoteFileNameList.indexOf(removedFileName);
                                    if (index > -1)
                                    {
                                        QByteArray outblock;
                                        QDataStream out(&outblock, QIODevice::WriteOnly);

                                        out << SYNCACTION::SYNC_STARTCODE << (quint64)0 << (quint8)SYNCACTION::SYNC_OPERATION << (quint8)SYNCACTION::SYNC_SUB_FILE_REMOVE;
                                        out << removedFileName;
                                        out << SYNCACTION::SYNC_ENDCODE;

                                        out.device()->seek(4);
                                        out << (quint64)outblock.size();
                                        pClient->write(outblock);
                                            pClient->flush();

                                            // Update remote lists
                                            szRemoteFileNameList.removeAt(index);
                                            szRemoteFileMd5List.removeAt(index);
                                            szRemoteFileModifyTimeList.removeAt(index);
                                        }
                                        else
                                        {
                                            // It doesn't exist on remote, do nothing.
                                        }
                                    }
                                }
                            }

                            /*
                             * Notice :
                             *
                             * We must update both remote and local lists to avoid send/receive the same data twice.
                             *
                             */
                            // compare from server list
                            for (int i=0; i<szRemoteFileNameList.size(); i++)
                            {
                                int index = m_FileNameList.indexOf(szRemoteFileNameList.at(i));
                                if (index > -1)
                                {
                                    // find the same file name in local
                                    if (szRemoteFileMd5List.at(i) != m_FileMd5List.at(index))
                                    {
                                        // md5 is different
                                        uint uiRemoteFileModifyTime = szRemoteFileModifyTimeList.at(i).toUInt();
                                        uint uiLocalFileModifyTime = m_FileModifyTimeList.at(index).toUInt();

                                        if (uiRemoteFileModifyTime > uiLocalFileModifyTime)
                                        {
                                            // use remote file
                                            displayLog(tr("Post to server: find new file on server, request it from server : \n\t%1") .arg(szRemoteFileNameList.at(i)));

                                            QByteArray outblock;
                                            QDataStream out(&outblock, QIODevice::WriteOnly);

                                            out << SYNCACTION::SYNC_STARTCODE << (quint64)0 << (quint8)SYNCACTION::SYNC_GET << (quint8)SYNCACTION::SYNC_SUB_FILE;
                                            out << szRemoteFileNameList.at(i);
                                            out << SYNCACTION::SYNC_ENDCODE;

                                            out.device()->seek(4);
                                            out << (quint64)outblock.size();
                                            pClient->write(outblock);
                                            pClient->flush();

                                            // --- update local list ---
                                            m_FileNameList.replace(index, szRemoteFileNameList.at(i));
                                            m_FileMd5List.replace(index, szRemoteFileMd5List.at(i));
                                            m_FileModifyTimeList.replace(index, szRemoteFileModifyTimeList.at(i));
                                        }
                                        else if (uiRemoteFileModifyTime < uiLocalFileModifyTime)
                                        {
                                            // use local file and update to server
                                            displayLog(tr("Post to server: find new file on local, send it to server : \n\t%1") .arg(m_FileNameList.at(index)));

                                            QByteArray outblock;
                                            QDataStream out(&outblock, QIODevice::WriteOnly);

                                            out << SYNCACTION::SYNC_STARTCODE << (quint64)0 << (quint8)SYNCACTION::SYNC_POST << (quint8)SYNCACTION::SYNC_SUB_FILE;
                                            out << m_FileNameList.at(index);

                                            QFile qfLocal(tr("%1%2") .arg(m_szSyncDir, m_FileNameList.at(index)));
                                            qfLocal.open(QIODevice::ReadOnly);

                                            out.writeBytes(qfLocal.readAll().data(), qfLocal.size());
                                            out << SYNCACTION::SYNC_ENDCODE;

                                            out.device()->seek(4);
                                            out << (quint64)outblock.size();
                                            pClient->write(outblock);
                                            pClient->flush();

                                            // --- update remote list ---
                                            szRemoteFileNameList.replace(i, m_FileNameList.at(index));
                                            szRemoteFileMd5List.replace(i, m_FileMd5List.at(index));
                                            szRemoteFileModifyTimeList.replace(i, m_FileModifyTimeList.at(index));
                                        }
                                        else
                                        {
                                            // error, Md5 is different but modify date is the same
                                            displayLog(tr("error, Md5 is different but modify date is the same : file %1") .arg(m_FileNameList.at(index)));
                                        }
                                    }
                                    else
                                    {
                                        // Md5 is the same, do nothing
                                    }
                                }
                                else
                                {
                                    // Cannot find this file in local, get from server
                                    displayLog(tr("Find no file on local, request from server : \n\t%1") .arg(szRemoteFileNameList.at(i)));

                                    QByteArray outblock;
                                    QDataStream out(&outblock, QIODevice::WriteOnly);

                                    out << SYNCACTION::SYNC_STARTCODE << (quint64)0 << (quint8)SYNCACTION::SYNC_GET << (quint8)SYNCACTION::SYNC_SUB_FILE;
                                    out << szRemoteFileNameList.at(i);
                                    out << SYNCACTION::SYNC_ENDCODE;

                                    out.device()->seek(4);
                                    out << (quint64)outblock.size();
                                    pClient->write(outblock);
                                    pClient->flush();

                                    // --- update local list ---
                                    m_FileNameList.append(szRemoteFileNameList.at(i));
                                    m_FileMd5List.append(szRemoteFileMd5List.at(i));
                                    m_FileModifyTimeList.append(szRemoteFileModifyTimeList.at(i));
                                }
                            }

                            // compare from local list
                            for (int i=0; i<m_FileNameList.size(); i++)
                            {
                                int index = szRemoteFileNameList.indexOf(m_FileNameList.at(i));
                                if (index > -1)
                                {
                                    // find the same file name in remote list
                                    if (m_FileMd5List.at(i) != szRemoteFileMd5List.at(index))
                                    {
                                        // md5 is different
                                        uint uiRemoteFileModifyTime = szRemoteFileModifyTimeList.at(index).toUInt();
                                        uint uiLocalFileModifyTime = m_FileModifyTimeList.at(i).toUInt();

                                        if (uiRemoteFileModifyTime > uiLocalFileModifyTime)
                                        {
                                            // use remote file
                                            displayLog(tr("Post to server: find new file on server, request it from server : \n\t%1") .arg(szRemoteFileNameList.at(index)));

                                            QByteArray outblock;
                                            QDataStream out(&outblock, QIODevice::WriteOnly);

                                            out << SYNCACTION::SYNC_STARTCODE << (quint64)0 << (quint8)SYNCACTION::SYNC_GET << (quint8)SYNCACTION::SYNC_SUB_FILE;
                                            out << szRemoteFileNameList.at(index);
                                            out << SYNCACTION::SYNC_ENDCODE;

                                            out.device()->seek(4);
                                            out << (quint64)outblock.size();
                                            pClient->write(outblock);
                                            pClient->flush();
                                        }
                                        else if (uiRemoteFileModifyTime < uiLocalFileModifyTime)
                                        {
                                            // use local file and update to server
                                            displayLog(tr("Post to server: find new file on local, send it to server : \n\t%1") .arg(m_FileNameList.at(i)));

                                            QByteArray outblock;
                                            QDataStream out(&outblock, QIODevice::WriteOnly);

                                            out << SYNCACTION::SYNC_STARTCODE << (quint64)0 << (quint8)SYNCACTION::SYNC_POST << (quint8)SYNCACTION::SYNC_SUB_FILE;
                                            out << m_FileNameList.at(i);

                                            QFile qfLocal(tr("%1%2") .arg(m_szSyncDir, m_FileNameList.at(i)));
                                            qfLocal.open(QIODevice::ReadOnly);

                                            out.writeBytes(qfLocal.readAll().data(), qfLocal.size());
                                            out << SYNCACTION::SYNC_ENDCODE;

                                            out.device()->seek(4);
                                            out << (quint64)outblock.size();
                                            pClient->write(outblock);
                                            pClient->flush();
                                        }
                                        else
                                        {
                                            // error, Md5 is different but modify date is the same
                                            displayLog(tr("error, Md5 is different but modify date is the same : file %1") .arg(m_FileNameList.at(i)));
                                        }
                                    }
                                    else
                                    {
                                        // Md5 is the same, do nothing
                                    }
                                }
                                else
                                {
                                    // Cannot find this file in remote, post to server
                                    displayLog(tr("Find no file on server, send it to server: \n\t%1") .arg(m_FileNameList.at(i)));

                                    QByteArray outblock;
                                    QDataStream out(&outblock, QIODevice::WriteOnly);

                                    out << SYNCACTION::SYNC_STARTCODE << (quint64)0 << (quint8)SYNCACTION::SYNC_POST << (quint8)SYNCACTION::SYNC_SUB_FILE;
                                    out << m_FileNameList.at(i);

                                    QFile qfLocal(tr("%1%2") .arg(m_szSyncDir, m_FileNameList.at(i)));
                                    qfLocal.open(QIODevice::ReadOnly);

                                    out.writeBytes(qfLocal.readAll().data(), qfLocal.size());
                                    out << SYNCACTION::SYNC_ENDCODE;

                                    out.device()->seek(4);
                                    out << (quint64)outblock.size();
                                    pClient->write(outblock);
                                    pClient->flush();
                                }
                            }

                            // --- Critical Section End ---
                            m_pFileListMutex->unlock();
                            break;
                        }
                    case SYNCACTION::SYNC_SUB_FILE:
                        {
                            QString szReadin;
                            in >> szReadin;

                            displayLog(tr("Receive from server: request file : %1") .arg(szReadin));

                            quint32 dataLength = 0;
                            in >> dataLength;
                            char *pData = (char*)malloc(sizeof(char)*dataLength);
                            int iRet = in.readRawData(pData, dataLength);

                            QString szFileName(tr("%1%2") .arg(m_szSyncDir) .arg(szReadin));
                            QString szTmp = szFileName.left(szFileName.lastIndexOf("/"));
                            QDir qdTmp(szTmp);
                            qdTmp.mkpath(szTmp);
                            QFile qfFile(szFileName);

                            qfFile.open(QIODevice::WriteOnly);
                            qfFile.write(pData, iRet);
                            qfFile.waitForBytesWritten(-1);
                            qfFile.close();
                            free(pData);
                            break;
                        }
                    default:
                        displayLog(tr("Received REPLY command, and there is No relative operation"));
                    }

                    break;
                }
            case SYNCACTION::SYNC_OPERATION:
                {
                    quint8 subAction = 0;
                    in >> subAction;

                    switch (subAction)
                    {
                    case SYNCACTION::SYNC_SUB_FILE_ADD:
                        {
                            break;
                        }
                    case SYNCACTION::SYNC_SUB_FILE_REMOVE:
                        {
                            QString szRemovedFileName;
                            in >> szRemovedFileName;

                            displayLog(tr("Receiving Remove file, name = \"%1\"") .arg(szRemovedFileName));
                            QFile qfRemovedFile(m_szSyncDir+szRemovedFileName);
                            bool bRet = qfRemovedFile.remove();
                            displayLog(tr("\tremove %1") .arg(bRet?"success":"failed"));
                            qfRemovedFile.close();
                            break;
                        }
                    case SYNCACTION::SYNC_SUB_FILE_RENAME:
                        {
                            QString szOrgFileName;
                            QString szRenFileName, szRenFileMd5, szRenFileModifyTime;

                            in >> szOrgFileName >> szRenFileName >> szRenFileMd5 >> szRenFileModifyTime;

                            displayLog(tr("Receiving Rename file \"%1\" to \"%2\"") .arg(szOrgFileName, szRenFileName));
                            QFile qfRenamedFile(m_szSyncDir+szOrgFileName);
                            bool bRet = qfRenamedFile.rename(m_szSyncDir+szRenFileName);
                            displayLog(tr("\trename %1") .arg(bRet?"success":"failed"));
                            qfRenamedFile.close();
                            break;
                        }
                    case SYNCACTION::SYNC_SUB_FILE_MODIFY:
                        {
                            QString szReadin;
                            in >> szReadin;

                            displayLog(tr("Receiving Modifiy file, name = \"%1\"") .arg(szReadin));

                            quint32 dataLength = 0;
                            in >> dataLength;

                            char *pData = (char*)malloc(sizeof(char)*dataLength);
                            int iRet = in.readRawData(pData, dataLength);

                            QString szFileName(tr("%1%2") .arg(m_szSyncDir) .arg(szReadin));
                            QString szTmp = szFileName.left(szFileName.lastIndexOf("/"));
                            QDir qdTmp(szTmp);
                            qdTmp.mkpath(szTmp);
                            QFile qfFile(szFileName);

                            qfFile.open(QIODevice::WriteOnly);
                            qfFile.write(pData, iRet);
                            qfFile.waitForBytesWritten(-1);
                            qfFile.close();
                            free(pData);

                            break;
                        }
                    default:
                        displayLog(tr("Received OPERATION command, and there is No relative operation"));
                    }

                    break;
                }
            default:
                displayLog(tr("Received main command, and there is No relative operation"));
            }
        }

        in >> endcode;
        if (endcode != SYNCACTION::SYNC_ENDCODE)
            displayLog(tr("Error in packet reply"));
    }
}

void CSyncexServer::slotOnTimer()
{
}

void CSyncexServer::displayLog(QString msg)
{
    if (!m_pTEditLog)
        return;

    m_pTEditLog->append(msg);
    qDebug() << msg;
    QScrollBar *bar = m_pTEditLog->verticalScrollBar();
    bar->setValue(bar->maximum());
    m_pTEditLog->adjustSize();
}

void CSyncexServer::slotEnableButtonRun()
{
    m_pButtonRun->setEnabled(!m_pEditServerPort->text().isEmpty()
                             && !m_pEditSyncDir->text().isEmpty()
                             && !m_pEditScanPeriod->text().isEmpty());
}
