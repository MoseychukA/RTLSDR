
#include "Buzzer.h"

//--------------------------------------------------------------------------------------------------------------------------------------
BuzzerClass Buzzer;
//--------------------------------------------------------------------------------------------------------------------------------------

TIM_HandleTypeDef htim4;

static void MX_TIM4_Init(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim);
 
static void MX_TIM4_Init(void)
{

    TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
    TIM_MasterConfigTypeDef sMasterConfig = { 0 };
    TIM_OC_InitTypeDef sConfigOC = { 0 };

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 719;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 24999;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
    {
//        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
    {
//        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
    {
//        Error_Handler();
    }
    if (HAL_TIM_OnePulse_Init(&htim4, TIM_OPMODE_SINGLE) != HAL_OK)
    {
//        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
    {
//        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM2;
    sConfigOC.Pulse = 20000;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
    {
//        Error_Handler();
    }
    __HAL_TIM_DISABLE_OCxPRELOAD(&htim4, TIM_CHANNEL_3);
  
    HAL_TIM_MspPostInit(&htim4);

}


void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    if (htim->Instance == TIM4)
    {

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**TIM4 GPIO Configuration
        PB8     ------> TIM4_CH3
        */
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    }

}


BuzzerClass::BuzzerClass()
{
  active = false;
}
//--------------------------------------------------------------------------------------------------------------------------------------
void BuzzerClass::begin()
{
 
  MX_TIM4_Init();
 // HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
 
}

//--------------------------------------------------------------------------------------------------------------------------------------
void BuzzerClass::buzz()
{
  if(active)
    return;

  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
 
 }

//--------------------------------------------------------------------------------------------------------------------------------------
