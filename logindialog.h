#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

class QLineEdit;
class QPushButton;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    LoginDialog(QWidget *parent = nullptr);
    QString getServerIp() const;
    QString getUsername() const;

private slots:
    void onConnectClicked();

private:
    QLineEdit *ipEdit;
    QLineEdit *nameEdit;
    QPushButton *connectBtn;
};

#endif