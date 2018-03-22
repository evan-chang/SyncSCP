/*
 * Syncex - For cross platform exchanger syncronization your data on multi-platform PC
 *
 * Support WinXP/Vista/7, Mac OS X Leopard/Snow Leopard, Ubunto
 * Author : Evan Chang
 *
 * Reserved Copyrightc Evan Chang
 */

#pragma once
#ifndef SYNCEXCLIENT_H
#define SYNCEXCLIENT_H

#include <QtGui>
#include <QWidget>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>

#include "syncaction.h"

class CSyncexClient : public QDialog
{
    Q_OBJECT
public:
    explicit CSyncexClient(QWidget *parent = 0);
    ~CSyncexClient();

private:
    QTcpSocket *m_pSyncexClient;
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

    // widget
    QLabel *m_pLabelHost;
    QLineEdit *m_pEditConnectTo;
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
    void sendMd5ListRequest();

signals:

public slots:
    void slotOpenSyncDirectory();
    void slotConnectTo();
    void slotDisconnectTo();
    void slotDisconnections();
    void slotDataReceived();
    void slotOnTimer();
    void slotEnableButtonRun();
};

#endif // SYNCEXCLIENT_H
