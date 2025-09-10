#include <windows.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string msg;

    if (argc > 1) {
        // Se mensagem foi passada como argumento, usa ela
        msg = argv[1];
    }
    else {
        // Caso contrário, pede para digitar
        std::cout << "Digite uma mensagem: ";
        std::getline(std::cin, msg);
    }

    // Envia mensagem para o servidor via stdout
    std::cout << msg << std::endl;

    return 0;
}


