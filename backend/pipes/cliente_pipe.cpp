#include <windows.h>
#include <iostream>
#include <string>

int main() {
    // le uma mensagem digitada pelo usuario no terminal
    std::string msg;
    std::cout << "Digite uma mensagem: ";
    std::getline(std::cin, msg);

    // envia a mensagem para o servidor escrevendo no stdout
    // esse stdout sera redirecionado para o pipe pelo backend
    std::cout << msg << std::endl;

    return 0;
}


