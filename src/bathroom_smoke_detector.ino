/**
 * IoT Bathroom Cigarette Smoke & Gas Detection System
 * ----------------------------------------------------
 * Microcontroller : NodeMCU ESP8266
 * Sensors         : MQ-2 (Gas & Smoke), DHT22 (Temperature & Humidity)
 * Actuators       : Active Buzzer, I2C LCD 1602
 * Connectivity    : Wi-Fi (802.11 b/g/n), LINE Notify API
 *
 * Course          : Introduction to Computer Engineering (ปีการศึกษา 2566)
 * Institution     : มหาวิทยาลัยเทคโนโลยีราชมงคลอีสาน (RMUTI)
 */

#include <ESP8266WiFi.h>
#include <TridentTD_LineNotify.h>
#include <DHT.h>
#include <LCD_I2C.h>

// --- Configuration ---
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"
#define LINE_TOKEN      "YOUR_LINE_NOTIFY_TOKEN"

// --- Pin Definitions ---
const int PIN_SENSOR_DHT = D5;  // DHT22 Data Pin
const int PIN_SENSOR_MQ  = A0;  // MQ-2 Analog Pin
const int PIN_BUZZER     = D0;  // Active Buzzer Pin

#define DHTTYPE DHT22
DHT dht(PIN_SENSOR_DHT, DHTTYPE);
LCD_I2C lcd(0x27, 16, 2);

// --- Thresholds & Timers ---
const int SMOKE_THRESHOLD_PPM = 250;
unsigned long lastDisplayTime = 0;
const unsigned long DISPLAY_INTERVAL = 3000;

void sendLineAlert(int gasLevel);
void triggerAlarm(int gasLevel);

void setup() {
  pinMode(PIN_SENSOR_DHT, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  lcd.begin();
  dht.begin();
  lcd.backlight();

  Serial.begin(115200);
  Serial.println();
  Serial.println("System Initializing: IoT Smoke Detector");

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to Wi-Fi: %s\n", WIFI_SSID);

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print(">> WiFi <<");

  int dotCol = 0;
  while (WiFi.status() != WL_CONNECTED) {
    lcd.setCursor(dotCol, 1);
    lcd.print(".");
    Serial.print(".");
    delay(400);
    dotCol++;
    if (dotCol >= 16) {
      dotCol = 0;
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print(">> WiFi <<");
    }
  }

  Serial.printf("\nWi-Fi Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  LINE.setToken(LINE_TOKEN);

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("System Ready");
  delay(1500);
}

void loop() {
  int gasPPM = analogRead(PIN_SENSOR_MQ);
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Warning: Failed to read from DHT sensor!"));
    return;
  }

  // Display Mode 1: Gas PPM & Temperature
  if ((millis() - lastDisplayTime) >= DISPLAY_INTERVAL) {
    unsigned long switchStart = millis();
    do {
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Gas: ");
      lcd.print(gasPPM);
      lcd.print(" ppm");

      lcd.setCursor(2, 1);
      lcd.print("Temp: ");
      lcd.print(temperature, 1);
      lcd.print(" C");

      triggerAlarm(gasPPM);
      delay(200);
    } while ((millis() - switchStart) <= 5000);
    lastDisplayTime = millis();
  }

  // Display Mode 2: Gas PPM & Relative Humidity
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Gas: ");
  lcd.print(gasPPM);
  lcd.print(" ppm");

  lcd.setCursor(3, 1);
  lcd.print("RH: ");
  lcd.print(humidity, 1);
  lcd.print(" %");

  triggerAlarm(gasPPM);
  delay(200);
}

void triggerAlarm(int gasLevel) {
  if (gasLevel >= SMOKE_THRESHOLD_PPM) {
    for (int i = 0; i < 4; i++) {
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("! DANGER !");
      lcd.setCursor(2, 1);
      lcd.print("Gas = ");
      lcd.print(gasLevel);
      lcd.print(" ppm");

      digitalWrite(PIN_BUZZER, HIGH);
      delay(100);
      digitalWrite(PIN_BUZZER, LOW);
      delay(100);
    }
    sendLineAlert(gasLevel);
  }
}

void sendLineAlert(int gasLevel) {
  String message = "\nตรวจพบควันบุหรี่ในห้องน้ำ!\nค่าตรวจจับก๊าซ: " + String(gasLevel) + " ppm (เกินเกณฑ์ปลอดภัย 250 ppm)";
  LINE.notify(message);
}
