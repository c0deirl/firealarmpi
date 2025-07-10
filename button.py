#!/usr/bin/env python3

# D. Greathouse
# St Paul Lutheran Church
from gpiozero import Button
import os
import time
import socket
import datetime
from notification import notify
import sys

# Define the GPIO Dry contacts from the alarm panel
alarm = Button(4, bounce_time=1, hold_time=5)
trouble = Button(17, bounce_time=1, hold_time=5)

"""
Notify list is now by NTFY app..  There is no distinction between 
Trouble and Alarm and Weekly notify lists

"""

# Log file location
logloc = "/home/dgreathouse/logs/"
os.makedirs(logloc, exist_ok=True)

# Ensure we are where we are supposed to be
os.chdir("/home/dgreathouse/python-projects")
print(os.getcwd())

# Global variables
alerted = False
sys_error = False
tested = False

def ts_now():
    now = datetime.datetime.now()
    return now.strftime("%Y-%m-%d %H:%M:%S")

def log_it(lline, lf):
    now = datetime.datetime.now()
    formattime = now.strftime("%Y-%m-%d %H:%M:%S")
    logl = "\n" + formattime + " - " + lline
    lf.write(logl)
    lf.flush()

def is_connected():
    try:
        socket.create_connection(("www.google.com", 80), timeout=5)
        return True
    except OSError:
        return False

def delete_old_files(directory, days_old, lf):
    cutoff_time = time.time() - (days_old * 24 * 60 * 60)
    for filename in os.listdir(directory):
        file_path = os.path.join(directory, filename)
        if os.path.isfile(file_path) and os.path.getmtime(file_path) < cutoff_time:
            os.remove(file_path)
            print(f"Deleted: {file_path}")
            log_it(f"Deleted: {file_path}", lf)

# Event callbacks
def alarm_active():
    print(ts_now(), "Fire Alarm Detected!")
    log_it("Fire Alarm Detected!", lf)

def alarm_held():
    global alerted
    alerted = True
    print(ts_now(), "Alarm confirmed, Notifying:")
    log_it("Alarm confirmed, Notifying:", lf)
    body = ts_now() + " Fire Alarm Activated at: \n St Paul Lutheran Church \n 3500 Broad St, Parkersburg \nThis is an automated message"
    notify(body)
    print(ts_now(), "Notification Complete.")
    log_it("Notification Complete.", lf)

def alarm_clear():
    global alerted
    if alerted:
        alerted = False
        print(ts_now(), "Alarm Clear Notifying:")
        log_it("Alarm Clear Notifying:", lf)
        body = ts_now() + " Fire Alarm has cleared at: \n St Paul Lutheran Church \n 3500 Broad St, Parkersburg \nThis is an automated message"
        notify(body)
        print(ts_now(), "Notification Complete.")
        log_it("Notification Complete.", lf)

def trouble_active():
    print(ts_now(), "Trouble Alarm is active")
    log_it("Trouble Alarm is active", lf)

def trouble_held():
    global sys_error
    sys_error = True
    print(ts_now(), "Trouble confirmed, Notifying:")
    log_it("Trouble confirmed, Notifying", lf)
    body = ts_now() + " Trouble Alarm is active at: \n St Paul Lutheran Church \n 3500 Broad St, Parkersburg \nThis is an automated message"
    notify(body)
    print(ts_now(), "Notification Complete.")
    log_it("Notification Complete.", lf)

def trouble_clear():
    global sys_error
    if sys_error:
        sys_error = False
        print(ts_now(), "Trouble Clear, Notifying:")
        log_it("Trouble Clear, Notifying:", lf)
        body = ts_now() + " Trouble Alarm has cleared at: \n St Paul Lutheran Church \n 3500 Broad St, Parkersburg \nThis is an automated message"
        notify(body)
        print(ts_now(), "Notification Complete.")
        log_it("Notification Complete.", lf)

def weekly_test(lf):
    print("Weekly Test")
    body = (ts_now() +
            " Weekly notification test issued...\n" +
            "System in ALARM?: " + str(alarm.is_pressed) + "\n" +
            "System in TROUBLE?: " + str(trouble.is_pressed) + "\n")
    log_it("Weekly notification test issued...\n" +
           "System in ALARM?: " + str(alarm.is_pressed) + "\n" +
           "System in TROUBLE?: " + str(trouble.is_pressed) + "\n", lf)
    notify(body)
    print(ts_now(), "Notification Complete.")

def open_logfile():
    now = datetime.datetime.now()
    filen = logloc + now.isoformat(sep=":", timespec='seconds') + ".log"
    lf = open(filen, "w")
    print("\n\n")
    print("Opening logfile at: " + logloc)
    print("Log file = " + filen)
    return lf, filen

def graceful_exit(lf):
    print("Termination signal received")
    log_it("Termination signal received", lf)
    print(ts_now(), "Fire Alarm notification SHUTDOWN!...")
    body = ts_now() + " Fire Alarm Monitoring is no longer running.."
    notify(body)
    print(ts_now(), "Notification Complete.")
    print(ts_now(), "Program Exiting...")
    log_it("Program Exiting...", lf)
    print(ts_now(), "Closing Logfile...<WAIT>")
    lf.close()
    time.sleep(10)
    sys.exit()

if __name__ == "__main__":
    # Open logfile
    lf, filen = open_logfile()
    log_it("Log file = " + filen, lf)
    print("Waiting on network...")
    log_it("Waiting on network...", lf)
    while not is_connected():
        time.sleep(1)
    print("Network is connected!")
    log_it("Network is connected!", lf)

    # Startup notification
    print(ts_now(), "Sending notification of program monitoring start...")
    log_it("Sending notification of program monitoring start...", lf)
    body = ts_now() + " Monitoring Program Initialized.."
    print(body)
    notify(body)
    print(ts_now(), "Notification Complete.")
    print(ts_now(), "Fire Alarm Monitoring is active - press CTRL_C to quit...")
    log_it("Fire Alarm Monitoring is active...", lf)

    # Set event handlers ONCE
    alarm.when_pressed = alarm_active
    alarm.when_held = alarm_held
    alarm.when_released = alarm_clear

    trouble.when_pressed = trouble_active
    trouble.when_held = trouble_held
    trouble.when_released = trouble_clear

    tested = False
    try:
        while True:
            myDateTime = datetime.datetime.now()
            hr = myDateTime.hour
            day = myDateTime.weekday()
            minute = myDateTime.minute

            # day 0=Monday, 1=Tuesday ... 5=Saturday, 6=Sunday
            if (day == 5 and hr == 12 and minute == 0 and not tested):
                tested = True
                weekly_test(lf)
                lf.close()
                lf, filen = open_logfile()
                log_it("Logfile recycle weekly test", lf)
                print(ts_now(), "Weekly Test Issued")
                print(ts_now(), "Fire Alarm Monitoring is active...")
                log_it("Removing old files...", lf)
                delete_old_files(logloc, 30, lf)
                log_it("Fire Alarm Monitoring is active...", lf)
            if (day != 5 or hr != 12 or minute != 0):
                tested = False

            time.sleep(1)  # Prevent busy wait

    except KeyboardInterrupt:
        graceful_exit(lf)
    except Exception as e:
        print(f"Unexpected error: {e}")
        log_it(f"Unexpected error: {e}", lf)
        graceful_exit(lf)
