#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"

// Webszerver példányosítása a 80-as porton
WebServer server(80);

// Főprogramban és a segédfüggvényekben definiált elemek hivatkozása
extern void updateRGB(uint8_t mode);
extern String allapotSzovegge(sTune::TunerStatus a);
extern String getBVI();
extern void tuner_init();

// ---------------------------------------------------------
// FÁJL KISZOLGÁLÓ VÉGPONTOK (LittleFS-ből)
// ---------------------------------------------------------

void handleRoot() {
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
        server.send(404, "text/plain", "Hiba: /index.html nem talalhato a LittleFS-en!");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

void handleCss() {
    File file = LittleFS.open("/style.css", "r");
    if (!file) {
        server.send(404, "text/plain", "Not found");
        return;
    }
    server.streamFile(file, "text/css");
    file.close();
}

void handleChartJs() {
    File file = LittleFS.open("/chart.min.js", "r");
    if (!file) {
        server.send(404, "text/plain", "Not found");
        return;
    }
    server.streamFile(file, "application/javascript");
    file.close();
}

void handleLogs() {
    File file = LittleFS.open(logFile, "r");
    if (!file) {
        server.send(404, "text/plain", "A log fajl meg ures vagy nem letezik.");
        return;
    }
    server.streamFile(file, "text/csv");
    file.close();
}

// ---------------------------------------------------------
// API VÉGPONTOK
// ---------------------------------------------------------

void handleApiData() {
    DynamicJsonDocument doc(1024);
    
    // Alap mérési adatok és paraméterek
    doc["temp"] = Input;
    doc["internalTemp"] = internalTemp;
    doc["status"] = tuningStarted ? allapotSzovegge(aktualisStatusz) : "Üzemkész";
    
    // Kimenet százalékos értékének kiszámítása a löket max értékéből
    // A kimenet százalékát csak akkor számoljuk, ha valóban fut a folyamat
    float outputPercent = 0;
    if (tuningStarted) {
        outputPercent = (config.outputSpan > 0) ? (Output / config.outputSpan * 100.0) : 0;
    }
    doc["output"] = outputPercent;
    
    doc["kp"] = kp;
    doc["ki"] = ki;
    doc["kd"] = kd;
    doc["bvi"] = tuningStarted ? getBVI() : "--:--:--";

    // Konfigurációs objektum az NVS adatokkal
    JsonObject conf = doc.createNestedObject("config");
    conf["inputSpan"] = config.inputSpan;
    conf["tempLimit"] = config.tempLimit;
    conf["testTimeSec"] = config.testTimeSec;
    conf["outputStep"] = config.outputStep;
    conf["settleTime"] = config.settleTime;
    conf["samples"] = config.samples;
    conf["loggingEnabled"] = config.loggingEnabled;

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleApiHistory() {
    // Memóriahatékony JSON streamelés, chunk-olt adatküldéssel
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");
    
    String buffer = "{\"labels\":[";
    
    // Adatpontok ritkítása a webes limit szerint
    int step = history.size() / WEB_HISTORY_LIMIT;
    if (step < 1) step = 1;
    
    bool first = true;
    for (size_t i = 0; i < history.size(); i += step) {
        if (!first) buffer += ",";
        buffer += String(history[i].time);
        
        // Ha a puffer megtelt, küldjük a hálózatba (Chunk kiürítés)
        if (buffer.length() > 1024) {
            server.sendContent(buffer);
            buffer = "";
        }
        first = false;
    }
    
    buffer += "],\"values\":[";
    first = true;
    for (size_t i = 0; i < history.size(); i += step) {
        if (!first) buffer += ",";
        buffer += String(history[i].temp, 2);
        
        if (buffer.length() > 1024) {
            server.sendContent(buffer);
            buffer = "";
        }
        first = false;
    }
    buffer += "]}";
    
    // Maradék puffer küldése
    if (buffer.length() > 0) {
        server.sendContent(buffer);
    }
    server.sendContent(""); // Stream lezárása
}

void handleApiControl() {
    if (server.hasArg("action")) {
        String action = server.arg("action");
        if (action == "start" && !tuningStarted) {
            tunestartTime = millis();
            tuningStarted = true;
            currentStatus = 0; 
            RGB_Color = COLOR_BLUE;
            updateRGB(RGB_ON);
            Serial.println("Kliens parancs: Hangolás elindítva!");
            server.send(200, "text/plain", "Started");
        } else if (action == "stop" && tuningStarted) {
            tuningStarted = false;
            Output = 0;
            digitalWrite(relayPin, LOW);
            RGB_Color = COLOR_GREEN;
            updateRGB(RGB_ON);
            Serial.println("Kliens parancs: Hangolás leállítva!");
            server.send(200, "text/plain", "Stopped");
        } else {
            server.send(400, "text/plain", "Ervenytelen allapot vagy muvelet");
        }
    } else {
        server.send(400, "text/plain", "Hianyzo 'action' parameter");
    }
}

void handleApiSync() {
    if (server.hasArg("epoch")) {
        uint32_t epoch = server.arg("epoch").toInt();
        // Helyi időszámítás eltolása (CEST UTC+2 = 7200 másodperc).
        RtcDateTime dt;
        dt.InitWithEpoch32Time(epoch + 7200);
        Rtc.SetDateTime(dt);
        Serial.println("RTC idő sikeresen szinkronizálva a böngészővel.");
        server.send(200, "text/plain", "Szinkronizálva");
    } else {
        server.send(400, "text/plain", "Hianyzo 'epoch' parameter");
    }
}

void handleApiSave() {
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        DynamicJsonDocument doc(1024);
        DeserializationError err = deserializeJson(doc, body);
        
        if (err) {
            server.send(400, "text/plain", "Hibas JSON formátum!");
            return;
        }

        // Változók mentése a RAM memóriába
        config.inputSpan = doc["inputSpan"];
        config.tempLimit = doc["tempLimit"];
        config.testTimeSec = doc["testTimeSec"];
        config.outputStep = doc["outputStep"];
        config.settleTime = doc["settleTime"];
        config.samples = doc["samples"];
        config.loggingEnabled = doc["loggingEnabled"];

        // Konfiguráció kiírása a NVS-be a túléléshez (Preferences)
        prefs.begin("tuner-cfg", false);
        prefs.putFloat("ispan", config.inputSpan);
        prefs.putFloat("limit", config.tempLimit);
        prefs.putUInt("test", config.testTimeSec);
        prefs.putFloat("outStep", config.outputStep);
        prefs.putUInt("settle", config.settleTime);
        prefs.putUInt("samples", config.samples);
        prefs.putBool("logEn", config.loggingEnabled);
        prefs.end();

        // Tuner struktúra újra-inicializálása az új határértékekkel
        tuner_init();

        Serial.println("Sikeres NVS mentés és tuner paraméterek frissítése a weben keresztül.");
        server.send(200, "text/plain", "Mentve");
    } else {
        server.send(400, "text/plain", "Hianyzo JSON adatok");
    }
}

// ---------------------------------------------------------
// WEBSZERVER INICIALIZÁLÁSA
// ---------------------------------------------------------

void initWebServer() {
    // Alap file rendszer útvonalak
    server.on("/", HTTP_GET, handleRoot);
    server.on("/style.css", HTTP_GET, handleCss);
    server.on("/chart.min.js", HTTP_GET, handleChartJs);
    server.on("/logs", HTTP_GET, handleLogs);
    
    // API végpontok az index.html Fetch hívásaihoz
    server.on("/api/data", HTTP_GET, handleApiData);
    server.on("/api/history", HTTP_GET, handleApiHistory);
    server.on("/api/control", HTTP_GET, handleApiControl);
    server.on("/api/sync", HTTP_GET, handleApiSync);
    server.on("/api/save", HTTP_POST, handleApiSave);
    
    server.begin();
    Serial.println("Webszerver és API interfész elindítva a 80-as porton.");
}

#endif // WEB_HANDLERS_H
