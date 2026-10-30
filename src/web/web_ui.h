
#pragma once
#include <Arduino.h>

// Страница хранится во flash, а не занимает оперативную память.
static const char WEB_UI[] PROGMEM = R"LUCHHTML(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LuchClock — Башенные часы</title>
<style>
:root {
  color-scheme: dark;
  --bg:#101318; --panel:#191f27; --line:#303843;
  --text:#f0f3f8; --muted:#aab6c4; --green:#65d6a6;
  --yellow:#f1c46c; --red:#f27c80; --blue:#91b9ef;
}
* {box-sizing:border-box}
body {
  margin:0; background:var(--bg); color:var(--text);
  font-family:system-ui,-apple-system,"Segoe UI",sans-serif;
}
main {max-width:1000px;margin:auto;padding:18px 18px 22px}
header,.between {display:flex;justify-content:space-between;align-items:center;gap:12px}
header {margin-bottom:12px}
h1 {font-size:22px;font-weight:600;letter-spacing:.04em;margin:0 0 4px}
h2 {font-size:14px;font-weight:600;margin:0}
small,.muted {color:var(--muted)}
.card {
  background:var(--panel);border:1px solid var(--line);
  border-radius:11px;padding:18px;margin-bottom:10px;
}
.pill {
  padding:7px 11px;border-radius:25px;background:#28382f;
  color:#a6f0d0;font-size:12px;font-weight:600;
  border:1px solid #39775c;
}
.time-grid {display:grid;grid-template-columns:1fr 1fr;gap:15px}
.time {font-size:clamp(24px,5vw,37px);font-weight:700;letter-spacing:1px}
.label {color:var(--muted);font-size:12px;margin-bottom:7px}
.status-grid {display:grid;grid-template-columns:repeat(4,1fr);gap:10px;margin-top:12px}
.status {
  background:#222b35;border-radius:11px;padding:12px;
  min-height:65px;
}
.status .name {font-size:12px;color:var(--muted);margin-bottom:9px}
.status .value {font-size:13px;font-weight:600}
.ok {color:var(--green)}
.warn {color:var(--yellow)}
.bad {color:var(--red)}
.section-title {margin-bottom:12px}
.mode-heading {display:flex;align-items:center;gap:12px;flex-wrap:wrap}
.mode {display:flex;gap:4px;padding:4px;background:#111720;border:1px solid var(--line);border-radius:11px}
button {
  border:1px solid var(--line);background:#2a3443;
  color:var(--text);padding:9px 13px;border-radius:8px;
  font:inherit;font-size:13px;cursor:pointer;
}
button.active {background:#294966;border-color:#426b91}
.controls button.running {background:#1b5145;border-color:#65d6a6;color:#fff;box-shadow:0 0 0 1px #65d6a6}
.controls button.running:disabled {background:#1b5145!important;border-color:#65d6a6!important;color:#fff!important;box-shadow:0 0 0 1px #65d6a6}
#commandInfo {min-height:26px}
#modeState {display:block;margin-top:6px;color:var(--muted);font-size:12px}
#autoPanel {text-align:center}
#controlArea {transition:background-color .2s ease}
#transitionTrack {height:5px;background:#293545;border-radius:20px;overflow:hidden;margin:10px auto 2px;max-width:440px}
#transitionBar {height:100%;width:100%;background:var(--blue);transition:width .1s linear}
#manualProgress {max-width:440px;margin:auto}
#manualProgress[hidden],#autoProgress[hidden] {display:none!important}
#autoPanel,#manualPanel {animation:modeFade .2s ease}
@keyframes modeFade {from {opacity:.3;transform:translateY(4px)} to {opacity:1;transform:translateY(0)}}
button:disabled {opacity:1;cursor:not-allowed;background:#151b23!important;border-color:#333c48!important;color:#677586!important;box-shadow:inset 0 0 0 1px #202833}
button:active:not(:disabled) {transform:scale(.98)}
.controls {display:grid;grid-template-columns:1fr 1fr;gap:9px;margin:15px auto 0;max-width:560px}
.stop {background:#572c35;border-color:#b75a68;color:#fff;font-weight:700}
.info {
  padding:12px;background:#222b35;border-radius:10px;
  font-size:13px;color:var(--muted);margin-top:10px;
}
.diag {display:grid;grid-template-columns:1fr 1fr;gap:8px}
.diag-item {
  background:#222b35;border-radius:9px;padding:11px 13px;
}
.diag-item .label {color:#d3dce7;font-weight:500}
.diag-item .value {margin-top:6px;font-size:13px;color:var(--text)}
.log {
  max-height:220px;overflow-y:auto;min-height:0;
  padding-right:14px;
  scrollbar-width:thin;
  scrollbar-color:#64758a #222b35;
}
.log::-webkit-scrollbar {width:8px}
.log::-webkit-scrollbar-track {background:#222b35;border-radius:8px}
.log::-webkit-scrollbar-thumb {background:#64758a;border-radius:8px;border:2px solid #222b35}
.log::-webkit-scrollbar-thumb:hover {background:#8495aa}
.event {display:flex;align-items:baseline;justify-content:space-between;gap:16px;padding:10px 0;border-bottom:1px solid var(--line)}
.event:last-child {border:0}
.event .when {color:var(--muted);font-size:13px;white-space:nowrap;order:-1;min-width:155px}
[hidden] {display:none !important}
.mode button {border:0;background:transparent}
.mode button.active {background:#294966}
#autoInfo {display:none}
#controlArea {min-height:112px;display:flex;align-items:center;border-top:1px solid var(--line);margin-top:8px;padding-top:12px}
#autoPanel,#manualPanel {width:100%}
#manualPanel .controls {margin-top:0}
#manualPanel .info {display:block}
#autoPanel {font-size:12px;color:var(--muted)}
#manualPanel .info {background:transparent;padding:8px 0 0;color:var(--text);text-align:center}
@media (min-width:800px) and (min-height:650px) {
  main {height:100dvh;display:flex;flex-direction:column;overflow:hidden}
  header {flex:none}
  .card {flex:none;margin-bottom:10px;padding:14px 18px}
  .card:first-of-type .status {min-height:54px;padding:9px 11px}
  .card:first-of-type .status .name {margin-bottom:4px}
  .card:nth-of-type(3) .diag-item {padding:9px 11px}
  .card:nth-of-type(3) .diag-item .value {margin-top:3px}
  .card:last-of-type {flex:1;min-height:105px;display:flex;flex-direction:column}
  .card:last-of-type .log {flex:1;max-height:none}
  .event {padding:7px 0}
  footer {flex:none;margin-top:0}
}
footer {text-align:center;color:var(--muted);font-size:12px;margin-top:18px}
@media(max-width:600px) {
  .card {padding:15px}
  .status-grid {grid-template-columns:1fr 1fr}
  .diag {grid-template-columns:1fr}
  .between {align-items:flex-start;flex-direction:column}
  .mode {width:100%}
  .mode-heading {gap:5px}
  .mode button {flex:1}
  .controls button {padding:11px 5px}
  .event {display:block}
  .event .when {display:block;margin-bottom:5px}
}
</style>
</head>
<body>
<main>
<header>
  <div>
    <h1>Башенные часы</h1>
    <small>LuchClock · ESP8266</small>
  </div>
  <span class="pill" id="overall">Загрузка...</span>
</header>

<section class="card">
  <div class="time-grid">
    <div>
      <div class="label">Точное время</div>
      <div class="time" id="time">--:--:--</div>
    </div>
    <div>
      <div class="label">Положение энкодера (минуты)</div>
      <div class="time" id="hands">Нет данных</div>
    </div>
  </div>
  <div class="status-grid" hidden>
    <div class="status">
      <div class="name">Wi-Fi</div>
      <div class="value" id="wifi">Проверка</div>
    </div>
    <div class="status">
      <div class="name">NTP</div>
      <div class="value" id="ntp">Не настроен</div>
    </div>
    <div class="status">
      <div class="name">PCF8574</div>
      <div class="value" id="sensor">Не подключён</div>
    </div>
    <div class="status">
      <div class="name">Код энкодера</div>
      <div class="value" id="magnet">Нет данных</div>
    </div>
  </div>
</section>

<section class="card">
  <div class="between section-title">
    <div class="mode-heading"><h2>Режим работы</h2><small id="modeHint">Автоматический · ожидание времени</small></div>
    <div class="mode">
      <button id="autoBtn" onclick="setMode('auto')">Автоматический</button>
      <button id="manualBtn" onclick="setMode('manual')">Ручной</button>
    </div>
  </div>
  <div id="controlArea">
  <div id="autoPanel"><strong id="autoStatus">Ожидание состояния</strong><span id="modeState">Автоматическое движение пока не реализовано</span><span id="motorStatus" hidden>Остановлен</span>
  <div id="autoProgress" hidden><div id="transitionTrack"><div id="autoBar" style="height:100%;width:100%;background:var(--blue);transition:width .1s linear"></div></div></div></div>
  <div id="manualPanel" hidden>
    <div class="controls">
      <button id="backwardBtn" hidden disabled>← Назад</button>
      <button id="stopBtn" class="stop" onclick="sendMotorCommand('stop')">■ Стоп</button>
      <button id="forwardBtn" onclick="sendMotorCommand('forward')">Вперёд →</button>
    </div>
    <div id="manualProgress" hidden><div id="transitionTrack"><div id="manualBar" style="height:100%;width:100%;background:var(--blue);transition:width .1s linear"></div></div></div>
    <div class="info" id="commandInfo">
      Тестовый выход D5 / GPIO14: вперёд — 3,3 В, стоп — 0 В.
    </div>
  </div>
  </div>
</section>

<section class="card">
  <div class="between section-title">
    <h2>Диагностика</h2>
    <small id="diagSummary">Получение состояния...</small>
  </div>
  <div class="diag">
    <div class="diag-item">
      <div class="label">Подключение Wi-Fi</div>
      <div class="value" id="wifiDetail">—</div>
    </div>
    <div class="diag-item">
      <div class="label">Синхронизация NTP</div>
      <div class="value" id="ntpDetail">—</div>
    </div>
    <div class="diag-item">
      <div class="label">Связь с PCF8574</div>
      <div class="value" id="sensorDetail">—</div>
    </div>
    <div class="diag-item">
      <div class="label">Сырой код / позиция</div>
      <div class="value" id="magnetDetail">—</div>
    </div>
  </div>
</section>

<section class="card">
  <div class="between section-title">
    <h2>Журнал событий</h2>
    <small>Последние события</small>
  </div>
  <div class="log" id="log">
    <small>Загрузка журнала...</small>
  </div>
</section>

<footer>
  LuchClock v0.3 · Тестовый режим ·
  Время работы: <span id="uptime">—</span>
</footer>
</main>

<script>
const $ = id => document.getElementById(id);
let currentMode = 'auto';
let modeRequestPending = false;
let lastLogSignature = '';
let refreshPending = false;
let commandPending = false;
let lastMotorState = 'stopped';
let motorPermissions = {can_forward:false,can_reverse:false,can_stop:true};
let statusFailures = 0;
let statusKnown = false;
let modeSwitchUntil = 0;
let lastStatusAt = 0;
let lastMotorWait = 0;
let lastModeWait = 0;
let lastStatus = null;
let pendingAction = '';
let commandLockUntil = 0;

function formatUptime(seconds) {
  const n = Math.max(0, Number(seconds) || 0);
  const h = Math.floor(n / 3600);
  const m = Math.floor((n % 3600) / 60);
  const sec = Math.floor(n % 60);
  return (h ? h + ' ч ' : '') + m + ' мин ' + sec + ' с';
}
function text(id, value, css) {
  const el = $(id);
  if (!el) return;
  if (el.textContent !== String(value)) el.textContent = value;
  if (css && el.className !== css) el.className = css;
}

function renderStatus(s) {
  if (!modeRequestPending && currentMode !== s.mode) showMode(s.mode);

  text(
    'wifi',
    s.wifi ? 'Подключён' : 'Нет связи',
    'value ' + (s.wifi ? 'ok' : 'bad')
  );

  text(
    'wifiDetail',
    s.wifi ? 'Подключено · ' + s.ip : 'Отключено'
  );

  if (!s.ntp_synced) {
    text('time', '--:--:--');
    text('ntp', 'Ожидание', 'value warn');
    text('ntpDetail', 'Ожидание первой синхронизации');
  } else {
    text('time', s.time_local || '--:--:--');

    if (s.ntp_recent) {
      text('ntp', 'Синхронизировано', 'value ok');

      text(
        'ntpDetail',
        'Последняя синхронизация: ' + s.last_ntp_sync
      );
    } else {
      text('ntp', 'Устаревшая синхронизация', 'value warn');

      text(
        'ntpDetail',
        'Последняя синхронизация: ' + s.last_ntp_sync +
        ' · требуется проверка NTP'
      );
    }
  }

  // Абсолютный ACE-128: 128 позиций отображаются в 0..59 минут.
  text('hands', s.encoder_valid ? String(s.encoder_minutes).padStart(2,'0') + ' мин' : 'Нет данных');

  text('sensor', s.encoder_connected ? 'Подключён' : 'Не найден', 'value ' + (s.encoder_connected ? 'ok' : 'warn'));
  text('sensorDetail', s.encoder_connected ? 'I²C адрес 0x' + Number(s.encoder_address).toString(16).toUpperCase() : 'Нет ответа на I²C (D1/D2)');
  text('magnet', s.encoder_valid ? 'Позиция ' + s.encoder_position : 'Нет данных', 'value ' + (s.encoder_valid ? 'ok' : 'warn'));
  text('magnetDetail', s.encoder_connected ? 'Код: ' + s.encoder_raw + ' (0x' + Number(s.encoder_raw).toString(16).toUpperCase().padStart(2,'0') + ')' + (s.encoder_valid ? ' · позиция: ' + s.encoder_position + '/127' : ' · код не распознан') : 'PCF8574 не найден');

  text('uptime', formatUptime(s.uptime_seconds));
  lastStatus = s;
  lastStatusAt = Date.now();
  statusKnown = true;
  const motorLabels = {stopped:'Остановлен',forward:'Вперёд',reverse:'Назад',waiting:'Остановка · защитная пауза',fault:'Ошибка привода'};
  text('motorStatus', motorLabels[s.motor_state] || 'Неизвестно');
  lastMotorState = s.motor_state || 'stopped';
  motorPermissions = {can_forward:!!s.can_forward,can_reverse:!!s.can_reverse,can_stop:!!s.can_stop};
  lastMotorWait = s.motor_wait_ms || 0;
  lastModeWait = s.mode_wait_ms || 0;
  // Ответ API является единственным источником истины.
  if (pendingAction && ((pendingAction === 'stop' && s.motor_state !== 'forward' && s.motor_state !== 'reverse') ||
      (pendingAction === 'forward' && s.motor_state === 'forward') ||
      (pendingAction === 'backward' && s.motor_state === 'reverse'))) pendingAction = '';
  updateManualButtons(lastMotorState, lastMotorWait);
  updateProgress();
  if (s.motor_state === 'waiting') text('motorStatus', 'Остановка · ' + Math.ceil(s.motor_wait_ms / 1000) + ' с');
  $('autoBtn').disabled = modeRequestPending || (s.mode === 'manual' && !s.can_auto);
  $('autoBtn').title = (s.mode === 'manual' && !s.can_auto) ? 'Сначала нажмите Стоп и дождитесь остановки' : '';
  $('manualBtn').disabled = modeRequestPending || (s.mode === 'auto' && (s.mode_transition || s.motor_state !== 'stopped'));
  $('manualBtn').title = (s.mode === 'auto' && s.mode_transition) ? 'Дождитесь завершения защитной паузы' : '';
  if (s.mode === 'auto') {
    text('autoStatus', s.mode_transition ? 'Остановка · защитная пауза' :
      s.auto_ready ? 'Автоматический режим готов' : 'Автоматический режим не готов');
    text('modeState', s.mode_transition ? 'Ожидание ' + Math.ceil(s.mode_wait_ms / 1000) + ' с · запуск запрещён' :
      s.auto_ready ? 'Автоматическое движение пока не реализовано' : 'Ожидание остановки привода');
  }
  if (s.mode === 'auto') {
    text('modeHint', s.mode_transition ? 'Переключение · ожидание остановки' :
      s.auto_ready ? 'Автоматический · готов к управлению' : 'Автоматический · не готов');
  } else {
    text('modeHint', s.mode_transition ? 'Ручной · ожидание остановки' : 'Ручное управление · тест GPIO14');
  }

  if (s.ntp_recent) {
    text('overall', 'NTP работает');
  } else if (s.ntp_synced) {
    text('overall', 'Проверить NTP');
  } else {
    text('overall', 'Ожидание NTP');
  }

  text(
    'diagSummary',
    s.ntp_recent
      ? 'Время синхронизировано · энкодер: ' + (s.encoder_valid ? s.encoder_minutes + ' мин' : 'нет данных')
      : 'Проверка синхронизации времени'
  );
}

function showMode(mode) {
  currentMode = mode;
  $('autoBtn').classList.toggle('active', mode === 'auto');
  $('manualBtn').classList.toggle('active', mode === 'manual');
  $('manualPanel').hidden = mode !== 'manual';
  $('autoPanel').hidden = mode === 'manual';
  text('modeHint', mode === 'manual' ? 'Ручное управление · тест GPIO14' : 'Автоматический режим');
}

function updateProgress() {
  const s = lastStatus;
  if (!s) return;
  const active = !!s.mode_transition || s.motor_state === 'waiting';
  const left = s.mode_transition ? lastModeWait : lastMotorWait;
  const elapsed = Date.now() - lastStatusAt;
  const remaining = Math.max(0, left - elapsed);
  // Длительность и оставшееся время приходят от ESP; браузер только анимирует шкалу.
  const total = s.mode_transition ? (s.mode_wait_total_ms || 1000) : (s.motor_wait_total_ms || 1000);
  const pct = Math.max(0, Math.min(100, (total - remaining) / total * 100));
  $('autoProgress').hidden = !(active && s.mode === 'auto');
  $('manualProgress').hidden = !(active && s.mode === 'manual');
  $('autoBar').style.width = pct + '%';
  $('manualBar').style.width = pct + '%';
}
function updateManualButtons(state, remaining) {
  const busy = commandPending || modeRequestPending || !!pendingAction;
  const s = lastStatus;
  const transition = s && s.mode_transition;
  const allowedForward = statusKnown && !busy && !transition && motorPermissions.can_forward && state !== 'forward';
  const allowedBackward = statusKnown && !busy && !transition && motorPermissions.can_reverse && state !== 'reverse';
  $('forwardBtn').disabled = !allowedForward;
  $('backwardBtn').disabled = !allowedBackward;
  $('stopBtn').disabled = commandPending || !statusKnown || !motorPermissions.can_stop;
  if (s && s.mode === 'manual') {
    $('autoBtn').disabled = modeRequestPending || !s.can_auto || state !== 'stopped';
  }
  $('forwardBtn').classList.toggle('running', state === 'forward');
  $('backwardBtn').classList.toggle('running', state === 'reverse');
  if (state === 'fault') text('commandInfo', 'Авария привода · управление заблокировано');
  else if (transition) text('commandInfo', 'Переключение режима · пауза ' + Math.ceil((s.mode_wait_ms || 0) / 1000) + ' с');
  else if (state === 'waiting') text('commandInfo', 'Остановка · защитная пауза ' + Math.ceil(remaining / 1000) + ' с');
  else if (pendingAction) text('commandInfo', 'Ожидание подтверждения команды от ESP');
  else text('commandInfo', 'Привод: ' + ({stopped:'остановлен',forward:'движется вперёд',reverse:'движется назад'}[state] || 'состояние неизвестно'));
}
async function refresh() {
  if (refreshPending) return;
  refreshPending = true;
  try {
    const response = await fetch('/api/status', {cache:'no-store'});
    if (!response.ok) throw Error('HTTP ' + response.status);
    const data = await response.json();
    statusFailures = 0;
    renderStatus(data);
  } catch (e) {
    statusFailures++;
    if (statusFailures >= 3) {
      text('overall', 'Связь с ESP нестабильна');
      text('wifiDetail', 'Нет ответа API · состояние Wi-Fi неизвестно');
      motorPermissions = {can_forward:false,can_reverse:false,can_stop:true};
      statusKnown = false;
      updateManualButtons(lastMotorState, 0);
    }
  } finally {
    refreshPending = false;
  }
}

async function post(path) {
  const response = await fetch(path, {method:'POST'});
  if (!response.ok) {
    const msg = await response.text();
    throw Error(msg || 'Ошибка запроса');
  }
}

async function setMode(mode) {
  if (modeRequestPending || commandPending || mode === currentMode) return;
  if (mode === 'auto' && (!lastStatus || !lastStatus.can_auto || lastMotorState !== 'stopped')) return;
  if (mode === 'manual' && (!lastStatus || lastStatus.mode_transition || lastMotorState !== 'stopped')) return;
  const previous = currentMode;
  modeRequestPending = true;
  showMode(mode);
  $('autoBtn').disabled = true;
  $('manualBtn').disabled = true;
  try {
    await post('/api/mode?value=' + mode);
    await refresh();
    loadLog();
  } catch (e) {
    showMode(previous);
    alert(e.message);
  } finally {
    modeRequestPending = false;
    // Не снимаем защитную блокировку до получения актуального статуса ESP.
    await refresh();
    if (lastStatus) {
      $('autoBtn').disabled = lastStatus.mode === 'manual' && !lastStatus.can_auto;
      $('manualBtn').disabled = lastStatus.mode === 'auto' &&
        (lastStatus.mode_transition || lastStatus.motor_state !== 'stopped');
    }
  }
}

async function sendMotorCommand(action) {
  if (commandPending || modeRequestPending || pendingAction ||
      (action === 'forward' && $('forwardBtn').disabled) ||
      (action === 'backward' && $('backwardBtn').disabled) ||
      (action === 'forward' && !motorPermissions.can_forward) ||
      (action === 'backward' && !motorPermissions.can_reverse)) return;
  commandPending = true;
  pendingAction = action;
  // Не затираем разрешения, полученные от ESP: они обновятся при следующем статусе.
  // Пока запрос выполняется, блокировку обеспечивает commandPending.
  updateManualButtons(lastMotorState, 0);
  try {
    await post('/api/command?action=' + action);
    await refresh();
  } catch (e) {
    pendingAction = '';
    alert(e.message);
  } finally {
    commandPending = false;
    await refresh();
    // При сбое подтверждения не оставляем кнопки навсегда заблокированными.
    if (pendingAction) {
      pendingAction = '';
      updateManualButtons(lastMotorState, lastMotorWait);
    }
  }
}

async function loadLog() {
  try {
    const response = await fetch('/api/events', {cache:'no-store'});
    if (!response.ok) throw Error();
    const data = await response.json();
    const signature = JSON.stringify(data.events);
    if (signature === lastLogSignature) return;
    lastLogSignature = signature;
    const container = $('log');
    container.replaceChildren();
    if (!data.events.length) {
      const el = document.createElement('small');
      el.textContent = 'Событий пока нет';
      container.appendChild(el);
      return;
    }
    for (const event of data.events) {
      const item = document.createElement('div');
      item.className = 'event';
      const title = document.createElement('div');
      title.textContent = event.message;
      const when = document.createElement('div');
      when.className = 'when';
      when.textContent = event.time;
      item.append(title, when);
      container.appendChild(item);
    }
  } catch (e) {
    text('log', 'Не удалось получить журнал');
  }
}

refresh();
loadLog();
setInterval(refresh, 250);
setInterval(() => {
  if (!lastStatus) return;
  const elapsed = Date.now() - lastStatusAt;
  if (lastStatus.motor_state === 'waiting') {
    const remaining = Math.max(0, lastMotorWait - elapsed);
    updateManualButtons('waiting', remaining);
  }
  updateProgress();
  if (lastStatus.mode_transition) {
    const remaining = Math.max(0, lastModeWait - elapsed);
    if (currentMode === 'auto') text('modeState', 'Ожидание ' + (remaining / 1000).toFixed(1) + ' с · запуск запрещён');
    else text('commandInfo', 'Переключение режима · пауза ' + (remaining / 1000).toFixed(1) + ' с');
  }
}, 100);
setInterval(loadLog, 10000);
</script>
</body>
</html>
)LUCHHTML";
