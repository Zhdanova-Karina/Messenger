#include <iostream>
#include <string>
#include <cstring>
#include <thread>      // Для создания потоков (многопоточность)
#include <vector>
#include <map>

// ========== КРОССПЛАТФОРМЕННАЯ ЧАСТЬ ==========
#ifdef _WIN32
    // Windows: Winsock2 библиотека для работы с сокетами
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSE_SOCKET(s) closesocket(s)
#else
    // Linux/UNIX: POSIX сокеты
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
typedef int socket_t;
#define INVALID_SOCKET -1
#define CLOSE_SOCKET(s) close(s)
#endif

// ========== ГЛОБАЛЬНЫЕ ДАННЫЕ ==========
std::vector<socket_t> clients;                    // Список всех подключённых клиентов (сокеты)
std::map<socket_t, std::string> socket_to_name;   // Отображение: сокет → имя пользователя
std::map<std::string, socket_t> name_to_socket;   // Отображение: имя пользователя → сокет

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========

/**
 * Отправляет сообщение конкретному клиенту
 * sock - сокет получателя
 * msg - текст сообщения
 */
void sendMsg(socket_t sock, const std::string& msg) {
    send(sock, msg.c_str(), (int)msg.length(), 0);
}

/**
 * Рассылает актуальный список пользователей ВСЕМ подключённым клиентам
 * Это нужно для автоматического обновления списка в интерфейсе
 */
void broadcastUserList() {
    if (clients.empty()) return;  // Нет клиентов — нечего отправлять

    // Формируем строку со списком пользователей
    std::string user_list = "Users: ";
    for (const auto& p : socket_to_name) {
        user_list += p.second + " ";
    }
    user_list += "\n";

    // Отправляем каждому клиенту
    for (socket_t client : clients) {
        sendMsg(client, user_list);
    }
    std::cout << "Broadcast user list to " << clients.size() << " clients: " << user_list;
}

// ========== ОБРАБОТКА ОДНОГО КЛИЕНТА (ВЫПОЛНЯЕТСЯ В ОТДЕЛЬНОМ ПОТОКЕ) ==========
/**
 * Функция обработки клиента.
 * ВЫПОЛНЯЕТСЯ В ОТДЕЛЬНОМ ПОТОКЕ ДЛЯ КАЖДОГО КЛИЕНТА!
 *
 * clientSock - сокет клиента, который нужно обслуживать
 */
void handleClient(socket_t clientSock) {
    char buf[1024];           // Буфер для приёма сообщений
    std::string name;         // Имя пользователя (после логина)
    bool logged = false;      // Флаг: авторизован ли пользователь

    // Бесконечный цикл обработки команд от клиента
    // Цикл прерывается при отключении клиента или команде exit
    while (true) {
        // Очищаем буфер и принимаем данные от клиента
        memset(buf, 0, sizeof(buf));
        int bytes = recv(clientSock, buf, sizeof(buf) - 1, 0);

        // Если получили 0 или отрицательное значение — клиент отключился
        if (bytes <= 0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        }

        // Преобразуем полученные данные в строку
        std::string cmd(buf);

        // Удаляем символы перевода строки (\n \r)
        if (!cmd.empty() && cmd.back() == '\n') cmd.pop_back();
        if (!cmd.empty() && cmd.back() == '\r') cmd.pop_back();

        std::cout << "Got: " << cmd << std::endl;

        // Пропускаем пустые команды
        if (cmd.empty()) {
            continue;
        }

        // ===== 1. АВТОРИЗАЦИЯ (ЛОГИН) =====
        else if (!logged && cmd.find("login ") == 0) {
            name = cmd.substr(6);   // Извлекаем имя после "login "

            // Проверяем, не занято ли имя
            if (name_to_socket.find(name) == name_to_socket.end()) {
                // Добавляем нового пользователя
                clients.push_back(clientSock);
                socket_to_name[clientSock] = name;
                name_to_socket[name] = clientSock;
                logged = true;

                // Отправляем подтверждение
                sendMsg(clientSock, "OK\n");
                std::cout << "User " << name << " logged in" << std::endl;

                // Уведомляем всех остальных клиентов об изменении списка
                broadcastUserList();
            }
            else {
                // Имя уже занято
                sendMsg(clientSock, "ERR Name taken\n");
            }
        }

        // ===== 2. ОТПРАВКА ЛИЧНОГО СООБЩЕНИЯ =====
        else if (logged && cmd.find("msg ") == 0) {
            std::string rest = cmd.substr(4);
            size_t space = rest.find(' ');

            if (space != std::string::npos) {
                std::string to = rest.substr(0, space);          // Получатель
                std::string text = rest.substr(space + 1);       // Текст сообщения

                // Ищем получателя в списке активных пользователей
                auto it = name_to_socket.find(to);
                if (it != name_to_socket.end()) {
                    // Отправляем сообщение получателю
                    std::string msg = "[" + name + "]: " + text + "\n";
                    sendMsg(it->second, msg);

                    // Подтверждаем отправителю
                    sendMsg(clientSock, "OK sent\n");
                    std::cout << "PM from " << name << " to " << to << std::endl;
                }
                else {
                    // Получатель не в сети
                    sendMsg(clientSock, "ERR user offline\n");
                }
            }
        }

        // ===== 3. ЗАПРОС СПИСКА ПОЛЬЗОВАТЕЛЕЙ =====
        else if (logged && cmd == "users") {
            std::string list = "Users: ";
            for (const auto& p : socket_to_name) {
                list += p.second + " ";
            }
            list += "\n";
            sendMsg(clientSock, list);
        }

        // ===== 4. ВЫХОД ИЗ СИСТЕМЫ =====
        else if (logged && cmd == "exit") {
            sendMsg(clientSock, "OK bye\n");
            break;  // Выходим из цикла, завершаем поток для этого клиента
        }

        // ===== 5. НЕИЗВЕСТНАЯ КОМАНДА =====
        else {
            sendMsg(clientSock, "ERR unknown\n");
        }
    }

    // ===== УДАЛЕНИЕ КЛИЕНТА ПРИ ОТКЛЮЧЕНИИ =====
    // Удаляем сокет из списка клиентов
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (*it == clientSock) {
            clients.erase(it);
            break;
        }
    }

    // Удаляем информацию о пользователе из словарей
    std::string name2 = socket_to_name[clientSock];
    socket_to_name.erase(clientSock);
    name_to_socket.erase(name2);

    // Закрываем сокет
    CLOSE_SOCKET(clientSock);
    std::cout << "User " << name2 << " left" << std::endl;

    // Уведомляем остальных клиентов об изменении списка
    broadcastUserList();
}

// ========== ТОЧКА ВХОДА (ГЛАВНЫЙ ПОТОК) ==========
int main() {
    // Инициализация сокетов (для Windows требуется WSAStartup)
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    // 1. Создание серверного сокета
    socket_t listenSock = socket(AF_INET, SOCK_STREAM, 0);

    // 2. Настройка адреса сервера
    sockaddr_in addr;
    addr.sin_family = AF_INET;           // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;   // Слушаем все сетевые интерфейсы
    addr.sin_port = htons(8888);         // Порт 8888

    // 3. Привязка сокета к адресу и порту
    bind(listenSock, (sockaddr*)&addr, sizeof(addr));

    // 4. Начинаем прослушивание входящих подключений
    listen(listenSock, 10);

    std::cout << "Server on port 8888" << std::endl;

    // ========== ГЛАВНЫЙ ЦИКЛ ПРИЁМА ПОДКЛЮЧЕНИЙ (МНОГОПОТОЧНОСТЬ) ==========
    while (true) {
        // Принимаем новое подключение
        socket_t clientSock = accept(listenSock, NULL, NULL);
        std::cout << "New client!" << std::endl;

        // === СОЗДАНИЕ НОВОГО ПОТОКА ДЛЯ КЛИЕНТА
        std::thread(handleClient, clientSock).detach();
            }

    // Закрываем серверный сокет (сюда никогда не доходит, так как цикл бесконечный)
    CLOSE_SOCKET(listenSock);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
