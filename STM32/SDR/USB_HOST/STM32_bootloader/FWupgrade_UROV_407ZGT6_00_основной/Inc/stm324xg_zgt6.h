/**
  ******************************************************************************
  * @file    stm324xg_eval.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for STM324xG_EVAL's LEDs, 
  *          push-buttons and COM ports hardware resources.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM324xG_ZGT6_H
#define __STM324xG_ZGT6_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
   
typedef enum 
{
  LED1 = 0,
  LED2 = 1,
	LED3 = 2
}Led_TypeDef;

typedef enum 
{  
  BUTTON_KEY1 = 0,
  BUTTON_KEY2 = 1,
  BUTTON_KEY3    = 2
}Button_TypeDef;

typedef enum 
{  
  BUTTON_MODE_GPIO = 0,
  BUTTON_MODE_EXTI = 1
}ButtonMode_TypeDef;
  


/** 
  * @brief  Define for STM324xG_EVAL board  
  */ 
#if !defined (USE_STM324xG_EVAL)
 #define USE_STM324xG_EVAL
#endif

/** @addtogroup STM324xG_EVAL_LOW_LEVEL_LED STM324xG EVAL LOW LEVEL LED
  * @{
  */
#define LEDn                             3

#define LED1_PIN                         GPIO_PIN_5
#define LED1_GPIO_PORT                   GPIOE
#define LED1_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOE_CLK_ENABLE()
#define LED1_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOE_CLK_DISABLE()
  
#define LED2_PIN                         GPIO_PIN_4
#define LED2_GPIO_PORT                   GPIOE
#define LED2_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOE_CLK_ENABLE()
#define LED2_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOE_CLK_DISABLE()

#define LED3_PIN                         GPIO_PIN_6
#define LED3_GPIO_PORT                   GPIOE
#define LED3_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOE_CLK_ENABLE()
#define LED3_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOE_CLK_DISABLE()
  

#define LEDx_GPIO_CLK_ENABLE(__INDEX__)  do{if((__INDEX__) == 0) LED1_GPIO_CLK_ENABLE(); else \
                                            if((__INDEX__) == 1) LED2_GPIO_CLK_ENABLE(); else \
	                                          if((__INDEX__) == 1) LED3_GPIO_CLK_ENABLE(); \
                                            }while(0)

																						
																						
#define LEDx_GPIO_CLK_DISABLE(__INDEX__) do{if((__INDEX__) == 0) LED1_GPIO_CLK_DISABLE(); else \
	                                          if((__INDEX__) == 0) LED2_GPIO_CLK_DISABLE(); else \
                                            if((__INDEX__) == 1) LED3_GPIO_CLK_DISABLE(); \
                                            }while(0)
/**
  * @}
  */ 
  
/** @addtogroup STM324xG_EVAL_LOW_LEVEL_BUTTON STM324xG EVAL LOW LEVEL BUTTON
  * @{
  */  
/* Joystick pins are connected to IO Expander (accessible through I2C1 interface) */
#define BUTTONn                              3

/**
  * @brief Wakeup push-button
  */
#define BUTTON_KEY1_PIN                    GPIO_PIN_11
#define BUTTON_KEY1_GPIO_PORT              GPIOE
#define BUTTON_KEY1_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOE_CLK_ENABLE()
#define BUTTON_KEY1_GPIO_CLK_DISABLE()     __HAL_RCC_GPIOE_CLK_DISABLE()
#define BUTTON_KEY1_EXTI_IRQn              EXTI15_10_IRQn

/**
  * @brief Tamper push-button
  */
#define BUTTON_KEY2_PIN                    GPIO_PIN_12
#define BUTTON_KEY2_GPIO_PORT              GPIOE
#define BUTTON_KEY2_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOE_CLK_ENABLE()
#define BUTTON_KEY2_GPIO_CLK_DISABLE()     __HAL_RCC_GPIOE_CLK_DISABLE()
#define BUTTON_KEY2_EXTI_IRQn              EXTI15_10_IRQn

/**
  * @brief Key push-button
  */
#define BUTTON_KEY3_PIN                       GPIO_PIN_13
#define BUTTON_KEY3_GPIO_PORT                 GPIOE
#define BUTTON_KEY3_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOE_CLK_ENABLE()
#define BUTTON_KEY3_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOE_CLK_DISABLE()
#define BUTTON_KEY3_EXTI_IRQn                 EXTI15_10_IRQn

#define BUTTONx_GPIO_CLK_ENABLE(__INDEX__)  do{if((__INDEX__) == 0) BUTTON_KEY1_GPIO_CLK_ENABLE(); else \
                                               if((__INDEX__) == 1) BUTTON_KEY2_GPIO_CLK_ENABLE(); else \
                                               if((__INDEX__) == 2) BUTTON_KEY3_GPIO_CLK_ENABLE(); \
                                               }while(0)
#define BUTTONx_GPIO_CLK_DISABLE(__INDEX__) do{if((__INDEX__) == 0) BUTTON_KEY1_GPIO_CLK_DISABLE(); else \
                                               if((__INDEX__) == 1) BUTTON_KEY2_GPIO_CLK_DISABLE(); else \
                                               if((__INDEX__) == 2) BUTTON_KEY3_GPIO_CLK_DISABLE(); \
                                               }while(0)


/** @defgroup STM324xG_EVAL_LOW_LEVEL_Exported_Functions STM324xG EVAL LOW LEVEL Exported Functions
  * @{
  */
uint32_t         BSP_GetVersion(void);  
void             BSP_LED_Init(Led_TypeDef Led);
void             BSP_LED_On(Led_TypeDef Led);
void             BSP_LED_Off(Led_TypeDef Led);
void             BSP_LED_Toggle(Led_TypeDef Led);
void             BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode);
uint32_t         BSP_PB_GetState(Button_TypeDef Button);



#ifdef __cplusplus
}
#endif

#endif /* __STM324xG_EVAL_H */
 
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
