#!/usr/bin/env python3

# D. Greathouse
# St Paul Lutheran Church
from gpiozero import Button

import os
import time
import socket
import datetime
from notification import notify
# from yahoo import notify

import tty
import sys
import termios

#define the GPIO Dry contacts from the alarm panel
alarm = Button(4,bounce_time=1,hold_time=5)
trouble = Button(17,bounce_time=1,hold_time=5)

"""
Notify list is now by NTFY app..  There is no distinction between 
Trouble and Alarm and Weekly notify lists

"""

# Initialize some variables
logloc = "/home/dgreathouse/logs/"
alerted = False
sys_error = False
formattime = ""
first_pass = True
global tested
tested = False
result = False
x=0
count = 0

notify_lst = ""
trouble_lst = ""
#Make sure we are where we are supposed to be...
os.chdir("/home/dgreathouse/python-projects")
print(os.getcwd())

#define the functions we will use
def ts_now():  #needed time formats

    now=datetime.datetime.now()
    formattime = now.strftime("%Y-%m-%d %H:%M:%S")
    return(formattime)


def alarm_active(): #this is an immediate detection
    print (ts_now(),"Fire Alarm Detected!")
    log_it("Fire Alarm Detected!")

def alarm_held(): #this is a confirmed (not intermittant) alarm NEED ACTION notify EVERONE
    global alerted
    alerted = True
    print(ts_now(),"Alarm confirmed, Notifying:")
    log_it("Alarm confirmed, Notifying:")
    body = ts_now()+" Fire Alarm Activated at: \n St Paul Lutheran Church \n 3500 Broad St, Parkersburg \nThis is an automated message"
    notify(body)
    print(ts_now(),"Notification Complete.")
    log_it("Notification Complete.")

def alarm_clear(): #alarm has cleared Notify EVERYONE
    global alerted
    if alerted:
        alerted = False
        print(ts_now(),"Alarm Clear Notifying:")
        log_it("Alarm Clear Notifying:")
        body = ts_now()+" Fire Alarm has cleared at: \n St Paul Lutheran Church \n 3500 Broad St, Parkersburg \nThis is an automated message"
        notify(body)
        print(ts_now(),"Notification Complete.")
        log_it("Notification Complete.")


def trouble_active(): # this is an immediate trouble alarm
    print(ts_now(),"Trouble Alarm is active")
    log_it("Trouble Alarm is active")

def trouble_held(): #trouble alarm has maintained and is confirmed Notify
    global sys_error
    sys_error = True
    print(ts_now(),"Trouble confirmed, Notifying:")
    log_it("Trouble confirmed, Notifying")
    body = ts_now()+"Trouble Alarm is active at: \n St Paul Lutheran Church \n 3500 Broad St, Parkersburg \nThis is an automated message"
    notify(body)
    print(ts_now(),"Notification Complete.")
    log_it("Notification Complete.")


def trouble_clear(): #trouble alarm has cleared, notify 
    global sys_error
    if sys_error:
        sys_error = False
        print(ts_now(),"Trouble Clear, Notifying:")
        log_it("Trouble Clear, Notifying:")

        body = ts_now()+" Trouble Alarm has cleared at: \n St Paul Lutheran Church \n 3500 Broad St, Parkersburg \nThis is an automated message"
        notify(body)
        print(ts_now(),"Notification Complete.")
        log_it("Notification Complete.")

def weekly_test(): #weekly notification for everyone, give status
    print("Weekly Test")
    body = ts_now()+" Weekly notiication test issued...\n"+"System in ALARM?:"+str(alarm.is_pressed)+"\nSystem in TROUBLE?:"+str(trouble.is_pressed)+"\n"
    log_it(" Weekly notiication test issued...\n"+"System in ALARM?:"+str(alarm.is_pressed)+"\nSystem in TROUBLE?:"+str(trouble.is_pressed)+"\n")
    notify(body)
    print(ts_now(),"Notification Complete.")

def delete_old_files(directory, days_old):
    """Deletes files older than a specified number of days in a directory.

    Args:
        directory: The path to the directory to search.
        days_old: The number of days old a file must be to be deleted.
    Example usage: Delete files older than 30 days in the current directory
    delete_old_files(".", 30)

    """
    cutoff_time = time.time() - (days_old * 24 * 60 * 60)

    for filename in os.listdir(directory):
        file_path = os.path.join(directory, filename)
        if os.path.isfile(file_path):
            if os.path.getmtime(file_path) < cutoff_time:
                os.remove(file_path)
                print(f"Deleted: {file_path}")
                log_it(f"Deleted: {file_path}")

def log_it(lline): #log routine
    now=datetime.datetime.now()
    formattime = now.strftime("%Y-%m-%d %H:%M:%S")
    logl = "\n" + formattime + " - " + lline
    lf.write(logl)
    lf.flush()

def is_connected(): #check/wait for internet access
    try:
        socket.create_connection(("www.google.com", 80), timeout=5)
        return True
    except OSError:
        pass
    return False


#open logfile with current date/time as filename

now=datetime.datetime.now()
filen = logloc+now.isoformat(sep=":",timespec='seconds')+".log"


lf=open(filen, "w")

print("\n\n")
print("Opening logfile at: "+ logloc)
print("Log file = "+filen)
log_it("Log file = "+filen)
print("Waiting on network...")
log_it("Waiting on network...")
while not is_connected(): #wait for network
    time.sleep(1)
print("Network is connected!")
log_it("Network is connected!")

# startup of program and initialization
print(ts_now(), "Sending notification of program monitoring start...")
log_it("Sending notification of program monitoring start...")
body = ts_now()+" Monitoring Program Initialized.."
print (body)
notify(body)
print(ts_now(),"Notification Complete.")
print(ts_now(),"Fire Alarm Monitoring is active - press CTRL_C to quit...")
log_it("Fire Alarm Monitoring is active...")


x = 0
last_minute = 0

myDateTime = datetime.datetime.now()
lasthour = myDateTime.minute

#main body of program
while(True):
        try:
            alarm.when_pressed = alarm_active
            alarm.when_held = alarm_held
            alarm.when_released = alarm_clear

            trouble.when_pressed = trouble_active
            trouble.when_held = trouble_held
            trouble.when_released = trouble_clear

            myDateTime = datetime.datetime.now()

            hr = myDateTime.hour
            day = myDateTime.weekday()
            minute=myDateTime.minute

        # day 0=Monday, 1=Tuesday ... 6=Sunday
            if (day == 5 and hr == 12 and minute == 00 and not(tested)): #Saturday

                    tested = True
                    weekly_test()  #issue weekly test and status and flush/renew logfile
                    lf.close() #close logfile
                    filen = logloc+myDateTime.isoformat(sep=":",timespec='seconds')+".log" #new filename
                    lf=open(filen, "w") #open logfile
                    log_it("Logfile recycle weekly test")
                    print(ts_now(),"Weekly Test Issued")
                    print(ts_now(),"Fire Alarm Monitoring is active...")
                    log_it("Removing old files...")
                    delete_old_files("/home/dgreathouse/logs", 30) #remove old logfiles
                    log_it("Fire Alarm Monitoring is active...")
            if (day != 5 or hr !=12 or minute !=00 ):
                tested = False
        except:
    # here is where we exit normally
            print("Termination signal received")
            log_it("Termination signal received")
            print(ts_now(), "Fire Alarm notification SHUTDOWN!...")
            body = ts_now()+" Fire Alarm Monitoring is no longer running.."
            notify(body)
            print(ts_now(),"Notification Complete.")
            print(ts_now(),"Program Exiting...")
            log_it("Program Exiting...")
            print(ts_now(),"Closing Logfile...<WAIT>")
            lf.close()
            time.sleep(10)
            sys.exit()

