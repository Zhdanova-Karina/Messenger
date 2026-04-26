#include <QApplication>
#include "logindialog.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    LoginDialog login;
    if (login.exec() == QDialog::Accepted) {
        MainWindow w(login.getServerIp(), login.getUsername());
        w.show();
        return app.exec();
    }

    return 0;
}
