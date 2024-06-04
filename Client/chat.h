#ifndef CHAT_H
#define CHAT_H

#include <QMainWindow>
#include <QtNetwork>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include "header.h"

namespace Ui {
class Chat;
}

class Chat : public QWidget
{
    Q_OBJECT

public:
    explicit Chat(QWidget *parent = nullptr);
    Chat(QTcpSocket *socket, QString clientName, QWidget *parent = nullptr);
    ~Chat();

private:
    void addDialogToUI(QString message, QColor color);
    void addNewClientToUI(QString clientName);
    void deleteClientFromUI(QString clientName);
    void sendMessage(MessageType type, QByteArray message);
    void sendFile(QString filePath);
    void requestFile(QString fileName);
    void addNewSharedFileToUI(QString fileName);

private slots:
    void on_action_attachFileButton_clicked();
    void on_action_sendButton_clicked();
    void removeAttachFile(QListWidgetItem* item);
    void downloadFile(QListWidgetItem* item);
    void readDataFromSocket();

private:
    Ui::Chat *ui;
    QTcpSocket *socket;
    QString clientName;
    QStandardItemModel *clientListModel;
    QList<QString> filePathList;
};

#endif // CHAT_H
