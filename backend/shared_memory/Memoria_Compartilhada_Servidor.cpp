#include <windows.h>
#include <iostream>
#include <string>

int main() {
    const char* sharedName = "MySharedMemory";
    const char* mutexName = "MySharedMemoryMutex";

    // Mensagem simulada recebida
    std::string received = "Dados recebidos com sucesso!";
    std::string jsonResponse =
        "{\"connected\":true,\"ok\":true,\"received\":\"" + received + "\"}";

    // 1. Cria memória compartilhada
    HANDLE hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        512,
        sharedName
    );

    if (hMapFile == NULL) {
        std::cerr << "CreateFileMapping failed: " << GetLastError() << std::endl;
        return 1;
    }

    // 2. Mapeia memória
    LPVOID pBuf = MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0, 0, 512
    );

    if (pBuf == NULL) {
        std::cerr << "MapViewOfFile failed: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    // 3. Cria mutex
    HANDLE hMutex = CreateMutexA(NULL, FALSE, mutexName);
    if (hMutex == NULL) {
        std::cerr << "CreateMutex failed: " << GetLastError() << std::endl;
        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);
        return 1;
    }

    // 4. Checa estado do mutex (sem bloquear)
    DWORD waitResult = WaitForSingleObject(hMutex, 0);
    if (waitResult == WAIT_OBJECT_0) {
        std::cout << "[Servidor] Mutex estava livre -> bloqueado pelo servidor." << std::endl;
    }
    else if (waitResult == WAIT_TIMEOUT) {
        std::cout << "[Servidor] Mutex esta ocupado por outro processo." << std::endl;
        // Para fins de teste, força bloqueio
        WaitForSingleObject(hMutex, INFINITE);
    }
    else {
        std::cout << "[Servidor] Erro ao verificar estado do mutex." << std::endl;
    }

    // 5. Escreve JSON na memória
    CopyMemory((PVOID)pBuf, jsonResponse.c_str(), jsonResponse.size() + 1);

    std::cout << "[Servidor] Escreveu JSON: " << jsonResponse << std::endl;

    // Dump da memória em HEX
    unsigned char* raw = (unsigned char*)pBuf;
    std::cout << "[Servidor] Dump da memoria: ";
    for (int i = 0; i < 64; i++) {
        if (raw[i] == 0) break;
        printf("%02X ", raw[i]);
    }
    std::cout << std::endl;

    std::cout << "[Servidor] Aguardando cliente ler (pressione ENTER para sair)..." << std::endl;
    std::cin.get();

    // 6. Libera mutex
    ReleaseMutex(hMutex);

    // 7. Fecha recursos
    CloseHandle(hMutex);
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    return 0;
}

