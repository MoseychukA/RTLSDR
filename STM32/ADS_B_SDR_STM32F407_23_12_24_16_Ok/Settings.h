#pragma once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "AT24CX.h"
#include "DS3231.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
  int raw;
  float voltage;
  
} VoltageData;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class SettingsClass
{
public:

  bool read8(uint32_t addr, uint8_t& val);
  bool read16(uint32_t addr, uint16_t& val);
  bool read32(uint32_t addr, uint32_t& val);

  void write8(uint32_t addr, uint8_t val);
  void write16(uint32_t addr, uint16_t val);
  void write32(uint32_t addr, uint32_t val);

	SettingsClass();

	void begin();

	void update();

  void reloadSettings();

  EEPROM_CLASS* getEEPROM() {return eeprom;}


	DS3231Temperature getTemperature() { return coreTemp; }

	void set3V3RawVoltage(uint16_t raw);
	void set5VRawVoltage(uint16_t raw);
	void set200VRawVoltage(uint16_t raw);
	void setVer(String ver);
	String getVer();
	VoltageData get3V3Voltage() { return voltage3V3; }
	VoltageData get5Vvoltage() { return voltage5V; }
	VoltageData get200Vvoltage() { return voltage200V; }

	String getUUID(const char* passedUUID);
    
  private:

  
    EEPROM_CLASS* eeprom;
    DS3231Temperature coreTemp;
    uint32_t timer;
	String ver_prog;
    VoltageData voltage3V3, voltage5V, voltage200V;
  
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern SettingsClass Settings;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
