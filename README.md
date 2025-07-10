# Fire Alarm Pi
Fire Alarm Notification Integration with a Pi

## Development
This is the development version, Just makig changes, and seeing what works!

## Summary of Changes
Event handler assignments are set once, not inside the loop.  
Log file is always closed before opening a new one.  
Used context-specific exception handling for clean shutdown.  
All log writing functions now require the file handle (lf) as a parameter to avoid confusion and side effects.  
Added a small sleep in the main loop to avoid busy waiting.  
Directory for logs is ensured to exist.  
More robust cleanup on exit.  

## Run as a Service  
Create a "firealarm.service" file. Place that in /etc/systemd/system/  
Make sure the script is executible  

Should look like below:  

```
[Unit]  
Description=Fire Alarm Pi
After=network.target
After=systemd-user-sessions.service
After=network-online.target

[Service]
ExecStart=/path/to/button.py
Restart=on-failure

[Install]
wantdeBy=multi-user.target
```
You can then start it with:  
```
sudo systemctl start firealarm  
```
To enable it to run at boot:  
```
sudo systemctl enable firealarm
```
To stop the service:  
```
sudo systemctl stop firealarm
```
To remove the service completely:
```
sudo systemctl disable firealarm
```
