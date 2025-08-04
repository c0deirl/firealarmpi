#include <WiFi.h>
#include <HTTPClient.h>
#include "SPIFFS.h"
#include <WebServer.h>
#include <time.h>

#define ALARM_PIN 4
#define TROUBLE_PIN 17

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* ntfy_topic_url = "https://ntfy.sh/YOUR_TOPIC";

WebServer server(80);

String logFilePath = "/log.txt";
unsigned long wifiConnectStart;

void writeLog(const String &msg) {
  File logFile = SPIFFS.open(logFilePath, FILE_APPEND);
  if (logFile) {
    logFile.println(msg);
    logFile.close();
  }
}

void sendNotification(const String &body) {
  HTTPClient http;
  http.begin(ntfy_topic_url);
  http.addHeader("Content-Type", "text/plain");
  int httpResponseCode = http.POST(body);
  http.end();
  writeLog("Notification sent: " + body + " (Code: " + String(httpResponseCode) + ")");
}

String getCurrentTimeStr() {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char buf[32];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
  return String(buf);
}

void handleRoot() {
  String html = R"rawliteral(
    <html>
      <head>
        <title>ESP32 Fire Alarm Status</title>
        <style>
          body { font-family: Arial; background: #eef; }
          .status { display: flex; gap: 30px; margin-bottom: 12px; }
          .indicator { display: flex; flex-direction: column; align-items: center; }
          .circle {
            width: 30px; height: 30px; border-radius: 50%; border: 2px solid #333;
            background: #eee; margin-bottom: 5px;
            box-shadow: 1px 2px 5px #999;
          }
          .active { background: #e22; }
          .inactive { background: #2e2; }
          .logbox { margin-top: 20px; background: #fff; border: 1px solid #aaa; padding: 10px; max-height: 350px; overflow-y: auto;}
        </style>
      </head>
      <body>
        <h2>ESP32 Fire Alarm Monitoring</h2>
        <div class="status">
          <div class="indicator">
            <div id="alarmCircle" class="circle inactive"></div>
            <span>Alarm (GPIO 4)</span>
          </div>
          <div class="indicator">
            <div id="troubleCircle" class="circle inactive"></div>
            <span>Trouble (GPIO 17)</span>
          </div>
          <div class="indicator">
            <span id="wifiRSSI">WiFi: -- dBm</span>
          </div>
          <div class="indicator">
            <span id="curTime">Time: --</span>
          </div>
        </div>
        <div class="logbox" id="logbox">Loading logs...</div>
        <script>
          function updateStatus() {
            fetch('/status').then(r=>r.json()).then(j=>{
              document.getElementById('alarmCircle').className = 'circle ' + (j.alarm ? 'active' : 'inactive');
              document.getElementById('troubleCircle').className = 'circle ' + (j.trouble ? 'active' : 'inactive');
              document.getElementById('wifiRSSI').textContent = 'WiFi: ' + j.rssi + ' dBm';
              document.getElementById('curTime').textContent = 'Time: ' + j.time;
            });
          }
          function updateLogs() {
            fetch('/logs').then(r=>r.text()).then(t=>{
              document.getElementById('logbox').textContent = t;
            });
          }
          setInterval(updateStatus, 1000);
          setInterval(updateLogs, 5000);
          updateStatus(); updateLogs();
        </script>
      </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleStatus() {
  int alarmState = digitalRead(ALARM_PIN);
  int troubleState = digitalRead(TROUBLE_PIN);
  int rssi = WiFi.RSSI();
  String now = getCurrentTimeStr();
  String json = "{\"alarm\":" + String(alarmState) + ",\"trouble\":" + String(troubleState) +
                ",\"rssi\":" + String(rssi) + ",\"time\":\"" + now + "\"}";
  server.send(200, "application/json", json);
}

void handleLogs() {
  File logFile = SPIFFS.open(logFilePath, FILE_READ);
  String log = "";
  if (logFile) {
    while (logFile.available()) {
      log += logFile.readStringUntil('\n') + "\n";
    }
    logFile.close();
  } else {
    log = "No log file found.";
  }
  server.send(200, "text/plain", log);
}

void setupTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  delay(1000); // Time will sync within a second or two
}

void setup() {
  Serial.begin(115200);

  pinMode(ALARM_PIN, INPUT_PULLDOWN);
  pinMode(TROUBLE_PIN, INPUT_PULLDOWN);

  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  writeLog(getCurrentTimeStr() + " ESP32 booting...");

  // WiFi connect with timeout
  wifiConnectStart = millis();
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int wifiTries = 0;
  bool wifiFail = false;
  while (WiFi.status() != WL_CONNECTED && wifiTries < 30) { // 15 seconds max
    Serial.print(".");
    delay(500);
    wifiTries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
    writeLog(getCurrentTimeStr() + " WiFi connected: " + WiFi.localIP().toString());
    sendNotification(getCurrentTimeStr() + " ESP32 started and monitoring fire alarm system.");
  } else {
    Serial.println("\nWiFi connection failed.");
    writeLog(getCurrentTimeStr() + " WiFi connection failed.");
    // Optionally send notification if ESP32 can connect to another network
    wifiFail = true;
  }

  setupTime();

  // Web server routes
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/logs", handleLogs);
  server.begin();
  writeLog(getCurrentTimeStr() + " Web server started.");

  writeLog(getCurrentTimeStr() + " Monitoring started.");
  if (!wifiFail)
    sendNotification(getCurrentTimeStr() + " Monitoring started.");
}

void loop() {
  static bool alarmAlerted = false;
  static bool troubleAlerted = false;

  // Poll alarm pin
  if (digitalRead(ALARM_PIN) == HIGH && !alarmAlerted) {
    alarmAlerted = true;
    writeLog(getCurrentTimeStr() + " Fire Alarm Detected!");
    sendNotification(getCurrentTimeStr() + " Fire Alarm Activated!");
  } else if (digitalRead(ALARM_PIN) == LOW && alarmAlerted) {
    alarmAlerted = false;
    writeLog(getCurrentTimeStr() + " Fire Alarm Cleared!");
    sendNotification(getCurrentTimeStr() + " Fire Alarm Cleared!");
  }

  // Poll trouble pin
  if (digitalRead(TROUBLE_PIN) == HIGH && !troubleAlerted) {
    troubleAlerted = true;
    writeLog(getCurrentTimeStr() + " Trouble Alarm Detected!");
    sendNotification(getCurrentTimeStr() + " Trouble Alarm Activated!");
  } else if (digitalRead(TROUBLE_PIN) == LOW && troubleAlerted) {
    troubleAlerted = false;
    writeLog(getCurrentTimeStr() + " Trouble Alarm Cleared!");
    sendNotification(getCurrentTimeStr() + " Trouble Alarm Cleared!");
  }

  server.handleClient();
  delay(150);
}