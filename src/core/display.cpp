#include "core/display.h"
#include "system/config.h"
#include <WiFi.h>

DisplayOutputs &DisplayOutputs::get() {
  static DisplayOutputs instance;
  return instance;
}

void DisplayOutputs::initialize() {
  // 1. Initialize LCD
  Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcdPrint("Starting up...", "Please wait.");

  // 2. Connect to WiFi
  Serial.println("Connecting to Router...");
  const char *ssid = "iss";
  const char *password = "robincarl";
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  SystemConfig &config = SystemConfig::get();

  if (WiFi.status() == WL_CONNECTED) {
    config.serverIP = WiFi.localIP().toString();
    Serial.print("✅ Web Server IP: http://");
    Serial.println(config.serverIP);
    lcdPrint("WiFi Connected!", "IP: " + config.serverIP);
  } else {
    Serial.println("❌ WiFi Failed.");
    lcdPrint("WiFi Failed!", "Check Network.");
  }

  delay(2000); 

  // 3. Start Web Server
  setupRoutes();
  server.begin();
}

// Helper to quickly print to the LCD
void DisplayOutputs::lcdPrint(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1.substring(0, 16)); 
  lcd.setCursor(0, 1);
  lcd.print(line2.substring(0, 16));
}

void DisplayOutputs::handleClient() { server.handleClient(); }

void DisplayOutputs::setupRoutes() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/start", HTTP_POST, [this]() { handleStart(); });
  server.on("/status", HTTP_GET, [this]() { handleStatus(); });
}

void DisplayOutputs::handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Hallway Dashboard</title>
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; background-color: #000000; color: #ffffff; margin: 0; padding: 20px; display: flex; flex-direction: column; align-items: center; min-height: 100vh; box-sizing: border-box; }
    .container { width: 100%; max-width: 360px; text-align: center; }
    h1 { font-size: 24px; font-weight: bold; margin-bottom: 10px; margin-top: 20px; }
    h2 { font-size: 14px; font-weight: normal; color: #aaaaaa; margin-top: 0; margin-bottom: 30px; }
    .btn { display: block; width: 100%; padding: 18px 0; margin-bottom: 15px; font-size: 18px; background-color: transparent; color: #ffffff; border: 2px solid #ffffff; border-radius: 0; cursor: pointer; box-sizing: border-box; }
    .btn:active { background-color: #333333; }
    .hidden { display: none; }
    .timer-box { border: 2px solid #ffffff; padding: 30px 10px; margin: 30px 0; }
    .timer { font-size: 56px; font-weight: bold; color: #ff3333; margin: 0; font-variant-numeric: tabular-nums; }
    .success-text { font-size: 36px; font-weight: bold; color: #00ff00; margin-top: 50px; }
    .error-text { font-size: 36px; font-weight: bold; color: #ff0000; margin-top: 50px; }
    .sub-text { color: #aaaaaa; font-size: 14px; margin-top: 20px; }
    .rules-list { text-align: left; font-size: 14px; color: #aaaaaa; padding-left: 10px; }
    .rules-list li { margin-bottom: 10px; }
    .warning-text { color: #ff0000; }
    .info-box { background-color: #222222; padding: 15px; margin-bottom: 20px; border: 1px solid #444; }
    .info-label { font-size: 12px; color: #888888; margin-bottom: 5px; }
    .info-value { font-size: 18px; font-weight: bold; margin-bottom: 10px; }
  </style>
</head>
<body>
  <div class="container">
    <div id="scan-pass-screen" class="hidden">
      <h1>Hallway Pass</h1>
      <h2>Tap your Hallway Pass Card.</h2>
      <div class="timer-box" style="border-color: #00ff00; padding: 20px;">
        <h3 style="margin: 0; color: #00ff00;">TAP PASS CARD</h3>
      </div>
    </div>

    <div id="home-screen" class="hidden">
      <h1>Dashboard</h1>
      <h2 id="welcome-text">Select your destination:</h2>
      <button class="btn" onclick="startTimer('Clinic')">Clinic (20m)</button>
      <button class="btn" onclick="startTimer('Restroom')">Restroom (1m)</button>
      <button class="btn" onclick="startTimer('Faculty')">Faculty (15m)</button>
    </div>

    <div id="scan-id-screen" class="hidden">
      <h1 id="dest-title">Clinic</h1>
      <h2>Rules & Reminders</h2>
      <ul class="rules-list">
        <li>1. Walk, do not run.</li>
        <li>2. Return within the time limit.</li>
        <li class="warning-text">3. Failure to return is a violation.</li>
      </ul>
      <div class="timer-box" style="border-color: #ffaa00; padding: 20px;">
        <h3 style="margin: 0; color: #ffaa00;">TAP STUDENT ID</h3>
        <p style="margin: 10px 0 0 0; color: #aaaaaa; font-size: 12px;">Verify identity to start.</p>
      </div>
    </div>

    <div id="countdown-screen" class="hidden">
      <h1>Active Pass</h1>
      
      <!-- ADDED INFO BOX HERE -->
      <div class="info-box">
        <div class="info-label">STUDENT</div>
        <div class="info-value" id="info-name">Loading...</div>
        
        <div class="info-label">GENDER</div>
        <div class="info-value" id="info-gender">Loading...</div>
        
        <div class="info-label">DESTINATION</div>
        <div class="info-value" id="info-dest">Loading...</div>
      </div>

      <div class="timer-box"><p class="timer" id="timer-display">00:00</p></div>
      <p class="sub-text">Return the pass card before time runs out.</p>
    </div>

    <div id="result-screen" class="hidden">
      <h1 id="result-title" class="success-text">PASS RECORDED</h1>
      <p id="result-sub" class="sub-text">Wait...</p>
      
      <!-- SHOW INFO ON RESULT SCREEN TOO -->
      <div class="info-box" style="margin-top: 30px;">
        <div class="info-label">STUDENT</div>
        <div class="info-value" id="result-name">-</div>
        <div class="info-label">DESTINATION</div>
        <div class="info-value" id="result-dest">-</div>
      </div>
    </div>
  </div>

  <script>
    let currentName = "";
    let currentGender = "";
    let currentDest = "";

    function showScreen(id) {
      document.querySelectorAll('div[id$="-screen"]').forEach(el => el.classList.add('hidden'));
      document.getElementById(id).classList.remove('hidden');
    }

    function startTimer(dest) {
      showScreen('scan-id-screen');
      document.getElementById('dest-title').innerText = dest;
      currentDest = dest;
      fetch('/start', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'dest=' + encodeURIComponent(dest)
      });
    }

    setInterval(() => {
      fetch('/status').then(res => res.text()).then(data => {
        // The status now sends comma-separated data: STATE, NAME, GENDER, TIME
        let parts = data.split(",");
        let state = parts[0];

        if (state === "SCAN_PASS") showScreen('scan-pass-screen');
        else if (state === "HOME") showScreen('home-screen');
        else if (state === "SCAN_ID") showScreen('scan-id-screen');
        else if (state === "COUNTDOWN") {
          showScreen('countdown-screen');
          
          currentName = parts[1] || "Unknown";
          currentGender = parts[2] || "Unknown";
          let time = parts[3] || "00:00";

          document.getElementById('info-name').innerText = currentName;
          document.getElementById('info-gender').innerText = currentGender;
          document.getElementById('info-dest').innerText = currentDest;
          document.getElementById('timer-display').innerText = time;
        } 
        else if (state === "SUCCESS") {
          showScreen('result-screen');
          document.getElementById('result-title').className = "success-text";
          document.getElementById('result-title').innerText = "PASS RECORDED";
          document.getElementById('result-sub').innerText = "Returned within time.";
          
          document.getElementById('result-name').innerText = currentName;
          document.getElementById('result-dest').innerText = currentDest;
        } 
        else if (state === "ERROR") {
          showScreen('result-screen');
          document.getElementById('result-title').className = "error-text";
          document.getElementById('result-title').innerText = "VIOLATION";
          document.getElementById('result-sub').innerText = "Time limit exceeded.";
          
          document.getElementById('result-name').innerText = currentName;
          document.getElementById('result-dest').innerText = currentDest;
        } 
      }); 
    }, 1000); 
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void DisplayOutputs::handleStart() {
  SystemConfig &config = SystemConfig::get();
  if (server.hasArg("dest")) {
    config.setDestination(server.arg("dest"));
    config.currentState = DisplayState::SCAN_ID; 
  }
  server.send(200, "text/plain", "OK");
}

void DisplayOutputs::handleStatus() {
  SystemConfig &config = SystemConfig::get();
  String status = "";

  if (config.currentState == DisplayState::SCAN_PASS) {
    status = "SCAN_PASS";
  } 
  else if (config.currentState == DisplayState::HOME) {
    status = "HOME";
  } 
  else if (config.currentState == DisplayState::SCAN_ID) {
    status = "SCAN_ID";
  } 
  else if (config.currentState == DisplayState::COUNTDOWN) {
    int secondsLeft = config.getSecondsRemaining();
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d", secondsLeft / 60, secondsLeft % 60);
    
    // Send Name, Gender, and Time to the frontend
    status = "COUNTDOWN," + config.studentName + "," + config.passCardGender + "," + String(timeStr);
  } 
  else if (config.currentState == DisplayState::SUCCESS) {
    status = "SUCCESS";
  } 
  else if (config.currentState == DisplayState::ERROR) {
    status = "ERROR";
  }

  server.send(200, "text/plain", status);
}