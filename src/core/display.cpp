#include "core/display.h"
#include "system/config.h"
#include <WiFi.h> // Added for router connection

DisplayOutputs &DisplayOutputs::get() {
  static DisplayOutputs instance;
  return instance;
}

void DisplayOutputs::initialize() {
  Serial.println("Connecting to Router...");

  // --- WIFI CREDENTIALS ---
  const char *ssid = "GTSI-2374-2.4G";
  const char *password = "gtsiwifi3212374";
  // ------------------------

  // 1. Connect to Router
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Waiting for connection");
  unsigned long startAttemptTime = millis();

  // Wait up to 10 seconds for connection
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("✅ Web Server IP: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("❌ WiFi Failed. Web Server will not start.");
    return;
  }

  // 2. Setup and start Web Server
  setupRoutes();
  server.begin();
  Serial.println("Web Server Started.");
}

void DisplayOutputs::handleClient() { server.handleClient(); }

void DisplayOutputs::setupRoutes() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/start", HTTP_POST, [this]() { handleStart(); });
  server.on("/status", HTTP_GET, [this]() { handleStatus(); });
  server.on("/simulate", HTTP_POST, [this]() { handleSimulateRfid(); });
}

void DisplayOutputs::handleSimulateRfid() {
  SystemConfig &config = SystemConfig::get();

  if (config.currentState == DisplayState::RULES) {
    config.startTimer();
    config.currentState = DisplayState::COUNTDOWN;
    Serial.println("Simulation: RFID Tapped! Countdown started.");
  } else if (config.currentState == DisplayState::COUNTDOWN) {
    config.currentState = DisplayState::SUCCESS;
    Serial.println("Simulation: RFID Tapped! Success.");
  }

  server.send(200, "text/plain", "OK");
}

void DisplayOutputs::handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <style>
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background-color: #000000; color: #ffffff; margin: 0; padding: 20px;
      display: flex; flex-direction: column; align-items: center; min-height: 100vh; box-sizing: border-box;
    }
    .container { width: 100%; max-width: 360px; text-align: center; }
    h1 { font-size: 24px; font-weight: bold; margin-bottom: 10px; margin-top: 20px; }
    h2 { font-size: 14px; font-weight: normal; color: #aaaaaa; margin-top: 0; margin-bottom: 30px; }
    
    .input-box {
      width: 100%; padding: 15px; margin-bottom: 20px; font-size: 16px;
      background-color: #222222; color: #ffffff; border: 2px solid #555555;
      box-sizing: border-box; text-align: center;
    }
    .input-box:focus { outline: none; border-color: #ffffff; }

    .btn {
      display: block; width: 100%; padding: 18px 0; margin-bottom: 15px;
      font-size: 18px; background-color: transparent; color: #ffffff;
      border: 2px solid #ffffff; border-radius: 0; cursor: pointer; box-sizing: border-box;
    }
    .btn:active { background-color: #333333; }
    .hidden { display: none; }
    
    .timer-box { border: 2px solid #ffffff; padding: 30px 10px; margin: 30px 0; }
    .timer { font-size: 56px; font-weight: bold; color: #ff3333; margin: 0; font-variant-numeric: tabular-nums; }
    
    .success-text { font-size: 36px; font-weight: bold; color: #00ff00; margin-top: 100px; }
    .error-text { font-size: 36px; font-weight: bold; color: #ff0000; margin-top: 100px; }
    .sub-text { color: #aaaaaa; font-size: 14px; margin-top: 20px; }
    
    .rules-list { text-align: left; font-size: 14px; color: #aaaaaa; padding-left: 10px; }
    .rules-list li { margin-bottom: 10px; }
    .warning-text { color: #ff0000; }
  </style>
</head>
<body>
  <div class="container">
    <!-- HOME -->
    <div id="home-screen">
      <h1>Hallway Pass</h1>
      <h2>Enter your name to begin.</h2>
      
      <!-- NAME INPUT -->
      <input type="text" id="student-name" class="input-box" placeholder="Enter your full name" autocomplete="off">
      
      <h2>Select your destination:</h2>
      <button class="btn" onclick="startTimer('Clinic')">Clinic (20m)</button>
      <button class="btn" onclick="startTimer('Restroom')">Restroom (10m)</button>
      <button class="btn" onclick="startTimer('Faculty')">Faculty (15m)</button>
    </div>

    <!-- RULES -->
    <div id="rules-screen" class="hidden">
      <h1 id="dest-title">Clinic</h1>
      <h2 id="rules-name">Student Name</h2>
      <ul class="rules-list">
        <li>1. Walk, do not run.</li>
        <li>2. Return within the time limit.</li>
        <li class="warning-text">3. Failure to return will result in a violation.</li>
        <li>Please use your time wisely.</li>
      </ul>
      <div class="timer-box" style="border-color: #00ff00; padding: 20px;">
        <h3 style="margin: 0; color: #00ff00;">TAP RFID TO START</h3>
      </div>
      
      <button class="btn" onclick="simulateRfid()" style="border-color: #ffaa00; color: #ffaa00; margin-top: 20px;">
        [TEST] Simulate RFID Tap
      </button>
    </div>

    <!-- COUNTDOWN -->
    <div id="countdown-screen" class="hidden">
      <h1 id="countdown-dest">Destination</h1>
      <h2 id="countdown-name">Student Name</h2>
      <div class="timer-box"><p class="timer" id="timer-display">00:00</p></div>
      <p class="sub-text">Failed to clock out in time is a violation.</p>
    </div>

    <!-- RESULT -->
    <div id="result-screen" class="hidden">
      <h1 id="result-title" class="success-text">PASS RECORDED</h1>
      <p id="result-sub" class="sub-text">Wait 5 seconds...</p>
    </div>
  </div>

  <script>
    function showScreen(id) {
      document.querySelectorAll('div[id$="-screen"]').forEach(el => el.classList.add('hidden'));
      document.getElementById(id).classList.remove('hidden');
    }

    function startTimer(dest) {
      let name = document.getElementById('student-name').value.trim();
      if (name === "") {
        alert("Please enter your name first.");
        return;
      }

      showScreen('rules-screen');
      document.getElementById('dest-title').innerText = dest;
      document.getElementById('rules-name').innerText = name;
      document.getElementById('countdown-name').innerText = name;
      
      // Send BOTH name and destination to ESP32
      fetch('/start', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'dest=' + encodeURIComponent(dest) + '&name=' + encodeURIComponent(name)
      });
    }

    function simulateRfid() {
      fetch('/simulate', { method: 'POST' });
    }

    setInterval(() => {
      fetch('/status')
        .then(response => response.text())
        .then(state => {
          if (state.startsWith("COUNTDOWN")) {
            showScreen('countdown-screen');
            let parts = state.split(",");
            document.getElementById('timer-display').innerText = parts[1];
          } 
          else if (state === "SUCCESS") {
            showScreen('result-screen');
            document.getElementById('result-title').className = "success-text";
            document.getElementById('result-title').innerText = "PASS RECORDED";
            document.getElementById('result-sub').innerText = "You have returned within the time limit.";
          } 
          else if (state === "ERROR") {
            showScreen('result-screen');
            document.getElementById('result-title').className = "error-text";
            document.getElementById('result-title').innerText = "VIOLATION";
            document.getElementById('result-sub').innerText = "Time limit exceeded. Alert sent.";
          } 
          else if (state === "HOME") {
            showScreen('home-screen');
          }
        }); 
    }, 5000); // Live polling for every 5 seconds.
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void DisplayOutputs::handleStart() {
  SystemConfig &config = SystemConfig::get();

  if (server.hasArg("dest") && server.hasArg("name")) {
    // Save both the destination and the student's name
    config.setDestination(server.arg("dest"));
    config.studentName = server.arg("name");
    config.currentState = DisplayState::RULES;
  }
  server.send(200, "text/plain", "OK");
}

void DisplayOutputs::handleStatus() {
  SystemConfig &config = SystemConfig::get();
  String status = "HOME";

  if (config.currentState == DisplayState::RULES) {
    status = "RULES";
  } else if (config.currentState == DisplayState::COUNTDOWN) {
    int secondsLeft = config.getSecondsRemaining();
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d", secondsLeft / 60, secondsLeft % 60);
    status = "COUNTDOWN," + String(timeStr);
  } else if (config.currentState == DisplayState::SUCCESS) {
    status = "SUCCESS";
  } else if (config.currentState == DisplayState::ERROR) {
    status = "ERROR";
  }

  server.send(200, "text/plain", status);
}
