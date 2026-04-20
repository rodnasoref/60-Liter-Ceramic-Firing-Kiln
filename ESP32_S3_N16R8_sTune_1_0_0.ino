/**********************************************************************
60 Literes Kerámiaégető kemence hangolása v1.0.0
    Űrtartalom: 60 liter
    Célhőmérséklet: 1100 °C
    Teljesítmény: 3,9 kW
    Tesztelési idő: 90 perc (5400 másodperc a kódban)
    Vezérlő: ESP32-S3 (N16R8)
    Kijelző: 2.0 inch TFT Display OLED LCD Drive IC ST7789V (LovyanGFX) 240RGBx320 Dot-Matrix SPI Interface
    Hőmérsékletmérés: S típusú hőelem, MAX31856 chippel 
    Vezérlőelem: SSR (Solid State Relay - szilárdtest relé)
    Idő mérésére: DS1302 RTC
    Szoftver: stune könyvtár (a PID-szabályozás hangolásához), Software PWM (szoftveres PWM)
    - aktuális/cél hőmérséklet,
    - eltelt/hátralevő idő,
    - státusz (pillanatnyilag mit csinál a vezérlő).
    A művelet befejezésekor a kikalkulált értékek jelenjenek meg a kijelzőn
A működés minden paramétere állítható webes felületen
Kérés esetén logfájlt készít a LittleFS fájlrendszerbe
Az S-típusú hőelem hibája esetén a program azonnal kapcsolja ki az SSR-t.
túlhevülés gátlás
***********************************************************************/

/**********************************************************************
Változások a 1.0.0 verzióban:

- Teljesen újra lett struktúrálva a software

***********************************************************************/

#include "s3_stune.h"
#include "config.h"
#include "system_init.h"
#include "max31856.h"
#include "web_handlers.h"
#include "segedfuggvenyek.h"

LGFX_ESP32S3_ST7789 tft;                          // LovyanGFX kijelző vezérlő (ST7789 driver) 
LGFX_Sprite header(&tft);                         // Dupla puffereléshez használt sprite (villogásmentes frissítés) 
LGFX_Sprite content(&tft);
LGFX_Sprite footer(&tft);
LGFX_Sprite TCcj(&tft);
LGFX_Sprite aktido(&tft);


void setup() {
Serial.begin(115200);
  esp_reset_reason_t reason = esp_reset_reason();
  String reasonStr = getResetReason(reason);
  Serial.println("--- RENDSZER INDULÁS ---");
  Serial.print("Reset oka: ");
  Serial.println(reasonStr);
  
    
  // Watchdog konfigurálása (10 másodperces időtúllépés)
  esp_task_wdt_config_t wdt_config = { .timeout_ms = 10000, .trigger_panic = true }; // 
  esp_err_t err = esp_task_wdt_reconfigure(&wdt_config);
  if (err == ESP_ERR_INVALID_STATE) {
    esp_task_wdt_init(&wdt_config); 
    }
  esp_task_wdt_add(NULL);                         // Aktuális thread (loop) hozzáadása a figyeléshez


  // 1. ELLENŐRZÉS: Van-e korábbról mentett hiba az NVS-ben?
  String mentettHiba = GetNVS();

  currentStatus = -1;
 // Hardver és szoftver környezet inicializálása
  initPins();
  initDisplay();                        // Kijelző inicializálása
  updateDisplayHeader(String("sTune ") + FIRMWARE_VERSION, TFT_BLACK, TFT_WHITE);
  updateDisplayContentText2("Készítette:", font32, TFT_BLACK, TFT_DARKGREY, "Rodnas Oref", font32, TFT_DARKGREY); 
    // Megszakítás hozzárendelése: lefutó élre (FALLING) indul
  updateDisplayFooter("LittleFS Inicializálása", TFT_BLACK, TFT_YELLOW);
  if (!initFileSystem()){                // Fájlrendszer inicializálása, és a logfájl létrehozása ha szükséges
    SaveNVS("LittleFS inicializálás sikertelen!");
    updateDisplayContentText2p1("HIBA!:", font32, TFT_RED, TFT_BLACK, "LittleFS inicializálás!", font24, TFT_BLACK, "Újraindítás: BOOT gomb", font24, TFT_BLUE);
    STOPToBootButton(true);
  };

  updateDisplayFooter("RTC Inicializálása", TFT_BLACK, TFT_YELLOW);
  initRTC();                                // RTC indítása
  updateDisplayFooter("RGB Inicializálása", TFT_BLACK, TFT_YELLOW);
  initRGBLED();
  RGB_Color = COLOR_GREEN;
  RGB_Freq  = 125;
  updateRGB(RGB_ON);
  
  // Ha van mentett hiba, megállítjuk a rendszert és kiírjuk a kijelzőre
  if (mentettHiba.length() > 0) {
 //   RGB_Color = COLOR_RED;
    updateRGB(RGB_FLASH);
    updateDisplayContentText2p1("KORÁBBI HIBA!:", font24, TFT_ORANGE, TFT_BLACK, 
                                 mentettHiba, font20, TFT_BLACK, 
                                 "Törlés: BOOT gomb", font20, TFT_BLUE); 
    
    Serial.println("Kritikus hiba található az NVS-ben: " + mentettHiba);
    STOPToBootButton(false);                                                     // Itt várakozik, amíg a felhasználó nyugtázza 
    // A STOPToBootButton meghívja a DeleteNVS()-t és továbbengedi a rendszert 
  }
  // KRITIKUS BIZTONSÁGI LOGIKA:
  if (reason == ESP_RST_TASK_WDT || reason == ESP_RST_PANIC || reason == ESP_RST_BROWNOUT) {
    RGB_Color = COLOR_RED;
    // Ha hiba miatt indult újra a gép fűtés közben, 
    // akkor biztonsági okból tiltsd le a fűtést és várj felhasználói beavatkozásra.
    Serial.print("FIGYELEM: A rendszer hiba után indult újra!   Oka:");
    Serial.println(reasonStr);
    saveLogToFS(now, -1, -1, -1, -1, -1, -1, -1, "Újraindulás oka: " + reasonStr);
    updateDisplayContentText2p1("HIBA!:", font32, TFT_ORANGE, TFT_BLACK, reasonStr, font24, TFT_BLACK, "Újraindítás: BOOT gomb", font24, TFT_BLUE);
    STOPToBootButton(true);
 }
 
  updateDisplayFooter("AP konfigurálása...", TFT_BLACK, TFT_YELLOW);
  initWiFi();                           // Hálózati indítása
  updateDisplayFooter("31856 Inicializálása", TFT_BLACK, TFT_YELLOW);
  initMAX31856();
  MAX_Fault();
  updateDisplayFooter("Webszerver Inicializálása", TFT_BLACK, TFT_YELLOW);
  initWebServer();
  updateDisplayFooter("Beállítások betöltése", TFT_BLACK, TFT_YELLOW);
  loadSettings();                       // Mentett beállítások betöltése
  updateDisplayFooter("Inicializálás kész", TFT_BLACK, TFT_YELLOW);

  history.reserve(maxHistory);                    // Egyetlen nagy foglalás az elején, nincs több másolgatás futás közben
  pinMode(relayPin, OUTPUT);


  tuner_init();
  
}

void loop() {
  unsigned long jelenlegiMillis = millis();
  server.handleClient();
    // Gombnyomás figyelése az indításhoz
  if (!tuningStarted && digitalRead(bootButtonPin) == LOW) {
    delay(50); // Debounce
    if (digitalRead(bootButtonPin) == LOW) {
      tunestartTime = millis();
      tuningStarted = true;
      currentStatus = 0; // Futás állapotba lépés
      Serial.println("Hangolás elindítva!") ;
      RGB_Color = COLOR_BLUE;
      updateRGB(RGB_ON);
    }    
  }

    if (tuningStarted) {
    // Itt hívódnak meg az sTune könyvtár futtató funkciói
    // tuner.Runtime(); stb.
    aktualisStatusz =(sTune::TunerStatus)tuner.Run();
    tuner.softPwm(relayPin, Input, Output, 0, config.outputSpan, 1);
    switch (aktualisStatusz) {
    case tuner.tunings:                                                              // active once per sample during test
      updateDisplayFooter("Hangolás kész...", TFT_BLACK, TFT_YELLOW);
      tuner.GetAutoTunings(&kp, &ki, &kd); // sketch variables updated by sTune
      Output = 0; 
      digitalWrite(relayPin, LOW);
      tuningStarted = false;
      saveLogToFS(now, internalTemp, Input, Output, currentStatus, kp, ki, kd);

// 2. Eredmények megjelenítése (Stringbe formázva)
      char pidBuf[32];
      snprintf(pidBuf, sizeof(pidBuf), "P:%.2f I:%.2f D:%.2f", kp, ki, kd);
      
      updateDisplayContentText2p1("SIKERES HANGOLÁS!", font24, TFT_BLUE, TFT_BLACK, 
                                   pidBuf, font20, TFT_WHITE, 
                                   "Kilépés: BOOT gomb", font20, TFT_YELLOW);
      
      updateDisplayFooter("Hangolás kész.", TFT_BLACK, TFT_GREEN);
      
      // Itt megállíthatjuk a programot, amíg a felhasználó nem nyugtázza a gombbal
      STOPToBootButton(false); 
      break;
    default:
      if (!digitalRead(drdyPin)) Input = maxthermo.getTemperature();
      updateDisplayFooter("Hangolás fut...", TFT_BLACK, TFT_MAGENTA);
    break;
    }
    esp_task_wdt_reset();
  }

  // Csak 500ms-onként fut le, nem foglalja le a CPU-t folyamatosan
  if (jelenlegiMillis - utolsoOraFrissites >= oraInterval) {
    utolsoOraFrissites = jelenlegiMillis;
    internalTemp = maxthermo.getColdJunctionTemperature();
    maxthermo.sample();
    updateDisplayaktido(); 
    updateDisplayTCcj();
    if (!tuningStarted) {
    updateDisplayContentText2p1("Üzemkész!", font32, TFT_GREEN, TFT_BLACK, "Inicializálás kész!", font24, TFT_BLACK, "Indítás: BOOT gomb", font24, TFT_YELLOW);
    updateDisplayFooter("Indításra vár...", TFT_BLACK, TFT_GREEN);
    }else{
      String hofok = String(Input,1) + "°C";
      String celhofok = String(config.inputSpan,1) + "°C";
      updateDisplayContentText2p1(hofok, font40, TFT_BLACK, TFT_MAGENTA, celhofok, font24, TFT_BLUE, getBVI(), font20, TFT_DARKGREY);
     }
    MAX_Fault(); 
  }


  // Watchdog számláló nullázása (Jelzi a rendszernek, hogy nem fagytunk le)
  esp_task_wdt_reset();




    // Hőmérsékleti előzmények mentése 5 másodpercenként 
    if (millis() - lastHistorySave > HISTORY_SAVE_INTERVAL) {
        if (history.size() >= maxHistory) history.erase(history.begin()); // Túlcsordulás megelőzése
        // RTC idő lekérése a mentéshez
    RtcDateTime now = Rtc.GetDateTime();
    // Kiszámoljuk a napból eltelt összes másodpercet a könnyebb tároláshoz
    uint32_t secondsToday = (uint32_t)now.Hour() * 3600 + (uint32_t)now.Minute() * 60 + now.Second();

       // Memóriába mentés (a grafikonnak)
        history.push_back({Input, secondsToday});

        // FÁJLRENDSZERBE MENTÉS (naplózás)
        if (tuningStarted && config.loggingEnabled) {
              saveLogToFS(now, internalTemp, Input, Output, currentStatus, kp, ki, kd);
        }
        lastHistorySave = millis();
    }

}

