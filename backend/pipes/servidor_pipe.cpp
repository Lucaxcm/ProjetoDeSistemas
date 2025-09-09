#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>

int main() {
    HANDLE hStdInRead, hStdInWrite;
    HANDLE hStdOutRead, hStdOutWrite;
    SECURITY_ATTRIBUTES saAttr;

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Pipe para STDOUT do cliente
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0)) {
        std::cerr << "{\"error\":\"CreatePipe stdout\"}" << std::endl;
        return 1;
    }
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    // Pipe para STDIN do cliente
    if (!CreatePipe(&hStdInRead, &hStdInWrite, &saAttr, 0)) {
        std::cerr << "{\"error\":\"CreatePipe stdin\"}" << std::endl;
        return 1;
    }
    SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0);

    // Configuração para iniciar cliente
    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    STARTUPINFOA siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
    siStartInfo.cb = sizeof(STARTUPINFOA);
    siStartInfo.hStdError = hStdOutWrite;
    siStartInfo.hStdOutput = hStdOutWrite;
    siStartInfo.hStdInput = hStdInRead;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Cria cliente como processo filho
    if (!CreateProcessA(
        NULL,
        (LPSTR)"cliente_pipe.exe",
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &siStartInfo,
        &piProcInfo)) {
        std::cerr << "{\"error\":\"CreateProcess failed\",\"code\":" << GetLastError() << "}" << std::endl;
        return 1;
    }

    CloseHandle(hStdOutWrite);
    CloseHandle(hStdInRead);

    // Loop: lê mensagens do Node (stdin) e encaminha ao cliente
    std::string line;
    char buffer[256];
    DWORD written, read;

    while (std::getline(std::cin, line)) {
        // Envia para o cliente
        WriteFile(hStdInWrite, line.c_str(), (DWORD)line.size(), &written, NULL);
        WriteFile(hStdInWrite, "\n", 1, &written, NULL);

        // Lê resposta do cliente
        std::ostringstream clientOutput;
        while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &read, NULL) && read > 0) {
            buffer[read] = '\0';
            clientOutput << buffer;
            if (buffer[read - 1] == '\n') break; // assume resposta única por linha
        }

        // Retorna resposta JSON para o Node (stdout)
        std::cout << clientOutput.str() << std::endl;
        std::cout.flush();
    }

    // Encerramento
    CloseHandle(hStdInWrite);
    CloseHandle(hStdOutRead);
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    return 0;
}


