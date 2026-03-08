#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

#define TRIG_PIN 32
#define ECHO_PIN 35
#define DHTPIN 33
#define DHTTYPE DHT22
#define SOIL_PIN 34
#define FAN_PIN 5
#define PUMP_PIN 14
#define SD_CS 4   // SD card Chip Select

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
File dataFile;

char auth[] = "Your_Blynk_Auth_Token";  // replace with your Blynk token
char ssid[] = "Your_WiFi_SSID";         // WiFi SSID
char pass[] = "Your_WiFi_PASSWORD";     // WiFi password

unsigned long lastLogTime = 0;  
const long logInterval = 60000; // log every 1 min (adjust as needed)

// Soil moisture calibration values (adjust after testing)
const int SOIL_WET = 1500;
const int SOIL_DRY = 3000;

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Plant");
  delay(1500);
  lcd.clear();

  // DHT
  dht.begin();

  // SD Card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card failed!");
  } else {
    Serial.println("SD Card initialized.");
  }

  // WiFi + Blynk
  WiFi.begin(ssid, pass);
  Blynk.begin(auth, ssid, pass);

  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  delay(1000);
  lcd.clear();
}

void loop() {
  Blynk.run();

  float Temp = dht.readTemperature();
  float Huma = dht.readHumidity();
  int moistureValue = analogRead(SOIL_PIN);

  // Ultrasonic distance
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;

  // FAN CONTROL 
  if (Temp > 30.0) {
    digitalWrite(FAN_PIN, HIGH);  // Fan ON
  } else {
    digitalWrite(FAN_PIN, LOW);   // Fan OFF
  }

  // PUMP CONTROL
  if (moistureValue > SOIL_DRY) {
    digitalWrite(PUMP_PIN, HIGH);  // Pump ON when dry
  } else {
    digitalWrite(PUMP_PIN, LOW);   // Pump OFF
  }

  // LCD DISPLAY
  lcd.clear();
  if (isnan(Temp) || isnan(Huma)) {
    lcd.setCursor(0, 0);
    lcd.print("DHT Error");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("H:");
    lcd.print(distance, 0);
    lcd.print("cm ");

    lcd.setCursor(8, 0);
    lcd.print("S:");
    if (moistureValue < SOIL_WET) lcd.print("Wet");
    else if (moistureValue < SOIL_DRY) lcd.print("Moist");
    else lcd.print("Dry");

    lcd.setCursor(0, 1);
    lcd.print("T:");
    lcd.print(Temp, 1);
    lcd.print("C ");
    lcd.print("H:");
    lcd.print(Huma, 0);
    lcd.print("%");
  }

  // BLYNK SEND 
  Blynk.virtualWrite(V0, Temp);         // Temperature
  Blynk.virtualWrite(V1, Huma);         // Humidity
  Blynk.virtualWrite(V2, moistureValue); // Soil Moisture
  Blynk.virtualWrite(V3, distance);     // Plant Height

  // SD CARD LOG
  if (millis() - lastLogTime > logInterval) {
    lastLogTime = millis();
    dataFile = SD.open("/plant_log.txt", FILE_APPEND);
    if (dataFile) {
      dataFile.print("Temp: "); dataFile.print(Temp);
      dataFile.print(", Humidity: "); dataFile.print(Huma);
      dataFile.print(", Soil: "); dataFile.print(moistureValue);
      dataFile.print(", Height: "); dataFile.println(distance);
      dataFile.close();
      Serial.println("Data logged to SD.");
    }
  }

  delay(2000);
}