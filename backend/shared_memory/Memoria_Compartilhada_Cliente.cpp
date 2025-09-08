#include <windows.h>
#include <iostream>
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

    // 1. Abre memória compartilhada
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, sharedName);
    if (hMapFile == NULL) {
        std::cerr << "{\"error\":\"OpenFileMapping failed\",\"code\":" << GetLastError() << "}" << std::endl;
        return 1;
    }

    // 2. Mapeia memória
    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 512);
    if (pBuf == NULL) {
        std::cerr << "{\"error\":\"MapViewOfFile failed\",\"code\":" << GetLastError() << "}" << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    // 3. Abre mutex
    HANDLE hMutex = OpenMutexA(SYNCHRONIZE, FALSE, mutexName);
    if (hMutex == NULL) {
        std::cerr << "{\"error\":\"OpenMutex failed\",\"code\":" << GetLastError() << "}" << std::endl;
        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);
        return 1;
    }

    // 4. Checa estado do mutex
    std::string mutexState;
    DWORD waitResult = WaitForSingleObject(hMutex, 0);
    if (waitResult == WAIT_OBJECT_0) {
        mutexState = "free";
    }
    else if (waitResult == WAIT_TIMEOUT) {
        mutexState = "locked";
    }
    else {
        mutexState = "error";
    }

    // 5. Lê JSON da memória
    std::string jsonStr = (char*)pBuf;

    // 6. Gera dump em hex
    unsigned char* raw = (unsigned char*)pBuf;
    std::string hexDump = bytesToHex(raw, 64);

    // 7. Monta saída em JSON para frontend
    std::ostringstream out;
    out << "{";
    out << "\"mutex\":\"" << mutexState << "\",";
    out << "\"jsonMessage\":" << "\"" << jsonStr << "\",";
    out << "\"hexDump\":" << hexDump;
    out << "}";

    std::cout << out.str() << std::endl;

    // 8. Libera mutex
    ReleaseMutex(hMutex);

    // 9. Fecha recursos
    CloseHandle(hMutex);
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    return 0;
}

