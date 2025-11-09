/*
  ESP32 MQ-4 SoftAP Portal (Open network, NO LOGIN)
  - SoftAP name: "ESP_MQ4_PORTAL" (open)
  - Shows MQ-4 live readings directly (ADC, voltage, %, approx PPM)
  - Captive portal redirect using DNSServer
  - Sensor analog pin: GPIO 34
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

const char* apSSID = "ESP_MQ4_PORTAL";  // Open network, no password
const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);

const int mq4Pin = 34; // MQ-4 analog output pin

// ---------- Approximate PPM calculation (placeholder) ----------
float approxPPM(int raw) {
  // Map raw 0–4095 → ppm (approx, uncalibrated)
  float percent = raw * 100.0 / 4095.0;
  float ppm = percent * 100; // scale placeholder
  return ppm;
}

// ---------- Routes ----------
void handleRoot() {
  server.send(200, "text/html", dashboardHtml());
}

void handleMQ4() {
  int raw = analogRead(mq4Pin);
  float voltage = raw * 3.3 / 4095.0;
  float percent = raw * 100.0 / 4095.0;
  float ppm = approxPPM(raw);

  String json = "{";
  json += "\"adc\":" + String(raw) + ",";
  json += "\"voltage\":" + String(voltage, 3) + ",";
  json += "\"percent\":" + String(percent, 2) + ",";
  json += "\"ppm\":" + String(ppm, 1);
  json += "}";
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(500);

  analogReadResolution(12); // 0..4095
  analogSetPinAttenuation(mq4Pin, ADC_11db); // up to ~3.3V

  // Start SoftAP (open)
  WiFi.mode(WIFI_AP);
  bool apok = WiFi.softAP(apSSID);
  IPAddress apIP = WiFi.softAPIP();

  Serial.println();
  Serial.print("Started AP: ");
  Serial.println(apSSID);
  Serial.print("AP IP: ");
  Serial.println(apIP);

  // DNS redirect for captive portal
  dnsServer.start(DNS_PORT, "*", apIP);

  // Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/mq4", HTTP_GET, handleMQ4);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}

/* ----------- HTML Dashboard ------------- */

String dashboardHtml() {
  String s = "<!doctype html><html><head><meta charset='utf-8'>";
  s += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  s += "<title>MQ-4 Dashboard</title>";
  s += "<style>body{font-family:Arial;background:#eef3f9;color:#111;margin:0;padding:12px} ";
  s += ".card{background:#fff;padding:16px;border-radius:10px;box-shadow:0 2px 8px rgba(0,0,0,0.1);max-width:720px;margin:auto} ";
  s += "h1{margin-top:0;text-align:center;color:#007bff} .row{display:flex;gap:12px;flex-wrap:wrap;justify-content:center} ";
  s += ".box{flex:1;min-width:160px;background:#f9fbff;padding:12px;border-radius:8px;text-align:center;border:1px solid #ddd} ";
  s += "h3{margin:4px 0;color:#333} .value{font-size:22px;font-weight:bold;color:#222}";
  s += "</style>";
  s += "<script>";
  s += "function fetchData(){fetch('/mq4').then(r=>r.json()).then(j=>{";
  s += "document.getElementById('adc').innerText=j.adc;";
  s += "document.getElementById('voltage').innerText=j.voltage+' V';";
  s += "document.getElementById('percent').innerText=j.percent+' %';";
  s += "document.getElementById('ppm').innerText=j.ppm+' ppm';";
  s += "document.getElementById('time').innerText=new Date().toLocaleTimeString();";
  s += "}).catch(e=>console.log(e));}";
  s += "setInterval(fetchData,1000);window.onload=fetchData;";
  s += "</script></head><body>";
  s += "<div class='card'><h1>MQ-4 Live Dashboard</h1>";
  s += "<div class='row'>";
  s += "<div class='box'><h3>ADC</h3><div id='adc' class='value'>--</div></div>";
  s += "<div class='box'><h3>Voltage</h3><div id='voltage' class='value'>--</div></div>";
  s += "<div class='box'><h3>Percent</h3><div id='percent' class='value'>--</div></div>";
  s += "<div class='box'><h3>Approx PPM</h3><div id='ppm' class='value' style='color:#d33'>--</div></div>";
  s += "</div><hr><p style='text-align:center;font-size:0.9em;color:#555'>Last update: <span id='time'>--</span></p>";
  s += "<p style='text-align:center;font-size:0.85em;color:#666'>Note: PPM is approximate — use calibration for accurate values.</p>";
  s += "</div></body></html>";
  return s;
}
