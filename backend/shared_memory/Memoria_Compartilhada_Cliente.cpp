#include <windows.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    const char* sharedName = "MySharedMemory";
    const char* mutexName = "MySharedMemoryMutex";

    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, sharedName);
    if (!hMapFile) { std::cerr << "{\"error\":\"OpenFileMapping failed\"}"; return 1; }

    HANDLE hMutex = OpenMutexA(SYNCHRONIZE, FALSE, mutexName);
    if (!hMutex) { std::cerr << "{\"error\":\"OpenMutex failed\"}"; CloseHandle(hMapFile); return 1; }

    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 512);
    if (!pBuf) { std::cerr << "{\"error\":\"MapViewOfFile failed\"}"; CloseHandle(hMutex); CloseHandle(hMapFile); return 1; }

    std::string msg;

    if (argc > 1) {
        // Mensagem recebida como parâmetro
        msg = argv[1];
    }
    else {
        // Caso não haja parâmetro, pede ao usuário
        std::cout << "Digite a mensagem para o servidor: ";
        std::getline(std::cin, msg);
    }

    // Escreve a mensagem na memória compartilhada
    WaitForSingleObject(hMutex, INFINITE);
    strcpy_s((char*)pBuf, 512, msg.c_str());
    ReleaseMutex(hMutex);

    // Aguarda o servidor processar
    Sleep(1500);

    // Lê a resposta do servidor
    WaitForSingleObject(hMutex, INFINITE);
    std::string resposta = (char*)pBuf;
    ReleaseMutex(hMutex);

    std::cout << "JSON do servidor: " << resposta << std::endl;

    UnmapViewOfFile(pBuf);
    CloseHandle(hMutex);
    CloseHandle(hMapFile);

    return 0;
}

