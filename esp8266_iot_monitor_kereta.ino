/*
  MONITOR IoT PALANG KERETA API - ESP8266 NodeMCU (ESP-12E)
  ------------------------------------------------------------
  Fungsi:
  - Menerima data status dari Arduino Uno via SoftwareSerial
    (format teks: "STATUS:<state>:<sisa_detik>")
  - Menyediakan web dashboard sederhana (auto-refresh tiap 3 detik)
  - Menyediakan endpoint JSON /status untuk integrasi aplikasi lain

  Library yang dibutuhkan (bawaan Board Manager ESP8266):
  - ESP8266WiFi.h
  - ESP8266WebServer.h
  - SoftwareSerial.h

  CATATAN WIRING:
  Pin TX Arduino Uno (5V) -> WAJIB melalui voltage divider -> pin RX ESP8266 (3.3V)
  Pin RX Arduino Uno (A2) <- pin TX ESP8266 (D1/GPIO5, aman langsung ke Arduino)
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>

// ---------------- KONFIGURASI WIFI ----------------
const char* ssid     = "NAMA_WIFI_ANDA";
const char* password = "PASSWORD_WIFI_ANDA";

// ---------------- SERIAL KE ARDUINO ----------------
// Sesuaikan pin dengan wiring Anda. D2 = GPIO4 (RX), D1 = GPIO5 (TX)
SoftwareSerial arduinoSerial(D2, D1);

ESP8266WebServer server(80);

String statusKereta = "IDLE";
long sisaDetik = 0;
unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(9600);
  arduinoSerial.begin(9600);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Terhubung! Alamat IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/set_speed", handleSetSpeed);
  server.begin();
  Serial.println("Web server aktif.");
}

void loop() {
  server.handleClient();
  bacaDataArduino();
}

void bacaDataArduino() {
  if (arduinoSerial.available()) {
    String line = arduinoSerial.readStringUntil('\n');
    line.trim();
    // format: STATUS:CROSSING:120
    if (line.startsWith("STATUS:")) {
      int idx1 = line.indexOf(':');
      int idx2 = line.indexOf(':', idx1 + 1);
      if (idx2 > idx1) {
        statusKereta = line.substring(idx1 + 1, idx2);
        sisaDetik = line.substring(idx2 + 1).toInt();
        lastUpdate = millis();
        Serial.println("Update diterima: " + statusKereta + " sisa " + String(sisaDetik) + "s");
      }
    }
  }
}

void handleSetSpeed() {
  if (server.hasArg("v")) {
    float speed = server.arg("v").toFloat();
    if (speed > 0) {
      arduinoSerial.print("SET_SPEED:");
      arduinoSerial.println(speed);
      Serial.println("Kecepatan kereta diset ke: " + String(speed) + " m/s");
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStatus() {
  String json = "{";
  json += "\"status\":\"" + statusKereta + "\",";
  json += "\"sisa_detik\":" + String(sisaDetik) + ",";
  json += "\"last_update_ms_ago\":" + String(millis() - lastUpdate);
  json += "}";
  server.send(200, "application/json", json);
}

void handleRoot() {
  String warna = "green";
  if (statusKereta == "WARNING") warna = "orange";
  if (statusKereta == "CROSSING") warna = "red";

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='3'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Monitor Palang Kereta</title>";
  html += "<style>body{font-family:sans-serif;text-align:center;padding-top:40px;background:#f4f4f4;}";
  html += "h1{color:#333;font-size:1.4em;}";
  html += ".status{font-size:2.2em;font-weight:bold;color:" + warna + ";margin:20px;}";
  html += ".sisa{font-size:1.5em;color:#555;}</style></head><body>";
  html += "<h1>Monitor Perlintasan Kereta Api - 2 Bidang</h1>";
  html += "<div class='status'>" + statusKereta + "</div>";
  if (statusKereta == "CROSSING") {
    int menit = sisaDetik / 60;
    int detik = sisaDetik % 60;
    html += "<div class='sisa'>Sisa waktu lintas: " + String(menit) + " menit " + String(detik) + " detik</div>";
  }
  html += "<hr><h3>Atur kecepatan kereta (simulasi jarak sensor 50m)</h3>";
  html += "<form action='/set_speed' method='GET'>";
  html += "<input type='number' step='0.1' name='v' placeholder='m/s, contoh 20'>";
  html += "<button type='submit'>Terapkan</button></form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}
