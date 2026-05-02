#include "mainwindow.h"
#include <QTcpSocket>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QTime>
#include <QScrollBar>
#include <QFont>
#include <QGridLayout>
#include <QDate>

MainWindow::MainWindow(const QString& serverIp, const QString& name, QWidget *parent)
    : QMainWindow(parent), username(name)
{
    setWindowTitle("Мессенджер - " + name);
    resize(800, 550);

    // ========== ЛЕВАЯ ПАНЕЛЬ: список чатов ==========
    userList = new QListWidget(this);
    userList->setMaximumWidth(200);

    QFont font = userList->font();
    font.setBold(true);
    font.setPointSize(10);
    userList->setFont(font);

    userList->setStyleSheet(
        "QListWidget { outline: none; border: none; background-color: white; }"
        "QListWidget::item { padding: 8px; border-bottom: 1px solid #FFA500; color: #000000; }"
        "QListWidget::item:selected { background-color: #FFD700; color: #000000; }"
        "QListWidget::item:selected:hover { background-color: #FFD700; }"
        "QListWidget::item:!selected:hover { background-color: #FFFFE0; }"
        "QListWidget::item:focus { outline: none; }"
        );

    // ========== ПРАВАЯ ПАНЕЛЬ: стек чатов ==========
    chatStack = new QStackedWidget(this);

    // Страница-заглушка "Выберите чат"
    QWidget *emptyPage = new QWidget(this);
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyPage);
    QLabel *emptyLabel = new QLabel("💬 Выберите чат", this);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet(
        "QLabel { font-size: 20px; color: #000000; font-weight: bold; background-color: #FFFFE0; }"
        );
    emptyLayout->addWidget(emptyLabel);
    emptyPage->setLayout(emptyLayout);
    chatStack->addWidget(emptyPage);

    // Размещение
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(userList);
    splitter->addWidget(chatStack);
    setCentralWidget(splitter);

    // Подключаем сигналы
    connect(userList, &QListWidget::itemClicked, this, &MainWindow::onUserSelected);

    // Подключаемся к серверу
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &MainWindow::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onError);

    appendMessage("system", "Система", "Подключение к " + serverIp + ":8888...");
    socket->connectToHost(serverIp, 8888);
}

MainWindow::~MainWindow()
{
    if (socket && socket->state() == QTcpSocket::ConnectedState) {
        sendCommand("exit");
        socket->flush();
    }
}

void MainWindow::addChatTab(const QString& contact)
{
    if (chats.contains(contact)) return;

    QWidget *chatPage = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(chatPage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Область сообщений
    QTextEdit *chatArea = new QTextEdit(this);
    chatArea->setReadOnly(true);
    chatArea->setStyleSheet("QTextEdit { background-color: #FFFFE0; border: none; }");

    // Панель со смайликами
    QWidget *emojiPanel = new QWidget(this);
    emojiPanel->setVisible(false);
    emojiPanel->setStyleSheet(
        "QWidget { background-color: #FFFFFF; border: 1px solid #ccc; border-radius: 8px; }"
        );

    QGridLayout *emojiLayout = new QGridLayout(emojiPanel);
    emojiLayout->setSpacing(5);
    emojiLayout->setContentsMargins(10, 10, 10, 10);

    // Список смайликов
    QStringList emojis = {
        "😊", "😂", "🥰", "😍", "🎉", "❤️", "👍", "🔥",
        "😎", "🤔", "😢", "🥺", "😘", "😁", "🙈", "💀"
    };

    for (int i = 0; i < emojis.size(); ++i) {
        QPushButton *btn = new QPushButton(emojis[i]);
        btn->setFixedSize(45, 45);
        btn->setStyleSheet(
            "QPushButton { font-size: 24px; border: none; border-radius: 8px; }"
            "QPushButton:hover { background-color: #f0f0f0; }"
            );
        connect(btn, &QPushButton::clicked, this, [this, contact, emojis, i]() {
            if (chats.contains(contact)) {
                chats[contact].inputField->insert(emojis[i]);
                chats[contact].inputField->setFocus();
            }
        });
        emojiLayout->addWidget(btn, i / 8, i % 8);
    }

    // Поле ввода
    QLineEdit *inputField = new QLineEdit(this);
    inputField->setPlaceholderText("Сообщение...");
    inputField->setMinimumHeight(40);
    inputField->setStyleSheet(
        "QLineEdit { padding-left: 15px; padding-right: 10px; font-size: 14px; "
        "border: 1px solid #ccc; border-radius: 8px; }"
        "QLineEdit:focus { border: 1px solid #FFD700; }"
        );

    // Кнопка смайликов
    QPushButton *emojiBtn = new QPushButton("😊", this);
    emojiBtn->setFixedSize(40, 40);
    emojiBtn->setStyleSheet(
        "QPushButton { font-size: 20px; background-color: #FFD700; border-radius: 8px; }"
        "QPushButton:hover { background-color: #FFA500; }"
        );

    // Кнопка отправки
    QPushButton *sendBtn = new QPushButton("➤", this);
    sendBtn->setFixedSize(40, 40);
    sendBtn->setStyleSheet(
        "QPushButton { font-size: 24px; background-color: #FFD700; color: white; "
        "border: none; border-radius: 8px; }"
        "QPushButton:hover { background-color: #FFA500; }"
        "QPushButton:pressed { background-color: #FFA500; }"
        );

    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(inputField);
    inputLayout->addWidget(emojiBtn);
    inputLayout->addWidget(sendBtn);
    inputLayout->setContentsMargins(10, 10, 10, 10);
    inputLayout->setSpacing(8);

    layout->addWidget(chatArea);
    layout->addWidget(emojiPanel);
    layout->addLayout(inputLayout);

    chatPage->setLayout(layout);
    chatStack->addWidget(chatPage);

    // Сохраняем виджеты
    ChatWidgets widgets;
    widgets.chatArea = chatArea;
    widgets.inputField = inputField;
    widgets.sendButton = sendBtn;
    widgets.emojiButton = emojiBtn;
    widgets.emojiPanel = emojiPanel;
    widgets.lastMessageDate = "";  // инициализируем пустой строкой
    chats[contact] = widgets;

    connect(sendBtn, &QPushButton::clicked, this, [this, contact]() {
        sendMessageForContact(contact);
    });
    connect(inputField, &QLineEdit::returnPressed, this, [this, contact]() {
        sendMessageForContact(contact);
    });

    connect(emojiBtn, &QPushButton::clicked, this, [this, contact]() {
        if (chats.contains(contact)) {
            bool visible = chats[contact].emojiPanel->isVisible();
            chats[contact].emojiPanel->setVisible(!visible);
        }
    });
}

void MainWindow::sendMessageForContact(const QString& contact)
{
    if (!chats.contains(contact)) return;

    QString text = chats[contact].inputField->text().trimmed();
    if (text.isEmpty()) return;

    sendCommand("msg " + contact + " " + text);
    appendMessage(contact, username, text);
    chats[contact].inputField->clear();
}

void MainWindow::switchToChat(const QString& contact)
{
    if (chats.contains(currentContact)) {
        chats[currentContact].emojiPanel->setVisible(false);
    }

    bool isNewChat = !chats.contains(contact);

    if (isNewChat) {
        addChatTab(contact);
    }

    if (unreadCounts.contains(contact) && unreadCounts[contact] > 0) {
        unreadCounts[contact] = 0;
        if (!lastUsersList.isEmpty()) {
            updateUserList(lastUsersList);
        } else {
            sendCommand("users");
        }
    }

    currentContact = contact;
    QWidget *targetPage = chats[contact].chatArea->parentWidget();
    chatStack->setCurrentWidget(targetPage);
}

void MainWindow::appendMessage(const QString& chatId, const QString& from, const QString& text)
{
    if (!chats.contains(chatId) && chatId != "system") {
        addChatTab(chatId);
    }

    // Добавляем разделитель даты, если нужно (только для не-системных чатов)
    if (chatId != "system") {
        checkAndAddDateSeparator(chatId, QDate::currentDate());
    }

    QTime currentTime = QTime::currentTime();
    QString timestamp = currentTime.toString("hh:mm");

    QString formatted;
    if (from == username) {
        formatted = QString(
                        "<table width='100%' style='margin-bottom: 2px 0;'>"
                        "<tr>"
                        "<td align='left' style='color:#000000;'>"
                        "<b>%1</b> [%2]: %3"
                        "</td>"
                        "<tr>"
                        "</table>"
                        ).arg(from, timestamp, text.toHtmlEscaped());
    } else if (from == "Система") {
        formatted = QString(
                        "<table width='100%' style='margin-bottom: 2px 0;'>"
                        "<tr>"
                        "<td align='left' style='color:#888888; font-style:italic;'>"
                        "%1"
                        "</td>"
                        "</tr>"
                        "</tr>"
                        ).arg(text);
    } else {
        formatted = QString(
                        "<table width='100%' style='margin-bottom: 2px 0;'>"
                        "<tr>"
                        "<td align='left'>"
                        "<b>%1</b> [%2]: %3"
                        "</td>"
                        "</tr>"
                        "</table>"
                        ).arg(from, timestamp, text.toHtmlEscaped());
    }

    if (chatId == "system") {
        if (chats.contains(currentContact)) {
            chats[currentContact].chatArea->append(formatted);
            QTextEdit *area = chats[currentContact].chatArea;
            area->verticalScrollBar()->setValue(area->verticalScrollBar()->maximum());
        }
    } else if (chats.contains(chatId)) {
        chats[chatId].chatArea->append(formatted);
        QTextEdit *area = chats[chatId].chatArea;
        area->verticalScrollBar()->setValue(area->verticalScrollBar()->maximum());
    }
}

void MainWindow::checkAndAddDateSeparator(const QString& chatId, const QDate& messageDate)
{
    if (!chats.contains(chatId)) return;

    QString formattedDate;
    QDate currentDate = QDate::currentDate();
    QDate yesterday = currentDate.addDays(-1);

    if (messageDate == currentDate) {
        formattedDate = "Сегодня";
    } else if (messageDate == yesterday) {
        formattedDate = "Вчера";
    } else {
        formattedDate = messageDate.toString("d MMMM yyyy");
    }

    QString dateString = messageDate.toString("yyyy-MM-dd");

    // Используем date из структуры чата
    if (chats[chatId].lastMessageDate != dateString) {
        QString separator = QString(
                                "<div style='text-align: center;'>"
                                "<span style='background-color: #E0E0E0; padding: 4px 12px; border-radius: 16px; "
                                "font-size: 12px; color: #666;'>%1</span>"
                                "</div>"
                                ).arg(formattedDate);

        chats[chatId].chatArea->append(separator);
        chats[chatId].lastMessageDate = dateString;
    }
}

void MainWindow::updateUserList(const QString& users)
{
    lastUsersList = users;
    userList->clear();

    if (users.isEmpty()) return;

    QStringList userList_ = users.split(' ', Qt::SkipEmptyParts);

    for (const QString& user : userList_) {
        if (user == username) continue;

        QString displayName = user;
        if (unreadCounts.contains(user) && unreadCounts[user] > 0) {
            displayName = QString("%1 🟠").arg(user);
        }
        userList->addItem(displayName);
    }
}

void MainWindow::onConnected()
{
    appendMessage("system", "Система", "Подключено! Авторизация...");
    sendCommand("login " + username);
}

void MainWindow::onError()
{
    appendMessage("system", "Ошибка", socket->errorString());
}

void MainWindow::onReadyRead()
{
    QString data = socket->readAll();
    QStringList lines = data.split('\n');

    for (QString line : lines) {
        if (line.isEmpty()) continue;

        if (line.startsWith("OK")) {
            // успех
        }
        else if (line.startsWith("Users:") || line.startsWith("ONLINE:")) {
            QString users = line;
            users.remove("Users:");
            users.remove("ONLINE:");
            users = users.trimmed();
            updateUserList(users);
        }
        else if (line.startsWith("[")) {
            int endOfName = line.indexOf("]:");
            if (endOfName > 0) {
                QString from = line.mid(1, endOfName - 1);
                QString text = line.mid(endOfName + 3);

                if (from != username) {
                    unreadCounts[from] = unreadCounts.value(from, 0) + 1;
                    updateUserList(lastUsersList);
                }
                appendMessage(from, from, text);
            }
        }
        else if (line.startsWith("ERR")) {
            appendMessage("system", "Ошибка", line.mid(4));
        }
        else {
            appendMessage("system", "Система", line);
        }
    }
}

void MainWindow::onUserSelected()
{
    if (userList->currentItem()) {
        QString selected = userList->currentItem()->text();
        QString contact = selected.split(" 🟠").first();
        contact = contact.trimmed();
        switchToChat(contact);
    }
}

void MainWindow::sendCommand(const QString& cmd)
{
    if (socket && socket->state() == QTcpSocket::ConnectedState) {
        socket->write((cmd + "\n").toUtf8());
    }
}
