#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <vector>
#include <map>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSE_SOCKET(s) closesocket(s)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET -1
#define CLOSE_SOCKET(s) close(s)
#endif

std::vector<socket_t> clients;
std::map<socket_t, std::string> socket_to_name;
std::map<std::string, socket_t> name_to_socket;

// Отправка сообщения клиенту
void sendMsg(socket_t sock, const std::string& msg) {
    send(sock, msg.c_str(), (int)msg.length(), 0);
}

// Рассылка всем
void broadcast(const std::string& msg, socket_t exclude = INVALID_SOCKET) {
    for (socket_t s : clients) {
        if (s != exclude) {
            sendMsg(s, msg);
        }
    }
}

// Обработка одного клиента
void handleClient(socket_t clientSock) {
    char buf[1024];
    std::string name;
    bool logged = false;

    while (true) {
        memset(buf, 0, sizeof(buf));
        int bytes = recv(clientSock, buf, sizeof(buf) - 1, 0);

        if (bytes <= 0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        }

        std::string cmd(buf);
        // Убираем перевод строки
        if (!cmd.empty() && cmd.back() == '\n') cmd.pop_back();
        if (!cmd.empty() && cmd.back() == '\r') cmd.pop_back();

        std::cout << "Got: " << cmd << std::endl;
        // Если команда пустая — просто игнорируем
        if (cmd.empty()) {
            continue;
        }
        // --- ЛОГИН ---
        else if (!logged && cmd.find("login ") == 0) {
            name = cmd.substr(6);

            // Проверяем, не занято ли имя
            if (name_to_socket.find(name) == name_to_socket.end()) {
                clients.push_back(clientSock);
                socket_to_name[clientSock] = name;
                name_to_socket[name] = clientSock;
                logged = true;

                sendMsg(clientSock, "OK\n");
                std::cout << "User " << name << " logged in" << std::endl;

                // Сообщаем всем о новом пользователе
                broadcast("ONLINE: " + name + "\n", clientSock);
            }
            else {
                sendMsg(clientSock, "ERR Name taken\n");
            }
        }

        // --- ОТПРАВКА СООБЩЕНИЯ ---
        else if (logged && cmd.find("msg ") == 0) {
            std::string rest = cmd.substr(4);
            size_t space = rest.find(' ');
            if (space != std::string::npos) {
                std::string to = rest.substr(0, space);
                std::string text = rest.substr(space + 1);

                auto it = name_to_socket.find(to);
                if (it != name_to_socket.end()) {
                    std::string msg = "[" + name + "]: " + text + "\n";
                    sendMsg(it->second, msg);
                    sendMsg(clientSock, "OK sent\n");
                    std::cout << "PM from " << name << " to " << to << std::endl;
                }
                else {
                    sendMsg(clientSock, "ERR user offline\n");
                }
            }
        }

        // --- СПИСОК ПОЛЬЗОВАТЕЛЕЙ ---
        else if (logged && cmd == "users") {
            std::string list = "Users: ";
            for (const auto& p : socket_to_name) {
                list += p.second + " ";
            }
            list += "\n";
            sendMsg(clientSock, list);
        }

        // --- ВЫХОД ---
        else if (logged && cmd == "exit") {
            sendMsg(clientSock, "OK bye\n");
            break;
        }

        // --- НЕИЗВЕСТНАЯ КОМАНДА ---
        else {
            sendMsg(clientSock, "ERR unknown\n");
        }
    }

    // Удаляем клиента
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (*it == clientSock) {
            clients.erase(it);
            break;
        }
    }

    std::string name2 = socket_to_name[clientSock];
    socket_to_name.erase(clientSock);
    name_to_socket.erase(name2);

    CLOSE_SOCKET(clientSock);
    std::cout << "User " << name2 << " left" << std::endl;
}

int main() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    socket_t listenSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8888);

    bind(listenSock, (sockaddr*)&addr, sizeof(addr));
    listen(listenSock, 10);

    std::cout << "Server on port 8888" << std::endl;

    while (true) {
        socket_t clientSock = accept(listenSock, NULL, NULL);
        std::cout << "New client!" << std::endl;
        std::thread(handleClient, clientSock).detach();
    }

    CLOSE_SOCKET(listenSock);
#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}