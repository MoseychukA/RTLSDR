//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
 BLACK F407VG
 Serial commanication: SerialUART1
 Upload method: STLink
 */
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include "CONFIG.h"
#include "TFTMenu.h"
#include "DS3231.h"               // подключаем часы
#include "ConfigPin.h"
#include "AT24CX.h"
#include "InterruptHandler.h"
#include "DelayedEvents.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// подключаем наши экраны
#include "Screen1.h"              // Главный экран

#include "Screen3.h"              //

#include "InterruptScreen.h"      // экран с графиком прерывания
#include "Buttons.h"              // наши железные кнопки
#include "Feedback.h"             // обратная связь (диоды и прочее)
#include "Settings.h"
#include "CoreCommandBuffer.h"
#include <Wire.h>

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TwoWire Wire1 = TwoWire(I2C2, PB11, PB10); // второй I2C
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t screenIdleTimer = 0;
bool setupDone = false;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void screenAction(AbstractTFTScreen* screen)
{
	// какое-то действие на экране произошло.
	// тут просто сбрасываем таймер ничегонеделанья.
	screenIdleTimer = millis();
}


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void setup()
{
  Serial.begin(SERIAL_SPEED);
  while(!Serial && millis() < 2000);



  DBGLN(F("Init I2C..."));  
  Wire1.begin();  
  DBGLN(F("I2C inited."));

#ifndef _RTC_OFF  
  DBGLN(F("Init RTC..."));
  RealtimeClock.begin(); 
 // RealtimeClock.setTime(0,1,11,1,7,2,2018);
  DBGLN(F("RTC inited."));
#endif // #ifndef _RTC_OFF


  ConfigPin::setup();

  DBGLN(F("Init settings..."));
  Settings.begin();
  DBGLN(F("Settings inited."));


  DBGLN(F("Init screen..."));
  Screen.setup();

 DBGLN(F("Add screen1...")); 
  Screen.addScreen(Screen1::create());           // первый экран покажется по умолчанию


#ifndef _SCREEN_3_OFF
  DBGLN(F("Add screen3..."));
  // добавляем третий экран. Переход в меню настройки
  Screen.addScreen(Screen3::create());
#endif // !_SCREEN_3_OFF



  DBGLN(F("Add interrupt screen..."));
  // добавляем экран с графиком прерываний
  Screen.addScreen(InterruptScreen::create());  
  
  screenIdleTimer = millis();
  Screen.onAction(screenAction);

  // настраиваем обратную связь (информационные диоды и пр.)
  Feedback.begin();


  // переключаемся на первый экран
  Screen.switchToScreen("Main");


  // настраиваем железные кнопки
  Buttons.begin();

// поднимаем АЦП
#ifndef _ADC_OFF

  adcSampler.setLowBorder(Settings.getTransformerLowBorder());
  adcSampler.setHighBorder(Settings.getTransformerHighBorder());
  
  adcSampler.begin();  
#endif  


  // поднимаем наши прерывания
  InterruptHandler.begin();


  DBGLN(F("Inited."));

  CommandHandler.getVER(&Serial);

  setupDone = true;

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void loop() 
{
 

  #ifndef _DELAYED_EVENT_OFF
    CoreDelayedEvent.update();
  #endif // _DELAYED_EVENT_OFF
  
 
  Settings.update();
  
  // обновляем кнопки
  Buttons.update();

  Screen.update();

  InterruptHandler.update();

  // проверяем, какой экран активен. Если активен главный экран - сбрасываем таймер ожидания. Иначе - проверяем, не истекло ли время ничегонеделанья.
  AbstractTFTScreen* activeScreen = Screen.getActiveScreen();
  if(activeScreen == mainScreen)
  {
    screenIdleTimer = millis();
  }
  else
  {
      if(millis() - screenIdleTimer > RESET_TO_MAIN_SCREEN_DELAY)
      {
		 // DBGLN(F("ДОЛГОЕ БЕЗДЕЙСТВИЕ, ПЕРЕКЛЮЧАЕМСЯ НА ГЛАВНЫЙ ЭКРАН!"));
        screenIdleTimer = millis();
        Screen.switchToScreen(mainScreen);
      }
  } // else


#ifndef _COM_COMMANDS_OFF
  // обрабатываем входящие команды
  CommandHandler.handleCommands();
#endif // _COM_COMMANDS_OFF


}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool nestedYield = false;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

