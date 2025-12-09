#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <DHT.h>
#include <Adafruit_SHT31.h>
#include <Wire.h>

// --- API Endpoint Configuration ---
const char* serverPath = "http://192.168.1.2:3000/api/vitaltrack";

// --- Sensor/Device Settings ---
#define DEVICE_ID "Worker01"
#define DHTPIN 17
#define DHTTYPE DHT22
#define LED_PIN 32
#define BUZZER_PIN 25
#define SDA_PIN 21
#define SCL_PIN 22
#define BUZZER_CHANNEL 0
#define BUZZER_RESOLUTION 10  // improved resolution

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SHT31 sht31 = Adafruit_SHT31();

void connectToWiFi() {
  WiFiManager wm;
  Serial.println("Starting WiFi Configuration...");

  if (!wm.autoConnect("VitalTrackSetup", "password")) {
    Serial.println("Failed to connect and timed out.");
    delay(3000);
    ESP.restart();
  }

  Serial.print("WiFi connected! IP: ");
  Serial.println(WiFi.localIP());
}

void sendSensorData(float temp, float hum) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String httpURL = serverPath;
    httpURL += "?device=" + String(DEVICE_ID);
    httpURL += "&temp=" + String(temp, 1);
    httpURL += "&humidity=" + String(hum, 1);

    http.begin(httpURL);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("HTTP Error: ");
      Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  dht.begin();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(BUZZER_PIN, OUTPUT);

  // Proper LEDC setup
  ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);

  if (!sht31.begin(0x44)) {
    Serial.println("SHT31 NOT FOUND!");
  } else {
    Serial.println("SHT31 ONLINE");
  }

  connectToWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(100);
    return;
  }

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  bool dhtOK = !(isnan(temperature) || isnan(humidity));

  if (!dhtOK) {
    Serial.println("DHT22 read error!");
  } else {
    sendSensorData(temperature, humidity);

    Serial.print("Temp: ");
    Serial.print(temperature);
    Serial.print(" | Hum: ");
    Serial.println(humidity);
  }

  bool danger = false;

  // force test threshold
  if (dhtOK && temperature > 2.0) {
    Serial.println(">> HIGH TEMP DETECTED <<");
    danger = true;
  }

  if (danger) {
    digitalWrite(LED_PIN, HIGH);
    ledcWriteTone(BUZZER_CHANNEL, 2000); // buzzer ON
  } else {
    digitalWrite(LED_PIN, LOW);
    ledcWriteTone(BUZZER_CHANNEL, 0); // buzzer OFF
  }

  delay(5000);
}
