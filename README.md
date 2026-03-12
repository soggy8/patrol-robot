# 🤖 Patrol Robot

> AI-powered patrol robot with ESP32 + Raspberry Pi. Computer vision (YOLO) for person and bottle detection, obstacle avoidance, environmental sensors, and a web dashboard.

---

## 🏗 Architecture

| Component | Role |
|-----------|------|
| **ESP32** | Motor control, ultrasonic, DHT11, light sensor, servo, OLED. Talks to Pi over serial. |
| **Raspberry Pi** | YOLOv8 object detection, video stream, Flask web dashboard. |

---

## 📌 ESP32 Pinout

From `pins.odt`:

| Component | Signal | GPIO |
|-----------|--------|------|
| **L298N** | IN1 | 18 |
| | IN2 | 19 |
| | IN3 | 21 |
| | IN4 | 20 |
| | ENA | 47 |
| | ENB | 48 |
| **Ultrasonic (HC-SR04)** | TRIG | 12 |
| | ECHO | 13 |
| **DHT11** | DATA | 15 |
| **Light sensor** | DATA | 5 |
| **OLED (SSD1306)** | SCL | 41 |
| | SDA | 42 |
| **Servo** | PWM | 37 |

---

## 🔧 Hardware

**ESP32:** L298N, HC-SR04, DHT11, light sensor, servo, SSD1306 OLED (128×64)  
**Raspberry Pi:** Pi Camera 2, USB serial to ESP32

---

## 📁 Project Structure

```
patrolRobot/
├── esp_working/       # Main ESP32 firmware
├── raspberry/         # Pi app (AI brain + web server)
├── test_everything/   # ESP32 sensor test sketch
└── pins.odt           # Pin reference
```

---

## ⚡ Setup

### ESP32 (Arduino IDE)
1. Install ESP32 board support
2. Libraries: Adafruit GFX, Adafruit SSD1306, DHT, ESP32Servo
3. Open `esp_working/esp_working.ino` → Upload

### Raspberry Pi
```bash
pip install opencv-python pyserial flask ultralytics picamera2
python raspberry/ai_brain.py
```
- Connect ESP32 via USB (`/dev/ttyACM0`)
- YOLO model (`yolov8n.pt`) downloads on first run

### Web Dashboard
Open `http://<pi-ip>:5000`:
- Live camera feed with detection overlays
- Sensor data (distance, temp, humidity, light)
- Manual control (FORWARD, BACK, LEFT, RIGHT, STOP)
- Toggle AI / Manual mode

---

## 📡 Serial Protocol

| Direction | Format |
|-----------|--------|
| **Pi → ESP32** | `FORWARD` \| `BACK` \| `LEFT` \| `RIGHT` \| `STOP` \| `PERSON` \| `NOPERSON` \| `SERVO:<70-110>` |
| **ESP32 → Pi** | `DIST:<cm>;TEMP:<°C>;HUM:<%>;LIGHT:<0\|1>` |

---

## 🧠 Behavior

| Mode | Description |
|------|-------------|
| **AI** | Tracks persons, stops for bottles. Commands based on object position. |
| **Manual** | Web buttons control motors. |
| **Obstacle** | ESP32 stops and turns when ultrasonic &lt; 20 cm. |

---

## 📄 License

MIT
