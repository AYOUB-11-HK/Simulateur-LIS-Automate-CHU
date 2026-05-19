#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

ESP8266WebServer server(80);

String state     = "EN ATTENTE";
String echantillon = "--";
String analyse   = "--";
String resultat  = "--";
int    progress  = 0;
int    etape     = 1;

bool   nouvelleCommande = false;
String cmdId   = "";
String cmdType = "";

// =========================================================================
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>LIS — Automate ASTM</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

    :root {
      --navy:    #0b1622;
      --panel:   #111d2c;
      --card:    #162032;
      --border:  #1e3a5f;
      --accent:  #1a8cff;
      --accent2: #0d6ecc;
      --green:   #00c896;
      --amber:   #f5a623;
      --red:     #e84040;
      --text1:   #e8f0fe;
      --text2:   #7a9bbf;
      --text3:   #3d5c7a;
      --mono:    'Courier New', 'Lucida Console', monospace;
      --sans:    system-ui, -apple-system, 'Segoe UI', sans-serif;
    }

    body {
      font-family: var(--sans);
      background: var(--navy);
      color: var(--text1);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
    }

    /* ── HEADER ─────────────────────────────────────────────── */
    header {
      background: var(--panel);
      border-bottom: 1px solid var(--border);
      padding: 0 24px;
      height: 56px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      position: sticky;
      top: 0;
      z-index: 10;
    }
    .header-left {
      display: flex;
      align-items: center;
      gap: 14px;
    }
    .logo-mark {
      width: 34px; height: 34px;
      background: var(--accent);
      border-radius: 8px;
      display: flex; align-items: center; justify-content: center;
      font-family: var(--mono);
      font-weight: 700;
      font-size: 13px;
      letter-spacing: -0.5px;
      color: #fff;
    }
    .app-title {
      font-size: 15px;
      font-weight: 600;
      letter-spacing: 0.3px;
      color: var(--text1);
    }
    .app-sub {
      font-size: 11px;
      color: var(--text2);
      letter-spacing: 0.8px;
      text-transform: uppercase;
      margin-top: 1px;
    }
    .header-status {
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .status-dot {
      width: 8px; height: 8px;
      border-radius: 50%;
      background: var(--green);
      animation: pulse 2s infinite;
    }
    @keyframes pulse {
      0%,100% { opacity: 1; }
      50%      { opacity: 0.4; }
    }
    .status-text {
      font-size: 12px;
      font-family: var(--mono);
      color: var(--green);
      letter-spacing: 0.5px;
    }
    .ip-badge {
      font-size: 11px;
      font-family: var(--mono);
      color: var(--text2);
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 5px;
      padding: 3px 8px;
    }

    /* ── MAIN LAYOUT ─────────────────────────────────────────── */
    main {
      flex: 1;
      padding: 24px 20px;
      max-width: 1100px;
      width: 100%;
      margin: 0 auto;
      display: flex;
      flex-direction: column;
      gap: 20px;
    }

    /* ── SECTION LABEL ───────────────────────────────────────── */
    .section-label {
      font-size: 10px;
      font-family: var(--mono);
      letter-spacing: 2px;
      text-transform: uppercase;
      color: var(--accent);
      margin-bottom: 12px;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .section-label::after {
      content: '';
      flex: 1;
      height: 1px;
      background: var(--border);
    }

    /* ── TWO COLUMN ──────────────────────────────────────────── */
    .two-col {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 20px;
    }
    @media (max-width: 720px) {
      .two-col { grid-template-columns: 1fr; }
    }

    /* ── PANEL CARD ──────────────────────────────────────────── */
    .panel {
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: 14px;
      padding: 20px;
      display: flex;
      flex-direction: column;
      gap: 14px;
    }
    .panel-title {
      font-size: 13px;
      font-weight: 600;
      color: var(--text1);
      letter-spacing: 0.3px;
    }
    .panel-icon {
      width: 30px; height: 30px;
      border-radius: 8px;
      display: flex; align-items: center; justify-content: center;
      font-size: 15px;
    }
    .panel-icon.blue  { background: rgba(26,140,255,0.15); color: var(--accent); }
    .panel-icon.green { background: rgba(0,200,150,0.15);  color: var(--green); }
    .panel-head {
      display: flex;
      align-items: center;
      gap: 10px;
      padding-bottom: 14px;
      border-bottom: 1px solid var(--border);
    }

    /* ── FORM ELEMENTS ───────────────────────────────────────── */
    .field-group {
      display: flex;
      flex-direction: column;
      gap: 5px;
    }
    .field-label {
      font-size: 11px;
      text-transform: uppercase;
      letter-spacing: 1px;
      color: var(--text2);
    }
    .field-input {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 8px;
      color: var(--text1);
      font-family: var(--mono);
      font-size: 14px;
      padding: 10px 12px;
      width: 100%;
      outline: none;
      transition: border-color 0.2s;
    }
    .field-input:focus {
      border-color: var(--accent);
    }
    .field-input option {
      background: var(--card);
    }

    .btn-send {
      margin-top: 4px;
      padding: 11px 0;
      background: var(--accent);
      border: none;
      border-radius: 9px;
      color: #fff;
      font-family: var(--sans);
      font-size: 14px;
      font-weight: 600;
      cursor: pointer;
      width: 100%;
      letter-spacing: 0.3px;
      transition: background 0.2s, transform 0.1s;
    }
    .btn-send:hover  { background: var(--accent2); }
    .btn-send:active { transform: scale(0.98); }

    .btn-send.sending {
      background: var(--text3);
      pointer-events: none;
    }

    /* ── METRIC CARDS ────────────────────────────────────────── */
    .metrics {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr;
      gap: 10px;
    }
    .metric {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 12px;
    }
    .metric-label {
      font-size: 10px;
      font-family: var(--mono);
      text-transform: uppercase;
      letter-spacing: 1.2px;
      color: var(--text2);
      margin-bottom: 6px;
    }
    .metric-value {
      font-size: 18px;
      font-family: var(--mono);
      font-weight: 700;
      color: var(--text1);
      word-break: break-all;
    }
    .metric-value.result { color: var(--green); }
    .metric-value.pending { color: var(--amber); }

    /* ── PROGRESS BAR ────────────────────────────────────────── */
    .prog-wrap {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 14px 16px;
    }
    .prog-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 10px;
    }
    .prog-label {
      font-size: 11px;
      text-transform: uppercase;
      letter-spacing: 1px;
      color: var(--text2);
    }
    .prog-pct {
      font-family: var(--mono);
      font-size: 13px;
      color: var(--green);
    }
    .prog-track {
      height: 6px;
      background: var(--border);
      border-radius: 3px;
      overflow: hidden;
    }
    .prog-fill {
      height: 100%;
      width: 0%;
      background: linear-gradient(90deg, var(--accent), var(--green));
      border-radius: 3px;
      transition: width 0.5s ease;
    }

    /* ── STATUS BAR ──────────────────────────────────────────── */
    .state-bar {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 10px 16px;
      display: flex;
      align-items: center;
      gap: 10px;
    }
    .state-indicator {
      width: 6px; height: 6px;
      border-radius: 50%;
      background: var(--green);
      flex-shrink: 0;
    }
    .state-indicator.active { background: var(--amber); animation: pulse 0.8s infinite; }
    .state-value {
      font-family: var(--mono);
      font-size: 13px;
      font-weight: 700;
      letter-spacing: 1px;
      color: var(--text1);
    }

    /* ── TIMELINE ────────────────────────────────────────────── */
    .timeline-wrap {
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: 14px;
      padding: 20px;
    }
    .timeline {
      display: flex;
      align-items: flex-start;
      justify-content: space-between;
      position: relative;
      margin-top: 8px;
    }
    .tl-track {
      position: absolute;
      top: 16px;
      left: calc(40px / 2);
      right: calc(40px / 2);
      height: 2px;
      background: var(--border);
    }
    .tl-fill {
      height: 100%;
      background: var(--green);
      width: 0%;
      transition: width 0.6s ease;
    }
    .step {
      position: relative;
      z-index: 2;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 8px;
      flex: 1;
    }
    .step-circle {
      width: 34px; height: 34px;
      border-radius: 50%;
      background: var(--card);
      border: 2px solid var(--border);
      display: flex; align-items: center; justify-content: center;
      font-family: var(--mono);
      font-size: 13px;
      font-weight: 700;
      color: var(--text2);
      transition: all 0.3s ease;
    }
    .step-label {
      font-size: 10px;
      text-align: center;
      color: var(--text2);
      max-width: 70px;
      line-height: 1.4;
      letter-spacing: 0.3px;
    }
    .step.done .step-circle {
      background: rgba(0,200,150,0.15);
      border-color: var(--green);
      color: var(--green);
    }
    .step.done .step-label { color: var(--green); }
    .step.active .step-circle {
      background: rgba(245,166,35,0.15);
      border-color: var(--amber);
      color: var(--amber);
      box-shadow: 0 0 12px rgba(245,166,35,0.25);
    }
    .step.active .step-label { color: var(--amber); }

    /* ── LOG AREA ────────────────────────────────────────────── */
    .log-area {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 12px 14px;
      font-family: var(--mono);
      font-size: 12px;
      color: var(--text2);
      max-height: 90px;
      overflow-y: auto;
      line-height: 1.8;
    }
    .log-entry { color: var(--text2); }
    .log-entry.ok   { color: var(--green); }
    .log-entry.warn { color: var(--amber); }
    .log-entry.info { color: var(--accent); }

    /* ── FOOTER ──────────────────────────────────────────────── */
    footer {
      text-align: center;
      padding: 16px;
      font-size: 11px;
      font-family: var(--mono);
      color: var(--text3);
      border-top: 1px solid var(--border);
    }
  </style>
</head>
<body>

  <header>
    <div class="header-left">
      <div class="logo-mark">LIS</div>
      <div>
        <div class="app-title">Laboratoire Information System</div>
        <div class="app-sub">Interface Automate ASTM</div>
      </div>
    </div>
    <div class="header-status">
      <span class="ip-badge" id="ipAddr">192.168.4.1</span>
      <div class="status-dot" id="statusDot"></div>
      <span class="status-text" id="statusText">SYSTÈME EN LIGNE</span>
    </div>
  </header>

  <main>

    <!-- ── ROW 1 : Saisie LIS + Traitement Automate ─────────── -->
    <div class="two-col">

      <!-- Panel Saisie LIS -->
      <div class="panel">
        <div class="panel-head">
          <div class="panel-icon blue">&#9999;</div>
          <div>
            <div class="panel-title">Saisie LIS</div>
            <div style="font-size:11px;color:var(--text2);margin-top:2px;">Prescription médecin</div>
          </div>
        </div>

        <div class="field-group">
          <div class="field-label">N° Échantillon</div>
          <input class="field-input" type="text" id="inputId" placeholder="ex : 456" value="456" maxlength="20">
        </div>

        <div class="field-group">
          <div class="field-label">Type d'analyse</div>
          <select class="field-input" id="inputType">
            <option value="GLUCOSE">Glucose</option>
            <option value="CHOLESTEROL">Cholestérol</option>
            <option value="UREE">Urée</option>
          </select>
        </div>

        <div class="field-group">
          <div class="field-label">Prescripteur (optionnel)</div>
          <input class="field-input" type="text" id="inputDoc" placeholder="Dr. Nom Prénom">
        </div>

        <button class="btn-send" id="btnSend" onclick="envoyerPrescription()">
          &#9654; Envoyer l'ordre à l'Automate
        </button>

        <div class="log-area" id="logArea">
          <div class="log-entry info">&gt; LIS démarré — en attente d'ordres...</div>
        </div>
      </div>

      <!-- Panel Traitement Automate -->
      <div class="panel">
        <div class="panel-head">
          <div class="panel-icon green">&#9881;</div>
          <div>
            <div class="panel-title">Traitement Automate</div>
            <div style="font-size:11px;color:var(--text2);margin-top:2px;">Résultats en temps réel</div>
          </div>
        </div>

        <div class="state-bar">
          <div class="state-indicator" id="stateInd"></div>
          <div class="state-value" id="stateBadge">EN ATTENTE</div>
        </div>

        <div class="metrics">
          <div class="metric">
            <div class="metric-label">Échantillon</div>
            <div class="metric-value" id="echID">--</div>
          </div>
          <div class="metric">
            <div class="metric-label">Analyse</div>
            <div class="metric-value" id="anaType">--</div>
          </div>
          <div class="metric">
            <div class="metric-label">Résultat</div>
            <div class="metric-value result" id="resultVal">--</div>
          </div>
        </div>

        <div class="prog-wrap">
          <div class="prog-header">
            <span class="prog-label">Progression analyse</span>
            <span class="prog-pct" id="progPct">0 %</span>
          </div>
          <div class="prog-track">
            <div class="prog-fill" id="progressBar"></div>
          </div>
        </div>

        <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;">
          <div class="metric">
            <div class="metric-label">Étape</div>
            <div class="metric-value" id="etapeVal">1 / 5</div>
          </div>
          <div class="metric">
            <div class="metric-label">Horodatage</div>
            <div class="metric-value" id="timeVal" style="font-size:12px;">--:--:--</div>
          </div>
        </div>
      </div>

    </div>

    <!-- ── ROW 2 : Timeline ──────────────────────────────────── -->
    <div class="timeline-wrap">
      <div class="section-label">Flux de traitement ASTM</div>
      <div class="timeline">
        <div class="tl-track"><div class="tl-fill" id="tlFill"></div></div>
        <div class="step" id="s1">
          <div class="step-circle">01</div>
          <div class="step-label">Prescription LIS</div>
        </div>
        <div class="step" id="s2">
          <div class="step-circle">02</div>
          <div class="step-label">Réception Automate</div>
        </div>
        <div class="step" id="s3">
          <div class="step-circle">03</div>
          <div class="step-label">Analyse en cours</div>
        </div>
        <div class="step" id="s4">
          <div class="step-circle">04</div>
          <div class="step-label">Résultat prêt</div>
        </div>
        <div class="step" id="s5">
          <div class="step-circle">05</div>
          <div class="step-label">Validation LIS</div>
        </div>
      </div>
    </div>

  </main>

  <footer>LIS v2.0 &nbsp;|&nbsp; Automate ASTM &nbsp;|&nbsp; ESP8266 AP &mdash; CHU_Automate_Pro</footer>

  <script>
    var lastEtape = 1;

    function timestamp() {
      var d = new Date();
      var pad = function(n){ return n < 10 ? '0'+n : n; };
      return pad(d.getHours())+':'+pad(d.getMinutes())+':'+pad(d.getSeconds());
    }

    function addLog(msg, cls) {
      var log = document.getElementById('logArea');
      var div = document.createElement('div');
      div.className = 'log-entry ' + (cls || '');
      div.textContent = '[' + timestamp() + '] ' + msg;
      log.appendChild(div);
      log.scrollTop = log.scrollHeight;
    }

    function envoyerPrescription() {
      var id   = document.getElementById('inputId').value.trim();
      var type = document.getElementById('inputType').value;
      var doc  = document.getElementById('inputDoc').value.trim();
      if (!id) { addLog('Erreur : numéro échantillon requis.', 'warn'); return; }
      var btn = document.getElementById('btnSend');
      btn.textContent = '↻ Envoi en cours...';
      btn.classList.add('sending');
      addLog('Ordre envoyé — ID:' + id + ' / ' + type + (doc ? ' / ' + doc : ''), 'info');
      fetch('/api/order?id=' + encodeURIComponent(id) + '&type=' + type)
        .then(function(){ setTimeout(function(){ btn.textContent = '▶ Envoyer l\'ordre à l\'Automate'; btn.classList.remove('sending'); }, 1500); })
        .catch(function(){ addLog('Erreur réseau.', 'warn'); btn.textContent = '▶ Envoyer l\'ordre à l\'Automate'; btn.classList.remove('sending'); });
    }

    function fetchData() {
      fetch('/api/data')
        .then(function(r){ return r.json(); })
        .then(function(d) {
          document.getElementById('echID').textContent    = d.echantillon;
          document.getElementById('anaType').textContent  = d.analyse;
          document.getElementById('resultVal').textContent= d.resultat;
          document.getElementById('progressBar').style.width = d.progress + '%';
          document.getElementById('progPct').textContent  = d.progress + ' %';
          document.getElementById('stateBadge').textContent = d.state;
          document.getElementById('etapeVal').textContent = d.etape + ' / 5';
          document.getElementById('timeVal').textContent  = timestamp();

          var ind = document.getElementById('stateInd');
          ind.className = 'state-indicator' + (d.etape > 1 && d.etape < 5 ? ' active' : '');

          for (var i = 1; i <= 5; i++) {
            var s = document.getElementById('s' + i);
            s.className = 'step' + (i < d.etape ? ' done' : (i == d.etape ? ' active' : ''));
          }
          var pct = ((d.etape - 1) / 4) * 100;
          document.getElementById('tlFill').style.width = pct + '%';

          if (d.etape !== lastEtape) {
            var msgs = ['', 'Prescription reçue', 'Bien reçu par l\'automate', 'Analyse démarrée', 'Résultat disponible', 'Validé — envoyé au LIS'];
            if (msgs[d.etape]) addLog(msgs[d.etape], d.etape === 5 ? 'ok' : 'info');
            lastEtape = d.etape;
          }
        })
        .catch(function(){});
    }

    setInterval(fetchData, 1000);
  </script>
</body>
</html>
)=====";
// =========================================================================

void handleRoot()  { server.send(200, "text/html", MAIN_page); }

void handleData() {
  String json = "{\"state\":\"" + state
              + "\",\"echantillon\":\"" + echantillon
              + "\",\"analyse\":\"" + analyse
              + "\",\"resultat\":\"" + resultat
              + "\",\"progress\":" + String(progress)
              + ",\"etape\":" + String(etape) + "}";
  server.send(200, "application/json", json);
}

void handleOrder() {
  cmdId   = server.arg("id");
  cmdType = server.arg("type");
  nouvelleCommande = true;
  server.send(200, "text/plain", "OK");
}

void updateOLED() {
  display.clearDisplay();
  display.fillRect(0, 0, 128, 12, WHITE);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(2, 2);
  display.print("IP: "); display.print(WiFi.softAPIP().toString());

  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, 16);
  display.print("Etat: "); display.println(state);
  display.drawLine(0, 26, 128, 26, WHITE);
  display.setCursor(0, 30);
  display.print("Ech: ");  display.println(echantillon);
  display.print("Type: "); display.println(analyse);

  if (progress > 0) {
    display.drawRect(0, 50, 128, 10, WHITE);
    int fillWidth = map(progress, 0, 100, 0, 124);
    display.fillRect(2, 52, fillWidth, 6, WHITE);
  } else {
    display.setCursor(0, 50);
    display.print("Res: "); display.print(resultat);
  }
  display.display();
}

void attenteActive(int tempsMs) {
  unsigned long debut = millis();
  while (millis() - debut < tempsMs) {
    server.handleClient();
    yield();
    delay(10);
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin(12, 14);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erreur OLED");
    for (;;);
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP("CHU_Automate_Pro", "12345678");

  server.on("/",          handleRoot);
  server.on("/api/data",  handleData);
  server.on("/api/order", handleOrder);
  server.begin();

  randomSeed(analogRead(0));
  updateOLED();
}

void loop() {
  server.handleClient();

  if (nouvelleCommande) {
    nouvelleCommande = false;

    // ── ETAPE 2 : Réception
    etape      = 2;
    state      = "BIEN RECU";
    echantillon = cmdId;
    analyse    = cmdType;
    resultat   = "...";
    updateOLED();
    attenteActive(2000);

    // ── ETAPE 3 : Analyse
    etape = 3;
    state = "ANALYSE EN COURS";
    for (int i = 0; i <= 100; i += 5) {
      progress = i;
      updateOLED();
      attenteActive(200);
    }

    float valeur = 0.0;
    if      (cmdType == "GLUCOSE")     valeur = random(70,  120) / 100.0;
    else if (cmdType == "CHOLESTEROL") valeur = random(150, 240) / 100.0;
    else                               valeur = random(20,  50)  / 10.0;

    // ── ETAPE 4 : Résultat
    etape    = 4;
    progress = 0;
    state    = "RESULTAT PRET";
    resultat = String(valeur) + " g/L";
    updateOLED();

    // Envoi au Arduino (LIS) via Serial
    Serial.println("R|" + cmdId + "|" + cmdType + "|" + resultat);
    attenteActive(2000);

    // ── ETAPE 5 : Validation
    etape = 5;
    state = "VALIDE - LIS";
    updateOLED();
    attenteActive(3000);

    // ── Retour état initial
    etape      = 1;
    state      = "EN ATTENTE";
    echantillon = "--";
    analyse    = "--";
    resultat   = "--";
    updateOLED();
  }
}
