#include "core/display.h"
#include "system/config.h"
#include <WiFi.h>
#include <LittleFS.h>

DisplayOutputs &DisplayOutputs::get() {
  static DisplayOutputs instance;
  return instance;
}

void DisplayOutputs::initialize() {
  // 1. Initialize the LCD display
  Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN); // Set up I2C using the configured SDA and SCL pins
  lcd.init();                           // Initialize the LCD
  lcd.backlight();                      // Turn on the LCD backlight
  lcdPrint("Starting up...", "Please wait."); // Display startup message

  // 2. Initialize LittleFS file system
  // LittleFS stores files, such as the university background image,
  // in the ESP32's flash memory.
  if (!LittleFS.begin(true)) {
    // Print an error if LittleFS fails to mount
    Serial.println("LittleFS mount failed");
    return; // Stop initialization if LittleFS cannot be mounted
  }

  // Confirm that LittleFS is ready to access stored files
  Serial.println("LittleFS mounted successfully");

  // 3. Connect the ESP32 to the Wi-Fi network
  Serial.println("Connecting to Router...");

  const char *ssid = "iss";           // Wi-Fi network name
  const char *password = "robincarl"; // Wi-Fi password

  WiFi.mode(WIFI_STA);       // Set the ESP32 to station mode
  WiFi.begin(ssid, password); // Start connecting to the Wi-Fi network

  // Record the start time so the connection attempt has a time limit
  unsigned long startAttemptTime = millis();

  // Wait until connected or until 10 seconds have passed
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < 10000) {
    delay(500);             // Wait half a second before checking again
    Serial.print(".");      // Show connection progress in Serial Monitor
  }

  Serial.println(); // Move to a new line after the connection attempts

  // Access the shared system configuration
  SystemConfig &config = SystemConfig::get();

  // Check whether the ESP32 successfully connected to Wi-Fi
  if (WiFi.status() == WL_CONNECTED) {
    // Save the ESP32's assigned local IP address
    config.serverIP = WiFi.localIP().toString();

    // Print the web server address to Serial Monitor
    Serial.print("✅ Web Server IP: http://");
    Serial.println(config.serverIP);

    // Display the successful connection and IP address on the LCD
    lcdPrint("WiFi Connected!", "IP: " + config.serverIP);
  } else {
    // Inform the user if the Wi-Fi connection failed
    Serial.println("❌ WiFi Failed.");

    // Display an error message on the LCD
    lcdPrint("WiFi Failed!", "Check Network.");
  }

  delay(2000); // Keep the connection status visible for 2 seconds

  // 4. Configure the HTTP routes for the web dashboard
  // This includes the routes handled by setupRoutes(), such as
  // the main dashboard and the background image, if configured there.
  setupRoutes();

  // Start the web server so the ESP32 can handle browser requests
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
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Hallway Pass Dashboard</title>

  <style>
    :root {
      --maroon: #800020;
      --dark-maroon: #570016;
      --red: #b5122b;
      --white: #ffffff;
      --gray: #666666;
      --light: #f7f5f5;
    }

    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      min-height: 100vh;
      padding: 24px 16px;
      font-family: Arial, Helvetica, sans-serif;
      color: #282020;
      background:
        linear-gradient(
          rgba(255, 255, 255, 0.83),
          rgba(255, 255, 255, 0.91)
        ),
        url('/background.jpg') center / cover fixed no-repeat;
      display: flex;
      justify-content: center;
      align-items: center;
    }

    .container {
      width: 100%;
      max-width: 440px;
      background: rgba(255, 255, 255, 0.96);
      border: 1px solid #e5d4d8;
      border-radius: 20px;
      overflow: hidden;
      box-shadow: 0 12px 35px rgba(70, 0, 20, 0.16);
    }

    .header {
      background: linear-gradient(135deg, var(--dark-maroon), var(--maroon));
      color: white;
      padding: 25px 20px;
      text-align: center;
      border-bottom: 5px solid #d6b36a;
    }

    .anniversary {
      font-size: 12px;
      font-weight: bold;
      letter-spacing: 3px;
      text-transform: uppercase;
      opacity: 0.9;
    }

    .header h1 {
      margin: 10px 0 6px;
      font-size: 27px;
      letter-spacing: 1px;
    }

    .header p {
      margin: 0;
      font-size: 12px;
      letter-spacing: 1.5px;
      line-height: 1.6;
    }

    .content {
      padding: 25px 22px 30px;
    }

    .hidden {
      display: none !important;
    }

    h2 {
      margin: 0 0 8px;
      color: var(--maroon);
      font-size: 23px;
      text-align: center;
    }

    .subtitle {
      margin: 0 0 24px;
      color: var(--gray);
      text-align: center;
      font-size: 14px;
      line-height: 1.5;
    }

    .destination {
      display: block;
      width: 100%;
      padding: 17px;
      margin: 12px 0;
      border: 2px solid var(--maroon);
      border-radius: 12px;
      background: white;
      color: var(--maroon);
      font-size: 16px;
      font-weight: bold;
      text-align: left;
      cursor: pointer;
      transition: 0.2s;
    }

    .destination span {
      float: right;
      color: var(--gray);
      font-size: 13px;
      font-weight: normal;
    }

    .destination:hover,
    .destination:active {
      background: var(--maroon);
      color: white;
    }

    .destination:hover span,
    .destination:active span {
      color: white;
    }

    .panel {
      padding: 18px;
      margin: 18px 0;
      border: 1px solid #eadde0;
      border-radius: 12px;
      background: var(--light);
    }

    .scan-icon {
      width: 68px;
      height: 68px;
      margin: 0 auto 14px;
      display: flex;
      align-items: center;
      justify-content: center;
      border: 3px solid var(--maroon);
      border-radius: 50%;
      color: var(--maroon);
      font-size: 32px;
    }

    .scan-title {
      color: var(--maroon);
      font-size: 17px;
      font-weight: bold;
      text-align: center;
    }

    .scan-description {
      margin-top: 8px;
      color: var(--gray);
      text-align: center;
      font-size: 13px;
      line-height: 1.5;
    }

    .rules {
      padding-left: 20px;
      color: #51474a;
      font-size: 14px;
      line-height: 1.7;
    }

    .rules li {
      margin-bottom: 8px;
    }

    .warning {
      color: #a20d28;
      font-weight: bold;
    }

    .info-row {
      padding: 12px 0;
      border-bottom: 1px solid #eadde0;
    }

    .info-row:last-child {
      border-bottom: none;
    }

    .info-label {
      margin-bottom: 5px;
      color: #777;
      font-size: 11px;
      font-weight: bold;
      letter-spacing: 1px;
    }

    .info-value {
      color: var(--dark-maroon);
      font-size: 16px;
      font-weight: bold;
      overflow-wrap: anywhere;
    }

    .timer-box {
      margin: 20px 0;
      padding: 24px 12px;
      border: 2px solid var(--maroon);
      border-radius: 14px;
      text-align: center;
      background: #fffafa;
    }

    .timer-label {
      color: var(--gray);
      font-size: 12px;
      font-weight: bold;
      letter-spacing: 2px;
    }

    .timer {
      margin: 10px 0 0;
      color: var(--maroon);
      font-size: 54px;
      font-weight: bold;
      font-variant-numeric: tabular-nums;
    }

    .result-icon {
      margin: 10px 0;
      font-size: 56px;
      text-align: center;
    }

    .success-text {
      color: #168047;
    }

    .error-text {
      color: #b5122b;
    }

    .footer {
      padding: 15px;
      border-top: 1px solid #eadde0;
      color: #777;
      font-size: 11px;
      text-align: center;
      line-height: 1.6;
    }

    @media (max-width: 380px) {
      .header h1 {
        font-size: 23px;
      }

      .content {
        padding: 20px 16px;
      }

      .timer {
        font-size: 46px;
      }
    }
  </style>
</head>

<body>
  <main class="container">

    <header class="header">
      <div class="anniversary">Manuel S. Enverga University</div>
      <h1>HALLWAY PASS</h1>
      <p>ONE LEGACY, ONE GLOBAL VISION</p>
    </header>

    <section class="content">

      <!-- Screen 1: Scan the hallway pass card -->
      <div id="scan-pass-screen">
        <h2>Welcome</h2>
        <p class="subtitle">Student Hallway Pass System</p>

        <div class="panel">
          <div class="scan-icon">▣</div>
          <div class="scan-title">TAP YOUR PASS CARD</div>
          <p class="scan-description">
            Scan your hallway pass card to continue.
          </p>
        </div>
      </div>

      <!-- Screen 2: Select destination. Change the time if changed to matched.-->
      <div id="home-screen" class="hidden">
        <h2>Dashboard</h2>
        <p class="subtitle" id="welcome-text">
          Select your destination
        </p>

        <button class="destination" onclick="startTimer('Clinic')">
          Clinic <span>20 minutes</span>
        </button>

        <button class="destination" onclick="startTimer('Restroom')">
          Restroom <span>10 minute</span>
        </button>

        <button class="destination" onclick="startTimer('Faculty')">
          Faculty <span>15 minutes</span>
        </button>
      </div>

      <!-- Screen 3: Read rules and scan student ID -->
      <div id="scan-id-screen" class="hidden">
        <h2 id="dest-title">Clinic</h2>
        <p class="subtitle">Rules &amp; Reminders</p>

        <div class="panel">
          <ol class="rules">
            <li>Walk, do not run.</li>
            <li>Return within the time limit.</li>
            <li class="warning">
              Failure to return is a violation.
            </li>
          </ol>
        </div>

        <div class="panel">
          <div class="scan-icon">▣</div>
          <div class="scan-title">TAP STUDENT ID</div>
          <p class="scan-description">
            Verify your identity to start the pass.
          </p>
        </div>
      </div>

      <!-- Screen 4: Active pass and countdown -->
      <div id="countdown-screen" class="hidden">
        <h2>Active Pass</h2>
        <p class="subtitle">Please return before time expires.</p>

        <div class="panel">
          <div class="info-row">
            <div class="info-label">STUDENT</div>
            <div class="info-value" id="info-name">Loading...</div>
          </div>

          <div class="info-row">
            <div class="info-label">GENDER</div>
            <div class="info-value" id="info-gender">Loading...</div>
          </div>

          <div class="info-row">
            <div class="info-label">DESTINATION</div>
            <div class="info-value" id="info-dest">Loading...</div>
          </div>
        </div>

        <div class="timer-box">
          <div class="timer-label">TIME REMAINING</div>
          <p class="timer" id="timer-display">00:00</p>
        </div>

        <p class="scan-description">
          Return the pass card before the timer reaches zero.
        </p>
      </div>

      <!-- Screen 5: Success or violation -->
      <div id="result-screen" class="hidden">
        <div id="result-icon" class="result-icon">✓</div>

        <h2 id="result-title" class="success-text">
          PASS RECORDED
        </h2>

        <p id="result-sub" class="subtitle">
          Returned within time.
        </p>

        <div class="panel">
          <div class="info-row">
            <div class="info-label">STUDENT</div>
            <div class="info-value" id="result-name">-</div>
          </div>

          <div class="info-row">
            <div class="info-label">DESTINATION</div>
            <div class="info-value" id="result-dest">-</div>
          </div>
        </div>
      </div>

    </section>

    <footer class="footer">
      Hallway Pass Monitoring System<br>
      One Legacy, One Global Vision
    </footer>

  </main>

  <script>
    let currentName = "";
    let currentGender = "";
    let currentDest = "";

    // Display only the requested screen.
    function showScreen(id) {
      document.querySelectorAll('[id$="-screen"]').forEach(screen => {
        screen.classList.add("hidden");
      });

      document.getElementById(id).classList.remove("hidden");
    }

    // Send the selected destination to the ESP32.
    function startTimer(dest) {
      currentDest = dest;
      document.getElementById("dest-title").innerText = dest;

      showScreen("scan-id-screen");

      fetch("/start", {
        method: "POST",
        headers: {
          "Content-Type": "application/x-www-form-urlencoded"
        },
        body: "dest=" + encodeURIComponent(dest)
      }).catch(error => {
        console.error("Could not send destination:", error);
      });
    }

    // Poll the ESP32 for the current pass status every second.
    async function updateStatus() {
      try {
        const response = await fetch("/status");
        if (!response.ok) return;

        const data = await response.text();
        const parts = data.split(",");
        const state = parts[0];

        if (state === "SCAN_PASS") {
          showScreen("scan-pass-screen");
        } else if (state === "HOME") {
          showScreen("home-screen");
        } else if (state === "SCAN_ID") {
          showScreen("scan-id-screen");
        } else if (state === "COUNTDOWN") {
          showScreen("countdown-screen");

          currentName = parts[1] || "Unknown";
          currentGender = parts[2] || "Unknown";
          const time = parts[3] || "00:00";

          document.getElementById("info-name").innerText = currentName;
          document.getElementById("info-gender").innerText = currentGender;
          document.getElementById("info-dest").innerText = currentDest;
          document.getElementById("timer-display").innerText = time;
        } else if (state === "SUCCESS") {
          showScreen("result-screen");

          document.getElementById("result-icon").innerText = "✓";
          document.getElementById("result-title").className = "success-text";
          document.getElementById("result-title").innerText = "PASS RECORDED";
          document.getElementById("result-sub").innerText =
            "Returned within time.";

          document.getElementById("result-name").innerText = currentName;
          document.getElementById("result-dest").innerText = currentDest;
        } else if (state === "ERROR") {
          showScreen("result-screen");

          document.getElementById("result-icon").innerText = "!";
          document.getElementById("result-title").className = "error-text";
          document.getElementById("result-title").innerText = "VIOLATION";
          document.getElementById("result-sub").innerText =
            "Time limit exceeded.";

          document.getElementById("result-name").innerText = currentName;
          document.getElementById("result-dest").innerText = currentDest;
        }
      } catch (error) {
        console.error("Could not retrieve pass status:", error);
      }
    }

    // Start polling when the page loads.
    updateStatus();
    setInterval(updateStatus, 1000);
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
