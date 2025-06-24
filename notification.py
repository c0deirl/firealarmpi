# Complete Project Details: https://RandomNerdTutorials.com/raspberry-pi-send-email-python-smtp-server/

def notify(body):
 import requests
 

 url = "https://notify.codemov.com/fire"
 
 data = body
 response = requests.post(url, data=body, headers={ "Priority": "5" })

 
 print('>>SENT>>\n')


