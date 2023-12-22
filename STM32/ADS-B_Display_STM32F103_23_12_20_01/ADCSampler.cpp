
#include <stdio.h>                // define I/O functions
#include <Arduino.h>              // define I/O functions
#include "ADCSampler.h"
#include "CONFIG.h"
#include "Settings.h"
#include <stdlib.h>
//#include "hardwaretimer.h"



//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ADCStopper::ADCStopper()
{
  adcSampler.pause(); // останавливаем АЦП
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ADCStopper::~ADCStopper()
{
  adcSampler.resume(); // запускаем АЦП
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ADCSampler adcSampler;
static ADC_HandleTypeDef hadc1;
static DMA_HandleTypeDef hdma_adc1;
static TIM_HandleTypeDef htim2 = {0};




//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MX_DMA_Init(void)  // Enable DMA controller clock
{
DBGLN("MX_DMA_Init START.");

/* DMA controller clock enable */
__HAL_RCC_DMA1_CLK_ENABLE();

/* DMA interrupt init */
/* DMA1_Channel1_IRQn interrupt configuration */
HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

DBGLN("MX_DMA_Init END.");
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MX_ADC1_Init(void) // Init ADC1
{

    DBGLN("MX_ADC1_Init START.");

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    /* Peripheral clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    __HAL_RCC_ADC1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**ADC1 GPIO Configuration
    PA0-WKUP     ------> ADC1_IN0
    PA1     ------> ADC1_IN1
    PA2     ------> ADC1_IN2
    PA3     ------> ADC1_IN3
    PB0     ------> ADC1_IN8
    PB1     ------> ADC1_IN9
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


    /* ADC1 DMA Init */
     /* ADC1 Init */
    hdma_adc1.Instance = DMA1_Channel1;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
    {
      //  Error_Handler();
    }


    ADC_ChannelConfTypeDef sConfig = { 0 };

    /* USER CODE BEGIN ADC1_Init 1 */

    /* USER CODE END ADC1_Init 1 */
    /** Common config
    */
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_CC2;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 6;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
       // Error_Handler();
    }
    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_13CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
       // Error_Handler();
    }
    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_2;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
       // Error_Handler();
    }
    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_2;
    sConfig.Rank = ADC_REGULAR_RANK_3;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
       // Error_Handler();
    }
    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank = ADC_REGULAR_RANK_4;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
      //  Error_Handler();
    }
    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_8;
    sConfig.Rank = ADC_REGULAR_RANK_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
       // Error_Handler();
    }
    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_9;
    sConfig.Rank = ADC_REGULAR_RANK_6;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
       // Error_Handler();
    }

    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);



    /* ADC1 interrupt Init */
    HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);

    DBGLN("MX_ADC1_Init END.");
}

volatile bool LEDOn13 = 0;



//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MX_TIM2_Init(void)
{
    DBGLN("MX_TIM2_Init START.");

    ///* TIM2 interrupt Init */
    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);


    TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
    TIM_MasterConfigTypeDef sMasterConfig = { 0 };
    TIM_OC_InitTypeDef sConfigOC = { 0 };


    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 7200 - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 1000 - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
       // Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
    {
       // Error_Handler();
    }
    if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
    {
       // Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
       // Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_FORCED_ACTIVE;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
    {
       // Error_Handler();
    }


    __HAL_RCC_TIM2_CLK_ENABLE();


 DBGLN("MX_TIM3_Init END.");
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//extern "C" {
//    void TIM2_UP_IRQHandler(void) 
//    {
//        HAL_TIM_IRQHandler(&htim2);
//        digitalWrite(LCD_LED, !digitalRead(LCD_LED));
//       // adcSampler.handleInterrupt();
//    }
//}

//
////extern "C" {
//    void HAL_TIM_TriggerCallback(TIM_HandleTypeDef* htim)
//    {
//        HAL_TIM_IRQHandler(&htim2);
//        if (htim == &htim2)
//        {
//            digitalWrite(LCD_LED, !digitalRead(LCD_LED));
//        }
//
//    }
////}

//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
//{
//   // HAL_TIM_IRQHandler(&htim2);
//    if (htim == &htim2)
//    {
//       // HAL_GPIO_TogglePin(N_CS_GPIO_Port, N_CS_Pin);
//    }
//}

//extern "C"  void TIM2_IRQHandler(void) // обработчик тика таймера
//{
//  
//  HAL_TIM_IRQHandler(&htim2);
//
//  // вызываем обработчик прерывания, для накопления сэмплов АЦП
//	adcSampler.handleInterrupt();
//}
// 
// extern "C" void DMA1_Channel1_IRQHandler(void)
//{
//    /* USER CODE BEGIN DMA1_Channel1_IRQn 0 */
//
//    /* USER CODE END DMA1_Channel1_IRQn 0 */
//    HAL_DMA_IRQHandler(&hdma_adc1);
//    /* USER CODE BEGIN DMA1_Channel1_IRQn 1 */
//    adcSampler.handleInterrupt();
//   // digitalWrite(LCD_LED, !digitalRead(LCD_LED));
//    /* USER CODE END DMA1_Channel1_IRQn 1 */
//}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef _CURRENT_COLLECT_OFF
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
volatile uint16_t avgSamplesDone = 0; // кол-во собранных семплов для усреднения
// списки для усреднения
int avgChannel1[CURRENT_AVG_SAMPLES] = {0};
int avgChannel2[CURRENT_AVG_SAMPLES] = {0};
int avgChannel3[CURRENT_AVG_SAMPLES] = {0};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#endif // _CURRENT_COLLECT_OFF
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ADCSampler::ADCSampler()
{
  dataReady = false;
  filledBufferIndex = 0;
  workingBufferIndex = 0;
  countOfSamples = 0;
  currentOscillTimer = 0;
  canCollectCurrentData = true;
  _stopped = false;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ADCSampler::setLowBorder(uint32_t val) 
{
  _compare_Low = val; 
} 
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ADCSampler::setHighBorder(uint32_t val) 
{
  _compare_High = val; 
} 
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ADCSampler::begin()
{  
DBGLN("ADCSampler::begin START.");  

  _stopped = false;
  dataReady = false;

  oscillData.init();
  currentOscillTimer = 0;
  canCollectCurrentData = true;
  
  #ifndef _CURRENT_COLLECT_OFF
    avgSamplesDone = 0;
  #endif

  filledBufferIndex = 0;
  workingBufferIndex = 0;
  
  countOfSamples = 0;  
  memset(adcBuffer,0,sizeof(adcBuffer));
  
 MX_DMA_Init();
 MX_ADC1_Init();
 MX_TIM2_Init();  


 HAL_ADCEx_Calibration_Start(&hadc1); 
  
  // говорим АЦП собирать данные по каналам в наш буфер
 HAL_ADC_Start_DMA(&hadc1,(uint32_t*) &tempADCBuffer,NUM_CHANNELS);
 delay(20);




  // запускаем таймер

  //HAL_TIM_Base_Start(&htim2);
 // HAL_TIM_Base_Start_IT ( &htim2 ); // Нужно поддвердить прерывание
  delay(200);

DBGLN("ADCSampler::begin END.");   
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ADCSampler::end()
{

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool ADCSampler::putAVG(uint16_t raw1, uint16_t raw2, uint16_t raw3)
{
#ifndef _CURRENT_COLLECT_OFF
  
      avgChannel1[avgSamplesDone] = raw1;
      avgChannel2[avgSamplesDone] = raw2;
      avgChannel3[avgSamplesDone] = raw3;

      avgSamplesDone++;
      
      if(avgSamplesDone >= CURRENT_AVG_SAMPLES)
      {
          avgSamplesDone = 0;
          return true;
      }
#endif // #ifndef _CURRENT_COLLECT_OFF      

 return false;     
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ADCSampler::getAVG(int& avg1, int& avg2, int& avg3)
{
  avg1 = avg2 = avg3 = 0;
  
#ifndef _CURRENT_COLLECT_OFF  

   int chMin1 = 0xFFFF;
   int chMin2 = 0xFFFF;
   int chMin3 = 0xFFFF;

   int chMax1 = 0;
   int chMax2 = 0;
   int chMax3 = 0;
  
      

  for(uint16_t i=0;i<CURRENT_AVG_SAMPLES;i++)
  {
    chMin1 = min(chMin1,avgChannel1[i]);
    chMin2 = min(chMin2,avgChannel2[i]);
    chMin3 = min(chMin3,avgChannel3[i]);

    chMax1 = max(chMax1,avgChannel1[i]);
    chMax2 = max(chMax2,avgChannel2[i]);
    chMax3 = max(chMax3,avgChannel3[i]);
  }

  if(chMin1 == 0xFFFF)
  {
    chMin1 = chMax1;
  }

  if(chMin2 == 0xFFFF)
  {
    chMin2 = chMax2;
  }

  if(chMin3 == 0xFFFF)
  {
    chMin3 = chMax3;
  }

  avg1 = chMax1/CURRENT_AVG_SAMPLES - chMin1/CURRENT_AVG_SAMPLES;
  avg2 = chMax2/CURRENT_AVG_SAMPLES - chMin2/CURRENT_AVG_SAMPLES;
  avg3 = chMax3/CURRENT_AVG_SAMPLES - chMin3/CURRENT_AVG_SAMPLES;

  #endif // #ifndef _CURRENT_COLLECT_OFF
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ADCSampler::setCanCollectCurrentData(bool val)
{
  noInterrupts();
      canCollectCurrentData = val;  
  interrupts();      
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
CurrentOscillData ADCSampler::getListOfCurrent(bool withNoInterrupts)
{
    pause(withNoInterrupts);

  // возвращаем нормализованный список, упорядоченный по времени
  CurrentOscillData result = oscillData.normalize();

  // очищаем локальный список осциллограмм тока
  oscillData.clear();
  
  #ifndef _CURRENT_COLLECT_OFF
  avgSamplesDone = 0;
  #endif
  
  resume(withNoInterrupts);
  
  return result;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
CurrentOscillData CurrentOscillData::normalize()
{
  // вот тут надо скопировать буфер так, чтобы учитывать индекс первой записи
  CurrentOscillData result;

  if(times.size() < CurrentOscillData::MAX_RECORDS)
  {
    result = *this;
  }
  else
  {
    // кол-во записей уже достигло максимального, надо учитывать индекс первой записи, и от него идти
    size_t readIndex = firstRecordIndex;
    for(size_t i=0;i<times.size();i++)
    {
        result.times.push_back(times[readIndex]);
        result.data1.push_back(data1[readIndex]);
        result.data2.push_back(data2[readIndex]);
        result.data3.push_back(data3[readIndex]);

        readIndex++;
        if(readIndex >= times.size())
        {
          readIndex = 0;
        }
    } // for
  }


  return result;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ADCSampler::handleInterrupt()
{
  // код обработки данных, поступающих с АЦП
  
  if(_stopped) // остановлены
  {
    return;
  }

  uint32_t InterruptTimer = micros();
    // заполняем буфер данными одного измерения
    uint16_t writeIndex = countOfSamples*NUM_CHANNELS;
    
    for(int i=0;i<NUM_CHANNELS;i++)
    {
        adcBuffer[workingBufferIndex][writeIndex++] = tempADCBuffer[i];
         /* DBG("tempADCBuffer[i] ");
          DBGLN(tempADCBuffer[i]);*/
    } // for

  /*  DBG("InterruptTimer ");
    DBGLN(micros() - InterruptTimer);*/
      // данные по осциллограмме тока

      // считаем показания по трём каналам        
        uint32_t raw1 = 0;
        uint32_t raw2 = 0;
        uint32_t raw3 = 0;

        float currentCoeff = Settings.getCurrentCoeff();
        currentCoeff /= 1000; // у нас в тысячных долях

        raw1 = (COEFF_1*(tempADCBuffer[0]))/currentCoeff;
        raw2 = (COEFF_1*(tempADCBuffer[1]))/currentCoeff;
        raw3 = (COEFF_1*(tempADCBuffer[2]))/currentCoeff;


   /*     DBG("tempADCBuffer[0] ");
        DBGLN(tempADCBuffer[0]);*/


        // тут собираем данные по осциллограмме тока
        #ifndef _CURRENT_COLLECT_OFF
        
        if(canCollectCurrentData)
        {
          if(micros() - currentOscillTimer >= CURRENT_TIMER_PERIOD)
          {
            if(putAVG(raw1,raw2,raw3))
            {
              int avg1,avg2,avg3;
              getAVG(avg1,avg2,avg3);
              
              oscillData.add(micros(),avg1,avg2,avg3);
            }
              currentOscillTimer = micros();
          }
        } // canCollectCurrentData
        
        #endif // _CURRENT_COLLECT_OFF

    countOfSamples++;

    if(countOfSamples >= ADC_BUFFER_SIZE/NUM_CHANNELS)
    {
      // буфер заполнили      
      countOfSamples = 0; // обнуляем кол-во сэмплов
      
      filledBufferIndex = workingBufferIndex; // запоминаем, какой буфер мы заполнили
      
      workingBufferIndex++; // перемещаемся на заполнение следующего буфера
      
      if(workingBufferIndex >= NUMBER_OF_BUFFERS) // если закончили заполнять все буфера - перемещаемся на старт
      {
        workingBufferIndex = 0;
      }

     // DBGLN("dataReady.");
      dataReady = true; // Данные сформированы
      
      
    } // if(countOfSamples >= ADC_BUFFER_SIZE/NUM_CHANNELS)

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ADCSampler::pause(bool withNoInterrupts)
{
  if(withNoInterrupts)
  {
    noInterrupts();  
  }
  bool thisStopped = _stopped;
  
  if(withNoInterrupts)
  {
    interrupts();
  }
  
  if(thisStopped)
  {
    return;
  }

  if(withNoInterrupts)
  {
    noInterrupts();
  }
  _stopped = true;

  if(withNoInterrupts)
  {
    interrupts();
  }
  
  // останавливаем таймер
  HAL_NVIC_DisableIRQ  ( TIM3_IRQn );
  HAL_NVIC_ClearPendingIRQ(TIM3_IRQn);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ADCSampler::resume(bool withNoInterrupts)
{
 if(withNoInterrupts)
  {
    noInterrupts();  
  }
  bool thisStopped = _stopped;
  
  if(withNoInterrupts)
  {
    interrupts();
  }

  if(!thisStopped)
  {
    return;
  }

  if(withNoInterrupts)
  {
    noInterrupts();
  }
  _stopped = false;

  if(withNoInterrupts)
  {
    interrupts();
  }

  // запускаем таймер
  HAL_NVIC_EnableIRQ  ( TIM3_IRQn );
 
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool ADCSampler::available()
{
  return dataReady;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint16_t* ADCSampler::getADCBuffer(int *bufferLength)
{
  *bufferLength = ADC_BUFFER_SIZE;
  return adcBuffer[filledBufferIndex];
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ADCSampler::reset()
{
  dataReady = false;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
