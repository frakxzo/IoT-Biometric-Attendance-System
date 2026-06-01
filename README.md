# ESP32 Smart Biometric Attendance System

![ESP32](https://img.shields.io/badge/Hardware-ESP32-blue.svg)
![C++](https://img.shields.io/badge/Language-C++-00599C.svg)
![Google Sheets](https://img.shields.io/badge/Database-Google%20Sheets-109D59.svg)
![Arduino IDE](https://img.shields.io/badge/IDE-Arduino-00979D.svg)

An IoT based biometric attendance tracking system that bridges embedded hardware with the cloud. Utilizing an **ESP32**, an optical fingerprint sensor, and an I2C LCD, this system authenticates users locally and logs their precise "Entry" and "Exit" timestamps directly to a Google Sheet over Wi-Fi.

---

## ✨ Features

* **Direct Cloud Logging:** Transmits attendance data directly to a Google Apps Script Webhook, automatically appending rows to a Google Sheet with local Indian Standard Time (IST) timestamps.
* **Smart Entry/Exit Tracking:** The cloud script intelligently reads the user's last logged status and automatically toggles between "Entry" and "Exit" for seamless tracking.
* **Non-Volatile Local Storage:** Uses the ESP32 `Preferences.h` library to store user details (Name, Roll, Branch) directly on the microcontroller's flash memory.
* **Serial Admin Console:** Manage the system without flashing new code. Send simple commands via the Serial Monitor to `[R]`egister, `[D]`elete, or `[L]`ist enrolled students.
* **Audio-Visual Feedback:** Features an I2C LCD for real-time status updates and a buzzer for distinct success/error auditory cues.

---

## 🛠️ Hardware Setup & Pin Mapping

* **Microcontroller:** ESP32 Development Board
* **Biometric Sensor:** AS608 (or compatible) Optical Fingerprint Sensor
* **Display:** 16x2 LCD with I2C module (Address `0x27`)
* **Audio:** Active Buzzer

### Wiring Guide
| Component Pin | ESP32 Pin | Connection Note |
| :--- | :--- | :--- |
| **Fingerprint TX** | `GPIO 16` | Hardware Serial2 (RX2) |
| **Fingerprint RX** | `GPIO 17` | Hardware Serial2 (TX2) |
| **I2C LCD SDA** | `GPIO 21` | Default I2C Data |
| **I2C LCD SCL** | `GPIO 22` | Default I2C Clock |
| **Buzzer (+)** | `GPIO 15` | Digital Output |

---

## 🔌 Circuit Diagram

Below is the schematic mapping the physical connections between the ESP32, the optical biometric scanner, the I2C LCD screen, and the audio buzzer module:

<p align="center">
  <img src="assets/ckt diagram.png" alt="ESP32 Biometric System Circuit Diagram" width="650">
</p>

---

## 🚀 Software & Library Requirements

Ensure you have the following libraries installed via the Arduino Library Manager:
* `Adafruit Fingerprint Sensor Library`
* `LiquidCrystal_I2C`
* *(Built-in ESP32 Core Libraries: `WiFi.h`, `HTTPClient.h`, `Preferences.h`)*

---

## ⚙️ Configuration & Deployment

### 1. Google Sheets Webhook Setup
1. Create a blank Google Sheet.
2. Navigate to **Extensions > Apps Script**.
3. Clear any default code and paste the script from `scripts/WebHook.js`.
4. Click **Deploy > New Deployment**.
5. Select **Web App** as the deployment type, set executing access to **"Me"**, and set permissions to **"Anyone"**.
6. Click **Deploy** and copy the generated **Web App URL**.

### 2. Firmware Compilation & Upload
1. Open `firmware/Fingerprint_Code/Fingerprint_Code.ino` using the Arduino IDE.
2. Locate the network configurations and update them with your environment settings:

   <ins>cpp/.ino file:</ins>

   const char* ssid     = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";

3. Paste your copied deployment URL into the ***GOOGLE_SCRIPT_URL*** variable:
   
    <ins>cpp/.ino file:</ins>
   
   const char* GOOGLE_SCRIPT_URL = "YOUR_GOOGLE_APPS_SCRIPT_WEB_APP_URL";

4. Connect your ESP32 board, select the correct COM port, and upload the sketch.

## 🎮 How to Operate the System

* **Marking Attendance**
Simply press a registered finger against the optical scanner. The ESP32 verifies the biometric signature locally, queries its flash memory for the matching metadata, and pushes a secure request to the cloud Webhook. The connected LCD display will instantly update to indicate a successful entry (IN) or departure (OUT).

* **Admin Console Commands**
Open your Arduino IDE Serial Monitor and set the communication baud rate to 115200. You can pass single characters into the input bar to manage user records:

--> Send **R** to Register a Student: Prompts you to select an open registration ID slot (1-127), scan the finger twice to construct a template, and type in their custom metadata profile (Name, Branch, and Roll Number).

--> Send **D** to Delete a Student: Prompts you for a specific ID number to purge their fingerprint template from hardware memory and clear their corresponding data from local flash storage.

--> Send **L** to List Records: Iterates through the active flash registers and prints an organized table of all currently enrolled student names and allocations directly to the console.

   
   
