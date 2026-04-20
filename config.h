#ifndef CONFIG_H
#define CONFIG_H

#include <LovyanGFX.hpp>
#include <PWFusion_MAX31856.h>
#include <RtcDS1302.h>
#include <vector>
#include <Arduino.h>

/**********************************************************************
--- Verzióinformáció ---
***********************************************************************/
const char* const FIRMWARE_VERSION = "1.0.0"; 

/**********************************************************************
// Fontok definiálása  
***********************************************************************/
//const lgfx::IFont* font8 = &fonts::lgfxJapanGothicP_8;
//const lgfx::IFont* font12 = &fonts::lgfxJapanGothicP_12;
const lgfx::IFont* font16 = &fonts::lgfxJapanGothicP_16;
const lgfx::IFont* font20 = &fonts::lgfxJapanGothicP_20;
const lgfx::IFont* font24 = &fonts::lgfxJapanGothicP_24;
//const lgfx::IFont* font28 = &fonts::lgfxJapanGothicP_28;
const lgfx::IFont* font32 = &fonts::lgfxJapanGothicP_32;
//const lgfx::IFont* font36 = &fonts::lgfxJapanGothicP_36;
const lgfx::IFont* font40 = &fonts::lgfxJapanGothicP_40;


/**********************************************************************
// Színek definiálása uint32_t típusként (Hexa formátum: 0xRRGGBB)
***********************************************************************/
#define COLOR_RED     0xFF0000
#define COLOR_GREEN   0x00FF00
#define COLOR_BLUE    0x0000FF
#define COLOR_ORANGE  0xFF8C00
#define COLOR_OFF     0x000000


/**********************************************************************
Hardware beállítások
***********************************************************************/
// --- MAX31856 (Hőelem interfész) Beállítás és Lábkiosztás ---

const uint8_t TC_Type         = K_TYPE; 
const uint8_t TC_Cut          = CUTOFF_50HZ; 
const uint8_t TC_Avg          = AVG_SEL_16SAMP; 
const uint8_t TC_Cmode        = CMODE_AUTO; 
const uint8_t TC_Fault        = 14; 

#define MAX_CS 10
//#define MAX_DI 11
//#define MAX_DO 13
//#define MAX_CLK 12

int16_t TCstatus = -1;                            // A max 31856 által visszaadott érték, a -1 a nem értelmezett.

float internalTemp;                               // A MAX31856 chip belső (hidegpont) hőmérséklete

// --- DS1302 RTC Lábkiosztás ---
const uint8_t RTC_IO          = 2;                // DAT 
const uint8_t RTC_SCLK        = 1;                // CLK 
const uint8_t RTC_CE          = 3;                // RST 
ThreeWire myWire(RTC_IO, RTC_SCLK, RTC_CE);
RtcDS1302<ThreeWire> Rtc(myWire);
RtcDateTime now = Rtc.GetDateTime();              // Pillanatnyi idő az RTC-ben


// --- Kijelző (ST7789 SPI) Konfiguráció ---
const uint8_t TFT_SCLK       = 7;   
const uint8_t TFT_MOSI       = 15;   
const int8_t TFT_MISO        = -1;   
const uint8_t TFT_DC         = 17;   
const uint8_t TFT_CS         = 18;   
const uint8_t TFT_RST        = 16;    
const uint32_t TFT_SPI_FREQ  = 40000000; 
const uint32_t TFT_SPI_RFREQ = 16000000; 
const uint16_t TFT_WIDTH     = 240;  
const uint16_t TFT_HEIGHT    = 320;  
const bool TFT_INVERT        = true; 
const uint8_t TFT_ROTATION   = 1;    
const uint16_t DISPLAY_UPDATE_INTERVAL = 500;   // Kijelző frissítése (500ms)


// --- RGB LED Konfiguráció ---
const uint8_t RGB_LED_PIN     = 48; // ESP32-S3 N16R8 esetén gyakran 48 vagy 38
const uint8_t LED_COUNT       = 1;
uint8_t RGB_Brightness        = 30;
const uint8_t RGB_OFF         = 0;
const uint8_t RGB_ON          = 1;
const uint8_t RGB_FLASH       = 2;
bool ledState                 = false;
uint32_t RGB_Color            = COLOR_OFF;
uint16_t RGB_Freq             = 125;

// --- Vezérlőelemek és egyebek ---
const uint8_t relayPin        = 5;   
const uint8_t bootButtonPin   = 0;   
const uint8_t drdyPin         = 4;   

/**********************************************************************
Paraméterek, és változók
***********************************************************************/
const char* logFile = "/tuning_log.csv"; 
unsigned long utolsoOraFrissites = 0; 
const unsigned long oraInterval = 500;                        // 500 ms A képernyő, óra frissítési időszakasza


// --- Adatszerkezet az előzményekhez ---
struct DataPoint { 
    float temp; 
    uint32_t time;
};

// Egyedi allokátor, amely a PSRAM-ot használja
template <class T>
struct PSRAMAllocator {
  typedef T value_type;
  PSRAMAllocator() = default;

  template <class U> 
  PSRAMAllocator(const PSRAMAllocator<U>&) {}

  T* allocate(std::size_t n) {
    if (n > std::size_t(-1) / sizeof(T)) throw std::bad_alloc();
    // ps_malloc használata a belső malloc helyett
    if (auto p = static_cast<T*>(ps_malloc(n * sizeof(T)))) return p;
    throw std::bad_alloc();
  }

  void deallocate(T* p, std::size_t) noexcept {
    free(p);
  }
};

std::vector<DataPoint, PSRAMAllocator<DataPoint>> history;                  // Dinamikus tömb az adatok tárolására
const uint16_t HISTORY_SAVE_INTERVAL   = 5000;                              // Előzmények mentése (5mp)
const uint16_t WEB_HISTORY_LIMIT      = 300;                                // Max pontok száma a webes grafikonon (ritkítási korlát)
const size_t maxHistory           = 15000;                                  // Maximálisan tárolt előzmények

// A hangoláshoz szükséges paraméterek, beállítások (Webes felületen módosíthatóak)
struct Config {
  uint32_t testTimeSec            = 5400;                           // A teszt teljes időtartama
  float inputSpan                 = 1100.0;                         // a bemeneti tartomány max hőmérsékleti értéke.
  float outputSpan                = 3000.0;                         // A kimeneti vezérlőjel (PWM) maximális értéke
  float outputStep                = 3000.0;                         // Ez az a löket, amivel a tuner teszteli a rendszert. Egy 3,9 kW-os kemencénél óvatosan bánj vele! Ha túl nagy, túlszalad a hőmérséklet; ha túl kicsi, elvész a zajban.
  float tempLimit                 = 1150.0;    
  uint32_t settleTime             = 20;                             // Ez az idő (másodpercben), amíg a tuner vár, hogy a rendszer stabilizálódjon a tesztelés előtt. Mivel a kemence ~60 literes, itt érdemes akár 20-120 másodpercet hagyni.
  uint32_t samples                = 2700;                           // Hány mérést vegyen alapul a görbe illesztéséhez. A testTimeSec alatt (0.5 mp)
  char apPassword[32]             = "kemence123"; 
  char apSSID[32]                 = "Kemence_sTune"; 
  bool loggingEnabled             =  false;
};

int currentStatus;                                // A hangolás pillanatnyi állapota
float kp = 0, ki = 0, kd = 0;                     // Kiszámított PID konstansok 
float Input, Output;                              // PID bemenet (hőmérséklet) és kimenet (PWM jel)
bool tuningStarted = false;                       // A tuning el van-e indítva.
unsigned long tunestartTime = 0;
float elteltIdo;
static uint32_t lastHistorySave = 0;
sTune::TunerStatus aktualisStatusz;




/**********************************************************************
Specifikus Osztályok
***********************************************************************/

class LGFX_ESP32S3_ST7789 : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789  _panel_instance;
  lgfx::Bus_SPI       _bus_instance; 
public:
  LGFX_ESP32S3_ST7789(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host              = SPI3_HOST; 
      cfg.dma_channel           = SPI_DMA_CH_AUTO; 
      cfg.spi_mode              = 3; 
      cfg.freq_write            = TFT_SPI_FREQ;
      cfg.freq_read             = TFT_SPI_RFREQ;    // Olvasási sebesség
      cfg.pin_sclk              = TFT_SCLK;
      cfg.pin_mosi              = TFT_MOSI;
      cfg.pin_miso              = TFT_MISO;
      cfg.pin_dc                = TFT_DC;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs                = TFT_CS;
      cfg.pin_rst               = TFT_RST;
      cfg.panel_width           = TFT_WIDTH;
      cfg.panel_height          = TFT_HEIGHT;
      cfg.offset_x              = 0;
      cfg.offset_y              = 0;
      cfg.invert                = TFT_INVERT; 
      cfg.rgb_order             = false;
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};


#endif