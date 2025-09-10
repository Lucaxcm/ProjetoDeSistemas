const runBtn = document.getElementById('runBtn');
const logBox = document.getElementById('logs');
const etapasBox = document.getElementById('etapas');
const mechanismSelect = document.getElementById('mechanism');
const messageInput = document.getElementById('message');

/**
 * Formata um array de objetos de etapa em uma string legível.
 * @param {object[]} steps - O array de etapas.
 * @returns {string} - A string formatada.
 */
function formatSteps(steps) {
    if (!steps || steps.length === 0) {
        return "Nenhuma etapa de comunicação foi identificada.";
    }
    return steps.map((step, index) => {
        return `[${index + 1}] Processo: ${step.process}\n    Etapa: ${step.step}\n    Detalhes: ${step.details}`;
    }).join('\n\n');
}

async function runMechanism() {
    // Desabilita o botão para evitar cliques duplos
    runBtn.disabled = true;
    runBtn.textContent = 'Executando...';

    // Limpa os resultados anteriores
    logBox.textContent = 'Iniciando execução...';
    etapasBox.textContent = 'Aguardando dados do servidor...';

    const mechanism = mechanismSelect.value;
    const message = messageInput.value;

    try {
        const response = await fetch(`http://localhost:3000/run`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ mechanism, message })
        });

        const data = await response.json();

        // Popula os campos com os dados da resposta
        logBox.textContent = data.logs ? data.logs.join('\n') : 'Nenhum log recebido.';
        etapasBox.textContent = formatSteps(data.steps);

        if (!response.ok) {
            logBox.textContent += `\n\nERRO DO SERVIDOR: ${data.error || 'Erro desconhecido.'}`;
        }

    } catch (err) {
        logBox.textContent = "Erro de conexão ao tentar falar com o servidor Node.js.\nVerifique se ele está rodando e tente novamente.\n\nDetalhes: " + err.message;
        etapasBox.textContent = "Falha na comunicação."
    } finally {
        // Reabilita o botão
        runBtn.disabled = false;
        runBtn.textContent = 'Executar';
    }
}

document.addEventListener("DOMContentLoaded", () => {
    runBtn.addEventListener("click", runMechanism);
});