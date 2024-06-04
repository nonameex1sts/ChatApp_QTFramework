#include "server.h"
#include "header.h"

Server::Server() {
    server = new QTcpServer();

    // Clear the file directory when the server starts
    QDir dir(FILE_DIR);
    if(!dir.exists())
    {
        dir.mkpath(".");
    }
    else
    {
        foreach(QString fileName, dir.entryList(QDir::Files))
        {
            dir.remove(fileName);
        }
    }

    // Start the server
    if(!server->listen(QHostAddress::LocalHost, 1234))
    {
        qDebug() << "Could not start server";
    }
    else
    {
        connect(server, SIGNAL(newConnection()), this, SLOT(newConnection()));
        qDebug() << "Server started";
    }
}

Server::~Server() {
    qDebug() << "Server destroyed";
}

void Server::newConnection() {
    while(server->hasPendingConnections())
    {
        QTcpSocket *client = server->nextPendingConnection();
        addNewClients(client);
    }
}

// When a client disconnects
void Server::clientDisconnected() {
    QTcpSocket *client = reinterpret_cast<QTcpSocket *>(sender());

    // Remove the client from the list of clients
    clients.removeAll(client);

    // Send a disconnection message to all clients
    QString clientName = clientNames[client];
    Header header(MessageType::Disconnection, clientName.size(), 1, 1);
    Packet packet(header, clientName);

    foreach (QTcpSocket* socket, clients) {
        QDataStream stream(socket);
        stream.setVersion(QDataStream::Qt_6_7);
        stream << packet.toByteArray();
    }

    // Remove the client from the list of client names
    clientNames.remove(client);

    qDebug() << "Client disconnected at port " << client->peerPort() << " with address " << client->peerAddress().toString();
}

// Read data from the client
void Server::readDataFromClient() {
    QTcpSocket *client = reinterpret_cast<QTcpSocket *>(sender());

    QByteArray DataBuffer;

    QDataStream stream(client);
    stream.setVersion(QDataStream::Qt_6_7);

    // Read the data from the socket until the start byte is found
    while(client->read(1)[0] != START_BYTE)
    {
    }

    bool firstPacketFlag = true;

    // Read the data from the socket until there is no more data to read
    while(client->bytesAvailable() >= HEADER_SIZE + DATA_SIZE + TAIL_SIZE - firstPacketFlag)
    {
        // If this is the first packet, add the start byte to the buffer
        if(firstPacketFlag)
        {
            DataBuffer = client->read(HEADER_SIZE + DATA_SIZE + TAIL_SIZE - 1);
            DataBuffer.prepend(START_BYTE);
            firstPacketFlag = false;
        }
        else
        {
            DataBuffer = client->read(HEADER_SIZE + DATA_SIZE + TAIL_SIZE);
        }

        // Parse the data buffer and handle the data
        Packet packet(DataBuffer);
        Header header = packet.getHeader();
        QByteArray data = packet.getData();

        switch (header.type){
            case MessageType::Text:
            {
                sendPacketToAllOtherClients(client, packet);
                break;
            }
            case MessageType::Connection:
            {
                sendPacketToAllOtherClients(client, packet);

                // Add the client to the list of clients
                clientNames[client] = data.split('\n')[0];

                // Send the list of current clients to the new client
                QString clientList;
                foreach(QString clientName, clientNames.values())
                {
                    clientList.append(clientName + '\n');
                }
                Header feedbackHeader(MessageType::Connection, clientList.size(), 1, 1);
                Packet feedbackPacket(feedbackHeader, clientList);
                stream << feedbackPacket.toByteArray();

                break;
            }
            case MessageType::Disconnection:
            {
                clientDisconnected();
                break;
            }
            case MessageType::FileData:
            {
                QFile file(FILE_DIR + header.fileName);

                // Append data to file
                if(file.open(QIODevice::Append))
                {
                    for(int i = 0; i < header.dataSize; i++)
                    {
                        file.putChar(data[i]);
                    }
                    file.close();
                }
                else
                {
                    qDebug() << "Could not open file";
                }

                // Send file info to all clients
                if(header.no == header.totalPacket)
                {
                    QString senderName = clientNames[client];
                    Header fileInfoHeader(MessageType::FileInfo, header.fileName, senderName.size(), 1, 1);
                    Packet fileInfoPacket(fileInfoHeader, senderName);

                    foreach (QTcpSocket* forwardClient, clients) {
                        QDataStream forwardStream(forwardClient);
                        forwardStream.setVersion(QDataStream::Qt_6_7);
                        forwardStream << fileInfoPacket.toByteArray();
                    }
                }

                break;
            }
            case MessageType::FileInfo:
            {
                sendFile(client, header.fileName);
                break;
            }
            default:
            {
                qDebug() << "Unknown message type";
                break;
            }
        }
    }
}

// Add new clients to the server and connect signals
void Server::addNewClients(QTcpSocket *client) {
    clients.append(client);
    connect(client, &QTcpSocket::readyRead, this, &Server::readDataFromClient);
    connect(client, &QTcpSocket::disconnected, this, &Server::clientDisconnected);
    qDebug() << "Client connected at port " << client->peerPort() << " with address " << client->peerAddress().toString();
}

// Send file to the client who requested it
void Server::sendFile(QTcpSocket *client, QString fileName) {
    if(client->isOpen())
    {
        QByteArray fileData;

        // Open the file and read the data
        QFile file(FILE_DIR + fileName);
        if(file.open(QIODevice::ReadOnly))
        {
            fileData = file.readAll();
            file.close();
        }

        QDataStream stream(client);
        stream.setVersion(QDataStream::Qt_6_7);

        // Calculate the number of packets needed to send the file
        int numOfPackets = fileData.length() / DATA_SIZE + 1;

        // Create each packet and send to the client
        for(int i = 0; i <= numOfPackets; i++)
        {
            QByteArray rawData = fileData.mid(i * DATA_SIZE, DATA_SIZE);
            Header header(MessageType::FileData, fileName, rawData.size(), numOfPackets, i + 1);
            Packet packet(header, rawData);

            stream << packet.toByteArray();
        }
    }
    else
    {
        qDebug() << "Client is not connected";
    }
}

// Send a packet to all clients except the one that sent the packet
void Server::sendPacketToAllOtherClients(QTcpSocket *currentClient, Packet packet) {
    foreach (QTcpSocket* forwardClient, clients) {
        if(forwardClient != currentClient)
        {
            QDataStream forwardStream(forwardClient);
            forwardStream.setVersion(QDataStream::Qt_6_7);
            forwardStream << packet.toByteArray();
        }
    }
}
