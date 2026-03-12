# Patrol Robot

An AI-powered patrol robot built with ESP32 and Raspberry Pi. Uses computer vision (YOLO) for person and bottle detection, with obstacle avoidance, environmental sensors, and a web dashboard for monitoring and manual control.

## Architecture

- **ESP32** – Motor control, ultrasonic sensor, DHT11 (temp/humidity), light sensor, servo, OLED display. Communicates with the Pi over serial.
- **Raspberry Pi** – Runs YOLOv8 for object detection, streams video, and serves a Flask web dashboard.

## Hardware

### ESP32 Components
- L298N motor driver (DC motors)
- HC-SR04 ultrasonic sensor
- DHT11 temperature/humidity sensor
- Light sensor
- Servo motor
- SSD1306 OLED (128×64)

### Raspberry Pi
- Pi Camera 2
- USB serial connection to ESP32

## Project Structure

```
patrolRobot/
├── esp_working/       # Main ESP32 firmware
├── raspberry/         # Pi Python app (AI brain + web server)
└── test_everything/   # ESP32 test sketch for all sensors
```

## Setup

### ESP32 (Arduino IDE)
1. Install ESP32 board support
2. Install libraries: Adafruit GFX, Adafruit SSD1306, DHT, ESP32Servo
3. Open `esp_working/esp_working.ino` and upload

### Raspberry Pi
1. Install dependencies:
   ```bash
   pip install opencv-python pyserial flask ultralytics picamera2
   ```
2. Download YOLO model (auto-downloaded on first run, or place `yolov8n.pt` in the project)
3. Connect ESP32 via USB (`/dev/ttyACM0`)
4. Run:
   ```bash
   python raspberry/ai_brain.py
   ```

### Web Dashboard
Open `http://<raspberry-pi-ip>:5000` in a browser to:
- View live camera feed with detection overlays
- See sensor data (distance, temp, humidity, light)
- Control the robot manually (FORWARD, BACK, LEFT, RIGHT, STOP)
- Toggle between AI and manual mode

## Serial Protocol

**Pi → ESP32:**
- `FORWARD`, `BACK`, `LEFT`, `RIGHT`, `STOP` – motor commands
- `PERSON`, `NOPERSON` – person detection state
- `SERVO:<angle>` – set servo angle (70–110)

**ESP32 → Pi:**
- `DIST:<cm>;TEMP:<°C>;HUM:<%>;LIGHT:<0|1>`

## Behavior

- **AI mode:** Detects persons (follows/tracks) and bottles (stops). Sends motor commands based on object position.
- **Manual mode:** User controls via web buttons.
- **Obstacle avoidance:** ESP32 stops and turns when ultrasonic detects objects &lt; 20 cm.

## License

MIT
