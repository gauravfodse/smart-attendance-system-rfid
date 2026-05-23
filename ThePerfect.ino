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

//---------------- GOOGLE SHEET URL ----------------
const String base_url = "https://script.google.com/macros/s/AKfycbyilY4Z2DomNeMkeazGkDhwIyGEK6oov2HPgIOXwNxSIwsYQ4Tqx6wTx63jF3qlwFckPw/exec";

//---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x3F, 16, 2);

//---------------- 1% INCREMENT SETTINGS ----------------
int totalClasses = 100; // Each scan is now 1/100th (1%)
int gauravCount = 74;   // Starts at 74% (One scan makes him 'OK')
int adityaCount = 85;   // Starts at 85%
int vivekCount = 60;    // Starts at 60%
int jiteshCount = 45;   // Starts at 45%

void setup() {
  Serial.begin(9600);
  
  lcd.init();
  lcd.begin(D2, D1);
  lcd.backlight();
  lcd.print("WiFi Connecting");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  lcd.clear();
  lcd.print("System Ready");

  pinMode(BUZZER, OUTPUT);
  SPI.begin();
  mfrc522.PCD_Init();
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

void loop() {
  lcd.setCursor(0, 0);
  lcd.print("Scan Card...     ");

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  if (ReadDataFromBlock(blockNum, readBlockData)) {
    String name = "";
    for (int i = 0; i < 16; i++) {
      if (readBlockData[i] >= 32 && readBlockData[i] <= 126) name += (char)readBlockData[i];
    }
    name.trim();

    int currentCount = 0;
    
    // Logic: Increase count by 1 (which is 1% of 100)
    if (name == "Gaurav") { gauravCount++; currentCount = gauravCount; }
    else if (name == "Aditya") { adityaCount++; currentCount = adityaCount; }
    else if (name == "Vivek") { vivekCount++; currentCount = vivekCount; }
    else if (name == "Jitesh") { jiteshCount++; currentCount = jiteshCount; }
    else { return; }

    // Math: (currentCount / 100) * 100 = currentCount
    float percent = (currentCount * 100.0) / totalClasses;
    String statusStr = (percent < 75.0) ? "Defaulter" : "OK";

    lcd.clear();
    lcd.print(name + ": " + String((int)percent) + "%");
    lcd.setCursor(0, 1);
    lcd.print("Status: " + statusStr);

    // Buzzer Feedback
    digitalWrite(BUZZER, HIGH);
    delay(200);
    digitalWrite(BUZZER, LOW);

    sendToGoogle(name, percent, statusStr, currentCount);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(3000); 
}

void sendToGoogle(String name, float percent, String statusStr, int count) {
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
    
    String url = base_url + "?name=" + name + "&percent=" + String(percent) + "&status=" + statusStr + "&count=" + String(count);

    https.setTimeout(15000);
    if (https.begin(*client, url)) {
      https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      https.GET();
      https.end();
    }
  }
}

bool ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) return false;
  byte len = 18; 
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &len);
  return (status == MFRC522::STATUS_OK);
}