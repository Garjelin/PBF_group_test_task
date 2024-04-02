#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fstream>
#include <ctime>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <future>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

class TCPServer {
private:
    int serverSocket;
    int port;
    std::queue<std::string> logQueue;
    std::mutex logMutex;
    std::ofstream logFile;
    std::atomic<bool> running;
    std::atomic<bool> logThreadRunning;
    std::atomic<bool> stopServerFlag;

    bool initServer() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            std::cerr << "Failed to create socket." << std::endl;
            return false;
        }

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            std::cerr << "Failed to bind socket." << std::endl;
            return false;
        }

        logFile.open("log.txt", std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Failed to open log file." << std::endl;
            return false;
        }

        return true;
    }

    bool startListening() {
        if (listen(serverSocket, 10) == -1) {
            std::cerr << "Failed to start listening." << std::endl;
            return false;
        }
        return true;
    }

    void clientHandler(int clientSocket) {
        char buffer[1024];

        while (true) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0)
                break;

            buffer[bytesReceived] = '\0'; // Добавляем завершающий нуль-символ

            // Запись в лог
            logMutex.lock();
            logFile << buffer;
            std::cout << buffer; // Для вывода в консоль сервера
            logFile.flush();
            logMutex.unlock();
        }

        close(clientSocket);
    }

    void acceptConnections() {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        while (!stopServerFlag) {
            // Установка тайм-аута для accept с помощью select
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(serverSocket, &readfds);
            struct timeval timeout;
            timeout.tv_sec = 0;  // Установка тайм-аута на 0 секунд
            timeout.tv_usec = 10000;  // Установка тайм-аута на 10 миллисекунд

            int ready = select(serverSocket + 1, &readfds, nullptr, nullptr, &timeout);
            if (ready == -1) {
                std::cerr << "Error in select" << std::endl;
                break;
            } else if (ready > 0) {
                int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
                if (clientSocket == -1) {
                    //std::cerr << "Failed to accept connection." << std::endl;
                    continue;
                }

                std::thread clientThread(&TCPServer::clientHandler, this, clientSocket);
                clientThread.detach();
            }
        }
    }


    void logWorker() {
        while (running  && logThreadRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (!logQueue.empty()) {
                logMutex.lock();
                while (!logQueue.empty()) {
                    std::string logEntry = logQueue.front();
                    std::cout << logEntry;  // Вывод в консоль
                    logFile << logEntry;    // Запись в файл
                    logQueue.pop();
                }
                logFile.flush();
                logMutex.unlock();
            }
        }
    }

    void inputHandler() {
        while (running) {
            if (kbhit()) {
                char c = getchar(); // Неблокирующее чтение символа с консоли
                if (c == 'q') {
                    //std::cout << "Received 'q', stopping..." << std::endl; 
                    stop();
                    return;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    int kbhit() {
        struct termios oldt, newt;
        int ch;
        int oldf;

        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

        ch = getchar();

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, oldf);

        if (ch != EOF) {
            ungetc(ch, stdin);
            return 1;
        }

        return 0;
    }

    public:
    TCPServer(int _port) : port(_port), running(true), logThreadRunning(true), stopServerFlag(false) {}

    ~TCPServer() {
        stop();
    }

    void start() {
        if (!initServer()) {
            return;
        }

        // Добавляем запись "Starting the server" с текущим временем
        logMutex.lock();
        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char timeStr[64];
        std::strftime(timeStr, sizeof(timeStr), "[%Y-%m-%d %H:%M:%S] ", std::localtime(&now));
        logFile << timeStr << "Starting the server" << std::endl;
        std::cout << timeStr << "Starting the server" << std::endl;
        logMutex.unlock();

        if (!startListening()) {
            std::cerr << "Failed to start listening." << std::endl;
            return;
        }
        std::thread loggerThread(&TCPServer::logWorker, this);
        std::thread inputThread(&TCPServer::inputHandler, this);
        acceptConnections();
        loggerThread.join();
        inputThread.join();
    }

    void stop() {
        if (!stopServerFlag) {
            // Добавляем запись "Shutting down the server" с текущим временем
            logMutex.lock();
            std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            char timeStr[64];
            std::strftime(timeStr, sizeof(timeStr), "[%Y-%m-%d %H:%M:%S] ", std::localtime(&now));
            logFile << timeStr << "Shutting down the server" << std::endl;
            std::cout << timeStr << "Shutting down the server" << std::endl;
            logMutex.unlock();
        }

        stopServerFlag = true;

        running = false;
        close(serverSocket);
        logThreadRunning = false;
        if (logFile.is_open()) {
            logFile.close();
        }
    }

};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port number." << std::endl;
        return 1;
    }

    TCPServer server(port);
    server.start();

    return 0;
}


