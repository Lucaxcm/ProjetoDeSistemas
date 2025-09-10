# ProjetoDeSistemas
Projeto universitário desenvolvido na PUCPR para a disciplina de Sistemas da Computação.

BECKEND
PIPES
# Comandos para inicialização dos Pipes
1º  No Developer Command Prompt for VS 2022:
-   cl servidor_pipe.cpp /EHsc
-   cl cliente_pipe.cpp /EHsc

2º  Abrir um Promt de Comando:
-   servidor_pipe.exe | cliente_pipe.exe

MEMORIA COMPARTILHADA
# Comandos para inicialização da Memoria Compartilhada
1º  No Developer Command Prompt for VS 2022:
-   cl Memoria_Compartilhada_Servidor.cpp /EHsc
-   cl Memoria_Compartilhada_Cliente.cpp /EHsc

2º  Abrir um Promt de Comando:
-   Memoria_Compartilhada_Servidor.exe

3º  Abrir um Promt de Comando:
-   Memoria_Compartilhada_Cliente.exe

SOCKETS
# Comandos para inicialização dos Sockets (HTTP JSON)
1º  Ou no MinGW (g++) no VS Code/Prompt:
-   g++ server_http_json.cpp -o bin/server.exe -lws2_32
-   g++ client_http_post.cpp -o bin/client.exe -lws2_32

2º  Executar em dois Prompts de Comando:
-   Primeiro:  server.exe
-   Depois:   client.exe "mensagem"

FRONTEND
SERVER
# Comandos para inicialização do Frontend 
1º Antes de tudo é necessário baixar as dependencias do Node.js no Command Prompt
-   cd server
-   npm i

INDEX
2º Abrir arquivo index.html no browser do computador
-   Doubleclick no local file:///{caminho_para_seu_diretorio}/frontend/index.html