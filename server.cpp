#include <iostream>
#include <netinet/in.h>
#include <fstream>
#include <queue>
#include <thread>
#include <future>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

class TCPServer {
    int serverSocket;
    int port;
    std::queue<std::string> logQueue;
    std::mutex logMutex;
    std::ofstream logFile;
    std::atomic<bool> running;

    bool initServer();
    bool startListening();
    void clientHandler(int clientSocket);
    void acceptConnections();
    void logWorker();
    void inputHandler();
    int kbhit();
    void log(const std::string& message);

public:
    TCPServer(int _port) : port(_port), running(false) {}
    ~TCPServer() {stop();}
    void start();
    void stop();
};

bool TCPServer::initServer() {
    // Создание сокета сервера: 
    // AF_INET - семейство протоколов IPv4; 
    // SOCK_STREAM - TCP-соединение;
    // 0 - выбор протокола автоматически.
    serverSocket = socket(AF_INET, SOCK_STREAM, 0); 
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return false;
    }

    // Cтруктура для представления сетевого адреса
    // AF_INET - семейство протоколов IPv4
    // htonl(INADDR_ANY) для преобразования 32-битного целого числа из формата хоста в сетевой формат (big-endian)
    // htons(port) для преобразования 16-битного целого числа из формата хоста в сетевой формат (big-endian)
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    // привязка сокета к адресу и порту
    // bind принимает указатель на общий тип sockaddr, поэтому здесь используется приведение типов
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Failed to bind socket." << std::endl;
        return false;
    }

    // Открытие лога "log.txt" для записи. 
    // std::ios::out - файл будет открыт для записи 
    // std::ios::app - новые записи будут добавляться в конец файла
    logFile.open("log.txt", std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file." << std::endl;
        return false;
    }

    return true;
}

    // Запуск прослушивания сокета на входящие соединения
    // 10 - максимальное количество соединений в очереди до того, как они будут приняты
bool TCPServer::startListening() {
    if (listen(serverSocket, 10) == -1) {
        std::cerr << "Failed to start listening." << std::endl;
        return false;
    }
    return true;
}

void TCPServer::clientHandler(int clientSocket) {
    char buffer[1024];

    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0)
            break;

        buffer[bytesReceived] = '\0'; // Добавляем завершающий нуль-символ

        // Запись в лог
        logMutex.lock();
        logQueue.push(std::string(buffer)); // Добавляем данные в очередь
        logFile.flush();
        logMutex.unlock();
    }
    close(clientSocket);
}


void TCPServer::acceptConnections() {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    while (running) {
        // Установка тайм-аута для accept с помощью select
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        struct timeval timeout;
        timeout.tv_sec = 0;  // Установка тайм-аута на 0 секунд
        timeout.tv_usec = 10000;  // Установка тайм-аута на 10 миллисекунд

        int ready = select(serverSocket + 1, &readfds, nullptr, nullptr, &timeout);
        if (ready > 0) {
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

void TCPServer::logWorker() {
    while (running) {
        // проверка: пуста ли очередь
        if (!logQueue.empty()) {
            // захват мьютекса для обеспечения доступа к ресурсам в потокобезопасном режиме
            logMutex.lock();
            while (!logQueue.empty()) {
                // извлечение записи из начала очереди
                std::string logEntry = logQueue.front();
                std::cout << logEntry;  // Вывод в консоль
                logFile << logEntry;    // Запись в файл
                // удаление этой записи из начала очереди
                logQueue.pop();
            }
            // освобождение мьютекса
            logMutex.unlock();
        }
        // ожидание 100 милисекунд
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void TCPServer::inputHandler() {
    while (running) {
        // Неблокирующее чтение символа с консоли
        if (kbhit()) {
            char c = getchar(); 
            if (c == 'q') {
                // Вызов метода остановки сервера при получении 'q'
                stop();
                return;
            }
        }
        // ожидание 100 милисекунд
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

    //проверка ввода на клавиатуре без блокирования основного потока
int TCPServer::kbhit() {
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

    // Добавляем запись в лог с текущим временем
void TCPServer::log(const std::string& message) {
    logMutex.lock();
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char timeStr[64];
    std::strftime(timeStr, sizeof(timeStr), "[%Y-%m-%d %H:%M:%S] ", std::localtime(&now));
    logFile << timeStr << message << std::endl;
    std::cout << timeStr << message << std::endl;
    logMutex.unlock();
}

void TCPServer::start() {
    if (!initServer()) return;
    running = true;

    log("Starting the server");

    if (!startListening()) return;

    // создание потока логирования для записи сообщений из очереди в файл журнала
    std::thread loggerThread(&TCPServer::logWorker, this);
    // создание потока логирования для обработки ввода пользователя с консоли
    std::thread inputThread(&TCPServer::inputHandler, this);
    acceptConnections();
    loggerThread.join();
    inputThread.join();
}

void TCPServer::stop() {
    if (running) log("Shutting down the server");

    running = false;
    // закрытие сокета
    close(serverSocket);
    if (logFile.is_open()) {
        logFile.close();
    }
}

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


