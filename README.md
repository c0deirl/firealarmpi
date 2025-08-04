# Fire Alarm System
Fire Alarm Notification Integration with a ESP32

## Setup Instructions
- Replace SSID, Wifi Password , and NTFY Topic with your settings.

- In Arduino IDE, install libraries:
  - ESP32 board support
  - SPIFFS
  - WebServer

- Connect alarm input to GPIO 4  
- Connect Trouble Input to GPIO 17    
- Access logs via http://<ESP32_IP>/ from your browser  
- Logs are backed up to an FTP server every hour  
- Logs are backed up remotely and deleted from the ESP32 daily  

## Features Covered
- WiFi connection.
- GPIO monitoring with notifications (NTFY).
- Persistent logging.
- Web interface for log viewing.
- Remote FTP Backup of the log files

## Features
- Startup & Monitoring Notifications: Sends a NTFY notification when the ESP32 boots and begins monitoring.  
- NTFY Notifications when the GPIO pins are triggered.  
- WiFi Failure Logging: Logs if the ESP32 cannot connect to WiFi.
#### TO DO
 - [ ] Email Notifications  
 - [ ] Relay Outputs  

## Webpage:
- Live GPIO Status Indicators (Red = active HIGH, Green = inactive LOW) for alarm and trouble pins.  
- Current Time.  
- WiFi Signal Strength.  
- Logs.  

The web UI uses JavaScript to poll ESP32 for live status and updates indicators/time/signal strength dynamically (without page reload).
