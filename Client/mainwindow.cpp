#include "mainwindow.h"
#include "ui_mainwindow.h"

std::pmr::map<MessageType, QString> MessageTypeMap = {
    {Connection, "Connection"},
    {Disconnection, "Disconnection"},
    {Text, "Text"},
    {FileInfo, "FileInfo"},
    {FileData, "FileData"}
};

MainWindow::MainWindow(QTcpSocket *socket, QString clientName, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Chat Application");

    this->socket = socket;
    this->clientName = clientName;

    connect(ui->attachFileButton, &QPushButton::clicked, this, &MainWindow::on_action_attachFileButton_clicked);
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::on_action_sendButton_clicked);

    model = new QStandardItemModel();
    ui->clientList->setModel(model);
    addNewClientToUI(clientName);

    sendData(MessageType::Connection, clientName.toUtf8());
}

MainWindow::~MainWindow()
{
    qDebug() << "MainWindow destroyed";
    delete ui;
}

void MainWindow::addNewClientToUI(QString clientName)
{
    model->appendRow(new QStandardItem(clientName));
}

void MainWindow::deleteClientFromUI(QString clientName)
{
    for(int i = 0; i < model->rowCount(); i++)
    {
        if(model->item(i)->text() == clientName)
        {
            model->removeRow(i);
            break;
        }
    }
}

void MainWindow::sendData(MessageType type, QByteArray data)
{
    if(socket)
    {
        if(socket->isOpen())
        {
            QByteArray header;
            header.prepend(("Type:" + MessageTypeMap[type] + "\0").toUtf8());
            header.resize(HEADER_SIZE);

            data.prepend(header);

            QDataStream stream(socket);
            stream.setVersion(QDataStream::Qt_6_7);
            stream << data;
        }
        else
        {
            QMessageBox::critical(this, "Error", "Could not connect to server");
        }
    }
    else
    {
        QMessageBox::critical(this, "Error", "Could not connect to server");
    }
}

void MainWindow::on_action_attachFileButton_clicked()
{

}

void MainWindow::on_action_sendButton_clicked()
{
    QString message = ui->messageInputText->text();
    if(!message.isEmpty())
    {
        message.prepend(clientName + "> ");
        sendData(MessageType::Text, message.toUtf8());
        ui->chatDialogText->append(message);
        ui->messageInputText->clear();
    }
}

void MainWindow::readDataFromSocket()
{
    if(socket)
    {
        if(socket->isOpen())
        {
            QByteArray DataBuffer;

            QDataStream stream(socket);
            stream.setVersion(QDataStream::Qt_6_7);
            stream >> DataBuffer;

            QString header = DataBuffer.mid(0, HEADER_SIZE);
            QString type = header.split(":")[1].split('\0')[0];
            QString data = DataBuffer.mid(HEADER_SIZE);

            if(type == MessageTypeMap[Text])
            {
                ui->chatDialogText->append(data);
            }
            else if(type == MessageTypeMap[Connection])
            {
                addNewClientToUI(data);
            }
            else if(type == MessageTypeMap[Disconnection])
            {
                deleteClientFromUI(data);
            }
        }
        else
        {
            QMessageBox::critical(this, "Error", "Could not connect to server");
        }
    }
    else
    {
        QMessageBox::critical(this, "Error", "Could not connect to server");
    }
}
