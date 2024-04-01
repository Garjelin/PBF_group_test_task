#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <fstream>
#include <chrono>

class TCPClient {
private:
    std::string clientName;
    int port;
    int period;

public:
    TCPClient(std::string name, int p, int per) : clientName(name), port(p), period(per) {}

    void run() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(period));
            sendMessage();
        }
    }

    void sendMessage() {
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            std::cerr << "Error: Could not create socket\n";
            return;
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Error: Connection failed\n";
            close(clientSocket);
            return;
        }

        char message[1024];
        snprintf(message, sizeof(message), "[%s] \"%s\"\n", getCurrentTime().c_str(), clientName.c_str());
        send(clientSocket, message, strlen(message), 0);

        close(clientSocket);
    }

    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        char buffer[100];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_c));
        return std::string(buffer);
    }
};

class TCPServer {
private:
    int port;

public:
    TCPServer(int p) : port(p) {}

    void run() {
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            std::cerr << "Error: Could not create socket\n";
            return;
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Error: Binding failed\n";
            close(serverSocket);
            return;
        }

        if (listen(serverSocket, 5) < 0) {
            std::cerr << "Error: Listening failed\n";
            close(serverSocket);
            return;
        }

        std::ofstream logFile("log.txt", std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Error: Unable to open log file\n";
            close(serverSocket);
            return;
        }

        while (true) {
            int clientSocket = accept(serverSocket, nullptr, nullptr);
            if (clientSocket < 0) {
                std::cerr << "Error: Accepting connection failed\n";
                continue;
            }

            std::thread clientThread(&TCPServer::handleClient, this, clientSocket, std::ref(logFile));
            clientThread.detach(); // Detach the thread to run independently
        }
    }

    void handleClient(int clientSocket, std::ofstream& logFile) {
        char buffer[1024];
        int valread = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (valread > 0) {
            buffer[valread] = '\0';
            logFile << buffer;
        }

        close(clientSocket);
    }
};