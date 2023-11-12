import urllib.request
import json
from datetime import datetime, timedelta
import colorsys
import math
import time
import json
import paho.mqtt.client as mqtt
import os
from array import *

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError


def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

def on_message(client, userdata, msg):
    print (msg)


def update_freeleccy():
    SCOPES = ["https://www.googleapis.com/auth/calendar.readonly"]

    free_leccy_start = ''
    free_leccy_end = ''

    creds = Credentials.from_authorized_user_file("token.json", SCOPES)

    service = build("calendar", "v3", credentials=creds)

    now = datetime.utcnow().isoformat() + "Z"

    events_result = (
        service.events()
        .list(
            calendarId="primary",
            timeMin=now,
            maxResults=1,
            singleEvents=True,
            q="leccy",
            orderBy="startTime",
        )
        .execute()
    )
    events = events_result.get("items", [])

    for event in events:
        free_leccy_start = event["start"].get("dateTime", event["start"].get("date"))
        free_leccy_end = event["end"].get("dateTime", event["end"].get("date"))

    if (len(events) > 0):
        with open('/services/octoclock/page.json') as f:
            data = json.load(f)['results']

        new_results = []

        for entry in data:
            if (entry["valid_from"] >= free_leccy_start and entry["valid_to"] <= free_leccy_end):
                entry["value_exc_vat"] = -0.01
                entry["value_inc_vat"] = -0.01

            new_results.append(entry)

        json_out = {"results": new_results}

        with open("/services/octoclock/page.json", "w") as outfile:
            outfile.write(json.dumps(json_out))

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.username_pw_set(username="xxx", password="xxx")
client.connect("x.x.x.x", 1883, 60)

client.loop_start()

while True:
    end_color = (0, 1, 1)
    start_color = (1/3, 1, 1)

    now = datetime.now()
    #now = datetime.now() - timedelta(hours=-1)

    if now.minute >= 30:
        now = now + timedelta(minutes=30-now.minute, seconds=-now.second, microseconds=-now.microsecond)
    else:
        now = now + timedelta(minutes=-now.minute, seconds=-now.second, microseconds=-now.microsecond)
        if now.minute > 0:
            now = now + timedelta(minutes=30-now.minute)


    data = []

    if ((os.path.isfile('/services/octoclock/page.json') == False) or ((datetime.now() - datetime.fromtimestamp(os.path.getmtime('/services/octoclock/page.json'))).total_seconds() > 3600)):
        url = "https://api.octopus.energy/v1/products/AGILE-FLEX-22-11-25/electricity-tariffs/E-1R-AGILE-FLEX-22-11-25-J/standard-unit-rates/"
        urllib.request.urlretrieve(url, '/services/octoclock/page.json')
        url = "https://api.octopus.energy/v1/products/AGILE-FLEX-22-11-25/electricity-tariffs/E-1R-AGILE-FLEX-22-11-25-J/standard-unit-rates/?page=2"
        urllib.request.urlretrieve(url, '/services/octoclock/page2.json')
        update_freeleccy()


    with open('/services/octoclock/page.json') as f:
        data = json.load(f)['results']
    with open('/services/octoclock/page2.json') as f:
        data.extend(json.load(f)['results'])

    sorted_data = sorted(data, key=lambda e: e['valid_to'])

    filter = 0
    running_total = 0
    running_max = 0
    running_min = 1000

    current_wholesale = 0

    for val in sorted_data: 
        if (val['valid_from'][0:16] == now.isoformat()[0:16]):
            filter=24
            current_wholesale = val['value_inc_vat']
        if (filter > 0):
            filter=filter-1
            running_total = running_total + val['value_inc_vat']
            if (val['value_inc_vat'] > running_max):
                running_max = val['value_inc_vat']
            if (val['value_inc_vat'] < running_min):
                running_min = val['value_inc_vat']

    filter = 0

    val_list = []

    for val in sorted_data: 
        if (val['valid_from'][0:16] == now.isoformat()[0:16]):
            filter=24
        if (filter > 0):
            filter=filter-1
            proportion = ((val['value_inc_vat']-running_min)/(running_max-running_min))
            if (val["value_inc_vat"] > 0):
                proportion=proportion+0.01
            val_list.append(proportion)

    out={};
    out['avg_12hr']=str(math.floor(running_total/len(val_list)))

    if (len(val_list) < 24):
        for i in range(24-len(val_list)):
            val_list.append('X')

    out['current_wholesale']=str(round(current_wholesale,0))
    out['ws_vals']=[(255,255,255)];

    val_list.append(val_list[len(val_list)-1])

    print (val_list)

    for i in range(2,61):
        proportion2 = 0
        rgb_color = [];

        if (val_list[math.floor(i/2.5)] == "X") or (val_list[math.floor((i+1)/2.5)] == "X"):
            out['ws_vals'].append([0,0,0])
        else:
            if (val_list[math.floor(i/2.5)] != val_list[math.floor((i+1)/2.5)]):
                proportion2 = (val_list[math.floor(i/2.5)]+val_list[math.floor((i+1)/2.5)])/2
            else:
                proportion2 = val_list[math.floor(i/2.5)]

            if (proportion2 <= 0):
                rgb_color = (255,0,255)
            else:
                current_color = (start_color[0] + (end_color[0] - start_color[0]) * proportion2,
                                 start_color[1] + (end_color[1] - start_color[1]) * proportion2,
                                 start_color[2] + (end_color[2] - start_color[2]) * proportion2)
                
                rgb_color = array('f', colorsys.hsv_to_rgb(current_color[0], current_color[1], current_color[2]))

                rgb_color = (math.floor(rgb_color[0]*255),math.floor(rgb_color[1]*255),math.floor(rgb_color[2]*255))


            out['ws_vals'].append(rgb_color)


    current_hour = time.localtime().tm_hour
    if (current_hour > 12):
        current_hour = current_hour-12

    current_minute = time.localtime().tm_min

    numOfRotations = ((12-current_hour)*5)-(math.floor(current_minute/12))
    out['ws_vals'] = out['ws_vals'][numOfRotations:]+out['ws_vals'][:numOfRotations]

    client.publish('octoclock', json.dumps(out))
    
    time.sleep(60)