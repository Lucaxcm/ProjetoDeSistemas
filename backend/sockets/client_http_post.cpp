// ============================================================================
// client_http_post.cpp
//
// Cliente Windows/Winsock2 que envia um POST /echo com JSON {"message":"..."}
// para o servidor HTTP local em http://127.0.0.1:8080.
//
// Uso:
//   client.exe "sua mensagem aqui"
//
// Exemplo de execução:
//   .\client.exe "oi"
//
// Compilar (MinGW):
//   g++ client_http_post.cpp -o bin/client.exe -lws2_32
//
// Compilar (MSVC):
//   cl /EHsc client_http_post.cpp ws2_32.lib
//
// Observações:
// - Este cliente monta manualmente uma requisição HTTP (linha de requisição,
//   cabeçalhos e corpo) e a envia via TCP para o servidor.
// - Em seguida, lê toda a resposta até o servidor fechar a conexão.
// - O objetivo é ser simples e didático, não um cliente HTTP completo.
// ============================================================================

#define _WINSOCK_DEPRECATED_NO_WARNINGS   // permite inet_addr em alguns toolchains
#include <winsock2.h>     // API de sockets no Windows
#include <ws2tcpip.h>     // funções auxiliares
#include <iostream>       // std::cout / std::cerr
#include <string>         // std::string
#include <cstring>        // std::memset

#pragma comment(lib, "Ws2_32.lib") // (apenas MSVC) linka contra Winsock2

// ----------------------------------------------------------------------------
/*
  json_escape
  - Prepara uma string para ser embutida no JSON (escapa \ e " e trata \n).
  - Importante para não "quebrar" o JSON da requisição se a mensagem tiver
    aspas ou quebras de linha.
*/
static std::string json_escape(const std::string& s) {
    std::string out; out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\\' || c == '\"') { out.push_back('\\'); out.push_back(c); }
        else if (c == '\r') { /* ignora CR */ }
        else if (c == '\n') { out += "\\n"; }   // normaliza quebra de linha
        else { out.push_back(c); }
    }
    return out;
}

int main(int argc, char* argv[]) {
    // (1) Captura a mensagem da linha de comando (argv)
    //     Se não for passada, usa um default para facilitar teste.
    std::string msg = (argc >= 2) ? argv[1] : "hello from client";

    // (2) Inicializa a DLL do Winsock (obrigatório antes de usar sockets)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "WSAStartup falhou. Cod: " << WSAGetLastError() << "\n";
        return 1;
    }

    // (3) Cria um socket TCP (AF_INET IPv4, SOCK_STREAM/TCP)
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() falhou. Cod: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // (Opcional) Desativar Nagle para enviar logo o request (latência menor).
    // Não é necessário para este exemplo, mas fica aqui como referência:
    // BOOL nodelay = TRUE;
    // setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&nodelay, sizeof(nodelay));

    // (4) Prepara o endereço do servidor (127.0.0.1:8080)
    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(8080);                 // porta do servidor
    srv.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost

    // (5) Conecta ao servidor
    if (connect(sock, (sockaddr*)&srv, sizeof(srv)) == SOCKET_ERROR) {
        std::cerr << "connect() falhou. Cod: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // (6) Monta o corpo JSON no formato esperado pelo servidor: {"message":"..."}
    //     Usamos json_escape para garantir que as aspas e barras sejam válidas.
    std::string body = std::string("{\"message\":\"") + json_escape(msg) + "\"}";

    // (7) Monta o request HTTP:
    //     - Linha de requisição: "POST /echo HTTP/1.1\r\n"
    //     - Cabeçalhos básicos: Host, Content-Type, Content-Length, Connection
    //     - Linha em branco "\r\n"
    //     - Corpo JSON
    //
    // Importante: HTTP exige CRLF (\r\n) ao final de cada linha de cabeçalho.
    std::string req;
    req += "POST /echo HTTP/1.1\r\n";
    req += "Host: 127.0.0.1:8080\r\n";
    req += "Content-Type: application/json\r\n";
    req += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    req += "Connection: close\r\n";   // servidor vai fechar a conexão após responder
    req += "\r\n";                    // linha em branco separando headers do body
    req += body;                      // corpo JSON

    // (8) Envia a requisição para o servidor
    if (send(sock, req.c_str(), (int)req.size(), 0) == SOCKET_ERROR) {
        std::cerr << "send() falhou. Cod: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // (9) Lê a resposta até o servidor fechar a conexão (porque pedimos "Connection: close")
    //     - Montamos tudo em uma string 'resp' para mostrar ao final.
    const int BUFSZ = 4096;
    char buf[BUFSZ];
    std::string resp;

    while (true) {
        int n = recv(sock, buf, BUFSZ, 0);
        if (n == 0) {
            // 0 = servidor fechou a conexão (fim da resposta)
            break;
        }
        if (n < 0) {
            // <0 = erro de leitura
            std::cerr << "recv() falhou. Cod: " << WSAGetLastError() << "\n";
            closesocket(sock);
            WSACleanup();
            return 1;
        }
        resp.append(buf, n); // acumula no buffer de resposta
    }

    // (10) Exibe a resposta completa (status line + headers + body)
    //      Se preferir só o corpo, você pode separar pelo "\r\n\r\n".
    std::cout << "Resposta do servidor:\n" << resp << "\n";

    // Exemplo (opcional): separar e mostrar apenas o corpo JSON
    // size_t p = resp.find("\r\n\r\n");
    // if (p != std::string::npos) {
    //     std::string bodyOnly = resp.substr(p + 4);
    //     std::cout << "JSON: " << bodyOnly << "\n";
    // }

    // (11) Fecha o socket e finaliza Winsock
    closesocket(sock);
    WSACleanup();
    return 0;
}
