
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
  --text:#f0f3f8; --muted:#98a5b6; --green:#58d6a0;
  --yellow:#f1c46c; --red:#f27c80; --blue:#79aefa;
}
* {box-sizing:border-box}
body {
  margin:0; background:var(--bg); color:var(--text);
  font-family:system-ui,-apple-system,"Segoe UI",sans-serif;
}
main {max-width:1000px;margin:auto;padding:24px 18px 42px}
header,.between {display:flex;justify-content:space-between;align-items:center;gap:12px}
header {margin-bottom:20px}
h1 {font-size:23px;margin:0 0 4px}
h2 {font-size:16px;margin:0}
small,.muted {color:var(--muted)}
.card {
  background:var(--panel);border:1px solid var(--line);
  border-radius:14px;padding:20px;margin-bottom:12px;
}
.pill {
  padding:7px 11px;border-radius:25px;background:#28382f;
  color:var(--green);font-size:12px;font-weight:600;
}
.time-grid {display:grid;grid-template-columns:1fr 1fr;gap:15px}
.time {font-size:clamp(24px,5vw,39px);font-weight:700;letter-spacing:1px}
.label {color:var(--muted);font-size:12px;margin-bottom:7px}
.status-grid {display:grid;grid-template-columns:repeat(4,1fr);gap:10px;margin-top:16px}
.status {
  background:#252e3b;border-radius:11px;padding:12px;
  min-height:65px;
}
.status .name {font-size:12px;color:var(--muted);margin-bottom:9px}
.status .value {font-size:13px;font-weight:600}
.ok {color:var(--green)}
.warn {color:var(--yellow)}
.bad {color:var(--red)}
.section-title {margin-bottom:12px}
.mode {display:flex;gap:4px;padding:4px;background:#111720;border:1px solid var(--line);border-radius:11px}
button {
  border:1px solid var(--line);background:#2a3443;
  color:var(--text);padding:9px 13px;border-radius:8px;
  font:inherit;font-size:13px;cursor:pointer;
}
button.active {background:#294966;border-color:#426b91}
button:disabled {opacity:.45;cursor:not-allowed}
button:active:not(:disabled) {transform:scale(.98)}
.controls {display:grid;grid-template-columns:1fr 1fr 1fr;gap:9px;margin-top:15px}
.stop {border-color:#86525b;color:#ffb6ba}
.info {
  padding:12px;background:#252e3b;border-radius:10px;
  font-size:13px;color:var(--muted);margin-top:10px;
}
.diag {display:grid;grid-template-columns:1fr 1fr;gap:8px}
.diag-item {
  background:#222b35;border-radius:9px;padding:11px 13px;
}
.diag-item .value {margin-top:6px;font-size:13px}
.log {max-height:220px;overflow-y:auto}
.event {display:flex;align-items:baseline;justify-content:space-between;gap:16px;padding:10px 0;border-bottom:1px solid var(--line)}
.event:last-child {border:0}
.event .when {color:var(--muted);font-size:11px;white-space:nowrap;order:-1;min-width:135px}
[hidden] {display:none !important}
.mode button {border:0;background:transparent}
.mode button.active {background:#294966}
#autoInfo {display:none}
#manualPanel {border-top:1px solid var(--line);padding-top:13px;margin-top:14px}
#manualPanel .info {background:transparent;padding:4px 0 0}
footer {text-align:center;color:var(--muted);font-size:12px;margin-top:18px}
@media(max-width:600px) {
  .card {padding:15px}
  .status-grid {grid-template-columns:1fr 1fr}
  .diag {grid-template-columns:1fr}
  .between {align-items:flex-start;flex-direction:column}
  .mode {width:100%}
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
      <div class="label">Время стрелок</div>
      <div class="time" id="hands">--:--</div>
    </div>
  </div>
  <div class="status-grid">
    <div class="status">
      <div class="name">Wi-Fi</div>
      <div class="value" id="wifi">Проверка</div>
    </div>
    <div class="status">
      <div class="name">NTP</div>
      <div class="value" id="ntp">Не настроен</div>
    </div>
    <div class="status">
      <div class="name">AS5600</div>
      <div class="value" id="sensor">Не подключён</div>
    </div>
    <div class="status">
      <div class="name">Магнит</div>
      <div class="value" id="magnet">Нет данных</div>
    </div>
  </div>
</section>

<section class="card">
  <div class="between section-title">
    <h2>Режим работы</h2>
    <div class="mode">
      <button id="autoBtn" onclick="setMode('auto')">Автоматический</button>
      <button id="manualBtn" onclick="setMode('manual')">Ручной</button>
    </div>
  </div>
  <div id="manualPanel" hidden>
    <div class="controls">
      <button onclick="command('backward')">← Назад</button>
      <button class="stop" onclick="command('stop')">■ Стоп</button>
      <button onclick="command('forward')">Вперёд →</button>
    </div>
    <div class="info" id="commandInfo">
      Тестовый режим: команды не управляют GPIO.
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
      <div class="label">Связь с AS5600</div>
      <div class="value" id="sensorDetail">—</div>
    </div>
    <div class="diag-item">
      <div class="label">Магнитное поле</div>
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

function text(id, value, css) {
  const el = $(id);
  el.textContent = value;
  if (css) el.className = css;
}

function renderStatus(s) {
  currentMode = s.mode;

  $('autoBtn').classList.toggle('active', s.mode === 'auto');
  $('manualBtn').classList.toggle('active', s.mode === 'manual');

  $('manualPanel').hidden = s.mode !== 'manual';

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

  // Положение механических стрелок пока неизвестно.
  text('hands', '--:--');

  text('sensor', 'Не подключён', 'value warn');
  text('sensorDetail', 'Проверка пока отключена');

  text('magnet', 'Нет данных', 'value warn');
  text('magnetDetail', 'Ожидает подключения AS5600');

  text('uptime', formatUptime(s.uptime_seconds));

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
      ? 'Время синхронизировано · датчики не подключены'
      : 'Проверка синхронизации времени'
  );
}

async function refresh() {
  try {
    const response = await fetch('/api/status', {cache:'no-store'});
    if (!response.ok) throw Error('HTTP ' + response.status);
    renderStatus(await response.json());
  } catch (e) {
    text('overall', 'Нет связи с ESP');
    text('wifi', 'Нет связи', 'value bad');
  }
}

async function post(path) {
  const response = await fetch(path, {method:'POST'});
  if (!response.ok) {
    const msg = await response.text();
    throw Error(msg || 'Ошибка запроса');
  }
  await refresh();
}

async function setMode(mode) {
  try {
    await post('/api/mode?value=' + mode);
  } catch (e) {
    alert(e.message);
  }
}

async function command(action) {
  try {
    await post('/api/command?action=' + action);
    text('commandInfo',
      'Команда «' + action + '» обработана в симуляции. GPIO не изменялись.');
  } catch (e) {
    alert(e.message);
  }
}

async function loadLog() {
  try {
    const response = await fetch('/api/events', {cache:'no-store'});
    if (!response.ok) throw Error();
    const data = await response.json();
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
setInterval(refresh, 2000);
setInterval(loadLog, 10000);
</script>
</body>
</html>
)LUCHHTML";
