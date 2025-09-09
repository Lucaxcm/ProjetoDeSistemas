#include <windows.h>
#include <iostream>
#include <string>

int main() {
    std::string msg;
    std::cout << "Digite uma mensagem: ";
    std::getline(std::cin, msg);

    // Envia para o servidor via stdout
    std::cout << msg << std::endl;

    return 0;
}

