#include "../SERVER/tcp.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    int port = std::atoi(argv[1]);

    TCPServer server(port);
    std::thread serverThread(&TCPServer::run, &server);
    serverThread.detach(); // Detach the server thread to run independently

    for (int i = 2; i < argc; i += 3) {
        std::string name = argv[i];
        int clientPort = std::atoi(argv[i + 1]);
        int period = std::atoi(argv[i + 2]);

        TCPClient client(name, clientPort, period);
        std::thread clientThread(&TCPClient::run, &client);
        clientThread.detach(); // Detach the client thread to run independently
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}