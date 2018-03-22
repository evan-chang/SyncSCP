/*
 * Syncex - For cross platform exchanger syncronization your data on multi-platform PC
 *
 * Support WinXP/Vista/7, Mac OS X Leopard/Snow Leopard, Ubunto
 * Author : Evan Chang
 *
 * Reserved Copyrightc Evan Chang
 */

#pragma once
#ifndef SYNCEXSERVER_H
#define SYNCEXSERVER_H

#include <QWidget>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtGui>

#include "syncaction.h"

class CSyncexServer : public QDialog
{
    Q_OBJECT
public:
    explicit CSyncexServer(QWidget *parent = 0);
    ~CSyncexServer();

private:
    QTcpServer *m_pSyncexServer;
    QList<QTcpSocket*> m_SocketList;

    quint16 m_u16Port;
    QString m_szSyncDir;
    QStringList m_FileNameList;
    QStringList m_FileMd5List;
    QStringList m_FileModifyTimeList;
    QStringList m_orgFileNameList;
    QStringList m_orgFileMd5List;
    QStringList m_orgFileModifyTimeList;
    QByteArray m_qbBuffer;
    QTimer *m_pTimer;
    QMutex *m_pFileListMutex;
    int m_SyncProgressValue;

    // widget
    QLabel *m_pLabelPort;
    QLineEdit *m_pEditServerPort;
    QLabel *m_pLabelSyncDir;
    QLineEdit *m_pEditSyncDir;
    QPushButton *m_pButtonSyncDir;
    QPushButton *m_pButtonRun;
    QPushButton *m_pButtonStop;
    QLabel *m_pLabelScanPeriod;
    QLineEdit *m_pEditScanPeriod;
    QTextEdit *m_pTEditLog;

    bool hashSyncFiles();
    void displayLog(QString msg);

signals:

public slots:
    void slotOpenSyncDirectory();
    void slotCreateServer();
    void slotStopServer();
    void slotConnections();
    void slotDisconnections();
    void slotDataReceived();
    void slotOnTimer();
    void slotEnableButtonRun();
};

#endif // SYNCEXSERVER_H
