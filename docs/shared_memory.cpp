#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

// Linka com a biblioteca Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

int main() {
    // 1. Inicializa o Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    // 2. Configura a estrutura do socket
    struct addrinfo* result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;     // Para escutar na porta

    iResult = getaddrinfo(NULL, "8080", &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    // 3. Cria o socket
    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        std::cerr << "socket failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // 4. Associa o socket à porta
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "bind failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);

    // 5. Entra em modo de escuta
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Servidor C++ escutando na porta 8080..." << std::endl;

    // 6. Aceita a conexão do cliente
    SOCKET ClientSocket = INVALID_SOCKET;
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        std::cerr << "accept failed with error: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // 7. Processa a comunicação
    const int recvbuflen = 512;
    char recvbuf[recvbuflen];
    int recvResult;

    // Recebe dados do cliente
    recvResult = recv(ClientSocket, recvbuf, recvbuflen - 1, 0);
    if (recvResult > 0) {
        recvbuf[recvResult] = '\0';
        std::cout << "Dados recebidos do cliente Python: " << recvbuf << std::endl;

        // Cria a resposta JSON manualmente
        std::string jsonResponse = "{\"status\":\"ok\",\"message\":\"Dados recebidos com sucesso!\"}";

        // Envia a resposta JSON de volta ao cliente
        iResult = send(ClientSocket, jsonResponse.c_str(), (int)jsonResponse.length(), 0);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
        }
        else {
            std::cout << "Resposta JSON enviada para o cliente." << std::endl;
        }

    }
    else if (recvResult == 0) {
        std::cout << "Conexao fechada pelo cliente." << std::endl;
    }
    else {
        std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
    }

    // 8. Encerra a conexão e limpa
    closesocket(ClientSocket);
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}