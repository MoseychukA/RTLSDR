#include "InterruptHandler.h"
#include "InterruptScreen.h"
#include "Feedback.h"
#include "Settings.h"
#include "DelayedEvents.h"
#include "ADCSampler.h"

//--------------------------------------------------------------------------------------------------------------------------------------
InterruptHandlerClass InterruptHandler;
CurrentOscillData     OscillData; // данные по току, актуальные на момент прерывания
//--------------------------------------------------------------------------------------------------------------------------------------
InterruptTimeList InterruptData; // список времён срабатываний прерываний на энкодере штанги
MachineState machineState = msIdle; // состояние конечного автомата
volatile bool canHandleEncoder = false; // флаг, что мы можем собирать прерывания с энкодера
volatile uint32_t timer = 0; // служебный таймер
DS3231Time relayTriggeredTime; // время срабатывания защиты
volatile bool downEndstopTriggered = false; // состояние нижнего концевика на момент срабатывания защиты
//--------------------------------------------------------------------------------------------------------------------------------------
volatile uint16_t interruptSkipCounter = 0; // счётчик пойманных импульсов, для пропуска лишних
volatile bool paused = false; // флаг, что обработчик - на паузе
//--------------------------------------------------------------------------------------------------------------------------------------
#define REASON_RELAY  1 // причина старабывания - внешнее реле
#define REASON_PREDICT 2 // причина срабатывания - предсказания
volatile uint8_t trigReason = 0; // причина срабатывания
volatile uint32_t trigReasonTimer = 0; // таймер отсчёта от причины срабатывания
//--------------------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------------------
// ПРЕДСКАЗАНИЯ
//--------------------------------------------------------------------------------------------------------------------------------------
#ifdef PREDICT_ENABLED
//--------------------------------------------------------------------------------------------------------------------------------------
volatile bool predictEnabledFlag = true; // флаг, что мы можем собирать информацию о предсказаниях срабатывания защиты
InterruptTimeList predictList; // список для предсказаний
volatile bool predictTriggeredFlag = false; // флаг срабатывания предсказания
//--------------------------------------------------------------------------------------------------------------------------------------
void predictOff() // выключаем предсказание
{
  if(predictEnabledFlag)
  {
    predictEnabledFlag = false; // отключаем сбор предсказаний
    predictList.clear(); // очищаем список предсказаний
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------
void predictOn() // включаем предсказание
{
  if(!predictEnabledFlag)
  {
    predictEnabledFlag = true; // включаем сбор предсказаний
    predictList.clear(); // очищаем список предсказаний
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------
bool predictTriggered() // возвращает флаг срабатывания предсказания, однократно (т.е. флаг срабатывания предсказания сбрасывается перед выходом из функции)
{
  bool f = predictTriggeredFlag;
  if(f)
  {
    noInterrupts();
    predictTriggeredFlag = false;  
    interrupts();
  }
  return f;
}
//--------------------------------------------------------------------------------------------------------------------------------------
#endif // PREDICT_ENABLED
//--------------------------------------------------------------------------------------------------------------------------------------
InterruptEventSubscriber* subscriber = NULL; // подписчик для обработки результатов пачки прерываний
//--------------------------------------------------------------------------------------------------------------------------------------
void EncoderPulsesHandler() // обработчик импульсов энкодера
{
  if(paused) // на паузе
  {
    return;
  }

  // тут проверяем, надо ли пропустить N импульсов
  uint32_t toSkip = Settings.getSkipCounter();

  if(toSkip > 1) // каждый первый - пропускать бессмысленно.
  {
      interruptSkipCounter++;
      if(interruptSkipCounter % toSkip)
      {
         // надо пропустить
         return;
      }
      else
      {
        interruptSkipCounter = 0;
      }
  }
  else
  {
    interruptSkipCounter = 0;
  }
  
  #ifdef PREDICT_ENABLED // включены предсказания?
  
  if(predictEnabledFlag && !predictTriggeredFlag) // можем делать предсказания о срабатывании защиты
  {
    // включено предсказание срабатывания защиты по импульсам
    
    if(predictList.size() < PREDICT_PULSES)
    {
      predictList.push_back(micros()); // сохраняем время импульса в нашем списке
    }

    if(predictList.size() >= PREDICT_PULSES) // накопили достаточное количество импульсов
    {
      // список наполнился, можем делать предсказания
      uint32_t first = predictList[0];
      uint32_t last = predictList[predictList.size()-1];
      
      if(last - first <= PREDICT_TIME) // время между крайними импульсами укладывается в настройку
      {
        // предсказание сработало
        predictTriggeredFlag = true;
      }
      else
      {
        // предсказание не сработало, просто чистим список
        predictList.clear();        
      }
    }
  } // predictEnabledFlag
  
  #endif // PREDICT_ENABLED
  
  if(!canHandleEncoder || InterruptData.size() >= MAX_PULSES_TO_CATCH) // не надо собирать импульсы с энкодера
  {
    return;
  }
  
    uint32_t now = micros();
    InterruptData.push_back(now);
    timer = now; // обновляем значение времени, когда было последнее срабатывание энкодера  


    #ifndef DISABLE_CATCH_ENCODER_DIRECTION
        // определяем направление вращения энкодера.
        if (digitalRead(ENCODER_PIN2))
        {
          // по часовой
          Settings.setRodDirection(rpUp);
        }
        else
        {
          // против часовой
          Settings.setRodDirection(rpDown);
        }
    #endif
       
}
//--------------------------------------------------------------------------------------------------------------------------------------
InterruptHandlerClass::InterruptHandlerClass()
{
  subscriber = NULL;
  hasAlarm = false;
}
//--------------------------------------------------------------------------------------------------------------------------------------
void InterruptHandlerClass::begin()
{

// резервируем память
  InterruptData.reserve(MAX_PULSES_TO_CATCH);


  // настраиваем первый выход энкодера на чтение
#if (ENCODER_INTERRUPT_LEVEL == RISING)
  pinMode(ENCODER_PIN1, INPUT_PULLUP);
#else
  pinMode(ENCODER_PIN1, INPUT);
#endif

  // настраиваем второй выход энкодера на чтение
  pinMode(ENCODER_PIN2, INPUT);

  // ждём, пока устаканится питание
  delay(50);
  

  // считаем импульсы на штанге по прерыванию
  attachInterrupt((ENCODER_PIN1),EncoderPulsesHandler, ENCODER_INTERRUPT_LEVEL);

}
//--------------------------------------------------------------------------------------------------------------------------------------
void InterruptHandlerClass::normalizeList(InterruptTimeList& list)
{
  size_t sz = list.size();
  
  if(sz < 2)
    return;

  // нормализуем список относительно первого значения
  uint32_t first = list[0];
  list[0] = 0;

  for(size_t i=1;i<sz;i++)
  {
    list[i] = (list[i] - first);
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------
uint32_t InterruptHandlerClass::writeLogRecord(int32_t dataArrivedTime, CurrentOscillData* oscData, InterruptTimeList& _list, EthalonCompareResult compareResult
, EthalonCompareNumber num, /*InterruptTimeList& ethalonData*/const String& ethalonFileName, bool toEEPROM, uint32_t curEEPROMWriteAddress)
{

  uint32_t written = 0;
  
  if(_list.size() < 2) // ничего в списке прерываний нет
  {
    return written;
  }

 const uint8_t CHANNEL_NUM = 0;
 EEPROM_CLASS* eeprom = Settings.getEEPROM();

 uint8_t workBuff[5] = {0};

 

  if(toEEPROM)
  {
    eeprom->write(curEEPROMWriteAddress,workBuff,1);
    written++;
    curEEPROMWriteAddress++;
/*
    for(uint8_t c=0;c<1;c++)
    {
      LastTriggeredInterruptRecord.push_back(workBuff[c]);
    }
*/    
  }
  else
  {

  }
  


 

return written;
}
//--------------------------------------------------------------------------------------------------------------------------------------
void InterruptHandlerClass::writeToLog(
  int32_t dataArrivedTime, 
  DS3231Time& tm,
	CurrentOscillData* oscData,
	InterruptTimeList& lst1, 
	EthalonCompareResult res1, 
	EthalonCompareNumber num1,
	//InterruptTimeList& ethalonData1,
  const String& ethalonFileName,
  bool toEEPROM
)
{
  EEPROM_CLASS* eeprom = Settings.getEEPROM();

  if(toEEPROM && !eeprom)
  {
    return;
  }

  PAUSE_ADC; // останавливаем АЦП

  uint8_t workBuff[10] = {0};
  uint32_t eepromAddress = EEPROM_LAST_3_DATA_ADDRESS;
  uint32_t recordStartAddress = 0;
  uint32_t recordTotalLength = 0;
  
  uint8_t idx = 0;
  

  if(toEEPROM)
  {
//    LastTriggeredInterruptRecord.clear(); // очищаем список срабатывания
    LastTriggeredInterruptRecordIndex = -1;
    
    // вычисляем адрес для записи в EEPROM
    // сначала смотрим, под каким индексом записывать?
    int header1 = eeprom->read(eepromAddress);
    int header2 = eeprom->read(eepromAddress+1);
    int header3 = eeprom->read(eepromAddress+2);
    
    if(header1 == RECORD_HEADER1 && header2 == RECORD_HEADER2 && header3 == RECORD_HEADER3)
    {
      // прочитали текущий индекс, и инкрементировали его
      /*
      idx = eeprom->read(eepromAddress+3);
      idx++;
      if(idx > 2) // пишем только последние 3 срабатывания
      {
        idx = 0;
      }

      // записали новый индекс
      eeprom->write(eepromAddress+3,idx);
      */
      idx = 0; //TODO: ПОКА ТОЛЬКО ОДНА ЗАПИСЬ !!!
    }
    else
    {
      // нет записей, неправильные заголовки, надо записать
      eeprom->write(eepromAddress,RECORD_HEADER1);
      eeprom->write(eepromAddress+1,RECORD_HEADER2);
      eeprom->write(eepromAddress+2,RECORD_HEADER3);
      eeprom->write(eepromAddress+3,idx);
    }

    // теперь надо просчитать смещение для старта начала записи. При этом от начального адреса пропускаем 4 байта (адрес хранения индекса текущей записи)
    eepromAddress = EEPROM_LAST_3_DATA_ADDRESS + 4 + idx*EEPROM_LAST_3_RECORD_SIZE;

    // сначала записываем заголовок для нашей записи
    eeprom->write(eepromAddress,RECORD_HEADER1);
    eeprom->write(eepromAddress+1,RECORD_HEADER2);
    eeprom->write(eepromAddress+2,RECORD_HEADER3);
    eepromAddress += 3;

    // мы теперь на начале данных записи, надо пропустить 4 байта (куда мы потом запишем длину записи), и сохранить указатель на начало записи
    recordStartAddress = eepromAddress;
    eepromAddress += 4; // теперь можем писать данные, начиная с этого адреса
    
  }

 
}
//--------------------------------------------------------------------------------------------------------------------------------------
void InterruptHandlerClass::pause()
{
  if(paused) // уже на паузе
  {
    return;
  }

  noInterrupts();
  paused = true;
  interrupts();
}
//--------------------------------------------------------------------------------------------------------------------------------------
void InterruptHandlerClass::resume()
{
  if(!paused) // не на паузе
  {
    return;
  }

  // поскольку мы были на паузе - начинаем сначала
  #ifdef PREDICT_ENABLED
    predictEnabledFlag = true; // флаг, что мы можем собирать информацию о предсказаниях срабатывания защиты
    predictList.clear(); // список для предсказаний
    predictTriggeredFlag = false; // флаг срабатывания предсказания
  #endif

  noInterrupts();
  paused = false;
  InterruptData.empty();
  machineState = msIdle; // состояние конечного автомата
  canHandleEncoder = false; // флаг, что мы можем собирать прерывания с энкодера
  downEndstopTriggered = false; // состояние нижнего концевика на момент срабатывания защиты  
  OscillData.clear();
  interrupts();  
}
//--------------------------------------------------------------------------------------------------------------------------------------
void InterruptHandlerClass::update()
{

  if(paused) // на паузе
  {
    return;
  }

  // проверяем состояние конечного автомата
  switch(machineState)
  {
    case msIdle:
    {
 
    }
    break; // msIdle

    case msWaitHandleInterrupts:
    {
      // ждём начала импульсов с энкодера
      if(micros() - timer >= Settings.getRelayDelay())
      {
     //   DBGLN(F("WAIT DONE, COLLECT ENCODER PULSES..."));
                
        noInterrupts();
          InterruptData.empty(); // очищаем список прерываний
          timer = micros();
          canHandleEncoder = true; // разрешаем обработчику прерываний энкодера собирать информацию
          machineState = msHandleInterrupts; // можем собирать прерывания с энкодера
          
          if(trigReason == REASON_RELAY) // причиной срабатывания был сигнал внешней защиты
          {
            trigReasonTimer = micros(); // устанавливаем таймер ожидания только после ожидания, настроенного через конфигуратор
          }
        interrupts(); 
           
      }
    }
    break; // msWaitHandleInterrupts

    case msHandleInterrupts:
    {
      // собираем прерывания с энкодера
      
      noInterrupts();      
          uint32_t thisTimer = timer; // копируем значение времени последнего прерывания с энкодера локально
          size_t catchedPulses = InterruptData.size();
      interrupts();

      if(trigReason == REASON_RELAY) // причиной срабатывания был сигнал внешней защиты, проверяем, насколько давно были импульсы с энкодера
      {
          if(micros() - trigReasonTimer >= 20000ul)
          {
            trigReason = 0; // сбрасываем причину срабатывания
            
            // прошло 20 миллисекунд, можно проверять, всё ли в порядке
            // если нет импульсов с энкодера - это авария

              pause(); // ставим на паузу

              if(catchedPulses < 1)
              {
                  // за 20 миллисекунд не поймали ни одного импульса с энкодера, это авария
                  Feedback.failureDiode(); // зажигаем светодиод АВАРИЯ
                  Feedback.setFailureLineLevel(); // говорим на выходящей линии, что это авария
                  
                  uint8_t asuTpFlags = Settings.getAsuTpFlags();

                  if(asuTpFlags & 4) // только если флаг выдачи сигнала в третью линию АСУ ТП - установлен
                  {
                    // Формируем сигнал срабатывания системы на выводах АСУ ТП
                    // №3 - НО контакт: «неисправность выключателя» (параллельно красному светодиоду. При выходе параметров кривой движения за допустимые границы)
                    digitalWrite(out_asu_tp3,asu_tp_level);
                  }
        
        
              }

            // переключаемся на ветку ожидания отщёлкивания концевика защиты
            machineState = msWaitGuardRelease;
            
            resume(); // продолжаем работу
          }
      } // if(trigReason == REASON_RELAY)
      
      if(micros() - thisTimer >= INTERRUPT_MAX_IDLE_TIME) // прошло максимальное время для сбора импульсов, т.е. последний импульс с энкодера был очень давно
      {

    //    Serial.println("START WORK WITH INTERRUPT, STAGE 1!"); Serial.flush();

        PAUSE_ADC; // останавливаем АЦП на время

        pause(); // ставим на паузу
              
        noInterrupts();
          canHandleEncoder = false; // выключаем обработку импульсов энкодера
          #ifdef PREDICT_ENABLED
          predictOn(); // включаем сбор предсказаний          
          #endif
        interrupts(); 
        

 //      Serial.println("STAGE 2"); Serial.flush();

        uint8_t asuTpFlags = Settings.getAsuTpFlags();

        // обновляем моторесурс, т.к. было срабатывание защиты
        uint32_t motoresource = Settings.getMotoresource();
        motoresource++;
        Settings.setMotoresource(motoresource);
        

//        Serial.println("STAGE 3"); Serial.flush();

        // проверяем, авария ли?
        hasAlarm = !InterruptData.size();
        
        // выставляем флаг аварии, в зависимости от наличия данных в списках
        if(hasAlarm)
        {
  //        Serial.println("STAGE ALARM"); Serial.flush();
          Feedback.setFailureLineLevel(); // говорим на выходящей линии, что это авария
        }    


//        Serial.println("STAGE 4"); Serial.flush();

        // запрещаем собирать данные по току
        adcSampler.setCanCollectCurrentData(false);

//        noInterrupts();
        
           // заканчиваем сбор данных по току, копируем данные по току в локальный список
           OscillData.clear();
           OscillData = adcSampler.getListOfCurrent();//false);
        
//        interrupts();

        // разрешаем собирать данные по току
        adcSampler.setCanCollectCurrentData(true);


  //      Serial.println("STAGE 5"); Serial.flush();
        

        // вычисляем смещение от начала записи по току до начала поступления данных
        
        int32_t datArrivTm = 0;
        if(OscillData.times.size() > 0 && InterruptData.size() > 0)
        {
          uint32_t earlierCurrentRecord = OscillData.earlierRecordTime();
          uint32_t firstInterruptRecord = InterruptData[0];

          datArrivTm = 0;
          if(earlierCurrentRecord != 0xFFFFFFFF)
          {
            datArrivTm = max(earlierCurrentRecord,firstInterruptRecord) - min(earlierCurrentRecord,firstInterruptRecord);
          }
        }
        

//        Serial.println("STAGE 6"); Serial.flush();

       // datArrivTm = 500000ul;//TODO: УБРАТЬ!!!

        // нормализуем список времен записей по току
        normalizeList(OscillData.times);

         // нормализуем список прерываний
         normalizeList(InterruptData);

//         Serial.println("STAGE 7"); Serial.flush();

         // начинаем работать со списком прерываний
         EthalonCompareResult compareRes1 = COMPARE_RESULT_NoSourcePulses;
         EthalonCompareNumber compareNumber1;
         String ethalonFileName;

          bool needToLog = false;

        // теперь смотрим - надо ли нам самим чего-то обрабатывать?
        if(InterruptData.size() > 1)
        {
    
          // зажигаем светодиод "ТЕСТ" (желтый)
          Feedback.testDiode();

    
          needToLog = true; // говорим, что надо записать в лог


        } // if(InterruptData.size() > 1)

 

        bool wantToInformSubscriber = (InterruptData.size() > 1);

        if(wantToInformSubscriber)
        { 
          //  DBGLN(F("Надо уведомить подписчика прерываний!"));
          if(subscriber)
          {
//            Serial.println("STAGE INFORM SUBSCRIBER BEGIN"); Serial.flush();
            //  DBGLN(F("Подписчик найден!"));  
              
            // уведомляем подписчика
            informSubscriber(&OscillData,compareRes1);

//            Serial.println("STAGE INFORM SUBSCRIBER END"); Serial.flush();
    
          } // if(subscriber)
          else
          {
//            Serial.println("STAGE RESUME BEGIN 1"); Serial.flush();
            resume(); // подписчика нет, просто начинаем сначала            
//            Serial.println("STAGE RESUME END 1"); Serial.flush();
          }
          
        }   // if(wantToInformSubscriber)
        else
        {
//          Serial.println("STAGE RESUME BEGIN 2"); Serial.flush();
          resume(); // подписчика нет, просто начинаем сначала
//          Serial.println("STAGE RESUME END 2"); Serial.flush();
        }
                

        // переключаемся на ветку ожидания отщёлкивания концевика защиты
        machineState = msWaitGuardRelease;

      }

    }
    break; // msHandleInterrupts

    
  } // switch  
}
//--------------------------------------------------------------------------------------------------------------------------------------
InterruptEventSubscriber* InterruptHandlerClass::getSubscriber()
{
	return subscriber;
}
//--------------------------------------------------------------------------------------------------------------------------------------
void InterruptHandlerClass::setSubscriber(InterruptEventSubscriber* h)
{  
  // устанавливаем подписчика результатов прерываний.
  subscriber = h;
}
//--------------------------------------------------------------------------------------------------------------------------------------
bool InterruptHandlerClass::informSubscriber(CurrentOscillData* oscData, EthalonCompareResult compareResult)
{
	if (subscriber)
	{
		//DBGLN(F("Subscriber exists!"));

    // сообщаем обработчику, что данные по срабатыванию есть
		subscriber->OnInterruptRaised(oscData, compareResult);

		//DBGLN(F("Subscriber informed!"));
   return true;
	}

 return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------
