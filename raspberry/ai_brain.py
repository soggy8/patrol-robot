import os
import time
import cv2
import serial
from flask import Flask, Response, jsonify, request
from picamera2 import Picamera2
from ultralytics import YOLO

# ---------------- SETTINGS ----------------

SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200

ser = None
esp_connected = False
last_command_sent = None

# Live state
current_command = "NONE"
detected_person = False
detected_bottle = False
manual_override = False

# Sensor stats
distance = 0
temperature = 0.0
humidity = 0.0
light = 0

# ---------------- SERIAL ----------------

def init_serial():
    global ser, esp_connected

    if os.path.exists(SERIAL_PORT):
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            time.sleep(2)
            esp_connected = True
            print("✅ ESP32 connected")
        except:
            ser = None
            esp_connected = False
            print("⚠ Failed to open serial")
    else:
        ser = None
        esp_connected = False
        print("⚠ ESP32 not detected")

def read_serial():
    global distance, temperature, humidity, light, esp_connected

    if ser and ser.in_waiting:
        try:
            line = ser.readline().decode().strip()
            if line:
                parts = line.split(";")
                for p in parts:
                    if ":" in p:
                        k, v = p.split(":")
                        if k == "DIST":
                            distance = int(v)
                        elif k == "TEMP":
                            temperature = float(v)
                        elif k == "HUM":
                            humidity = float(v)
                        elif k == "LIGHT":
                            light = int(v)
                esp_connected = True
        except:
            esp_connected = False

def send_command(cmd):
    global ser, last_command_sent, current_command, esp_connected

    if cmd == last_command_sent:
        return

    last_command_sent = cmd
    current_command = cmd

    if ser:
        try:
            ser.write((cmd + "\n").encode())
            esp_connected = True
            print("➡ Sent:", cmd)
        except:
            print("⚠ Serial lost")
            ser = None
            esp_connected = False
    else:
        print("🧠 (No ESP32)", cmd)

# ---------------- LOAD YOLO ----------------

print("Loading YOLO model...")
model = YOLO("yolov8n.pt")

# ---------------- CAMERA ----------------

picam2 = Picamera2()
picam2.configure(
    picam2.create_preview_configuration(
        main={"format": "BGR888", "size": (640, 480)}
    )
)
picam2.start()

# ---------------- FLASK ----------------

app = Flask(__name__)

# ---------------- VIDEO GENERATOR ----------------

def generate_frames():
    global detected_person, detected_bottle, manual_override

    while True:
        read_serial()

        if ser is None and os.path.exists(SERIAL_PORT):
            print("🔄 Trying reconnect...")
            init_serial()

        frame = picam2.capture_array()

        detected_person = False
        detected_bottle = False
        person_position = None
        frame_center = frame.shape[1] // 2

        if not manual_override:
            results = model(frame, imgsz=320, conf=0.5)

            for r in results:
                for box in r.boxes:
                    cls_id = int(box.cls[0])
                    label = model.names[cls_id]

                    x1, y1, x2, y2 = map(int, box.xyxy[0])
                    box_center = (x1 + x2) // 2

                    if label == "person":
                        detected_person = True
                        person_position = box_center

                    if label == "bottle":
                        detected_bottle = True

                    cv2.rectangle(frame, (x1, y1), (x2, y2), (0,255,0), 2)
                    cv2.putText(frame, label, (x1, y1-10),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.5,
                                (0,255,0), 2)

            # ---- DECISION LOGIC ----
            if detected_bottle:
                send_command("STOP")

            elif detected_person and person_position is not None:
                send_command("PERSON")

                if person_position < frame_center - 80:
                    send_command("LEFT")
                elif person_position > frame_center + 80:
                    send_command("RIGHT")
                else:
                    send_command("SLOW")

            else:
                send_command("NOPERSON")
                send_command("FORWARD")

        ret, buffer = cv2.imencode(".jpg", frame)
        frame_bytes = buffer.tobytes()

        yield (b"--frame\r\n"
               b"Content-Type: image/jpeg\r\n\r\n" +
               frame_bytes + b"\r\n")

# ---------------- WEB ROUTES ----------------

@app.route("/")
def index():
    return """
<html>
<head>
<title>AI Patrol Robot</title>
<style>
body { background:#111; color:white; font-family:Arial; }
.container { display:flex; justify-content:center; gap:40px; }
.panel { background:#222; padding:20px; border-radius:10px; width:300px; }
button { width:100px; margin:5px; padding:10px; }
</style>

<script>
async function updateStatus(){
    const r = await fetch('/status');
    const d = await r.json();

    document.getElementById("esp").innerText = d.esp;
    document.getElementById("cmd").innerText = d.command;
    document.getElementById("person").innerText = d.person;
    document.getElementById("bottle").innerText = d.bottle;
    document.getElementById("mode").innerText = d.mode;
    document.getElementById("dist").innerText = d.distance + " cm";
    document.getElementById("temp").innerText = d.temperature + " °C";
    document.getElementById("hum").innerText = d.humidity + " %";
    document.getElementById("light").innerText = d.light ? "Bright" : "Dark";
}
setInterval(updateStatus, 1000);

async function sendManual(cmd){
    await fetch('/manual',{
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body:JSON.stringify({command:cmd})
    });
}

async function toggleMode(){
    await fetch('/toggle_mode', {
        method: 'POST'
    });
}
</script>
</head>

<body>
<h1 style="text-align:center;">AI Patrol Robot Dashboard</h1>

<div class="container">
    <div>
        <img src="/video_feed" width="720">
    </div>

    <div class="panel">
        <h2>Status</h2>
        ESP32: <span id="esp">...</span><br>
        Command: <span id="cmd">...</span><br>
        Person: <span id="person">...</span><br>
        Bottle: <span id="bottle">...</span><br>
        Mode: <span id="mode">...</span><br><br>

        <strong>Sensors:</strong><br>
        Distance: <span id="dist">...</span><br>
        Temperature: <span id="temp">...</span><br>
        Humidity: <span id="hum">...</span><br>
        Light: <span id="light">...</span><br><br>

        <h2>Manual Control</h2>
        <button onclick="sendManual('FORWARD')">FORWARD</button>
        <button onclick="sendManual('BACK')">BACK</button>
        <button onclick="sendManual('LEFT')">LEFT</button>
        <button onclick="sendManual('RIGHT')">RIGHT</button>
        <button onclick="sendManual('STOP')">STOP</button>
        <br><br>
        <button onclick="toggleMode()">Toggle AI / Manual</button>
    </div>
</div>

</body>
</html>
"""

@app.route("/video_feed")
def video_feed():
    return Response(generate_frames(),
        mimetype="multipart/x-mixed-replace; boundary=frame")

@app.route("/status")
def status():
    return jsonify({
        "esp": "Connected" if esp_connected else "Not Connected",
        "command": current_command,
        "person": "Yes" if detected_person else "No",
        "bottle": "Yes" if detected_bottle else "No",
        "mode": "Manual" if manual_override else "AI",
        "distance": distance,
        "temperature": temperature,
        "humidity": humidity,
        "light": light
    })

@app.route("/manual", methods=["POST"])
def manual():
    global manual_override
    data = request.json
    cmd = data.get("command", "").upper()

    if cmd:
        manual_override = True
        send_command(cmd)

    return jsonify({"status":"ok"})

@app.route("/toggle_mode", methods=["POST"])
def toggle_mode():
    global manual_override
    manual_override = not manual_override
    return jsonify({"mode": "Manual" if manual_override else "AI"})

# ---------------- RUN ----------------

if __name__ == "__main__":
    print("🚀 Starting AI Patrol System with Sensors...")
    init_serial()
    app.run(host="0.0.0.0", port=5000, threaded=True)