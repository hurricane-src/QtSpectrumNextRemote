#pragma once

#include <QObject>
#include <QTcpSocket>
#include "NEX.h"

class QtSpectrumNextRemoteTask: public QObject
{
    Q_OBJECT

public:
    QtSpectrumNextRemoteTask(QObject* parent, const QString& host, const QString& file, int flow_kbs=8192, int port=1023):
            QObject(parent),_host(host),_file(file),
            _flow_bytes_sent(0),_flow_epoch(0),
            _banks{0},_flow_kbs(flow_kbs),_port(port) {}

public slots:
    void run();
signals:
    void finished();

private:
    void sendNEX(NEX& nex);
    int send(const void* data, size_t size);
    int recv(void* data, size_t size);
    const QString _host;
    const QString _file;
    QTcpSocket _socket;
    int64_t _flow_bytes_sent;
    int64_t _flow_epoch;
    uint8_t _banks[8];
    int _flow_kbs;
    int _port;
};
