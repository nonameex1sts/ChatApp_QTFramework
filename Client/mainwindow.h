#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#define HEADER_SIZE 64

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

enum MessageType
{
    Connection,
    Disconnection,
    Text,
    FileInfo,
    FileData
};

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    MainWindow(QTcpSocket *socket, QString clientName, QWidget *parent = nullptr);
    ~MainWindow();

private:
    void addNewClientToUI(QString clientName);
    void deleteClientFromUI(QString clientName);
    void sendData(MessageType type, QByteArray data);

private slots:
    void on_action_attachFileButton_clicked();
    void on_action_sendButton_clicked();
    void readDataFromSocket();

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;
    QString clientName;
    QStandardItemModel *model;
};
#endif // MAINWINDOW_H
