#include "chat.h"
#include "ui_chat.h"
#include "packet.h"

Chat::Chat(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Chat)
{
    ui->setupUi(this);
}

Chat::Chat(QTcpSocket *socket, QString clientName, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Chat)
{
    ui->setupUi(this);
    this->setWindowTitle("Chat Application");

    this->socket = socket;
    this->clientName = clientName;

    // Connect the buttons and list widgets to their respective functions
    connect(ui->attachFileButton, &QPushButton::clicked, this, &Chat::on_action_attachFileButton_clicked);
    connect(ui->sendButton, &QPushButton::clicked, this, &Chat::on_action_sendButton_clicked);
    connect(ui->attachedFileList, &QListWidget::itemDoubleClicked, this, &Chat::removeAttachFile);
    connect(ui->sharedFileList, &QListWidget::itemDoubleClicked, this, &Chat::downloadFile);

    // Connect the socket to the readDataFromSocket function
    connect(socket, &QTcpSocket::readyRead, this, &Chat::readDataFromSocket);

    // Create a new client list model and set it to the client list widget
    clientListModel = new QStandardItemModel();
    ui->clientList->setModel(clientListModel);

    // Send a connection message to the server
    sendMessage(MessageType::Connection, (clientName + '\n').toUtf8());
}

Chat::~Chat()
{
    delete ui;
}

// Add a dialog line to the chat dialog text widget
void Chat::addDialogToUI(QString message, QColor color)
{
    ui->chatDialogText->setTextColor(color);
    ui->chatDialogText->append(message);
}

// Add a new client to the client list widget
void Chat::addNewClientToUI(QString clientName)
{
    clientListModel->appendRow(new QStandardItem(clientName));
}

// Delete a client from the client list widget
void Chat::deleteClientFromUI(QString clientName)
{
    for(int i = 0; i < clientListModel->rowCount(); i++)
    {
        if(clientListModel->item(i)->text() == clientName)
        {
            clientListModel->removeRow(i);
            break;
        }
    }
}

// Send a message to the server
void Chat::sendMessage(MessageType type, QByteArray message)
{
    if(socket)
    {
        if(socket->isOpen())
        {
            // Create a new packet with the message and send it to the server using the socket
            Header header(type, message.size(), 1, 1);
            Packet packet(header, message);

            QDataStream stream(socket);
            stream.setVersion(QDataStream::Qt_6_7);

            stream << packet.toByteArray();
        }
        else
        {
            QMessageBox::critical(this, "Error", "Could not connect to server");
        }
    }
}

// Send a file to the server
void Chat::sendFile(QString filePath)
{
    if(socket)
    {
        if(socket->isOpen())
        {
            QByteArray fileData;

            // Open the file and read the data
            QFile file(filePath);
            if(file.open(QIODevice::ReadOnly))
            {
                fileData = file.readAll();
                file.close();
            }
            else
            {
                QMessageBox::critical(this, "Error", "Could not open file " + filePath);
                return;
            }

            QDataStream stream(socket);
            stream.setVersion(QDataStream::Qt_6_7);

            // Calculate the number of packets needed to send the file
            int numOfPackets = fileData.length() / DATA_SIZE + 1;

            // Create each packet and send to the server
            for(int i = 0; i <= numOfPackets; i++)
            {
                QByteArray rawData = fileData.mid(i * DATA_SIZE, DATA_SIZE);
                Header header(MessageType::FileData, filePath.split('/').constLast(), rawData.size(), numOfPackets, i + 1);
                Packet packet(header, rawData);

                stream << packet.toByteArray();
            }
        }
        else
        {
            QMessageBox::critical(this, "Error", "Could not connect to server");
        }
    }
}

// Request a file from the server
void Chat::requestFile(QString fileName)
{
    if(socket)
    {
        if(socket->isOpen())
        {
            // Create a new packet with the file name and send it to the server using the socket
            Header header(MessageType::FileInfo, fileName, 0, 1, 1);
            Packet packet(header, QByteArray());

            QDataStream stream(socket);
            stream.setVersion(QDataStream::Qt_6_7);

            stream << packet.toByteArray();
        }
        else
        {
            QMessageBox::critical(this, "Error", "Could not connect to server");
        }
    }
}

// Add a new shared file's name to the shared file list widget
void Chat::addNewSharedFileToUI(QString fileName)
{
    QListWidgetItem *item = new QListWidgetItem(fileName);
    ui->sharedFileList->addItem(item);
}

// When the attach file button is clicked, open the "browse file" window
void Chat::on_action_attachFileButton_clicked()
{
    // Get list of file paths from the user
    QStringList filePaths = QFileDialog::getOpenFileNames(this, "Attach File", QDir::homePath());

    if(!filePaths.isEmpty())
    {
        foreach(QString filePath, filePaths)
        {
            // Add the file path to the list and add the file name to the attached file list widget
            filePathList.append(filePath);

            QListWidgetItem *item = new QListWidgetItem(filePath.split('/').constLast());
            ui->attachedFileList->addItem(item);
        }
    }
}

// When the file in the attached file list widget is double clicked, remove it from the list
void Chat::removeAttachFile(QListWidgetItem* item)
{
    int index = ui->attachedFileList->row(item);
    filePathList.removeAt(index);
    delete item;
}

// When the file in the shared file list widget is double clicked, request to download the file from the server
void Chat::downloadFile(QListWidgetItem* item)
{
    QString fileName = item->text();
    requestFile(fileName);
}

// When the send button is clicked, send the message and attached files to the server
void Chat::on_action_sendButton_clicked()
{
    // Send the message to the server if it is not empty
    QString message = ui->messageInputText->text();
    if(!message.isEmpty())
    {
        message.prepend(clientName + "> ");
        sendMessage(MessageType::Text, message.toUtf8());
        addDialogToUI(message, Qt::white);
        ui->messageInputText->clear();
    }

    // Send the attached files to the server
    foreach(QString filePath, filePathList)
    {
        sendFile(filePath);
    }

    // Clear the list of attached files
    filePathList.clear();
    ui->attachedFileList->clear();
}

void Chat::readDataFromSocket()
{
    if(socket)
    {
        if(socket->isOpen())
        {
            QByteArray DataBuffer;

            QDataStream stream(socket);
            stream.setVersion(QDataStream::Qt_6_7);

            // Read the data from the socket until the start byte is found
            while(socket->read(1)[0] != START_BYTE)
            {
            }

            bool firstPacketFlag = true;

            // Read the data from the socket until there is no more data to read
            while(socket->bytesAvailable() >= HEADER_SIZE + DATA_SIZE + TAIL_SIZE - firstPacketFlag)
            {
                // If this is the first packet, add the start byte to the buffer
                if(firstPacketFlag)
                {
                    DataBuffer = socket->read(HEADER_SIZE + DATA_SIZE + TAIL_SIZE - 1);
                    DataBuffer.prepend(START_BYTE);
                    firstPacketFlag = false;
                }
                else
                {
                    DataBuffer = socket->read(HEADER_SIZE + DATA_SIZE + TAIL_SIZE);
                }

                // Parse the data buffer and handle the data
                Packet packet(DataBuffer);
                Header header = packet.getHeader();
                QByteArray data = packet.getData();

                // Handle the data based on the message type
                switch (header.type) {
                    case MessageType::Text:
                    {
                        // Add the message to the chat dialog widget
                        addDialogToUI(data, Qt::white);
                        break;
                    }
                    case MessageType::Connection:
                    {
                        // Parse the list of clients
                        QByteArrayList clientList = data.split('\n');
                        clientList.pop_back();

                        // Add them to the client list widget and add messages to the chat dialog widget
                        foreach(QByteArray client, clientList)
                        {
                            addDialogToUI(client + " has joined the chat", Qt::gray);
                            addNewClientToUI(client);
                        }
                        break;
                    }
                    case MessageType::Disconnection:
                    {
                        // Remove the client from the client list widget and add a message to the chat dialog widget
                        addDialogToUI(data + " has left the chat", Qt::gray);
                        deleteClientFromUI(data);
                        break;
                    }
                    case MessageType::FileInfo:
                    {
                        // Add the file to the shared file list widget and add a message to the chat dialog widget
                        addDialogToUI(data + " has shared " + header.fileName, Qt::cyan);
                        addNewSharedFileToUI(header.fileName);
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
                        else
                        {
                            qDebug() << "Could not open file";
                        }

                        break;
                    }
                    default:
                        qDebug() << "Unknown message type " << header.type;
                        break;
                }
            }
        }
        else
        {
            QMessageBox::critical(this, "Error", "Could not connect to server");
        }
    }
}
