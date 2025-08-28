
(function(){
  const logEl = document.getElementById('log');
  const statusEl = document.getElementById('status');
  const tbody = document.querySelector('#snapshot tbody');
  const btnReload = document.getElementById('btn-reload');
  const btnClear = document.getElementById('btn-clear');

  function appendLog(line){
    const atBottom = (logEl.scrollTop + logEl.clientHeight + 5) >= logEl.scrollHeight;
    logEl.textContent += line + "\n";
    if(atBottom){
      logEl.scrollTop = logEl.scrollHeight;
    }
  }

  function renderSnapshot(snap){
    tbody.innerHTML = '';
    const keys = Object.keys(snap).sort();
    if(keys.length === 0){
      const tr = document.createElement('tr');
      tr.innerHTML = '<td colspan="2" style="opacity:.7">Nicio valoare parse-ată încă...</td>';
      tbody.appendChild(tr);
      return;
    }
    keys.forEach(k => {
      const tr = document.createElement('tr');
      tr.innerHTML = `<td>${k}</td><td>${snap[k]}</td>`;
      tbody.appendChild(tr);
    });
  }

  function connectSSE(){
    const es = new EventSource('/stream');
    es.onopen = () => { statusEl.textContent = 'Status: conectat'; };
    es.onerror = () => { statusEl.textContent = 'Status: eroare/conexiune întreruptă'; };
    es.onmessage = (ev) => {
      try{
        const data = JSON.parse(ev.data);
        if(data.line) appendLog(`[${data.ts}] ${data.line}`);
        if(data.snapshot) renderSnapshot(data.snapshot);
      }catch(e){
        console.error(e);
      }
    };
    return es;
  }

  let es = connectSSE();

  btnReload.addEventListener('click', async () => {
    try{
      const r = await fetch('/api/latest');
      const j = await r.json();
      renderSnapshot(j.snapshot || {});
      if(Array.isArray(j.raw)){
        logEl.textContent = j.raw.map(l => `[${j.time}] ${l}`).join('\n') + '\n';
      }
    }catch(e){
      console.error(e);
    }
  });

  btnClear.addEventListener('click', () => {
    logEl.textContent = '';
  });
})();
