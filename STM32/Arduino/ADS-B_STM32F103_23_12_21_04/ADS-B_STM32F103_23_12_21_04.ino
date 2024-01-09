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

#include "string.h" // это для функции strlen()
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include "stm32f1xx_hal.h"

#include "serial_cmd.h"




TFTModule tftModule;

uint32_t screenIdleTimer = 0;


void setup()
{
    Serial.begin(Serial_SPEED);
    while (!Serial && millis() < 1000);
    Serial.flush();
    delay(1000);

    Serial.println("Start system");
    Serial.println();


  String ver_soft = __FILE__;
  int val_srt = ver_soft.lastIndexOf('\\');
  ver_soft.remove(0, val_srt+1);
  val_srt = ver_soft.lastIndexOf('.');
  ver_soft.remove(val_srt);
  Serial.println(ver_soft);

  tftModule.Setup();

  MainScreen->saveVer(ver_soft);  // Сохранить строку с текущей версией.
   
  byte error;

  Wire.begin();

  Wire.beginTransmission(0x1A);
  error = Wire.endTransmission();

  if (error == 0)
  {
      Serial.println("I2C device found at address 0x1A");
 
  }

  R820T2_init();





  
  Serial.println("Start system END");
  //start_setup = true;
}

static void flush_input(void)
{
    while (Serial.available() > 0)
        Serial.read();
}

void loop()
{
  

     Serial_CMD();  // Uncomment to enable R820T2 console.
    //checkCAT();


   tftModule.Update();

  yield();
}



//
//static void MX_USART1_UART_Init(void)
//{
//
//    /* USER CODE BEGIN USART1_Init 0 */
//
//    /* USER CODE END USART1_Init 0 */
//
//    /* USER CODE BEGIN USART1_Init 1 */
//
//    /* USER CODE END USART1_Init 1 */
//    huart1.Instance = USART1;
//    huart1.Init.BaudRate = 115200;
//    huart1.Init.WordLength = UART_WORDLENGTH_8B;
//    huart1.Init.StopBits = UART_STOPBITS_1;
//    huart1.Init.Parity = UART_PARITY_NONE;
//    huart1.Init.Mode = UART_MODE_TX_RX;
//    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
//    if (HAL_UART_Init(&huart1) != HAL_OK)
//    {
//        Error_Handler();
//    }
//    /* USER CODE BEGIN USART1_Init 2 */
//
//    /* USER CODE END USART1_Init 2 */
//
//}