const { spawn } = require('child_process');
const fs = require('fs');
const express = require("express");
const path = require("path");
const app = express();
const cors = require("cors");
const { PassThrough } = require('stream');

app.use(express.json());
app.use(cors());

const BASE_DIR = path.resolve(__dirname, '../backend');

const mechanisms = {
    sockets: {
        server: path.join(BASE_DIR, 'sockets/bin/server.exe'),
        client: path.join(BASE_DIR, 'sockets/bin/client.exe')
    },
    shared_memory: {
        server: path.join(BASE_DIR, 'shared_memory/Memoria_Compartilhada_Servidor.exe'),
        client: path.join(BASE_DIR, 'shared_memory/Memoria_Compartilhada_Cliente.exe')
    },
    pipes: {
        server: path.join(BASE_DIR, 'pipes/servidor_pipe.exe'),
        client: path.join(BASE_DIR, 'pipes/cliente_pipe.exe')
    }
};

function parseOutputLine(line, processType) {
    try {
        const jsonData = JSON.parse(line);
        // Para o socket_client, a resposta não é JSON, então tratamos separadamente
        if (processType === 'client' && typeof jsonData !== 'object') {
             return null;
        }
        return {
            process: processType,
            step: jsonData.status || 'data_received',
            details: JSON.stringify(jsonData, null, 2)
        };
    } catch (error) {
        // Tentativa de extrair passos do socket server que não usa JSON puro
        if (line.includes('conectado de')) return { process: 'server', step: 'accepted_connection', details: line.trim() };
        if (line.includes('message extraída:')) return { process: 'server', step: 'message_extracted', details: line.trim() };
        return null;
    }
}

// Função genérica para executar um processo e capturar logs e etapas
function runProcess(file, args = [], options = {}) {
    return new Promise((resolve, reject) => {
        if (!fs.existsSync(file)) {
            return reject(new Error(`Executável não encontrado: ${file}`));
        }

        const logs = [];
        const steps = [];
        const processType = file.includes('client') ? 'client' : 'server';
        
        const proc = spawn(file, args, { cwd: path.dirname(file), ...options });
        logs.push(`[START] ${file} ${args.join(' ')}`);

        const processOutput = (data, type) => {
            const lines = data.toString().split('\n').filter(line => line.trim() !== '');
            lines.forEach(line => {
                const step = parseOutputLine(line, processType);
                if (step) {
                    steps.push(step);
                }
                logs.push(`[${type}] ${line}`);
            });
        };

        proc.stdout.on('data', d => processOutput(d, 'OUT'));
        proc.stderr.on('data', d => processOutput(d, 'ERR'));
        
        proc.on('error', err => {
            logs.push(`[FATAL] Falha ao iniciar o processo: ${err.message}`);
            reject({ logs, steps, error: err });
        });

        proc.on('close', code => {
            logs.push(`[EXIT] ${file} (código ${code})`);
            resolve({ logs, steps, code });
        });
    });
}


app.post('/run', async (req, res) => {
    const { mechanism, message } = req.body;
    const mech = mechanisms[mechanism];

    if (!mech) {
        return res.status(400).json({ error: 'Mecanismo inválido' });
    }

    if (mechanism === 'sockets' || mechanism === 'shared_memory') {
        let serverProc = null;
        const allLogs = [];
        const allSteps = [];

        try {
            if (!fs.existsSync(mech.server)) throw new Error(`Executável do servidor não encontrado: ${mech.server}`);
            
            const serverLogs = new PassThrough();
            serverProc = spawn(mech.server, [], { cwd: path.dirname(mech.server) });
            allLogs.push(`[START] ${mech.server}`);

            const serverReady = new Promise((resolve, reject) => {
                let resolved = false;
                const onData = (data) => {
                    const lines = data.toString().split('\n').filter(line => line.trim() !== '');
                    lines.forEach(line => {
                         allLogs.push(`[OUT] ${line}`);
                         const step = parseOutputLine(line, 'server');
                         if(step) allSteps.push(step);
                    });

                    const isReady = (mechanism === 'sockets' && data.toString().includes('HTTP JSON server em')) ||
                                    (mechanism === 'shared_memory' && data.toString().includes('Servidor iniciado'));
                    
                    // CORREÇÃO 1: Resolve a promise apenas uma vez e NÃO para de ouvir os logs do servidor.
                    if (isReady && !resolved) {
                        resolved = true;
                        resolve();
                    }
                };
                serverProc.on('error', reject);
                serverProc.stdout.on('data', onData);
                serverProc.stderr.on('data', onData); // Também ouvir stderr para logs
            });

            await Promise.race([
                serverReady,
                new Promise((_, reject) => setTimeout(() => reject(new Error('Timeout: Servidor demorou para iniciar.')), 5000))
            ]);
            
            // CORREÇÃO 2: Voltamos a usar a função runProcess para o cliente, que sabe como extrair as etapas.
            const clientResult = await runProcess(mech.client, [message || ""]);
            allLogs.push(...clientResult.logs);
            allSteps.push(...clientResult.steps);

            res.json({ logs: allLogs, steps: allSteps });

        } catch (error) {
            allLogs.push(`[ERRO] ${error.message}`);
            res.status(500).json({ error: 'Falha ao executar o processo', logs: allLogs, steps: allSteps });
        } finally {
            if (serverProc) {
                serverProc.kill('SIGTERM');
                allLogs.push(`[KILL] ${mech.server}`);
            }
        }
        return;
    }

    if (mechanism === 'pipes') {
        // A lógica de Pipes estava correta e permanece a mesma.
        const allLogs = [];
        const allSteps = [];
        try {
            if (!fs.existsSync(mech.client) || !fs.existsSync(mech.server)) {
                throw new Error('Executáveis de cliente ou servidor de pipe não encontrados.');
            }

            const clientProc = spawn(mech.client, [message || ""], { cwd: path.dirname(mech.client) });
            allLogs.push(`[START] ${mech.client} "${message || ""}"`);

            const serverProc = spawn(mech.server, [], { cwd: path.dirname(mech.server) });
            allLogs.push(`[START] ${mech.server}`);

            clientProc.stdout.pipe(serverProc.stdin);
            allLogs.push('[PIPE] Conectado stdout do cliente -> stdin do servidor');

            let finalResponse = '';
            serverProc.stdout.on('data', (data) => {
                finalResponse += data.toString();
            });

            clientProc.stderr.on('data', (data) => allLogs.push(`[CLIENT ERR] ${data}`));
            serverProc.stderr.on('data', (data) => allLogs.push(`[SERVER ERR] ${data}`));

            const clientExit = new Promise(resolve => clientProc.on('close', code => {
                allLogs.push(`[CLIENT EXIT] Código ${code}`);
                resolve();
            }));
            const serverExit = new Promise(resolve => serverProc.on('close', code => {
                allLogs.push(`[SERVER EXIT] Código ${code}`);
                resolve();
            }));

            await Promise.all([clientExit, serverExit]);
            allLogs.push('[PIPE] Pipeline finalizada.');

            const step = parseOutputLine(finalResponse, 'server');
            if (step) {
                allSteps.push(step);
            }
            allLogs.push(`[SERVER OUT] ${finalResponse}`);

            res.json({ logs: allLogs, steps: allSteps });

        } catch (error) {
            allLogs.push(`[ERRO] ${error.message}`);
            res.status(500).json({ error: 'Falha ao executar a pipeline', logs: allLogs, steps: allSteps });
        }
    }
});

app.listen(3000, () => console.log('Servidor rodando em http://localhost:3000'));