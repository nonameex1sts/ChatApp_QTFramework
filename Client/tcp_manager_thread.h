#ifndef TCP_MANAGER_THREAD_H
#define TCP_MANAGER_THREAD_H

#include <QThread>
#include <QTcpSocket>
#include <QMutex>

#include "header.h"
#include "packet.h"

#define PACKET_BUFFER_SIZE 50000

namespace Network {
class TCPManagerThread;
}

class TCPManagerThread : public QThread
{
    Q_OBJECT

public:
    TCPManagerThread(QTcpSocket *socket);
    ~TCPManagerThread();
    void sendMessage(MessageType type, QByteArray message);
    void readFiles(QStringList filePath);
    void requestFile(QString fileName);

signals:
    void newMessageReceived(MessageType type, QString message);
    void newClientConnected(QString clientName);
    void clientDisconnected(QString clientName);
    void newFileReceived(QString fileName);
    void fileProgress(int progress);
    void connectionError();

private slots:
    void readDataFromSocket();
    void sendFileDataPacket();

private:
    QTcpSocket *socket;
    QTimer *timer;
    Packet *fileDataPackets;
    mutable QMutex mutex;
    int endFileDataPacketIndex;
    int currentFileDataPacketIndex;
};

#endif // TCP_MANAGER_THREAD_H
