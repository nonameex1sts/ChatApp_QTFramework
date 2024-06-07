#include "chatUI.h"
#include "ui_chat.h"

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
    this->clientName = clientName;

    // Create a new thread to manage the TCP connection
    this->tcpManager = new TCPManagerThread(socket);

    // Connect the signals from the TCPManagerThread to the functions in this class
    connect(tcpManager, &TCPManagerThread::newMessageReceived, this, &Chat::addDialogToUI);
    connect(tcpManager, &TCPManagerThread::newClientConnected, this, &Chat::addNewClientToUI);
    connect(tcpManager, &TCPManagerThread::clientDisconnected, this, &Chat::deleteClientFromUI);
    connect(tcpManager, &TCPManagerThread::newFileReceived, this, &Chat::addNewSharedFileToUI);

    // Start the TCP manager thread
    this->tcpManager->start();

    // Connect the buttons and list widgets to their respective functions
    connect(ui->attachFileButton, &QPushButton::clicked, this, &Chat::on_action_attachFileButton_clicked);
    connect(ui->sendButton, &QPushButton::clicked, this, &Chat::on_action_sendButton_clicked);
    connect(ui->attachedFileList, &QListWidget::itemDoubleClicked, this, &Chat::removeAttachFile);
    connect(ui->sharedFileList, &QListWidget::itemDoubleClicked, this, &Chat::downloadFile);

    // Create a new client list model and set it to the client list widget
    clientListModel = new QStandardItemModel();
    ui->clientList->setModel(clientListModel);

    // Send a connection message to the server
    this->tcpManager->sendMessage(MessageType::Connection, (clientName + '\n').toUtf8());
}

Chat::~Chat()
{
    tcpManager->quit();
    delete tcpManager;
    delete ui;
}

// Add a dialog line to the chat dialog text widget
void Chat::addDialogToUI(MessageType type, QString message)
{
    switch (type)
    {
        case MessageType::Connection:
        {
            ui->chatDialogText->setTextColor(Qt::gray);
            break;
        }
        case MessageType::Disconnection:
        {
            ui->chatDialogText->setTextColor(Qt::gray);
            break;
        }
        case MessageType::FileInfo:
        {
            ui->chatDialogText->setTextColor(Qt::cyan);
            break;
        }
        case MessageType::FileData:
        {
            ui->chatDialogText->setTextColor(Qt::green);
            break;
        }
        default:
        {
            ui->chatDialogText->setTextColor(Qt::white);
            break;
        }
    }

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

// When the send button is clicked, send the message and attached files to the server
void Chat::on_action_sendButton_clicked()
{
    // Send the message to the server if it is not empty
    QString message = ui->messageInputText->text();
    if(!message.isEmpty())
    {
        message.prepend(clientName + "> ");
        tcpManager->sendMessage(MessageType::Text, message.toUtf8());
        addDialogToUI(MessageType::Text, message);
        ui->messageInputText->clear();
    }

    // Send the attached files to the server
    tcpManager->readFiles(filePathList);

    // Clear the list of attached files
    filePathList.clear();
    ui->attachedFileList->clear();
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
    tcpManager->requestFile(fileName);
}
