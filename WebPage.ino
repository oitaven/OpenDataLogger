
const char WEB_PAGE[] = R"WEB(
<!DOCTYPE html>
<html lang="pt">
<head>
<meta charset="utf-8">
<meta name="viewport"
 content="width=device-width,initial-scale=1">
<title>OpenDataLogger 1.0.2 · Diagnóstico e análise</title>
<style>
* { box-sizing:border-box }
body {
  margin:0; padding:20px;
  font:14px system-ui,sans-serif;
  background:#101827; color:#e5e7eb;
}
h1 { margin:0 }
h2 { font-size:17px }
small,.muted { color:#94a3b8 }
.panel,.card {
  background:#1e293b;
  border-radius:12px;
  padding:15px;
  margin:12px 0;
}
.row {
  display:flex; flex-wrap:wrap;
  align-items:center; gap:10px;
}
.grid {
  display:grid;
  grid-template-columns:
    repeat(auto-fit,minmax(290px,1fr));
  gap:12px;
}
button,input,select {
  background:#334155;
  color:#f8fafc;
  border:1px solid #475569;
  padding:7px;
  border-radius:6px;
}
button { cursor:pointer }
button:hover { background:#475569 }
input[type=color] {
  width:45px; height:34px;
  padding:2px;
}
input[type=number] { width:95px }
.card canvas {
  width:100%;
  height:240px;
  background:#0f172a;
  border-radius:6px;
}
#charts .card canvas {
  display:block;
  width:100%; height:auto; /* proporcao de cada modo preservada */
}
#charts.wide, #charts.stacked { display:block; }
#charts.stacked .card { margin:9px 0; }
#charts.stacked .card canvas { max-height:300px; }
.hint {color:#94a3b8; font-size:12px; margin:8px 0; }
#triggerState { font-weight:600; }
.capture-meta {
  color:#a5b4c8; font-size:12px; line-height:1.55;
  display:flex; flex-wrap:wrap; gap:5px 13px; margin:3px 0 12px;
}
.capture-meta span {white-space:nowrap}

@media(max-width:650px) { body {padding:10px} .panel,.card {padding:11px} }

.item {
  border-bottom:1px solid #334155;
  padding:12px 0;
}
.item:last-child { border:0 }
.legend {
  display:flex; flex-wrap:wrap;
  gap:12px; margin:10px 0;
}
.dot {
  display:inline-block;
  width:10px; height:10px;
  border-radius:50%;
  margin-right:5px;
}
.tabs {
  display:flex;
  gap:8px;
  flex-wrap:wrap;
  margin:18px 0;
}
.tabs button.active {
  background:#2563eb;
  border-color:#60a5fa;
}
.tab { display:none; }
.tab.active { display:block; }
#clock {
  margin-top:8px;
  font-size:13px;
  color:#93c5fd;
}
#message { color:#93c5fd }
.mode-controls { display:flex;align-items:center;gap:9px;flex-wrap:wrap;margin:10px 0; }
.mode-button { min-width:130px; font-weight:700; background:#26344a; }
.mode-button.active { background:#1d4ed8; border-color:#93c5fd; color:#fff; }
.mode-button.active.real { background:#166534; border-color:#86efac; }
.mode-status { color:#cbd5e1; font-size:12px; }
.session-actions button.active { background:#166534; border-color:#86efac; font-weight:700; }
.session-actions button.selected-stop { background:#334155; border-color:#93c5fd; font-weight:700; }
.state-pill { display:inline-block; padding:5px 9px; border-radius:8px; background:#334155; border:1px solid #475569; font-weight:600; }
.state-pill.recording { background:#14532d; border-color:#4ade80; }
#wifiForm .wifi-grid { display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,260px),1fr));gap:12px; }
#wifiForm label { display:flex;flex-direction:column;gap:5px;min-width:0;overflow-wrap:anywhere; }
#wifiForm label input { display:block;width:100%;min-width:0;max-width:100%; }
#wifiForm .wifi-actions { display:flex;align-items:end;flex-wrap:wrap;gap:12px;margin-top:12px; }
#wifiForm .wifi-actions label { flex:1 1 250px;max-width:420px; }
#wifiForm .wifi-actions button {flex:0 0 auto;}
.analysis-table { width:max-content;max-width:100%;border-collapse:collapse;margin-top:9px; }
.analysis-table td { border-bottom:1px solid #334155;padding:5px 13px 5px 0;vertical-align:top; }
.analysis-table td:last-child {padding-left:14px; font-variant-numeric:tabular-nums;white-space:nowrap;text-align:right;}
.analysis-table-container {overflow-x:auto;}
</style>
</head>
<body>
<header>
<h1>OpenDataLogger</h1>
<small>UNO R4 WiFi · v1.0.3 experimental · Made by Ricardo Oitavén &amp; Eyla IA</small>
<div id="clock"></div>
<p id="message">A estabelecer comunicação...</p>
</header>

<nav class="tabs">
  <button class="active" onclick="showTab('monitor')">
    Monitorização
  </button>
  <button onclick="showTab('channels')">
    Canais
  </button>
  <button onclick="showTab('analysis')">Análise</button>
  <button onclick="showTab('sessions')">Sessões</button>
  <button onclick="showTab('system')">Sistema</button>
</nav>

<section id="tab-monitor" class="tab active">

<div class="panel">
 <h2>Aquisição</h2>
 <div id="storage" class="muted"></div>
 <div id="liveLatency" class="hint"></div>
 <div id="digitalEdgeInfo" class="hint">Transições D2/D3: a aguardar…</div>
 <div class="mode-controls" role="group" aria-label="Fonte da aquisição">
  <button type="button" id="realMode" class="mode-button real" onclick="setOperationMode(false)">AQUISIÇÃO REAL</button>
  <button type="button" id="demoMode" class="mode-button" onclick="setOperationMode(true)">DEMO</button>
  <span id="modeStatus" class="mode-status">A verificar modo…</span>
 </div>
 <p class="hint">DEMO: A0 triangular, A1 sinusoidal, D2 onda quadrada e D3 impulsos breves. AQUISIÇÃO REAL: lê os pinos físicos (entradas desligadas podem mostrar ruído). Alterar o modo reinicia o histórico em RAM; não interrompe sessões já exportadas.</p>
 <div class="row">
  <label>Intervalo
   <select id="rate" onchange="changeRate()">
    <option value="100">100 ms</option>
    <option value="250">250 ms</option>
    <option value="500">500 ms</option>
    <option value="1000">1 s</option>
    <option value="2000">2 s</option>
    <option value="5000">5 s</option>
    <option value="10000" selected>10 s</option>
   </select>
  </label>
  <button onclick="exportCSV()">CSV</button>
  <button onclick="exportDigitalEdgesCSV()">CSV impulsos</button>
  <button onclick="saveSettings()">Guardar configuração</button>
 </div>
</div>

<div class="panel">
 <h2>Disparo / Trigger</h2>
 <div class="row">
  <label>Canal <select id="triggerChannel" onchange="refreshTriggerOptions()"></select></label>
  <label>Condição <select id="triggerCondition"></select></label>
  <label>Limiar <input type="number" id="triggerLevel" value="2.5" step="any"></label>
  <label>Pré-trigger (ms) <input type="number" id="triggerPre" value="5000" min="100" max="30000"></label>
  <label>Pós-trigger (ms) <input type="number" id="triggerPost" value="5000" min="100" max="30000"></label>
 </div>
 <div class="row">
  <button onclick="armCapture()">Armar disparo</button>
  <button onclick="continuousCapture()">Contínuo</button>
  <button onclick="stopCapture()">Parar / congelar</button>
  <span id="triggerState">Contínuo</span>
 </div>
 <p class="hint">Flancos digitais D2/D3: detectados por interrupção para o trigger. As curvas analógicas são amostradas ao intervalo escolhido; a captura não equivale a um osciloscópio de alta velocidade.</p>
</div>

<div class="panel">
 <h2>Visualização</h2>
 <div class="row">
  <label>Apresentação
   <select id="view" onchange="drawAll()">
    <option value="grid">Quadros adaptativos</option>
    <option value="stack">Quadros verticais (tempo alinhado)</option>
    <option value="single">Um canal</option>
    <option value="overlay">Todos sobrepostos · normalizado</option>
    <option value="lanes" selected>Gráfico único · faixas independentes</option>
   </select>
  </label>
  <label>Canal
   <select id="focus" onchange="focusChanged()"></select>
  </label>
  <label>Canal na matriz LED
  <select id="matrixSelect" onchange="changeMatrix()"></select>
  </label>
  <label>Janela da matriz
   <select id="matrixWindow" onchange="changeMatrixWindow()">
    <option value="1000">1 segundo</option>
    <option value="3000">3 segundos</option>
    <option value="10000" selected>10 segundos</option>
    <option value="30000">30 segundos</option>
   </select>
  </label>
  <label>Janela
   <select id="window" onchange="offset=0;drawAll()">
    <option value="0" selected>Automática · memória disponível</option>
    <option value="30000">30 segundos</option>
    <option value="120000">2 minutos</option>
    <option value="300000">5 minutos</option>
    <option value="900000">15 minutos</option>
    <option value="3600000">1 hora</option>
   </select>
  </label>
 </div>
 <div class="row">
  <button onclick="move(1)">◀ Recuar</button>
  <button onclick="move(-1)">Avançar ▶</button>
  <button onclick="offset=0;drawAll()">Agora</button>
 </div>
 <div class="hint" id="windowInfo"></div>
 <div class="row"><button onclick="clearCursors()">Limpar cursores</button>
 <span class="muted">Clica duas vezes nos gráficos para medir Δt e Δvalor.</span></div>
 <div class="hint" id="cursorInfo">Cursores: selecciona dois instantes no gráfico.</div>
</div>

<div id="charts" class="grid"></div>
</section>
<section id="tab-channels" class="tab">
<div class="panel">
 <h2>Configuração dos canais</h2>
 <p class="muted">
  As alterações são aplicadas ao sair de Canais.
  Apenas a alteração do tipo de entrada reinicia o histórico.
  Use Guardar configuração para conservar as definições após reiniciar.
 </p>
 <div class="row"><button type="button" onclick="resetGenericNames()">Repor nomes genéricos</button>
 <small class="muted">Não altera o tipo nem a calibração. Aplicar ao sair de Canais; guardar configuração para persistir.</small></div>
 <div id="settings"></div>
</div>

</section>
<section id="tab-analysis" class="tab">
<div class="panel"><h2>Canais calculados</h2>
 <p class="hint">Os resultados partilham o tempo dos canais físicos. São calculados no Safari, sem interferir na aquisição.</p>
 <div id="virtualSettings"></div><button onclick="saveVirtuals()">Aplicar canais calculados</button></div>
<div class="panel"><h2>Alarmes e condições compostas</h2>
 <p class="hint">Até quatro regras avaliadas pelo Arduino, mesmo sem navegador. Podem apenas registar ou disparar uma captura armada.</p>
 <div id="alarmSettings"></div><button onclick="saveAlarms()">Aplicar alarmes</button>
 <button onclick="loadAlarmLog()">Actualizar ocorrências</button>
 <div id="alarmHistory" class="hint"></div></div>
<div class="panel"><h2>Calibração NTC multiponto</h2>
 <p class="hint"><b>Como usar:</b> (1) em Canais, selecciona «NTC / temperatura» e define R fixa, R25 e Beta; (2) muda para AQUISIÇÃO REAL; (3) coloca a sonda junto de um termómetro de referência e espera estabilizar; (4) introduz a temperatura real em °C e clica em «Guardar ponto da leitura actual»; (5) repete noutras temperaturas, até três pontos, e guarda a configuração na EEPROM. Com um ponto corrigimos o desvio; com dois interpolamos; com três usamos Steinhart–Hart. Para uma correcção rápida de um ponto também existe «Calibrar NTC» em Canais.</p>
 <div id="calibrationSettings"></div></div>
<div class="panel"><h2>Análise de ciclos e extremos por canal</h2>
 <p class="hint">O relatório é calculado quando pedes e não suspende a aquisição. Para começar ou terminar uma análise prolongada, grava uma sessão no Safari, no separador Sessões. A análise utiliza a sessão disponível, ou o histórico RAM actual quando não existe sessão.</p>
 <button onclick="showTab('sessions')">Gerir sessão / período de análise</button>
 <button onclick="renderCycleAnalysis()">Calcular relatório</button>
 <button onclick="exportAnalysisCSV()">Exportar relatório CSV</button>
 <div id="cycleAnalysis" class="hint"></div></div>
</section>
<section id="tab-sessions" class="tab">
<div class="panel"><h2>Sessão no navegador</h2>
 <p class="hint">Enquanto esta página estiver aberta, a sessão pode acumular mais dados do que a RAM do Arduino. Para não a perder, exporta-a antes de fechar o Safari.</p>
 <div class="row"><label>Nome <input id="sessionName" maxlength="60" value="Experiência 1"></label>
 <span class="session-actions"><button id="sessionStart" onclick="startBrowserSession()">Iniciar sessão</button>
 <button id="sessionStop" onclick="stopBrowserSession()">Parar sessão</button></span>
 <span id="sessionState" class="state-pill">Inactiva</span>
 <button onclick="exportSessionJSON()">Exportar sessão JSON</button>
 <button onclick="exportSessionCSV()">Exportar sessão CSV</button></div>
 <div id="sessionInfo" class="hint"></div></div>
<div class="panel"><h2>Importar e comparar sessões</h2>
 <div class="row"><input id="sessionFile" type="file" accept=".json,.csv,application/json,text/csv" onchange="importSessionFile(event)">
 <label>Canal <select id="compareChannel"></select></label>
 <label>Alinhamento <select id="compareAlign" onchange="drawSessionComparison()"><option value="start">Início da sessão</option><option value="first-edge">Primeiro flanco digital de subida</option></select></label>
 <button onclick="drawSessionComparison()">Comparar</button></div>
 <div id="importInfo" class="hint">Importa uma sessão JSON exportada pelo Safari ou um CSV gravado no microSD.</div>
 <canvas id="comparisonCanvas" width="1000" height="360" style="width:100%;background:#0f172a;border-radius:8px"></canvas>
 <h3>Reprodução da sessão importada</h3>
 <div class="row"><button onclick="togglePlayback()" id="playButton">Reproduzir</button>
 <label>Posição <input id="playPosition" type="range" min="0" max="1000" value="1000" style="width:min(400px,50vw)" oninput="seekPlayback()"></label>
 <label>Velocidade <select id="playSpeed"><option value="1">1×</option><option value="10" selected>10×</option><option value="60">60×</option></select></label></div>
 <div id="playbackInfo" class="hint"></div>
 <canvas id="replayCanvas" width="1000" height="460" style="width:100%;background:#0f172a;border-radius:8px"></canvas>
</div>
<div class="panel"><h2>Cartão microSD (opcional)</h2>
 <p class="hint">O Arduino procura um cartão no arranque. Se o encontrar, inicia uma sessão CSV automaticamente. Sem cartão, mantém a gravação no Safari e na RAM. A escrita SD decorre fora das interrupções.</p>
 <p class="hint"><b>Preparação:</b> cartão microSD em FAT16/FAT32 (preferencialmente até 32 GB); no macOS, Utilitário de Discos → Apagar → «MS-DOS (FAT)» e esquema «Master Boot Record». Faz cópia antes de formatar. Não existe botão de formatação remoto nesta versão: seria uma operação destrutiva e não é suportada directamente pela biblioteca SD utilizada.</p>
 <div class="row"><button onclick="sdCommand('start')">Iniciar gravação</button>
 <button onclick="sdCommand('stop')">Parar gravação</button>
 <button onclick="refreshSdFiles()">Listar ficheiros</button></div>
 <div id="sdInfo" class="hint"></div><div id="sdFiles"></div></div>
</section>
<section id="tab-system" class="tab">
<div class="panel"><h2>Capacidade e autonomia do histórico</h2>
 <div id="memoryCapacity" class="muted">A aguardar a configuração dos canais…</div>
 <p class="hint">A capacidade é calculada pelos canais físicos activos e pelo intervalo de aquisição. Refere-se ao histórico circular em RAM; os eventos digitais têm memória separada. Não corresponde à duração de uma sessão gravada no navegador ou no microSD.</p>
</div>
<div class="panel"><h2>Rede e relógio</h2><div id="network" class="muted"></div></div>
<div class="panel"><h2>Desempenho e latência</h2><div id="diagnostics" class="muted">A aguardar medições…</div></div>
<div class="panel"><h2>Configuração Wi-Fi pela Web</h2>
 <p class="hint">Configuração protegida pela macro <code>ODL_WEB_ADMIN_KEY</code> definida em <code>arduino_secrets.h</code>. As palavras-passe não são devolvidas pelo Arduino. A alteração exige reinício. Usar apenas numa rede de confiança: o servidor HTTP não usa TLS.</p>
 <form id="wifiForm" onsubmit="submitWifi(event)">
 <div class="wifi-grid"><label>Rede principal (SSID)<input id="ssid0" maxlength="32" autocomplete="off"></label>
 <label>Palavra-passe da rede principal<input id="pass0" type="password" maxlength="64" placeholder="Deixar vazio para manter" autocomplete="new-password"></label>
 <label>Rede secundária (SSID)<input id="ssid1" maxlength="32" autocomplete="off"></label>
 <label>Palavra-passe da rede secundária<input id="pass1" type="password" maxlength="64" placeholder="Deixar vazio para manter" autocomplete="new-password"></label>
 <label>Access Point (SSID)<input id="apName" maxlength="32"></label>
 <label>Palavra-passe do AP<input id="apPassword" type="password" maxlength="64" placeholder="Deixar vazio para manter" autocomplete="new-password"></label></div>
 <div class="wifi-actions"><label>Código de administração<input id="adminKey" type="password" autocomplete="off" placeholder="Definido no firmware"></label>
 <button type="submit">Guardar Wi-Fi na EEPROM</button></div>
 <div id="wifiResult" class="hint"></div>
 </form></div>
</section>

<script>
// OpenDataLogger — Made by Ricardo Oitavén & Eyla IA
const types = [
 "Desactivado",
 "Tensão",
 "NTC / temperatura",
 "Corrente",
 "Potência",
 "Personalizado",
 "Digital"
];

const units = ["","V","°C","A","W","u","","u"];

let statusData=null;
let data={ids:[],rows:[]};
let digitalEvents=[]; // {seq, t, us, id, level}, independentes da taxa ADC
let lastEventSequence=0;
let digitalEventGap=false;
let eventsOverwritten=0;
let offset=0;
let lastDiagnosticsFetch=0;
let lastApiRoundtripMs=0;
let layoutSignature='';
let currentGroups=[];

const $=id=>document.getElementById(id);

function esc(s) {
 return String(s).replace(/[&<>"']/g,c=>({
  "&":"&amp;","<":"&lt;",">":"&gt;",
  '"':"&quot;","'":"&#39;"
 }[c]));
}

async function api(path) {
 const response=await fetch(path,{cache:"no-store"});
 if(!response.ok) throw Error("HTTP "+response.status);
 return response.json();
}

function physicalChannels(){return (statusData?.channels||[]).filter(c=>c.type!==0);}
function activeChannels() {
 const original=physicalChannels();
 const virtual=(statusData?.virtuals||[]).filter(v=>v.mode>0).map(v=>({
   ...v, type:7,auto:1,min:0,max:1, gain:1,offset:0
 }));
 return original.concat(virtual);
}

function unit(c) {
 if(c.type===7){
  const left=statusData?.channels?.[c.a],right=statusData?.channels?.[c.b];
  const u1=left?units[left.type]||'':'',u2=right?units[right.type]||'':'';
  if(c.mode===3)return '';
  if(c.mode===4)return (u1||'u')+'/s';
  if(c.mode===5)return u1;
  return u1===u2?u1:'u';
 }
 return units[c.type] || "";
}

function convert(c,raw) {
 if(raw===null||raw===undefined)return NaN;
 if(c.type===7)return raw;
 if(c.type===6) return raw ? 1 : 0;

 const v=raw*5/4095;

 if(c.type===2) {
  if(raw<=0 || raw>=4095) return NaN;

  const resistance=
   c.rfixed*raw/(4095-raw);

  const baseFromRaw = r => {
    const resistance=c.rfixed*r/(4095-r);
    return 1/(1/298.15+Math.log(resistance/c.r25)/c.beta)-273.15;
  };
  const calibration=statusData?.calibration?.[c.id];
  if(calibration?.sh?.length===3){
    const L=Math.log(resistance),[a,b,d]=calibration.sh;
    const inv=a+b*L+d*L*L*L;
    if(inv>0 && Number.isFinite(inv))return 1/inv-273.15+c.trim;
  }
  const base=baseFromRaw(raw);
  const pts=calibration?.points||[];
  if(pts.length===1)return base+pts[0][1]-baseFromRaw(pts[0][0])+c.trim;
  if(pts.length===2){
    const sorted=pts.map(p=>[baseFromRaw(p[0]),p[1]-baseFromRaw(p[0])]).sort((a,b)=>a[0]-b[0]);
    const amount=sorted[1][0]===sorted[0][0]?0:Math.max(0,Math.min(1,(base-sorted[0][0])/(sorted[1][0]-sorted[0][0])));
    return base+sorted[0][1]+(sorted[1][1]-sorted[0][1])*amount+c.trim;
  }
  return base+c.trim;
 }

 return v*c.gain+c.offset;
}

function valueAt(row,id) {
 if(id>=8){
   const v=statusData?.virtuals?.[id-8];
   if(!v||!v.mode)return null;
   const ca=statusData.channels[v.a],cb=statusData.channels[v.b];
   const rawA=valueAt(row,v.a),rawB=valueAt(row,v.b);
   if(rawA===null||!ca)return null;
   const a=convert(ca,rawA),b=rawB===null||!cb?NaN:convert(cb,rawB);
   if(v.mode===1)return a-b;
   if(v.mode===2)return a+b;
   if(v.mode===3)return b===0?NaN:a/b;
   const idx=data.rows.indexOf(row);
   if(v.mode===4){
     if(idx<1)return NaN;
     const prev=data.rows[idx-1],prevRaw=valueAt(prev,v.a);
     if(prevRaw===null)return NaN;
     const secs=(row[0]-prev[0])/1000;
     return secs>0?(a-convert(ca,prevRaw))/secs:NaN;
   }
   if(v.mode===5){
     if(idx<0)return a;
     let sum=0,count=0;
     for(let j=Math.max(0,idx-4);j<=idx;j++){
       const rr=valueAt(data.rows[j],v.a);
       if(rr!==null){sum+=convert(ca,rr);count++;}
     }
     return count?sum/count:NaN;
   }
   return NaN;
 }
 const pos=data.ids.indexOf(id);
 return pos<0 ? null : row[pos+1];
}

function fmt(v,unitName) {
 if(!Number.isFinite(v)) return "--";
 return v.toFixed(unitName==="°C"?2:3)+
  (unitName?" "+unitName:"");
}

function displayClock(t) {
 if(!statusData.epoch) return "T+"+(t/1000).toFixed(1)+"s";

 const epochMs=
  statusData.epoch*1000+
  (t-statusData.clockMillis);

 return new Date(epochMs).toLocaleString("pt-PT");
}

// Formata o eixo X com a data/hora local obtida do relogio do logger.
// Sem sincronizacao temporal, apresenta apenas o tempo de funcionamento.
function axisTime(timestamp, spanMs) {
  if (!statusData || !statusData.epoch) {
    return "T+" + (timestamp / 1000).toFixed(0) + " s";
  }

  const utcMs = statusData.epoch * 1000 +
    (timestamp - statusData.clockMillis);
  const dt = new Date(utcMs);
  if (!Number.isFinite(dt.getTime())) return "Sem hora";

  if (spanMs >= 3600000) {
    return dt.toLocaleString("pt-PT", {
      day: "2-digit", month: "2-digit",
      hour: "2-digit", minute: "2-digit"
    });
  }
  if (offset > 0) {
    return dt.toLocaleString("pt-PT", {
      day: "2-digit", month: "2-digit",
      hour: "2-digit", minute: "2-digit", second: "2-digit"
    });
  }
  return dt.toLocaleTimeString("pt-PT", {
    hour: "2-digit", minute: "2-digit", second: "2-digit"
  });
}

// Se o periodo pedido exceder o historico, usamos a memoria real.
// Evita a grande area vazia quando existem 34 s mas a janela pede 2 min.
function plotWindowMs() {
  const requested=Number($('window').value);
  if (!data.rows.length) return requested || 30000;
  const first=data.rows[0][0], last=data.rows[data.rows.length-1][0];
  const lastEdge=digitalEvents.length?digitalEvents[digitalEvents.length-1].t:last;
  const span=Math.max(0,Math.max(last,lastEdge)-first);
  const minimum=Math.max(250,Number(statusData?.sampleMs)||1000);
  const available=Math.max(span,minimum);
  return requested===0 ? available : Math.min(requested,available);
}
function updateWindowInfo(){
 const el=$('windowInfo');if(!el)return;
 const ms=plotWindowMs();
 const chosen=Number($('window').value);
 el.textContent=chosen && ms<chosen
   ? 'Janela ajustada à memória disponível: '+formatDuration(ms)+
     ' (período escolhido: '+formatDuration(chosen)+')'
   : 'Janela visível: '+formatDuration(ms)+
     ' · as leituras antigas são substituídas quando a RAM enche';
}

// A mesma janela temporal em todos os modos; bandas independentes
// para comparar tensoes, temperaturas e sinais ON/OFF sem misturar unidades.
function drawPlot(canvas, selected, mode) {
 const ctx=canvas.getContext('2d');
 const allInOne = mode==='overlay' || mode==='lanes';
 const wide = mode!=='grid';
 const W=wide?1120:690;
 const H=mode==='lanes'
    ?Math.max(360,selected.length*144+65)
    :(mode==='overlay'||mode==='single'?490:286);
 if(canvas.width!==W)canvas.width=W;
 if(canvas.height!==H)canvas.height=H;
 const L=92,R=19,T=29,B=51;
 const chartW=W-L-R, chartH=H-T-B;
 const lastSample=data.rows.length?data.rows[data.rows.length-1][0]:0;
 const lastEdge=digitalEvents.length?digitalEvents[digitalEvents.length-1].t:0;
 const isFrozen=statusData?.trigger?.state===3;
 const currentEnd=(isFrozen
   ? Math.max(lastSample,lastEdge)
   : (Number.isFinite(deviceNowMs) && deviceNowMs>0
      ? Math.max(deviceNowMs,lastSample,lastEdge)
      : Math.max(lastSample,lastEdge)))-offset;
 const windowMs=plotWindowMs();
 const startTime=currentEnd-windowMs;
 const visible=data.rows.filter(r=>r[0]>=startTime&&r[0]<=currentEnd);
 const getX=t=>L+(t-startTime)/windowMs*chartW;
 canvas._plot={L,right:W-R,startTime,windowMs};
 ctx.clearRect(0,0,W,H);
 ctx.font='12px system-ui';
 ctx.textBaseline='middle';
 const rows=visible;
 const hasVisibleEdges=digitalEvents.some(e=>e.t>=startTime&&e.t<=currentEnd);
 if(!rows.length&&!hasVisibleEdges){
  ctx.fillStyle='#94a3b8';
  ctx.fillText('Sem dados neste intervalo',L+10,55);
  return;
 }

 function getRange(c){
  if(c.type===6)return [0,1];
  if(!c.auto && c.max>c.min)return [c.min,c.max];
  const vals=rows.map(r=>convert(c,valueAt(r,c.id))).filter(Number.isFinite);
  if(!vals.length)return [0,1];
  const lo=Math.min(...vals),hi=Math.max(...vals);
  const pad=Math.max((hi-lo)*0.1,0.01);
  return [lo-pad,hi+pad];
 }
 const ranges=new Map(selected.map(c=>[c.id,getRange(c)]));
 const yFor=(c,value,top,height)=>{
   const [lo,hi]=ranges.get(c.id);
   return top+height-(value-lo)/(hi-lo)*height;
 };
 const label=(text,x,y,align='left')=>{
  ctx.fillStyle='#a9b6ca';ctx.textAlign=align;ctx.fillText(text,x,y);
 };
 // shared X grid and date labels
 ctx.lineWidth=1;
 for(let k=0;k<=4;k++){
   const t=startTime+(windowMs*k/4),x=getX(t);
   ctx.strokeStyle='#334155';ctx.setLineDash([3,5]);
   ctx.beginPath();ctx.moveTo(x,T);ctx.lineTo(x,H-B);ctx.stroke();
   ctx.setLineDash([]);
   label(axisTime(t,windowMs),x,H-21,k===0?'left':(k===4?'right':'center'));
 }
 ctx.textAlign='left';
 if(mode==='lanes'){
   const laneH=chartH/selected.length;
   selected.forEach((c,i)=>{
     const top=T+i*laneH+12,bottom=T+(i+1)*laneH-14,height=bottom-top;
     const [lo,hi]=ranges.get(c.id);
     ctx.strokeStyle='#354356';ctx.beginPath();ctx.moveTo(L,top);ctx.lineTo(W-R,top);ctx.moveTo(L,bottom);ctx.lineTo(W-R,bottom);ctx.stroke();
     label(c.name,L+5,top-8);
     label((c.type===6?'ON':fmt(hi,unit(c))),L-7,top,'right');
     label((c.type===6?'OFF':fmt(lo,unit(c))),L-7,bottom,'right');
     plotTrace(c,top,height);
   });
 }else{
   for(let k=0;k<=4;k++){
     const yy=T+chartH*k/4;
     ctx.strokeStyle='#344155';ctx.beginPath();ctx.moveTo(L,yy);ctx.lineTo(W-R,yy);ctx.stroke();
     if(mode==='overlay')label((100-k*25)+'%',L-12,yy,'right');
     else if(selected.length){
       const c=selected[0],[lo,hi]=ranges.get(c.id);
       label((hi-(hi-lo)*k/4).toFixed(c.type===6?0:2),L-12,yy,'right');
     }
   }
   selected.forEach(c=>plotTrace(c,T,chartH));
 }

 function plotTrace(c,top,height){
  // Os digitais usam os FLANCOS captados pelas interrupcoes, nao apenas
  // o estado ADC/GPIO observado nas amostras periodicas.
  if(c.type===6){
   const events=digitalEvents.filter(e=>e.id===c.id &&
     e.t>=startTime && e.t<=currentEnd);
   let before=null;
   for(const e of digitalEvents){
     if(e.id===c.id && e.t<startTime)before=e;
   }
   if(before||events.length){
     // Se nao existia flanco anterior conhecido, usamos a primeira
     // leitura como referencia aproximada (nao inventamos impulsos).
     let level=before?before.level:0;
     if(!before){
       // A referencia inicial e o complemento do primeiro flanco
       // conhecido na janela, tal como na matriz LED. Uma leitura
       // periodica antiga poderia mostrar um sinal digital desfasado.
       if(events.length)level=events[0].level?0:1;
       else {
         const firstSample=rows.find(r=>valueAt(r,c.id)!==null);
         if(firstSample)level=valueAt(firstSample,c.id)?1:0;
       }
     }
     const yLevel=l=>yFor(c,l,top,height);
     ctx.strokeStyle=c.color;
     ctx.lineWidth=2.7;
     ctx.beginPath();
     ctx.moveTo(L,yLevel(level));
     for(const e of events){
       const x=getX(e.t);
       ctx.lineTo(x,yLevel(level));
       ctx.lineTo(x,yLevel(e.level));
       level=e.level;
     }
     ctx.lineTo(getX(currentEnd),yLevel(level));
     ctx.stroke();
     ctx.lineWidth=1;
     return;
   }
  }
  // Compatibilidade: se ainda nao ha eventos, usa amostras periodicas.
  ctx.beginPath();ctx.strokeStyle=c.color;ctx.lineWidth=2.7;
  let started=false,prev=null;
  for(const r of rows){
    const raw=valueAt(r,c.id);
    if(raw===null)continue;
    const val=convert(c,raw);
    if(!Number.isFinite(val)){started=false;prev=null;continue;}
    const x=getX(r[0]);
    const y=yFor(c,val,top,height);
    if(!started){ctx.moveTo(x,y);started=true;}
    else{
      if(c.type===6 && prev!==null)ctx.lineTo(x,yFor(c,prev,top,height));
      ctx.lineTo(x,y);
    }
    prev=val;
  }
  ctx.stroke();ctx.lineWidth=1;
 }

 if(cursorOne!==null||cursorTwo!==null){
   ctx.save();ctx.setLineDash([6,4]);ctx.lineWidth=1;
   for(const [xTime,color] of [[cursorOne,'#facc15'],[cursorTwo,'#f472b6']]){
     if(xTime===null)continue;
     const x=getX(xTime);if(x<L||x>W-R)continue;
     ctx.strokeStyle=color;ctx.beginPath();ctx.moveTo(x,T);ctx.lineTo(x,H-B);ctx.stroke();
   }
   ctx.restore();
 }
 ctx.textAlign='left';
}

// Cabecalho compacto para documentar capturas de ecrã.
// Nao implica pedidos HTTP adicionais nem processamento no Arduino.
function samplingLabel(ms) {
 const v=Number(ms)||0;
 return v>=1000 ? (v/1000).toLocaleString('pt-PT')+' s' : v+' ms';
}
function captureMetadata(){
 const t=statusData?.trigger||{state:0};
 const states=['Contínuo','Armado','Pós-trigger','Congelado'];
 let trig=states[t.state]||'—';
 if(t.state!==0){
   const c=statusData.channels.find(c=>c.id===Number(t.channel));
   const conditions={1:'subida',2:'descida',3:'acima do limiar',4:'abaixo do limiar'};
   trig+=' · '+(c?.name||'CH'+t.channel)+' ('+(conditions[t.condition]||'?')+')';
 }
 const latest=data.rows.length?data.rows[data.rows.length-1][0]:null;
 const parts=[
   'Aquisição: '+samplingLabel(statusData.sampleMs),
   'Modo: '+trig,
   'Janela: '+formatDuration(plotWindowMs()),
   statusData.simulation?'Simulação':'Entradas reais'
 ];
 if(latest!==null)parts.push('Última: '+displayClock(latest));
 return parts;
}
function chartCard(title,showMetadata) {
 const card=document.createElement('div');
 card.className='card';
 const heading=document.createElement('h3');
 heading.textContent=title;
 card.append(heading);
 if(showMetadata){
   const meta=document.createElement('div');
   meta.className='capture-meta';
   meta.setAttribute('aria-label','Condições de aquisição');
   card.append(meta);
 }
 const canvas=document.createElement('canvas');
 canvas.addEventListener('click',chartCursorClick);
 card.append(canvas);
 return {card,canvas};
}
function updateChartMetadata(card){
 const el=card.querySelector('.capture-meta');
 if(!el)return;
 const parts=captureMetadata();
 // Renderizar apenas se o texto mudar.
 const text=parts.join(' | ');
 if(el.dataset.summary===text)return;
 el.dataset.summary=text;
 el.replaceChildren(...parts.map(part=>{
   const span=document.createElement('span');
   span.textContent=part;
   return span;
 }));
}

function drawAll() {
 if(!statusData)return;
 updateWindowInfo();
 // Evita desenhar um separador que nao esta visivel.
 if(currentTab!=='monitor')return;
 const root=$('charts');
 const mode=$('view').value;
 const all=activeChannels();
 const focus=Number($('focus').value);
 let groups=[];
 if(mode==='single'){
   const c=all.find(c=>c.id===focus)||all[0];if(c)groups=[[c]];
 }else if(mode==='overlay'||mode==='lanes'){
   if(all.length)groups=[all];
 }else groups=all.map(c=>[c]);

 // So refaz HTML quando a configuracao visual mudar.
 const signature=mode+'|'+focus+'|'+all.map(c=>
   [c.id,c.type,c.name,c.color].join(':')).join('|');
 if(signature!==layoutSignature || root.children.length!==groups.length){
   root.replaceChildren();
   root.className=mode==='grid'?'grid':(mode==='stack'?'stacked':'wide');
   root.style.gridTemplateColumns=mode==='grid'
     ?'repeat(auto-fit,minmax(290px,1fr))':'1fr';
   groups.forEach(group=>{
     const title=mode==='overlay'?'Canais sobrepostos · 0–100%':
       mode==='lanes'?'Gráfico único · faixas por canal':
       group[0].name+' ('+unit(group[0])+')';
     const {card,canvas}=chartCard(title,mode!=='grid');
     const legend=document.createElement('div');legend.className='legend';
     card.append(legend);root.append(card);
   });
   layoutSignature=signature;
 }
 groups.forEach((group,index)=>{
   const card=root.children[index];
   updateChartMetadata(card);
   const legend=card.querySelector('.legend');
   const html=group.map(c=>{
     const raw=data.rows.length?valueAt(data.rows[data.rows.length-1],c.id):null;
     const val=raw===null?NaN:convert(c,raw);
     return `<span><span class="dot" style="background:${c.color}"></span>`+
       esc(c.name)+': '+esc(fmt(val,unit(c)))+'</span>';
   }).join('');
   if(legend.innerHTML!==html)legend.innerHTML=html;
   drawPlot(card.querySelector('canvas'),group,mode);
 });
}

function refreshTriggerOptions(){
 const id=Number($('triggerChannel').value);
 const c=statusData?.channels.find(c=>c.id===id);
 const digital=c?.type===6;
 const select=$('triggerCondition');
 const old=select.value;
 select.innerHTML=digital
  ?'<option value="1">Passa a ON</option><option value="2">Passa a OFF</option>'
  :'<option value="3">Sobe acima do limiar</option><option value="4">Desce abaixo do limiar</option>';
 if([...select.options].some(o=>o.value===old))select.value=old;
 $('triggerLevel').disabled=digital;
}
function triggerLabel(t){
 if(!t)return 'Contínuo';
 const base=['Contínuo','Armado · à espera de evento','Disparado · pós-captura','Congelado'][t.state]||'—';
 return base+(t.momentMs?' · '+axisTime(t.momentMs,120000):'');
}
async function triggerAction(action){
 try{
   const p=new URLSearchParams({action});
   if(action==='arm'){
     p.set('id',$('triggerChannel').value);
     p.set('condition',$('triggerCondition').value);
     p.set('level',$('triggerLevel').value);
     p.set('pre',$('triggerPre').value);
     p.set('post',$('triggerPost').value);
   }
   const response=await api('/api/trigger?'+p);
   statusData.trigger=response.trigger;
   if(action==='arm'){data.rows=[];offset=0;}
   $('triggerState').textContent=triggerLabel(statusData.trigger);
   drawAll();
 }catch(e){$('message').textContent='Trigger: '+e.message;}
}
function armCapture(){return triggerAction('arm');}
function continuousCapture(){return triggerAction('continuous');}
function stopCapture(){return triggerAction('stop');}

function focusChanged() {
  drawAll();
}

async function changeMatrixWindow() {
 try {
   const v=Number($('matrixWindow').value);
   const response=await api('/api/matrix?window='+v);
   statusData.matrixWindowMs=response.matrixWindowMs;
 }catch(e){$('message').textContent='Matriz: '+e.message;}
}

async function changeMatrix() {
  const id = Number($("matrixSelect").value);

  if (!Number.isInteger(id)) return;

  await api("/api/matrix?id=" + id);
  statusData.matrix = id;
}

function move(direction) {
 const windowMs=plotWindowMs();
 const rows=data.rows;
 const available=rows.length>1?rows[rows.length-1][0]-rows[0][0]:0;
 const maxOffset=Math.max(0,available-windowMs);
 offset=Math.max(0,Math.min(maxOffset,offset+direction*windowMs/2));
 drawAll();
}

function renderSettings() {
 const root=$("settings");

 root.innerHTML=statusData.channels.map(c=>{
  const options=types.map((name,i)=>{
   if(c.id>=6 && i!==0 && i!==6) return "";
   return `<option value="${i}" ${
    c.type===i?"selected":""}>${name}</option>`;
  }).join("");

  return `
  <div class="item">
   <div class="row">
    <b>${esc(c.pin)}</b>
    <input id="name${c.id}" maxlength="19"
      value="${esc(c.name)}">
    <select id="type${c.id}">${options}</select>
    <input id="color${c.id}" type="color"
      value="${esc(c.color)}">
   </div>
   <div class="row">
    <label>Ganho
     <input id="gain${c.id}" type="number"
      step="any" value="${c.gain}">
    </label>
    <label>Offset
     <input id="offset${c.id}" type="number"
      step="any" value="${c.offset}">
    </label>
    <label>
     <input id="auto${c.id}" type="checkbox"
      ${c.auto?"checked":""}> Escala automática
    </label>
    <label>Mín.
     <input id="min${c.id}" type="number"
      step="any" value="${c.min}">
    </label>
    <label>Máx.
     <input id="max${c.id}" type="number"
      step="any" value="${c.max}">
    </label>
   </div>
   <div class="row">
    <label>R fixa (Ω)
     <input id="rfixed${c.id}" type="number"
      value="${c.rfixed}">
    </label>
    <label>R25 (Ω)
     <input id="r25${c.id}" type="number"
      value="${c.r25}">
    </label>
    <label>Beta
     <input id="beta${c.id}" type="number"
      value="${c.beta}">
    </label>
    <button onclick="saveChannel(${c.id})">
     Aplicar canal
    </button>
   </div>
   ${c.type===2?`
    <div class="row">
     <label>Temperatura conhecida (°C)
      <input id="cal${c.id}" type="number"
       step="0.1" value="25">
     </label>
     <button onclick="calibrate(${c.id})">
      Calibrar NTC
     </button>
     <span class="muted">Correcção: ${c.trim.toFixed(2)} °C</span>
    </div>`:""}
  </div>`;
 }).join("");
}

// Repõe apenas os nomes nos campos visíveis; não perde o histórico.
function resetGenericNames() {
  for(let id=0;id<8;++id){
    const field=$('name'+id);
    if(field)field.value=id<6?'A'+id:'D'+(id-4);
  }
  $('message').textContent='Nomes genéricos preparados. Sair de Canais para aplicar.';
}

function channelParameters(id) {
  const p = new URLSearchParams({id: String(id)});
  for (const key of ["name", "type", "color", "gain",
      "offset", "min", "max", "rfixed", "r25", "beta"]) {
    p.set(key, $(key + id).value);
  }
  p.set("auto", $("auto" + id).checked ? "1" : "0");
  return p;
}

function channelChanged(c) {
  const id = c.id;
  if (!$('name' + id)) return false;
  const p = channelParameters(id);
  if (p.get('name') !== c.name ||
      p.get('color').toLowerCase() !== c.color.toLowerCase() ||
      Number(p.get('type')) !== Number(c.type) ||
      Number(p.get('auto')) !== Number(c.auto)) return true;
  return [
    ['gain', c.gain], ['offset', c.offset],
    ['min', c.min], ['max', c.max],
    ['rfixed', c.rfixed], ['r25', c.r25], ['beta', c.beta]
  ].some(([key, original]) => Number(p.get(key)) !== Number(original));
}

function validateChannel(id) {
  if (!$("name" + id).value.trim())
    throw Error('O canal ' + (id + 1) + ' necessita de um nome.');
  for (const key of ['gain', 'offset', 'min', 'max',
                     'rfixed', 'r25', 'beta']) {
    const field = $(key + id);
    if (!field.value.trim() || !Number.isFinite(Number(field.value)))
      throw Error('Valor inválido em ' + key + ', canal ' + (id + 1));
  }
  if (Number($("max" + id).value) <= Number($("min" + id).value))
    throw Error('Escala mínima/máxima inválida no canal ' + (id + 1));
}

async function saveChannel(id) {
  validateChannel(id);
  await api('/api/channel?' + channelParameters(id));
  await reload();
}

// Aplica todos os canais alterados quando se sai do separador.
// Nao grava EEPROM; so o botao "Guardar configuracao" o faz.
async function applyModifiedChannels() {
  if (!statusData) return true;
  const changed = statusData.channels.filter(channelChanged);
  if (!changed.length) return true;
  try {
    for (const c of changed) validateChannel(c.id);
    $('message').textContent = 'A aplicar ' + changed.length +
      ' canal(is)...';
    for (const c of changed) {
      await api('/api/channel?' + channelParameters(c.id));
    }
    await reload();
    $('message').textContent = 'Canais aplicados · aquisição em curso';
    return true;
  } catch (e) {
    $('message').textContent = 'Erro na configuração: ' + e.message;
    return false;
  }
}

async function calibrate(id) {
 if(statusData?.simulation){$('message').textContent='Para calibrar uma NTC, selecciona AQUISIÇÃO REAL.';return;}
 const temp=$("cal"+id).value;

 await api("/api/cal?id="+id+
  "&temp="+encodeURIComponent(temp));

 await reload();
}

async function changeRate() {
 await api("/api/rate?ms="+$("rate").value);
 await reload();
}

function renderOperationMode(){
  if(!statusData)return;
  const demo=!!statusData.simulation;
  $('realMode')?.classList.toggle('active',!demo);
  $('demoMode')?.classList.toggle('active',demo);
  $('realMode')?.setAttribute('aria-pressed',String(!demo));
  $('demoMode')?.setAttribute('aria-pressed',String(demo));
  if($('modeStatus')) $('modeStatus').textContent=demo?'DEMO · sinais simulados':'AQUISIÇÃO REAL · entradas físicas';
}
async function setOperationMode(demo){
  if(!statusData||!!statusData.simulation===demo)return;
  if(browserSession?.active && !confirm('Mudar a fonte reinicia o histórico do Arduino. Parar a gravação actual do Safari e mudar de modo?'))return;
  if(browserSession?.active){browserSession.active=false;renderSessionInfo();}
  try{
    await api('/api/sim?on='+(demo?1:0));
    await reload();
    $('message').textContent=demo?'DEMO activo · sinais simulados':'AQUISIÇÃO REAL activa · entradas físicas';
  }catch(e){$('message').textContent='Falha ao mudar o modo: '+e.message;}
}

async function saveSettings() {
 await api("/api/save");
 $("message").textContent=
  "Configuração guardada na EEPROM.";
}

// Exporta os flancos como eventos, com duracao aproximada dos impulsos.
// Os tempos provem da interrupcao D2/D3, nao de leitura periodica.
function exportDigitalEdgesCSV(){
 if(!digitalEvents.length){
  $('message').textContent='Sem transições digitais para exportar.';
  return;
 }
 const lines=['DataHora_UTC;Tempo_ms;Micros_fraccao;Canal;Estado;Duracao_ON_us'];
 const lastRise=new Map();
 for(const e of digitalEvents){
   const key=e.id;
   let duration='';
   if(e.level)lastRise.set(key,e);
   else if(lastRise.has(key)){
     const rise=lastRise.get(key);
     duration=String((e.t-rise.t)*1000+e.us-rise.us);
     lastRise.delete(key);
   }
   const date=statusData.epoch
     ?new Date(statusData.epoch*1000+(e.t-statusData.clockMillis)).toISOString()
     :'';
   const channel=statusData.channels[key]?.name||('D'+(key-4));
   lines.push([date,e.t,e.us,channel,e.level?'ON':'OFF',duration].join(';'));
 }
 const blob=new Blob([lines.join('\n')],{type:'text/csv;charset=utf-8'});
 const url=URL.createObjectURL(blob),a=document.createElement('a');
 a.href=url;a.download='OpenDataLogger_transicoes.csv';a.click();
 URL.revokeObjectURL(url);
}

function exportCSV() {
 if(!data.rows.length) return;

 let cols=["DataHora_UTC", "Tempo_ms"];

 [...data.ids,...activeChannels().filter(c=>c.id>=8).map(c=>c.id)].forEach(id=>{
  const c=activeChannels().find(ch=>ch.id===id)||statusData.channels[id];
  cols.push(c?.name||('Canal '+id));
 });

 let lines=[cols.join(";")];

 data.rows.forEach(r=>{
  const absolute = statusData.epoch
    ? new Date(statusData.epoch * 1000 +
        r[0] - statusData.clockMillis).toISOString()
    : "";
  const fields=[absolute, r[0]];

  [...data.ids,...activeChannels().filter(c=>c.id>=8).map(c=>c.id)].forEach(id=>{
   const c=activeChannels().find(ch=>ch.id===id)||statusData.channels[id];
   fields.push(convert(c,valueAt(r,id)));
  });

  lines.push(fields.join(";"));
 });

 const blob=new Blob([lines.join("\n")],
  {type:"text/csv;charset=utf-8"});

 const url=URL.createObjectURL(blob);
 const a=document.createElement("a");

 a.href=url;
 a.download="OpenDataLogger.csv";
 a.click();

 URL.revokeObjectURL(url);
}

async function reload() {
 statusData=await api("/api/status");
 data=await fetchFastPacket(true);
 mergeDigitalEvents(data,true);
 incrementalSupported = typeof data.reset === "boolean" &&
   Number.isFinite(data.capacity);
 lastStatusCheck = Date.now();

 $("rate").value=statusData.sampleMs;
 renderOperationMode();

 // Capacidade actualizada em cada resposta da aquisicao.
 updateStorage();

 const focus=$("focus");
 const previous=focus.value;

 focus.innerHTML=activeChannels().map(c=>
  `<option value="${c.id}">${esc(c.name)}</option>`
 ).join("");

 if([...focus.options].some(o=>o.value===previous))
  focus.value=previous;

const matrixSelect = $("matrixSelect");

matrixSelect.innerHTML = physicalChannels().map(c =>
  `<option value="${c.id}">${esc(c.name)}</option>`
).join("");

matrixSelect.value = String(statusData.matrix);
if($('matrixWindow'))$('matrixWindow').value =
 String(statusData.matrixWindowMs || 10000);

if (matrixSelect.selectedIndex < 0 &&
    matrixSelect.options.length > 0) {
  matrixSelect.selectedIndex = 0;
  await changeMatrix();
}

 // Preserva a escolha do utilizador enquanto actualiza os canais activos.
 const trg=$('triggerChannel');
 const selectedTrigger=trg.value || String(statusData.trigger?.channel ?? 6);
 trg.innerHTML=physicalChannels().map(c=>
   `<option value="${c.id}">${esc(c.name)}</option>`).join('');
 trg.value=selectedTrigger;
 if(trg.selectedIndex<0&&trg.options.length)trg.selectedIndex=0;
 refreshTriggerOptions();
 if(statusData.trigger){
  $('triggerState').textContent=triggerLabel(statusData.trigger);
  $('triggerLevel').value=statusData.trigger.level;
  $('triggerPre').value=statusData.trigger.preMs;
  $('triggerPost').value=statusData.trigger.postMs;
  if(String(statusData.trigger.channel)===trg.value){
    $('triggerCondition').value=String(statusData.trigger.condition);
  }
 }
 renderSettings();
 renderNetwork();
 renderAdvanced();
 drawAll();
}

function renderNetwork() {
 const s=statusData;

 const clock=s.clockSource===1?"NTP":
  s.clockSource===2?"Navegador":"Sem hora";

 $("network").textContent=
  `IP: ${s.ip} · Gateway: ${s.gateway} · `+
  `DNS: ${s.dns} · Relógio: ${clock}`;
}

function formatDuration(ms) {
  const seconds = Math.max(0, ms) / 1000;
  if (seconds >= 3600) return (seconds / 3600).toFixed(1) + " h";
  if (seconds >= 60) return (seconds / 60).toFixed(1) + " min";
  return seconds.toFixed(1) + " s";
}

function updateMemoryCapacity() {
  if (!statusData) return;
  const box = $("memoryCapacity");
  if (!box) return;
  // Capacidade calculada no firmware para o formato REAL de cada registo.
  // Aqui apenas se apresentam os valores já recebidos, sem novo pedido HTTP.
  const active = (statusData.channels || []).filter(c => Number(c.type) !== 0).length;
  const cap = Number(statusData.capacity) || 0;
  const bytesPerRecord = Number(statusData.recordBytes) || 0;
  const ramBytes = Number(statusData.bytes) || 4096;
  const sample = Number(statusData.sampleMs) || 1000;
  const availableTimeMs = cap > 1 ? (cap - 1) * sample : 0;
  const currentUsed = Number(statusData.used) || 0;
  const usedTimeMs = currentUsed > 1 ? (currentUsed - 1) * sample : 0;
  box.textContent =
    `Canais físicos activos: ${active} / 8 · RAM do histórico: ${ramBytes.toLocaleString('pt-PT')} bytes` +
    ` · Registo: ${bytesPerRecord} bytes (4 de tempo + ${active} × 2 de leitura)` +
    ` · Capacidade: ${cap.toLocaleString('pt-PT')} registos` +
    ` · Intervalo: ${sample} ms` +
    ` · Duração máxima estimada: ${formatDuration(availableTimeMs)}` +
    ` · Ocupação actual: ${currentUsed.toLocaleString('pt-PT')} registos (~${formatDuration(usedTimeMs)})`;
}

function updateStorage() {
  if (!statusData) return;
  updateMemoryCapacity();
  const entries = data.rows || [];
  const actualMs = entries.length > 1
    ? Math.max(0, entries[entries.length - 1][0] - entries[0][0])
    : 0;

  const cap = Number(statusData.capacity) || 0;
  const period = Number(statusData.sampleMs) || 1000;
  const estimatedMs = cap > 0 ? (cap - 1) * period : 0;

  $("storage").textContent =
    `${statusData.used} / ${cap} amostras · ` +
    `${statusData.recordBytes} bytes/registo · ` +
    `intervalo ${period} ms · ` +
    `retenção estimada ${formatDuration(estimatedMs)} · ` +
    `histórico actual ${formatDuration(actualMs)}`;
}

// ODLB/2: amostras e flancos digitais, binários, num único pedido HTTP.
// Existe fallback para a API JSON das versões anteriores.
let binarySupported=true;
let binaryGeneration=0;
let deviceNowMs=null;
let lastTransferBytes=0;
let lastTransferRows=0;
let totalTransferredBytes=0;

function decodeBinary(buffer){
  const d=new DataView(buffer);
  if(d.byteLength<48 || String.fromCharCode(...new Uint8Array(buffer,0,4))!=='ODLB')
    throw new Error('Formato ODLB inválido');
  const version=d.getUint8(4);
  if(version!==1&&version!==2)throw new Error('Versão ODLB desconhecida');
  const countChannels=d.getUint8(6);
  const n=d.getUint16(32,true);
  const stride=d.getUint16(34,true);
  const headerSize=version===2?64:48;
  const edgeCount=version===2?d.getUint16(48,true):0;
  if(countChannels>8 || stride!==4+2*countChannels ||
      d.byteLength!==headerSize+n*stride+edgeCount*12)
    throw new Error('Comprimento ODLB inválido');
  const ids=Array.from(new Uint8Array(buffer,36,countChannels));
  const rows=new Array(n);
  for(let i=0;i<n;i++){
    const at=headerSize+i*stride;
    const row=new Array(1+countChannels);
    row[0]=d.getUint32(at,true);
    for(let j=0;j<countChannels;j++)row[j+1]=d.getUint16(at+4+j*2,true);
    rows[i]=row;
  }
  const events=[];
  for(let i=0;i<edgeCount;i++){
    const at=headerSize+n*stride+i*12;
    events.push({
      seq:d.getUint32(at,true),
      t:d.getUint32(at+4,true),
      us:d.getUint16(at+8,true),
      id:d.getUint8(at+10),
      level:d.getUint8(at+11)
    });
  }
  return {
    ids, rows, events, version,
    reset:!!(d.getUint8(5)&1),
    eventGap:!!(d.getUint8(5)&2),
    eventsOverwritten:version===2?d.getUint32(54,true):0,
    triggerState:d.getUint8(7),
    capacity:d.getUint16(8,true),
    used:d.getUint16(10,true),
    sampleMs:d.getUint32(12,true),
    triggerMomentMs:d.getUint32(16,true),
    generation:d.getUint32(20,true),
    acquired:d.getUint32(44,true),
    deviceNowMs:version===2?d.getUint32(58,true):null
  };
}

async function fetchFastPacket(forceFull=false){
  if(!binarySupported) {
    const old=data.rows.length?data.rows[data.rows.length-1][0]:null;
    const path=forceFull||old===null?'/api/data':'/api/data?after='+old;
    return api(path);
  }
  const old=data.rows.length?data.rows[data.rows.length-1][0]:null;
  const query=forceFull||old===null?'':
    '?after='+old+'&gen='+binaryGeneration+'&edgeAfter='+lastEventSequence;
  const response=await fetch('/api/bin'+query,{cache:'no-store'});
  if(!response.ok)throw new Error('HTTP '+response.status);
  const type=response.headers.get('content-type')||'';
  if(!type.includes('application/octet-stream')){
    binarySupported=false;
    return api(forceFull?'/api/data':old===null?'/api/data':'/api/data?after='+old);
  }
  const buffer=await response.arrayBuffer();
  const packet=decodeBinary(buffer);
  binaryGeneration=packet.generation;
  lastTransferBytes=buffer.byteLength;
  totalTransferredBytes+=buffer.byteLength;
  lastTransferRows=packet.rows.length;
  return packet;
}

// Identifica automaticamente se o firmware ja suporta /api/data?after=.
// Se nao suportar, continua a pedir o historico completo (sem duplicados).
let incrementalSupported = false;
let polling = false;
let lastStatusCheck = 0;

async function updateDiagnostics() {
 if(!statusData || Date.now()-lastDiagnosticsFetch<5000)return;
 lastDiagnosticsFetch=Date.now();
 try {
  const d=await api('/api/diagnostics');
  $('diagnostics').textContent=
   `Modo: ${d.timerMode ? 'GPT (prioritário)' : 'cooperativo de reserva'} · ticks IRQ: ${d.irqTicks||0} · fila: ${d.queueDepth||0}/63 (máx. ${d.maxQueueDepth||0}) · perdas FIFO: ${d.queueDrops||0} · maior desvio IRQ: ${d.maxIrqJitterMs||0} ms · períodos IRQ saltados: ${d.skippedIrqPeriods||0} · `+
   `Amostras adquiridas: ${d.acquired} · instantes perdidos: ${d.missedSlots} · `+
   `atraso máximo de aquisição: ${d.maxLateMs} ms · `+
   `HTTP último: ${(d.lastHttpUs/1000).toFixed(1)} ms · `+
   `HTTP máximo: ${(d.maxHttpUs/1000).toFixed(1)} ms · `+
   `intervalo máximo do loop: ${d.maxLoopGapMs} ms · `+
   `Wi-Fi (Safari, última consulta): ${lastApiRoundtripMs.toFixed(0)} ms · `+
   `transições em RAM: ${d.edgesStored??0}/128 · substituídas: ${d.edgesOverwritten??0}`;
 }catch(e){$('diagnostics').textContent='Diagnóstico indisponível: '+e.message;}
}

function mergeDigitalEvents(packet, force=false){
  if(!Array.isArray(packet.events)){
    if(force){digitalEvents=[];lastEventSequence=0;}
    return false;
  }
  const reset=force||packet.reset||packet.eventGap;
  if(reset){digitalEvents=[];lastEventSequence=0;}
  let changed=reset;
  for(const e of packet.events){
    if(e.seq<=lastEventSequence)continue;
    if(e.id!==6&&e.id!==7)continue;
    digitalEvents.push(e);
    lastEventSequence=e.seq;
    changed=true;
  }
  // Mantem apenas os eventos preservados na memoria do Arduino.
  // O limite do browser evita crescimento infinito da pagina.
  if(digitalEvents.length>128)
    digitalEvents.splice(0,digitalEvents.length-128);
  digitalEventGap=!!packet.eventGap;
  eventsOverwritten=packet.eventsOverwritten||0;
  updateDigitalEdgeInfo();
  return changed;
}
function updateDigitalEdgeInfo(){
  const el=$('digitalEdgeInfo');
  if(!el)return;
  const prefix='Transições digitais D2/D3: '+digitalEvents.length+
    ' em memória Web · registo por flancos (independente da amostragem)';
  el.textContent=prefix+(digitalEventGap
    ? ' · ATENÇÃO: eventos antigos substituídos antes da leitura'
    : '');
}

async function refreshData() {
  if (!statusData || polling) return;
  polling = true;

  try {
    const oldLast = data.rows.length
      ? data.rows[data.rows.length - 1][0] : null;
    const started=performance.now();
    const packet = await fetchFastPacket(false);
    if(Number.isFinite(packet.deviceNowMs) && packet.deviceNowMs>0) deviceNowMs=packet.deviceNowMs;
    lastApiRoundtripMs=performance.now()-started;
    $('liveLatency').textContent = binarySupported
      ? 'Web binária · '+lastApiRoundtripMs.toFixed(0)+' ms · '+
        lastTransferRows+' amostras · '+lastTransferBytes+' bytes'
      : 'Web JSON · '+lastApiRoundtripMs.toFixed(0)+' ms';

    if (!Array.isArray(packet.ids) || !Array.isArray(packet.rows)) {
      throw new Error("Formato de dados inesperado");
    }

    const hasIncremental = typeof packet.reset === "boolean" &&
      Number.isFinite(packet.capacity);
    incrementalSupported = hasIncremental;

    const idsChanged = JSON.stringify(packet.ids) !==
      JSON.stringify(data.ids);
    const fullRefresh = !hasIncremental || packet.reset ||
      idsChanged || oldLast === null;

    const edgesChanged=mergeDigitalEvents(packet,fullRefresh);
    captureSessionPacket(packet,fullRefresh);
    if (fullRefresh) {
      data.rows = packet.rows;
    } else {
      // Apenas as leituras novas; evita acumular duplicados.
      data.rows.push(...packet.rows);
    }
    data.ids = packet.ids;

    // O navegador tambem respeita o tamanho do historico circular.
    const limit = hasIncremental ? packet.capacity : statusData.capacity;
    if (Number.isFinite(limit) && limit >= 0 && data.rows.length > limit) {
      data.rows.splice(0, data.rows.length - limit);
    }

    if(hasIncremental&&Number.isInteger(packet.triggerState)){
      if(!statusData.trigger)statusData.trigger={};
      statusData.trigger.state=packet.triggerState;
      statusData.trigger.momentMs=packet.triggerMomentMs;
      $('triggerState').textContent=triggerLabel(statusData.trigger);
    }
    if (hasIncremental) {
      statusData.used = packet.used;
      statusData.capacity = packet.capacity;
      statusData.recordBytes = packet.recordBytes;
      statusData.sampleMs = packet.sampleMs;
      statusData.retentionMs = packet.retentionMs;
    }

    // No firmware antigo, consultar periodicamente /api/status
    // para actualizar a capacidade e a frequencia efectiva.
    const checkEveryMs = hasIncremental ? 30000 : 5000;
    if (currentTab === 'system' && Date.now() - lastStatusCheck >= checkEveryMs) {
      const fresh = await api("/api/status");
      Object.assign(statusData, fresh);
      lastStatusCheck = Date.now();
      renderNetwork();
    }

    updateStorage();
    // Durante a monitorizacao fazemos APENAS /api/data;
    // os diagnosticos sao consultados quando abrimos Sistema.
    if(currentTab==='system' && Date.now()-lastDiagnosticsFetch>5000){
      await updateDiagnostics();
    }
    $("rate").value = String(statusData.sampleMs);

    const newLast = data.rows.length
      ? data.rows[data.rows.length - 1][0] : null;
    // Se nada mudou, o eixo temporal precisa de avancar; limitar
    // o redesenho a ~10 fps para nao ocupar desnecessariamente o Safari.
    const nowUi=performance.now();
    if(idsChanged || newLast!==oldLast || packet.reset || edgesChanged ||
       (currentTab==='monitor' && nowUi-(window.lastDrawClock||0)>100)) {
      drawAll();
      window.lastDrawClock=nowUi;
    }
    $("message").textContent = statusData.simulation?
      'Online · DEMO · aquisição de sinais simulados':
      'Online · AQUISIÇÃO REAL · histórico em curso';
  } catch (e) {
    $("message").textContent = "Falha de comunicação: " + e.message;
  } finally {
    polling = false;
  }
}

async function start() {
 await reload();

 if(!statusData.epoch) {
  await api("/api/time?epoch="+
   Math.floor(Date.now()/1000));

  statusData=await api("/api/status");
  renderNetwork();
 }

 $("message").textContent=
  "Online · aquisição em curso";

 // Poll sequencial ADAPTATIVO. Evita consultar a placa 5x/segundo
 // quando a aquisicao foi configurada, por exemplo, para 5 segundos.
 // Mantemos um tempo minimo entre INICIOS de pedidos, nunca sobrepostos.
 async function poll() {
   const started=performance.now();
   try { await refreshData(); }
   catch(e){$('message').textContent='Falha HTTP: '+e.message;}
   const rate=Number(statusData?.sampleMs)||1000;
   const target=Math.min(650,Math.max(140,Math.round(rate*0.8)));
   const elapsed=performance.now()-started;
   setTimeout(poll,Math.max(55,target-elapsed));
 }
 poll();
}

let currentTab = 'monitor';
let tabSwitching = false;

async function showTab(name) {
  if (tabSwitching || name === currentTab) return;
  tabSwitching = true;
  try {
    if (currentTab === 'channels') {
      const ok = await applyModifiedChannels();
      if (!ok) return;  // Mantem o separador Canais se houver erro.
    }
    document.querySelectorAll('.tab').forEach(el => {
      el.classList.toggle('active', el.id === 'tab-' + name);
    });
    document.querySelectorAll('.tabs button').forEach(button => {
      const action = button.getAttribute('onclick');
      button.classList.toggle('active', action === `showTab('${name}')`);
    });
    currentTab = name;
    if (name === 'monitor') drawAll();
    if (name === 'system') {
      updateMemoryCapacity();
      lastDiagnosticsFetch=0;
      updateDiagnostics();
    }
    if(name==='analysis'){renderAdvanced();loadAlarmLog();renderCycleAnalysis();}
    if(name==='sessions'){renderSessionInfo();refreshSdFiles();}
  } finally {
    tabSwitching = false;
  }
}

function updateClock() {
  const now = new Date();

  $("clock").textContent = now.toLocaleString(
    "pt-PT", {
      weekday: "short",
      day: "2-digit",
      month: "2-digit",
      year: "numeric",
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit"
    }
  ) + " · Hora local";
}

updateClock();
setInterval(updateClock, 1000);

// Funcionalidades adicionais da versao 1.0.1 (interface, nao ISR).
// As funcoes encontram-se abaixo no mesmo ficheiro para nao depender de CDN.
// OpenDataLogger 1.0.2 - UI de analise e sessoes, sem bibliotecas externas.
let cursorOne=null,cursorTwo=null;
function chartCursorClick(event){
  const canvas=event.currentTarget,p=canvas._plot;if(!p)return;
  const rect=canvas.getBoundingClientRect();
  const x=(event.clientX-rect.left)*canvas.width/rect.width;
  if(x<p.L||x>p.right)return;
  const t=p.startTime+(x-p.L)/(p.right-p.L)*p.windowMs;
  if(cursorOne===null||cursorTwo!==null){cursorOne=t;cursorTwo=null;}
  else cursorTwo=t;
  updateCursorInfo();drawAll();
}
function clearCursors(){cursorOne=null;cursorTwo=null;updateCursorInfo();drawAll();}
function nearestRow(t){
  if(!data.rows.length)return null;
  let lo=0,hi=data.rows.length-1;
  while(lo<hi){const m=(lo+hi)>>1;if(data.rows[m][0]<t)lo=m+1;else hi=m;}
  if(lo>0&&Math.abs(data.rows[lo-1][0]-t)<Math.abs(data.rows[lo][0]-t))lo--;
  return data.rows[lo];
}
function updateCursorInfo(){
  const target=$('cursorInfo');if(!target)return;
  if(cursorOne===null){target.textContent='Cursores: clica em dois instantes de qualquer gráfico.';return;}
  if(cursorTwo===null){target.textContent='Cursor A: '+displayClock(cursorOne)+' · selecciona o segundo instante.';return;}
  const t1=Math.min(cursorOne,cursorTwo),t2=Math.max(cursorOne,cursorTwo);
  const r1=nearestRow(t1),r2=nearestRow(t2);
  const d=activeChannels().map(c=>{
    if(!r1||!r2)return '';
    const v1=convert(c,valueAt(r1,c.id)),v2=convert(c,valueAt(r2,c.id));
    return Number.isFinite(v1)&&Number.isFinite(v2)?`${c.name}: Δ${(v2-v1).toFixed(3)} ${unit(c)}`:'';
  }).filter(Boolean).join(' · ');
  target.textContent=`A ${displayClock(t1)} · B ${displayClock(t2)} · Δt ${formatDuration(t2-t1)} · ${d}`;
}
function configChannelOptions(selected,filterDigital=false){
  return statusData.channels.filter(c=>!filterDigital||c.type===6).map(c=>
    `<option value="${c.id}" ${Number(selected)===c.id?'selected':''}>${esc(c.name)} (${esc(c.pin)})</option>`).join('');
}
const virtualNames=['Desactivado','Diferença (A−B)','Soma (A+B)','Razão (A/B)','Derivada de A (por segundo)','Média móvel de A (5 leituras)'];
const alarmNames=['Desactivado','Canal A acima do limiar','Canal A abaixo do limiar','Digital A ligado por tempo','Digital A desligado por tempo','Diferença A−B acima do limiar','Digital A ON E analógico B acima do limiar'];
function renderAdvanced(){
  if(!statusData)return;
  const virtualRoot=$('virtualSettings');
  if(virtualRoot){
    virtualRoot.innerHTML=(statusData.virtuals||[]).map((v,i)=>`<div class="item"><div class="row">
       <label>Nome <input id="vn${i}" maxlength="19" value="${esc(v.name)}"></label>
       <label>Cor <input id="vc${i}" type="color" value="${esc(v.color)}"></label>
       <label>Operação <select id="vm${i}">${virtualNames.map((n,j)=>`<option value="${j}" ${v.mode===j?'selected':''}>${esc(n)}</option>`).join('')}</select></label>
       <label>A <select id="va${i}">${configChannelOptions(v.a)}</select></label>
       <label>B <select id="vb${i}">${configChannelOptions(v.b)}</select></label>
      </div></div>`).join('');
  }
  const alarmRoot=$('alarmSettings');
  if(alarmRoot){
    alarmRoot.innerHTML=(statusData.alarms||[]).map((a,i)=>`<div class="item"><div class="row"><b>Regra ${i+1}</b>
      <select id="am${i}">${alarmNames.map((n,j)=>`<option value="${j}" ${a.mode===j?'selected':''}>${esc(n)}</option>`).join('')}</select>
      <label>A <select id="aa${i}">${configChannelOptions(a.a)}</select></label>
      <label>B <select id="ab${i}">${configChannelOptions(a.b)}</select></label>
      <label>Limiar <input id="al${i}" type="number" step="any" value="${a.level}"></label>
      <label>Duração mínima (ms) <input id="ah${i}" type="number" min="0" max="86400000" step="100" value="${a.holdMs}"></label>
      <label>Acção <select id="ax${i}"><option value="0" ${a.action===0?'selected':''}>Registar</option>
      <option value="1" ${a.action===1?'selected':''}>Disparar trigger armado</option></select></label>
    </div></div>`).join('');
  }
  const calRoot=$('calibrationSettings');
  if(calRoot){
    calRoot.innerHTML=statusData.channels.filter(c=>c.type===2).map(c=>{
      const info=statusData.calibration?.[c.id]||{points:[],sh:null};
      return `<div class="item"><b>${esc(c.name)} (${esc(c.pin)})</b>
       <div class="hint">${info.points.length}/3 referências · ${info.sh?'Steinhart–Hart activa':(info.points.length?'Correcção da curva Beta':'Sem referências')} · ${statusData.simulation?'DEMO activo: mudar para aquisição real antes de calibrar':'Usa a temperatura medida pelo termómetro de referência'}</div>
       <div class="row"><label>Temperatura de referência (°C) <input id="calref${c.id}" type="number" step="any" value="25"></label>
       <button ${statusData.simulation?'disabled title="Desliga o modo DEMO antes de calibrar"':''} onclick="addCalibrationPoint(${c.id})">Guardar ponto da leitura actual</button>
       <button onclick="clearCalibration(${c.id})">Limpar pontos</button></div>
       <div class="hint">Pontos: ${info.points.map(p=>`ADC ${p[0]} → ${p[1]} °C`).join(' · ')||'Nenhum'}</div></div>`;
    }).join('')||'<p class="hint">Activa um canal NTC em Canais para configurar esta calibração.</p>';
  }
  const cmp=$('compareChannel');
  if(cmp){const prev=cmp.value;cmp.innerHTML=physicalChannels().map(c=>`<option value="${c.id}">${esc(c.name)}</option>`).join('');
    if([...cmp.options].some(o=>o.value===prev))cmp.value=prev;}
  if($('ssid0')&&statusData.networkSetup){
    const n=statusData.networkSetup;
    $('ssid0').value=n.ssid0||'';$('ssid1').value=n.ssid1||'';
    $('apName').value=n.apName||'OpenDataLogger';
  }
}
async function saveVirtuals(){
 try{
  for(let i=0;i<(statusData.virtuals||[]).length;++i){
   const params=new URLSearchParams({index:i,name:$('vn'+i).value,color:$('vc'+i).value,
    mode:$('vm'+i).value,a:$('va'+i).value,b:$('vb'+i).value});
   await api('/api/virtual?'+params);
  }
  await reload();$('message').textContent='Canais calculados aplicados. Para persistir, guardar configuração.';
 }catch(e){$('message').textContent='Erro: '+e.message;}
}
async function saveAlarms(){
 try{
  for(let i=0;i<(statusData.alarms||[]).length;++i){
   const p=new URLSearchParams({index:i,mode:$('am'+i).value,a:$('aa'+i).value,b:$('ab'+i).value,
    level:$('al'+i).value,holdMs:$('ah'+i).value,action:$('ax'+i).value});
   await api('/api/alarm?'+p);
  }
  await reload();await loadAlarmLog();
  $('message').textContent='Alarmes aplicados. Para persistir, guardar configuração.';
 }catch(e){$('message').textContent='Erro alarmes: '+e.message;}
}
async function addCalibrationPoint(id){
 try{
  if(statusData?.simulation)throw Error('Passa para AQUISIÇÃO REAL antes de calibrar');
  const v=Number($('calref'+id).value);if(!Number.isFinite(v)||v< -100||v>150)throw Error('Temperatura inválida');
  await api('/api/calpoint?'+new URLSearchParams({channel:id,temp:v,action:'add'}));
  await reload();$('message').textContent='Referência registada. Guarda a configuração na EEPROM.';
 }catch(e){$('message').textContent='Erro de calibração: '+e.message;}
}
async function clearCalibration(id){
 await api('/api/calpoint?'+new URLSearchParams({channel:id,action:'clear'}));
 await reload();
}
async function loadAlarmLog(){
 const el=$('alarmHistory');if(!el||currentTab!=='analysis')return;
 try{
  const log=await api('/api/alarmlog');
  el.innerHTML=`<b>${log.total||0} alarmes detectados desde o arranque</b> · `+
    (log.events||[]).slice(-12).reverse().map(e=>
      `<div>${esc(displayClock(e.t))} · Regra ${e.rule+1} · valor ${Number(e.value).toFixed(3)}</div>`).join('');
 }catch(e){el.textContent=e.message;}
}
async function submitWifi(event){
 event.preventDefault();
 const result=$('wifiResult');result.textContent='A guardar as credenciais...';
 try{
  const body=new URLSearchParams({
   admin:$('adminKey').value,ssid0:$('ssid0').value,pass0:$('pass0').value,
   ssid1:$('ssid1').value,pass1:$('pass1').value,
   apName:$('apName').value,apPassword:$('apPassword').value
  });
  const response=await fetch('/api/wifi',{method:'POST',cache:'no-store',
   headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
  const answer=await response.json();
  if(!response.ok||!answer.ok)throw Error(answer.error||('HTTP '+response.status));
  result.textContent='Redes guardadas. Reinicia o Arduino; o endereço IP poderá mudar.';
  $('pass0').value='';$('pass1').value='';$('apPassword').value='';$('adminKey').value='';
 }catch(e){result.textContent='Falha: '+e.message;}
}

// Cursos de funcionamento, contagens e tempos derivados dos flancos;
// sem assumir que os dados carregados representam um ciclo completo.
let lastCycleReport=null;
function analysisSource(){
 const capture=browserSession?.rows?.length?browserSession:null;
 if(capture)return {rows:capture.rows,ids:capture.ids,events:capture.events,
  channels:capture.status.channels,source:'Sessão do navegador'};
 return {rows:data.rows,ids:data.ids,events:digitalEvents,
  channels:statusData?.channels||[],source:'Histórico RAM actual'};
}
function cycleReport(){
 const src=analysisSource();
 const edges=[...(src.events||[])].sort((a,b)=>a.t-b.t||a.us-b.us);
 const digital=[];
 const channelList=src.channels||[];
 const candidateIds=new Set([
   ...channelList.filter(c=>c?.type===6).map(c=>c.id),
   ...edges.map(e=>e.id)
 ]);
 for(const id of [...candidateIds].sort((a,b)=>a-b)){
  const c=channelList[id];
  if(c && c.type!==6)continue;
  let began=null,starts=0,completed=0,onMs=0;
  const durations=[];
  for(const edge of edges.filter(e=>e.id===id)){
   const when=edge.t+((edge.us||0)/1000);
   if(edge.level){starts++;began=when;}
   else if(began!==null){
    const delta=Math.max(0,when-began);
    onMs+=delta;durations.push(delta);completed++;began=null;
   }
  }
  digital.push({id,name:c?.name||('Digital '+id),starts,completed,onMs,
    averageMs:completed?onMs/completed:0,
    incomplete:began!==null});
 }
 const analog=[];
 for(const id of src.ids){
  const c=channelList[id];
  if(!c||c.type===0||c.type===6)continue;
  const index=src.ids.indexOf(id);
  let mn=Infinity,mx=-Infinity,count=0;
  for(const row of src.rows){
   const value=convert(c,row[index+1]);
   if(!Number.isFinite(value))continue;
   if(value<mn)mn=value;if(value>mx)mx=value;count++;
  }
  if(count)analog.push({name:c.name,min:mn,max:mx,unit:unit(c),count});
 }
 let first=Infinity,last=-Infinity;
 for(const row of src.rows){first=Math.min(first,row[0]);last=Math.max(last,row[0]);}
 for(const edge of edges){first=Math.min(first,edge.t);last=Math.max(last,edge.t);}
 const periodMs=Number.isFinite(first)&&Number.isFinite(last)?Math.max(0,last-first):0;
 return {source:src.source,rows:src.rows.length,edges:edges.length,
         digital,analog,periodMs};
}
function renderCycleAnalysis(){
 const div=$('cycleAnalysis');if(!div||!statusData)return;
 lastCycleReport=cycleReport();const r=lastCycleReport;
 const digitalRows=r.digital.map(d=>`<tr><td>${esc(d.name)} · flancos ON</td><td>${d.starts}</td></tr>
  <tr><td>${esc(d.name)} · ciclos completos</td><td>${d.completed}</td></tr>
  <tr><td>${esc(d.name)} · tempo ON conhecido</td><td>${formatDuration(d.onMs)}</td></tr>
  <tr><td>${esc(d.name)} · duração média ON</td><td>${formatDuration(d.averageMs)}</td></tr>`).join('');
 const analogRows=r.analog.map(a=>`<tr><td>${esc(a.name)} · mínimo/máximo</td>
  <td>${a.min.toFixed(3)} / ${a.max.toFixed(3)} ${esc(a.unit)}</td></tr>`).join('');
 div.innerHTML=`<p>Origem: <b>${esc(r.source)}</b> · ${r.rows} amostras · ${r.edges} transições · janela ${formatDuration(r.periodMs)}</p>
  <div class="analysis-table-container"><table class="analysis-table"><tbody>
  ${digitalRows||'<tr><td colspan="2">Sem flancos digitais disponíveis.</td></tr>'}
  ${analogRows}
  </tbody></table></div>
  <p>Tempos ON calculados apenas entre flancos completos, sem extrapolações. Eventos em falta ou ciclos parciais podem subestimar os totais.</p>`;
}
function exportAnalysisCSV(){
 const r=cycleReport();
 const safe=s=>String(s).replace(/[;\r\n]/g,' ');
 const lines=['Indicador;Canal;Valor;Unidade',`Origem;;${safe(r.source)};`,
  `Amostras;;${r.rows};`, `Transicoes;;${r.edges};`, `Janela;;${r.periodMs};ms`];
 for(const d of r.digital){
  lines.push(`Flancos_ON;${safe(d.name)};${d.starts};`,
   `Ciclos_completos;${safe(d.name)};${d.completed};`,
   `Tempo_ON;${safe(d.name)};${d.onMs};ms`,
   `Duracao_media_ON;${safe(d.name)};${d.averageMs};ms`);
 }
 for(const a of r.analog){
  lines.push(`Minimo;${safe(a.name)};${a.min};${safe(a.unit)}`,
    `Maximo;${safe(a.name)};${a.max};${safe(a.unit)}`);
 }
 downloadText('OpenDataLogger_relatorio.csv',lines.join('\n'),'text/csv;charset=utf-8');
}
function downloadText(name,body,type){
 const url=URL.createObjectURL(new Blob([body],{type}));
 const a=document.createElement('a');a.href=url;a.download=name;document.body.appendChild(a);a.click();a.remove();
 setTimeout(()=>URL.revokeObjectURL(url),1000);
}

// Uma sessao do Safari acumula dados independentemente do ring buffer do R4.
// Limite explicito para impedir crescimento infinito da RAM do browser.
let browserSession=null,importedSession=null,playRunning=false,playTimestamp=0;
let playLastFrame=0;
const MAX_SESSION_ROWS=100000,MAX_SESSION_EDGES=50000;
function sanitizedStatus(){
 const s=JSON.parse(JSON.stringify(statusData));
 delete s.networkSetup;delete s.ip;delete s.gateway;delete s.dns;
 return s;
}
function startBrowserSession(){
 if(!statusData||browserSession?.active)return;
 if(browserSession?.rows?.length && !confirm('A sessão anterior ainda está no navegador. Exporta-a antes de iniciar outra. Substituir mesmo assim?'))return;
 browserSession={format:'ODL-session-1',version:'1.0.2',name:$('sessionName').value||'Sessão',
  created:new Date().toISOString(),active:true,ids:[...data.ids],rows:data.rows.map(r=>[...r]),
  events:digitalEvents.map(e=>({...e})),status:sanitizedStatus(),lost:0,recorded:0};
 renderSessionInfo();
}
function stopBrowserSession(){if(browserSession)browserSession.active=false;renderSessionInfo();}
function captureSessionPacket(packet,fullRefresh){
 const sess=browserSession;
 if(!sess?.active||!Array.isArray(packet.rows))return;
 if(JSON.stringify(sess.ids)!==JSON.stringify(packet.ids)){
   sess.active=false;renderSessionInfo();return;
 }
 if(fullRefresh&&packet.reset)sess.lost++;
 const last=sess.rows.length?sess.rows[sess.rows.length-1][0]:-1;
 for(const row of packet.rows){
   if(row[0]<=last)continue;
   if(sess.rows.length>=MAX_SESSION_ROWS){sess.active=false;sess.lost++;break;}
   sess.rows.push([...row]);sess.recorded++;
 }
 const lastSeq=sess.events.length?sess.events[sess.events.length-1].seq:0;
  let latestSeq=lastSeq;
  for(const edge of packet.events||[]){
   if(edge.seq<=latestSeq)continue;
   if(sess.events.length>=MAX_SESSION_EDGES){sess.lost++;break;}
   sess.events.push({...edge});latestSeq=edge.seq;
 }
 if(packet.eventGap)sess.lost++;
 if(Date.now()-(sess.lastDisplay||0)>1000){sess.lastDisplay=Date.now();renderSessionInfo();}
}
function renderSessionInfo(){
 const div=$('sessionInfo');if(!div)return;
 const started=!!browserSession,recording=!!browserSession?.active;
 $('sessionStart')?.classList.toggle('active',recording);
 $('sessionStop')?.classList.toggle('selected-stop',started&&!recording);
 $('sessionStart')?.setAttribute('aria-pressed',String(recording));
 $('sessionStop')?.setAttribute('aria-pressed',String(started&&!recording));
 const badge=$('sessionState');if(badge){
   badge.classList.toggle('recording',recording);
   badge.textContent=recording?'A GRAVAR':(started?'PARADA':'INACTIVA');
 }
 if(!started){div.textContent='Sem sessão iniciada. A aquisição do Arduino continua independentemente desta gravação.';return;}
 const s=browserSession;
 div.textContent=`${s.name} · ${recording?'A GRAVAR':'Parada'} · ${s.rows.length} amostras · ${s.events.length} flancos · avisos de lacunas: ${s.lost||0}`;
}
function exportSessionJSON(){
 if(!browserSession){$('message').textContent='Inicia uma sessão primeiro.';return;}
 const sess={...browserSession,active:false};
 delete sess.lastDisplay;
 downloadText('OpenDataLogger_sessao_'+Date.now()+'.json',JSON.stringify(sess),'application/json;charset=utf-8');
}
function exportSessionCSV(){
 if(!browserSession)return;
 const s=browserSession;
 const cols=['Tempo_ms',...s.ids.map(id=>'RAW_'+(s.status.channels[id]?.name||('CH'+id)))];
 const lines=[cols.join(';')];
 for(const row of s.rows)lines.push(row.join(';'));
 downloadText('OpenDataLogger_sessao_'+Date.now()+'.csv',lines.join('\n'),'text/csv;charset=utf-8');
}
function parseSdCSV(text,name){
  // Formato opcional do cartao SD: "# ...", S=amostra, E=flanco,
  // C=definicao de canal. Os valores de S sao ADC brutos (nao °C/V).
  const lines=text.split(/\r?\n/);
  let ids=[],rate=1000,epoch=0,clock=0;
  const status=sanitizedStatus();
  const rows=[],events=[];
  let seq=0;
  for(const line of lines){
    if(line.startsWith('# sample_ms=')){
      const m=line.match(/sample_ms=(\d+).*epoch_anchor=(\d+).*clock_ms=(\d+).*channels=([0-7,]*)/);
      if(m){rate=Number(m[1]);epoch=Number(m[2]);clock=Number(m[3]);ids=m[4].split(',').filter(Boolean).map(Number);}
    }else if(line.startsWith('# C;')){
      const v=line.slice(4).split(';').map(Number),c=status.channels[v[0]];
      if(c&&v.length>=8){c.type=v[1];c.gain=v[2];c.offset=v[3];c.rfixed=v[4];c.r25=v[5];c.beta=v[6];c.trim=v[7];}
    }else if(line.startsWith('S;')){
      const v=line.split(';');
      if(v.length<4)continue;
      const time=Number(v[1]),values=v.slice(3).filter(x=>x.length).map(Number);
      if(Number.isFinite(time)&&values.every(Number.isFinite)&&rows.length<MAX_SESSION_ROWS)rows.push([time,...values]);
    }else if(line.startsWith('E;')){
      const v=line.split(';').map(Number);
      if(v.length===6&&events.length<MAX_SESSION_EDGES){
        events.push({t:v[1],seq:v[2],us:v[3],id:v[4],level:v[5]});seq=Math.max(seq,v[2]);
      }
    }
  }
  if(!ids.length||!rows.length||rows.some(r=>r.length!==ids.length+1))throw Error('CSV não corresponde ao formato de sessão SD');
  status.epoch=epoch;status.clockMillis=clock;status.sampleMs=rate;
  return {format:'ODL-session-1',version:'1.0.2',name,created:new Date().toISOString(),
    ids,rows,events,status,active:false,lost:0};
}
async function importSessionFile(event){
 const file=event.target.files?.[0];if(!file)return;
 const info=$('importInfo');
 try{
  if(file.size>30000000)throw Error('Ficheiro maior que 30 MB');
  const rawText=await file.text();
  const parsed=file.name.toLowerCase().endsWith('.csv')?
    parseSdCSV(rawText,file.name):JSON.parse(rawText);
  if(parsed.format!=='ODL-session-1'||!Array.isArray(parsed.ids)||!Array.isArray(parsed.rows)||
     !Array.isArray(parsed.events)||!Array.isArray(parsed.status?.channels)||parsed.rows.length>MAX_SESSION_ROWS||parsed.events.length>MAX_SESSION_EDGES)
    throw Error('Formato de sessão inválido ou demasiado grande');
  if(parsed.ids.length>8||parsed.rows.some(r=>!Array.isArray(r)||r.length!==parsed.ids.length+1||!Number.isFinite(r[0])))
    throw Error('Estrutura de amostras inválida');
  importedSession=parsed;playTimestamp=parsed.rows.length?parsed.rows[parsed.rows.length-1][0]:0;
  $('playPosition').value=1000;playRunning=false;$('playButton').textContent='Reproduzir';
  info.textContent=`Importada: ${parsed.name} · ${parsed.rows.length} amostras · ${parsed.events.length} flancos`;
  drawSessionComparison();drawImportedReplay();
 }catch(e){importedSession=null;info.textContent='Erro: '+e.message;}
}
function sessionOrigin(s,align){
 const defaultStart=s.rows.length?s.rows[0][0]:0;
 if(align==='first-edge')return [...s.events].filter(e=>e.level===1).sort((a,b)=>a.t-b.t)[0]?.t??defaultStart;
 return defaultStart;
}
function drawSessionComparison(){
 const canvas=$('comparisonCanvas');if(!canvas)return;
 const ctx=canvas.getContext('2d');const W=canvas.width,H=canvas.height;
 ctx.clearRect(0,0,W,H);ctx.font='13px system-ui';ctx.fillStyle='#94a3b8';
 if(!importedSession){ctx.fillText('Importa uma sessão JSON para comparar com a aquisição actual.',25,30);return;}
 const left=browserSession?.rows?.length?browserSession:{rows:data.rows,ids:data.ids,events:digitalEvents,status:statusData,name:'Actual'};
 const right=importedSession;
 const id=Number($('compareChannel').value||0),align=$('compareAlign').value;
 const inputs=[left,right];
 const prepared=inputs.map((s,k)=>{
  const idx=s.ids.indexOf(id),c=s.status?.channels?.[id],origin=sessionOrigin(s,align);
  if(idx<0||!c)return {points:[],color:k===0?'#38bdf8':'#f472b6'};
  const step=Math.max(1,Math.floor(s.rows.length/3500));
  const points=[];
  for(let i=0;i<s.rows.length;i+=step){
   const row=s.rows[i],raw=row[idx+1];
   const v=c.type===6?Number(!!raw):(
    c.type===2?calcNTCWithoutSession(raw,c,s.status):raw*5/4095*c.gain+c.offset);
   if(Number.isFinite(v))points.push([row[0]-origin,v]);
  }
  return {points,color:k===0?'#38bdf8':'#f472b6',name:s.name||'Sessão '+(k+1)};
 });
 const all=prepared.flatMap(p=>p.points);if(!all.length){ctx.fillText('Canal sem leituras comparáveis.',25,30);return;}
 const tmin=Math.min(0,...all.map(p=>p[0])),tmax=Math.max(1,...all.map(p=>p[0]));
 let ymin=Math.min(...all.map(p=>p[1])),ymax=Math.max(...all.map(p=>p[1]));
 if(ymin===ymax){ymin-=1;ymax+=1;}
 const L=88,R=25,T=35,B=50;
 ctx.strokeStyle='#334155';ctx.strokeRect(L,T,W-L-R,H-T-B);
 for(let i=0;i<=4;i++){
  const x=L+(W-L-R)*i/4;
  ctx.fillStyle='#94a3b8';ctx.fillText(formatDuration(tmin+(tmax-tmin)*i/4),x,H-20);
 }
 for(const series of prepared){
  ctx.beginPath();ctx.strokeStyle=series.color;ctx.lineWidth=2;let started=false;
  for(const [t,v] of series.points){
   const x=L+(t-tmin)/(tmax-tmin)*(W-L-R),y=T+(ymax-v)/(ymax-ymin)*(H-T-B);
   if(!started){ctx.moveTo(x,y);started=true;}else ctx.lineTo(x,y);
  }
  ctx.stroke();
 }
 ctx.fillStyle=prepared[0].color;ctx.fillText(prepared[0].name||'Actual',L,18);
 ctx.fillStyle=prepared[1].color;ctx.fillText(prepared[1].name||'Importada',L+260,18);
}
function calcNTCWithoutSession(raw,c,sessionStatus){
 if(raw<=0||raw>=4095)return NaN;
 const r=c.rfixed*raw/(4095-raw);
 const base=v=>{
   const resistance=c.rfixed*v/(4095-v);
   return 1/(1/298.15+Math.log(resistance/c.r25)/c.beta)-273.15;
 };
 const info=sessionStatus?.calibration?.[c.id];
 if(info?.sh?.length===3){
   const L=Math.log(r),[a,b,d]=info.sh,inv=a+b*L+d*L*L*L;
   if(inv>0)return 1/inv-273.15+(c.trim||0);
 }
 const pts=info?.points||[],v=base(raw);
 if(pts.length===1)return v+pts[0][1]-base(pts[0][0])+(c.trim||0);
 if(pts.length===2){
   const sorted=pts.map(p=>[base(p[0]),p[1]-base(p[0])]).sort((a,b)=>a[0]-b[0]);
   const frac=sorted[1][0]!==sorted[0][0]?(v-sorted[0][0])/(sorted[1][0]-sorted[0][0]):0;
   return v+sorted[0][1]+Math.max(0,Math.min(1,frac))*(sorted[1][1]-sorted[0][1])+(c.trim||0);
 }
 return v+(c.trim||0);
}
function drawImportedReplay(){
 const canvas=$('replayCanvas');if(!canvas)return;
 if(!importedSession?.rows?.length)return;
 const sess=importedSession,all=sess.rows;
 const start=all[0][0],end=all[all.length-1][0];
 if(!playTimestamp)playTimestamp=end;
 const until=playTimestamp;
 const shown=all.filter(r=>r[0]<=until);
 const step=Math.max(1,Math.floor(shown.length/3200));
 const subset=shown.filter((_,i)=>i%step===0);
 const oldData=data,oldStatus=statusData,oldEvents=digitalEvents,oldNow=deviceNowMs;
 try{
  data={ids:sess.ids,rows:subset};statusData=sess.status;
  digitalEvents=sess.events.filter(e=>e.t<=until);deviceNowMs=until;
  const chans=activeChannels().slice(0,8);
  if(chans.length)drawPlot(canvas,chans,'lanes');
 }finally{data=oldData;statusData=oldStatus;digitalEvents=oldEvents;deviceNowMs=oldNow;}
 $('playbackInfo').textContent='Posição: '+formatDuration(until-start)+' / '+formatDuration(end-start)+' · amostras originais conservadas no ficheiro';
}
function seekPlayback(){
 if(!importedSession?.rows?.length)return;
 const a=importedSession.rows[0][0],b=importedSession.rows[importedSession.rows.length-1][0];
 playTimestamp=a+(b-a)*Number($('playPosition').value)/1000;
 drawImportedReplay();
}
function togglePlayback(){
 if(!importedSession?.rows?.length)return;
 playRunning=!playRunning;$('playButton').textContent=playRunning?'Pausa':'Reproduzir';
 playLastFrame=performance.now();
 if(playRunning)requestAnimationFrame(playFrame);
}
function playFrame(now){
 if(!playRunning||!importedSession)return;
 const rows=importedSession.rows;
 const end=rows[rows.length-1][0],start=rows[0][0];
 playTimestamp=Math.min(end,playTimestamp+(now-playLastFrame)*Number($('playSpeed').value));
 playLastFrame=now;
 $('playPosition').value=Math.round((playTimestamp-start)/Math.max(1,end-start)*1000);
 drawImportedReplay();
 if(playTimestamp>=end){playRunning=false;$('playButton').textContent='Reproduzir';return;}
 requestAnimationFrame(playFrame);
}
async function sdCommand(action){
 const out=$('sdInfo');try{
  await api('/api/sd/'+action);await refreshSdFiles();
 }catch(e){out.textContent='SD: '+e.message;}
}
async function refreshSdFiles(){
 const out=$('sdInfo'),listing=$('sdFiles');if(!out||currentTab!=='sessions')return;
 try{
  const s=await api('/api/sd/status');
  out.textContent=s.compiled? (s.ready?`Cartão ligado · ${s.logging?'A gravar':'Parado'} · ${s.file||'sem sessão'} · ${s.bytes} bytes · erros ${s.errors} · flancos perdidos ${s.missedEdges||0}`:'Firmware SD activo, cartão não detectado'):
    'SD não detectado: mantém o registo no navegador e na RAM; verifica o cartão e reinicia.';
  if(!s.ready){listing.textContent='';return;}
  const r=await api('/api/sd/list');
  listing.innerHTML=(r.files||[]).map(name=>`<div><a href="/api/sd/download?name=${encodeURIComponent(name)}" download>${esc(name)}</a></div>`).join('');
 }catch(e){out.textContent='Erro SD: '+e.message;}
}


start().catch(e=>{
 $("message").textContent="Erro: "+e.message;
});
</script>
</body>
</html>
)WEB";
