#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>

int main() {
    std::string received;

    // Lê uma linha do stdin (vinda do cliente)
    if (std::getline(std::cin, received)) {
        // Monta a resposta JSON
        std::ostringstream response;
        response << "{"
            << "\"connected\":true,"
            << "\"ok\":true,"
            << "\"received\":\"" << received << "\","
            << "\"status\":\"mensagem recebida com sucesso\""
            << "}";

        // Imprime resposta e encerra
        std::cout << response.str() << std::endl;
    }
    else {
        // Caso não tenha recebido nada
        std::cerr << "{\"error\":\"nenhuma mensagem recebida\"}" << std::endl;
    }

    return 0;
}


