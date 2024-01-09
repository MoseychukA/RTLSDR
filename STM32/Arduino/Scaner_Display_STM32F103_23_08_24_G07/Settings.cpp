#include "Settings.h"
#include "TFTMenu.h"
#include <Wire.h>                 //
#include "AT24CX.h"
#include "Memory.h"
#include "ADCSampler.h"


AT24CX mem;

#define FONT_HEIGHT(dc) dc->fontHeight(1)


//--------------------------------------------------------------------------------------------------------------------------------
SettingsClass Settings;
//--------------------------------------------------------------------------------------------------------------------------------
SettingsClass::SettingsClass()
{

    voltageAkk.raw      = 0;
    voltageAkk.voltage_Akk = 0;

}

//--------------------------------------------------------------------------------------------------------------------------------
TFTCalibrationData SettingsClass::GetTftCalibrationData()
{
  TFTCalibrationData result;

  byte corr_data = mem.read(TftCalibrationData_ADDRESS);
  result.isValid = mem.read(TftCalibrationData_ADDRESS+2);
  int adr_data = 0;

  for (int i = 0; i < 5; i++)
  {
      result.points[i] = mem.readInt(TftCalibrationData_ADDRESS + 4 + adr_data);
      adr_data++;
      adr_data++;
  }

  if (corr_data != CORRECT_DATA)
  {
    result.isValid = false;
    return result;
  }
  
  result.isValid = true;
  return result;  
}
//--------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::SetTftCalibrationData(TFTCalibrationData& data)
{
  data.isValid = true;
 
  mem.write(TftCalibrationData_ADDRESS, CORRECT_DATA);
  mem.write(TftCalibrationData_ADDRESS+2, data.isValid);

  int adr_data = 0;

  for (int i = 0; i < 5; i++)
  {
      mem.writeInt(TftCalibrationData_ADDRESS + 4+ adr_data, data.points[i]);
      adr_data++;
      adr_data++;
  }
  
}
//--------------------------------------------------------------------------------------------------------------------------------

uint16_t SettingsClass::GetTimeLedLCD()  // Получить время отключения подсветки дисплея при отсутсвии активности
{
	byte corr_data = mem.read(TimeLedLCD_ADDRESS);
    uint16_t result = mem.readInt(TimeLedLCD_ADDRESS + 2);
    if (corr_data != CORRECT_DATA)
    {
        result = BACKLIGHT_OFF_DELAY; // по умолчанию 
    }

    if (result < 1)
    {
        result = BACKLIGHT_OFF_DELAY;
    }

    return result;
}
//--------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::SetTimeLedLCD(uint16_t val) // Записать время отключения подсветки дисплея при отсутсвии активности
{
    if (val < 1)
    {
        val = BACKLIGHT_OFF_DELAY;
    }
    mem.write(TimeLedLCD_ADDRESS, CORRECT_DATA);
    mem.writeInt(TimeLedLCD_ADDRESS + 2, val);
}
//--------------------------------------------------------------------------------------------------------------------------------

uint16_t SettingsClass::GetTimePowerOff()  // Получить время отключения прибора при отсутсвии активности
{
	byte corr_data = mem.read(TimePowerOff_ADDRESS);
    uint16_t result = mem.readInt(TimePowerOff_ADDRESS + 2);

    if (corr_data != CORRECT_DATA)
    {
        result = TIME_POWER_OFF_DELAY; // по умолчанию 
    }

    if (result < 1)
    {
        result = TIME_POWER_OFF_DELAY;
    }

    return result;
}
//--------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::SetTimePowerOff(uint16_t val) // Записать время отключения прибора при отсутсвии активности
{
    if (val < 1)
    {
        val = TIME_POWER_OFF_DELAY;
    }
    mem.write(TimePowerOff_ADDRESS, CORRECT_DATA);
    mem.writeInt(TimePowerOff_ADDRESS + 2, val);
}
//--------------------------------------------------------------------------------------------------------------------------------

int SettingsClass::SettingsClass::GetControllerID(const char* passedUUID)
{
	byte corr_data = mem.read(ControllerID_ADDRESS);
    uint16_t result = mem.readInt(ControllerID_ADDRESS + 2);

    if (corr_data != CORRECT_DATA)
    {
        result = 0;
    }

    return result;
}
//--------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::SetControllerID(uint16_t val)
{
    if (val < 1)
    {
        val = DEFAULT_ControllerID;
    }
    mem.write(ControllerID_ADDRESS, CORRECT_DATA);
    mem.writeInt(ControllerID_ADDRESS + 2, val);
}

void SettingsClass::turnPowerOff()
{
     // выключаем питание контроллера
    digitalWrite(POWER_ON_OUT, LOW);
}
//--------------------------------------------------------------------------------------------------------------------------------
PowerType SettingsClass::getPowerType()
{
    return powerType;
}
//--------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::checkPower() // Определение от какого источника питается прибор. Определяется в момент включения прибора.
{
  //  if (!digitalRead(POWER_ON_IN))
  //  {
  //      Settings.powerType = batteryPower;
		////DBGLN("");
  ////      DBGLN(F("BATTERY POWER !!!"));
  //  }
  //  else
  //  {
  //      Settings.powerType = powerViaUSB;
		////DBGLN("");
  ////      DBGLN(F("POWER  VIA USB !!!"));
  //  }
}
//--------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::displayBacklight(bool bOn)
{
    digitalWrite(LCD_LED, bOn ? LOW : HIGH);
    backlightFlag = bOn;
}
//--------------------------------------------------------------------------------------------------------------------------------
uint16_t SettingsClass::getPowerVoltageAkk(uint16_t pin) // Контроль напряжения питания внутренних источников (аккумуляторов).
{
    for (int i = 0; i < 20; i++)
    {
        voltageAkk.raw += analogRead(pin);
    }
    voltageAkk.raw = voltageAkk.raw/20;

	dimension_array[array_count] = voltageAkk.raw;
	array_count++;
	int val_voltage = 0;
	if (array_count > array_size)                    // проверка заполнения массива первичными данными о уровне напряжения аккумулятора
	{
		array_count = 0;
		array_countMax = true;                        //Разрешить выдавать данные об уровне напряжения аккумулятора
	}

	sum = 0;                                         //

	if (array_countMax)                              // формируем данные об уровне напряжения аккумулятора
	{
		for (int i = 0; i < array_size; i++)
		{
			sum += dimension_array[i];
		}
		val_voltage = sum / array_size;
	}
	else
	{
		for (int i = 0; i < array_count; i++)       //формируем первичные (заполняем массив) данные об уровне напряжения аккумулятора
		{
			sum += dimension_array[array_count - 1];
		}
		val_voltage = sum / array_count;
	}
 /*   DBG("voltage Bat - ");
    DBGLN(val_voltage);*/

    voltageAkk.voltage_Akk = map(val_voltage, 1060, 2150, 10, 230);
	if (voltageAkk.voltage_Akk > 230) voltageAkk.voltage_Akk = 230;  // Напряжение питания  

    return voltageAkk.voltage_Akk;
}

//--------------------------------------------------------------------------------------------------------------------------------
bool SettingsClass::getTouch() // Чтение нажатия на экран
{
    uint16_t touch_x, touch_y;
   
    bool touch = _URTouch->getTouch(&touch_x, &touch_y);
    return touch;
}

//--------------------------------------------------------------------------------------------------------------------------------------
uint8_t SettingsClass::read8(uint16_t address, uint8_t defaultVal)
{
   
    uint8_t curVal = mem.read(address);
    if (curVal == 0xFF)
        curVal = defaultVal;

    return curVal;
}

//--------------------------------------------------------------------------------------------------------------------------------------
uint16_t SettingsClass::read16(uint16_t address, uint16_t defaultVal)
{
    uint16_t val = 0;
    byte* b = (byte*)&val;

    for (byte i = 0; i < 2; i++)
        *b++ = mem.read(address + i);

    if (val == 0xFFFF)
        val = defaultVal;

    return val;
}

//--------------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::write8(uint16_t address, uint8_t val)
{
    byte* b = (byte*)&val;
    mem.write(address, *b);
}


//--------------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::write16(uint16_t address, uint16_t val)
{
    byte* b = (byte*)&val;

    for (byte i = 0; i < 2; i++)
        mem.write(address + i, *b++);

}
//--------------------------------------------------------------------------------------------------------------------------------------
unsigned long SettingsClass::read32(uint16_t address, unsigned long defaultVal)
{
    unsigned long val = 0;
    byte* b = (byte*)&val;

    for (byte i = 0; i < 4; i++)
        *b++ = mem.read(address + i);

    if (val == 0xFFFFFFFF)
        val = defaultVal;

    return val;
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::write32(uint16_t address, unsigned long val)
{
    byte* b = (byte*)&val;

    for (byte i = 0; i < 4; i++)
        mem.write(address + i, *b++);
}
//--------------------------------------------------------------------------------------------------------------------------------------
String SettingsClass::readString(uint16_t address, byte maxlength)
{
    String result;
    Serial.println("readString ..");
    for (byte i = 0; i < maxlength; i++)
    {
        byte b = mem.read(address++);
        if (b == 0/*'\0'*/)
        {
            Serial.print("break ..");
            Serial.println((char)b, HEX);
            Serial.print("address ..");
            Serial.println(address);
            break;
        }
 
        result += (char)b;
 		Serial.print((char)b);

    }
 
	//Serial.println(result);
    Serial.println();
    return result;
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::writeString(uint16_t address, const String& v, byte maxlength)
{
    byte val = v.length();
    Serial.println("writeString ..");
    for (byte i = 0; i < maxlength; i++)
    {
        if (i >= v.length())
            break;

		mem.write(address++, v[i]);
		Serial.print(v[i]);
    }
    // пишем завершающий ноль
	mem.write(address + v.length()+1, 0/*'\0'*/);
 /*   Serial.print("address ..");
    Serial.println(address);*/
    Serial.println();
}



//--------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::setup()
{

	// настраиваем "подхват питания"
    pinMode(POWER_ON_IN, INPUT);
    digitalWrite(POWER_ON_IN, HIGH);         // Подключаем к кнопке включения питания подтягиваючий резистор 
    pinMode(POWER_ON_OUT, OUTPUT);
	pinMode(POWER_BATTERY, INPUT);
	pinMode(LCD_LED, OUTPUT);                // Подсветка дисплея
    digitalWrite(LCD_LED, HIGH);             // 

    pinMode(TOUCH_IRQ, INPUT);               // pin индикации нажатия тачскрина
    digitalWrite(TOUCH_IRQ, HIGH);           // Подключаем к подтягиваючий резистор 

    pinMode(V_THRESHOLD1, INPUT);
    pinMode(V_THRESHOLD2, INPUT);
    pinMode(V_THRESHOLD3, INPUT);

	
    // проверяем тип питания
    checkPower();

    
       
    powerButton.begin(POWER_ON_IN, true, ret_interval);



}
//--------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::update()
{

    TFTMenu* menuManager;
    powerButton.update();
    // isClicked
    if (powerButton.isRetention())
    {
        DBGLN(F("POWER KEY DETECTED, TURN POWER OFF!!!"));
        Vector<const char*> lines;
        lines.push_back("Устройство");
        lines.push_back("готово");
        lines.push_back("к отключению.");

        //MessageBox->halt("СООБЩЕНИЕ", lines, NULL);
        MessageBox->halt("СООБЩЕНИЕ", lines, true, true);
        DBGLN(F("POWER OFF!!!"));

#ifdef USE_BUZZER
        Buzzer.buzz();
#endif
        Settings.turnPowerOff();
    }

    if (powerButton.isDoubleClicked())// дважды нажать кнопку для вызова калибровки тача
    {
#ifdef USE_BUZZER
        Buzzer.buzz();
        delay(200);
        Buzzer.buzz();
        delay(200);
        Buzzer.buzz();
#endif

        TFTScreen->switchToScreen("TOUCH_CALIBRATION"); // экран калибровки тача
    }

    static uint32_t tmr = millis();
    if (millis() - tmr > 1000)
    {
        int Power5 = Settings.getPowerVoltageAkk(POWER_BATTERY);
        tmr = millis();
    }

    
#ifdef USE_TFT_MODULE


    int outputValue1 = 100;// analogRead(V_THRESHOLD1);      // уровень порога1
    int outputValue2 = 200;// analogRead(V_THRESHOLD2);      // уровень порога2
    int outputValue3 = 300; //analogRead(V_THRESHOLD3);      // уровень порога3
 
        ValueThreshold1 = map(outputValue1, 0, 4095, ROWS_Count * ROW_Height, 0);
        ValueThreshold2 = map(outputValue2, 0, 4095, ROWS_Count * ROW_Height, 0);
        ValueThreshold3 = map(outputValue3, 0, 4095, ROWS_Count * ROW_Height, 0);

#endif 

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t SettingsClass::getCurrentCoeff()
{
    return currentCoeff;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::setCurrentCoeff(uint32_t val)
{
    currentCoeff = val;
    write32(CURRENT_COEFF_STORE_ADDRESS, currentCoeff);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t SettingsClass::getRSSILowBorder()
{
    return  RSSILowBorder;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t SettingsClass::getRSSIHighBorder()
{
    return  RSSIHighBorder;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::setRSSILowBorder(uint32_t val)
{
    RSSILowBorder = val;
    adcSampler.setLowBorder(val);

    write32(RSSI_LOW_BORDER_STORE_ADDRESS, RSSILowBorder);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::setRSSIHighBorder(uint32_t val)
{
    RSSIHighBorder = val;
    adcSampler.setHighBorder(val);

    write32(RSSI_HIGH_BORDER_STORE_ADDRESS, RSSIHighBorder);

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

////--------------------------------------------------------------------------------------------------------------------------------
//uint16_t SettingsClass::GetThreshold1()           // Получить время отключения прибора при отсутсвии активности
//{
// 
//    return ValueThreshold1;
//}
//
////--------------------------------------------------------------------------------------------------------------------------------
//uint16_t SettingsClass::GetThreshold2()           // Получить время отключения прибора при отсутсвии активности
//{
//    return ValueThreshold2;
//}
//
////--------------------------------------------------------------------------------------------------------------------------------
//uint16_t SettingsClass::GetThreshold3()           // Получить время отключения прибора при отсутсвии активности
//{
//    return ValueThreshold3;
//}
////--------------------------------------------------------------------------------------------------------------------------------
//

//--------------------------------------------------------------------------------------------------------------------------------
