#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <Wire.h>

// ---------------- PIN DEFINITIONS ----------------
#define IN1 18
#define IN2 19
#define IN3 21
#define IN4 20
#define ENA 47
#define ENB 48

#define TRIG 12
#define ECHO 13

#define DHTPIN 15
#define DHTTYPE DHT11

#define LIGHT_PIN 5
#define SERVO_PIN 37

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 42
#define OLED_SCL 41

#define PWM_FREQ 1000
#define PWM_RESOLUTION 8
#define PWM_CHANNEL_A 0
#define PWM_CHANNEL_B 1

// ---------------- GLOBALS ----------------
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHTPIN, DHTTYPE);
Servo servoMotor;

String currentCommand = "STOP";
bool avoidingObstacle = false;
bool personDetected = false;

int motorSpeed = 200;

// Servo scan settings
int servoCenter = 90;          // center reference
int servoMin = 70;             // left limit
int servoMax = 110;            // right limit
int servoAngle = servoCenter;
int scanDirection = 1;
unsigned long lastServoMove = 0;

// ---------------- MOTOR FUNCTIONS ----------------
void applySpeed(int speed) {
  ledcWrite(PWM_CHANNEL_A, speed);
  ledcWrite(PWM_CHANNEL_B, speed);
}

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// ---------------- ULTRASONIC ----------------
long readDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long duration = pulseIn(ECHO, HIGH, 30000);
  return duration * 0.034 / 2;
}

// ---------------- SERVO SCANNING ----------------
void scanServo() {
  if (millis() - lastServoMove < 40) return;
  lastServoMove = millis();

  servoAngle += scanDirection;

  if (servoAngle >= servoMax) {
    servoAngle = servoMax;
    scanDirection = -1;
  }

  if (servoAngle <= servoMin) {
    servoAngle = servoMin;
    scanDirection = 1;
  }

  servoMotor.write(servoAngle);
}

// ---------------- SERIAL HANDLER ----------------
void handleCommand(String cmd) {
  cmd.trim();
  currentCommand = cmd;

  if (cmd == "PERSON") {
    personDetected = true;
    return;
  }
  if (cmd == "NOPERSON") {
    personDetected = false;
    return;
  }
  if (cmd.startsWith("SERVO:")) {
    int angle = cmd.substring(6).toInt();
    angle = constrain(angle, servoMin, servoMax);
    servoMotor.write(angle);
    servoAngle = angle;
    return;
  }

  if (cmd == "FORWARD") moveForward();
  else if (cmd == "BACK") moveBackward();
  else if (cmd == "LEFT") turnLeft();
  else if (cmd == "RIGHT") turnRight();
  else if (cmd == "STOP") stopMotors();
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LIGHT_PIN, INPUT);

  dht.begin();

  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENA, PWM_CHANNEL_A);

  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENB, PWM_CHANNEL_B);

  applySpeed(motorSpeed);

  servoMotor.attach(SERVO_PIN);
  servoMotor.write(servoCenter);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
}

// ---------------- LOOP ----------------
void loop() {
  // Serial commands from Pi
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    handleCommand(cmd);
  }

  long distance = readDistance();
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int lightValue = digitalRead(LIGHT_PIN);

  // ----- OBSTACLE AVOIDANCE -----
  if (distance > 0 && distance < 20) {
    avoidingObstacle = true;
  }

  if (avoidingObstacle) {
    stopMotors();
    turnRight();
    delay(150);

    if (readDistance() > 25) {
      avoidingObstacle = false;
      moveForward();
    }
  } else {
    if (currentCommand == "FORWARD") moveForward();
    else if (currentCommand == "BACK") moveBackward();
    else if (currentCommand == "LEFT") turnLeft();
    else if (currentCommand == "RIGHT") turnRight();
    else stopMotors();
  }

  // ----- SERVO LOGIC -----
  if (!personDetected && !avoidingObstacle) {
    scanServo(); // Only scan if no person detected
  }

  // ----- DISPLAY -----
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Person: "); display.println(personDetected ? "YES" : "NO");
  display.print("Servo: "); display.println(servoAngle);
  display.print("Dist: "); display.println(distance);
  display.print("Temp: "); display.println(temperature);
  display.print("Hum: "); display.println(humidity);
  display.print("Light: "); display.println(lightValue ? "Bright" : "Dark");
  display.display();

  // ----- SERIAL STATUS TO PI -----
  Serial.print("DIST:"); Serial.print(distance);
  Serial.print(";TEMP:"); Serial.print(temperature);
  Serial.print(";HUM:"); Serial.print(humidity);
  Serial.print(";LIGHT:"); Serial.println(lightValue);

  delay(40);
}
