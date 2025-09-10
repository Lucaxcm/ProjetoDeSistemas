# ProjetoDeSistemas
Projeto universitário desenvolvido na PUCPR para a disciplina de Sistemas da Computação.

# Comandos para inicialização dos Pipes
1º  No Developer Command Prompt for VS 2022:
-   cl servidor_pipe.cpp /EHsc
-   cl cliente_pipe.cpp /EHsc

2º  Abrir um Promt de Comando:
-   servidor_pipe.exe | cliente_pipe.exe

# Comandos para inicialização da Memoria Compartilhada
1º  No Developer Command Prompt for VS 2022:
-   cl Memoria_Compartilhada_Servidor.cpp /EHsc
-   cl Memoria_Compartilhada_Cliente.cpp /EHsc

2º  Abrir um Promt de Comando:
-   Memoria_Compartilhada_Servidor.exe

3º  Abrir um Promt de Comando:
-   Memoria_Compartilhada_Cliente.exe

# Comandos para inicialização dos Sockets (HTTP JSON)
1º  Ou no MinGW (g++) no VS Code/Prompt:
-   g++ server_http_json.cpp -o bin/server.exe -lws2_32
-   g++ client_http_post.cpp -o bin/client.exe -lws2_32

2º  Executar em dois Prompts de Comando:
-   Primeiro:  server.exe
-   Depois:   client.exe "mensagem"
