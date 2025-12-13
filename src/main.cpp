#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <DHT.h>
#include <Adafruit_SHT31.h>
#include <Wire.h>

/* ==============================
   SERVER CONFIG
   ============================== */
#define SERVER_IP     "192.168.1.4" 
#define SERVER_PORT   "3000"
#define API_ENDPOINT  "/api/vitaltrack"

/* ==============================
   DEVICE CONFIG
   ============================== */
#define DEVICE_ID "Worker01"
#define DHTPIN 17
#define DHTTYPE DHT22
#define LED_PIN 32
#define BUZZER_PIN 25
#define SDA_PIN 21
#define SCL_PIN 22
#define BUZZER_CHANNEL 0
#define BUZZER_RESOLUTION 10

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SHT31 sht31 = Adafruit_SHT31();

/* ==============================
   WIFI CONNECTION
   ============================== */
void connectToWiFi() {
  WiFiManager wm;
  Serial.println("📡 Starting WiFi Manager...");

  if (!wm.autoConnect("VitalTrackSetup", "password")) {
    Serial.println("❌ WiFi failed. Restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.print("✅ WiFi Connected | ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void sendSensorData(float temp, float hum) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = "http://" + String(SERVER_IP) + ":" + SERVER_PORT + API_ENDPOINT;

  http.begin(url);
  

  http.addHeader("Content-Type", "application/json"); 

 
  String payload = "{";
  payload += "\"temperature\": " + String(temp, 2) + ",";
  payload += "\"humidity\": " + String(hum, 2) + ",";
  payload += "\"device\": \"" + String(DEVICE_ID) + "\"";
  payload += "}";

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    Serial.print("📤 Sent JSON | HTTP ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("❌ HTTP Error: ");
    Serial.println(http.errorToString(httpResponseCode));
  }

  http.end();
}

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);
  dht.begin();

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);

  if (sht31.begin(0x44)) {
    Serial.println("✅ SHT31 Ready");
  } else {
    Serial.println("⚠️ SHT31 Not Found (DHT22 still works)");
  }

  connectToWiFi();
}


void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(100);
    return;
 }

  float tempSum = 0;
  float humSum = 0;
  int validReads = 0;

  // 🔁 Take 5 readings to smooth noise
  for (int i = 0; i < 5; i++) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      tempSum += t;
      humSum += h;
      validReads++;
    }
    delay(200);
  }

  if (validReads == 0) {
    Serial.println("❌ Sensor read failed");
    delay(5000);
    return;
  }

  float temperature = tempSum / validReads;
  float humidity = humSum / validReads;

  sendSensorData(temperature, humidity);

  Serial.print("🌡 ");
  Serial.print(temperature, 2);
  Serial.print(" °C | 💧 ");
  Serial.print(humidity, 2);
  Serial.println(" %");

  bool danger = temperature >= 38.0;

  digitalWrite(LED_PIN, danger ? HIGH : LOW);
  ledcWriteTone(BUZZER_CHANNEL, danger ? 2000 : 0);

  delay(5000);
}