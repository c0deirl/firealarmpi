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
