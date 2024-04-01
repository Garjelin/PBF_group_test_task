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

class TCPServer {
private:
    int serverSocket;
    int port;
    std::ofstream logFile;

public:
    TCPServer(int _port) : port(_port) {}

    ~TCPServer() {
        close(serverSocket);
        logFile.close();
    }

    void start() {
        if (!initServer()) {
            std::cerr << "Failed to initialize server." << std::endl;
            return;
        }

        if (!startListening()) {
            std::cerr << "Failed to start listening." << std::endl;
            return;
        }

        acceptConnections();
    }

private:
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

            time_t now = time(0);
            tm* localTime = localtime(&now);
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "[%Y-%m-%d %H:%M:%S.000] ", localTime);

            std::string logEntry = std::string(timeStr) + std::string(buffer, bytesReceived) + "\n";
            std::cout << logEntry;
            // Записываем в файл в критической секции, чтобы избежать гонок данных
            pthread_mutex_t mutex;
            pthread_mutex_init(&mutex, NULL);
            pthread_mutex_lock(&mutex);
            logFile << logEntry;
            logFile.flush(); // Принудительно записываем данные в файл
            pthread_mutex_unlock(&mutex);
            pthread_mutex_destroy(&mutex);
        }

        close(clientSocket);
        pthread_exit(NULL);
    }

    void acceptConnections() {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        while (true) {
            int* clientSocket = new int(accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen));
            if (*clientSocket == -1) {
                std::cerr << "Failed to accept connection." << std::endl;
                continue;
            }

            pthread_t threadId;
            if (pthread_create(&threadId, NULL, &TCPServer::clientHandlerHelper, clientSocket) != 0) {
                std::cerr << "Failed to create client handler thread." << std::endl;
                close(*clientSocket);
                delete clientSocket;
                continue;
            }

            pthread_detach(threadId);
        }
    }

    static void* clientHandlerHelper(void* context) {
        int* clientSocket = static_cast<int*>(context);
        TCPServer server(*clientSocket);
        server.clientHandler(*clientSocket);
        delete clientSocket;
        return nullptr;
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
