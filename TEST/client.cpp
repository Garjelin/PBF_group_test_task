#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ctime>
#include <termios.h>
#include <fcntl.h>

class TCPClient {
private:
    int clientSocket;
    std::string clientName;
    int serverPort;
    int connectionPeriod;

public:
    TCPClient(const std::string& _clientName, int _serverPort, int _connectionPeriod)
        : clientName(_clientName), serverPort(_serverPort), connectionPeriod(_connectionPeriod) {}

    ~TCPClient() {
        close(clientSocket);
    }

    void start() {
        if (!connectToServer()) {
            std::cerr << "Failed to connect to server." << std::endl;
            return;
        }

        sendClientMessage("Starting the client " + clientName); // Отправляем сообщение на сервер

        while (true) {
            if (kbhit()) {
                char c = getchar();
                if (c == 'q') {
                    sendClientMessage("Shutting down the client " + clientName); // Отправляем сообщение на сервер
                    break;
                }
            }

            sendMessage();
            sleep(connectionPeriod);
        }
    }

private:
    bool connectToServer() {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            std::cerr << "Failed to create socket." << std::endl;
            return false;
        }

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);

        if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
            std::cerr << "Invalid address/Address not supported." << std::endl;
            return false;
        }

        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            std::cerr << "Connection failed." << std::endl;
            return false;
        }

        return true;
    }

    void sendMessage() {
        time_t now = time(0);
        tm* localTime = localtime(&now);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "[%Y-%m-%d %H:%M:%S.000]", localTime);

        std::string message = timeStr + std::string(" ") + clientName + "\n";
        send(clientSocket, message.c_str(), message.size(), 0);
    }

    void sendClientMessage(const std::string& message) {
        std::string formattedMessage = "[" + getCurrentTime() + "] " + message + "\n";
        std::cout << formattedMessage << std::endl; // Выводим сообщение в консоль клиента
        send(clientSocket, formattedMessage.c_str(), formattedMessage.size(), 0); // Отправляем сообщение на сервер
    }

    std::string getCurrentTime() {
        time_t now = time(0);
        tm* localTime = localtime(&now);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localTime);
        return std::string(timeStr);
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
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <clientName> <serverPort> <connectionPeriod>" << std::endl;
        return 1;
    }

    std::string clientName = argv[1];
    int serverPort = std::atoi(argv[2]);
    int connectionPeriod = std::atoi(argv[3]);

    if (serverPort <= 0 || serverPort > 65535 || connectionPeriod <= 0) {
        std::cerr << "Invalid arguments." << std::endl;
        return 1;
    }

    TCPClient client(clientName, serverPort, connectionPeriod);
    client.start();

    return 0;
}
