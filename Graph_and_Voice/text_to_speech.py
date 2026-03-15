from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import pyttsx3 
import time
import threading 

#CONFIGURATION
ENDPOINT = "REPLACE_WITH_AWS_IOT_ENDPOINT" 
CLIENT_ID = "SmartStick_Grapher"
PATH_TO_CERT = "REPLACE_WITH_CERT_PATH"
PATH_TO_KEY = "REPLACE_WITH_PRIVATE_KEY_PATH"
PATH_TO_ROOT = "REPLACE_WITH_ROOT_CA_PATH"
TOPIC = "smartstick/sensors"

# AUDIO SETUP
engine = pyttsx3.init()
engine.setProperty('rate', 170) 

last_speech_time = 0
SPEECH_COOLDOWN = 2.0  

def speak_alert(text):
    """Run speech in a separate thread so it doesn't freeze the graph"""
    def run():
        engine.say(text)
        engine.runAndWait()
    thread = threading.Thread(target=run)
    thread.start()

# DATA STORAGE
MAX_POINTS = 50
left_data = deque([0] * MAX_POINTS, maxlen=MAX_POINTS)
center_data = deque([0] * MAX_POINTS, maxlen=MAX_POINTS)
right_data = deque([0] * MAX_POINTS, maxlen=MAX_POINTS)

# MQTT CALLBACK
def on_message_received(client, userdata, message):
    global last_speech_time
    try:
        payload = message.payload.decode('utf-8')
        parts = payload.split(',')
        
        # Parse Values (0 or 1)
        left = int(parts[0])
        center = int(parts[1])
        right = int(parts[2])

        # Add to graph data (with offsets for visual separation)
        left_data.append(left + 4)
        center_data.append(center + 2)
        right_data.append(right)
        
        # --- AUDIO LOGIC ---
        current_time = time.time()
        
        # Only speak if enough time has passed since the last alert
        if current_time - last_speech_time > SPEECH_COOLDOWN:
            alert_msg = ""
            if left == 1:
                alert_msg = "Object on Left"
            elif right == 1:
                alert_msg = "Object on Right"
            elif center == 1:
                alert_msg = "Watch your step"
            
            if alert_msg != "":
                print(f"Speaking: {alert_msg}")
                speak_alert(alert_msg)
                last_speech_time = current_time

    except Exception as e:
        print("Error:", e)

# CONNECT TO AWS
myMQTTClient = AWSIoTMQTTClient(CLIENT_ID)
myMQTTClient.configureEndpoint(ENDPOINT, 8883)
myMQTTClient.configureCredentials(PATH_TO_ROOT, PATH_TO_KEY, PATH_TO_CERT)
myMQTTClient.connect()
myMQTTClient.subscribe(TOPIC, 1, on_message_received)
print("Connected. Listening for sensors...")

# SETUP GRAPH
fig, ax = plt.subplots()
ax.set_ylim(-1, 6)
ax.set_title('Smart Stick Live Monitor')
ax.set_yticks([0, 1, 2, 3, 4, 5])
ax.set_yticklabels(['Safe', 'Right Alert', 'Safe', 'Dip Alert', 'Safe', 'Left Alert'])

line_left, = ax.plot(left_data, label='Left', color='red')
line_center, = ax.plot(center_data, label='Center (Dip)', color='blue')
line_right, = ax.plot(right_data, label='Right', color='green')
ax.legend(loc='upper left')

def update(frame):
    line_left.set_ydata(left_data)
    line_center.set_ydata(center_data)
    line_right.set_ydata(right_data)
    return line_left, line_center, line_right

ani = animation.FuncAnimation(fig, update, interval=100)
plt.show()