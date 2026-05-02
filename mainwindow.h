#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QMap>

class QListWidget;
class QTextEdit;
class QLineEdit;
class QPushButton;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const QString& serverIp, const QString& username, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnected();
    void onError();
    void onReadyRead();
    void onUserSelected();
    void updateUserList(const QString& users);

private:
    void sendCommand(const QString& cmd);
    void appendMessage(const QString& chatId, const QString& from, const QString& text);
    void switchToChat(const QString& contact);
    void addChatTab(const QString& contact);
    void sendMessageForContact(const QString& contact);
    void createEmojiPanel(const QString& contact);

    QTcpSocket *socket;
    QString username;
    QString currentContact;
    QString lastUsersList;

    QListWidget *userList;
    QStackedWidget *chatStack;

    struct ChatWidgets {
        QTextEdit *chatArea;
        QLineEdit *inputField;
        QPushButton *sendButton;
        QPushButton *emojiButton;
        QWidget *emojiPanel;
    };
    QMap<QString, ChatWidgets> chats;
    QMap<QString, int> unreadCounts;
};

#endif
