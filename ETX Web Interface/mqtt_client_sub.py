import paho.mqtt.client as paho
import mysql.connector
import json
from datetime import datetime
import logging

# import time

# def on_connect(client, userdata, flags, rc):
#     print('CONNACK received with code %d.' % (rc))

def on_subscribe(client, userdata, mid, granted_qos):
    print("Subscribed: "+str(mid)+" "+str(granted_qos))

def on_message(client, userdata, msg):
    # print(msg.topic+" "+str(msg.qos)+" "+str(msg.payload)) 
    data_in=json.loads(msg.payload)
    # print(data_in['ID'])
    now=datetime.today().strftime('%Y-%m-%d %H:%M:%S')
    # now=time.ctime(time.time())
    try:
        db=mysql.connector.connect(
          host="localhost",
          user="vegaav",
          password="vegaav123",
          database="mqtt_data_log"
        )

        dbcursor=db.cursor()
        sql="INSERT INTO log (ID,A1,V1,E1,P1,S1,T1,A2,V2,E2,P2,S2,T2,A3,V3,E3,P3,S3,T3,ax,ay,az,X,Y,Z,mx,my,mz,timestamp) \
        VALUES (%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)"
        val=(data_in['ID'],data_in['A1'],data_in['V1'],data_in['E1'],data_in['P1'],data_in['S1'],data_in['T1'],data_in['A2'],data_in['V2'],data_in['E2'],data_in['P2'],data_in['S2'],data_in['T2'],data_in['A3'],data_in['V3'],data_in['E3'],data_in['P3'],data_in['S3'],data_in['T3'],data_in['ax'],data_in['ay'],data_in['az'],data_in['X'],data_in['Y'],data_in['Z'],data_in['mx'],data_in['my'],data_in['mz'],now)
        dbcursor.execute(sql,val)

        db.commit()
    except:
        logging.error('%s raised an error','Database')
        
client = paho.Client(clean_session=True)
# client.on_connect = on_connect
client.on_subscribe = on_subscribe
client.on_message = on_message
client.connect('broker.mqtt-dashboard.com', 1883)
client.subscribe('TopicHidden',qos=0)
client.loop_forever()


