// client_http_post.cpp
// Cliente Windows/Winsock2 que envia POST /echo com JSON {"message":"..."} para http://127.0.0.1:8080
// Uso: client.exe "sua mensagem aqui"
// Compilar (MinGW): g++ client_http_post.cpp -o client.exe -lws2_32
// Compilar (MSVC):  cl /EHsc client_http_post.cpp ws2_32.lib

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

// Função utilitária simples para escapar aspas e barras no JSON
static std::string json_escape(const std::string& s) {
    std::string out; out.reserve(s.size()+8);
    for (char c : s) {
        if (c == '\\' || c == '\"') { out.push_back('\\'); out.push_back(c); }
        else if (c == '\r') { /* ignora */ }
        else if (c == '\n') { out += "\\n"; }
        else { out.push_back(c); }
    }
    return out;
}

int main(int argc, char* argv[]) {
    // 1) Pegar mensagem do argv (ex.: client.exe "oi")
    std::string msg = (argc >= 2) ? argv[1] : "hello from client";

    // 2) Iniciar Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "WSAStartup falhou. Cod: " << WSAGetLastError() << "\n";
        return 1;
    }

    // 3) Criar socket TCP
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() falhou. Cod: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // 4) Conectar em 127.0.0.1:8080 (porta do seu servidor HTTP)
    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(8080);
    srv.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (sockaddr*)&srv, sizeof(srv)) == SOCKET_ERROR) {
        std::cerr << "connect() falhou. Cod: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // 5) Montar o corpo JSON esperado pelo servidor
    std::string body = std::string("{\"message\":\"") + json_escape(msg) + "\"}";

    // 6) Montar o request HTTP (CRLF = \r\n é obrigatório)
    std::string req;
    req += "POST /echo HTTP/1.1\r\n";
    req += "Host: 127.0.0.1:8080\r\n";
    req += "Content-Type: application/json\r\n";
    req += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    req += "Connection: close\r\n";
    req += "\r\n";
    req += body;

    // 7) Enviar request
    if (send(sock, req.c_str(), (int)req.size(), 0) == SOCKET_ERROR) {
        std::cerr << "send() falhou. Cod: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // 8) Ler a resposta HTTP até o servidor fechar a conexão ("Connection: close")
    const int BUFSZ = 4096;
    char buf[BUFSZ];
    std::string resp;
    while (true) {
        int n = recv(sock, buf, BUFSZ, 0);
        if (n <= 0) break; // 0 = conexão fechada, <0 = erro
        resp.append(buf, n);
    }

    // 9) Mostrar resposta (opcional: separar headers do body)
    std::cout << "Resposta do servidor:\n" << resp << "\n";

    // 10) Encerrar
    closesocket(sock);
    WSACleanup();
    return 0;
}
