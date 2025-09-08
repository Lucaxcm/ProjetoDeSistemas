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

    // Pipe para STDOUT do cliente -> leitura no servidor
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0)) {
        std::cerr << "{\"error\":\"CreatePipe failed for stdout\"}" << std::endl;
        return 1;
    }
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    // Pipe para STDIN do cliente -> escrita no servidor
    if (!CreatePipe(&hStdInRead, &hStdInWrite, &saAttr, 0)) {
        std::cerr << "{\"error\":\"CreatePipe failed for stdin\"}" << std::endl;
        return 1;
    }
    SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0);

    // Configura o processo filho (cliente)
    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    STARTUPINFOA siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
    siStartInfo.cb = sizeof(STARTUPINFOA);
    siStartInfo.hStdError = hStdOutWrite;
    siStartInfo.hStdOutput = hStdOutWrite;
    siStartInfo.hStdInput = hStdInRead;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Lança o cliente
    if (!CreateProcessA(
        NULL,
        (LPSTR)"cliente_pipe.exe", // nome do binário do cliente
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

    // Envia mensagem para o cliente via pipe
    std::string jsonMessage = "{\"message\":\"Ola do servidor\"}";
    DWORD written;
    WriteFile(hStdInWrite, jsonMessage.c_str(), (DWORD)jsonMessage.size(), &written, NULL);
    CloseHandle(hStdInWrite);

    // Lê resposta do cliente
    char buffer[256];
    DWORD read;
    std::ostringstream output;
    while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &read, NULL) && read > 0) {
        buffer[read] = '\0';
        output << buffer;
    }
    CloseHandle(hStdOutRead);

    // Exibe resposta final em JSON para frontend
    std::cout << output.str() << std::endl;

    // Aguarda o cliente encerrar
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    return 0;
}
