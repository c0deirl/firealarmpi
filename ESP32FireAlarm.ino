#include <WiFi.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include "SPIFFS.h"
#include <WebServer.h>
#include <time.h>
#include <ESP32_FTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiClientSecure.h>



// Email Includes
#define ENABLE_SMTP
#define ENABLE_DEBUG
#define READYMAIL_DEBUG_PORT Serial
#include <ReadyMail.h>

#define ALARM_PIN 4
#define TROUBLE_PIN 17

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//SMTP Server Settings for Gmail - Change it if you are using other email servers
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "greathouse.matthew@gmail.com" //Put your Sender email id
#define AUTHOR_PASSWORD "***********"  // App password not gmail password - this is for 2FA enabled gmail email id.
#define RECIPIENT_EMAIL "*************" //Receiver email id

//Email Setup
WiFiClientSecure client;
SMTPClient smtp(client);

// WiFi Credentials and NTFY Topic
const char* ssid = "N8MDG";
const char* password = "mattg123";
const char* ntfy_topic_url = "https://notify.codemov.com/testfire";

// FTP credentials
char ftp_server[] = "132.145.171.18";
uint16_t ftp_port = 2121;
char ftp_user[]   = "stpaul";
char ftp_pass[]   = "**********";
char ftp_path[]   = "/logs/";
uint16_t ftp_timeout = 5000;
uint8_t ftp_debug = 2;

ESP32_FTPClient ftp(ftp_server, ftp_port, ftp_user, ftp_pass, ftp_timeout, ftp_debug);

// Web server
WebServer server(80);

// Daily log file path
String logFilePath = "";
String currentLogDate = "";

unsigned long lastFTPTime = 0;
const unsigned long ftpInterval = 43200000; // 12 hours in ms
const unsigned long ftpIntervalday = 86400000; // 1 day in ms
volatile bool ftpManualRequested = false;

//Email SMTP Status Check
void smtpStatus(SMTPStatus status) {
  Serial.println(status.text);
}


void sendemail(String emailbody) {

   client.setInsecure();  // Insecure, for dev testing
  configTime(-14400, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time...");
  while (time(nullptr) < 100000) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  smtp.connect(SMTP_HOST, SMTP_PORT, smtpStatus, true);
  if (!smtp.isConnected()) return;

  smtp.authenticate(AUTHOR_EMAIL, AUTHOR_PASSWORD, readymail_auth_password);
  if (!smtp.isAuthenticated()) return;

  SMTPMessage msg;
  msg.headers.add(rfc822_from, AUTHOR_EMAIL);
  msg.headers.add(rfc822_to, RECIPIENT_EMAIL);
  msg.headers.add(rfc822_subject, " St Paul Fire Alarm System");
  msg.headers.addCustom("X-Priority", "1");
  msg.headers.addCustom("Importance", "High");

  String body = emailbody;
 // String body = " --- ALERT ---,\r\n\r\n";
 // body += "The ST Paul Fire Alarm System has detected a change\r\n";
 // body += emailbody + " -- -- .\r\n\r\n";
  msg.html.body(body);

  msg.timestamp = time(nullptr);

  smtp.send(msg, "SUCCESS,FAILURE");
  writeLog(getCurrentTimeStr() + " Email Notification Sent");
  // smtp.disconnect(); // REMOVE or comment this out
}


void waitForNTPSync() {
  // The First Value should be -14400 for EST
  configTime(-14400, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync");
  int maxTries = 40;
  int tries = 0;
  time_t now = time(nullptr);
  while ((now < 1533081600) && tries < maxTries) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    tries++;
  }
  Serial.println();
}

String getCurrentDateStr() {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char buf[16];
  sprintf(buf, "%04d-%02d-%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday);
  return String(buf);
}

String getCurrentTimeStr() {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char buf[32];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
  return String(buf);
}

void updateLogFilePath() {
  String today = getCurrentDateStr();
  if (today != currentLogDate) {
    currentLogDate = today;
    logFilePath = "/" + today + ".log";
    Serial.println("Log file changed to: " + logFilePath);
    writeLog(getCurrentTimeStr() + " New daily log file started.");
  }
}

void writeLog(const String &msg) {
  updateLogFilePath();
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
  http.addHeader("X-Actions", "view, Fire Alarm System, https://fire.stpaulwv.org");
  int httpResponseCode = http.POST(body);
  http.end();
  writeLog("Notification sent: " + body + " (Code: " + String(httpResponseCode) + ")");
}

void handleRoot() {
  String html = R"rawliteral(
    <html>
      <head>
        <title>St Paul Fire Alarm Status</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          body { 
            font-family: Arial, Helvetica, sans-serif; 
            background: linear-gradient(180deg,rgba(0, 0, 0, 1) 50%, rgba(130, 0, 0, 1) 100%);
            margin: 0;
            padding: 0;
          }
          .container {
            max-width: 480px;
            margin: 0 auto;
            padding: 16px;
            background: #fff;
            box-shadow: 0 2px 8px #bbb;
            border-radius: 12px;
            margin-top: 24px;
          }
          h2 {
            text-align: center;
            font-size: 1.5em;
            margin-bottom: 10px;
          }
          .status {
            display: flex; 
            flex-wrap: wrap;
            justify-content: center;
            gap: 18px; 
            margin-bottom: 12px;
          }
          .indicator { 
            display: flex; 
            flex-direction: column; 
            align-items: center; 
            min-width: 80px;
          }
          .circle {
            width: 34px; height: 34px; border-radius: 50%; border: 2px solid #333;
            background: #eee; margin-bottom: 5px;
            box-shadow: 1px 2px 5px #999;
          }
          .active { background: #e22; }
          .inactive { background: #2e2; }
          .logbox { 
            margin-top: 20px; 
            background: #f9f9f9; 
            border: 1px solid #aaa; 
            padding: 10px; 
            max-height: 350px; 
            overflow-y: auto; 
            white-space: pre-line;
            font-size: 1em;
            border-radius: 7px;
          }
          .wifibar {
            display: flex;
            align-items: flex-end;
            height: 34px;
            width: 50px;
            margin-bottom: 5px;
            gap: 2px;
          }
          .bar {
            width: 8px;
            background: #9ae89a;
            border-radius: 2px 2px 0 0;
            transition: height 0.3s, background 0.3s;
          }
          .bar.low { background: #e22; }
          .bar.medium { background: #ffd900; }
          .bar.high { background: #2e2; }
          .ftpbtn {
            margin-top: 16px;
            background: #2e2;
            color: #fff;
            border: none;
            border-radius: 6px;
            padding: 10px 18px;
            font-size: 1em;
            cursor: pointer;
            box-shadow: 0 2px 4px #aaa;
            transition: background 0.2s;
          }
          .ftpbtn:active { background: #1a1; }
          #ftpStatus {
            display: block;
            margin-top: 8px;
            font-size: 0.97em;
            color: #555;
            text-align: center;
            min-height: 1.2em;
          }
          @media (max-width: 520px) {
            .container { max-width: 98vw; margin-top: 6vw; }
            .status { gap: 8px;}
            .indicator { min-width: 70px;}
            .logbox { font-size: 0.92em; padding: 7px;}
            .wifibar { width: 38px; height: 28px;}
            .bar { width: 6px;}
            .ftpbtn { font-size: 0.98em; padding: 8px 12px;}
          }
        </style>
      </head>
      <body>
        <div class="container">
          <h2>St. Paul Fire Alarm Monitoring</h2>
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
              <div class="wifibar" id="wifiBar">
                <div class="bar" id="bar1" style="height:6px"></div>
                <div class="bar" id="bar2" style="height:12px"></div>
                <div class="bar" id="bar3" style="height:20px"></div>
                <div class="bar" id="bar4" style="height:28px"></div>
                <div class="bar" id="bar5" style="height:34px"></div>
              </div>
              <span id="wifiRSSI">WiFi: -- dBm</span>
            </div>
            <div class="indicator">
              <span id="curTime">Time: --</span>
            </div>
          </div>
          <div class="logbox" id="logbox">Loading logs...</div>
          <div class="status">
          <button class="ftpbtn" onclick="ftpBackup()" id="ftpBackupBtn">Backup Log to FTP</button>
          <button class="ftpbtn" style="background:#e22;margin-left:8px;" onclick="deleteLogFile()" id="deleteLogBtn">Delete Log File</button>
          <span id="ftpStatus"></span>
        </div> </div>
        <script>
          function setWifiBars(rssi) {
            let bars = 0;
            if (rssi > -55) bars = 5;
            else if (rssi > -65) bars = 4;
            else if (rssi > -75) bars = 3;
            else if (rssi > -85) bars = 2;
            else if (rssi > -95) bars = 1;
            else bars = 0;
            for (let i = 1; i <= 5; i++) {
              let bar = document.getElementById("bar"+i);
              if (i <= bars) {
                bar.style.opacity = "1";
                bar.classList.remove("low","medium","high");
                if (bars <= 2) bar.classList.add("low");
                else if (bars == 3) bar.classList.add("medium");
                else bar.classList.add("high");
              } else {
                bar.style.opacity = "0.18";
                bar.classList.remove("low","medium","high");
              }
            }
          }
          function updateStatus() {
            fetch('/status').then(r=>r.json()).then(j=>{
              document.getElementById('alarmCircle').className = 'circle ' + (j.alarm ? 'active' : 'inactive');
              document.getElementById('troubleCircle').className = 'circle ' + (j.trouble ? 'active' : 'inactive');
              document.getElementById('wifiRSSI').textContent = 'WiFi: ' + j.rssi + ' dBm';
              setWifiBars(j.rssi);
              document.getElementById('curTime').textContent = 'Time: ' + j.time;
            });
          }
          function updateLogs() {
            fetch('/logs').then(r=>r.text()).then(t=>{
              document.getElementById('logbox').textContent = t;
            });
          }
          function ftpBackup() {
            document.getElementById('ftpStatus').textContent = "Uploading log to FTP...";
            fetch('/ftpbackup')
              .then(r => r.json())
              .then(j => {
                document.getElementById('ftpStatus').textContent = j.status;
              })
              .catch(e => {
                document.getElementById('ftpStatus').textContent = "FTP upload error";
              });
          }
          function deleteLogFile() {
            if (!confirm("Are you sure you want to DELETE the current log file?")) return;
            document.getElementById('ftpStatus').textContent = "Deleting log file...";
            fetch('/deletelog')
              .then(r => r.json())
              .then(j => {
                document.getElementById('ftpStatus').textContent = j.status;
                updateLogs();
              })
              .catch(e => {
                document.getElementById('ftpStatus').textContent = "Log file deletion error";
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
  updateLogFilePath();
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

// FTP upload logic
bool uploadLogToFTP(String localPath, String remotePath, String& response) {
  File file = SPIFFS.open(localPath, FILE_READ);
  if (!file) {
    response = "FTP upload failed: log file not found";
    Serial.println(response);
    writeLog(getCurrentTimeStr() + " " + response);
    return false;
  }
  ftp.OpenConnection();
  //ftp.ChangeWorkDir(ftp_path);
  ftp.InitFile("Type I");
  // Check if file exists on FTP server, if not, create it
  ftp.NewFile(remotePath.c_str()); // This creates the file if not present and opens it for writing
  while (file.available()) {
    uint8_t buf[128];
    size_t len = file.read(buf, sizeof(buf));
    if (len) ftp.WriteData(buf, len);
  }
  ftp.CloseFile();
  ftp.CloseConnection();
  file.close();
  response = "FTP upload successful: " + remotePath;
  Serial.println(response);
  writeLog(getCurrentTimeStr() + " " + response);
  sendNotification(getCurrentTimeStr() + " Log Backup to FTP Complete.");
  String uploademail = R"rawliteral(
    <html lang="en">
    <head>
	
<style>
.button {
  display: block;
  align: center;
  border: none;
  color: white;
  text-align: center;
  text-decoration: none;
  font-size: 16px;
  width: 100%;
  height: 50px;
  cursor: pointer;
  font-family: Arial;
  font-size: 25px;
  padding-top: 20px;
  padding-bottom: 5px;
  border-radius: 10px;
}
.title  {
    display: inline-block;
	background: white;
	font-family: Arial;
	font-weight: Bold;
	font-size: 30px;
    width: 100%;
	padding-top: 100px;
	padding-bottom: 100px;
	border-radius: 10px;
	text-align: center;
	vertical-align: middle;
}
body   {
	background-color:grey;
	}
.h1    {
font-family: Arial;
color: red;
text-align: center;
background-color: black; 
padding-top: 15px;
padding-bottom: 15px; 
border-radius: 8px;
}

.button1 {background-color: #04AA6D;} /* Green */
.button2 {background-color: #008CBA;} /* Blue */
</style>

        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>StPaul Fire System</title>
    </head>
    <body class=body>
        <h1 class=h1>St Paul Fire System</h1>
        <p style="font-family: Arial; font-size: 24px; text-align: center;">Fire Alarm System Notification</p>
        <div class=title>
            <span style="font-weight: bold; color: green;"> FTP Upload was Successful </span>
        <br> </div> <br> <br>
		<div style="text-align: center;">Contact Matt G for information on how to retrieve the archived logs<br><br>
		<a href="https://fire.stpaulwv.org" class="button button1">System Status Page</a>
		</div>
    </body>
    </html>
  )rawliteral";
  sendemail(uploademail);
  return true;
}


// Handler for manual FTP backup (called by button)
void handleFTPBackup() {
  updateLogFilePath();
  String remoteName = currentLogDate + ".log";
  String ftpResult;
  bool ok = uploadLogToFTP(logFilePath, remoteName, ftpResult);
  String json = "{\"status\":\"" + ftpResult + "\"}";
  server.send(200, "application/json", json);
 // SPIFFS.remove(logFilePath);
  writeLog(getCurrentTimeStr() + " Log file FTP backup successful.");
 // bool backupSuccess = uploadLogToFTP(logFilePath, remoteName, ftpResult);
 //   if (backupSuccess) {
 //     // Delete the log file after successful backup
 //     if (SPIFFS.exists(logFilePath)) {
 //       SPIFFS.remove(logFilePath);
 //       writeLog(getCurrentTimeStr() + " Log file deleted after FTP backup.");
 //       }
 //   } 
  }

 void handleDeleteLog() {
  updateLogFilePath();
  String remoteName = currentLogDate + "_backup.log";
  String ftpResult;
  bool ok = uploadLogToFTP(logFilePath, remoteName, ftpResult);
  writeLog(getCurrentTimeStr() + " Log file FTP backup successful.");
  remoteName = currentLogDate + ".log";
  if (SPIFFS.exists(logFilePath)) {
    ok = SPIFFS.remove(logFilePath);
  }
  String status;
  if (ok) {
    status = "Log file deleted: " + logFilePath;
    writeLog(getCurrentTimeStr() + " Log file deleted by user.");
  } else {
    status = "Log file not found or could not be deleted.";
  }
  String json = "{\"status\":\"" + status + "\"}";
  server.send(200, "application/json", json);
}
void setup() {
  Serial.begin(115200);
  // Initialize SSD1306 display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
    }
  display.clearDisplay();
  display.display();
  pinMode(ALARM_PIN, INPUT_PULLDOWN);
  pinMode(TROUBLE_PIN, INPUT_PULLDOWN);

  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  updateLogFilePath();
  writeLog("ESP32 booting...");

//WiFi Connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int wifiTries = 0;
  bool wifiFail = false;
  while (WiFi.status() != WL_CONNECTED && wifiTries < 30) {
    Serial.print(".");
    delay(500);
    wifiTries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
    writeLog(getCurrentTimeStr() + " WiFi connected: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed.");
    writeLog(getCurrentTimeStr() + " WiFi connection failed.");
    wifiFail = true;
  }

  waitForNTPSync();
  writeLog(getCurrentTimeStr() + " NTP time synchronized.");

  if (!wifiFail)
    sendNotification(getCurrentTimeStr() + " ESP32 System Started.");

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/logs", handleLogs);
  server.on("/ftpbackup", HTTP_GET, handleFTPBackup); // Add manual backup endpoint
  server.on("/deletelog", HTTP_GET, handleDeleteLog); // Manual log file removal
  server.begin();
  writeLog(getCurrentTimeStr() + " Web server started.");

  writeLog(getCurrentTimeStr() + " Monitoring started.");
  if (!wifiFail)
    sendNotification(getCurrentTimeStr() + " Fire Alarm Monitoring started.");

  lastFTPTime = millis(); // Schedule first FTP upload at startup
}

void loop() {
  static bool alarmAlerted = false;
  static bool troubleAlerted = false;

 // Read GPIOs
  bool alarmActive = digitalRead(ALARM_PIN) == HIGH;
  bool troubleActive = digitalRead(TROUBLE_PIN) == HIGH;

  // WiFi info
  int32_t rssi = WiFi.RSSI();
  String ip = WiFi.localIP().toString();

  // WiFi Reconnection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(F("Connecting..."));
      WiFi.begin(ssid, password);
      writeLog(getCurrentTimeStr() + " WiFi Re-connected!");
      sendNotification(getCurrentTimeStr() + " WiFi Re-connected!");
      }

  // Display output
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0,0);
  display.println("FireAlarm Status");

  display.setCursor(0,16);
  display.print("ALARM: ");
  display.println(alarmActive ? "ACTIVE" : "INACTIVE");

  display.setCursor(0,28);
  display.print("TROUBLE: ");
  display.println(troubleActive ? "ACTIVE" : "INACTIVE");

  display.setCursor(0,40);
  display.print("WiFi: ");
  display.print(rssi);
  display.println(" dBm");

  display.setCursor(0,52);
  display.print("IP: ");
  display.println(ip);

  display.display();

  updateLogFilePath();

//email HTML Templates
String alarmemail = R"rawliteral(
      <html lang="en">
    <head>
	
<style>
.button {
  display: block;
  align: center;
  border: none;
  color: white;
  text-align: center;
  text-decoration: none;
  font-size: 16px;
  width: 100%;
  height: 50px;
  cursor: pointer;
  font-family: Arial;
  font-size: 25px;
  padding-top: 20px;
  padding-bottom: 5px;
  border-radius: 10px;
}
.title  {
    display: block;
	background: white;
	font-family: Arial;
	font-weight: Bold;
	font-size: 30px;
    width: 100%;
	padding-top: 100px;
	padding-bottom: 100px;
	border-radius: 10px;
	text-align: center;
	vertical-align: middle;
}

.button1 {background-color: #04AA6D;} /* Green */
.button2 {background-color: #008CBA;} /* Blue */
</style>

        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>StPaul Fire System</title>
    </head>
    <body style="background-color:grey;">
        <h1 style="font-family: Arial; color: red; text-align: center; background-color: black; padding: 15px; border-radius: 8px;">St Paul Fire System</h1>
        <p style="font-family: Arial; font-size: 24px; text-align: center;">Fire Alarm System Notification</p>
        <div class=title>
            <span style="font-weight: bold; color: red;"> -- ACTIVE FIRE ALARM -- </span><br> 3500 Broad St. <br> Parkersburg wv, 26104 <br> 304-428-5826<br>
        <br> </div>
		<div style="text-align: center;"><br><br>
		<a href="https://fire.stpaulwv.org" class="button button1">System Status Page</a>
		</div>
    </body>
    </html>
      )rawliteral";

String clearedemail = R"rawliteral(
      <html lang="en">
    <head>
	
<style>
.button {
  display: block;
  align: center;
  border: none;
  color: white;
  text-align: center;
  text-decoration: none;
  font-size: 16px;
  width: 100%;
  height: 50px;
  cursor: pointer;
  font-family: Arial;
  font-size: 25px;
  padding-top: 20px;
  padding-bottom: 5px;
  border-radius: 10px;
}
.title  {
    display: block;
	background: white;
	font-family: Arial;
	font-weight: Bold;
	font-size: 30px;
    width: 100%;
	padding-top: 100px;
	padding-bottom: 100px;
	border-radius: 10px;
	text-align: center;
	vertical-align: middle;
}

.button1 {background-color: #04AA6D;} /* Green */
.button2 {background-color: #008CBA;} /* Blue */
</style>

        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>StPaul Fire System</title>
    </head>
    <body style="background-color:grey;">
        <h1 style="font-family: Arial; color: red; text-align: center; background-color: black; padding: 15px; border-radius: 8px;">St Paul Fire System</h1>
        <p style="font-family: Arial; font-size: 24px; text-align: center;">Fire Alarm System Notification</p>
        <div class=title>
            <span style="font-weight: bold; color: green;"> -- Alarms have cleared --</span>
        <br> </div>
		<div style="text-align: center;"><br><br>
		<a href="https://fire.stpaulwv.org" class="button button1">System Status Page</a>
		</div>
    </body>
    </html>
      )rawliteral";

String troubleemail = R"rawliteral(
      <html lang="en">
    <head>
	
<style>
.button {
  display: block;
  align: center;
  border: none;
  color: white;
  text-align: center;
  text-decoration: none;
  font-size: 16px;
  width: 100%;
  height: 50px;
  cursor: pointer;
  font-family: Arial;
  font-size: 25px;
  padding-top: 20px;
  padding-bottom: 5px;
  border-radius: 10px;
}
.title  {
    display: block;
	background: white;
	font-family: Arial;
	font-weight: Bold;
	font-size: 30px;
    width: 100%;
	padding-top: 100px;
	padding-bottom: 100px;
	border-radius: 10px;
	text-align: center;
	vertical-align: middle;
}

.button1 {background-color: #04AA6D;} /* Green */
.button2 {background-color: #008CBA;} /* Blue */
</style>

        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>StPaul Fire System</title>
    </head>
    <body style="background-color:grey;">
        <h1 style="font-family: Arial; color: red; text-align: center; background-color: black; padding: 15px; border-radius: 8px;">St Paul Fire System</h1>
        <p style="font-family: Arial; font-size: 24px; text-align: center;">Fire Alarm System Notification</p>
        <div class=title>
            <span style="font-weight: bold; color: yellow;"> SYSTEM TROUBLE <br></span> The fire alarm system is not active, please inspect the system console to determine the issue.
        <br> </div>
		<div style="text-align: center;"><br><br>
		<a href="https://fire.stpaulwv.org" class="button button1">System Status Page</a>
		</div>
    </body>
    </html>
      )rawliteral";

  // Poll alarm pins
  if (digitalRead(ALARM_PIN) == HIGH && !alarmAlerted) {
    alarmAlerted = true;
    writeLog(getCurrentTimeStr() + " Fire Alarm Detected!");
    sendNotification(getCurrentTimeStr() + " Fire Alarm Activated!");
    sendemail(alarmemail);
  } else if (digitalRead(ALARM_PIN) == LOW && alarmAlerted) {
    alarmAlerted = false;
    writeLog(getCurrentTimeStr() + " Fire Alarm Cleared!");
    sendNotification(getCurrentTimeStr() + " Fire Alarm Cleared!");
    sendemail(clearedemail);
  }

  // Poll trouble pin
  if (digitalRead(TROUBLE_PIN) == HIGH && !troubleAlerted) {
    troubleAlerted = true;
    writeLog(getCurrentTimeStr() + " Trouble Alarm Detected!");
    sendNotification(getCurrentTimeStr() + " Trouble Alarm Activated!");
    sendemail(troubleemail);
  } else if (digitalRead(TROUBLE_PIN) == LOW && troubleAlerted) {
    troubleAlerted = false;
    writeLog(getCurrentTimeStr() + " Trouble Alarm Cleared!");
    sendNotification(getCurrentTimeStr() + " Trouble Alarm Cleared!");
    sendemail(clearedemail);
  }

  server.handleClient();

  // Every hour: upload current day's log file to FTP
  if (millis() - lastFTPTime > ftpInterval) {
    lastFTPTime = millis();
    String remoteName = currentLogDate + ".log";
    String ftpResult;
    uploadLogToFTP(logFilePath, remoteName, ftpResult);
  }

  // upload current day's log file to FTP and delete if memory becomes an issue
  // Remove above if statement and replace with this one to rotate the internal logs. Also change ftpIntreval to 86400000, one day in ms
  if (millis() - lastFTPTime > ftpIntervalday) {
    lastFTPTime = millis();
    String remoteName = currentLogDate + "_daily.log";
    String ftpResult;
    bool backupSuccess = uploadLogToFTP(logFilePath, remoteName, ftpResult);
    if (backupSuccess) {
      // Delete the log file after successful backup
      if (SPIFFS.exists(logFilePath)) {
        SPIFFS.remove(logFilePath);
        writeLog(getCurrentTimeStr() + " Log file deleted after FTP backup.");
      }
   }
  }

  delay(250);
}
