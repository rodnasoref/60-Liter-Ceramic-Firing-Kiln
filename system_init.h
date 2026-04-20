#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include <LittleFS.h>
#include <ThreeWire.h>  
#include <Preferences.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <esp_task_wdt.h>

#include "config.h"

Adafruit_NeoPixel rgbLed(LED_COUNT, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
Config config;                                    // Rendszerkonfigurációs struktúra
Preferences prefs;                                // Nem-felejtő memória (NVS) kezelése az ESP32-n

//AsyncWebServer server(80);                        // Aszinkron webszerver a távoli eléréshez

extern LGFX_ESP32S3_ST7789 tft;
extern LGFX_Sprite header;
extern LGFX_Sprite content;
extern LGFX_Sprite footer;
extern LGFX_Sprite TCcj;
extern LGFX_Sprite aktido;


// --- Kijelző inicializálása ---
void initDisplay() {
  tft.init();
  tft.setRotation(TFT_ROTATION);
  header.createSprite(210, 40);
  TCcj.createSprite(110, 40);
  content.createSprite(320, 160);
  footer.createSprite(230, 40);
  aktido.createSprite(90, 40);
}

void initPins() {
  digitalWrite(relayPin, LOW);
  pinMode(relayPin, INPUT_PULLDOWN);
//  pinMode(TC_drdyPin, INPUT_PULLUP);
  pinMode(bootButtonPin, INPUT_PULLUP);
  pinMode(TC_Fault, INPUT_PULLUP);
}


// --- Fájlrendszer inicializálása ---
bool initFileSystem() {
// 1. LittleFS csatolása (Mount). A 'true' paraméter engedélyezi az automatikus formázást hiba esetén.
  if (!LittleFS.begin(true)) {
    Serial.println("Hiba: LittleFS nem csatolható!");
    return false;
  }

  // 2. Ellenőrizzük, hogy létezik-e már a fájl
  if (!LittleFS.exists(logFile)) {
    Serial.println("A log fájl nem létezik, létrehozás...");

    // 3. Fájl létrehozása írásra ("w")
    File file = LittleFS.open(logFile, FILE_WRITE);
    if (!file) {
      Serial.println("Hiba: Nem sikerült létrehozni a fájlt!");
      return false;
    }

    // 4. Fejléc írása
    const char* header = "Datum_Ido;Belso_T;Hoelem_T;PWM_Szazalek;Statusz;Kp;Ki;Kd";
    if (file.println(header)) {
      Serial.println("Log fájl és fejléc sikeresen létrehozva.");
    } else {
      Serial.println("Hiba: Nem sikerült a fejlécet beleírni!");
      file.close();
      return false;
    }
    file.close();
  } else {
    Serial.println("A log fájl már létezik, nincs szükség inicializálásra.");
  }
  return true;
}


void initRTC() {
Rtc.Begin();
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);    
    if (!Rtc.IsDateTimeValid()) {
        // Ha az elem lemerült vagy most lett bekapcsolva
        Serial.println("RTC hiba: Az idő nem érvényes, beállítás a fordítási időre...");
        Rtc.SetDateTime(compiled);
    }
    if (Rtc.GetIsWriteProtected()) {
        Serial.println("RTC írásvédettség feloldása...");
        Rtc.SetIsWriteProtected(false);
    }
    if (!Rtc.GetIsRunning()) {
        Serial.println("RTC indítása...");
        Rtc.SetIsRunning(true);
    }
}

// --- Mentett beállítások betöltése ---
void loadSettings() {
  prefs.begin("tuner-cfg", true);
  config.tempLimit   = prefs.getFloat("limit", config.tempLimit);         //
  config.testTimeSec = prefs.getUInt("test", config.testTimeSec);         //
  config.outputStep  = prefs.getFloat("outStep", config.outputStep);      //
  config.settleTime  = prefs.getUInt("settle", config.settleTime);        //
  config.samples     = prefs.getUInt("samples", config.samples);          //
  config.loggingEnabled = prefs.getBool("logEn", config.loggingEnabled);  // Alapértelmezetten kikapcsolva
  config.inputSpan   = prefs.getFloat("ispan", config.inputSpan);
  config.outputSpan  = prefs.getFloat("ospan", config.outputSpan);

  String tempPass = prefs.getString("APPwd", config.apPassword);
  tempPass.toCharArray(config.apPassword, 32);

  String tempSSID = prefs.getString("APSSID", config.apSSID);
  tempSSID.toCharArray(config.apSSID, 32);

  prefs.end();
  Serial.println("Beállítások betöltve.");

}

// Hálózati paraméterek beállítása
  void initWiFi() {
    WiFi.softAP(config.apSSID, config.apPassword, 1, 0, 1);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP címe: ");
    Serial.println(myIP);
  }; 

void initRGBLED(){
  rgbLed.begin();
  rgbLed.setBrightness(RGB_Brightness); 
}

#endif