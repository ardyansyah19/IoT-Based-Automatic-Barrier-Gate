/*
  PALANG PINTU KERETA API OTOMATIS 2 BIDANG - ARDUINO UNO
  ----------------------------------------------------------
  Fungsi:
  - Mendeteksi kereta via HC-SR04 (atau tombol manual untuk simulasi)
  - Sensor mewakili titik 50 meter sebelum perlintasan (JARAK_SENSOR_KE_PALANG)
  - Waktu peringatan sebelum palang menutup DIHITUNG OTOMATIS dari
    jarak (50m) dibagi kecepatan kereta -> waktu = jarak / kecepatan
  - Kecepatan kereta bisa diubah dari dashboard IoT (dikirim ESP8266 -> Arduino
    dengan format "SET_SPEED:<m/s>")
  - Mengontrol 2 traffic light (merah-kuning-hijau)
  - Menggerakkan 2 servo sebagai palang
  - Menampilkan hitung mundur waktu lintas kereta (5 menit) di TM1637
  - Membunyikan buzzer sebagai alarm
  - Mengirim status ke ESP8266 via SoftwareSerial untuk ditampilkan di web dashboard

  Library yang dibutuhkan (install via Library Manager):
  - Servo.h            (bawaan Arduino IDE)
  - TM1637Display       (by Avishorp)
  - SoftwareSerial.h    (bawaan Arduino IDE)

  CATATAN PENTING:
  HC-SR04 jangkauan fisiknya hanya ±4 meter, jadi "50 meter" di sini adalah
  PARAMETER SKALA/KONSEP untuk perhitungan waktu -- begitu sensor mendeteksi
  objek (mewakili kereta baru masuk radius 50m), sistem langsung menghitung
  mundur waktu tempuh berdasarkan kecepatan kereta, bukan mengukur jarak 50m
  secara literal.
*/

#include <Servo.h>
#include <TM1637Display.h>
#include <SoftwareSerial.h>

// ---------------- PIN MAPPING ----------------
#define TRIG_PIN      9
#define ECHO_PIN      8

#define TL1_RED       2
#define TL1_YELLOW    3
#define TL1_GREEN     4

#define TL2_RED       5
#define TL2_YELLOW    6
#define TL2_GREEN     7

#define SERVO1_PIN    10   // SG90  - palang bidang 1
#define SERVO2_PIN    11   // MG90S - palang bidang 2

#define TM1637_CLK    12
#define TM1637_DIO    13

#define BUZZER_PIN    A0
#define BUTTON_PIN    A1

#define ESP_RX        A2   // terhubung ke TX ESP8266
#define ESP_TX        A3   // terhubung ke RX ESP8266 (WAJIB via voltage divider 5V->3.3V)

// ---------------- OBJECTS ----------------
Servo servo1;
Servo servo2;
TM1637Display display(TM1637_CLK, TM1637_DIO);
SoftwareSerial espSerial(ESP_RX, ESP_TX);

// ---------------- KONSTANTA ----------------
const int JARAK_DETEKSI        = 15;        // cm, jarak deteksi objek/kereta (fisik sensor)
const unsigned long WAKTU_LINTAS = 300000UL; // 5 menit dalam milidetik
const int SERVO_BUKA  = 0;
const int SERVO_TUTUP = 90;
const unsigned long MIN_WARNING_MS = 2000UL; // batas bawah waktu peringatan (jaga2 kecepatan tinggi)

// Jarak konseptual sensor ke titik palang (meter) & kecepatan kereta (m/s)
// Kecepatan bisa diubah realtime dari dashboard IoT: "SET_SPEED:<m/s>"
const float JARAK_SENSOR_KE_PALANG = 50.0;
float kecepatanKereta = 20.0; // default 20 m/s (~72 km/jam), bisa diganti via IoT

// ---------------- STATE MACHINE ----------------
enum State { IDLE, WARNING, CLOSING, CROSSING, OPENING };
State state = IDLE;

unsigned long stateTimer      = 0;
unsigned long crossingStart   = 0;
unsigned long warningDuration = 3000; // dihitung ulang tiap deteksi kereta (jarak/kecepatan)
bool buzzerState              = false;
unsigned long lastBuzzerToggle= 0;
bool blinkState                = false;
unsigned long lastBlinkToggle  = 0;
unsigned long lastReportTime   = 0;

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(TL1_RED, OUTPUT); pinMode(TL1_YELLOW, OUTPUT); pinMode(TL1_GREEN, OUTPUT);
  pinMode(TL2_RED, OUTPUT); pinMode(TL2_YELLOW, OUTPUT); pinMode(TL2_GREEN, OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo1.write(SERVO_BUKA);
  servo2.write(SERVO_BUKA);

  display.setBrightness(5);
  display.clear();

  setLampuHijau();
  Serial.println(F("Sistem palang kereta siap."));
}

void loop() {
  switch (state) {
    case IDLE:      handleIdle();      break;
    case WARNING:   handleWarning();   break;
    case CLOSING:   handleClosing();   break;
    case CROSSING:  handleCrossing();  break;
    case OPENING:   handleOpening();   break;
  }
}

// ---------------- DETEKSI KERETA ----------------
bool keretaTerdeteksi() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long durasi = pulseIn(ECHO_PIN, HIGH, 30000);
  int jarak = durasi * 0.034 / 2;

  bool tombolDitekan = (digitalRead(BUTTON_PIN) == LOW);

  return (jarak > 0 && jarak < JARAK_DETEKSI) || tombolDitekan;
}

// ---------------- STATE HANDLERS ----------------
void handleIdle() {
  bacaPerintahESP(); // cek jika ada update kecepatan dari dashboard IoT

  if (keretaTerdeteksi()) {
    state = WARNING;
    stateTimer = millis();
    lastBlinkToggle = millis();

    // Hitung waktu tempuh: waktu = jarak / kecepatan (detik) -> ms
    unsigned long waktuTempuhMs = (unsigned long)((JARAK_SENSOR_KE_PALANG / kecepatanKereta) * 1000.0);
    if (waktuTempuhMs < MIN_WARNING_MS) waktuTempuhMs = MIN_WARNING_MS;
    warningDuration = waktuTempuhMs;

    Serial.print(F("Kereta terdeteksi 50m dari palang. Kecepatan: "));
    Serial.print(kecepatanKereta);
    Serial.print(F(" m/s -> palang menutup otomatis dalam "));
    Serial.print(warningDuration / 1000.0);
    Serial.println(F(" detik."));

    sendStatus("WARNING", warningDuration / 1000);
  }
}

void handleWarning() {
  // Lampu kuning berkedip selama warningDuration (dihitung dari jarak/kecepatan)
  if (millis() - lastBlinkToggle > 300) {
    blinkState = !blinkState;
    lastBlinkToggle = millis();
    digitalWrite(TL1_YELLOW, blinkState);
    digitalWrite(TL2_YELLOW, blinkState);
    digitalWrite(TL1_RED, LOW); digitalWrite(TL1_GREEN, LOW);
    digitalWrite(TL2_RED, LOW); digitalWrite(TL2_GREEN, LOW);
  }
  tone(BUZZER_PIN, 1000);

  if (millis() - stateTimer > warningDuration) {
    noTone(BUZZER_PIN);
    setLampuMerah();
    state = CLOSING;
  }
}

// Membaca perintah dari ESP8266, misal: "SET_SPEED:15.5"
void bacaPerintahESP() {
  if (espSerial.available()) {
    String line = espSerial.readStringUntil('\n');
    line.trim();
    if (line.startsWith("SET_SPEED:")) {
      float nilai = line.substring(10).toFloat();
      if (nilai > 0) {
        kecepatanKereta = nilai;
        Serial.print(F("Kecepatan kereta diubah menjadi: "));
        Serial.print(kecepatanKereta);
        Serial.println(F(" m/s"));
      }
    }
  }
}

void handleClosing() {
  for (int pos = SERVO_BUKA; pos <= SERVO_TUTUP; pos += 2) {
    servo1.write(pos);
    servo2.write(pos);
    delay(15);
  }
  state = CROSSING;
  crossingStart = millis();
  lastReportTime = millis();
  sendStatus("CROSSING", WAKTU_LINTAS / 1000);
}

void handleCrossing() {
  unsigned long elapsed = millis() - crossingStart;
  long sisaDetik = (WAKTU_LINTAS - elapsed) / 1000;
  if (sisaDetik < 0) sisaDetik = 0;

  tampilkanCountdown(sisaDetik);

  // Buzzer beep intermiten
  if (millis() - lastBuzzerToggle > 500) {
    lastBuzzerToggle = millis();
    buzzerState = !buzzerState;
    if (buzzerState) tone(BUZZER_PIN, 800); else noTone(BUZZER_PIN);
  }

  // Kirim update status ke ESP8266 tiap 5 detik
  if (millis() - lastReportTime > 5000) {
    lastReportTime = millis();
    sendStatus("CROSSING", sisaDetik);
  }

  if (elapsed >= WAKTU_LINTAS) {
    noTone(BUZZER_PIN);
    state = OPENING;
  }
}

void handleOpening() {
  for (int pos = SERVO_TUTUP; pos >= SERVO_BUKA; pos -= 2) {
    servo1.write(pos);
    servo2.write(pos);
    delay(15);
  }
  setLampuHijau();
  display.clear();
  sendStatus("IDLE", 0);
  Serial.println(F("Kereta telah lewat. Palang dibuka."));
  state = IDLE;
}

// ---------------- HELPER ----------------
void setLampuMerah() {
  digitalWrite(TL1_RED, HIGH); digitalWrite(TL1_YELLOW, LOW); digitalWrite(TL1_GREEN, LOW);
  digitalWrite(TL2_RED, HIGH); digitalWrite(TL2_YELLOW, LOW); digitalWrite(TL2_GREEN, LOW);
}

void setLampuHijau() {
  digitalWrite(TL1_RED, LOW); digitalWrite(TL1_YELLOW, LOW); digitalWrite(TL1_GREEN, HIGH);
  digitalWrite(TL2_RED, LOW); digitalWrite(TL2_YELLOW, LOW); digitalWrite(TL2_GREEN, HIGH);
}

void tampilkanCountdown(long detik) {
  int menit = detik / 60;
  int sisaDetik = detik % 60;
  int nilai = menit * 100 + sisaDetik;
  display.showNumberDecEx(nilai, 0b01000000, true); // format MM:SS dengan titik dua di tengah
}

void sendStatus(String status, long sisaDetik) {
  espSerial.print("STATUS:");
  espSerial.print(status);
  espSerial.print(":");
  espSerial.println(sisaDetik);
}
