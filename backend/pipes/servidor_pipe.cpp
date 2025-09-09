#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>

int main() {
    char buffer[256];
    DWORD read;

    // Loop contínuo: lê mensagens do stdin e responde
    while (ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, sizeof(buffer) - 1, &read, NULL) && read > 0) {
        buffer[read] = '\0';
        std::string received = buffer;

        // Monta resposta JSON
        std::ostringstream response;
        response << "{"
            << "\"connected\":true,"
            << "\"ok\":true,"
            << "\"received\":\"" << received << "\","
            << "\"status\":\"mensagem recebida com sucesso\""
            << "}";

        // Envia pelo stdout (para o backend -> frontend)
        std::cout << response.str() << std::endl;
    }

    return 0;
}

