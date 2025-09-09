const express = require('express');
const cors = require('cors');
const { spawn } = require('child_process');
const path = require('path');

const app = express();
app.use(cors());
app.use(express.json());

// Apenas Pipes neste protótipo
app.post('/pipes', async (req, res) => {
  const mensagem = (req.body && req.body.mensagem) || '';
  const binPath = path.join(__dirname, '../backend/pipes/servidor_pipe.exe');

  const proc = spawn(binPath, []);
  let stdout = '';
  let stderr = '';
  const etapas = [];

  function maybeParseJson(line) {
    try { return JSON.parse(line); } catch { return null; }
  }
  function pushEtapaFromEvent(obj) {
    if (!obj || !obj.event) return;
    const ev = obj.event;
    if (ev === 'init') etapas.push('Servidor inicializado');
    else if (ev === 'pipes_created') etapas.push('Pipes criados');
    else if (ev === 'client_spawned') etapas.push('Cliente iniciado');
    else if (ev === 'wrote_to_client') etapas.push('Mensagem enviada ao cliente');
    else if (ev === 'read_chunk') etapas.push('Resposta do cliente recebida');
    else if (ev === 'client_output_collected') etapas.push('Saída do cliente agregada');
    else if (ev === 'done') etapas.push('Fluxo concluído');
  }

  proc.stdout.on('data', (d) => {
    const text = d.toString();
    stdout += text;
    text.split(/\r?\n/).forEach(line => {
      if (!line.trim()) return;
      const obj = maybeParseJson(line.trim());
      if (obj) pushEtapaFromEvent(obj);
    });
  });
  proc.stderr.on('data', (d) => { stderr += d.toString(); });
  proc.on('error', (err) => { return res.json({ error: err.message }); });
  proc.on('close', (code) => {
    res.json({ stdout, stderr, code, etapas });
  });
});

app.listen(3000, () => console.log('Server listening on http://localhost:3000'));
