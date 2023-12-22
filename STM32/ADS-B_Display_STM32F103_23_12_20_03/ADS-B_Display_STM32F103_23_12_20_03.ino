/*
 * SoftRF(.ino) firmware
 * Copyright (C) 2016-2023 Linar Yusupov
 *
 * Author: Linar Yusupov, linar.r.yusupov@gmail.com
 *
 * Web: http://github.com/lyusupov/SoftRF
 *
 *
 */

#include <stdio.h>                // define I/O functions
#include <Arduino.h>              // define I/O functions
#include "SPI.h"
#include <TFT_eSPI.h>             // Поддержка TFT дисплея  
#include <Wire.h>                 // 
#include "TFTModule.h"
#include "CONFIG.h"               // Основные настройки программы

TFTModule tftModule;

uint32_t screenIdleTimer = 0;



void setup()
{
    Serial.begin(Serial_SPEED);
    while (!Serial && millis() < 1000);

    delay(1000);

    Serial.println("Start system");
    Serial.println();


  String ver_soft = __FILE__;
  int val_srt = ver_soft.lastIndexOf('\\');
  ver_soft.remove(0, val_srt+1);
  val_srt = ver_soft.lastIndexOf('.');
  ver_soft.remove(val_srt);
  Serial.println(ver_soft);

 // Serial.println(__FILE__);



  tftModule.Setup();

  MainScreen->saveVer(ver_soft);  // Сохранить строку с текущей версией.

 
  Serial.println("Start system END");
  //start_setup = true;
}

void loop()
{
 
 
  tftModule.Update();


  yield();
}



