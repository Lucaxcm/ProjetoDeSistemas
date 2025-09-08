#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "WSAStartup falhou. Cod: " << WSAGetLastError() << "\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() falhou. Cod: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in srv;
    std::memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(1500);

    unsigned long ip = inet_addr("127.0.0.1");
    if (ip == INADDR_NONE) {
        std::cerr << "IP inválido.\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    srv.sin_addr.s_addr = ip;

    if (connect(sock, (sockaddr*)&srv, sizeof(srv)) == SOCKET_ERROR) {
        std::cerr << "connect() falhou. Cod: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Conectado a 127.0.0.1:1500\n";
    std::cout << "Digite mensagens (Enter para enviar). Ctrl+C para sair.\n\n";

    const int BUFSZ = 1024;
    char rbuf[BUFSZ];

    while (true) {
        std::cout << "> ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\nEntrada encerrada.\n";
            break;
        }
        line.push_back('\n');

        if (send(sock, line.c_str(), (int)line.size(), 0) == SOCKET_ERROR) {
            std::cerr << "send() falhou. Cod: " << WSAGetLastError() << "\n";
            break;
        }

        int n = recv(sock, rbuf, BUFSZ - 1, 0);
        if (n == 0) {
            std::cout << "Servidor encerrou a conexão.\n";
            break;
        }
        if (n < 0) {
            std::cerr << "recv() falhou. Cod: " << WSAGetLastError() << "\n";
            break;
        }
        rbuf[n] = '\0';
        std::cout << "Eco: " << rbuf;
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
