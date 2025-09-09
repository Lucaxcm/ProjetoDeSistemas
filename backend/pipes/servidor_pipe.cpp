#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>

int main() {
    char buffer[256];
    DWORD read;

    // loop principal: le mensagens que chegam do stdin
    // stdin aqui esta conectado ao pipe anonimo criado pelo backend
    while (ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, sizeof(buffer) - 1, &read, NULL) && read > 0) {
        // adiciona terminador de string
        buffer[read] = '\0';
        std::string received = buffer;

        // cria a resposta no formato JSON
        std::ostringstream response;
        response << "{"
            << "\"connected\":true,"
            << "\"ok\":true,"
            << "\"received\":\"" << received << "\","
            << "\"status\":\"mensagem recebida com sucesso\""
            << "}";

        // envia a resposta JSON pelo stdout
        // o backend ira ler essa saida e repassar ao frontend
        std::cout << response.str() << std::endl;
    }

    return 0;
}


