#ifndef SERVER_H
#define SERVER_H

#include <QtCore>
#include <QtNetwork>
#include <QtWidgets>
#include <QDir>

#include "packet.h"

#define FILE_DIR "files/"

class Server : public QObject
{
    Q_OBJECT

private:
    QTcpServer *server;
    QList<QTcpSocket *> clients;
    QMap<QTcpSocket*, QString> clientNames;

private slots:
    void newConnection();
    void clientDisconnected();
    void readDataFromClient();

private:
    void addNewClients(QTcpSocket *client);
    void sendFile(QTcpSocket *client, QString fileName);
    void sendPacketToAllOtherClients(QTcpSocket *currentClient, Packet packet);

public:
    Server();
    ~Server();
};

#endif // SERVER_H
