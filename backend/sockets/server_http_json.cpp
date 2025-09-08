// Servidor HTTP mínimo (Windows/Winsock2) que devolve JSON
// Rotas:
//   GET  /health → {"connected":true}
//   POST /echo   (body JSON: {"message":"..."})
//                 → {"connected":true,"ok":true,"received":"..."}
// Compilar (MinGW): g++ server_http_json.cpp -o server.exe -lws2_32

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <cctype>

#pragma comment(lib, "Ws2_32.lib") // ignorado no MinGW, útil no MSVC

static std::string to_lower(std::string s){
    for (size_t i=0;i<s.size();++i) s[i] = (char)std::tolower((unsigned char)s[i]);
    return s;
}

// Extrai Content-Length se existir; retorna -1 se não houver
static int parse_content_length(const std::string& headers){
    std::string h = to_lower(headers);
    const std::string key = "content-length:";
    size_t p = h.find(key);
    if (p == std::string::npos) return -1;
    p += key.size();
    while (p < h.size() && std::isspace((unsigned char)h[p])) ++p;
    int len = 0;
    while (p < h.size() && std::isdigit((unsigned char)h[p])) {
        len = len*10 + (h[p]-'0');
        ++p;
    }
    return len;
}

// Extrai "message":"..." do JSON do body de forma simples (sem parser completo)
static std::string extract_message(const std::string& body){
    // procura por "message"
    size_t p = body.find("\"message\"");
    if (p == std::string::npos) return "";
    p = body.find(':', p);
    if (p == std::string::npos) return "";
    // pula espaços
    ++p;
    while (p < body.size() && std::isspace((unsigned char)body[p])) ++p;
    // aceita tanto "..." quanto '...'
    if (p >= body.size() || (body[p] != '"' && body[p] != '\'')) return "";
    char quote = body[p++];
    std::string out;
    while (p < body.size() && body[p] != quote){
        if (body[p] == '\\' && p+1 < body.size()){
            // copia escape simples
            out.push_back(body[p+1]);
            p += 2;
        } else {
            out.push_back(body[p++]);
        }
    }
    return out;
}

// Faz escape mínimo para embutir no JSON de resposta
static std::string json_escape(const std::string& s){
    std::string out; out.reserve(s.size()+8);
    for (size_t i=0;i<s.size();++i){
        char c = s[i];
        if (c == '\\' || c == '"') { out.push_back('\\'); out.push_back(c); }
        else if (c == '\r') { /* ignora */ }
        else if (c == '\n') { out += "\\n"; }
        else { out.push_back(c); }
    }
    return out;
}

int main(){
    // --- Winsock ---
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0){
        std::cerr << "WSAStartup falhou. Cod: " << WSAGetLastError() << "\n";
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET){
        std::cerr << "socket() falhou. Cod: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    BOOL opt = TRUE;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // só localhost; mude para htonl(INADDR_ANY) se quiser expor
    addr.sin_port = htons(8080);

    if (bind(listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR){
        std::cerr << "bind() falhou. Cod: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }
    if (listen(listenSock, 1) == SOCKET_ERROR){
        std::cerr << "listen() falhou. Cod: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    std::cout << "HTTP JSON server em http://127.0.0.1:8080\n";

    while (true){
        sockaddr_in cli; int cliLen = sizeof(cli);
        SOCKET s = accept(listenSock, (sockaddr*)&cli, &cliLen);
        if (s == INVALID_SOCKET){
            std::cerr << "accept() falhou. Cod: " << WSAGetLastError() << "\n";
            break;
        }

        // --- lê request (headers + body opcional) ---
        const int BUFSZ = 4096;
        char buf[BUFSZ];
        std::string req;

        // lê até encontrar \r\n\r\n; depois, se tiver Content-Length, lê corpo
        int contentLen = -1;
        size_t headerEnd = std::string::npos;

        while (true){
            int n = recv(s, buf, BUFSZ, 0);
            if (n <= 0) break;
            req.append(buf, n);

            if (headerEnd == std::string::npos){
                headerEnd = req.find("\r\n\r\n");
                if (headerEnd != std::string::npos){
                    std::string headers = req.substr(0, headerEnd+4);
                    contentLen = parse_content_length(headers);
                }
            }

            if (headerEnd != std::string::npos){
                size_t haveBody = req.size() - (headerEnd+4);
                if (contentLen <= 0 || haveBody >= (size_t)contentLen) break;
            }
        }

        // --- roteamento simples ---
        std::string method, path;
        {
            size_t sp1 = req.find(' ');
            size_t sp2 = (sp1 == std::string::npos)? std::string::npos : req.find(' ', sp1+1);
            method = (sp1==std::string::npos) ? "" : req.substr(0, sp1);
            path   = (sp2==std::string::npos) ? "" : req.substr(sp1+1, sp2-sp1-1);
        }

        std::string body;
        if (headerEnd != std::string::npos){
            size_t start = headerEnd + 4;
            if (contentLen > 0 && start + (size_t)contentLen <= req.size())
                body = req.substr(start, (size_t)contentLen);
            else if (start < req.size())
                body = req.substr(start);
        }

        std::string respBody;
        int status = 200;
        std::string statusText = "OK";

        // CORS preflight
        if (method == "OPTIONS"){
            respBody = "";
        } else if (method == "GET" && path == "/health"){
            respBody = "{\"connected\":true}";
        } else if (method == "POST" && path == "/echo"){
            std::string msg = extract_message(body);
            std::string esc = json_escape(msg);
            respBody = std::string("{\"connected\":true,\"ok\":true,\"received\":\"") + esc + "\"}";
        } else {
            status = 404; statusText = "Not Found";
            respBody = "{\"error\":\"not found\"}";
        }

        std::ostringstream oss;
        oss << "HTTP/1.1 " << status << " " << statusText << "\r\n"
            << "Content-Type: application/json\r\n"
            << "Access-Control-Allow-Origin: *\r\n"
            << "Access-Control-Allow-Headers: Content-Type\r\n"
            << "Access-Control-Allow-Methods: GET,POST,OPTIONS\r\n"
            << "Content-Length: " << respBody.size() << "\r\n"
            << "Connection: close\r\n\r\n"
            << respBody;

        std::string resp = oss.str();
        send(s, resp.c_str(), (int)resp.size(), 0);
        closesocket(s);
    }

    closesocket(listenSock);
    WSACleanup();
    return 0;
}
