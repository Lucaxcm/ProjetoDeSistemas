// ============================================================================
// Servidor HTTP mínimo (Windows/Winsock2) que devolve JSON
//
// Rotas atendidas:
//   - GET  /health  →  {"connected":true}
//   - POST /echo    (body JSON: {"message":"..."})
//                    → {"connected":true,"ok":true,"received":"..."}
//
// Destaques:
//   - Implementado com Winsock2 (Windows).
//   - Roteamento HTTP *bem simples* (apenas método + path).
//   - CORS liberado para facilitar testes no navegador (fetch).
//   - LOGs no console mostrando conexões, rota, corpo e mensagem extraída.
//   - Compatível com MinGW (g++) e MSVC (Visual Studio).
//
// Compilar (MinGW):
//   g++ server_http_json.cpp -o bin/server.exe -lws2_32
//
// Compilar (MSVC):
//   cl /EHsc server_http_json.cpp ws2_32.lib
// ============================================================================

#define _WINSOCK_DEPRECATED_NO_WARNINGS  // permite inet_addr/inet_ntoa em alguns toolchains
#include <winsock2.h>     // API de sockets no Windows
#include <ws2tcpip.h>     // funções extras (algumas dependem do toolchain)
#include <iostream>       // std::cout / std::cerr
#include <sstream>        // std::ostringstream (montar resposta HTTP)
#include <string>         // std::string
#include <cstring>        // std::memset / std::strncpy
#include <cctype>         // std::tolower / std::isdigit / std::isspace
#include <ctime>          // std::time / std::localtime (para timestamp nos logs)

#pragma comment(lib, "Ws2_32.lib") // (apenas MSVC) linka contra Winsock2

// ----------------------------------------------------------------------------
// Função utilitária: transforma uma string para minúsculas.
// Uso: facilitar a busca de cabeçalhos sem depender de "case" (ex.: Content-Length).
// ----------------------------------------------------------------------------
static std::string to_lower(std::string s){
    for (size_t i = 0; i < s.size(); ++i)
        s[i] = (char)std::tolower((unsigned char)s[i]);
    return s;
}

// ----------------------------------------------------------------------------
/*
  Função utilitária: parse_content_length
  - Recebe a parte de cabeçalhos de uma requisição HTTP (como string).
  - Procura por "Content-Length:" ignorando maiúsculas/minúsculas.
  - Se encontrar, lê os dígitos subsequentes e retorna o tamanho do corpo.
  - Se não encontrar, retorna -1 (sem Content-Length).
*/
static int parse_content_length(const std::string& headers){
    std::string h = to_lower(headers);
    const std::string key = "content-length:";
    size_t p = h.find(key);
    if (p == std::string::npos) return -1; // não tem cabeçalho Content-Length

    p += key.size();                        // pula "content-length:"
    // pula espaços
    while (p < h.size() && std::isspace((unsigned char)h[p])) ++p;

    // lê os dígitos do tamanho
    int len = 0;
    while (p < h.size() && std::isdigit((unsigned char)h[p])) {
        len = len * 10 + (h[p] - '0');
        ++p;
    }
    return len; // tamanho do corpo (em bytes), ou 0 se "0", ou -1 se não achou
}

// ----------------------------------------------------------------------------
/*
  Função utilitária: extract_message
  - Recebe o corpo (body) JSON da requisição como string.
  - Faz uma extração *simples* do campo "message" (sem usar um parser JSON completo).
  - Retorna o conteúdo de "message" sem aspas.
  - Se falhar (não tem "message" ou formato inesperado), retorna "".
  Obs.: é suficiente para nossos testes; para produção, prefira um parser JSON real.
*/
static std::string extract_message(const std::string& body){
    // procura pela chave "message"
    size_t p = body.find("\"message\"");
    if (p == std::string::npos) return "";

    // vai até o ':' após a chave
    p = body.find(':', p);
    if (p == std::string::npos) return "";

    // pula ':' e possíveis espaços
    ++p;
    while (p < body.size() && std::isspace((unsigned char)body[p])) ++p;

    // aceita valor entre aspas simples ou duplas
    if (p >= body.size() || (body[p] != '"' && body[p] != '\'')) return "";
    char quote = body[p++];

    // copia os caracteres até fechar a aspa (tratando escape simples de barra)
    std::string out;
    while (p < body.size() && body[p] != quote){
        if (body[p] == '\\' && p + 1 < body.size()){
            // copia o próximo char “escapado” literalmente (ex.: \" → ")
            out.push_back(body[p + 1]);
            p += 2;
        } else {
            out.push_back(body[p++]);
        }
    }
    return out; // conteúdo da mensagem
}

// ----------------------------------------------------------------------------
/*
  Função utilitária: json_escape
  - Prepara uma string para ser embutida em JSON de resposta:
    * escapa aspas duplas (") e barra invertida (\)
    * converte \n para sequência "\\n"
    * ignora \r
  - Retorna a string segura para ser concatenada no JSON de saída.
*/
static std::string json_escape(const std::string& s){
    std::string out; out.reserve(s.size() + 8);
    for (char c : s){
        if (c == '\\' || c == '"') { out.push_back('\\'); out.push_back(c); }
        else if (c == '\r') { /* ignora */ }
        else if (c == '\n') { out += "\\n"; }
        else { out.push_back(c); }
    }
    return out;
}

// ----------------------------------------------------------------------------
/*
  Função utilitária: now_str
  - Retorna um ponteiro para buffer estático contendo a hora atual no formato HH:MM:SS.
  - Usa std::localtime (portável entre g++ e MSVC para este uso simples).
  - Apenas para fins de LOG; não é thread-safe, mas ok aqui (single-thread).
*/
static const char* now_str(){
    static char buf[32];
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    if (!tm) { std::snprintf(buf, sizeof(buf), "??:??:??"); return buf; }
    std::strftime(buf, sizeof(buf), "%H:%M:%S", tm);
    return buf;
}

// ----------------------------------------------------------------------------
/*
  Função utilitária: ipv4_to_string
  - Converte endereço IPv4 do cliente (sockaddr_in) para string "A.B.C.D".
  - Usa inet_ntoa (compatível com MinGW/MSVC).
  - Copia para 'out' garantindo terminação NUL.
*/
static void ipv4_to_string(const sockaddr_in& cli, char* out, size_t outsz){
    const char* s = inet_ntoa(cli.sin_addr);
    if (!s) { std::snprintf(out, outsz, "?.?.?.?"); return; }
    std::strncpy(out, s, outsz);
    out[outsz - 1] = '\0';
}

// ============================================================================
//                                    main
// ============================================================================

int main(){
    // (1) Inicializa a DLL do Winsock (sempre obrigatório antes de usar sockets)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0){
        std::cerr << "WSAStartup falhou. Cod: " << WSAGetLastError() << "\n";
        return 1;
    }

    // (2) Cria socket TCP para escuta (servidor)
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET){
        std::cerr << "socket() falhou. Cod: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // (3) Habilita reuso de endereço/porta (ajuda em reinicializações rápidas)
    BOOL opt = TRUE;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    // (4) Preenche a estrutura de endereço para bind:
    //     - IPv4 (AF_INET)
    //     - 127.0.0.1 (localhost) → apenas máquina local
    //     - porta 8080 (HTTP de teste)
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8080);

    // (5) Associa o socket à porta/IP escolhidos
    if (bind(listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR){
        std::cerr << "bind() falhou. Cod: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    // (6) Coloca o socket em modo "escuta" (aceitar conexões)
    if (listen(listenSock, 16) == SOCKET_ERROR){
        std::cerr << "listen() falhou. Cod: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    std::cout << "HTTP JSON server em http://127.0.0.1:8080\n";

    int conn_id = 0; // contador de conexões (apenas para LOG)

    // (7) Loop principal do servidor (aceita e trata conexões sequencialmente)
    while (true){
        // (7.1) Bloqueia esperando um cliente conectar
        sockaddr_in cli{}; int cliLen = sizeof(cli);
        SOCKET s = accept(listenSock, (sockaddr*)&cli, &cliLen);
        if (s == INVALID_SOCKET){
            std::cerr << "accept() falhou. Cod: " << WSAGetLastError() << "\n";
            break; // sai do loop principal (poderia tentar continuar)
        }

        // (7.2) LOG básico da conexão
        ++conn_id;
        char ipbuf[64];
        ipv4_to_string(cli, ipbuf, sizeof(ipbuf));
        std::cout << "[" << now_str() << "] #" << conn_id
                  << " conectado de " << ipbuf << ":" << ntohs(cli.sin_port) << "\n";

        // Buffers para leitura do request
        const int BUFSZ = 4096;
        char buf[BUFSZ];
        std::string req;              // acumulador do request completo (header + body)

        int    contentLen = -1;       // tamanho do corpo, se houver
        size_t headerEnd  = std::string::npos; // posição do fim dos headers ("\r\n\r\n")

        // (7.3) Lê do socket até termos (a) cabeçalhos + (b) corpo suficiente (se Content-Length)
        while (true){
            int n = recv(s, buf, BUFSZ, 0);
            if (n <= 0) break;        // 0=conexão fechada pelo cliente, <0=erro
            req.append(buf, n);       // acumula no string

            // Se ainda não achamos o fim dos headers, procure por "\r\n\r\n"
            if (headerEnd == std::string::npos){
                headerEnd = req.find("\r\n\r\n");
                if (headerEnd != std::string::npos){
                    // cabeçalhos vão de 0 até headerEnd+4 (inclui CRLF CRLF)
                    std::string headers = req.substr(0, headerEnd + 4);
                    contentLen = parse_content_length(headers); // tenta achar Content-Length
                }
            }

            // Se já temos o fim dos headers...
            if (headerEnd != std::string::npos){
                // quantos bytes de body já temos no buffer?
                size_t haveBody = req.size() - (headerEnd + 4);

                // Se não há Content-Length, ou já temos o corpo inteiro, podemos parar de ler
                if (contentLen <= 0 || haveBody >= (size_t)contentLen) break;
            }
        }

        // (7.4) Roteamento *muito simples*: extrai método e o path da primeira linha do request
        std::string method, path;
        {
            size_t sp1 = req.find(' ');                                         // separa método
            size_t sp2 = (sp1 == std::string::npos) ? std::string::npos
                                                    : req.find(' ', sp1 + 1);   // separa path
            method = (sp1 == std::string::npos) ? "" : req.substr(0, sp1);
            path   = (sp2 == std::string::npos) ? "" : req.substr(sp1 + 1, sp2 - sp1 - 1);
        }

        // (7.5) Separamos o corpo (body), se houver
        std::string body;
        if (headerEnd != std::string::npos){
            size_t start = headerEnd + 4;                 // pula "\r\n\r\n"
            if (contentLen > 0 && start + (size_t)contentLen <= req.size())
                body = req.substr(start, (size_t)contentLen);
            else if (start < req.size())
                body = req.substr(start);                 // sem Content-Length (ex.: chunked) – aqui só pega o que chegou
        }

        // (7.6) LOG da requisição recebida
        std::cout << "  -> " << method << " " << path << "\n";
        if (!body.empty()) std::cout << "  -> body: " << body << "\n";

        // (7.7) Montagem da resposta conforme a rota
        std::string respBody;
        int         status     = 200;
        std::string statusText = "OK";

        if (method == "OPTIONS"){
            // Preflight CORS: devolvemos vazio com cabeçalhos CORS (adicionados abaixo)
            respBody = "";
        }
        else if (method == "GET" && path == "/health"){
            // Rota de health-check
            respBody = "{\"connected\":true}";
        }
        else if (method == "POST" && path == "/echo"){
            // Rota POST /echo: extrai "message" do body e devolve em JSON
            std::string msg = extract_message(body);
            std::cout << "  -> message extraída: \"" << msg << "\"\n";
            std::string esc = json_escape(msg);
            respBody = std::string("{\"connected\":true,\"ok\":true,\"received\":\"") + esc + "\"}";
        }
        else {
            // Qualquer outra rota: 404
            status = 404; statusText = "Not Found";
            respBody = "{\"error\":\"not found\"}";
        }

        // (7.8) Constrói a resposta HTTP completa (status line + headers + body)
        //       Observação: incluímos CORS para facilitar chamadas via navegador (fetch)
        std::ostringstream oss;
        oss << "HTTP/1.1 " << status << " " << statusText << "\r\n"
            << "Content-Type: application/json\r\n"
            << "Access-Control-Allow-Origin: *\r\n"
            << "Access-Control-Allow-Headers: Content-Type\r\n"
            << "Access-Control-Allow-Methods: GET,POST,OPTIONS\r\n"
            << "Content-Length: " << respBody.size() << "\r\n"
            << "Connection: close\r\n\r\n"
            << respBody;

        // (7.9) Envia a resposta e encerra a conexão com o cliente
        std::string resp = oss.str();
        send(s, resp.c_str(), (int)resp.size(), 0);
        closesocket(s);

        // (7.10) LOG do envio
        std::cout << "  <- resposta enviada (" << respBody.size() << " bytes)\n";
    }

    // (8) Limpeza final do socket de escuta e da DLL Winsock
    closesocket(listenSock);
    WSACleanup();
    return 0;
}
