#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <QtNetwork>
#include <QtWidgets>

namespace Ui {
class Login;
}

class Login : public QWidget
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();

private slots:
    void on_action_loginButton_clicked();

private:
    Ui::Login *ui;
};

#endif // LOGIN_H
