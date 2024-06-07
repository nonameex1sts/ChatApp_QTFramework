#include "loginUI.h"
#include "ui_loginUI.h"
#include "chatUI.h"

Login::Login(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);
    this->setWindowTitle("Login");

    // Connect the "connect" button to the login function
    connect(ui->connectButton, &QPushButton::clicked, this, &Login::on_action_loginButton_clicked);
}

Login::~Login()
{
    delete ui;
}

void Login::on_action_loginButton_clicked()
{
    QString username = ui->usernameText->text();

    // If username is empty, show an error message
    if(username.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Username cannot be empty");
    }
    else
    {
        // Create a new socket and connect to the server
        QTcpSocket *socket = new QTcpSocket();
        socket->connectToHost(QHostAddress::LocalHost, 1234);
        socket->open(QIODevice::ReadWrite);

        // If connection is successful, create a new chat window
        if(socket->waitForConnected(3000))
        {
            Chat *chatWindow = new Chat(socket, username);
            chatWindow->show();

            // Close the login window
            this->close();
        }
        // If connection is not successful, show an error message
        else
        {
            QMessageBox::critical(this, "Error", "Could not connect to server");
        }
    }
}
