#include "server.h"
#include "header.h"

Server::Server() {
    this->server = new QTcpServer();

    // Initialize the file data packets buffer
    this->fileDataPackets = new Packet[PACKET_BUFFER_SIZE];

    // Initialize the two pointers for the file data packets buffer
    this->endFileDataPacketIndex = 0;
    this->currentFileDataPacketIndex = 0;

    // Create a timer to send the file data packets to the clients
    this->timer = new QTimer(this);
    timer->setInterval(5);
    connect(timer, &QTimer::timeout, this, &Server::sendFileDataPacket);
    timer->start();

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

// Add new clients to the server and connect signals
void Server::addNewClients(QTcpSocket *client) {
    clients.append(client);
    connect(client, &QTcpSocket::readyRead, this, &Server::readDataFromClient);
    connect(client, &QTcpSocket::disconnected, this, &Server::clientDisconnected);
    qDebug() << "Client connected at port " << client->peerPort() << " with address " << client->peerAddress().toString();
}

// Send file to the client who requested it
void Server::readFile(QString fileName) {
    QByteArray fileData;

    // Open the file and read the data
    QFile file(FILE_DIR + fileName);
    if(file.open(QIODevice::ReadOnly))
    {
        fileData = file.readAll();
        file.close();
    }

    // Calculate the number of packets needed to send the file
    int numOfPackets = fileData.length() / DATA_SIZE + 1;

    // Create each packet push it to the file data packets buffer
    for(int i = 0; i < numOfPackets; i++)
    {
        QByteArray rawData = fileData.mid(i * DATA_SIZE, DATA_SIZE);
        Header header(MessageType::FileData, fileName, rawData.size(), numOfPackets, i + 1);
        Packet packet(header, rawData);

        // Increment the end file data packet index after inserting the packet
        fileDataPackets[endFileDataPacketIndex] = packet;
        endFileDataPacketIndex = (endFileDataPacketIndex + 1) % PACKET_BUFFER_SIZE;
    }
}

// Send a packet to all clients
void Server::sendPacketToAllClients(Packet packet) {
    foreach (QTcpSocket* forwardClient, clients) {
        // Lock the mutex
        mutex.lock();

        QDataStream forwardStream(forwardClient);
        forwardStream.setVersion(QDataStream::Qt_6_7);
        forwardStream << packet.toByteArray();

        // Unlock the mutex
        mutex.unlock();
    }
}

// Send a packet to all clients except the one that sent the packet
void Server::sendPacketToAllOtherClients(QTcpSocket *currentClient, Packet packet) {
    foreach (QTcpSocket* forwardClient, clients) {
        if(forwardClient != currentClient)
        {
            // Lock the mutex
            mutex.lock();

            QDataStream forwardStream(forwardClient);
            forwardStream.setVersion(QDataStream::Qt_6_7);
            forwardStream << packet.toByteArray();

            // Unlock the mutex
            mutex.unlock();
        }
    }
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

    // Send a disconnection message to all remaining clients
    QString clientName = clientNames[client];
    Header header(MessageType::Disconnection, clientName.size(), 1, 1);
    Packet packet(header, clientName);

    sendPacketToAllClients(packet);

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

    // Read the data from the socket until there is no more data to read
    while(client->bytesAvailable() >= HEADER_SIZE + DATA_SIZE + TAIL_SIZE)
    {
        // Read the data from the socket until the start byte is found
        while(client->read(1)[0] != START_BYTE)
        {
        }

        // Read the data from the socket and prepend the start byte
        DataBuffer = client->read(HEADER_SIZE + DATA_SIZE + TAIL_SIZE - 1);
        DataBuffer.prepend(START_BYTE);

        // Parse the data buffer and handle the data
        Packet packet(DataBuffer);
        Header header = packet.header;
        QByteArray data = packet.data;

        switch (header.type){
            case MessageType::Text:
            {
                // Forward the message to all other clients
                sendPacketToAllOtherClients(client, packet);
                break;
            }
            case MessageType::Connection:
            {
                // Forward the connection message to all other clients
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
                // Handle the disconnection
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

                // Send file info to all clients if all packets have been received
                if(header.no == header.totalPacket)
                {
                    QString senderName = clientNames[client];
                    Header fileInfoHeader(MessageType::FileInfo, header.fileName, senderName.size(), 1, 1);
                    Packet fileInfoPacket(fileInfoHeader, senderName);

                    sendPacketToAllClients(fileInfoPacket);
                }

                break;
            }
            case MessageType::FileInfo:
            {
                // Add the client to the file request queue
                fileRequestQueue.push(client);

                readFile(header.fileName);
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

// Send a file to the server
void Server::sendFileDataPacket()
{
    QTcpSocket *client = fileRequestQueue.front();
    if(client && client->isOpen())
    {
        if(currentFileDataPacketIndex != endFileDataPacketIndex)
        {
            // Lock the mutex
            mutex.lock();

            // Get the paclet
            Packet packet = fileDataPackets[currentFileDataPacketIndex];

            // Send the packet to the server using the socket
            QDataStream stream(client);
            stream.setVersion(QDataStream::Qt_6_7);

            // Increment the current file data packet index
            stream << packet.toByteArray();
            currentFileDataPacketIndex = (currentFileDataPacketIndex + 1) % PACKET_BUFFER_SIZE;

            // If the last packet was sent, remove the first-in-line client from the queue
            if(packet.header.no == packet.header.totalPacket)
            {
                fileRequestQueue.pop();
            }

            // Unlock the mutex
            mutex.unlock();
        }
    }
}
