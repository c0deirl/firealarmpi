# Fire Alarm System
Fire Alarm Notification Integration with a ESP32

## Setup Instructions
Replace YOUR_WIFI_SSID, YOUR_WIFI_PASSWORD, and YOUR_TOPIC with your settings.
In Arduino IDE, install libraries:
ESP32 board support
SPIFFS
WebServer
Connect alarm/trouble buttons/switches to GPIO 4 and 17 (or modify pins).
Upload code and access logs via http://<ESP32_IP>/ from your browser.

## Features Covered
WiFi connection.
GPIO monitoring with notifications (NTFY).
Persistent logging.
Web interface for log viewing.

## Features
Startup & Monitoring Notifications: Sends a NTFY notification when the ESP32 boots and begins monitoring.
WiFi Failure Logging: Logs (and optionally notifies) if the ESP32 cannot connect to WiFi.
Webpage Enhancements:
At the top:
Live GPIO Status Indicators (Red = active HIGH, Green = inactive LOW) for alarm and trouble pins.
Current Time (auto-updating).
WiFi Signal Strength.
Logs below indicators.
The web UI uses JavaScript to poll ESP32 for live status and updates indicators/time/signal strength dynamically (without page reload).
