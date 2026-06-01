#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ── WiFi & Google Sheets ────────────────────────────────────────────────────
const char* ssid     = "?";           // ← Replace
const char* password = "bruh1234";       // ← Replace

// Paste your deployed Google Apps Script Web app URL here
const char* GOOGLE_SCRIPT_URL = "https://script.google.com/macros/s/script_thing";

// ── HARDWARE ────────────────────────────────────────────────────────────────
const int BUZZER_PIN = 15;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);
Preferences prefs;

#define FINGER_TIMEOUT_MS 12000  // Max wait per scan attempt

// ── SETUP ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== FINGERPRINT ATTENDANCE SYSTEM - GOOGLE SHEETS ===\n");

  Serial2.begin(57600, SERIAL_8N1, 16, 17);  // RX=16, TX=17 (adjust if needed)

  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Starting...");
  delay(800);

  if (finger.verifyPassword()) {
    lcd.clear();
    lcd.print("Sensor OK");
    delay(1000);
    Serial.println("Fingerprint sensor OK");
    Serial.println("Commands: r = register, d = delete, l = list");
  } else {
    lcd.clear();
    lcd.print("Sensor FAIL!");
    Serial.println("Fingerprint sensor not found!");
    while (true) delay(1000);
  }

  prefs.begin("students", false);
  delay(400);

  connectToWiFi();

  showIdle();
}

// ── LOOP ────────────────────────────────────────────────────────────────────
void loop() {
  // Reconnect WiFi if needed
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 30000) {
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) connectToWiFi();
  }

  // Serial commands
  if (Serial.available()) {
    char cmd = toupper(Serial.read());
    while (Serial.available()) Serial.read(); // flush buffer

    if (cmd == 'R') registerFinger();
    if (cmd == 'D') deleteRecord();
    if (cmd == 'L') listRecords();
  }

  // Scan fingerprint
  int id = getFingerprintID();
  if (id == -2) {
    Serial.println("→ Unknown finger");
    lcd.clear();
    lcd.print("Access Denied");
    errorBeep();
    delay(1500);
    showIdle();
  } else if (id > 0) {
    displayAttendance(id);
    correctBeep();
    delay(2500);
    showIdle();
  }

  delay(50);
}

// ── WiFi ────────────────────────────────────────────────────────────────────
void connectToWiFi() {
  Serial.print("Connecting WiFi...");
  lcd.clear(); lcd.print("WiFi Connecting");
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(400);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected → IP: " + WiFi.localIP().toString());
    lcd.clear(); lcd.print("WiFi Connected");
    delay(1000);
  } else {
    Serial.println("\nWiFi failed");
    lcd.clear(); lcd.print("WiFi Failed");
    delay(2000);
  }
}

// ── Buzzer tones ────────────────────────────────────────────────────────────
void stepBeep()   { digitalWrite(BUZZER_PIN, HIGH); delay(80);  digitalWrite(BUZZER_PIN, LOW); }
void errorBeep()  { for(int i=0; i<2; i++) { digitalWrite(BUZZER_PIN,HIGH); delay(140); digitalWrite(BUZZER_PIN,LOW); delay(80); } }
void correctBeep(){ digitalWrite(BUZZER_PIN, HIGH); delay(1600); digitalWrite(BUZZER_PIN, LOW); }

// ── Fingerprint helpers ─────────────────────────────────────────────────────
int waitForImageAndConvert(int bufferID) {
  uint8_t p = 255;
  unsigned long tstart = millis();

  lcd.clear(); lcd.print("Place Finger");
  lcd.setCursor(0,1); lcd.print("Scan "); lcd.print(bufferID);
  Serial.printf("→ Scan %d: Place finger...\n", bufferID);

  while (true) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) break;
    if (p == FINGERPRINT_NOFINGER) {} // continue waiting
    else {
      Serial.printf("getImage error: 0x%02X\n", p);
      return p;
    }
    if (millis() - tstart > FINGER_TIMEOUT_MS) {
      Serial.println("Timeout");
      lcd.clear(); lcd.print("Timeout");
      errorBeep();
      return -1;
    }
    delay(40);
  }

  stepBeep();
  Serial.println("Image captured → converting...");
  p = finger.image2Tz(bufferID);

  if (p != FINGERPRINT_OK) {
    Serial.printf("image2Tz failed: 0x%02X\n", p);
    lcd.clear(); lcd.print("Bad Image");
    errorBeep();
    return p;
  }

  Serial.println("→ Remove finger now");
  lcd.clear(); lcd.print("Remove Finger");
  tstart = millis();

  while (true) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) break;
    if (millis() - tstart > 8000) {
      Serial.println("Removal timeout");
      errorBeep();
      return -1;
    }
    delay(40);
  }

  delay(400);
  return FINGERPRINT_OK;
}

// ── Register new fingerprint ────────────────────────────────────────────────
void registerFinger() {
  int id = 0;
  String name, branch, roll;

  lcd.clear(); lcd.print("REGISTER MODE");
  delay(1000);
  Serial.println("\n=== REGISTER NEW FINGERPRINT ===");

  Serial.print("Enter ID (1-127): ");
  while (!Serial.available()) delay(10);
  id = Serial.parseInt();
  while (Serial.available()) Serial.read();

  if (id < 1 || id > 127) {
    Serial.println("Invalid ID");
    lcd.clear(); lcd.print("Invalid ID");
    errorBeep();
    delay(1500);
    showIdle();
    return;
  }

  Serial.printf("Enrolling ID #%d (2 scans)\n", id);

  if (waitForImageAndConvert(1) != FINGERPRINT_OK) return;
  if (waitForImageAndConvert(2) != FINGERPRINT_OK) return;

  lcd.clear(); lcd.print("Processing...");
  Serial.print("Creating model... ");
  uint8_t p = finger.createModel();

  if (p != FINGERPRINT_OK) {
    Serial.printf("createModel failed: 0x%02X\n", p);
    lcd.clear(); lcd.print("Mismatch/Error");
    errorBeep();
    delay(2000);
    showIdle();
    return;
  }
  Serial.println("OK");

  Serial.printf("Storing model #%d... ", id);
  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) {
    Serial.printf("storeModel failed: 0x%02X\n", p);
    lcd.clear(); lcd.print("Store Error");
    errorBeep();
    delay(2000);
    showIdle();
    return;
  }
  Serial.println("OK");
  stepBeep();

  lcd.clear(); lcd.print("ID:"); lcd.print(id);
  lcd.setCursor(0,1); lcd.print("Enter details");

  Serial.print("Name   : "); while(!Serial.available()) delay(10);
  name = Serial.readStringUntil('\n'); name.trim();

  Serial.print("Branch : "); while(!Serial.available()) delay(10);
  branch = Serial.readStringUntil('\n'); branch.trim();

  Serial.print("Roll   : "); while(!Serial.available()) delay(10);
  roll = Serial.readStringUntil('\n'); roll.trim();

  String key = "s" + String(id);
  String data = name + "|" + branch + "|" + roll;
  prefs.putString(key.c_str(), data);

  Serial.println("Saved: " + data);
  lcd.clear(); lcd.print("Registered OK");
  lcd.setCursor(0,1); lcd.print(name.substring(0,10));
  correctBeep();
  delay(2200);
  showIdle();
}

// ── Attendance + Google Sheets ──────────────────────────────────────────────
void displayAttendance(int id) {
  String key = "s" + String(id);
  String data = prefs.getString(key.c_str(), "Unknown|N/A|N/A");

  int p1 = data.indexOf('|');
  int p2 = data.indexOf('|', p1 + 1);
  String name   = data.substring(0, p1);
  String branch = data.substring(p1 + 1, p2);
  String roll   = data.substring(p2 + 1);

  Serial.printf("Attendance → ID %d | %s | %s | %s\n", id, name.c_str(), roll.c_str(), branch.c_str());

  lcd.clear();
  lcd.print(name.substring(0,16));
  lcd.setCursor(0,1);
  lcd.print(roll + " " + branch.substring(0,8));

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(GOOGLE_SCRIPT_URL) + "?";
    url += "id="     + String(id);
    url += "&name="  + urlEncode(name);
    url += "&roll="  + urlEncode(roll);
    url += "&branch="+ urlEncode(branch);

    Serial.println("→ Sending to Sheets: " + url);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Response: " + payload);

      if (payload.indexOf("Entry") >= 0) {
        lcd.setCursor(13, 1); lcd.print("IN ");
      } else if (payload.indexOf("Exit") >= 0) {
        lcd.setCursor(13, 1); lcd.print("OUT");
      } else {
        lcd.setCursor(13, 1); lcd.print("??");
      }
    } else {
      Serial.printf("HTTP failed, code: %d\n", httpCode);
      lcd.setCursor(13, 1); lcd.print("ERR");
    }
    http.end();
  } else {
    Serial.println("No WiFi - skipping upload");
  }
}

// ── Core fingerprint search ─────────────────────────────────────────────────
int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK && finger.confidence > 45) {
    return finger.fingerID;
  }
  return -2;
}

// ── Utility functions ───────────────────────────────────────────────────────
void listRecords() {
  Serial.println("\n=== Stored Students ===");
  bool found = false;
  for (int i = 1; i <= 127; i++) {
    String key = "s" + String(i);
    if (prefs.isKey(key.c_str())) {
      found = true;
      Serial.printf("ID %3d → %s\n", i, prefs.getString(key.c_str()).c_str());
    }
  }
  if (!found) Serial.println("No records.");
}

void deleteRecord() {
  Serial.print("Delete ID: ");
  while (!Serial.available()) delay(10);
  int id = Serial.parseInt();
  while (Serial.available()) Serial.read();

  if (id < 1 || id > 127) {
    Serial.println("Invalid ID");
    return;
  }

  uint8_t p = finger.deleteModel(id);
  if (p == FINGERPRINT_OK) {
    prefs.remove(("s" + String(id)).c_str());
    Serial.printf("ID %d deleted.\n", id);
  } else {
    Serial.printf("Delete failed: 0x%02X\n", p);
  }
}

void showIdle() {
  lcd.clear();
  lcd.print("Place Finger");
  lcd.setCursor(0,1);
  lcd.print("to Mark");
}

String urlEncode(String str) {
  String encoded = "";
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      char buf[4];
      sprintf(buf, "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}