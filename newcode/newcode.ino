#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <LiquidCrystal_I2C.h>

//---------------- PINS ----------------
#define RST_PIN  D3
#define SS_PIN   D4
#define BUZZER   D8

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

//---------------- RFID ----------------
int blockNum = 2;
byte bufferLen = 18;
byte readBlockData[18];

//---------------- WIFI ----------------
#define WIFI_SSID "Realme Narzo "
#define WIFI_PASSWORD "12345678"

//---------------- GOOGLE SHEET URL (CLEANED) ----------------
const String base_url = "https://script.google.com/macros/s/AKfycbyilY4Z2DomNeMkeazGkDhwIyGEK6oov2HPgIOXwNxSIwsYQ4Tqx6wTx63jF3qlwFckPw/exec";

//---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x3F, 16, 2);

//---------------- ATTENDANCE DATA ----------------
int totalClasses = 10;
int gauravCount = 8;
int adityaCount = 9;
int vivekCount = 7;
int jiteshCount = 5;

//---------------- SETUP ----------------
void setup() {
  Serial.begin(9600);
  
  // LCD Setup
  lcd.init();
  lcd.begin(D2, D1);
  lcd.backlight();
  lcd.print("Connecting WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  lcd.clear();
  lcd.print("WiFi Ready!");

  pinMode(BUZZER, OUTPUT);
  SPI.begin();
  mfrc522.PCD_Init();

  // Set default key to FF FF FF FF FF FF
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

//---------------- LOOP ----------------
void loop() {
  lcd.setCursor(0, 0);
  lcd.print("Scan Your Card  ");

  // Look for card
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  if (ReadDataFromBlock(blockNum, readBlockData)) {
    // Convert buffer to String cleanly
    String name = "";
    for (int i = 0; i < 16; i++) {
      if (readBlockData[i] >= 32 && readBlockData[i] <= 126) { // Only printable characters
        name += (char)readBlockData[i];
      }
    }
    name.trim();

    lcd.clear();
    lcd.print("Hey " + name);

    // Calculate Logic
    int attendance = 0;
    if (name == "Gaurav") attendance = gauravCount;
    else if (name == "Aditya") attendance = adityaCount;
    else if (name == "Vivek") attendance = vivekCount;
    else if (name == "Jitesh") attendance = jiteshCount;

    float percent = (attendance * 100.0) / totalClasses;
    String statusStr = (percent < 75.0) ? "Defaulter" : "OK";

    lcd.setCursor(0, 1);
    lcd.print(statusStr + " " + String((int)percent) + "%");

    // Buzzer
    digitalWrite(BUZZER, HIGH);
    delay(statusStr == "Defaulter" ? 1000 : 200);
    digitalWrite(BUZZER, LOW);

    // Network request
    sendToGoogle(name, percent, statusStr);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(3000);
}

//---------------- SENDING DATA (FIXED TIMEOUT) ----------------
void sendToGoogle(String name, float percent, String statusStr) {
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure(); // Skip SSL cert verification

    HTTPClient https;
    String url = base_url + "?name=" + name + "&percent=" + String(percent) + "&status=" + statusStr;

    Serial.println("Attempting to send data...");
    
    // INCREASED TIMEOUT: 15 seconds allows for slow Google Script response
    https.setTimeout(15000); 

    if (https.begin(*client, url)) {
      // MANDATORY: Follow redirects (Google always redirects scripts)
      https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      
      int httpCode = https.GET();

      if (httpCode > 0) {
        Serial.printf("Response Code: %d\n", httpCode);
        lcd.clear();
        lcd.print("Data Recorded!");
      } else {
        Serial.printf("Error: %s\n", https.errorToString(httpCode).c_str());
        lcd.clear();
        lcd.print("Link Error");
      }
      https.end();
    }
  } else {
    Serial.println("WiFi Disconnected");
  }
}

//---------------- READ RFID ----------------
bool ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) return false;

  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) return false;
  
  return true;
}