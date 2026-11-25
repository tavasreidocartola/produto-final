// ============================================================
//  Garra hidráulica (seringas + água) com MOTORES DC + PONTE H
//  controlada pelo celular.
//
//  NÃO PRECISA DE NENHUMA BIBLIOTECA EXTRA: usa só o que já vem
//  com o suporte ao ESP32 na Arduino IDE (WiFi, WebServer e LEDC).
//  Tudo está neste arquivo único: a configuração fica logo abaixo.
//
//  Funciona com ponte H L298N, L9110S, TB6612 ou parecidas
//  (2 entradas por motor). No L298N, deixe os jumpers ENA/ENB.
//
//  Comandos em http://192.168.4.1/cmd?c=<comando>
//    m,<motor>,<dir>   dir = 1 (botão +), -1 (botão −), 0 (parar)
//                      o celular repete enquanto o botão está apertado
//    v,<0-100>         velocidade em %
//    x                 PARAR tudo
//    p                 ping
//  Vários comandos podem ir juntos, separados por ';'  ex: m,0,1;m,1,-1
//  Toda resposta é um JSON com o estado atual.
// ============================================================
#include <WiFi.h>
#include <WebServer.h>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

// ============================================================
//  CONFIGURAÇÃO  (edite aqui: pinos, nomes, rede, velocidade)
// ============================================================

// ---------- Rede Wi-Fi criada pelo ESP32 (modo Access Point) ----------
// O celular conecta nesta rede e abre http://192.168.4.1
#define WIFI_SSID   "Garra-Carangueijo"
#define WIFI_SENHA  "garra1234"   // mínimo 8 caracteres

// ---------- Motores (cada um = 1 motor DC + 1 par de seringas) ----------
// 8 seringas = 4 pares = 4 motores = 2 módulos de ponte H (2 motores cada).
//
// pinoA / pinoB : vão nas entradas da ponte H (IN1/IN2 ou IN3/IN4).
// botaoMenos / botaoMais : texto dos botões no celular.
// invertido     : true se o motor gira ao contrário do botão.
//
//  Ponte H 1:  IN1 = 32  IN2 = 33  (Base)    |  IN3 = 25  IN4 = 26  (Ombro)
//  Ponte H 2:  IN1 = 23  IN2 = 22  (Cotovelo)|  IN3 = 19  IN4 = 18  (Garra)
//  (Ponte H 1 usa pinos do lado esquerdo da placa, a Ponte H 2 do lado direito.)
struct Motor {
  const char* nome;
  const char* botaoMenos;
  const char* botaoMais;
  uint8_t     pinoA;
  uint8_t     pinoB;
  bool        invertido;
};

#define NUM_MOTORES 4

static const Motor MOTORES[NUM_MOTORES] = {
  // nome        botão −      botão +     pinoA pinoB invertido
  { "Base",     "Esquerda",  "Direita",    32,   33,   false },
  { "Ombro",    "Desce",     "Sobe",       25,   26,   false },
  { "Cotovelo", "Recolhe",   "Estica",     23,   22,   false },
  { "Garra",    "Abre",      "Fecha",      19,   18,   false },
};

// ---------- Velocidade (PWM nas entradas da ponte H) ----------
#define VELOCIDADE_PADRAO  70    // % (ajustável pelo celular)
#define VELOCIDADE_MIN     30    // abaixo disso o motor costuma nem girar
#define PWM_FREQ         1000    // Hz
#define PWM_BITS            8    // 0–255
#define RAMPA_MS          150    // partida suave (evita tranco na hidráulica)

// ---------- Segurança ----------
// O celular manda o comando repetidamente enquanto o botão está apertado.
// Se parar de chegar por este tempo, o motor para (botão solto / sinal caiu).
#define TEMPO_SEM_COMANDO_MS  400
// Tempo máximo girando sem soltar o botão (protege o fim de curso da seringa).
#define TEMPO_MAX_MS         6000

// ============================================================
//  PÁGINA DO CELULAR
// ============================================================
// Página web servida pelo ESP32 (abre em qualquer navegador de celular).
// Motores DC: SEGURE o botão para mover, SOLTE para parar.

const char PAGINA_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="pt-BR">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
  <meta name="theme-color" content="#0a101c">
  <link rel="manifest" href="/manifest.json">
  <link rel="icon" href="/icon.svg" type="image/svg+xml">
  <link rel="apple-touch-icon" href="/icon.svg">
  <title>Garra hidráulica</title>
  <style>
    :root {
      color-scheme: dark;
      font-family: system-ui, sans-serif;
      background: #0a101c;
      color: #edf4ff;
      --cyan: #68e3ff;
      --muted: #9aacc6;
      --border: #273751;
      --surface: #121c2c;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100svh;
      background: radial-gradient(ellipse at 48% 0%, #142748, transparent 550px), #0a101c;
    }
    button, input { font: inherit; }
    button { -webkit-tap-highlight-color: transparent; }
    button:focus-visible, input:focus-visible {
      outline: 2px solid var(--cyan);
      outline-offset: 3px;
    }
    svg.icon {
      width: 24px;
      height: 24px;
      fill: none;
      stroke: currentColor;
      stroke-width: 1.8;
      stroke-linecap: round;
      stroke-linejoin: round;
      flex: none;
    }
    .wrap {
      max-width: 950px;
      margin: auto;
      padding: clamp(18px, 4vw, 48px) max(16px, env(safe-area-inset-right))
        max(24px, env(safe-area-inset-bottom)) max(16px, env(safe-area-inset-left));
    }
    header, .identity, .connection, .joint-heading, .speed-heading, .stop {
      display: flex;
      align-items: center;
    }
    header {
      justify-content: space-between;
      flex-wrap: wrap;
      gap: 16px;
      padding-bottom: 21px;
      border-bottom: 1px solid var(--border);
    }
    .identity { gap: 13px; }
    .logo {
      display: grid;
      place-items: center;
      width: 51px;
      height: 51px;
      flex: none;
      border: 1px solid #2a61a1;
      border-radius: 15px;
      color: var(--cyan);
      background: linear-gradient(145deg, #1c4380, #13243d);
    }
    .logo svg { width: 28px; height: 28px; }
    h1, h2, p { margin: 0; }
    h1 { font-size: clamp(1.4rem, 4.5vw, 2.15rem); letter-spacing: -.04em; line-height: 1.1; }
    .subtitle { color: var(--muted); font-size: .85rem; margin-top: 4px; }
    .connection {
      gap: 8px;
      padding: 7px 12px;
      color: #b1c0d5;
      background: #141f30;
      border: 1px solid var(--border);
      border-radius: 30px;
      font-size: .82rem;
      font-weight: 650;
    }
    .connection svg { width: 16px; height: 16px; }
    .connection[data-state="online"] { color: #77efd4; background: #102c31; border-color: #235158; }
    .connection[data-state="offline"] { color: #ffaebc; background: #301c2a; border-color: #5c3144; }
    .hint {
      display: flex;
      align-items: center;
      gap: 8px;
      color: #b4c6de;
      font-size: .91rem;
      padding: 17px 0;
      min-height: 56px;
    }
    .hint svg { width: 18px; height: 18px; color: var(--cyan); }
    .grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 14px; }
    .joint, .speed {
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: 17px;
      padding: 16px;
    }
    .joint[data-active="true"] { border-color: #407db7; }
    .joint-heading { gap: 10px; margin-bottom: 14px; }
    .joint-heading > svg { color: var(--cyan); width: 21px; height: 21px; }
    h2 { font-size: 1.04rem; }
    .joint-state {
      margin-left: auto;
      color: var(--muted);
      font-size: .75rem;
      font-weight: 600;
      text-align: right;
    }
    .joint[data-active="true"] .joint-state { color: var(--cyan); }
    .joint[data-limited="true"] .joint-state { color: #ffd38b; }
    .actions { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 9px; }
    .move {
      display: grid;
      justify-items: center;
      align-content: center;
      gap: 5px;
      min-height: 78px;
      padding: 7px 3px;
      border: 1px solid #344c71;
      border-radius: 12px;
      background: linear-gradient(#1b2c48, #16253d);
      color: #e6f0ff;
      font-weight: 650;
      font-size: .9rem;
      cursor: pointer;
      touch-action: none;
      user-select: none;
      -webkit-touch-callout: none;
      transition: background-color .15s, border-color .15s, transform .15s;
    }
    .move svg { color: #78c4ff; }
    @media (hover: hover) {
      .move:hover:enabled:not([aria-disabled]) { border-color: #62aaff; }
    }
    .move.on:enabled {
      color: #06121e;
      background: #70dcff;
      border-color: #70dcff;
      transform: scale(.98);
    }
    .move.on:enabled svg { color: inherit; }
    .move:disabled, .move[aria-disabled], input:disabled { opacity: .46; cursor: default; }
    .speed { margin-top: 14px; }
    .speed-heading { gap: 9px; }
    .speed-heading > svg { color: var(--cyan); width: 20px; height: 20px; }
    .speed output {
      margin-left: auto;
      color: var(--cyan);
      font-variant-numeric: tabular-nums;
      font-weight: 700;
    }
    .speed input { display: block; width: 100%; height: 29px; margin: 7px 0 0; accent-color: #56bcff; }
    .stop {
      position: sticky;
      bottom: max(10px, env(safe-area-inset-bottom));
      justify-content: center;
      gap: 9px;
      width: 100%;
      min-height: 53px;
      margin-top: 14px;
      border: 1px solid #ff7a8e;
      border-radius: 13px;
      color: #fff;
      background: #bd2846;
      box-shadow: 0 5px 18px #060b15aa;
      font-weight: 750;
      cursor: pointer;
      touch-action: manipulation;
    }
    .stop:hover { background: #ce2d4e; }
    .stop:active { background: #a51c38; }
    .stop svg { width: 19px; height: 19px; fill: currentColor; stroke: none; }
    @media (max-width: 575px) {
      .wrap {
        display: flex;
        flex-direction: column;
        height: 100svh;
        padding-top: max(14px, env(safe-area-inset-top));
        padding-bottom: max(12px, env(safe-area-inset-bottom));
      }
      header { gap: 10px; padding-bottom: 12px; }
      .identity { gap: 9px; }
      .logo { width: 44px; height: 44px; }
      .connection { padding: 6px 9px; }
      .controls { min-height: 0; overflow: auto; scrollbar-width: thin; }
      .hint { min-height: 44px; padding: 10px 0; }
      .grid { grid-template-columns: 1fr; gap: 8px; }
      .joint, .speed { padding: 10px; border-radius: 14px; }
      .joint-heading { margin-bottom: 7px; }
      .move { min-height: 52px; padding: 4px 3px; gap: 3px; line-height: 1.2; }
      .move svg { width: 20px; height: 20px; }
      .speed { margin-top: 10px; padding: 8px 10px; }
      .speed input { height: 26px; }
      .stop { position: static; flex: none; min-height: 48px; margin-top: 10px; }
    }
    @media (max-width: 374px) {
      h1 { font-size: 1.25rem; }
      .logo { width: 38px; height: 38px; }
    }
    @media (prefers-reduced-motion: reduce) {
      .move { transition: none; }
    }
  </style>
</head>
<body>
  <svg aria-hidden="true" width="0" height="0" style="position:absolute">
    <defs>
      <symbol id="arm" viewBox="0 0 24 24"><path d="M3 21h14M7 21v-6l5-6m3-2 3-3m0 0 3 1m-3-1-1 4"/><circle cx="13.5" cy="8.5" r="2"/><circle cx="7" cy="15" r="1.6"/></symbol>
      <symbol id="wifi" viewBox="0 0 24 24"><path d="M3 8a14 14 0 0 1 18 0M6 12a9 9 0 0 1 12 0m-9 4a4 4 0 0 1 6 0m-3 4h.01"/></symbol>
      <symbol id="info" viewBox="0 0 24 24"><circle cx="12" cy="12" r="9"/><path d="M12 11v5m0-8h.01"/></symbol>
      <symbol id="base" viewBox="0 0 24 24"><ellipse cx="12" cy="10" rx="8" ry="4"/><path d="M4 10v5c0 2.2 3.6 4 8 4s8-1.8 8-4v-5"/></symbol>
      <symbol id="shoulder" viewBox="0 0 24 24"><path d="M4 20h10m-5 0V9m0-1 9-4"/><circle cx="9" cy="9" r="2"/></symbol>
      <symbol id="elbow" viewBox="0 0 24 24"><path d="m4 18 5-9m3-1 8 5"/><circle cx="10.5" cy="8" r="2"/></symbol>
      <symbol id="claw" viewBox="0 0 24 24"><path d="M12 3v5M8 8h8l3 4v5l-4 4m-7-13-3 4v5l4 4"/></symbol>
      <symbol id="left" viewBox="0 0 24 24"><path d="M19 12H5m6-6-6 6 6 6"/></symbol>
      <symbol id="right" viewBox="0 0 24 24"><path d="M5 12h14m-6-6 6 6-6 6"/></symbol>
      <symbol id="up" viewBox="0 0 24 24"><path d="M12 19V5m-6 6 6-6 6 6"/></symbol>
      <symbol id="down" viewBox="0 0 24 24"><path d="M12 5v14m-6-6 6 6 6-6"/></symbol>
      <symbol id="in" viewBox="0 0 24 24"><path d="M3 12h6m-3-3 3 3-3 3m15-3h-6m3-3-3 3 3 3M12 5v14"/></symbol>
      <symbol id="out" viewBox="0 0 24 24"><path d="M9 12H3m3-3-3 3 3 3m9-3h6m-3-3 3 3-3 3M12 5v14"/></symbol>
      <symbol id="open" viewBox="0 0 24 24"><path d="M12 3v5M8 8h8l4 4v6l-1 2M8 8l-4 4v6l1 2"/></symbol>
      <symbol id="speed-icon" viewBox="0 0 24 24"><path d="M4 18a9 9 0 1 1 16 0H4Zm8-4 4-4"/></symbol>
      <symbol id="stop-icon" viewBox="0 0 24 24"><rect x="4" y="4" width="16" height="16" rx="3"/></symbol>
    </defs>
  </svg>
  <main class="wrap">
    <header>
      <div class="identity">
        <div class="logo"><svg class="icon" aria-hidden="true"><use href="#arm"/></svg></div>
        <div><h1>Garra hidráulica</h1><p class="subtitle">Controle local · ESP32</p></div>
      </div>
      <div class="connection" id="st" data-state="waiting" role="status">
        <svg class="icon" aria-hidden="true"><use href="#wifi"/></svg>
        <span id="connectionText">Conectando…</span>
      </div>
    </header>

    <div class="controls">
      <div class="hint">
        <svg class="icon" aria-hidden="true"><use href="#info"/></svg>
        <p>Segure um botão para mover. Solte para parar.</p>
      </div>

      <div class="grid" id="grid" aria-label="Controles de movimento"></div>

      <div class="speed">
        <div class="speed-heading">
          <svg class="icon" aria-hidden="true"><use href="#speed-icon"/></svg>
          <label for="vel"><strong>Velocidade</strong></label>
          <output id="vv" for="vel">—</output>
        </div>
        <input id="vel" type="range" min="30" max="100" value="70" step="1" disabled>
      </div>
    </div>

    <button id="stop" class="stop" type="button">
      <svg class="icon" aria-hidden="true"><use href="#stop-icon"/></svg>
      Parar todos os motores
    </button>
  </main>
  <script>
    const $ = id => document.getElementById(id);
    const ICONE_JUNTA = ['base', 'shoulder', 'elbow', 'claw'];
    const ICONES = [
      ['left', 'right'], ['down', 'up'], ['in', 'out'], ['open', 'claw']
    ];
    const ROTULOS = [
      ['Esquerda', 'Direita'], ['Descer', 'Subir'],
      ['Recolher', 'Estender'], ['Abrir', 'Fechar']
    ];
    const icone = id => '<svg class="icon" aria-hidden="true"><use href="#' + id + '"/></svg>';

    let estado = null, segurando = {}, ocupado = false, fila = null, falhas = 0;

    function online(ok) {
      falhas = ok ? 0 : falhas + 1;
      const on = ok || (falhas < 3 && $('st').dataset.state === 'online');
      $('st').dataset.state = on ? 'online' : 'offline';
      $('connectionText').textContent = on ? 'Conectado' : 'Desconectado';
      $('vel').disabled = !on;
      document.querySelectorAll('.hold').forEach(b => { b.disabled = !on; });
    }
    function vibrar(ms) { try { navigator.vibrate && navigator.vibrate(ms); } catch (e) {} }

    // Uma requisição por vez; comandos que chegam enquanto isso são juntados
    function enviar(m) {
      if (ocupado) { fila = fila ? fila + ';' + m : m; return; }
      ocupado = true;
      const ctl = new AbortController(); const to = setTimeout(() => ctl.abort(), 1500);
      fetch('/cmd?c=' + encodeURIComponent(m), { cache: 'no-store', signal: ctl.signal })
        .then(r => r.json()).then(s => { online(true); atualizar(s); })
        .catch(() => online(false))
        .finally(() => { clearTimeout(to); ocupado = false; if (fila) { const f = fila; fila = null; enviar(f); } });
    }

    function montar(j) {
      $('grid').replaceChildren(...j.map((motor, i) => {
        const painel = document.createElement('section');
        painel.className = 'joint';
        painel.setAttribute('aria-labelledby', 'junta' + i);
        painel.innerHTML = `
          <div class="joint-heading">
            ${icone(ICONE_JUNTA[i])}<h2 id="junta${i}"></h2><span class="joint-state" id="t${i}">Parado</span>
          </div>
          <div class="actions">
            <button type="button" class="move hold" data-i="${i}" data-d="-1">${icone(ICONES[i][0])}<span></span></button>
            <button type="button" class="move hold" data-i="${i}" data-d="1">${icone(ICONES[i][1])}<span></span></button>
          </div>`;
        painel.querySelector('h2').textContent = motor.n;
        painel.querySelectorAll('button span').forEach((rotulo, sentido) => {
          rotulo.textContent = ROTULOS[i][sentido];
        });
        return painel;
      }));
      document.querySelectorAll('.hold').forEach(b => {
        const i = +b.dataset.i, d = +b.dataset.d;
        const ini = e => {
          e.preventDefault();
          if (b.setPointerCapture) try { b.setPointerCapture(e.pointerId); } catch (_) {}
          segurando[i] = d; b.classList.add('on'); vibrar(12); enviar(`m,${i},${d}`);
        };
        const fim = () => {
          if (segurando[i] !== d) return;
          delete segurando[i]; b.classList.remove('on'); enviar(`m,${i},0`);
        };
        b.addEventListener('pointerdown', ini);
        ['pointerup', 'pointercancel', 'lostpointercapture'].forEach(ev => b.addEventListener(ev, fim));
        b.addEventListener('contextmenu', e => e.preventDefault());
      });
    }

    function atualizar(s) {
      if (!estado || estado.j.length !== s.j.length) montar(s.j);
      estado = s;
      s.j.forEach((x, i) => {
        const t = $('t' + i);
        const painel = t.closest('.joint');
        painel.dataset.active = x.d !== 0;
        painel.dataset.limited = x.t;
        t.textContent = x.t ? 'Solte para liberar'
          : x.d ? ROTULOS[i][x.d > 0 ? 1 : 0] : 'Parado';
      });
      if (document.activeElement !== $('vel')) $('vel').value = s.v;
      $('vv').textContent = s.v + '%';
    }

    function pararTudo() {
      Object.keys(segurando).forEach(k => delete segurando[k]);
      document.querySelectorAll('.hold.on').forEach(b => b.classList.remove('on'));
      fila = null; enviar('x');
    }
    $('stop').onclick = () => { vibrar([30, 40, 30]); pararTudo(); };
    $('vel').oninput = () => { $('vv').textContent = $('vel').value + '%'; enviar('v,' + $('vel').value); };

    // Enquanto houver botão apertado, repete o comando (se parar de chegar, o motor para)
    setInterval(() => {
      const ks = Object.keys(segurando);
      enviar(ks.length ? ks.map(i => `m,${i},${segurando[i]}`).join(';') : 'p');
    }, 150);
    document.addEventListener('visibilitychange', () => { if (document.hidden) pararTudo(); });
    window.addEventListener('blur', () => { if (Object.keys(segurando).length) pararTudo(); });
    enviar('p');
  </script>
</body>
</html>
)rawliteral";

// Ícone e manifesto: permitem "Adicionar à tela inicial" com cara de app
const char ICONE_SVG[] PROGMEM = R"rawliteral(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><rect width="512" height="512" rx="112" fill="#ffb020"/><g fill="none" stroke="#1a1200" stroke-width="40" stroke-linecap="round" stroke-linejoin="round"><path d="M110 410h170M195 410v-100l105-125M300 185l80-80M380 105l60 20M380 105l-20 60"/></g><circle cx="195" cy="310" r="30" fill="#1a1200"/></svg>)rawliteral";

const char MANIFESTO_JSON[] PROGMEM = R"rawliteral({"name":"Garra Hidráulica","short_name":"Garra","start_url":"/","display":"standalone","orientation":"any","background_color":"#0d1117","theme_color":"#0d1117","icons":[{"src":"/icon.svg","sizes":"any","type":"image/svg+xml","purpose":"any"}]})rawliteral";

// ============================================================
//  PROGRAMA
// ============================================================

int   direcao[NUM_MOTORES];          // -1, 0, 1
float duty[NUM_MOTORES];             // força atual (0..1), sobe na rampa
unsigned long ultimoCmd[NUM_MOTORES];
unsigned long inicioGiro[NUM_MOTORES];
bool  travado[NUM_MOTORES];          // parou por tempo máximo: espera soltar o botão
int   velocidade = VELOCIDADE_PADRAO;

WebServer server(80);

// ------------------------------------------------------------
//  PWM via LEDC (nativo do ESP32), compatível com core 2.x e 3.x
// ------------------------------------------------------------
#define PWM_MAX ((1 << PWM_BITS) - 1)

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  void pwmIniciar(uint8_t pino, uint8_t) { ledcAttach(pino, PWM_FREQ, PWM_BITS); }
  void pwmEscrever(uint8_t pino, uint8_t, uint32_t v) { ledcWrite(pino, v); }
#else
  void pwmIniciar(uint8_t pino, uint8_t canal) { ledcSetup(canal, PWM_FREQ, PWM_BITS); ledcAttachPin(pino, canal); }
  void pwmEscrever(uint8_t, uint8_t canal, uint32_t v) { ledcWrite(canal, v); }
#endif

void aplicarMotor(int i) {
  int d = direcao[i];
  if (MOTORES[i].invertido) d = -d;
  uint32_t v = (uint32_t)(duty[i] * PWM_MAX);
  uint32_t a = (d > 0) ? v : 0;
  uint32_t b = (d < 0) ? v : 0;
  pwmEscrever(MOTORES[i].pinoA, i * 2,     a);
  pwmEscrever(MOTORES[i].pinoB, i * 2 + 1, b);
}

void pararMotor(int i) {
  direcao[i] = 0;
  duty[i] = 0;
  aplicarMotor(i);
}

void pararTudo() {
  for (int i = 0; i < NUM_MOTORES; i++) pararMotor(i);
}

// ------------------------------------------------------------
String estadoJson() {
  String s = "{\"v\":" + String(velocidade) + ",\"j\":[";
  for (int i = 0; i < NUM_MOTORES; i++) {
    if (i) s += ",";
    s += "{\"n\":\"" + String(MOTORES[i].nome) + "\"" +
         ",\"m\":\"" + String(MOTORES[i].botaoMenos) + "\"" +
         ",\"p\":\"" + String(MOTORES[i].botaoMais) + "\"" +
         ",\"d\":"   + String(direcao[i]) +
         ",\"t\":"   + (travado[i] ? "true" : "false") + "}";
  }
  return s + "]}";
}

void tratarMensagem(const char* msg) {
  int j, v;
  unsigned long agora = millis();
  switch (msg[0]) {
    case 'm':
      if (sscanf(msg, "m,%d,%d", &j, &v) == 2 && j >= 0 && j < NUM_MOTORES) {
        v = constrain(v, -1, 1);
        if (v == 0) { pararMotor(j); travado[j] = false; break; }
        if (travado[j]) { ultimoCmd[j] = agora; break; }   // ainda segurando: ignora
        if (v != direcao[j]) {               // começou a girar (ou trocou o sentido)
          duty[j] = 0;
          inicioGiro[j] = agora;
          direcao[j] = v;
        }
        ultimoCmd[j] = agora;
      }
      break;
    case 'v':
      if (sscanf(msg, "v,%d", &v) == 1) velocidade = constrain(v, VELOCIDADE_MIN, 100);
      break;
    case 'x':
      pararTudo();
      Serial.println("PARAR acionado");
      break;
    default:   // 'p' (ping)
      break;
  }
}

// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Garra hidraulica ESP32 (motores DC) ===");

  for (int i = 0; i < NUM_MOTORES; i++) {
    pwmIniciar(MOTORES[i].pinoA, i * 2);
    pwmIniciar(MOTORES[i].pinoB, i * 2 + 1);
    pararMotor(i);
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_SENHA);
  Serial.printf("Rede: %s  |  Senha: %s\n", WIFI_SSID, WIFI_SENHA);
  Serial.print("Abra no celular: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send(200, "text/html", PAGINA_HTML); });
  server.on("/icon.svg", []() {
    server.sendHeader("Cache-Control", "max-age=86400");
    server.send(200, "image/svg+xml", ICONE_SVG);
  });
  server.on("/manifest.json", []() { server.send(200, "application/manifest+json", MANIFESTO_JSON); });
  server.on("/cmd", []() {
    if (server.hasArg("c")) {            // vários comandos separados por ';'
      String c = server.arg("c");
      int ini = 0;
      while (ini < (int)c.length() && ini < 200) {
        int fim = c.indexOf(';', ini);
        if (fim < 0) fim = c.length();
        tratarMensagem(c.substring(ini, fim).c_str());
        ini = fim + 1;
      }
    }
    server.send(200, "application/json", estadoJson());
  });
  server.onNotFound([]() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  server.begin();
}

void loop() {
  server.handleClient();

  static unsigned long t = 0;
  unsigned long agora = millis();
  if (agora - t < 10) return;
  t = agora;

  float alvo = velocidade / 100.0f;
  float passo = alvo * 10.0f / RAMPA_MS;

  for (int i = 0; i < NUM_MOTORES; i++) {
    // Destrava quando o botão foi solto (comandos pararam de chegar)
    if (travado[i] && agora - ultimoCmd[i] > TEMPO_SEM_COMANDO_MS) travado[i] = false;
    if (direcao[i] == 0) continue;

    // Segurança: botão solto / sinal perdido
    if (agora - ultimoCmd[i] > TEMPO_SEM_COMANDO_MS) {
      pararMotor(i);
      continue;
    }
    // Segurança: girando tempo demais sem soltar
    if (agora - inicioGiro[i] > TEMPO_MAX_MS) {
      Serial.printf("%s: tempo maximo atingido, parado\n", MOTORES[i].nome);
      pararMotor(i);
      travado[i] = true;
      continue;
    }
    // Partida suave
    duty[i] = min(alvo, duty[i] + passo);
    aplicarMotor(i);
  }
}
