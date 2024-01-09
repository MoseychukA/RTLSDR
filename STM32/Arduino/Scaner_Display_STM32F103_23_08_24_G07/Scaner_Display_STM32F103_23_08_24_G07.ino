/*
 Программа сканера радиосигналов

 Применяется микроконтроллер STM32F103CBT6

 Среда программирования Visual Studio 2019 с надстройкой vMicro.
 Настройки среды программирования:
 http://dan.drown.org/stm32duino/package_STM32duino_index.json
Generic STM32F1 series
STM32 board groups (Board to be selected from Tools submenu 'Board part number\) v2.6.0
  
 Версия печатной платы "Scaner_Display23_06_28_01R1"
*/

#include <stdio.h>                // define I/O functions
#include <Arduino.h>              // define I/O functions
#include "SPI.h"
#include <TFT_eSPI.h>             // Поддержка TFT дисплея 
#include <SD.h>                   // Поддержка SD карты
#include "CONFIG.h"               // Основные настройки программы
#include "Settings.h"             //  
#include "CoreButton.h"           //
#include "CoreCommandBuffer.h"    // обработчик входящих по UART команд
#include <Wire.h>                 //
#include "AT24CX.h"               // Поддержка энергонезависимой памяти
#include "ADCSampler.h"
#include "Drawing.h"

#ifdef USE_BUZZER
#include "Buzzer.h"               
#endif

#ifdef USE_TFT_MODULE
#include "TFTModule.h"
#endif


#ifdef USE_TFT_MODULE
TFTModule tftModule;

#endif

//--------------------------------------------------------------------------------------------------------------------------------
bool canCallYield = false;
//--------------------------------------------------------------------------------------------------------------------------------
bool lcd_ON = false;
uint32_t screenIdleTimer = 0;
uint32_t backlightTimer = 0;
uint32_t powerOffTimer = 0;
bool power_ON = false;

//--------------------------------------------------------------------------------------------------------------------------------
void screenAction(AbstractTFTScreen* screen)
{
    // какое-то действие на экране произошло.
    // тут просто сбрасываем таймер ничегонеделанья.
    screenIdleTimer = millis();           // Таймер переключения на главный экран
    backlightTimer = millis();            // Таймер отключения подсветки дисплея
	powerOffTimer = millis();             // Таймер отключения питания прибора
}
//--------------------------------------------------------------------------------------------------------------------------------
void batteryPowerOn()                     // Включение питания от аккумулятора
{
    int i = 0;
    int time_i = 20;                      // время нажатия на кнопку включения питания.
    do
    {
        delay(100);         
        if (!digitalRead(POWER_ON_IN))
        {
            i++;

            power_ON = true;
        }
        else
        {
            power_ON = false;
            break;
        }

    } while (i < time_i);

}

//--------------------------------------------------------------------------------------------------------------------------------

//HardwareSerial SerialRX485(PB11, PB10);

#define Serial Serial1

void setup() 
{
  canCallYield = false;

  // поднимаем первый UART
  SerialDEBUG.begin(Serial_SPEED);
  while (!SerialDEBUG && millis() < 1000);

  delay(1000);
 /* DBGLN("SerialRX485");
  SerialRX485.begin(Serial_SPEED);
  while (!SerialRX485 && millis() < 500);

  delay(1000);*/
 
  Wire.begin();

  Settings.setup();                     // настраиваем хранилище в EEPROM. Настраиваем кнопку управления питанием
 
#ifdef USE_BUZZER
  Buzzer.begin();
 #endif

  if (Settings.getPowerType() == batteryPower)
  {
     batteryPowerOn();
     if (power_ON == true)
     {
         pinMode(POWER_ON_OUT, OUTPUT);
         digitalWrite(POWER_ON_OUT, HIGH);
#ifdef USE_BUZZER
         Buzzer.buzz();
         delay(100);
         Buzzer.buzz();
         delay(100);
         Buzzer.buzz();
#endif
      }
  }
  else
  {
	#ifdef USE_BUZZER
		// пискнем при старте, если есть баззер
		Buzzer.buzz();
	#endif
  }


  
#ifdef USE_TFT_MODULE 
 tftModule.Setup();
#endif
 //DBGLN("tftModule.Setup()");
 screenIdleTimer = millis();

 TFTScreen->onAction(screenAction);  // 


// поднимаем АЦП
#ifndef _ADC_OFF

 adcSampler.setLowBorder(Settings.getRSSILowBorder());
 adcSampler.setHighBorder(Settings.getRSSIHighBorder());

 adcSampler.begin();
#endif  

 //scanI2C();  // Поиск устройств на шине I2C
 
 // Печатаем в SerialDEBUG готовность
 DBGLN("\nREADY");
 

 screenIdleTimer = millis();         // Таймер переключения на главный экран
 backlightTimer  = millis();         // Таймер отключения подсветки дисплея
 powerOffTimer   = millis();         // Таймер отключения питания прибора

  // выводим в UART версию прошивки
 //CommandHandler.getVER(&SerialDEBUG);
 
 canCallYield = true;

}


//--------------------------------------------------------------------------------------------------------------------------------
void loop()
{

	Settings.update();                    // Проверяем состояние кнопки питания

	lcd_ON = Settings.isBacklightOn();
	int time_LCD_Led = Settings.GetTimeLedLCD();
	int time_PowerOff = Settings.GetTimePowerOff();
	#ifdef USE_TFT_MODULE
		tftModule.Update();
	#endif 

	if (Settings.getPowerType() == powerViaUSB)
	{
		screenIdleTimer = millis();
		Settings.displayBacklight(true);
	}
	else
	{
		if (lcd_ON)
		{

			if (millis() - screenIdleTimer >= RESET_TO_MAIN_SCREEN_DELAY) // через XX секунд ничегонеделанья переключаемся на главный экран
			{
				AbstractTFTScreen* activeScreen = TFTScreen->getActiveScreen();
				if (activeScreen != TouchCalibrationScreen) // пока идёт калибровка тача - переключаться никуда нельзя
				{
					if (activeScreen != MainScreen)
						TFTScreen->switchToScreen(MainScreen);
				}

				screenIdleTimer = millis();
			}

			// При питании от внутреннего источника, отключать подсветку дисплея через XX минут при отсутствии активности на кнопках
			if (Settings.getPowerType() == batteryPower)
			{
				if (millis() - backlightTimer > (time_LCD_Led * 1000))
				{
					// backlightTimer = millis();
					TFTScreen->resetIdleTimer();
					Buzzer.buzz();
					Settings.displayBacklight(false);
				}
			}
		}
		else if (Settings.getPowerType() == batteryPower)
		{

			if (Settings.getTouch() == true)
			{
				Buzzer.buzz();
				Settings.displayBacklight(true); // включаем подсветку
				TFTScreen->resetIdleTimer();

			/*	while (Settings.getTouch() == true)
				{
					yield();
				}
				delay(1000);*/

			}
		}

		// При питании от внутреннего источника, отключать прибор через XX минут при отсутствии активности на кнопках
		if (Settings.getPowerType() == batteryPower)
		{
			if (millis() - powerOffTimer > (time_PowerOff * 1000))
			{
				//powerOffTimer = millis();  // Таймер отключения питания прибора
	#ifdef USE_BUZZER
				Buzzer.buzz();
				delay(200);
				Buzzer.buzz();
				delay(200);
				Buzzer.buzz();
				delay(200);
				Buzzer.buzz();
				delay(200);
				Buzzer.buzz();
				delay(400);
	#endif
				Settings.turnPowerOff();
			}
		}
	}


#ifdef _COM_COMMANDS_OFF
    // обрабатываем входящие команды
  //  CommandHandler.handleCommands();

#endif // _COM_COMMANDS_OFF

}

//--------------------------------------------------------------------------------------------------------------------------------
/*
void yield()
{
 
  if(!canCallYield)
    return;
    
// отсюда можно добавлять любой сторонний код, который надо вызывать, когда МК чем-то долго занят (например, чтобы успокоить watchdog)


// до сюда можно добавлять любой сторонний код


}
*/
//--------------------------------------------------------------------------------------------------------------------------------
void scanI2C()
{
	byte error, address;
	int nDevices;

	Serial.println("Scanning...");
	delay(50);
	nDevices = 0;
	for (address = 1; address < 128; address++) {
		// The i2c_scanner uses the return value of
		// the Write.endTransmisstion to see if
		// a device did acknowledge to the address.
		delay(50);
		Serial.print("\naddress ");
		Serial.print(address);
		Serial.print(" ");
		Wire.beginTransmission(address);
		error = Wire.endTransmission();

		if (error == 0) {
			Serial.print("I2C device found at address 0x");
			if (address < 16)
				Serial.print("0");
			Serial.print(address, HEX);

			nDevices++;
		}
		else if (error == 4) {
			Serial.print("Unknown error at address 0x");
			if (address < 16)
				Serial.print("0");
			Serial.println(address, HEX);
		}

	}
	if (nDevices == 0)
		Serial.println("No I2C devices found");
	else
		Serial.println("done");

	delay(2000);           // wait 5 seconds for next scan
}
//--------------------------------------------------------------------------------------------------------------------------------
