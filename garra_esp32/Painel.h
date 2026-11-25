#pragma once
#include <Arduino.h>

const char PAGINA_HTML[] PROGMEM = R"PAINEL(<!doctype html>
<html lang="pt-BR">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
  <meta name="theme-color" content="#0a101c">
  <link rel="icon" href="data:,">
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
    .move.held:enabled {
      color: #06121e;
      background: #70dcff;
      border-color: #70dcff;
      transform: scale(.98);
    }
    .move.held:enabled svg { color: inherit; }
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
      <div class="connection" id="connection" data-state="waiting" role="status">
        <svg class="icon" aria-hidden="true"><use href="#wifi"/></svg>
        <span id="connectionText">Conectando…</span>
      </div>
    </header>

    <div class="controls">
      <div class="hint">
        <svg class="icon" aria-hidden="true"><use href="#info"/></svg>
        <p>Segure um botão para mover. Solte para parar.</p>
      </div>

      <div class="grid" id="joints" aria-label="Controles de movimento"></div>

      <div class="speed">
        <div class="speed-heading">
          <svg class="icon" aria-hidden="true"><use href="#speed-icon"/></svg>
          <label for="speed"><strong>Velocidade</strong></label>
          <output id="speedValue" for="speed">—</output>
        </div>
        <input id="speed" type="range" min="30" max="100" value="70" step="1" disabled>
      </div>
    </div>

    <button id="stop" class="stop" type="button">
      <svg class="icon" aria-hidden="true"><use href="#stop-icon"/></svg>
      Parar todos os motores
    </button>
  </main>
  <script>
    'use strict';

    const layout = [
      { name: 'Base', icon: 'base', moves: [['Esquerda', 'left'], ['Direita', 'right']] },
      { name: 'Ombro', icon: 'shoulder', moves: [['Descer', 'down'], ['Subir', 'up']] },
      { name: 'Cotovelo', icon: 'elbow', moves: [['Recolher', 'in'], ['Estender', 'out']] },
      { name: 'Garra', icon: 'claw', moves: [['Abrir', 'open'], ['Fechar', 'claw']] }
    ];
    const icon = id => '<svg class="icon" aria-hidden="true"><use href="#' + id + '"/></svg>';
    const joints = document.getElementById('joints');
    const speed = document.getElementById('speed');
    const speedValue = document.getElementById('speedValue');
    const connection = document.getElementById('connection');
    const connectionText = document.getElementById('connectionText');
    const random = crypto.getRandomValues(new Uint32Array(2));
    const client = Array.from(random, n => n.toString(16).padStart(8, '0')).join('');
    const held = Array(4).fill(null);

    let epoch = '';
    let stamp = 0;
    let ownsMotion = false;
    let sendingControl = false;
    let connected = false;
    let available = false;
    let request = null;
    let stopping = null;
    let dirty = false;
    let pendingSpeed = null;

    const panels = layout.map((joint, index) => {
      const panel = document.createElement('section');
      panel.className = 'joint';
      panel.setAttribute('aria-labelledby', 'joint-' + index);
      panel.innerHTML =
        '<div class="joint-heading">' + icon(joint.icon) +
        '<h2 id="joint-' + index + '">' + joint.name + '</h2>' +
        '<span class="joint-state">—</span></div><div class="actions"></div>';

      joint.moves.forEach(([label, symbol], direction) => {
        const button = document.createElement('button');
        button.className = 'move';
        button.type = 'button';
        button.disabled = true;
        button.innerHTML = icon(symbol) + '<span>' + label + '</span>';
        button.addEventListener('pointerdown', event => {
          if (event.button !== 0 || held[index] || button.disabled || stopping) return;
          event.preventDefault();
          button.setPointerCapture(event.pointerId);
          press(index, direction, button, event.pointerId);
        });
        for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) {
          button.addEventListener(type, event => release(index, button, event.pointerId));
        }
        button.addEventListener('contextmenu', event => event.preventDefault());
        button.addEventListener('keydown', event => {
          if (!isMoveKey(event)) return;
          event.preventDefault();
          if (!event.repeat) press(index, direction, button, event.key);
        });
        button.addEventListener('keyup', event => {
          if (!isMoveKey(event)) return;
          event.preventDefault();
          release(index, button, event.key);
        });
        button.addEventListener('blur', () => {
          const active = held[index];
          if (active && typeof active.source === 'string') release(index, button, active.source);
        });
        panel.querySelector('.actions').append(button);
      });
      joints.append(panel);
      return panel;
    });
    const buttons = [...joints.querySelectorAll('.move')];

    function isMoveKey(event) {
      return event.key === ' ' || event.key === 'Enter';
    }

    function refreshAvailability() {
      const disabled = !connected || !available || document.hidden;
      buttons.forEach(button => {
        button.disabled = disabled;
        if (stopping) button.setAttribute('aria-disabled', 'true');
        else button.removeAttribute('aria-disabled');
      });
      speed.disabled = disabled || Boolean(stopping);
    }

    function render(state) {
      epoch = state.epoch;
      stamp = state.stamp;
      ownsMotion = state.owner === client;
      connected = true;
      available = !state.owner || state.owner === client;
      connection.dataset.state = 'online';
      connectionText.textContent = available ? 'Conectado' : 'Em uso em outro dispositivo';
      panels.forEach((panel, index) => {
        const { direction, limited } = state.motors[index];
        panel.dataset.active = direction !== 0;
        panel.dataset.limited = limited;
        panel.querySelector('.joint-state').textContent = limited ? 'Solte para liberar'
          : direction ? layout[index].moves[direction > 0 ? 1 : 0][0] : 'Parado';
      });
      const value = pendingSpeed ?? state.speed;
      speed.value = value;
      speedValue.textContent = value + '%';
      refreshAvailability();
    }

    function clearHeld() {
      held.forEach((active, index) => {
        if (active) active.button.classList.remove('held');
        held[index] = null;
      });
    }

    function disconnect() {
      connected = false;
      available = false;
      ownsMotion = false;
      epoch = '';
      dirty = false;
      pendingSpeed = null;
      clearHeld();
      connection.dataset.state = 'offline';
      connectionText.textContent = 'ESP32 desconectado';
      speedValue.textContent = '—';
      panels.forEach(panel => {
        panel.dataset.active = false;
        panel.dataset.limited = false;
        panel.querySelector('.joint-state').textContent = '—';
      });
      refreshAvailability();
    }

    function press(index, direction, button, source) {
      if (button.disabled || stopping || held[index]) return;
      held[index] = { button, source, direction: direction ? '+' : '-' };
      button.classList.add('held');
      dirty = true;
      pump();
    }

    function release(index, button, source) {
      const active = held[index];
      if (!active || active.button !== button || active.source !== source) return;
      held[index] = null;
      button.classList.remove('held');
      if (!held.some(Boolean)) {
        stopAll();
        return;
      }
      dirty = true;
      pump();
    }

    async function exchange(path, controller, body) {
      const timeout = setTimeout(() => controller.abort(), body ? 320 : 650);
      try {
        const response = await fetch(path, {
          method: body ? 'POST' : 'GET',
          headers: body ? { 'X-Garra': '1' } : {},
          body,
          cache: 'no-store',
          signal: controller.signal
        });
        if (!response.ok && response.status !== 409) throw new Error('ESP32 indisponível');
        return { state: await response.json(), conflict: response.status === 409 };
      } finally {
        clearTimeout(timeout);
      }
    }

    async function pump(poll = false) {
      if (request || stopping || document.hidden) return;
      const control = connected && available && (dirty || (poll && held.some(Boolean)));
      if (!control && !poll) return;

      const controller = new AbortController();
      request = controller;
      sendingControl = control;
      const body = control ? new URLSearchParams({
        client,
        epoch,
        stamp,
        motion: held.map(active => active ? active.direction : '0').join('')
      }) : undefined;
      if (control) {
        if (pendingSpeed !== null) body.set('speed', pendingSpeed);
        pendingSpeed = null;
        dirty = false;
      }

      try {
        const { state, conflict } = await exchange(control ? '/api/control' : '/api/status', controller, body);
        if (request !== controller) return;
        if (conflict) {
          clearHeld();
          dirty = false;
          pendingSpeed = null;
        }
        render(state);
      } catch {
        if (request === controller) {
          const hadMotion = control || held.some(Boolean);
          disconnect();
          if (hadMotion) stopAll();
        }
      } finally {
        if (request === controller) {
          request = null;
          sendingControl = false;
          if (dirty) pump();
        }
      }
    }

    async function stopAll(global = false) {
      clearHeld();
      dirty = false;
      pendingSpeed = null;
      if (stopping && !global) return;
      const stopRequest = new AbortController();
      const previousStop = stopping;
      stopping = stopRequest;
      if (previousStop) previousStop.abort();
      request?.abort();
      request = null;
      sendingControl = false;
      refreshAvailability();

      try {
        const body = new URLSearchParams({ stop: global ? 'all' : 'own', client });
        const { state } = await exchange('/api/stop', stopRequest, body);
        if (stopping === stopRequest) render(state);
      } catch {
        if (stopping === stopRequest) disconnect();
      } finally {
        if (stopping === stopRequest) {
          stopping = null;
          refreshAvailability();
        }
      }
    }

    document.getElementById('stop').addEventListener('click', () => stopAll(true));
    speed.addEventListener('input', () => {
      pendingSpeed = speed.value;
      speedValue.textContent = speed.value + '%';
      dirty = true;
      pump();
    });

    function stopOnLeave() {
      if (held.some(Boolean) || sendingControl || ownsMotion) stopAll();
    }

    document.addEventListener('visibilitychange', () => {
      if (document.hidden) stopOnLeave();
      else pump(true);
    });
    window.addEventListener('blur', stopOnLeave);
    window.addEventListener('pagehide', stopOnLeave);
    window.addEventListener('offline', () => {
      stopOnLeave();
      disconnect();
    });
    window.addEventListener('online', () => pump(true));
    setInterval(() => { if (held.some(Boolean)) pump(true); }, 110);
    setInterval(() => { if (!held.some(Boolean)) pump(true); }, 750);
    pump(true);
  </script>
</body>
</html>)PAINEL";
