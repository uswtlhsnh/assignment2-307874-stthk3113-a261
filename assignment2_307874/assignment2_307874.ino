
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// SENSOR PINS
#define DHTPIN 4
#define DHTTYPE DHT11
#define TRIG_PIN 5
#define ECHO_PIN 18

// CAPACITIVE TOUCH
#define TOUCH_PIN 27

// OLED
#define SDA_PIN 21
#define SCL_PIN 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

DHT dht(DHTPIN, DHTTYPE);

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// Sensor values
float temperature = NAN;
float humidity = NAN;
float distance = -1;

// OLED page
int currentPage = 0;

// Touch variables
uint16_t touchBaseline = 0;
uint16_t touchThreshold = 0;
uint16_t releaseThreshold = 0;
bool touchHeld = false;

// Timers
unsigned long lastDHT = 0;
unsigned long lastDistance = 0;
unsigned long lastTouch = 0;
unsigned long lastDisplay = 0;
unsigned long lastPageSwitch = 0;

const unsigned long DHT_INTERVAL = 2000;
const unsigned long DIST_INTERVAL = 200;
const unsigned long TOUCH_INTERVAL = 40;
const unsigned long DISPLAY_INTERVAL = 200;
const unsigned long DEBOUNCE_TIME = 300;

void calibrateTouch() {
  unsigned long sum = 0;

  // Do not touch the pin while calibrating
  for (int i = 0; i < 30; i++) {
    sum += touchRead(TOUCH_PIN);
    delay(20);
  }

  touchBaseline = sum / 30;
  touchThreshold = touchBaseline * 0.70;
  releaseThreshold = touchBaseline * 0.85;

  Serial.print("Touch baseline: ");
  Serial.println(touchBaseline);

  Serial.print("Touch threshold: ");
  Serial.println(touchThreshold);
}

void setup() {
  Serial.begin(115200);

  dht.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDRESS
  )) {
    Serial.println("OLED not detected!");
    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("SMART ROOM MONITOR");
  display.println();
  display.println("Calibrating touch");
  display.println("Do not touch pin!");
  display.display();

  calibrateTouch();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("System Ready!");
  display.println("Touch to switch");
  display.display();

  delay(1000);
}

void readDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t) && !isnan(h)) {
    temperature = t;
    humidity = h;
  } else {
    temperature = NAN;
    humidity = NAN;
    Serial.println("DHT read error");
  }
}

void readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration =
    pulseIn(ECHO_PIN, HIGH, 30000UL);

  if (duration > 0) {
    distance = duration * 0.0343 / 2.0;
  } else {
    distance = -1;
  }
}

void checkTouch() {
  uint16_t touchValue = touchRead(TOUCH_PIN);

  if (touchBaseline == 0) {
    return;
  }

  // Finger touches male pin
  if (!touchHeld &&
      touchValue < touchThreshold) {

    if (millis() - lastPageSwitch >
        DEBOUNCE_TIME) {

      currentPage = 1 - currentPage;

      lastPageSwitch = millis();

      Serial.print("Page changed to: ");
      Serial.println(currentPage + 1);
    }

    touchHeld = true;
  }

  // Finger released
  if (touchHeld &&
      touchValue > releaseThreshold) {
    touchHeld = false;
  }
}

void showEnvironmentPage() {
  display.println("ROOM ENVIRONMENT");
  display.println("---------------------");

  display.print("Temp: ");

  if (!isnan(temperature)) {
    display.print(temperature, 1);
    display.println(" C");
  } else {
    display.println("Error");
  }

  display.print("Humidity: ");

  if (!isnan(humidity)) {
    display.print(humidity, 1);
    display.println(" %");
  } else {
    display.println("Error");
  }

  display.println();

  if (isnan(temperature)) {
    display.println("SENSOR ERROR");
  } else if (temperature > 35) {
    display.println("HIGH TEMP!");
  } else {
    display.println("Status: NORMAL");
  }

  display.println("Touch: Next Page");
}

void showDistancePage() {
  display.println("PROXIMITY MONITOR");
  display.println("---------------------");

  display.print("Distance: ");

  if (distance >= 0) {
    display.print(distance, 1);
    display.println(" cm");
  } else {
    display.println("No Echo");
  }

  display.println();

  if (distance < 0) {
    display.println("Status: NO ECHO");
  } else if (distance < 10) {
    display.println("Status: TOO CLOSE!");
  } else {
    display.println("Status: SAFE");
  }

  display.println();
  display.println("Touch: Next Page");
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);

  if (currentPage == 0) {
    showEnvironmentPage();
  } else {
    showDistancePage();
  }

  display.display();
}

void loop() {
  unsigned long now = millis();

  // Update DHT11
  if (now - lastDHT >= DHT_INTERVAL) {
    lastDHT = now;
    readDHT();
  }

  // Update HC-SR04
  if (now - lastDistance >= DIST_INTERVAL) {
    lastDistance = now;
    readDistance();
  }

  // Check capacitive touch
  if (now - lastTouch >= TOUCH_INTERVAL) {
    lastTouch = now;
    checkTouch();
  }

  // Update OLED
  if (now - lastDisplay >= DISPLAY_INTERVAL) {
    lastDisplay = now;
    updateDisplay();
  }
}
