#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>

int main() {
    std::string line;

    while (std::getline(std::cin, line)) {
        std::ostringstream response;
        response << "{"
            << "\"connected\":true,"
            << "\"ok\":true,"
            << "\"received\":\"" << line << "\""
            << "}\n"; // \n para facilitar leitura no servidor

        std::cout << response.str();
        std::cout.flush();
    }

    return 0;
}

