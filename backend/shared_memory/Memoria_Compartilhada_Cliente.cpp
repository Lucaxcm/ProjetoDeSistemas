#include <windows.h>
#include <iostream>
#include <string>

int main() {
    const char* sharedName = "MySharedMemory";
    const char* mutexName = "MySharedMemoryMutex";

    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, sharedName);
    if (!hMapFile) { std::cerr << "{\"error\":\"OpenFileMapping failed\"}"; return 1; }

    HANDLE hMutex = OpenMutexA(SYNCHRONIZE, FALSE, mutexName);
    if (!hMutex) { std::cerr << "{\"error\":\"OpenMutex failed\"}"; CloseHandle(hMapFile); return 1; }

    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 512);
    if (!pBuf) { std::cerr << "{\"error\":\"MapViewOfFile failed\"}"; CloseHandle(hMutex); CloseHandle(hMapFile); return 1; }

    std::string msg;
    std::cout << "Digite a mensagem para o servidor: ";
    std::getline(std::cin, msg);

    WaitForSingleObject(hMutex, INFINITE);
    strcpy_s((char*)pBuf, 512, msg.c_str());
    ReleaseMutex(hMutex);

    Sleep(1500); // espera servidor processar

    WaitForSingleObject(hMutex, INFINITE);
    std::string resposta = (char*)pBuf;
    ReleaseMutex(hMutex);

    std::cout << "JSON do servidor: " << resposta << std::endl;

    UnmapViewOfFile(pBuf);
    CloseHandle(hMutex);
    CloseHandle(hMapFile);

    return 0;
}

