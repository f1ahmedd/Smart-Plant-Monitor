/*
  Smart Plant Monitor - ESP32-C5
  Reads: capacitive soil moisture + DHT11 (temp/humidity)
  Shows: 0.96" SSD1306 OLED + a web page you can open on your phone

  Libraries needed: Adafruit SSD1306, Adafruit GFX, DHT sensor library
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"

// ================== EDIT THESE ==================
const char* WIFI_SSID = "Ali Mansion";
const char* WIFI_PASS = "grumpycrown";

// Pins for a classic 30-pin ESP32 DevKit (silkscreen labels D34, D4, D21, D22)
#define SOIL_PIN 34  // ADC1, input-only: ideal for the soil sensor
#define DHT_PIN  4
#define SDA_PIN  21
#define SCL_PIN  22

// Soil calibration (see Serial Monitor for raw readings)
// SOIL_DRY = raw value with sensor in open air
// SOIL_WET = raw value with sensor tip in a glass of water
#define SOIL_DRY 2515
#define SOIL_WET 830
// ================================================

#define DHT_TYPE DHT11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);

float temperature = NAN;
float humidity = NAN;
int soilRaw = 0;
int soilPercent = 0;
bool oledOk = false;
unsigned long lastRead = 0;

// ---------- Web page ----------
const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
   <meta charset="UTF-8">
<title>Plant Monitor</title>
<style>
  body{font-family:sans-serif;background:#eef5ee;text-align:center;margin:0;padding:20px}
  h1{color:#2e7d32}
  .card{background:#fff;border-radius:16px;padding:18px;margin:14px auto;max-width:320px;
        box-shadow:0 2px 8px rgba(0,0,0,.12)}
  .val{font-size:2.6em;font-weight:bold;color:#333}
  .lbl{color:#777}
  .bar{height:14px;background:#ddd;border-radius:7px;overflow:hidden;margin-top:10px}
  .fill{height:100%;background:#2196f3;width:0%;transition:width .5s}
</style></head><body>
<h1>🌱 Plant Monitor</h1>
<div class="card"><div class="lbl">Soil moisture</div>
  <div class="val" id="soil">--</div>
  <div class="bar"><div class="fill" id="fill"></div></div>
  <div class="lbl" id="status"></div></div>
<div class="card"><div class="lbl">Temperature</div><div class="val" id="temp">--</div></div>
<div class="card"><div class="lbl">Humidity</div><div class="val" id="hum">--</div></div>
<script>
async function update(){
  try{
    const r = await fetch('/data'); const d = await r.json();
    document.getElementById('soil').textContent = d.soil + '%';
    document.getElementById('fill').style.width = d.soil + '%';
    document.getElementById('temp').textContent = d.temp === null ? '--' : d.temp.toFixed(1) + ' °C';
    document.getElementById('hum').textContent = d.hum === null ? '--' : d.hum.toFixed(0) + ' %';
    document.getElementById('status').textContent =
      d.soil < 30 ? '🌵 Needs water!' : (d.soil > 75 ? '💧 Very wet' : '✅ Happy plant');
  }catch(e){}
}
update(); setInterval(update, 3000);
</script></body></html>
)rawliteral";

void handleRoot() {
     server.send_P(200, "text/html; charset=utf-8", PAGE);
}

void handleData() {
  String json = "{";
  json += "\"soil\":" + String(soilPercent) + ",";
  json += "\"raw\":" + String(soilRaw) + ",";
  json += "\"temp\":" + (isnan(temperature) ? String("null") : String(temperature, 1)) + ",";
  json += "\"hum\":" + (isnan(humidity) ? String("null") : String(humidity, 0));
  json += "}";
  server.send(200, "application/json", json);
}

// ---------- Sensors ----------
int readSoilRaw() {
  long sum = 0;
  for (int i = 0; i < 16; i++) {
    sum += analogRead(SOIL_PIN);
    delay(2);
  }
  return sum / 16;
}

void readSensors() {
  soilRaw = readSoilRaw();
  // Capacitive sensor: HIGH raw = dry, LOW raw = wet
  soilPercent = map(soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity = h;

  Serial.printf("Soil raw: %d (%d%%) | Temp: %.1f C | Hum: %.0f %%\n",
                soilRaw, soilPercent, temperature, humidity);
}

// ---------- Display ----------
void drawDisplay() {
  if (!oledOk) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Plant Monitor");

  display.setTextSize(2);
  display.setCursor(0, 14);
  display.printf("Soil:%d%%", soilPercent);

  display.setTextSize(1);
  display.setCursor(0, 36);
  if (isnan(temperature)) display.print("Temp: --");
  else display.printf("Temp: %.1f C", temperature);

  display.setCursor(0, 46);
  if (isnan(humidity)) display.print("Hum:  --");
  else display.printf("Hum:  %.0f %%", humidity);

  display.setCursor(0, 56);
  if (WiFi.status() == WL_CONNECTED) display.print(WiFi.localIP());
  else display.print("No WiFi");

  display.display();
}

// ---------- Setup / Loop ----------
void setup() {
  Serial.begin(115200);
  delay(500);

  analogReadResolution(12);  // 0-4095

  Wire.begin(SDA_PIN, SCL_PIN);
  oledOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (!oledOk) Serial.println("OLED not found - check wiring/address");

  dht.begin();

  if (oledOk) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Connecting WiFi...");
    display.display();
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Open this on your phone: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi failed - check name/password");
  }

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  readSensors();
  drawDisplay();
}

void loop() {
  server.handleClient();

  // DHT11 can only be read about once per second; every 2s is safe
  if (millis() - lastRead >= 2000) {
    lastRead = millis();
    readSensors();
    drawDisplay();
  }
}
