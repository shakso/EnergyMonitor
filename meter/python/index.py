from glowmarkt import *
from datetime import datetime, time
import json
import paho.mqtt.client as mqtt
import requests

HA_API_URL = "xxx"
HA_API_TOKEN = "xxx"
headers = {"Authorization": f"Bearer {HA_API_TOKEN}"}

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))


def on_message(client, userdata, msg):
    print(msg)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.username_pw_set(username="xxx", password="xxx")
client.connect("x.x.x.x", 1883, 60)

cli = BrightClient("xxx", "xxx")
ents = cli.get_virtual_entities()

total_electric = 0
total_gas = 0

electric_rate = 0
gas_rate = 0

for ent in ents:
    for res in ent.get_resources():

        t_from = datetime.combine(datetime.today(), time.min)
        t_to = datetime.now()
        period = "PT30M"

        if res.name == "electricity cost":

            t_from = res.round(t_from, period)
            t_to = res.round(t_to, period)

            rdgs = res.get_readings(t_from, t_to, period)

            for r in rdgs:
                total_electric = total_electric + r[1].value

            total_electric = total_electric + float(
                res.get_tariff().current_rates.standing_charge.value
            )
            electric_rate = float(res.get_tariff().current_rates.rate.value)

        elif res.name == "gas cost":
            t_from = res.round(t_from, period)
            t_to = res.round(t_to, period)

            rdgs = res.get_readings(t_from, t_to, period)

            for r in rdgs:
                total_gas = total_gas + r[1].value

            total_gas = total_gas + float(
                res.get_tariff().current_rates.standing_charge.value
            )
            gas_rate = float(res.get_tariff().current_rates.rate.value)

output = {}
output["dccsourced"] = {}
output["dccsourced"]["total_electric"] = total_electric / 100
output["dccsourced"]["total_gas"] = total_gas / 100
output["dccsourced"]["electric_rate"] = electric_rate / 100
output["dccsourced"]["gas_rate"] = gas_rate / 100

client.publish("glow/dccsourced", json.dumps(output))
client.loop()
client.disconnect()

# Update Home Assistant entities with proper calculated values
data = {
    "entity_id": "input_number.total_electric",
    "value": round(output["dccsourced"]["total_electric"], 2),
}
response = requests.post(HA_API_URL, json=data, headers=headers)
data = {
    "entity_id": "input_number.total_gas",
    "value": round(output["dccsourced"]["total_gas"], 2),
}
response = requests.post(HA_API_URL, json=data, headers=headers)
