#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200); // GMT+7 for Jakarta, Indonesia

#define WIFI_SSID "bangg?"
#define WIFI_PASSWORD "hwvv1212"
#define API_KEY "AIzaSyBnPADOix0EWKu29eysInzDlbA5Rvv9Lz8"
#define DATABASE_URL "https://bismillah-ambil-data-beneran-default-rtdb.asia-southeast1.firebasedatabase.app"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;
int MQ4 = A0;
unsigned long sendDataPrevMillis = 0;
int k;
String msg;
String firestatus = "";

// Declare dt array
String dt[2];

// deklarasi fungsi
int scanSpecificWiFiNetwork();
bool shouldSendData();
void sendData(int MQ4, int k);
void waitForWiFi();

void setup() {
  Serial.begin(115200);
  pinMode(MQ4, INPUT);
  
  // Set WiFi to station mode
  WiFi.mode(WIFI_STA);

  // Disconnect from an AP if it was previously connected
  WiFi.disconnect();
  delay(100);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase signup successful");
    signupOK = true;
  } else {
    Serial.printf("Firebase signup failed: %s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize NTP client
  timeClient.begin();
  timeClient.setTimeOffset(25200); // GMT+7 for Jakarta, Indonesia
}

void loop() {
  // Get current time
  timeClient.update();
  String currentTime = timeClient.getFormattedTime();
  
  // Baca data yang dikirim dari Arduino
  while (Serial.available() > 0) {
    msg = Serial.readString();

    int j = 0;
    // inisialisasi variabel, (reset isi variabel)
    dt[0] = "";
    dt[1] = currentTime;
    // proses parsing data
    for (int i = 1; i < msg.length(); i++) {
      // pengecekan tiap karakter dengan karakter (#) dan (,)
      if (msg[i] == ',') {
        // increment variabel j, digunakan untuk merubah index array penampung
        j++;
        dt[j] = "";  // inisialisasi variabel array dt[j]
      } else {
        // proses tampung data saat pengecekan karakter selesai.
        dt[j] = dt[j] + msg[i];
      }
    }

    // Extract MQ4 value
    int mq4Value = dt[0].toInt();
    //Serial.println("MQ4: " + String(mq4Value));

    // Update Firebase with MQ4 value & timestamp
    if (Firebase.ready() && signupOK) {
      Firebase.RTDB.setFloat(&fbdo, "MQ4 Sensor/Value", mq4Value);
      Firebase.RTDB.setString(&fbdo, "Receiver/Time", dt[1]);
    }

    // Print data to Serial Monitor
    Serial.println("Data masuk: " + msg);
    Serial.println("MQ4: " + String(mq4Value));
    Serial.println("Receiver Time: " + dt[1]);    
  }      

  int32_t rssi;
  int scanResult;

  Serial.println(F("Scanning WiFi..."));

  scanResult = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);

  if (scanResult > 0) {
    // Find the specified WiFi network in the scan results
    for (int8_t i = 0; i < scanResult; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.equals(WIFI_SSID)) {
        rssi = WiFi.RSSI(i); // Get RSSI directly

        Serial.printf(PSTR("WiFi network found: %s\n"), ssid.c_str());
        
        // Display RSSI
        Serial.printf(PSTR("RSSI: %d dBm\n"), rssi);

        // Send RSSI to Firebase
        if (Firebase.ready() && signupOK) {
          Firebase.RTDB.setFloat(&fbdo, "WiFi/RSSI", rssi);
        }

        break; // Stop scanning once the specified network is found
      }
    }
  } else {
    Serial.println(F("No WiFi networks found"));
  }

  // Wait a bit before scanning again
  delay(5000);
}