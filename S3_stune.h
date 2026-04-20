#ifndef S3_STUNE_H
#define S3_STUNE_H

#include <sTune.h>
#include "config.h"
extern Config config;                                    // Rendszerkonfigurációs struktúra



// sTune objektum konfigurálása: NoOvershoot (túllövés mentes) algoritmus 
sTune tuner = sTune(&Input, &Output, tuner.Mixed_PID, tuner.directIP, tuner.printALL);



void tuner_init() {
  tuner.Configure(config.inputSpan, config.outputSpan, 0, config.outputStep, config.testTimeSec, config.settleTime, config.samples);
  tuner.SetEmergencyStop(config.tempLimit);                                         // Hardveres védelem szoftveres szinten
}




#endif