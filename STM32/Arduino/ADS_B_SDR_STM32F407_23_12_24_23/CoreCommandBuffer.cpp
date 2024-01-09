#include "CoreCommandBuffer.h"
#include "CONFIG.h"
#include <avr/pgmspace.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include "DS3231.h"
#include "Settings.h"

//--------------------------------------------------------------------------------------------------------------------------------------
// список поддерживаемых команд
//--------------------------------------------------------------------------------------------------------------------------------------
const char DATETIME_COMMAND[] PROGMEM = "DATETIME"; // получить/установить дату/время на контроллер
const char FREERAM_COMMAND[]  PROGMEM = "FREERAM";  // получить информацию о свободной памяти
const char PIN_COMMAND[]      PROGMEM = "PIN";      // установить уровень на пине
const char VOLTAGE_COMMAND[]  PROGMEM = "VDATA";    // получить вольтаж на входах
const char VERSION_COMMAND[]  PROGMEM = "VER";      // отдать информацию о версии


//--------------------------------------------------------------------------------------------------------------------------------------
extern "C" char* sbrk(int i);
//--------------------------------------------------------------------------------------------------------------------------------------
CoreCommandBuffer Commands(&Serial);
//--------------------------------------------------------------------------------------------------------------------------------------
CoreCommandBuffer::CoreCommandBuffer(Stream* s) : pStream(s)
{
    strBuff = new String();
	strBuff->reserve(201);
}
//--------------------------------------------------------------------------------------------------------------------------------------
bool CoreCommandBuffer::hasCommand()
{
  if(!(pStream && pStream->available()))
  {
    return false;
  }

    char ch;
    
    while(pStream->available())
    {
      ch = (char) pStream->read();

      if(ch == '\r')
        continue;
            
      if(ch == '\n')
      {
        return strBuff->length() > 0; // вдруг лишние управляющие символы придут в начале строки?
      } // if

      *strBuff += ch;
      // не даём вычитать больше символов, чем надо - иначе нас можно заспамить
      if(strBuff->length() >= 200)
      {
         clearCommand();
         return false;
      } // if
    } // while

    return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------
CommandParser::CommandParser()
{
  
}
//--------------------------------------------------------------------------------------------------------------------------------------
CommandParser::~CommandParser()
{
  clear();
}
//--------------------------------------------------------------------------------------------------------------------------------------
void CommandParser::clear()
{
  for(size_t i=0;i<arguments.size();i++)
  {
    delete [] arguments[i];  
  }

  arguments.clear();
  //while(arguments.size())
  //  arguments.pop();
}
//--------------------------------------------------------------------------------------------------------------------------------------
const char* CommandParser::getArg(size_t idx) const
{
  if(arguments.size() && idx < arguments.size())
    return arguments[idx];

  return NULL;
}
//--------------------------------------------------------------------------------------------------------------------------------------
bool CommandParser::parse(const String& command, bool isSetCommand)
{
  clear();
    // разбиваем на аргументы
    const char* startPtr = command.c_str() + strlen_P(isSetCommand ? (const char* )CORE_COMMAND_SET : (const char*) CORE_COMMAND_GET);
    size_t len = 0;

    while(*startPtr)
    {
      const char* delimPtr = strchr(startPtr,CORE_COMMAND_PARAM_DELIMITER);
            
      if(!delimPtr)
      {
        len = strlen(startPtr);
        char* newArg = new char[len + 1];
        memset(newArg,0,len+1);
        strncpy(newArg,startPtr,len);
        arguments.push_back(newArg);        

        return arguments.size();
      } // if(!delimPtr)

      size_t len = delimPtr - startPtr;

     
      char* newArg = new char[len + 1];
      memset(newArg,0,len+1);
      strncpy(newArg,startPtr,len);
      arguments.push_back(newArg);

      startPtr = delimPtr + 1;
      
    } // while      

  return arguments.size();
    
}
//--------------------------------------------------------------------------------------------------------------------------------------
// CommandHandlerClass
//--------------------------------------------------------------------------------------------------------------------------------------
CommandHandlerClass CommandHandler;
//--------------------------------------------------------------------------------------------------------------------------------------
CommandHandlerClass::CommandHandlerClass()
{
  
}
//--------------------------------------------------------------------------------------------------------------------------------------
void CommandHandlerClass::handleCommands()
{
  if(Commands.hasCommand())
  {    

    String command = Commands.getCommand();

    if(command.startsWith(CORE_COMMAND_GET) || command.startsWith(CORE_COMMAND_SET))
    {
      Stream* pStream = Commands.getStream();
      processCommand(command,pStream);
    }
    

    Commands.clearCommand(); // очищаем буфер команд
  
  } // if(Commands.hasCommand())  
}
//--------------------------------------------------------------------------------------------------------------------------------------
void CommandHandlerClass::processCommand(const String& command,Stream* pStream)
{
    bool commandHandled = false;

    if(command.startsWith(CORE_COMMAND_SET))
    {
      // команда на установку свойств

      CommandParser cParser;
	  if (cParser.parse(command, true))
	  {
		  const char* commandName = cParser.getArg(0);

		  if (!strcmp_P(commandName, PIN_COMMAND))
		  {
			  // запросили установить уровень на пине SET=PIN|13|ON, SET=PIN|13|1, SET=PIN|13|OFF, SET=PIN|13|0, SET=PIN|13|ON|2000 
			  if (cParser.argsCount() > 2)
			  {
				  commandHandled = setPIN(cParser, pStream);
			  }
			  else
			  {
				  // недостаточно параметров
				  commandHandled = printBackSETResult(false, commandName, pStream);
			  }
		  } // PIN_COMMAND        

		  else
			  if (!strcmp_P(commandName, DATETIME_COMMAND)) // DATETIME
			  {
				  if (cParser.argsCount() > 1)
				  {
					  // запросили установку даты/времени, приходит строка вида 25.12.2017 12:23:49
					  const char* paramPtr = cParser.getArg(1);
					  commandHandled = printBackSETResult(setDATETIME(paramPtr), commandName, pStream);
				  }
				  else
				  {
					  // недостаточно параметров
					  commandHandled = printBackSETResult(false, commandName, pStream);
				  }

			  } // DATETIME
  
      
      //TODO: тут разбор команды !!!
      
      } // if(cParser.parse(command,true))
      
    } // SET COMMAND
    else
    if(command.startsWith(CORE_COMMAND_GET))
    {
      // команда на получение свойств
      CommandParser cParser;
      
      if(cParser.parse(command,false))
      {
        const char* commandName = cParser.getArg(0);

        if(!strcmp_P(commandName, DATETIME_COMMAND))
        {
          commandHandled = getDATETIME(commandName,pStream);
                    
        } // DATETIME_COMMAND
		//else
		//if (!strcmp_P(commandName, ETHALON_REC_COMMAND)) // EREC, GET=EREC|UP, GET=EREC|DOWN
		//{
		//	commandHandled = getEREC(cParser, pStream);

		//} // EREC
   else 
    if (!strcmp_P(commandName, VERSION_COMMAND))
    {
      commandHandled = getVER(pStream);
    }
		else
        if(!strcmp_P(commandName, PIN_COMMAND))
        {
            commandHandled = getPIN(commandName,cParser,pStream);                    
          
        } // PIN_COMMAND
        //else
        //if(!strcmp_P(commandName, ASUTP_COMMAND))
        //{
        //    commandHandled = getASUTPFLAGS(commandName,cParser,pStream);                    
        //  
        //} // ASUTP_COMMAND       
        //else
        //if(!strcmp_P(commandName, PULSES_COMMAND))
        //{
        //    commandHandled = getPULSES(commandName,cParser,pStream);                    
        //  
        //} // PULSES_COMMAND       
        //else
        //if(!strcmp_P(commandName, RDELAY_COMMAND))
        //{
        //    commandHandled = getRDELAY(commandName,cParser,pStream);                    
        //  
        //} // RDELAY_COMMAND       
        //else
        //if(!strcmp_P(commandName, CURRENT_COEFF_COMMAND))
        //{
        //    commandHandled = getCCOEFF(commandName,cParser,pStream);                    
        //  
        //} // CURRENT_COEFF_COMMAND
        //else
        //if(!strcmp_P(commandName, ECDELTA_COMMAND))
        //{
        //    commandHandled = getECDELTA(commandName,cParser,pStream);                    
        //  
        //} // ECDELTA_COMMAND     
        //else
        //if(!strcmp_P(commandName, SKIPCOUNTER_COMMAND))
        //{
        //    commandHandled = getSKIPCOUNTER(commandName,cParser,pStream);                    
        //  
        //} // SKIPCOUNTER_COMMAND       
        //else
        //if(!strcmp_P(commandName, TBORDERMAX_COMMAND))
        //{
        //    commandHandled = getTBORDERMAX(commandName,cParser,pStream);                    
        //  
        //} // TBORDERMAX_COMMAND       
        //else
        //if(!strcmp_P(commandName, TBORDERMIN_COMMAND))
        //{
        //    commandHandled = getTBORDERMIN(commandName,cParser,pStream);                    
        //  
        //} // TBORDERMIN_COMMAND       
        //else
        //if(!strcmp_P(commandName, TBORDERS_COMMAND))
        //{
        //    commandHandled = getTBORDERS(commandName,cParser,pStream);                    
        //  
        //} // TBORDERS_COMMAND
		/*
		//DEPRECATED:
        else
        if(!strcmp_P(commandName, INDUCTIVE_COMMAND))
        {
            commandHandled = getINDUCTIVE(commandName,cParser,pStream);                    
          
        } // INDUCTIVE_COMMAND       
		*/
        else
        if(!strcmp_P(commandName, VOLTAGE_COMMAND))
        {
            commandHandled = getVOLTAGE(commandName,cParser,pStream);                    
          
        } // VOLTAGE_COMMAND       

                
        //TODO: тут разбор команды !!!
        
      } // if(cParser.parse(command,false))
      
    } // GET COMMAND
    
    if(!commandHandled)
      onUnknownCommand(command, pStream);  
}
//--------------------------------------------------------------------------------------------------------------------------------------
void CommandHandlerClass::onUnknownCommand(const String& command, Stream* outStream)
{
    outStream->print(CORE_COMMAND_ANSWER_ERROR);
    outStream->println(F("UNKNOWN_COMMAND"));  
}
//--------------------------------------------------------------------------------------------------------------------------------------
bool CommandHandlerClass::getVOLTAGE(const char* commandPassed, const CommandParser& parser, Stream* pStream)
{
  if(parser.argsCount() < 1)
    return false;  


  pStream->print(CORE_COMMAND_ANSWER_OK);

  pStream->print(commandPassed);
  pStream->print(CORE_COMMAND_PARAM_DELIMITER);
  
  pStream->print(Settings.get3V3Voltage().raw);
  pStream->print(CORE_COMMAND_PARAM_DELIMITER);

  pStream->print(Settings.get5Vvoltage().raw);
  pStream->print(CORE_COMMAND_PARAM_DELIMITER);

 
  return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------------------
bool CommandHandlerClass::getVER(Stream* pStream)
{  
 // pStream->print(F("UROV "));
 // pStream->println(SOFTWARE_VERSION);

  return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------
bool CommandHandlerClass::getPIN(const char* commandPassed, const CommandParser& parser, Stream* pStream)
{
  if(parser.argsCount() < 2)
  {
    return false;  
  }

   int16_t pinNumber = atoi(parser.getArg(1));   
   int16_t pinState = getPinState(pinNumber);

  pStream->print(CORE_COMMAND_ANSWER_OK);

  pStream->print(commandPassed);
  pStream->print(CORE_COMMAND_PARAM_DELIMITER);
  pStream->print(pinNumber);
  pStream->print(CORE_COMMAND_PARAM_DELIMITER);
  pStream->println(pinState ? F("ON") : F("OFF"));   

  return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------
bool CommandHandlerClass::setPIN(CommandParser& parser, Stream* pStream)
{

  if(parser.argsCount() < 3)
    return false;
  
  int16_t pinNumber = atoi(parser.getArg(1));
  const char* level = parser.getArg(2);
  
  bool isHigh = !strcasecmp(level,(const char*) ("ON")) || *level == '1';

  pinMode(pinNumber,OUTPUT);
  digitalWrite(pinNumber,isHigh);

  pStream->print(CORE_COMMAND_ANSWER_OK);

  pStream->print(parser.getArg(0));
  pStream->print(CORE_COMMAND_PARAM_DELIMITER);
  pStream->print(pinNumber);
  pStream->print(CORE_COMMAND_PARAM_DELIMITER);
  pStream->println(level);


  return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------
int16_t CommandHandlerClass::getPinState(uint8_t pin)
{
  return digitalRead(pin);
}
//--------------------------------------------------------------------------------------------------------------------------------------
int CommandHandlerClass::getFreeMemory()
{
    char top = 't';
    return &top - reinterpret_cast<char*>(sbrk(0));

}
//--------------------------------------------------------------------------------------------------------------------------------------
bool CommandHandlerClass::getFREERAM(const char* commandPassed, Stream* pStream)
{
  if(commandPassed)
  {
      pStream->print(CORE_COMMAND_ANSWER_OK);
      pStream->print(commandPassed);
      pStream->print(CORE_COMMAND_PARAM_DELIMITER);    
  }

  pStream->println(getFreeMemory());

  return true;
    
}
//--------------------------------------------------------------------------------------------------------------------------------------
bool CommandHandlerClass::setDATETIME(const char* param)
{
    // разбираем параметр на составные части
    int8_t day = 0;
    int8_t month = 0;
    int16_t year = 0;
    int8_t hour = 0;
    int8_t minute = 0;
    int8_t second = 0;

    // буфер под промежуточные данные
    char workBuff[5] = {0};
    char* writePtr = workBuff;

    // извлекаем день
    const char* delim = strchr(param,'.');
    if(!delim || (delim - param > 4))
      return false;

    while(param < delim)
      *writePtr++ = *param++;
    *writePtr = 0;
    writePtr = workBuff;

    day = atoi(workBuff);

    param = delim+1; // перемещаемся на следующий компонент - месяц
    delim = strchr(param,'.');
    if(!delim || (delim - param > 4))
      return false;

    while(param < delim)
      *writePtr++ = *param++;
    *writePtr = 0;
    writePtr = workBuff;

    month = atoi(workBuff);

    param = delim+1; // перемещаемся на следующий компонент - год
    delim = strchr(param,' ');
    if(!delim || (delim - param > 4))
      return false;

    while(param < delim)
      *writePtr++ = *param++;
    *writePtr = 0;
    writePtr = workBuff;

    year = atoi(workBuff);

    param = delim+1; // перемещаемся на следующий компонент - час
    delim = strchr(param,':');
    if(!delim || (delim - param > 4))
      return false;

    while(param < delim)
      *writePtr++ = *param++;
    *writePtr = 0;
    writePtr = workBuff;

    hour = atoi(workBuff);

    param = delim+1; // перемещаемся на следующий компонент - минута
    delim = strchr(param,':');
    if(!delim || (delim - param > 4))
      return false;

    while(param < delim)
      *writePtr++ = *param++;
    *writePtr = 0;
    writePtr = workBuff;

    minute = atoi(workBuff);

    param = delim+1; // перемещаемся на следующий компонент - секунда

    while(*param && writePtr < &(workBuff[4]))
      *writePtr++ = *param++;
    *writePtr = 0;

    second = atoi(workBuff);

    setCurrentDateTime(day, month, year,hour,minute,second);
    
  return true;
 
}
//--------------------------------------------------------------------------------------------------------------------------------------
void CommandHandlerClass::setCurrentDateTime(uint8_t day, uint8_t month, uint16_t year, uint8_t hour, uint8_t minute, uint8_t second)
{
   // вычисляем день недели
    int16_t dow;
    uint8_t mArr[12] = {6,2,2,5,0,3,5,1,4,6,2,4};
    dow = (year % 100);
    dow = dow*1.25;
    dow += day;
    dow += mArr[month-1];
    
    if (((year % 4)==0) && (month<3))
     dow -= 1;
     
    while (dow>7)
     dow -= 7;  


   RealtimeClock.setTime(second, minute, hour, dow, day, month, year);
    
}
//--------------------------------------------------------------------------------------------------------------------------------------
bool CommandHandlerClass::getDATETIME(const char* commandPassed, Stream* pStream)
{
  // запросили получение времени/даты

  DS3231Time tm = RealtimeClock.getTime();
  String dateStr = RealtimeClock.getDateStr(tm);
  String timeStr = RealtimeClock.getTimeStr(tm);
  
  pStream->print(CORE_COMMAND_ANSWER_OK);
  pStream->print(commandPassed);
  pStream->print(CORE_COMMAND_PARAM_DELIMITER);
  pStream->print(dateStr);
  pStream->print(" ");
  pStream->println(timeStr);

  return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------
bool CommandHandlerClass::printBackSETResult(bool isOK, const char* command, Stream* pStream)
{
  if(isOK)
    pStream->print(CORE_COMMAND_ANSWER_OK);
  else
    pStream->print(CORE_COMMAND_ANSWER_ERROR);

  pStream->print(command);
  pStream->print(CORE_COMMAND_PARAM_DELIMITER);

  if(isOK)
    pStream->println(F("OK"));
  else
    pStream->println(F("BAD_PARAMS"));

  return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------

