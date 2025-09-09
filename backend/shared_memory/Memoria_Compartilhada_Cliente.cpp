#include <windows.h>
#include <iostream>
#include <string>

int main() {
    // nomes dos objetos de IPC
    const char* sharedName = "MySharedMemory";
    const char* mutexName = "MySharedMemoryMutex";

    // abre o mapeamento de memoria criado pelo servidor
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, sharedName);
    if (!hMapFile) { std::cerr << "{\"error\":\"OpenFileMapping failed\"}"; return 1; }

    // abre o mutex nomeado para sincronizacao
    HANDLE hMutex = OpenMutexA(SYNCHRONIZE, FALSE, mutexName);
    if (!hMutex) { std::cerr << "{\"error\":\"OpenMutex failed\"}"; CloseHandle(hMapFile); return 1; }

    // projeta a memoria compartilhada no espaco do processo
    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 512);
    if (!pBuf) { std::cerr << "{\"error\":\"MapViewOfFile failed\"}"; CloseHandle(hMutex); CloseHandle(hMapFile); return 1; }

    // lê uma mensagem do usuario via stdin
    std::string msg;
    std::cout << "Digite a mensagem para o servidor: ";
    std::getline(std::cin, msg);

    // entra na regiao critica, escreve a mensagem, e libera
    WaitForSingleObject(hMutex, INFINITE);
    strcpy_s((char*)pBuf, 512, msg.c_str());
    ReleaseMutex(hMutex);

    // aguarda o servidor processar e responder na mesma memoria
    Sleep(1500);

    // entra na regiao critica, le a resposta json do servidor, e libera
    WaitForSingleObject(hMutex, INFINITE);
    std::string resposta = (char*)pBuf;
    ReleaseMutex(hMutex);

    // imprime a resposta json no stdout para consumo do frontend
    std::cout << "JSON do servidor: " << resposta << std::endl;

    // limpeza de recursos
    UnmapViewOfFile(pBuf);
    CloseHandle(hMutex);
    CloseHandle(hMapFile);

    return 0;
}

