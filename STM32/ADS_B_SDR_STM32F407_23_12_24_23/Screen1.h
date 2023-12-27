#pragma once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "TFTMenu.h"
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// экран номер 1
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Screen1 : public AbstractTFTScreen
{
  public:

  static AbstractTFTScreen* create()
  {
    return new Screen1();
  }
  
 
   virtual void onActivate();
   virtual void onDeactivate();


protected:
  
    virtual void doSetup(TFTMenu* menu);
    virtual void doUpdate(TFTMenu* menu);
    virtual void doDraw(TFTMenu* menu);
    virtual void onButtonPressed(TFTMenu* menu, int pressedButton);

private:
    Screen1();
 /*
    uint16_t oldChannel1Current, oldChannel2Current, oldChannel3Current;
    String oldCurrentString1,oldCurrentString2,oldCurrentString3;*/
   
    int last3V3Voltage, last5Vvoltage, last200Vvoltage;

    
	  int getFreeMemory();
	  int oldsecond;

    void drawTime(TFTMenu* menu);
  
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern Screen1* mainScreen;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
