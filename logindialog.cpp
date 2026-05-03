#include "logindialog.h"
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Вход в мессенджер");
    setFixedSize(300, 200);

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("IP адрес сервера:"));
    ipEdit = new QLineEdit("127.0.0.1");
    layout->addWidget(ipEdit);

    layout->addWidget(new QLabel("Ваше имя:"));
    nameEdit = new QLineEdit();
    layout->addWidget(nameEdit);

    connectBtn = new QPushButton("Подключиться");
    layout->addWidget(connectBtn);

    connect(connectBtn, &QPushButton::clicked, this, &LoginDialog::onConnectClicked);
}

void LoginDialog::onConnectClicked()
{
    if (ipEdit->text().isEmpty() || nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Заполните все поля!");
        return;
    }
    accept();
}

QString LoginDialog::getServerIp() const
{
    return ipEdit->text();
}

QString LoginDialog::getUsername() const
{
    return nameEdit->text();
}