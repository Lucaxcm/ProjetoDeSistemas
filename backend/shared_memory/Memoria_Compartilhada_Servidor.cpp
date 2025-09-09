#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

std::string bytesToHex(unsigned char* data, size_t length) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < length; i++) {
        if (data[i] == 0) break;
        oss << "\"0x" << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << "\"";
        if (i < length - 1) oss << ",";
    }
    oss << "]";
    return oss.str();
}

int main() {
    const char* sharedName = "MySharedMemory";
    const char* mutexName = "MySharedMemoryMutex";

    HANDLE hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 512, sharedName);
    if (!hMapFile) { std::cerr << "{\"error\":\"CreateFileMapping failed\"}"; return 1; }

    HANDLE hMutex = CreateMutexA(NULL, FALSE, mutexName);
    if (!hMutex) { std::cerr << "{\"error\":\"CreateMutex failed\"}"; CloseHandle(hMapFile); return 1; }

    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 512);
    if (!pBuf) { std::cerr << "{\"error\":\"MapViewOfFile failed\"}"; CloseHandle(hMutex); CloseHandle(hMapFile); return 1; }

    std::cout << "{\"status\":\"Servidor iniciado\"}" << std::endl;

    while (true) {
        // Verifica estado do mutex sem bloquear
        std::string mutexState;
        DWORD waitResult = WaitForSingleObject(hMutex, 0);
        if (waitResult == WAIT_OBJECT_0) mutexState = "free";
        else if (waitResult == WAIT_TIMEOUT) mutexState = "locked";
        else mutexState = "error";

        // Lê conteúdo da memória
        std::string msg = (char*)pBuf;

        if (!msg.empty()) {
            // Cria JSON completo
            std::ostringstream jsonOut;
            unsigned char* raw = (unsigned char*)pBuf;
            jsonOut << "{"
                << "\"mutex\":\"" << mutexState << "\","
                << "\"message\":\"" << msg << "\","
                << "\"hexDump\":" << bytesToHex(raw, 64) << ","
                << "\"status\":\"mensagem recebida com sucesso\""
                << "}";

            // Escreve na memória e envia para stdout
            strcpy_s((char*)pBuf, 512, jsonOut.str().c_str());
            std::cout << jsonOut.str() << std::endl;
        }

        ReleaseMutex(hMutex);
        Sleep(1000);
    }

    UnmapViewOfFile(pBuf);
    CloseHandle(hMutex);
    CloseHandle(hMapFile);
    return 0;
}
