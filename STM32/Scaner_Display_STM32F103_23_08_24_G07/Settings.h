#pragma once
//--------------------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include "Utils.h"
#include "CoreButton.h"
#include "TFTMenu.h"
#include "AT24CX.h"               // Поддержка энергонезависимой памяти
#include "CONFIG.h"               // Основные настройки программы
#include "TFT_Includes.h"


#ifdef USE_BUZZER
#include "Buzzer.h"
#endif

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Программа управления питанием одной кнопкой
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef enum
{
    powerViaUSB = 10,
    batteryPower = 20

} PowerType;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

typedef struct
{
    int raw;
  	float voltage_Akk;

} VoltageData;

//--------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push,1)
typedef struct
{
  bool isValid;
  uint16_t points[5];
  
} TFTCalibrationData;
#pragma pack(pop)
//--------------------------------------------------------------------------------------------------------------------------------
class SettingsClass
{
  public:
    SettingsClass();

   void setup();

   
    int GetControllerID(const char* passedUUID);
    void SetControllerID(uint16_t val);


  TFTCalibrationData GetTftCalibrationData();
  void SetTftCalibrationData(TFTCalibrationData& data);

   uint16_t GetTimeLedLCD();
  void SetTimeLedLCD(uint16_t val);

  uint16_t GetTimePowerOff();
  void SetTimePowerOff(uint16_t val);


  uint32_t getCurrentCoeff();
  void setCurrentCoeff(uint32_t c);

  // возвращает тип питания - от батарей или USB
  PowerType getPowerType();
  void turnPowerOff(); // выключает питание контроллера
     // управление подсветкой экрана
  void displayBacklight(bool bOn);
  bool isBacklightOn() { return backlightFlag; }

  void update();

  uint16_t getPowerVoltageAkk(uint16_t pin);

  VoltageData voltageAkk;  // Питание аккумуляторов
 
  bool getTouch();

  uint32_t getRSSILowBorder();
  void setRSSILowBorder(uint32_t val);

  uint32_t getRSSIHighBorder();
  void setRSSIHighBorder(uint32_t val);

  uint16_t GetThreshold1() { return ValueThreshold1; }
  uint16_t GetThreshold2() { return ValueThreshold2; }
  uint16_t GetThreshold3() { return ValueThreshold3; }
 
 private:


  Button powerButton;
  PowerType powerType;
  static void checkPower();
  bool backlightFlag;
 
  int16_t ret_interval = 0;

  bool array_countMax = false;
  int sum = 0;
  uint8_t array_count = 0;
  uint8_t array_size = 30;
  int dimension_array[30];

  bool wifiState = false;
  bool wifiConnect = false;
  String routerID;
  String routerPassword;
  String stationID;
  String stationPassword;

  uint8_t read8(uint16_t address, uint8_t defaultVal);
  uint16_t read16(uint16_t address, uint16_t defaultVal);
  void write8(uint16_t address, uint8_t val);
  void write16(uint16_t address, uint16_t val);

  unsigned long read32(uint16_t address, unsigned long defaultVal);
  void write32(uint16_t address, unsigned long val);

  String readString(uint16_t address, byte maxlength);
  void writeString(uint16_t address, const String& v, byte maxlength);

  uint32_t currentCoeff; // коэффициент по току
  uint32_t RSSIHighBorder, RSSILowBorder;

#ifdef USE_TFT_MODULE
  int ValueThreshold1;
  int ValueThreshold2;
  int ValueThreshold3;

  int OldValueThreshold1;
  int OldValueThreshold2;
  int OldValueThreshold3;

  TOUCH_Class* _URTouch;

#endif 

};
//--------------------------------------------------------------------------------------------------------------------------------
extern SettingsClass Settings;
//--------------------------------------------------------------------------------------------------------------------------------

