#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESP32Servo.h>

// -------------------- PIN DEFINITIONS --------------------

// L298N
#define IN1 18
#define IN2 19
#define IN3 21
#define IN4 20
#define ENA 47
#define ENB 48

// Ultrasonic
#define TRIG 12
#define ECHO 13

// DHT11
#define DHTPIN 15
#define DHTTYPE DHT11

// Light sensor (analog)
#define LIGHT_PIN 5

// Servo
#define SERVO_PIN 37

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ----------------------------------------------------------

DHT dht(DHTPIN, DHTTYPE);
Servo myServo;

void setup() {
  Serial.begin(115200);

  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Ultrasonic
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Light sensor
  pinMode(LIGHT_PIN, INPUT);

  // Start DHT
  dht.begin();

  // Start servo
  myServo.attach(SERVO_PIN);

  // Start I2C for OLED
  Wire.begin(42, 41); // SDA = 42, SCK = 41

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed!");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  Serial.println("System Test Starting...");
}

// ----------------------------------------------------------

float readUltrasonic() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH);
  float distance = duration * 0.034 / 2;

  return distance;
}

void testMotors() {
  Serial.println("Testing Motors...");

  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);

  // Forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(1000);

  // Stop
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  delay(500);
}

void testServo() {
  Serial.println("Testing Servo...");
  myServo.write(0);
  delay(500);
  myServo.write(90);
  delay(500);
  myServo.write(180);
  delay(500);
  myServo.write(90);
}

// ----------------------------------------------------------

void loop() {

  testMotors();
  testServo();

  float distance = readUltrasonic();
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int lightValue = analogRead(LIGHT_PIN);

  Serial.println("------ SENSOR DATA ------");
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.println(" %");

  Serial.print("Light: ");
  Serial.println(lightValue);

  // OLED Display
  display.clearDisplay();
  display.setCursor(0, 0);

  display.print("Dist: ");
  display.print(distance);
  display.println("cm");

  display.print("Temp: ");
  display.print(temp);
  display.println("C");

  display.print("Hum: ");
  display.print(hum);
  display.println("%");

  display.print("Light: ");
  display.println(lightValue);

  display.display();

  delay(2000);
}
