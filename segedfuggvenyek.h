#ifndef SEGEDFUGGVENYEK_H
#define SEGEDFUGGVENYEK_H

#include "config.h"
#include <Preferences.h>
#include <Ticker.h>
#include <iomanip> // a setprecision-höz
#include <sstream>

Ticker ledTicker;

String getResetReason(esp_reset_reason_t reason) {
  
  switch (reason) {
    case ESP_RST_POWERON:  return "Hidegindítás (Power-on)";
    case ESP_RST_EXT:      return "Külső Reset gomb";
    case ESP_RST_SW:       return "Szoftveres újraindítás";
    case ESP_RST_PANIC:    return "Szoftverösszeomlás (Exception)";
    case ESP_RST_INT_WDT:  return "Watchdog (Interrupt)";
    case ESP_RST_TASK_WDT: return "Watchdog (Task fagyás)";
    case ESP_RST_WDT:      return "Egyéb Watchdog hiba";
    case ESP_RST_DEEPSLEEP:return "Mélyalvásból ébredés";
    case ESP_RST_BROWNOUT: return "Tápfeszültség esés (Brownout)";
    case ESP_RST_SDIO:     return "SDIO Reset";
    default:               return "Ismeretlen ok";
  }
}
/*
// Segédfüggvény a formázott dátumhoz (a CSV-be)
String getFormattedDateTime() {
    RtcDateTime now = Rtc.GetDateTime();
    char buff[20];
    snprintf_P(buff, countof(buff), PSTR("%04u.%02u.%02u %02u:%02u:%02u"),
            now.Year(), now.Month(), now.Day(),
            now.Hour(), now.Minute(), now.Second());
    return String(buff);
}
*/

String getFormattedTime() {
    RtcDateTime now = Rtc.GetDateTime();
    char buff[9];
    snprintf_P(buff, countof(buff), PSTR("%02u:%02u:%02u"),
            now.Hour(), now.Minute(), now.Second());
    return String(buff);
}

  void updateDisplayHeader(const String& szoveg, int hatter, int szovegszin){
    header.setFont(font20);
    header.fillSprite(hatter);
    header.setTextDatum(ML_DATUM);
    header.setTextSize(1);
    header.setTextColor(szovegszin);
    header.drawString(szoveg, 5, header.height() / 2); 
 //   header.drawFastHLine(0, header.height()-1, header.width(), 0xC618); 
    header.pushSprite(0, 0); // És ki kell küldeni a kijelzőre
  };


void updateDisplayTCcj(){
    TCcj.setFont(font20);
    TCcj.fillSprite(TFT_BLACK);
    TCcj.setTextSize(1);
    TCcj.setTextColor(TFT_WHITE);
    TCcj.setTextDatum(MR_DATUM);
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << internalTemp;
    std::string s = stream.str()+"°C";
    TCcj.drawString(s.c_str(), TCcj.width() - 5, TCcj.height() / 2); 
//    TCcj.drawFastHLine(0, TCcj.height()-1, TCcj.width(), 0xC618); 
    TCcj.pushSprite(header.width() + 1, 0); // És ki kell küldeni a kijelzőre
}




  void updateDisplayContentText2(const String& szoveg1, const lgfx::IFont* betu1, int hatter, int szovegszin1, const String& szoveg2, const lgfx::IFont* betu2, int szovegszin2){
    content.setFont(betu1);
    content.fillSprite(hatter);
    content.setTextDatum(MC_DATUM);
    content.setTextSize(1);
    content.setTextColor(szovegszin1);
    content.drawString(szoveg1, content.width() / 2, content.height() / 4);
    content.setTextColor(szovegszin2);
    content.setFont(betu2);
    content.drawString(szoveg2, content.width() / 2, (content.height() / 4) * 3);
    content.pushSprite(0, 41); // És ki kell küldeni a kijelzőre
  };

  void updateDisplayContentText2p1(const String& szoveg1, const lgfx::IFont* betu1, int hatter, int szovegszin1, const String& szoveg2, const lgfx::IFont* betu2, int szovegszin2, const String& szoveg3, const lgfx::IFont* betu3, int szovegszin3){
    content.setFont(betu1);
    content.fillSprite(hatter);
    content.setTextDatum(MC_DATUM);
    content.setTextSize(1);
    content.setTextColor(szovegszin1);
    content.drawString(szoveg1, content.width() / 2, content.height() / 5);
    content.setFont(betu2);
    content.setTextColor(szovegszin2);
    content.drawString(szoveg2, content.width() / 2, (content.height() / 5) * 3);
    content.setFont(betu3);
    content.setTextColor(szovegszin3);
    content.drawString(szoveg3, content.width() / 2, (content.height() / 5) * 4);
    content.pushSprite(0, 41); // És ki kell küldeni a kijelzőre
  };
 
  void updateDisplayFooter(const String& statusz, int hatter, int szovegszin) {
    footer.setFont(font16);
    footer.fillSprite(hatter);                // Sprite törlése az aktuális háttérszínnel
    footer.setTextSize(1);
    footer.setTextColor(szovegszin);
//    footer.drawFastHLine(0, 0, footer.width(), 0xC618); 

    // 1. STÁTUSZ KIÍRÁSA BALRA
    // A (5, footer.height()/2) koordináta a bal szélétől 5 pixelre, függőlegesen középen van
    footer.setTextDatum(ML_DATUM);            // Middle-Left (Közép-Bal) igazítás
    footer.drawString(statusz, 5, footer.height() / 2);

    // Megjelenítés a kijelző alján (az Y koordináta a kijelző magasságától függ)
    // Mivel a kijelző 240 magas és a footer 40, a (0, 200) helyre toljuk ki
    footer.pushSprite(0, 200); 
};

void updateDisplayaktido(){
 // 2. IDŐ KIÍRÁSA JOBBRA
    // A getFormattedTime() függvényt a segedfuggvenyek.h-ból használjuk
    aktido.setFont(font16);
    aktido.fillSprite(TFT_BLACK);                // Sprite törlése az aktuális háttérszínnel
    aktido.setTextSize(1);
    aktido.setTextColor(TFT_WHITE);
    String ido = getFormattedTime(); 
    aktido.setTextDatum(MR_DATUM);            // Middle-Right (Közép-Jobb) igazítás
    aktido.drawString(ido, aktido.width() - 5, aktido.height() / 2);
//    aktido.drawFastHLine(0, 0, aktido.width(), 0xC618); 
    aktido.pushSprite(footer.width() + 1, 200); 
}


/**
 * Adatok mentése LittleFS-be CSV formátumban.
 * Formátum: Időbélyeg;Belső_T;Hőelem_T;Kimenet_%;Státusz;Kp;Ki;Kd
 */
void saveLogToFS(RtcDateTime now, float internaltemp, float input, float output, int currentstatus, float p, float i, float d, String megjegyzes = "") {
    File file = LittleFS.open(logFile, FILE_APPEND);
    if (!file) {
        Serial.println("Hiba a log fájl megnyitásakor!");
        return;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "%04d.%02d.%02d %02d:%02d:%02d;%.2f;%.2f;%.1f;%d;%.4f;%.4f;%.4f;%s\n",
             now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second(),
             internaltemp, input, (output / (config.outputSpan / 100.0)), currentstatus, p, i, d, megjegyzes.c_str());

    file.print(buf);
    file.close();
}

void SaveNVS(const char* reason){
  prefs.begin("tuner-cfg", false); 
  prefs.putString("lastError", reason); 
  prefs.end();
  }

  String GetNVS(){
  prefs.begin("tuner-cfg", true); 
  String lastSavedError = prefs.getString("lastError", ""); 
  prefs.end();
  return lastSavedError;
  }

  void DeleteNVS(){
    prefs.begin("tuner-cfg", false);
    prefs.remove("lastError");
    prefs.end();
  }

  void toggleLED() {
    if (ledState) {
      rgbLed.setPixelColor(0,COLOR_OFF);
    }
    else {rgbLed.setPixelColor(0,RGB_Color);
    }
  rgbLed.show();
  ledState = !ledState;

}

  void updateRGB(uint8_t mode){
    rgbLed.setBrightness(RGB_Brightness); 
    switch (mode){
      case RGB_ON:
        ledTicker.detach();
        rgbLed.setPixelColor(0,RGB_Color);
        rgbLed.show();
      break;
      case RGB_FLASH:
        ledTicker.attach(RGB_Freq / 1000.0, (void (*)(void))toggleLED);
      break;
      default:
        ledTicker.detach();
        rgbLed.clear();
        rgbLed.show();
      break;
    }
  }

  void STOPToBootButton(bool reboot){
    const uint32_t debounce_delay = 50; // 50ms a stabil állapothoz
    // 1. Várakozás a lenyomásra (LOW állapotra, mert a gomb lehúz a földre)
    while (true) {
      if (digitalRead(bootButtonPin) == LOW) {
        delay(debounce_delay); // Várunk, hogy megnyugodjon a jel
        if (digitalRead(bootButtonPin) == LOW) break; // Tényleg le van nyomva
        updateDisplayaktido(); 
        updateDisplayTCcj();
        server.handleClient();
      }
      esp_task_wdt_reset();
    }

    // 2. Várakozás az elengedésre (HIGH állapotra)
    while (true) {
      if (digitalRead(bootButtonPin) == HIGH) {
        delay(debounce_delay); // Várunk, hogy megnyugodjon a jel
        if (digitalRead(bootButtonPin) == HIGH) break; // Tényleg felengedték
        updateDisplayaktido(); 
        updateDisplayTCcj();

      } 
      esp_task_wdt_reset();
    }
    DeleteNVS();
    if (reboot) {
      esp_restart();
      } else {
        updateRGB(RGB_ON);
      }

  }


void MAX_Fault(){
  verifyTC(maxthermo);
  if (TCstatus){   
    RGB_Color = COLOR_RED;
    pinMode(relayPin, INPUT_PULLDOWN);
    digitalWrite(relayPin, LOW);
    updateRGB(RGB_FLASH);
    String hibastr = TCStstToSTR(TCstatus);
    Serial.println("HIBA: "+hibastr);
    now = Rtc.GetDateTime();
    String teljesHiba = "MAX31856! " + hibastr;
    saveLogToFS(now,-1,-1,-1,-1,-1,-1,-1,teljesHiba);
    SaveNVS(teljesHiba.c_str());
    updateDisplayContentText2p1("HIBA!", font32, TFT_RED, TFT_BLACK, hibastr, font24, TFT_BLACK, "Újraindítás: BOOT gomb", font24, TFT_WHITE);
    updateDisplayFooter("HOPPÁ!...", TFT_BLACK, TFT_RED);
    STOPToBootButton(true);
  }
}

String getBVI() {
    if (!tuningStarted) return "--:--:--"; 

    RtcDateTime now = Rtc.GetDateTime();
    elteltIdo = (millis() - tunestartTime) / 1000; 
    uint32_t remaining = (config.testTimeSec > elteltIdo) ? (config.testTimeSec - elteltIdo) : 0; 

    RtcDateTime bviTime = now + remaining;

    char buf[15];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", bviTime.Hour(), bviTime.Minute(), bviTime.Second());
    String vissza = "BVI: " + String(buf);
    return vissza;
}



String allapotSzovegge(sTune::TunerStatus a) {
  switch (a) {
    case sTune::sample: return "Mintavételezés";
    case sTune::test:   return "Tesztelés";
    case sTune::tunings:    return "Hangolás kész";
    case sTune::runPid:    return "Szabályozás";
    case sTune::timerPid:    return "Időzített PID)";
    default:      return "Ismeretlen";
  }
}
 
#endif
