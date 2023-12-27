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
#include "DelayedEvents.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// подключаем наши экраны
#include "Screen1.h"              // Главный экран
#include "Screen3.h"              //
#include "Screen4.h"              // Вызов меню установки времени и даты
#include "Screen5.h"              // Вызов установки времени
#include "Screen6.h"              // Вызов установки даты
#include "Buttons.h"              // наши железные кнопки
#include "Feedback.h"             // обратная связь (диоды и прочее)
#include "Settings.h"
#include "CoreCommandBuffer.h"
//#include <Wire.h>

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
  delay(1000);
  Serial.println(F("Start init!"));

  String ver_soft = __FILE__;
  int val_srt = ver_soft.lastIndexOf('\\');
  ver_soft.remove(0, val_srt + 1);
  val_srt = ver_soft.lastIndexOf('.');
  ver_soft.remove(val_srt);
  Serial.println(ver_soft);

 
#ifndef _RTC_OFF  
  DBGLN(F("Init RTC..."));
  RealtimeClock.begin(); 
 // RealtimeClock.setTime(0,1,11,1,7,2,2018);
  DBGLN(F("RTC inited."));
#endif // #ifndef _RTC_OFF

  pinMode(PC6, OUTPUT);                       // Выход на АСУ ТП №1
  digitalWrite(PC6, LOW);         // Выход на АСУ ТП №1

 
  DBGLN(F("Init settings..."));
  Settings.begin();
  DBGLN(F("Settings inited."));

  Settings.setVer(ver_soft);

  DBGLN(F("Init screen..."));
  Screen.setup();

 DBGLN(F("Add screen1...")); 
  Screen.addScreen(Screen1::create());           // первый экран покажется по умолчанию


#ifndef _SCREEN_3_OFF
  DBGLN(F("Add screen3..."));
  // добавляем третий экран. Переход в меню настройки
  Screen.addScreen(Screen3::create());
#endif // !_SCREEN_3_OFF
#ifndef _SCREEN_4_OFF
  DBGLN(F("Add screen4..."));
  // добавляем четвертый экран. Меню установки даты и времени
  Screen.addScreen(Screen4::create());
#endif // !_SCREEN_4_OFF

#ifndef _SCREEN_5_OFF
  DBGLN(F("Add screen5..."));
  // добавляем 5 экран. Установка времени
  Screen.addScreen(Screen5::create());
#endif // !_SCREEN_5_OFF

#ifndef _SCREEN_6_OFF
  DBGLN(F("Add screen6..."));
  // добавляем 6 экран. Установка даты
  Screen.addScreen(Screen6::create());
#endif // !_SCREEN_6_OFF

  screenIdleTimer = millis();
  Screen.onAction(screenAction);

  // настраиваем обратную связь (информационные диоды и пр.)
  Feedback.begin();

  // переключаемся на первый экран
  Screen.switchToScreen("Main");

  // настраиваем железные кнопки
  Buttons.begin();

  DBGLN(F("Inited."));

  CommandHandler.getVER(&Serial);

  setupDone = true;
  Serial.println(F("Done!"));
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

