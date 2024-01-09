#include "TFTMenu.h"
#include "ADCSampler.h"

#ifdef USE_TFT_MODULE

#ifdef USE_BUZZER
#include "Buzzer.h"
#endif

#include "Settings.h"
#include "Drawing.h"

Chart chart;
ChartSerie* serie1;
ChartSerie* serie2;
ChartSerie* serie3;

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t redCurrentInfoMax = 0; // макс. данные по сигналу, канал 1 (красный)
uint32_t redCurrentInfoMin = 0; // мин. данные по сигналу, канал 1 (красный)

uint32_t blueCurrentInfoMax = 0; // макс. данные по сигналу, канал 2 (синий)
uint32_t blueCurrentInfoMin = 0; // мин. данные по сигналу, канал 2 (синий)

uint32_t yellowCurrentInfoMax = 0; // макс. данные по сигналу, канал 3 (желтый)
uint32_t yellowCurrentInfoMin = 0; // мин. данные по сигналу, канал 3 (желтый)

uint8_t currentNumSamples = 0; // кол-во семплов измерений по току


uint16_t channel1Current = 0; // сигнал канала 1
uint16_t channel2Current = 0; // сигнал канала 2
uint16_t channel3Current = 0; // сигнал канала 3
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void loopADC()
{
#ifndef _ADC_OFF


    if (adcSampler.available())
    {
        int bufferLength = 0;
        uint16_t* cBuf = adcSampler.getADCBuffer(&bufferLength);    // Получить буфер с данными


        static uint16_t countOfPoints = 0;
        static uint16_t* serie1 = NULL;
        static uint16_t* serie2 = NULL;
        static uint16_t* serie3 = NULL;

        uint16_t currentCountOfPoints = bufferLength / NUM_CHANNELS;

        if (currentCountOfPoints != countOfPoints)
        {
            countOfPoints = currentCountOfPoints;

            delete[] serie1;
            delete[] serie2;
            delete[] serie3;

            serie1 = new uint16_t[countOfPoints];
            serie2 = new uint16_t[countOfPoints];
            serie3 = new uint16_t[countOfPoints];

        }


        uint16_t serieWriteIterator = 0;

        uint32_t raw5V = 0;
        uint32_t raw3V3 = 0;

        /*
          Буфер у нас для 3 каналов, индексы:

          0 - Аналоговый вход
          1 - Аналоговый вход
          2 - Аналоговый вход


      */

        for (int i = 0; i < bufferLength; i = i + NUM_CHANNELS, serieWriteIterator++)     // получить результат измерения поканально, с интервалом 3
        {
            serie1[serieWriteIterator] = cBuf[i + 0];        // Данные 1 графика  (красный)
            serie2[serieWriteIterator] = cBuf[i + 1];        // Данные 2 графика  (синий)
            serie3[serieWriteIterator] = cBuf[i + 2];        // Данные 3 графика  (желтый)

            //raw3V3  += cBuf[i + 3];                          // Данные Измерение 3V3


        } // for

      // у нас заполнен массив показаний, можно считать ток.
      // для этого собираем максимальные и минимальные значения по каждому из каналов,
      // и плюсуем их. Как только наберём нужное кол-во семплов - работаем дальше.
        //uint32_t ch1Min = 0xFFFFFFFF, ch1Max = 0, ch2Min = 0xFFFFFFFF, ch2Max = 0, ch3Min = 0xFFFFFFFF, ch3Max = 0;

        //for (uint16_t i = 0; i < countOfPoints; i++)
        //{
        //    ch1Min = min(ch1Min, serie1[i]);
        //    ch2Min = min(ch2Min, serie2[i]);
        //    ch3Min = min(ch3Min, serie3[i]);

        //    ch1Max = max(ch1Max, serie1[i]);
        //    ch2Max = max(ch2Max, serie2[i]);
        //    ch3Max = max(ch3Max, serie3[i]);

        //} // for

        //if (ch1Min == 0xFFFFFFFF)
        //{
        //    ch1Min = ch1Max;
        //}

        //if (ch2Min == 0xFFFFFFFF)
        //{
        //    ch2Min = ch2Max;
        //}

        //if (ch3Min == 0xFFFFFFFF)
        //{
        //    ch3Min = ch3Max;
        //}

        //// плюсуем полученные значения в накопительную часть
        //redCurrentInfoMin += ch1Min;
        //blueCurrentInfoMin += ch2Min;
        //yellowCurrentInfoMin += ch3Min;

        //redCurrentInfoMax += ch1Max;
        //blueCurrentInfoMax += ch2Max;
        //yellowCurrentInfoMax += ch3Max;

        // проверяем, собрали ли нужное кол-во семплов?
        currentNumSamples++;

        if (currentNumSamples >= CURRENT_NUM_SAMPLES)
        {
            // собрали нужное кол-во семплов, можно вычислять ток по каналам.

            // Вычисляем среднее делением на Х. От максимального отнимаем минимальное - получаем размах. Это будет величина переменного тока. 
            // Вернее, измеренное напряжение, которое мы потом преобразуем в ток из расчета 3 вольта равны 5 амперам.

            uint32_t channel1Avg = redCurrentInfoMax / CURRENT_NUM_SAMPLES - redCurrentInfoMin / CURRENT_NUM_SAMPLES;
            uint32_t channel2Avg = blueCurrentInfoMax / CURRENT_NUM_SAMPLES - blueCurrentInfoMin / CURRENT_NUM_SAMPLES;
            uint32_t channel3Avg = yellowCurrentInfoMax / CURRENT_NUM_SAMPLES - yellowCurrentInfoMin / CURRENT_NUM_SAMPLES;

            // вычислили напряжение, теперь вычисляем ток по формуле: 3В = 5А. Для этого напряжение надо умножить на 5, и разделить на 3

            float currentCoeff = Settings.getCurrentCoeff();
            currentCoeff /= 1000; // у нас в тысячных долях

            channel1Current = (COEFF_1 * channel1Avg) / currentCoeff;
            channel2Current = (COEFF_1 * channel2Avg) / currentCoeff;
            channel3Current = (COEFF_1 * channel3Avg) / currentCoeff;

            // отсекаем минимальный нижний порог
            if (channel1Current <= CURRENT_MIN_TREAT_AS_ZERO)
            {
                channel1Current = 0;
            }

            if (channel2Current <= CURRENT_MIN_TREAT_AS_ZERO)
            {
                channel2Current = 0;
            }

            if (channel3Current <= CURRENT_MIN_TREAT_AS_ZERO)
            {
                channel3Current = 0;
            }


            // не забываем чистить за собой, подготавливая к следующему обновлению
            currentNumSamples = 0;

            redCurrentInfoMin = 0;
            blueCurrentInfoMin = 0;
            yellowCurrentInfoMin = 0;

            redCurrentInfoMax = 0;
            blueCurrentInfoMax = 0;
            yellowCurrentInfoMax = 0;

        } // if



        //raw3V3 /= countOfPoints;
        //raw5V /= countOfPoints;

        //Settings.set3V3RawVoltage(raw3V3);
        //Settings.set5VRawVoltage(raw5V);

        adcSampler.reset();                                  // все данные переданы в ком

      /*  if (MainScreen && MainScreen->isActive())
        {
           MainScreen->requestToDrawChart(serie1, serie2, serie3, countOfPoints);
        }*/

    }

#endif // !_ADC_OFF
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------





//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define FONT_HEIGHT(dc) dc->fontHeight(1)


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int utf8GetCharSize(unsigned char bt) 
{ 
  if (bt < 128) 
  return 1; 
  else if ((bt & 0xE0) == 0xC0) 
  return 2; 
  else if ((bt & 0xF0) == 0xE0) 
  return 3; 
  else if ((bt & 0xF8) == 0xF0) 
  return 4; 
  else if ((bt & 0xFC) == 0xF8) 
  return 5; 
  else if ((bt & 0xFE) == 0xFC) 
  return 6; 

 
  return 1; 
} 
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ButtonPressed(int btn)
{
  if(btn != -1)
  {
    #ifdef USE_BUZZER
    Buzzer.buzz();
    #endif
  }

  TFTScreen->onButtonPressed(btn);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ButtonReleased(int btn)
{
  TFTScreen->onButtonReleased(btn);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawButtonsYield() // вызывается после отрисовки каждой кнопки
{
  yield();
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawScreenCaption(TFTMenu* hal, const String& str) // рисуем заголовок экрана
{

  TFT_Class* dc = hal->getDC();
  
  if(!dc)
  {
    return;
  }
  
  TFTRus* rusPrinter = hal->getRusPrinter();
  
  int screenWidth = dc->width();
  
  dc->setFreeFont(TFT_FONT);
  
  int fontHeight = FONT_HEIGHT(dc);
  int top = 10;

  // подложка под заголовок
  dc->fillRect(0, 0, screenWidth, top*2 + fontHeight, TFT_NAVY);
   
  int left = (screenWidth - rusPrinter->textWidth(str.c_str()))/2;

  rusPrinter->print(str.c_str(),left,top, TFT_NAVY, TFT_WHITE);    
  
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawValueInBox(TFTInfoBox* box, const String& strVal, FONTTYPE font)
{
  TFT_Class* dc = TFTScreen->getDC();
  
  if(!dc)
  {
    return;
  }
  
  TFTRus* rusPrinter = TFTScreen->getRusPrinter();
  
  TFTInfoBoxContentRect rc =  box->getContentRect(TFTScreen);
  dc->fillRect(rc.x,rc.y,rc.w,rc.h, INFO_BOX_BACK_COLOR);
  yield();

  dc->setFreeFont(font);

  
  int fontHeight = FONT_HEIGHT(dc);
  int strLen = rusPrinter->textWidth(strVal.c_str());

  int leftPos = rc.x + (rc.w - strLen)/2;
  int topPos = rc.y + (rc.h - fontHeight)/2;
  rusPrinter->print(strVal.c_str(),leftPos,topPos,INFO_BOX_BACK_COLOR,SENSOR_BOX_FONT_COLOR);
  yield();
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawValueInBox(TFTInfoBox* box, int val)
{
  return drawValueInBox(box,String(val));
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawValueInBox(TFTInfoBox* box, int16_t val)
{
  return drawValueInBox(box,String(val));
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawValueInBox(TFTInfoBox* box, uint16_t val)
{
  return drawValueInBox(box,String(val));
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawValueInBox(TFTInfoBox* box, int8_t val)
{
  return drawValueInBox(box,String(val));
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawValueInBox(TFTInfoBox* box, uint8_t val)
{
  return drawValueInBox(box,String(val));
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawValueInBox(TFTInfoBox* box, uint32_t val)
{
  return drawValueInBox(box,String(val));
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// TFTInfoBox
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TFTInfoBox::TFTInfoBox(const char* caption, int width, int height, int x, int y, int cxo)
{
  boxCaption = caption;
  boxWidth = width;
  boxHeight = height;
  posX = x;
  posY = y;
  captionXOffset = cxo;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TFTInfoBox::~TFTInfoBox()
{
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TFTInfoBox::drawCaption(TFTMenu* menuManager, const char* caption)
{
  TFT_Class* dc = menuManager->getDC();
  if(!dc)
  {
    return;
  }  
  
  dc->setFreeFont(TFT_FONT);
  
  menuManager->getRusPrinter()->print(caption,posX+captionXOffset,posY,TFT_BACK_COLOR,INFO_BOX_CAPTION_COLOR);  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TFTInfoBox::draw(TFTMenu* menuManager)
{
  drawCaption(menuManager,boxCaption);
  
  int curTop = posY;

  TFT_Class* dc = menuManager->getDC();
  if(!dc)
  {
    return;
  }

  dc->setFreeFont(TFT_FONT);
    
  int fontHeight = FONT_HEIGHT(dc);
  
  curTop += fontHeight + INFO_BOX_CONTENT_PADDING;

  dc->fillRoundRect(posX, curTop, boxWidth, (boxHeight - fontHeight - INFO_BOX_CONTENT_PADDING),2,INFO_BOX_BACK_COLOR);

  yield();

  dc->drawRoundRect(posX, curTop, boxWidth, (boxHeight - fontHeight - INFO_BOX_CONTENT_PADDING),2,INFO_BOX_BORDER_COLOR);

  yield();
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TFTInfoBoxContentRect TFTInfoBox::getContentRect(TFTMenu* menuManager)
{
    TFTInfoBoxContentRect result;
    TFT_Class* dc = menuManager->getDC();
	
	if(!dc)
	{
		return result;
	}	

    dc->setFreeFont(TFT_FONT);
    
    int fontHeight = FONT_HEIGHT(dc);

    result.x = posX + INFO_BOX_CONTENT_PADDING;
    result.y = posY + fontHeight + INFO_BOX_CONTENT_PADDING*2;

    result.w = boxWidth - INFO_BOX_CONTENT_PADDING*2;
    result.h = boxHeight - (fontHeight + INFO_BOX_CONTENT_PADDING*3);

    return result;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AbstractTFTScreen::AbstractTFTScreen()
{ 
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AbstractTFTScreen::~AbstractTFTScreen()
{ 
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// TFTMenu
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TFTMenu* TFTScreen = NULL;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TFTMenu::TFTMenu()
{
  TFTScreen = this;
  currentScreenIndex = -1;
  flags.isLCDOn = true;
  switchTo = NULL;
  switchToIndex = -1;
  tftDC = NULL;
  tftTouch = NULL;
  on_action = NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TFTMenu::setup()
{
  int rot = 0;
  int dRot = 0;

  
  tftDC = new TFT_eSPI();

	tftDC->init();
	tftDC->setRotation(dRot);
	tftDC->fillScreen(TFT_BACK_COLOR);

	tftDC->setFreeFont(TFT_FONT);

	tftDC->setTextColor(TFT_RED, TFT_BACK_COLOR);


	tftTouch = tftDC;

    delay(200);
    Settings.displayBacklight(true); // включаем подсветку

  TFTCalibrationData data = Settings.GetTftCalibrationData();
  if(data.isValid)
  {
    tftTouch->setTouch(data.points);
  }
  else
  {
    uint16_t dt[5] = {304, 3502, 280, 3507, 0};
    tftTouch->setTouch(dt);
  }

	tftTouch->setRotation(rot);
	tftTouch->begin();


  rusPrint.init(tftDC);

  
  resetIdleTimer();

  //// добавляем служебные экраны

  // окно сообщения
  TFTScreenInfo mbscrif;
    
  //TFTTouchCalibrationScreen
  mbscrif.screen = new TFTTouchCalibrationScreen();
  mbscrif.screen->setup(this);
  mbscrif.screenName = "TOUCH_CALIBRATION";
  screens.push_back(mbscrif);

  //TFTMenuScreen
  mbscrif.screen = new TFTMenuScreen();
  mbscrif.screen->setup(this);
  mbscrif.screenName = "MENU";
  screens.push_back(mbscrif);


  //TFTServiceMenuScreen
  mbscrif.screen = new TFTServiceMenuScreen();
  mbscrif.screen->setup(this);
  mbscrif.screenName = "SERVICE_MENU";
  screens.push_back(mbscrif);

  mbscrif.screen = MessageBoxScreen::create();
  mbscrif.screen->setup(this);
  mbscrif.screenName = "MB";
  screens.push_back(mbscrif);

 
  //TFTSetTimeLedLCDOff
  mbscrif.screen = new TFTSetTimeLedLCDOff();
  mbscrif.screen->setup(this);
  mbscrif.screenName = "SetTimeLedLCDOff";
  screens.push_back(mbscrif);

  //TFTSetTimePowerOff
  mbscrif.screen = new TFTSetTimePowerOff();
  mbscrif.screen->setup(this);
  mbscrif.screenName = "SetTimePowerOff";
  screens.push_back(mbscrif);
  
  
  //TFTSaveMenuScreen
  mbscrif.screen = new TFTSaveMenuScreen();
  mbscrif.screen->setup(this);
  mbscrif.screenName = "SAVE_MENU";
  screens.push_back(mbscrif);


/*
  // клавиатура
  mbscrif.screen = KeyboardScreen::create();
  mbscrif.screen->setup(this);
  mbscrif.screenName = "KBD";
  screens.push_back(mbscrif);
*/
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TFTMenu::onButtonPressed(int button)
{
  if(currentScreenIndex == -1)
    return;

  resetIdleTimer();
  TFTScreenInfo* currentScreenInfo = &(screens[currentScreenIndex]);
  currentScreenInfo->screen->onButtonPressed(this, button);

  if(on_action != NULL)
  {
    on_action(currentScreenInfo->screen);
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TFTMenu::onButtonReleased(int button)
{
  if(currentScreenIndex == -1)
    return;

  resetIdleTimer();
  TFTScreenInfo* currentScreenInfo = &(screens[currentScreenIndex]);
  currentScreenInfo->screen->onButtonReleased(this, button);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TFTMenu::update()
{
  if(!tftDC)
  {
    return;
  }
	
  if(currentScreenIndex == -1 && !switchTo)                         // ни разу не рисовали ещё ничего, исправляемся
  {
     TFTCalibrationData dt = Settings.GetTftCalibrationData();
   /*  
     SerialDEBUG.print("dt.isValid ");
     SerialDEBUG.println(dt.isValid);
     SerialDEBUG.println(dt.points[0]);
     SerialDEBUG.println(dt.points[1]);
     SerialDEBUG.println(dt.points[2]);
     SerialDEBUG.println(dt.points[3]);*/

      if(dt.isValid)
      {
        switchToScreen("MENU");
      }
      else
      {
        switchToScreen("TOUCH_CALIBRATION");
      }
  }

  if(switchTo != NULL)
  {
      tftDC->fillScreen(TFT_BACK_COLOR); // clear screen first      
      yield();
      currentScreenIndex = switchToIndex;
      switchTo->onActivate(this);
      switchTo->update(this);
      yield();
      switchTo->draw(this);
      yield();
      resetIdleTimer(); // сбрасываем таймер ничегонеделанья

      switchTo = NULL;
      switchToIndex = -1;
    return;
  }



  // обновляем текущий экран
  TFTScreenInfo* currentScreenInfo = &(screens[currentScreenIndex]);
  currentScreenInfo->screen->update(this);
  yield();
  
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AbstractTFTScreen* TFTMenu::getScreen(const char* screenName)
{
  for(size_t i=0;i<screens.size();i++)
  {
    TFTScreenInfo* si = &(screens[i]);
    if(!strcmp(si->screenName,screenName))
    {
      return si->screen;
    }
  }

  return NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TFTMenu::switchToScreen(AbstractTFTScreen* to)
{
	if(!tftDC)
	{
		return;
	}
   // переключаемся на запрошенный экран
  for(size_t i=0;i<screens.size();i++)
  {
    TFTScreenInfo* si = &(screens[i]);
    if(si->screen == to)
    {
      switchTo = si->screen;
      switchToIndex = i;
      break;

    }
  } 
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TFTMenu::switchToScreen(const char* screenName)
{
	if(!tftDC)
	{
		return;
	}
  
  // переключаемся на запрошенный экран
  for(size_t i=0;i<screens.size();i++)
  {
    TFTScreenInfo* si = &(screens[i]);
    if(!strcmp(si->screenName,screenName))
    {
      switchTo = si->screen;
      switchToIndex = i;
      break;

    }
  }

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AbstractTFTScreen* TFTMenu::getActiveScreen()
{
  if(currentScreenIndex > -1 && screens.size())
  {
    TFTScreenInfo* currentScreenInfo = &(screens[currentScreenIndex]);
     return (currentScreenInfo->screen);
  }  
  
  return NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TFTMenu::resetIdleTimer()
{
  idleTimer = millis();

  if(currentScreenIndex > -1 && screens.size() && on_action != NULL)
  {
    TFTScreenInfo* currentScreenInfo = &(screens[currentScreenIndex]);
    on_action(currentScreenInfo->screen);
  }
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// TFTTouchCalibrationScreen
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TFTTouchCalibrationScreen* TouchCalibrationScreen = NULL;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TFTTouchCalibrationScreen::TFTTouchCalibrationScreen()
{
	canSwitch = false;
  TouchCalibrationScreen = this;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TFTTouchCalibrationScreen::~TFTTouchCalibrationScreen()
{

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TFTTouchCalibrationScreen::setup(TFTMenu* menuManager)
{

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TFTTouchCalibrationScreen::update(TFTMenu* menuManager)
{
  menuManager->resetIdleTimer();                         // сбрасываем таймер ничегонеделанья, чтобы не переключилось на главный экран
  
	if (canSwitch)
	{
		canSwitch = false;
 		menuManager->switchToScreen("MENU");

	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TFTTouchCalibrationScreen::draw(TFTMenu* menuManager)
{

	TFT_Class* dc = menuManager->getDC();
	TFTRus* rusPrinter = menuManager->getRusPrinter();

	dc->setFreeFont(TFT_SMALL_FONT);
	dc->fillScreen(TFT_BLACK);
	int fontHeight = FONT_HEIGHT(dc);
	int screenWidth = dc->width();
	int screenHeight = dc->height();
	const int v_spacing = 2;

	Vector<const char*> lines;
	lines.push_back("ТРЕБУЕТСЯ КАЛИБРОВКА ТАЧСКРИНА.");
	lines.push_back("");
	lines.push_back("НАЖИМАЙТЕ ПООЧЕРЁДНО НА УГЛЫ.");

	int top = (screenHeight - lines.size()*(fontHeight + v_spacing)) / 2;

	for (size_t i = 0; i < lines.size(); i++)
	{
		int left = (screenWidth - rusPrinter->textWidth(lines[i])) / 2;

		rusPrinter->print(lines[i], left, top, TFT_BLACK, TFT_WHITE);

		top += fontHeight + v_spacing;
	}
	delay(400);
	TFTCalibrationData calData;

	dc->calibrateTouch(calData.points, TFT_WHITE, TFT_BLACK, 30);
	dc->setTouch(calData.points);
	Settings.SetTftCalibrationData(calData);

	dc->setFreeFont(TFT_FONT);
	canSwitch = true;

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// TFTServiceMenuScreen
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 TFTServiceMenuScreen::TFTServiceMenuScreen()
 {
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 TFTServiceMenuScreen::~TFTServiceMenuScreen()
 {
   delete screenButtons;  
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTServiceMenuScreen::onActivate(TFTMenu* menuManager)
 {

 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTServiceMenuScreen::setup(TFTMenu* menuManager)
 {


   TFT_Class* dc = menuManager->getDC();

   if (!dc)
   {
     return;
   }

   screenButtons = new TFT_Buttons_Rus(dc, menuManager->getTouch(), menuManager->getRusPrinter());
   screenButtons->setTextFont(TFT_FONT);
   screenButtons->setSymbolFont(SENSOR_FONT);

   screenButtons->setButtonColors(TFT_BUTTON_COLORS_BLUE);

   int screenWidth = dc->width();
   int screenHeight = dc->height();

  const int v_spacing = 8;
  const int h_spacing = 5;

  // у нас 4 кнопки
  int button_width = screenWidth - h_spacing*2;
  int button_height = (screenHeight - v_spacing*7)/6;
  int left = h_spacing;
  int top = v_spacing;



    ledLCDTimeButton = screenButtons->addButton(left, top, button_width, button_height, CAL_TIME_LCD_OFF);
    
    top += v_spacing + button_height;
    powerOffTimeButton = screenButtons->addButton(left, top, button_width, button_height, CAL_TIME_POWER_OFF);

    top += v_spacing + button_height;

    top += v_spacing + button_height;

  
    top += v_spacing + button_height;

 
    top += v_spacing + button_height;

    backButton  = screenButtons->addButton(left, top, button_width, button_height, WM_BACK_CAPTION);       //Кнопка "< НАЗАД"

 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  void TFTServiceMenuScreen::update(TFTMenu* menuManager)
 {
  int pressed_button = screenButtons->checkButtons(ButtonPressed, ButtonReleased);
  if(pressed_button != -1)
  {
    if(pressed_button == backButton)
    {
      menuManager->switchToScreen("MENU");
    }
    else
    if (pressed_button == ledLCDTimeButton)
    {
        menuManager->switchToScreen("SetTimeLedLCDOff");
    }
    else
    if (pressed_button == powerOffTimeButton)
    {
        menuManager->switchToScreen("SetTimePowerOff");
    }
   
  }

 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTServiceMenuScreen::draw(TFTMenu* menuManager)
 {

   if (screenButtons)
   {
     screenButtons->drawButtons(drawButtonsYield);
   }

 }

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// TFTSetTimeLedLCDOff
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 TFTSetTimeLedLCDOff::TFTSetTimeLedLCDOff()
 {
     stage = 0;
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 TFTSetTimeLedLCDOff::~TFTSetTimeLedLCDOff()
 {
     delete screenButtons;
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSetTimeLedLCDOff::setup(TFTMenu* menuManager)
 {
     TFT_Class* dc = menuManager->getDC();
     screenButtons = new TFT_Buttons_Rus(dc, menuManager->getTouch(), menuManager->getRusPrinter());
     screenButtons->setTextFont(TFT_FONT);
     screenButtons->setSymbolFont(SENSOR_FONT);

     screenButtons->setButtonColors(TFT_BUTTON_COLORS_BLUE);

     TFTRus* rusPrinter = menuManager->getRusPrinter();

     int screenWidth = dc->width();
     int screenHeight = dc->height();

     dc->setFreeFont(TFT_FONT);
     int textFontHeight = FONT_HEIGHT(dc);

     // создаём кнопки клавиатуры
     static const char* captions[] = {
      "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "C", ENTER_CAPTION
     };

     const int v_spacing = 5;
     const int buttons_in_row = 3;
     int button_width = (screenWidth - v_spacing * (1 + buttons_in_row)) / buttons_in_row;
     int button_height = textFontHeight * 2 + v_spacing * 2 + 4;
     int top = button_height + v_spacing * 3;
     int left = v_spacing;

     int row_cntr = 0;
     lastKeyButtonID = 0;
     for (int i = 0; i < sizeof(captions) / sizeof(captions[0]); i++)
     {
         if (row_cntr >= buttons_in_row)
         {
             row_cntr = 0;
             left = v_spacing;
             top += button_height + v_spacing;
         }

         lastKeyButtonID = screenButtons->addButton(left, top, button_width, button_height, captions[i]);
         left += v_spacing + button_width;
         row_cntr++;
     }

     int small_button_width = button_width;

     // добавляем текс-бокс
     textBox = screenButtons->addButton(v_spacing, v_spacing, screenWidth - v_spacing * 2, button_height, "");
     screenButtons->disableButton(textBox);
     screenButtons->setButtonInactiveFontColor(textBox, TFT_FONT_COLOR);

     // добавляем кнопку "Назад"
     button_width = screenWidth - v_spacing * 2;

     left = (screenWidth - button_width) / 2;
     top = (screenHeight - (button_height * 2 + v_spacing * 2));

     top += button_height + v_spacing;
     backButton = screenButtons->addButton(left, top, button_width, button_height, WM_BACK_CAPTION);

 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSetTimeLedLCDOff::update(TFTMenu* menuManager)
 {

     int pressed_button = screenButtons->checkButtons(ButtonPressed, ButtonReleased);
     if (pressed_button != -1)
     {
         if (pressed_button <= lastKeyButtonID) // кнопки клавиатуры
         {
             if (!strcmp(screenButtons->getLabel(pressed_button), ENTER_CAPTION))
             {
                 if (enteredTimeLCD.length() > 0)
                 {
                     // Сохранить 
                     int intVar;
                     intVar = enteredTimeLCD.toInt();
                     Settings.SetTimeLedLCD(intVar);
                 }
                 else
                 {
                     stage = 1;
                     enteredTimeLCD = "";
                     relabelStageMessage(true); // пишем приглашение
                 }
             }
             else if (!strcmp(screenButtons->getLabel(pressed_button), "C"))  // Очистить поле ввода данных
             {
                 enteredTimeLCD = "";
                 relabelStageMessage(true); // пишем приглашение
             }
             else if (enteredTimeLCD.length() < MAX_TIME_LCD_LENGTH) // защита от длинного ввода времени отключения подсветки дисплея
             {
                 enteredTimeLCD += screenButtons->getLabel(pressed_button);  // Отобразить новые данные
                 screenButtons->relabelButton(textBox, enteredTimeLCD.c_str(), true);
             }
         }
         else
         {
             // другие кнопки

             if (pressed_button == backButton)
             {
                 menuManager->switchToScreen("SERVICE_MENU");
             }
         }
     }
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSetTimeLedLCDOff::relabelStageMessage(bool redraw) // Приглашение к вводу новых данных
 {
     const char* message = ">Задержка сек.";
     switch (stage)
     {
     case 0: break;
     case 1: message = "> Введите данные"; break;
     }

     screenButtons->relabelButton(textBox, message, redraw);
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSetTimeLedLCDOff::onActivate(TFTMenu* menuManager)  // Отобразить текущие данные при запуске функции
 {
     enteredTimeLCD = "";
     int time_LCD = Settings.GetTimeLedLCD();
     enteredTimeLCD = String(time_LCD, DEC);
     stage = 0;
     screenButtons->relabelButton(textBox, enteredTimeLCD.c_str(), true);
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSetTimeLedLCDOff::draw(TFTMenu* menuManager)
 {

     // рисуем кнопки
     screenButtons->drawButtons(drawButtonsYield);

 }

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// TFTSetTimePowerOff
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 TFTSetTimePowerOff::TFTSetTimePowerOff()
 {
     stage = 0;
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 TFTSetTimePowerOff::~TFTSetTimePowerOff()
 {
     delete screenButtons;
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSetTimePowerOff::setup(TFTMenu* menuManager)
 {
     TFT_Class* dc = menuManager->getDC();
     screenButtons = new TFT_Buttons_Rus(dc, menuManager->getTouch(), menuManager->getRusPrinter());
     screenButtons->setTextFont(TFT_FONT);
     screenButtons->setSymbolFont(SENSOR_FONT);

     screenButtons->setButtonColors(TFT_BUTTON_COLORS_BLUE);

     TFTRus* rusPrinter = menuManager->getRusPrinter();

     int screenWidth = dc->width();
     int screenHeight = dc->height();

     dc->setFreeFont(TFT_FONT);
     int textFontHeight = FONT_HEIGHT(dc);

     // создаём кнопки клавиатуры
     static const char* captions[] = {
      "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "C", ENTER_CAPTION
     };

     const int v_spacing = 5;
     const int buttons_in_row = 3;
     int button_width = (screenWidth - v_spacing * (1 + buttons_in_row)) / buttons_in_row;
     int button_height = textFontHeight * 2 + v_spacing * 2 + 4;
     int top = button_height + v_spacing * 3;
     int left = v_spacing;

     int row_cntr = 0;
     lastKeyButtonID = 0;
     for (int i = 0; i < sizeof(captions) / sizeof(captions[0]); i++)
     {
         if (row_cntr >= buttons_in_row)
         {
             row_cntr = 0;
             left = v_spacing;
             top += button_height + v_spacing;
         }

         lastKeyButtonID = screenButtons->addButton(left, top, button_width, button_height, captions[i]);
         left += v_spacing + button_width;
         row_cntr++;
     }

     int small_button_width = button_width;

     // добавляем текс-бокс
     textBox = screenButtons->addButton(v_spacing, v_spacing, screenWidth - v_spacing * 2, button_height, "");
     screenButtons->disableButton(textBox);
     screenButtons->setButtonInactiveFontColor(textBox, TFT_FONT_COLOR);

     // добавляем кнопку "Назад"
     button_width = screenWidth - v_spacing * 2;

     left = (screenWidth - button_width) / 2;
     top = (screenHeight - (button_height * 2 + v_spacing * 2));

     top += button_height + v_spacing;
     backButton = screenButtons->addButton(left, top, button_width, button_height, WM_BACK_CAPTION);

 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSetTimePowerOff::update(TFTMenu* menuManager)
 {

     int pressed_button = screenButtons->checkButtons(ButtonPressed, ButtonReleased);
     if (pressed_button != -1)
     {
         if (pressed_button <= lastKeyButtonID) // кнопки клавиатуры
         {
             if (!strcmp(screenButtons->getLabel(pressed_button), ENTER_CAPTION))
             {
                 if (enteredPowerOff.length() > 0)
                 {
                     // Сохранить 
                     int intVar;
                     intVar = enteredPowerOff.toInt();
                     Settings.SetTimePowerOff(intVar);
                 }
                 else
                 {
                     stage = 1;
					 enteredPowerOff = "";
                     relabelStageMessage(true); // пишем приглашение
                 }
             }
             else if (!strcmp(screenButtons->getLabel(pressed_button), "C"))
             {
				 enteredPowerOff = "";
                 relabelStageMessage(true); // пишем приглашение
             }
             else if (enteredPowerOff.length() < MAX_TIME_POWER_LENGTH) // защита от длинного ввода таймера отключения питания
             {
				 enteredPowerOff += screenButtons->getLabel(pressed_button);
                 screenButtons->relabelButton(textBox, enteredPowerOff.c_str(), true);
             }
         }
         else
         {
             // другие кнопки

             if (pressed_button == backButton)
             {
                 menuManager->switchToScreen("SERVICE_MENU");
             }
         }
     }
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSetTimePowerOff::relabelStageMessage(bool redraw)
 {
     const char* message = ">Задержка сек.";
     switch (stage)
     {
     case 0: break;
     case 1: message = "> Введите данные"; break;
     }

     screenButtons->relabelButton(textBox, message, redraw);
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSetTimePowerOff::onActivate(TFTMenu* menuManager)
 {
	 enteredPowerOff = "";
     int PowerOff = Settings.GetTimePowerOff();
	 enteredPowerOff = String(PowerOff, DEC);
     stage = 0;
     screenButtons->relabelButton(textBox, enteredPowerOff.c_str(), true);
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSetTimePowerOff::draw(TFTMenu* menuManager)
 {

     // рисуем кнопки
     screenButtons->drawButtons(drawButtonsYield);

 }


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// TFTSaveMenuScreen
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 TFTSaveMenuScreen::TFTSaveMenuScreen()
 {
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 TFTSaveMenuScreen::~TFTSaveMenuScreen()
 {
     delete screenButtons;
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSaveMenuScreen::onActivate(TFTMenu* menuManager)
 {

 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSaveMenuScreen::setup(TFTMenu* menuManager)
 {


     TFT_Class* dc = menuManager->getDC();

     if (!dc)
     {
         return;
     }

     screenButtons = new TFT_Buttons_Rus(dc, menuManager->getTouch(), menuManager->getRusPrinter());
     screenButtons->setTextFont(TFT_FONT);
     screenButtons->setSymbolFont(SENSOR_FONT);

     screenButtons->setButtonColors(TFT_CHANNELS_BUTTON_COLORS);

     int screenWidth = dc->width();
     int screenHeight = dc->height();

     const int v_spacing = 8;
     const int h_spacing = 5;

     // у нас 4 кнопки
     int button_width = screenWidth - h_spacing * 2;
     int button_height = (screenHeight - v_spacing * 7) / 6;
     int left = h_spacing;
     int top = v_spacing;



     setDataButton = screenButtons->addButton(left, top, button_width, button_height, SET_FREE);       //Кнопка " "
     top += v_spacing + button_height;

     calButton = screenButtons->addButton(left, top, button_width, button_height, SET_FREE);           //Кнопка " "
     top += v_spacing + button_height;

     setAtmButton = screenButtons->addButton(left, top, button_width, button_height, SET_FREE);        //Кнопка " "
     top += v_spacing + button_height;

     timeButton = screenButtons->addButton(left, top, button_width, button_height, SET_FREE);           //Кнопка " "
     top += v_spacing + button_height;

     changePasswordButton = screenButtons->addButton(left, top, button_width, button_height, SET_CONTROLLER_ID); //Кнопка " "
     top += v_spacing + button_height;

     backButton = screenButtons->addButton(left, top, button_width, button_height, WM_BACK_CAPTION);    //Кнопка "< НАЗАД"
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSaveMenuScreen::update(TFTMenu* menuManager)
 {
    int pressed_button = screenButtons->checkButtons(ButtonPressed, ButtonReleased);
    if (pressed_button != -1)
    {
        if (pressed_button == backButton)
        {
            menuManager->switchToScreen("MENU");
        }
        else if (pressed_button == changePasswordButton)
        {
           // menuManager->switchToScreen("VARIANT_PASSWORD");
        }
        else if (pressed_button == timeButton)
        {
          //  menuManager->switchToScreen("PASSWORD_TIME");
        }
        else if (pressed_button == setAtmButton)
        {
           // menuManager->switchToScreen("SET_ATMOSFERA");
        }
        else if (pressed_button == setDataButton)
        {
           // menuManager->switchToScreen("SET_DATE_TIME");
        }
        else if (pressed_button == calButton)
        {
          //  menuManager->switchToScreen("CAL_SETTINGS");
        }
     }
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTSaveMenuScreen::draw(TFTMenu* menuManager)
 {

     if (screenButtons)
     {
         screenButtons->drawButtons(drawButtonsYield);
     }

 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// TFTMenuScreen
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

 TFTMenuScreen* MainScreen = NULL;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 TFTMenuScreen::TFTMenuScreen()
 {
  tickerButton = -1;
  MainScreen = this;
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 TFTMenuScreen::~TFTMenuScreen()
 {
	 delete screenButtons;	

     // станем неактивными, надо выключить подписчика результатов прерываний
    // InterruptHandler.setSubscriber(NULL);

    // last3V3Voltage = last5Vvoltage = last200Vvoltage = -1;

     // прекращаем отрисовку графика
     chart.stopDraw();
     inDrawingChart = false;
     canDrawChart = false;
     isRS485Online = false;

#ifndef _ADC_OFF
     canLoopADC = false;
#endif // !_ADC_OFF

     //  DBGLN(F("MainScreen::onDeactivate()"));

 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------


 void TFTMenuScreen::onActivate(TFTMenu* menuManager)
 {
	 if (!menuManager->getDC())
	 {
		 return;
	 }

    #ifndef _ADC_OFF
         canLoopADC = true;
    #endif // !_ADC_OFF

     oldChannel1Current = oldChannel2Current = oldChannel3Current = 0xFFFF;
     oldCurrentString1 = oldCurrentString2 = oldCurrentString3 = "";

    // isRS485Online = HasRS485Link;
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTMenuScreen::setup(TFTMenu* menuManager)
 {
	 TFT_Class* dc = menuManager->getDC();

	 if (!dc)
	 {
		 return;
	 }

	 screenButtons = new TFT_Buttons_Rus(dc, menuManager->getTouch(), menuManager->getRusPrinter());


	 screenButtons->setTextFont(TFT_FONT);
	 screenButtons->setSymbolFont(SENSOR_FONT);

	// screenButtons->setButtonColors(TFT_CHANNELS_BUTTON_COLORS);
     screenButtons->setButtonColors(TFT_BUTTON_COLORS_BLUE);
  
	 int screenWidth = dc->width();
	 int screenHeight = dc->height();

   const int BUTTON_WIDTH = 220;
   const int BUTTON_HEIGHT = 40;
   const int V_SPACING = 10;
   const int V_SPACING1 = 60;

   int left = 10;/*(screenWidth - BUTTON_WIDTH)/2;*/
   int top = 275;/*(screenHeight - (BUTTON_HEIGHT * 4 + V_SPACING1)) / 2;*/

   menuButton = screenButtons->addButton(left, top, BUTTON_WIDTH, BUTTON_HEIGHT, MENU_CAPTION);     //Кнопка "СЕРВИСНОЕ МЕНЮ" на главном экране

    // ТУТ НАСТРАИВАЕМ НАШ ГРАФИК
    // устанавливаем ему начальные точки отсчёта координат

   chart.setCoords(5, 120);
   // говорим, какое у нас кол-во точек по X и по Y
   chart.setPoints(CHART_POINTS_COUNT, 100);
   // добавляем наши тестовые графики, количеством 1

   serie1 = chart.addSerie({ 255,0,0 });     // первый график - красного цвета
   serie2 = chart.addSerie({ 0,0,255 });     // второй график - голубого цвета
   serie3 = chart.addSerie({ 255,255,0 });   // третий график - желтого цвета
   //DBGLN(F("Screen1::doSetup END"));

   tmr = millis();

 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  void TFTMenuScreen::onButtonPressed(TFTMenu* menuManager, int buttonID)
 {
	 tickerButton = -1;

	 //if (buttonID == dec25PercentsButton || buttonID == inc25PercentsButton || buttonID == dec50PercentsButton
		// || buttonID == inc50PercentsButton || buttonID == dec75PercentsButton || buttonID == inc75PercentsButton
		// || buttonID == dec100PercentsButton || buttonID == inc100PercentsButton)
	 //{
		// tickerButton = buttonID;
		// Ticker.start(this);
	 //}
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTMenuScreen::onButtonReleased(TFTMenu* menuManager, int buttonID)
 {
	 Ticker.stop();
	 tickerButton = -1;
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTMenuScreen::onTick()
 {
    TFTScreen->resetIdleTimer(); // сбрасываем таймер ничегонеделанья, чтобы не переключилось на главный экран
  
	/* if (tickerButton == dec25PercentsButton)
		 inc25Temp(-3);
	else
	if (tickerButton == inc100PercentsButton)
		inc100Temp(3);*/
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  void TFTMenuScreen::update(TFTMenu* menuManager)
 {
	

   static uint32_t tmr = millis();
   if(millis() - tmr > DATA_MEASURE_THRESHOLD)
   {
       drawVoltage(menuManager);
       tmr = millis();
      // DBGLN("\ndrawVoltage");
   }

   drawChart();



    	  
  int pressed_button = screenButtons->checkButtons(ButtonPressed, ButtonReleased);
  if(pressed_button != -1)
  {
    if (pressed_button == menuButton)
    {
        menuManager->switchToScreen("SERVICE_MENU");
    }
  }

 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTMenuScreen::draw(TFTMenu* menuManager)
 {
	 if (!menuManager->getDC())
	 {
		 return;
	 }

	 if (screenButtons)
	 {
		 screenButtons->drawButtons(drawButtonsYield);
	 }

 }


 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 // Контроль внутреннего источника питания (аккумуляторов)
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTMenuScreen::drawVoltage(TFTMenu* menuManager)
 {
	 TFT_Class* dc = menuManager->getDC();
	 if (!dc)
	 {
		 return;
	 }
	 TFTRus* rusPrinter = menuManager->getRusPrinter();


	 int screenWidth = dc->width();
	 int screenHeight = dc->height();

	 dc->setFreeFont(TFT_SMALL_FONT);
	 int textFontHeight = FONT_HEIGHT(dc);

	 String data = SOFTWARE_VERSION;

	 int textFontWidth = dc->textWidth(data);              // Returns pixel width of string in current font
	 uint16_t curX = screenWidth - textFontWidth - 10;     // Координаты вывода 
	 uint16_t curY = 0;// 305;                             // Координаты вывода версии



	 rusPrinter->print(data.c_str(), curX, curY, TFT_BLACK , TFT_WHITE); // Отображаем версию программы

	 dc->setFreeFont(TFT_FONT);

	 VoltageData vData5 = Settings.voltageAkk;     // Контроль источника питания +5.0в

     vData5.voltage_Akk = 180;
    // DBGLN(vData5.voltage_Akk);

	/* if (lastAkkVoltage != vData5.raw)
	 {*/
          lastAkkVoltage = vData5.raw;
     
		 int y_val = 13;
		 int x_val = map(vData5.voltage_Akk, 10, 230, 0, 100);
      //   DBGLN(x_val);



		 if (x_val < 20)
		 {
			 dc->fillRect(5, y_val, vData5.voltage_Akk, 5, TFT_RED);
		 }
		 else if ((x_val >= 20) && (x_val < 60))
		 {
			 dc->fillRect(5, y_val, vData5.voltage_Akk, 5, TFT_YELLOW);
		 }
		 else if (x_val >= 60)
		 {
			 dc->fillRect(5, y_val, vData5.voltage_Akk, 5, TFT_GREEN);
		 }
		 dc->fillRect(vData5.voltage_Akk, y_val-1, 230, 7, TFT_BLACK);
         dc->drawRect(5, y_val, 230, 6, TFT_WHITE);

	 //}

 }


 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 void TFTMenuScreen::drawChart()
 {

     //if (/*!isActive()*/ /*|| !canDrawChart */ /*||*/ inDrawingChart)
     //    return;
     // DBGLN(F("Screen1::drawChart()"));
     inDrawingChart = true;

     // рисуем сетку
     int gridX = GRID_X;          // начальная координата сетки по X
     int gridY1 = GRID_Y1;         // начальная координата сетки1 по Y
     int columnsCount = COLUMNS_Count;   // количество столбцов
     int rowsCount = ROWS_Count;      // количество строк
     int columnWidth = COLUMNS_Width;   // ширина столбца
     int rowHeight = ROW_Height;      // высота строки 
     RGBColor gridColor = { 0,200,0 };     // цвет сетки
     RGBColor ThresholColor = { 200,0,0 }; // цвет порога

     int gridY2 = gridY1 + (rowHeight * rowsCount) + 15; // начальная координата сетки2 по Y
     int gridY3 = gridY2 + (rowHeight * rowsCount) + 15; // начальная координата сетки3 по Y

     static uint32_t fpsMillis = 0;
     uint32_t now = millis();

     int Threshold1 = gridY1 + Settings.GetThreshold1();
     int Threshold2 = gridY2 + Settings.GetThreshold2();
     int Threshold3 = gridY3 + Settings.GetThreshold3();


     if (now - fpsMillis > (1000 / CHART_FPS))
     {
         // вызываем функцию для отрисовки сетки, её можно вызывать из каждого класса экрана
         Drawing::DrawGrid(gridX, gridY1, columnsCount, rowsCount, columnWidth, rowHeight, gridColor);
         Drawing::DrawGrid(gridX, gridY2, columnsCount, rowsCount, columnWidth, rowHeight, gridColor);
         Drawing::DrawGrid(gridX, gridY3, columnsCount, rowsCount, columnWidth, rowHeight, gridColor);

         Drawing::DrawThreshold(gridX, Threshold1, columnsCount, columnWidth, Threshold1_old, ThresholColor);
         Drawing::DrawThreshold(gridX, Threshold2, columnsCount, columnWidth, Threshold2_old, ThresholColor);
         Drawing::DrawThreshold(gridX, Threshold3, columnsCount, columnWidth, Threshold3_old, ThresholColor);

         Threshold1_old = Threshold1;
         Threshold2_old = Threshold2;
         Threshold3_old = Threshold3;

         chart.draw();     // просим график отрисовать наши серии

         fpsMillis = millis();
     }

     inDrawingChart = false;
     canDrawChart = false;
     // DBGLN(F("Screen1::drawChart() END"));
 }
 //------------------------------------------------------------------------------------------------------------------------------------------------------------------------





//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// KeyboardScreen
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
KeyboardScreen* Keyboard;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
KeyboardScreen::KeyboardScreen() : AbstractTFTScreen()
{
  inputTarget = NULL;
  maxLen = 20;
  isRusInput = true;
  
  if(!TFTScreen->getDC())
  {
    return;
  }
  

  buttons = new TFT_Buttons_Rus(TFTScreen->getDC(), TFTScreen->getTouch(),TFTScreen->getRusPrinter(),60);
  
  buttons->setTextFont(TFT_FONT);
  buttons->setButtonColors(TFT_CHANNELS_BUTTON_COLORS);
  buttons->setSymbolFont(VARIOUS_SYMBOLS_32x32);

  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
KeyboardScreen::~KeyboardScreen()
{
  for(size_t i=0;i<captions.size();i++)
  {
    delete captions[i];
  }
  for(size_t i=0;i<altCaptions.size();i++)
  {
    delete altCaptions[i];
  }
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::switchInput(bool redraw)
{
  isRusInput = !isRusInput;
  Vector<String*>* pVec = isRusInput ? &captions : &altCaptions;

  // у нас кнопки изменяемой клавиатуры начинаются с индекса 10
  size_t startIdx = 10;

  for(size_t i=startIdx;i<pVec->size();i++)
  {
    buttons->relabelButton(i,(*pVec)[i]->c_str(),redraw);
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::setup(TFTMenu* dc)
{
  if(!dc->getDC())
  {
    return;
  }
	
  createKeyboard(dc);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::update(TFTMenu* menu)
{
  if(!menu->getDC())
  {
    return;
  }
	
    // тут обновляем внутреннее состояние
    // раз нас вызвали, то пока не нажмут кнопки - мы не выйдем, поэтому всегда сообщаем, что на экране что-то происходит
    menu->resetIdleTimer();

    // мигаем курсором
    static uint32_t cursorTimer = millis();
    if(millis() - cursorTimer > 500)
    {
      static bool cursorVisible = true;
      cursorVisible = !cursorVisible;

      redrawCursor(menu,cursorVisible);

      cursorTimer = millis();
    }
    
    // проверяем на перемещение курсора
    TOUCH_Class* touch = menu->getTouch();

    uint16_t touch_x, touch_y;
    
    if(touch->getTouch(&touch_x, &touch_y))
    {
      // проверяем на попадание в прямоугольную область ввода текста
      TFT_Class* dc = menu->getDC();
      dc->setFreeFont(TFT_FONT);
      
      int screenWidth = dc->width();
      const int fontWidth = 8;
      
      if(touch_x >= KBD_SPACING && touch_x <= (screenWidth - KBD_SPACING) && touch_y >= KBD_SPACING && touch_y <= (KBD_SPACING + KBD_BUTTON_HEIGHT))
      {
        #ifdef USE_BUZZER
          Buzzer.buzz();
        #endif
        // кликнули на области ввода, ждём отпускания тача
        while (touch->getTouch(&touch_x, &touch_y)) { yield(); }
        

        // вычисляем, на какой символ приходится клик тачем
        int symbolNum = touch_x/fontWidth - 1;
        
        if(symbolNum < 0)
          symbolNum = 0;
          
        int valLen = menu->getRusPrinter()->getStringLength(inputVal.c_str());

        if(symbolNum > valLen)
          symbolNum = valLen;

        redrawCursor(menu,true);
        cursorPos = symbolNum;
        redrawCursor(menu,false);
      }
    } // if (touch->dataAvailable())
  
    int pressed_button = buttons->checkButtons(ButtonPressed, ButtonReleased);
    if(pressed_button != -1)
    {
      
       if(pressed_button == backspaceButton)
       {
        // удалить последний введённый символ
        drawValue(menu,true);
       }
       else
       if(pressed_button == okButton)
       {
          // закрыть всё нафик
          if(inputTarget)
          {
            inputTarget->onKeyboardInputResult(inputVal,true);
            inputVal = "";
          }
       }
        else
       if(pressed_button == switchButton)
       {
          // переключить раскладку
          switchInput(true);
       }
       else
       if(pressed_button == cancelButton)
       {
          // закрыть всё нафик
          if(inputTarget)
          {
            inputTarget->onKeyboardInputResult(inputVal,false);
            inputVal = "";
          }
       }
       else
       {
         // одна из кнопок клавиатуры, добавляем её текст к буферу, но - в позицию курсора!!!
         int oldLen = menu->getRusPrinter()->getStringLength(inputVal.c_str());
         const char* lbl = buttons->getLabel(pressed_button);
         
         if(!oldLen) // пустая строка
         {
          inputVal = lbl;
         }
         else
         if(oldLen < maxLen)
         {
            
            String buff;            
            const char* ptr = inputVal.c_str();
            
            for(int i=0;i<oldLen;i++)
            {
              unsigned char curChar = (unsigned char) *ptr;
              unsigned int charSz = utf8GetCharSize(curChar);
              for(byte k=0;k<charSz;k++) 
              {
                utf8Bytes[k] = *ptr++;
              }
              utf8Bytes[charSz] = '\0'; // добавляем завершающий 0
              
              if(i == cursorPos)
              {
                buff += lbl;
              }
              
              buff += utf8Bytes;
              
            } // for

            if(cursorPos >= oldLen)
              buff += lbl;

          inputVal = buff;
          
         } // if(oldLen < maxLen)
         

          int newLen = menu->getRusPrinter()->getStringLength(inputVal.c_str());

          if(newLen <= maxLen)
          {
            drawValue(menu);
                     
            if(newLen != oldLen)
            {
              redrawCursor(menu,true);
              cursorPos++;
              redrawCursor(menu,false);
            }
            
          }
          

         
       } // else одна из кнопок клавиатуры
    
    } // if(pressed_button != -1)
    
    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::draw(TFTMenu* menu)
{
  if(!menu->getDC())
  {
    return;
  }
	

  buttons->drawButtons(drawButtonsYield);

  TFT_Class* dc = menu->getDC();
  int screenWidth = dc->width();
  dc->drawRoundRect(KBD_SPACING, KBD_SPACING, screenWidth-KBD_SPACING*2, KBD_BUTTON_HEIGHT,2, TFT_LIGHTGREY);

  drawValue(menu);
  redrawCursor(menu,false);
}
//--------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::redrawCursor(TFTMenu* menu, bool erase)
{
  TFT_Class* dc = menu->getDC();
  TFTRus* rus = menu->getRusPrinter();

  dc->setFreeFont(TFT_FONT);
  uint8_t fontHeight = FONT_HEIGHT(dc);

  int top = KBD_SPACING + (KBD_BUTTON_HEIGHT - fontHeight)/2;
  
  String tmp = inputVal.substring(0,cursorPos);
  
  int left = KBD_SPACING*2 + rus->textWidth(tmp.c_str());

  uint16_t fgColor = TFT_BACK_COLOR;

  if(erase)
    fgColor = TFT_BACK_COLOR;
  else
    fgColor = TFT_FONT_COLOR;
  
  dc->fillRect(left,top,1,fontHeight,fgColor);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::drawValue(TFTMenu* menu, bool deleteCharAtCursor)
{
  if(!inputVal.length())
    return;


   TFT_Class* dc = menu->getDC();

  if(deleteCharAtCursor)
  {
    // надо удалить символ слева от позиции курсора.

    String buff;
    int len = menu->getRusPrinter()->getStringLength(inputVal.c_str());
    const char* ptr = inputVal.c_str();
    
    for(int i=0;i<len;i++)
    {
      unsigned char curChar = (unsigned char) *ptr;
      unsigned int charSz = utf8GetCharSize(curChar);
      for(byte k=0;k<charSz;k++) 
      {
        utf8Bytes[k] = *ptr++;
      }
      utf8Bytes[charSz] = '\0'; // добавляем завершающий 0
      
      if(i != (cursorPos-1)) // игнорируем удаляемый символ
      {
        buff += utf8Bytes;
      }
      
    } // for
    
    buff += ' '; // маскируем последний символ для корректной перерисовки на экране
    inputVal = buff;

  }

  dc->setFreeFont(TFT_FONT);
  
  uint8_t fontHeight = FONT_HEIGHT(dc);


  int top = KBD_SPACING + (KBD_BUTTON_HEIGHT - fontHeight)/2;
  int left = KBD_SPACING*2;

  menu->getRusPrinter()->print(inputVal.c_str(),left,top,TFT_BACK_COLOR,TFT_FONT_COLOR);

  if(deleteCharAtCursor)
  {
    // если надо удалить символ слева от позиции курсора, то в этом случае у нас последний символ - пробел, и мы его удаляем
    inputVal.remove(inputVal.length()-1,1);

    redrawCursor(menu,true);

    cursorPos--;
    if(cursorPos < 0)
      cursorPos = 0;

    redrawCursor(menu,false);
  }
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::createKeyboard(TFTMenu* menu)
{
  buttons->deleteAllButtons();

  TFT_Class* dc = menu->getDC();
  int screenWidth = dc->width();
  int screenHeight = dc->height();  

  // создаём клавиатуру

  int colCounter = 0;
  int left = KBD_SPACING;
  int top = KBD_SPACING*2 + KBD_BUTTON_HEIGHT;

  // сперва у нас кнопки 0-9
  for(uint8_t i=0;i<10;i++)
  {
    char c = '0' + i;
    String* s = new String(c);
    captions.push_back(s);

    String* altS = new String(c);
    altCaptions.push_back(altS);    

    /*int addedBtn = */buttons->addButton(left, top, KBD_BUTTON_WIDTH, KBD_BUTTON_HEIGHT, s->c_str());
   // buttons->setButtonBackColor(addedBtn, VGA_GRAY);
   // buttons->setButtonFontColor(addedBtn, VGA_BLACK);
    
    left += KBD_BUTTON_WIDTH + KBD_SPACING;
    colCounter++;
    if(colCounter >= KBD_BUTTONS_IN_ROW)
    {
      colCounter = 0;
      left = KBD_SPACING;
      top += KBD_SPACING + KBD_BUTTON_HEIGHT;
    }
  }
  // затем - А-Я
  const char* letters[] = {
    "А", "Б", "В", "Г", "Д", "Е",
    "Ж", "З", "И", "Й", "К", "Л",
    "М", "Н", "О", "П", "Р", "С",
    "Т", "У", "Ф", "Х", "Ц", "Ч",
    "Ш", "Щ", "Ъ", "Ы", "Ь", "Э",
    "Ю", "Я", NULL
  };

  const char* altLetters[] = {
    "A", "B", "C", "D", "E", "F",
    "G", "H", "I", "J", "K", "L",
    "M", "N", "O", "P", "Q", "R",
    "S", "T", "U", "V", "W", "X",
    "Y", "Z", ".", ",", ":", ";",
    "!", "?", NULL
  };  

  int lettersIterator = 0;
  while(letters[lettersIterator])
  {
    String* s = new String(letters[lettersIterator]);
    captions.push_back(s);

    String* altS = new String(altLetters[lettersIterator]);
    altCaptions.push_back(altS);

    buttons->addButton(left, top, KBD_BUTTON_WIDTH, KBD_BUTTON_HEIGHT, s->c_str());
    left += KBD_BUTTON_WIDTH + KBD_SPACING;
    colCounter++;
    if(colCounter >= KBD_BUTTONS_IN_ROW)
    {
      colCounter = 0;
      left = KBD_SPACING;
      top += KBD_SPACING + KBD_BUTTON_HEIGHT;
    } 

    lettersIterator++;
  }
  // затем - кнопка переключения ввода
    switchButton = buttons->addButton(left, top, KBD_BUTTON_WIDTH, KBD_BUTTON_HEIGHT, "q", BUTTON_SYMBOL);
    buttons->setButtonBackColor(switchButton, TFT_MAROON);
    buttons->setButtonFontColor(switchButton, TFT_WHITE);

    left += KBD_BUTTON_WIDTH + KBD_SPACING;
  
  // затем - пробел,
    spaceButton = buttons->addButton(left, top, KBD_BUTTON_WIDTH*5 + KBD_SPACING*4, KBD_BUTTON_HEIGHT, " ");
    //buttons->setButtonBackColor(spaceButton, VGA_GRAY);
    //buttons->setButtonFontColor(spaceButton, VGA_BLACK);
       
    left += KBD_BUTTON_WIDTH*5 + KBD_SPACING*5;
   
  // backspace, 
    backspaceButton = buttons->addButton(left, top, KBD_BUTTON_WIDTH*2 + KBD_SPACING, KBD_BUTTON_HEIGHT, ":", BUTTON_SYMBOL);
    buttons->setButtonBackColor(backspaceButton, TFT_MAROON);
    buttons->setButtonFontColor(backspaceButton, TFT_WHITE);

    left = KBD_SPACING;
    top = screenHeight - KDB_BIG_BUTTON_HEIGHT - KBD_SPACING;
   
  // OK,
    int okCancelButtonWidth = (screenWidth - KBD_SPACING*3)/2;
    okButton = buttons->addButton(left, top, okCancelButtonWidth, KDB_BIG_BUTTON_HEIGHT, "OK");
    left += okCancelButtonWidth + KBD_SPACING;
  
  // CANCEL
    cancelButton = buttons->addButton(left, top, okCancelButtonWidth, KDB_BIG_BUTTON_HEIGHT, "ОТМЕНА");

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::applyType(KeyboardType keyboardType)
{
  if(ktFull == keyboardType)
  {
    buttons->enableButton(spaceButton);
    buttons->enableButton(switchButton);

    // включаем все кнопки
    // у нас кнопки изменяемой клавиатуры начинаются с индекса 10
    size_t startIdx = 10;
  
    for(size_t i=startIdx;i<altCaptions.size();i++)
    {
      buttons->enableButton(i);
    }    

    isRusInput = false;
    switchInput(false);

    return;
  }

  if(ktNumbers == keyboardType)
  {
    buttons->disableButton(spaceButton);
    buttons->disableButton(switchButton);

    // выключаем все кнопки, кроме номеров и точки
    // у нас кнопки изменяемой клавиатуры начинаются с индекса 10
    size_t startIdx = 10;
  
    for(size_t i=startIdx;i<altCaptions.size();i++)
    {
      if(*(altCaptions[i]) != ".")
        buttons->disableButton(i);
    }        

    isRusInput = true;
    switchInput(false);

    return;
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::show(const String& val, int ml, KeyboardInputTarget* it, KeyboardType keyboardType, bool eng)
{
  if(!TFTScreen->getDC())
  {
    return;
  }
	
  inputVal = val;
  inputTarget = it;
  maxLen = ml;

  cursorPos = TFTScreen->getRusPrinter()->getStringLength(inputVal.c_str());

  applyType(keyboardType);

  if(eng && isRusInput)
    switchInput(false);

  TFTScreen->switchToScreen(this);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AbstractTFTScreen* KeyboardScreen::create()
{
    Keyboard = new KeyboardScreen();
    return Keyboard;  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
MessageBoxScreen* MessageBox;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
MessageBoxScreen::MessageBoxScreen() : AbstractTFTScreen()
{
  targetOkScreen = NULL;
  targetCancelScreen = NULL;
  resultSubscriber = NULL;
  caption = NULL;
  
  if(!TFTScreen->getDC())
  {
    return;
  }
  

  buttons = new TFT_Buttons_Rus(TFTScreen->getDC(), TFTScreen->getTouch(),TFTScreen->getRusPrinter());
  
  buttons->setTextFont(TFT_FONT);
  buttons->setButtonColors(TFT_CHANNELS_BUTTON_COLORS);
   
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::setup(TFTMenu* dc)
{
  if(!dc->getDC())
  {
    return;
  }

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::update(TFTMenu* dc)
{
  if(!dc->getDC())
  {
    return;
  }
	
    // тут обновляем внутреннее состояние    
 
    int pressed_button = buttons->checkButtons(ButtonPressed, ButtonReleased);
    if(pressed_button != -1)
    {
      // сообщаем, что у нас нажата кнопка
      dc->resetIdleTimer();
      
       if(pressed_button == noButton && targetCancelScreen)
       {
        if(resultSubscriber)
          resultSubscriber->onMessageBoxResult(false);
          
        dc->switchToScreen(targetCancelScreen);
       }
       else
       if(pressed_button == yesButton && targetOkScreen)
       {
          if(resultSubscriber)
            resultSubscriber->onMessageBoxResult(true);
            
            dc->switchToScreen(targetOkScreen);
       }
    
    } // if(pressed_button != -1)

    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::draw(TFTMenu* hal)
{
  TFT_Class* dc = hal->getDC();
  
  if(!dc)
  {
    return;
  }
  
  dc->setFreeFont(TFT_FONT);
  TFTRus* rusPrinter = hal->getRusPrinter();
  
  uint8_t fontHeight = FONT_HEIGHT(dc);
  
  int displayWidth = dc->width();
  int displayHeight = dc->height();
  
  int lineSpacing = 6; 
  int topOffset = 10;
  int curX = 0;
  int curY = topOffset;

  int lineLength = 0;

  uint16_t fgColor = TFT_NAVY, bgColor = TFT_WHITE;
  
  // подложка под заголовок
  if(boxType == mbHalt && errorColors)
  {
    fgColor = TFT_RED;
  }
  else
  {
    fgColor = TFT_NAVY;
  }
    
  dc->fillRect(0, 0, displayWidth, topOffset + fontHeight+4,fgColor);
  
  if(caption)
  {
    if(boxType == mbHalt && errorColors)
    {
      bgColor = TFT_RED;
      fgColor = TFT_WHITE;
    }
    else
    {
      bgColor = TFT_NAVY;
      fgColor = TFT_WHITE;      
    }
    lineLength = rusPrinter->textWidth(caption);
    curX = (displayWidth - lineLength)/2; 
    rusPrinter->print(caption,curX,curY,bgColor,fgColor);
  }

  curY = (displayHeight - ALL_CHANNELS_BUTTON_HEIGHT - (lines.size()*fontHeight + (lines.size()-1)*lineSpacing))/2;

  for(size_t i=0;i<lines.size();i++)
  {
    lineLength = rusPrinter->textWidth(lines[i]);
    curX = (displayWidth - lineLength)/2;    
    rusPrinter->print(lines[i],curX,curY,TFT_BACK_COLOR,TFT_FONT_COLOR);
    curY += fontHeight + lineSpacing;
  }

  buttons->drawButtons(drawButtonsYield);

  if(boxType == mbHalt && haltInWhile)
  {
    while(1)
    {
      #ifdef USE_EXTERNAL_WATCHDOG
        updateExternalWatchdog();
      #endif      
    }
  }

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::recreateButtons()
{
  buttons->deleteAllButtons();
  yesButton = -1;
  noButton = -1;
  
  TFT_Class* dc = TFTScreen->getDC();
  
  int screenWidth = dc->width();
  int screenHeight = dc->height();
  int buttonsWidth = 200;

  int numOfButtons = boxType == mbShow ? 1 : 2;

  int top = screenHeight - ALL_CHANNELS_BUTTON_HEIGHT - INFO_BOX_V_SPACING;
  int left = (screenWidth - (buttonsWidth*numOfButtons + INFO_BOX_V_SPACING*(numOfButtons-1)))/2;
  
  yesButton = buttons->addButton(left, top, buttonsWidth, ALL_CHANNELS_BUTTON_HEIGHT, boxType == mbShow ? "OK" : "ДА");

  if(boxType == mbConfirm)
  {
    left += buttonsWidth + INFO_BOX_V_SPACING;
    noButton = buttons->addButton(left, top, buttonsWidth, ALL_CHANNELS_BUTTON_HEIGHT, "НЕТ");  
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::halt(const char* _caption, Vector<const char*>& _lines, bool _errorColors, bool _haltInWhile)
{
  if(!TFTScreen->getDC())
  {
    return;
  }
	
  lines = _lines;
  caption = _caption;
  boxType = mbHalt;
  errorColors = _errorColors;
  haltInWhile = _haltInWhile;

  buttons->deleteAllButtons();
  yesButton = -1;
  noButton = -1;
    
  targetOkScreen = NULL;
  targetCancelScreen = NULL;
  resultSubscriber = NULL;  

  TFTScreen->switchToScreen(this);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::show(const char* _caption, Vector<const char*>& _lines, AbstractTFTScreen* okTarget, MessageBoxResultSubscriber* sub)
{
  if(!TFTScreen->getDC())
  {
    return;
  }
	
  lines = _lines;
  caption = _caption;
  errorColors = false;

  boxType = mbShow;
  recreateButtons();
    
  targetOkScreen = okTarget;
  targetCancelScreen = NULL;
  resultSubscriber = sub;

  TFTScreen->switchToScreen(this);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::confirm(const char* _caption, Vector<const char*>& _lines, AbstractTFTScreen* okTarget, AbstractTFTScreen* cancelTarget, MessageBoxResultSubscriber* sub)
{
  if(!TFTScreen->getDC())
  {
    return;
  }
	
  lines = _lines;
  caption = _caption;
  errorColors = false;

  boxType = mbConfirm;
  recreateButtons();
  
  targetOkScreen = okTarget;
  targetCancelScreen = cancelTarget;
  resultSubscriber = sub;

  TFTScreen->switchToScreen(this);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AbstractTFTScreen* MessageBoxScreen::create()
{
    MessageBox = new MessageBoxScreen();
    return MessageBox;  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// TickerClass
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TickerClass Ticker;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TickerClass::TickerClass()
{
  started = false;
  beforeStartTickInterval = 1000;
  tickInterval = 100;
  waitBefore = true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TickerClass::~TickerClass()
{
  stop();
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TickerClass::setIntervals(uint16_t _beforeStartTickInterval,uint16_t _tickInterval)
{
  beforeStartTickInterval = _beforeStartTickInterval;
  tickInterval = _tickInterval;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TickerClass::start(ITickHandler* h)
{
  if(started)
    return;

  handler = h;

  timer = millis();
  waitBefore = true;
  started = true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TickerClass::stop()
{
  if(!started)
    return;

  handler = NULL;

  started = false;
  waitBefore = true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TickerClass::tick()
{
  if(!started)
    return;

  uint32_t now = millis();

  if(waitBefore)
  {
    if(now - timer > beforeStartTickInterval)
    {
      waitBefore = false;
      timer = now;
      if(handler)
        handler->onTick();
    }
    return;
  }

  if(now - timer > tickInterval)
  {
    timer = now;
    if(handler)
      handler->onTick();
  }

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#endif // USE_TFT_MODULE
