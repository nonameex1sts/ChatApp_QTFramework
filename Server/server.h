#ifndef SERVER_H
#define SERVER_H

#include <QtCore>
#include <QtNetwork>
#include <QtWidgets>
#include <QDir>
#include <queue>

#include "packet.h"

#define FILE_DIR "files/"
#define PACKET_BUFFER_SIZE 50000

class Server : public QObject
{
    Q_OBJECT

private:
    void addNewClients(QTcpSocket *client);
    void readFile(QString fileName);
    void sendPacketToAllClients(Packet packet);
    void sendPacketToAllOtherClients(QTcpSocket *currentClient, Packet packet);

private slots:
    void newConnection();
    void clientDisconnected();
    void readDataFromClient();
    void sendFileDataPacket();

public:
    Server();
    ~Server();

private:
    QTcpServer *server;
    QList<QTcpSocket *> clients;
    QMap<QTcpSocket*, QString> clientNames;
    QTimer *timer;
    Packet *fileDataPackets;
    std::queue<QTcpSocket*> fileRequestQueue;
    QMutex mutex;
    int endFileDataPacketIndex;
    int currentFileDataPacketIndex;
};

#endif // SERVER_H
