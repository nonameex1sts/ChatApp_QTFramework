#ifndef CHAT_H
#define CHAT_H

#include <QMainWindow>
#include <QtNetwork>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include "tcp_manager_thread.h"

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

private slots:
    void addDialogToUI(MessageType type, QString message);
    void addNewClientToUI(QString clientName);
    void deleteClientFromUI(QString clientName);
    void addNewSharedFileToUI(QString fileName);
    void on_action_attachFileButton_clicked();
    void on_action_sendButton_clicked();
    void removeAttachFile(QListWidgetItem* item);
    void downloadFile(QListWidgetItem* item);


private:
    Ui::Chat *ui;
    TCPManagerThread *tcpManager;
    QString clientName;
    QStandardItemModel *clientListModel;
    QList<QString> filePathList;
};

#endif // CHAT_H
