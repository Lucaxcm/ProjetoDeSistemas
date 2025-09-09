const url = 'http://localhost:3000/'
function log(t) {
  const box = document.getElementById('log');
  box.textContent += t + "\n";
  box.scrollTop = box.scrollHeight;
};

document.getElementById('run').onclick = async () => {
  const mensagem = document.getElementById('msg').value || '';
  const mecanismo = document.getElementById('opt').value;


  const res = await fetch(url + mecanismo, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ mensagem })
  });
  
  
  const result = await res.json();
  // Logs
  if (result.stderr) log('STDERR:\n' + result.stderr);
  if (result.stdout) log('STDOUT:\n' + result.stdout);
  // Etapas
  const etapas = document.getElementById('etapas');
  etapas.innerHTML = '';
  const ul = document.createElement('ul');
  (result.etapas || []).forEach(e => {
    const li = document.createElement('li');
    li.textContent = e;
    ul.appendChild(li);
  });
  etapas.appendChild(ul);
};

document.getElementById('stop').onclick = () => {
  fetch(url + 'stop', { method: 'GET' });
};

document.getElementById('logs').onclick = async() => {
  const res = await fetch(url + 'logs', { method: 'GET' });
  const result = await res.json();
  console.log(result);
  try {
    if (result.type === 'progress') {
      document.getElementById('pg').value = result.value || 0;
      document.getElementById('status').textContent = "Status: " + result.status || '';
      document.getElementById('etapas').textContent = result.text || '';
    } else if (result.type === 'log') {
      log(result.text || '');
    } else if (result.type === 'done') {
      document.getElementById('status').textContent = "Status: " + result.result || 'Concluído';
      document.getElementById('etapas').textContent = result.text || 'Finalizado.';
    } else {
      log('retorno: ' + (JSON.stringify(result) || ''));
    }
  } catch (e) {
    log(e + result);
  }

}