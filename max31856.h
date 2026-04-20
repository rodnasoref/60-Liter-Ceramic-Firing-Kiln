#ifndef MAX31856_H
#define MAX31856_H

#include <PWFusion_MAX31856.h>
#include "config.h"


MAX31856  maxthermo;


void verifyTC(MAX31856 &tc) {
  TCstatus = tc.getStatus();
}


void initMAX31856() {
// MAX31856 inicializálás
  maxthermo.begin(MAX_CS);
  maxthermo.config(TC_Type, TC_Cut, TC_Avg, TC_Cmode);
  delay(150);
  maxthermo.sample();
}

String TCStstToSTR(uint8_t status){
  String hibastr = "";
  if(TC_FAULT_OPEN & status){ hibastr = "OPEN";}
  if(TC_FAULT_VOLTAGE_OOR & status) {if (hibastr.length()){hibastr += ", ";} hibastr += "Overvolt/Undervolt";}
  if(TC_FAULT_TC_TEMP_LOW & status) {if (hibastr.length()){hibastr += ", ";} hibastr += "TC Low";}
  if(TC_FAULT_TC_TEMP_HIGH & status) {if (hibastr.length()){hibastr += ", ";} hibastr += "TC High";}
  if(TC_FAULT_CJ_TEMP_LOW & status) {if (hibastr.length()){hibastr += ", ";} hibastr += "CJ Low";}
  if(TC_FAULT_CJ_TEMP_HIGH & status) {if (hibastr.length()){hibastr += ", ";} hibastr += "CJ High";}
  if(TC_FAULT_TC_OOR & status)       {if (hibastr.length()){hibastr += ", ";} hibastr += "TC Range";}
  if(TC_FAULT_CJ_OOR & status)       {if (hibastr.length()){hibastr += ", ";} hibastr += "CJ Range";}
  return hibastr;
}



#endif