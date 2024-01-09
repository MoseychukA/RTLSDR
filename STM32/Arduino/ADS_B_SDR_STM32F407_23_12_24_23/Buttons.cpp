//--------------------------------------------------------------------------------------------------
#include "Buttons.h"
#include "Feedback.h"
#include "CONFIG.h"

//--------------------------------------------------------------------------------------------------
ButtonsList Buttons;
//--------------------------------------------------------------------------------------------------
ButtonsList::ButtonsList()
{
  inited = false;
}
//--------------------------------------------------------------------------------------------------
void ButtonsList::begin()
{
  Button0.begin(BUTTON_0);
  Button1.begin(BUTTON_1);
  Button2.begin(BUTTON_3);

  inited = true;
}
//--------------------------------------------------------------------------------------------------
void ButtonsList::update()
{
  if(!inited)
    return;
    
  Button0.update();
  Button1.update();
  Button2.update();

  if(Button2.isClicked())
  {
    DBGLN(F("YELLOW BUTTON CLICKED!"));
  
    Feedback.failureDiode(false); // гасим светодиод ОШИБКА
    Feedback.readyDiode(false);   // гасим светодиод УСПЕХ
    
  }

  if(Button0.isClicked())
  {
    DBGLN(F("BUTTON 0 CLICKED!"));
  }

  if(Button1.isClicked())
  {
    DBGLN(F("BUTTON 1 CLICKED!"));
  }
  if (Button2.isClicked())
  {
      DBGLN(F("BUTTON 2 CLICKED!"));
  }
}
//--------------------------------------------------------------------------------------------------
