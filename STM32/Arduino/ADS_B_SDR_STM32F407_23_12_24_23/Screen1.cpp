//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "Screen1.h"
#include "DS3231.h"
//#include "ConfigPin.h"
#include "CONFIG.h"
#include "Settings.h"
#include "Utils.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Screen1* mainScreen = NULL;
extern "C" char* sbrk(int i);


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Screen1::Screen1() : AbstractTFTScreen("Main")
{
  oldsecond = 0;
  mainScreen = this;
 
  last3V3Voltage = last5Vvoltage = last200Vvoltage = -1;
 
}


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Screen1::onDeactivate()
{

  last3V3Voltage = last5Vvoltage = last200Vvoltage = -1;


//  DBGLN(F("MainScreen::onDeactivate()"));
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Screen1::onActivate()
{

  //DBGLN(F("MainScreen::onActivate()"));
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Screen1::doSetup(TFTMenu* menu)
{
#ifndef _DISABLE_MAIN_SCREEN_BUTTONS
	screenButtons->setSymbolFont(VARIOUS_SYMBOLS_32x32);
	// тут настраиваемся, например, можем добавлять кнопки
	screenButtons->addButton(5, 200, 150, 25, "НАСТРОЙКИ");
    screenButtons->addButton(165, 200, 150, 25, "ДАТА ВРЕМЯ");
#endif // !_DISABLE_MAIN_SCREEN_BUTTONS


}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Screen1::drawTime(TFTMenu* menu)
{

#ifndef _DISABLE_DRAW_TIME
    DS3231Time tm = RealtimeClock.getTime();
    if (oldsecond != tm.second)
    {
      TFT_Class* dc = menu->getDC();
      dc->setFreeFont(TFT_SMALL_FONT);
      
        oldsecond = tm.second;

      // получаем компоненты даты в виде строк
      String strDate = RealtimeClock.getDateStr(tm);
      String strTime = RealtimeClock.getTimeStr(tm);
  
      // печатаем их
      menu->print(strDate.c_str(), 5, 1);
      menu->print(strTime.c_str(), 65, 1);
  
#ifndef _DISABLE_DRAW_RAM_ON_SCREEN
      
      String str = "RAM: ";
      str += getFreeMemory();      
      Screen.print(str.c_str(), 135,1);
#endif // !_DISABLE_DRAW_RAM_ON_SCREEN
      dc->drawLine(0, 14, 319, 14, WHITE); // WHITE
    }

 

#endif // !_DISABLE_DRAW_TIME

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Screen1::doUpdate(TFTMenu* menu)
{


  drawTime(menu);

}


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Screen1::doDraw(TFTMenu* menu)
{
  
  drawTime(menu);

#ifndef _DISABLE_DRAW_SOFTWARE_VERSION

  // рисуем версию ПО
  TFT_Class* dc = menu->getDC();
  dc->setFreeFont(TFT_SMALL_FONT);

  uint16_t w = dc->width();

  String str = "ver. ";
  str += Settings.getVer(); //SOFTWARE_VERSION; 

  int strW = menu->getRusPrinter()->textWidth(str.c_str());

  int top = 228;
  int left = w - strW - 3;

  menu->print(str.c_str(),left,top);

  str = "STM32F7 USB FS RTL-SDR Host";
  strW = menu->getRusPrinter()->textWidth(str.c_str());
  top = 20;
  left = w - strW - (strW/2)-5;
  menu->print(str.c_str(), left, top);
     
#endif // !_DISABLE_DRAW_SOFTWARE_VERSION

    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Screen1::onButtonPressed(TFTMenu* menu, int pressedButton)
{
#ifndef _DISABLE_MAIN_SCREEN_BUTTONS
  // обработчик нажатия на кнопку. Номера кнопок начинаются с 0 и идут в том порядке, в котором мы их добавляли
	if (pressedButton == 0)
	{
		menu->switchToScreen("Settings"); // переключаемся на экран работы с SD
	}
	else if (pressedButton == 1)
	{
		menu->switchToScreen("SCREEN4"); // переключаемся на третий экран
	}
#endif // !_DISABLE_MAIN_SCREEN_BUTTONS
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int Screen1::getFreeMemory()
{
    char top = 't';
    return &top - reinterpret_cast<char*>(sbrk(0));

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
