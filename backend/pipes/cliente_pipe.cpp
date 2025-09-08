#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>

int main() {
    // Lê do stdin (o que o servidor escreveu)
    char buffer[256];
    DWORD read;
    std::string received;
    while (ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, sizeof(buffer) - 1, &read, NULL) && read > 0) {
        buffer[read] = '\0';
        received += buffer;
    }

    // Monta resposta JSON para stdout
    std::ostringstream response;
    response << "{"
        << "\"connected\":true,"
        << "\"ok\":true,"
        << "\"received\":\"" << received << "\""
        << "}";

    std::cout << response.str() << std::endl;

    return 0;
}
