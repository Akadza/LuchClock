
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

/* Unslop polish: precise instrument panel, quiet surfaces, 4px rhythm. */
:root {--bg:#111418;--panel:#1a1e23;--line:#30363d;--text:#eef0f2;--muted:#aab2bb;--green:#70c9a0;--yellow:#e4bc72;--red:#eb8588;--blue:#9ebcda;--raised:#242a30}
body {line-height:1.45}
main {max-width:1120px;padding:24px 24px 32px}
header {margin-bottom:20px}
h1 {font-size:24px;font-weight:500;letter-spacing:0;line-height:1.2}
h2 {font-size:15px;font-weight:500}
.card {background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:20px 24px;margin-bottom:16px}
.pill {border:1px solid var(--line);border-radius:8px;background:var(--raised);color:var(--text);font-weight:500;padding:6px 10px}
.time-grid {gap:24px}
.time {font-size:clamp(28px,5vw,40px);font-weight:500;letter-spacing:0;font-variant-numeric:tabular-nums;line-height:1.25}
.label {font-size:13px;margin-bottom:8px}
.section-title {margin-bottom:16px}
.mode {background:var(--bg);border-radius:10px}
button {background:var(--raised);border-color:var(--line);padding:10px 16px;border-radius:8px;font-size:14px;min-height:40px;transition:background-color .16s ease,border-color .16s ease,color .16s ease}
button:hover:not(:disabled) {background:#303840}
button:focus-visible {outline:2px solid var(--blue);outline-offset:3px}
.mode button.active {background:#39434e;color:var(--text)}
.controls button.running,.controls button.running:disabled {background:#25453a!important;border-color:#527d68!important;color:#eaf7f0!important;box-shadow:none}
button:disabled {background:#1b2026!important;border-color:#30363d!important;color:#89939f!important;box-shadow:none}
.stop {background:#553035;border-color:#875158;color:#fff;font-weight:500}
.stop:hover:not(:disabled) {background:#683a40}
#controlArea {min-height:120px;margin-top:12px;padding-top:16px}
#autoPanel {font-size:14px}
#modeState {font-size:13px;margin-top:8px}
.controls {gap:12px}
.diag {gap:12px}
.diag-item {background:var(--raised);border:1px solid #343b43;border-radius:10px;padding:16px}
.diag-item .label {color:var(--muted);font-weight:400;margin-bottom:4px}
.diag-item .value {margin-top:4px;font-size:14px;overflow-wrap:anywhere;font-variant-numeric:tabular-nums}
.log {scrollbar-color:#697580 var(--raised)}
.event {gap:20px;padding:12px 0}
.event .when {font-variant-numeric:tabular-nums;font-size:12px;min-width:156px}
footer {margin-top:16px;font-variant-numeric:tabular-nums}
@media (min-width:800px) and (min-height:650px) {
 main {height:auto;min-height:100dvh;display:block;overflow:visible}
 .card {padding:20px 24px;margin-bottom:16px}
 .card:last-of-type {display:block;min-height:0}
 .card:last-of-type .log {max-height:240px}
 .event {padding:12px 0}
}
@media (max-width:600px) {
 main {padding:16px}
 header {gap:8px;margin-bottom:16px}
 h1 {font-size:21px}
 .card {padding:16px;margin-bottom:12px}
 .time-grid {gap:12px}
 .time {font-size:clamp(24px,6vw,34px)}
 .diag-item {padding:12px}
 .controls {gap:8px}
 .controls button {min-height:44px}
 .pill {font-size:11px;padding:6px 8px}
}
@media (prefers-reduced-motion:reduce) {
 #autoPanel,#manualPanel {animation:none}
 button,#controlArea,#transitionBar {transition:none}
}

/* Desktop instrument console: fit the viewport, scroll events only. */
main {max-width:1024px}
.pill.ok {color:var(--green);border-color:#38624f;background:#20352c}
.pill.warn {color:var(--yellow);border-color:#695535;background:#352c20}
.pill.bad {color:var(--red);border-color:#714044;background:#39252a}
.event {justify-content:flex-start;align-items:baseline;gap:16px}
.event .when {order:0;flex:0 0 152px;min-width:0}
.event > div:first-child {order:1;flex:1;min-width:0;text-align:left}
@media (min-width:800px) and (min-height:650px) {
 main {height:100dvh;min-height:0;display:flex;flex-direction:column;overflow:hidden;padding-top:16px;padding-bottom:12px}
 header {flex:none;margin-bottom:12px}
 .card {flex:none;margin-bottom:12px;padding:14px 20px}
 .card:first-of-type .time {font-size:clamp(28px,4vh,36px)}
 .card:first-of-type .time-grid {gap:16px}
 .section-title {margin-bottom:10px}
 #controlArea {min-height:82px;margin-top:8px;padding-top:8px}
 #autoPanel {padding:4px 0}
 #modeState {margin-top:4px}
 .diag {gap:8px}
 .diag-item {padding:10px 12px}
 .diag-item .label {margin-bottom:2px}
 .diag-item .value {margin-top:2px}
 .card:last-of-type {flex:1;min-height:100px;display:flex;flex-direction:column}
 .card:last-of-type .log {flex:1;min-height:0;max-height:none}
 .event {padding:8px 0}
 footer {flex:none;margin-top:0;font-size:11px}
}
@media (min-width:800px) and (max-height:750px) {
 main {padding-top:10px;padding-bottom:8px}
 header {margin-bottom:8px}
 .card {margin-bottom:8px;padding:10px 16px}
 #controlArea {min-height:66px}
 .diag-item {padding:8px 10px}
 .card:last-of-type {min-height:80px}
 .event {padding:6px 0}
}
@media (max-width:600px) {
 .event .when {display:block;flex:none}
 .event > div:first-child {display:block}
}

/* Stable event columns, including relative time before NTP sync. */
.event {display:grid;grid-template-columns:minmax(0,184px) minmax(0,1fr);gap:16px;align-items:start;justify-content:initial}
.event .when {grid-column:1;grid-row:1;order:initial;min-width:0;max-width:100%;white-space:normal;overflow-wrap:anywhere;line-height:1.5}
.event > div:first-child {grid-column:2;grid-row:1;order:initial;min-width:0;overflow-wrap:anywhere;text-align:left;line-height:1.5}
@media (max-width:600px) {
 .event {grid-template-columns:minmax(0,1fr);gap:4px}
 .event .when {grid-column:1;grid-row:1;margin:0}
 .event > div:first-child {grid-column:1;grid-row:2}
}
/* Independent settings dialog with its own scroll; page stays compact. */
.header-actions{display:flex;align-items:stretch;gap:10px}
.header-actions .gear-button{display:flex;align-items:center;justify-content:center;font-size:18px;width:34px;height:auto;min-height:0;padding:0;line-height:1;border-radius:8px;flex:none;align-self:stretch}
.off-note{font-size:13px;color:var(--muted);margin:6px 0 0;background:none;border:0;padding:0}
body.modal-open{overflow:hidden}
.settings-overlay[hidden]{display:none!important}
.settings-overlay{position:fixed;inset:0;z-index:1000;background:rgba(0,0,0,.76);display:flex;align-items:center;justify-content:center;padding:16px}
.settings-dialog{width:min(740px,100%);max-height:min(92dvh,900px);background:var(--panel);border:1px solid var(--line);border-radius:14px;display:flex;flex-direction:column;min-height:0;box-shadow:0 24px 70px rgba(0,0,0,.45)}
.settings-heading{padding:16px 20px;border-bottom:1px solid var(--line);flex-direction:row;align-items:center}
.settings-scroll{overflow-y:auto;overscroll-behavior:contain;min-height:0;padding:18px 20px;flex:1}
.settings-footer{border-top:1px solid var(--line);padding:14px 20px;display:flex;align-items:center;justify-content:space-between;gap:10px;flex-wrap:wrap}
.settings-footer>div{display:flex;gap:8px}
#settingsFields input{max-width:100%}
@media(max-width:600px){.settings-overlay{padding:8px}.settings-dialog{max-height:96dvh}.settings-heading{flex-direction:row}.settings-scroll{padding:12px}.settings-footer{padding:12px}}
/* Subtle severity colors and settings actions. */
.save-button:not(:disabled){background:#284b3b;border-color:#44745a;color:#d6f2e2}
.save-button:hover:not(:disabled){background:#315a46}
.event-warning > div:first-child{color:#d8b574}
.event-critical > div:first-child{color:#d78b8d}
.event-symbol{display:inline-block;margin-right:9px;font-size:14px}
#runBar{transition:width .16s linear}
.mode-card{position:relative}
.fault-actions[hidden]{display:none!important}
.fault-actions{position:absolute;left:50%;bottom:10px;transform:translateX(-50%);display:flex;justify-content:center;z-index:2}
.fault-reset{font-size:12px;padding:6px 12px;background:#4c3034;border-color:#865155;color:#f1d7d9}
.fault-reset:hover{background:#624044}
.calibration-box{margin-top:14px;border-top:1px solid var(--line);padding-top:12px}
.calibration-box .between{align-items:center;gap:8px;flex-wrap:wrap}
.calibration-help{font-size:12px;color:var(--muted);line-height:1.5;margin:8px 0}
.calibration-actions{display:flex;gap:8px;flex-wrap:wrap}
.calibration-actions button{font-size:12px;padding:7px 12px}
#calibrationMessage{display:block;margin-top:8px;color:var(--muted)}
</style>
</head>
<body>
<main>
<header>
  <div>
    <h1>Башенные часы</h1>
    <small><span id="deviceName">LuchClock1</span> · ESP8266</small>
  </div>
  <div class="header-actions"><span class="pill" id="overall" role="status" aria-live="polite">Загрузка...</span><button id="settingsButton" class="gear-button" title="Настройки" aria-label="Настройки" onclick="toggleSettings()">⚙</button></div>
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

<section class="card mode-card">
  <div class="between section-title">
    <div class="mode-heading"><h2>Режим работы</h2><small id="modeHint">Автоматический · ожидание времени</small></div>
    <div class="mode">
      <button id="offBtn" onclick="setMode('off')">Выключен</button>
      <button id="autoBtn" onclick="setMode('auto')">Автоматический</button>
      <button id="manualBtn" onclick="setMode('manual')">Ручной</button>
    </div>
  </div>
  <div id="controlArea"><div id="offPanel" hidden><strong>Двигатель отключён</strong><p class="off-note">Для работы выберите режим.</p></div>
  <div id="autoPanel"><strong id="autoStatus">Ожидание состояния</strong><span id="modeState">Автоматическое движение пока не реализовано</span><span id="motorStatus" hidden>Остановлен</span>
  <div id="autoProgress" hidden><div id="transitionTrack"><div id="autoBar" style="height:100%;width:100%;background:var(--blue);transition:width .1s linear"></div></div></div></div>
  <div id="manualPanel" hidden>
    <div class="controls">
      <button id="backwardBtn" hidden disabled>← Назад</button>
      <button id="stopBtn" class="stop" onclick="sendMotorCommand('stop')">■ Стоп</button>
      <button id="forwardBtn" onclick="sendMotorCommand('forward')">Вперёд →</button>
    </div>
    <div id="runProgress" hidden style="max-width:440px;margin:10px auto"><div class="between"><small>Работа двигателя</small><small id="runTimer">00:00 / 15:00</small></div><div id="transitionTrack"><div id="runBar" style="height:100%;width:0;background:var(--green)"></div></div></div><div id="manualProgress" hidden><div id="transitionTrack"><div id="manualBar" style="height:100%;width:100%;background:var(--blue);transition:width .1s linear"></div></div></div>
    <div class="info" id="commandInfo">
      Тестовый выход D5 / GPIO14: вперёд — 3,3 В, стоп — 0 В.
    </div>
    
  </div>
  </div>
  <div id="faultActions" hidden class="fault-actions"><button id="resetFaultBtn" class="fault-reset" onclick="resetFault()">Сбросить аварию</button></div>
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
    <small>История работы контроллера</small>
  </div>
  <div class="log" id="log">
    <small>Загрузка журнала...</small>
  </div>
</section>

<footer>
  LuchClock v1.11 · Тестовый режим ·
  Время работы: <span id="uptime">—</span>
</footer>
</main>
<div id="settingsModal" class="settings-overlay" hidden onclick="if(event.target===this)toggleSettings()">
 <section class="settings-dialog" role="dialog" aria-modal="true" aria-labelledby="settingsTitle">
  <div class="between settings-heading"><h2 id="settingsTitle">Настройки контроллера</h2><button type="button" onclick="toggleSettings()" aria-label="Закрыть настройки">✕</button></div>
  <div class="settings-scroll"><div id="settingsPanel"><div id="settingsFields" class="diag"></div><div class="info">«Применить» — временно в RAM, «Сохранить» — во flash. Изменять можно только при остановленном двигателе.</div><div class="calibration-box"><div class="between"><strong>Калибровка энкодера</strong><small id="calibrationState">Проверка...</small></div><p class="calibration-help">Установите минутную стрелку на текущее время NTP. Калибровка сохраняется автоматически. Требуется ручной режим и остановленный двигатель.</p><div class="calibration-actions"><button id="calibrationBtn" class="save-button" onclick="runCalibration()" disabled>Откалибровать</button></div><small id="calibrationMessage"></small></div></div></div>
  <div class="settings-footer"><small id="settingsMessage"></small><div><button id="applySettingsBtn" onclick="submitSettings('apply')" disabled>Применить</button> <button id="saveSettingsBtn" class="save-button" onclick="submitSettings('save')" disabled>Сохранить</button></div></div>
 </section>
</div>

<script>
const $ = id => document.getElementById(id);
let currentMode = 'off';
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
  text('deviceName',s.device || 'LuchClock1');
  const calAllowed=s.calibration_allowed===true;
  $('calibrationBtn').disabled=!calAllowed || calibrationBusy || settingsBusy;
  $('calibrationBtn').title=calAllowed?'Откалибровать и сохранить':'Нужен ручной режим, остановленный двигатель, исправный энкодер и актуальный NTP';
  text('calibrationState',s.calibration_saved?'Сохранено · смещение '+s.calibration_offset:'Не откалиброван');
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

    if(s.mode==='off')text('modeHint','Выключен · выход D5 отключён');
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
  text('magnetDetail', s.encoder_connected ? 'Код: ' + s.encoder_raw + ' (0x' + Number(s.encoder_raw).toString(16).toUpperCase().padStart(2,'0') + ')' + (s.encoder_valid ? ' · сырая позиция: ' + s.encoder_raw_position + '/127' : ' · код не распознан') : 'PCF8574 не найден');

  text('uptime', formatUptime(s.uptime_seconds));
  const newlyFaulted=!!s.auto_fault && !(lastStatus && lastStatus.auto_fault);
  lastStatus = s;
  lastStatusAt = Date.now();
  if(newlyFaulted)loadLog();
  updateSettingsButtons();
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
  $('autoBtn').disabled = modeRequestPending || (s.mode !== 'auto' && !s.can_auto);
  $('offBtn').disabled = modeRequestPending || s.mode === 'off';
  $('faultActions').hidden = !(s.mode === 'off' && s.auto_fault);
  $('autoBtn').title = (s.mode === 'manual' && !s.can_auto) ? 'Сначала нажмите Стоп и дождитесь остановки' : '';
  $('manualBtn').disabled = modeRequestPending || (s.mode !== 'manual' && (s.mode_transition || s.motor_state !== 'stopped'));
  $('manualBtn').title = (s.mode === 'auto' && s.mode_transition) ? 'Дождитесь завершения защитной паузы' : '';
  if (s.mode === 'auto') {
    text('autoStatus', s.mode_transition ? 'Остановка · защитная пауза' :
      s.auto_ready ? 'Контроллер готов' : 'Автоматический режим не готов');
    text('modeState', s.mode_transition ? 'Ожидание ' + Math.ceil(s.mode_wait_ms / 1000) + ' с · запуск запрещён' :
      s.auto_ready ? 'Автоматическое движение пока не реализовано' : (s.motor_state === 'stopped' ? 'Автоматическое движение пока не реализовано' : 'Ожидание остановки привода'));
  }
  if (s.mode === 'auto') {
    text('modeHint', s.mode_transition ? 'Переключение · ожидание остановки' :
      s.auto_ready ? 'Автоматический · готов к управлению' : 'Автоматический · не готов');
  } else {
    text('modeHint', s.mode_transition ? 'Ручной · ожидание остановки' : 'Ручное управление · тест GPIO14');
  }

  if (s.ntp_recent) {
    text('overall', 'NTP работает', 'pill ok');
  } else if (s.ntp_synced) {
    text('overall', 'Проверить NTP', 'pill warn');
  } else {
    text('overall', 'Ожидание NTP', 'pill warn');
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
  $('offBtn').classList.toggle('active', mode === 'off');
  $('autoBtn').classList.toggle('active', mode === 'auto');
  $('manualBtn').classList.toggle('active', mode === 'manual');
  $('manualPanel').hidden = mode !== 'manual';
  $('autoPanel').hidden = mode !== 'auto';
  $('offPanel').hidden = mode !== 'off';
  text('modeHint', mode === 'manual' ? 'Ручное управление · тест GPIO14' : mode === 'off' ? 'Выключен · выход D5 отключён' : 'Автоматический режим');
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
  const run = s.mode==='manual' && s.motor_state==='forward';
  $('runProgress').hidden=!run;
  if(run){
    const ms=Math.min(s.motor_limit_ms,s.motor_run_ms+Math.max(0,Date.now()-lastStatusAt));
    const fmt=v=>String(Math.floor(v/60000)).padStart(2,'0')+':'+String(Math.floor(v/1000)%60).padStart(2,'0');
    text('runTimer',fmt(ms)+' / '+fmt(s.motor_limit_ms));
    $('runBar').style.width=Math.min(100,100*ms/s.motor_limit_ms)+'%';
  }
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
  $('stopBtn').disabled = commandPending || !statusKnown || !motorPermissions.can_stop || (state !== 'forward' && state !== 'reverse');
  if (s) {
    $('autoBtn').disabled=modeRequestPending || !s.encoder_valid || !s.can_auto;
    $('manualBtn').disabled=modeRequestPending || !s.encoder_valid || !!s.auto_fault || (s.mode!=='manual' && (s.mode_transition || s.motor_state!=='stopped'));
  }
  if (s && s.mode === 'manual') {
    $('autoBtn').disabled = modeRequestPending || !s.can_auto || state !== 'stopped';
  }
  $('forwardBtn').classList.toggle('running', state === 'forward');
  $('backwardBtn').classList.toggle('running', state === 'reverse');
  if(s && s.manual_pause_ms>0)text('commandInfo','Пауза после лимита · '+Math.ceil(s.manual_pause_ms/1000)+' с');
  else if (state === 'fault') text('commandInfo', 'Авария привода · управление заблокировано');
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
      text('overall', 'Связь с ESP нестабильна', 'pill bad');
      text('wifiDetail', 'Нет ответа API · состояние Wi-Fi неизвестно');
      motorPermissions = {can_forward:false,can_reverse:false,can_stop:true};
      statusKnown = false;
      $('calibrationBtn').disabled=true;
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
  if (mode === 'auto' && (!lastStatus || !lastStatus.encoder_valid || !lastStatus.can_auto || lastMotorState !== 'stopped')) return;
  if (mode === 'manual' && (!lastStatus || !lastStatus.encoder_valid || lastStatus.mode_transition || lastMotorState !== 'stopped')) return;
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
      $('autoBtn').disabled = !lastStatus.encoder_valid || !lastStatus.can_auto;
      $('manualBtn').disabled = !lastStatus.encoder_valid || lastStatus.auto_fault ||
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
      const warning=/лимит ручной работы|защитная пауза/i.test(event.message);
      const critical=/аварийн|критическ|потеря связи с энкодером|энкодер недоступен|не фиксирует движение/i.test(event.message);
      const item = document.createElement('div');
      item.className = 'event';
      const title = document.createElement('div');
      title.textContent = event.message;
      if(warning||critical){
        item.classList.add(critical?'event-critical':'event-warning');
        const mark=document.createElement('span');
        mark.className='event-symbol';mark.textContent=critical?'✖':'⚠';
        title.prepend(mark);
      }
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


const settingFields=[
 ['manualLimitSec','Лимит ручного режима, сек',1,900],
 ['autoLimitSec','Лимит авторежима, сек',1,900],
 ['manualPauseSec','Пауза после ручного лимита, сек',1,300],
 ['calibrationGuardSec','Запрет автокалибровки до и после смены минуты, сек',0,29],
 ['noMotionWarnSec','Аварийная остановка без движения энкодера, сек',1,900],
 ['autoStepMin','Шаг синхронизации, мин',1,10],
 ['wifiReconnectMs','Повтор Wi-Fi, мс',1000,300000],
 ['wifiDiagnosticMs','Диагностика Wi-Fi, мс',1000,300000],
 ['ntpStaleSec','Устаревание NTP, сек',60,86400],
 ['encoderPollMs','Опрос энкодера, мс',5,100],
 ['encoderProbeMs','Повтор поиска PCF8574, мс',250,10000],
 ['encoderStableSamples','Подтверждающих чтений энкодера',2,5]
];
let settingsLoaded=false;
let appliedSettings=null;
let savedSettings=false;
let settingsBusy=false;
let settingsOpenGeneration=0;
function currentSettings(){const result={};for(const [key] of settingFields)result[key]=Number($('setting_'+key).value);return result;}
function updateSettingsButtons(){
  if(!settingsLoaded)return;
  const changed=JSON.stringify(currentSettings())!==JSON.stringify(appliedSettings);
  const blocked=settingsBusy || !lastStatus || lastStatus.mode==='auto' || lastStatus.motor_state!=='stopped' || !!lastStatus.mode_transition;
  $('applySettingsBtn').disabled=blocked || !changed;
  $('saveSettingsBtn').disabled=blocked || (!changed && savedSettings);
}
function toggleSettings(){
  const p=$('settingsModal');
  settingsOpenGeneration++;
  text('calibrationMessage','');
  if(p.hidden){
    p.hidden=false;document.body.classList.add('modal-open');
    settingsLoaded=false;
    text('settingsMessage','');
    loadSettings();
  }else{
    p.hidden=true;document.body.classList.remove('modal-open');
    settingsLoaded=false;appliedSettings=null;
    $('settingsFields').replaceChildren();
    text('settingsMessage','');
  }
}
async function loadSettings(){
  const generation=settingsOpenGeneration;
  try{
    const response=await fetch('/api/settings',{cache:'no-store'});
    if(!response.ok)throw Error('HTTP '+response.status);
    const data=await response.json();
    if(generation!==settingsOpenGeneration || $('settingsModal').hidden)return;
    const root=$('settingsFields');root.replaceChildren();
    for(const [key,label,min,max] of settingFields){
      const item=document.createElement('label');item.className='diag-item';
      const title=document.createElement('div');title.className='label';title.textContent=label;
      const input=document.createElement('input');input.type='number';input.id='setting_'+key;
      input.min=min;input.max=max;input.step=1;input.value=data[key];
      input.style.cssText='width:100%;background:#111720;color:var(--text);border:1px solid var(--line);border-radius:6px;padding:8px';
      input.addEventListener('input',updateSettingsButtons);
      item.append(title,input);root.appendChild(item);
    }
    settingsLoaded=true;appliedSettings=currentSettings();savedSettings=!!data._saved;
    text('settingsMessage','');
    updateSettingsButtons();
  }catch(e){text('settingsMessage','Ошибка: '+e.message)}
}
async function submitSettings(operation){
  if(!settingsLoaded || settingsBusy)return;
  updateSettingsButtons();
  if($(operation==='save'?'saveSettingsBtn':'applySettingsBtn').disabled)return;
  const args=new URLSearchParams();
  for(const [key,label,min,max] of settingFields){
    const raw=$('setting_'+key).value;
    const n=Number(raw);
    if(!/^\d+$/.test(raw)||!Number.isInteger(n)||n<min||n>max){
      text('settingsMessage','Проверьте: '+label);return;
    }
    args.set(key,raw);
  }
  args.set('operation',operation);
  settingsBusy=true;updateSettingsButtons();
  try{
    const response=await fetch('/api/settings',{method:'POST',body:args});
    if(!response.ok)throw Error(await response.text());
    appliedSettings=currentSettings();
    savedSettings=operation==='save';
    await refresh();loadLog();
    await loadSettings();
    text('settingsMessage',operation==='save'?'Сохранено во flash':'Применено временно');
  }catch(e){text('settingsMessage','Ошибка: '+e.message)}
  finally{settingsBusy=false;updateSettingsButtons();}
}
let calibrationBusy=false;
async function runCalibration(){
  if(calibrationBusy || $('calibrationBtn').disabled)return;
  calibrationBusy=true;$('calibrationBtn').disabled=true;
  try{
    const response=await fetch('/api/calibration',{method:'POST'});
    if(!response.ok)throw Error(await response.text());
    text('calibrationMessage','Калибровка сохранена во flash.');
    loadLog();
  }catch(e){text('calibrationMessage',e.message)}
  finally{calibrationBusy=false;await refresh();}
}
async function resetFault(){
  try{await post('/api/reset-fault');await refresh();loadLog()}
  catch(e){alert(e.message)}
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
