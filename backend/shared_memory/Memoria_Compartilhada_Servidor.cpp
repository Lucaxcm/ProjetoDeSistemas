#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

// converte bytes do buffer para representacao hexadecimal em json
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

    // nomes dos objetos de IPC
    const char* sharedName = "MySharedMemory";
    const char* mutexName = "MySharedMemoryMutex";

    // cria o segmento de memoria compartilhada
    HANDLE hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 512, sharedName);
    if (!hMapFile) { std::cerr << "{\"error\":\"CreateFileMapping failed\"}"; return 1; }

    // cria ou abre o mutex nomeado para sincronizacao
    HANDLE hMutex = CreateMutexA(NULL, FALSE, mutexName);
    if (!hMutex) { std::cerr << "{\"error\":\"CreateMutex failed\"}"; CloseHandle(hMapFile); return 1; }

    // projeta a memoria no espaco do servidor
    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 512);
    if (!pBuf) { std::cerr << "{\"error\":\"MapViewOfFile failed\"}"; CloseHandle(hMutex); CloseHandle(hMapFile); return 1; }

    // avisa que o servidor subiu
    std::cout << "{\"status\":\"Servidor iniciado\"}" << std::endl;

    // loop principal: observa memoria, monta json e devolve
    while (true) {

        // consulta o estado do mutex sem bloquear para relatar ao frontend
        std::string mutexState;
        DWORD waitResult = WaitForSingleObject(hMutex, 0);
        if (waitResult == WAIT_OBJECT_0) mutexState = "free";
        else if (waitResult == WAIT_TIMEOUT) mutexState = "locked";
        else mutexState = "error";

        // Lê conteúdo da memória
        std::string msg = (char*)pBuf;

        // quando houver mensagem valida, monta a resposta detalhada
        if (!msg.empty()) {

            // Cria JSON completo
            std::ostringstream jsonOut;
            unsigned char* raw = (unsigned char*)pBuf;

            // constroi json com estado do mutex, mensagem lida, dump em hex e status
            jsonOut << "{"
                << "\"mutex\":\"" << mutexState << "\","
                << "\"message\":\"" << msg << "\","
                << "\"hexDump\":" << bytesToHex(raw, 64) << ","
                << "\"status\":\"mensagem recebida com sucesso\""
                << "}";

            // grava a resposta json de volta na memoria para o cliente ler
            strcpy_s((char*)pBuf, 512, jsonOut.str().c_str());

            // tambem envia a mesma resposta pelo stdout para o backend e frontend
            std::cout << jsonOut.str() << std::endl;
        }

        // garante liberar o mutex se ele foi adquirido no teste sem bloqueio
        ReleaseMutex(hMutex);

        // pequena pausa para reduzir uso de cpu
        Sleep(1000);
    }

    // limpeza de recursos (na pratica este codigo nao sera alcancado por causa do while true)
    UnmapViewOfFile(pBuf);
    CloseHandle(hMutex);
    CloseHandle(hMapFile);
    return 0;
}
