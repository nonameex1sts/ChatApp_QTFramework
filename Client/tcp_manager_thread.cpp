#include "tcp_manager_thread.h"

TCPManagerThread::TCPManagerThread(QTcpSocket *socket)
{
    this->socket = socket;

    // Initialize the file data packets buffer
    this->fileDataPackets = new Packet[PACKET_BUFFER_SIZE];

    // Initialize the two pointers for the file data packets buffer
    this->endFileDataPacketIndex = 0;
    this->currentFileDataPacketIndex = 0;

    // Connect the socket to the readDataFromSocket function
    connect(socket, &QTcpSocket::readyRead, this, &TCPManagerThread::readDataFromSocket);

    // Create a timer to send the file data packets to the server
    this->timer = new QTimer(this);
    timer->setInterval(10);
    connect(timer, &QTimer::timeout, this, &TCPManagerThread::sendFileDataPacket);
    timer->start();
}

TCPManagerThread::~TCPManagerThread()
{
    if(socket && socket->isOpen())
    {
        socket->close();
    }

    timer->stop();

    delete timer;
    delete socket;
    delete fileDataPackets;
}

// Send a message to the server
void TCPManagerThread::sendMessage(MessageType type, QByteArray message)
{
    if(socket && socket->isOpen())
    {
        // Create a new packet with the message and send it to the server using the socket
        Header header(type, message.size(), 1, 1);
        Packet packet(header, message);

        // Lock the mutex
        mutex.lock();

        // Send the packet to the server using the socket
        QDataStream stream(socket);
        stream.setVersion(QDataStream::Qt_6_7);

        stream << packet.toByteArray();

        // Unlock the mutex
        mutex.unlock();
    }
}

// Send a file to the server
void TCPManagerThread::sendFileDataPacket()
{
    if(socket && socket->isOpen())
    {
        // Check if there are packets to send
        if(currentFileDataPacketIndex != endFileDataPacketIndex)
        {
            // Lock the mutex
            mutex.lock();

            // Send the packet to the server using the socket
            QDataStream stream(socket);
            stream.setVersion(QDataStream::Qt_6_7);

            // Increment the current file data packet index
            stream << fileDataPackets[currentFileDataPacketIndex].toByteArray();
            currentFileDataPacketIndex = (currentFileDataPacketIndex + 1) % PACKET_BUFFER_SIZE;

            // Unlock the mutex
            mutex.unlock();
        }
    }
}

// Request a file from the server
void TCPManagerThread::requestFile(QString fileName)
{
    if(socket && socket->isOpen())
    {
        // Create a new packet with the file name and send it to the server using the socket
        Header header(MessageType::FileInfo, fileName, 0, 1, 1);
        Packet packet(header, QByteArray());

        // Lock the mutex
        mutex.lock();

        QDataStream stream(socket);
        stream.setVersion(QDataStream::Qt_6_7);

        stream << packet.toByteArray();

        // Unlock the mutex
        mutex.unlock();
    }
}

// Read the files, create the packets and push them to the file data packets buffer
void TCPManagerThread::readFiles(QStringList filePaths)
{
    foreach(QString filePath, filePaths)
    {
        QByteArray fileData;

        // Open the file and read the data
        QFile file(filePath);
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
            Header header(MessageType::FileData, filePath.split('/').constLast(), rawData.size(), numOfPackets, i + 1);
            Packet packet(header, rawData);

            // Increment the end file data packet index after inserting the packet
            fileDataPackets[endFileDataPacketIndex] = packet;
            endFileDataPacketIndex = (endFileDataPacketIndex + 1) % PACKET_BUFFER_SIZE;
        }
    }
}

void TCPManagerThread::readDataFromSocket()
{
    if(socket && socket->isOpen())
    {
        QByteArray DataBuffer;

        QDataStream stream(socket);
        stream.setVersion(QDataStream::Qt_6_7);

        // Read the data from the socket until there is no more data to read
        while(socket->bytesAvailable() >= HEADER_SIZE + DATA_SIZE + TAIL_SIZE)
        {
            // Read the data from the socket until the start byte is found
            while(socket->read(1)[0] != START_BYTE)
            {
            }

            // Read the data from the socket and prepend the start byte
            DataBuffer = socket->read(HEADER_SIZE + DATA_SIZE + TAIL_SIZE - 1);
            DataBuffer.prepend(START_BYTE);

            // Parse the data buffer and handle the data
            Packet packet(DataBuffer);
            Header header = packet.getHeader();
            QByteArray data = packet.getData();

            // Handle the data based on the message type
            switch (header.type) {
            case MessageType::Text:
            {
                // Emmit signal to add the message to the chat dialog widget
                emit newMessageReceived(header.type, data);
                break;
            }
            case MessageType::Connection:
            {
                // Parse the list of clients
                QByteArrayList clientList = data.split('\n');
                clientList.pop_back();

                // Emmit signal to add them to the client list widget and add messages to the chat dialog widget
                foreach(QByteArray client, clientList)
                {
                    emit newMessageReceived(header.type, client + " has joined the chat");
                    emit newClientConnected(client);
                }
                break;
            }
            case MessageType::Disconnection:
            {
                // Emmit signal to remove the client from the client list widget and add a message to the chat dialog widget
                emit newMessageReceived(header.type, data + " has left the chat");
                emit clientDisconnected(data);
                break;
            }
            case MessageType::FileInfo:
            {
                // Emmit signal to add the file to the shared file list widget and add a message to the chat dialog widget
                emit newMessageReceived(header.type, data + " has shared " + header.fileName);
                emit newFileReceived(header.fileName);
                break;
            }
            case MessageType::FileData:
            {
                // Create new file in the download folder
                QFile file(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/" + header.fileName);

                // Append data to file
                if(file.open(QIODevice::Append))
                {
                    for(int i = 0; i < header.dataSize; i++)
                    {
                        file.putChar(data[i]);
                    }
                    file.close();
                }

                // If the last packet has been received, emmit a signal to add a message to the chat dialog widget
                if(header.no == header.totalPacket){
                    emit newMessageReceived(header.type, header.fileName + " has been downloaded");
                }

                break;
            }
            default:
                qDebug() << "Unknown message type " << header.type;
                break;
            }
        }
    }
}
