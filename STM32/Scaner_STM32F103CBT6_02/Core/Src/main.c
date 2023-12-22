/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h" // это для функции strlen()
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include "stdio.h"

#include "r820t2.h"

#include "ILI9341_GFX.h"
#include "fonts.h"
#include "img.h"
#include "xpt2046_touch.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
volatile uint8_t flag = 0;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
char trans_str[64] = {0,};
volatile uint16_t adc[6] = {0,}; // у нас два канала поэтому массив из двух элементов

volatile uint32_t R820T2_freq = 860000000;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	
	if(hadc->Instance == ADC1)
    {
        flag = 1;
    }
}

void UART_Printf(const char* fmt, ...) {
    char buff[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buff, sizeof(buff), fmt, args);
    HAL_UART_Transmit(&huart1, (uint8_t*)buff, strlen(buff),
                      HAL_MAX_DELAY);
    va_end(args);
}

HAL_StatusTypeDef UART_ReceiveString(
        UART_HandleTypeDef *huart, uint8_t *pData,
        uint16_t Size, uint32_t Timeout) {
    const char newline[] = "\r\n";
    const char delete[] = "\x08 \x08";
    HAL_StatusTypeDef status;

    if(Size == 0)
        return HAL_ERROR;

    int i = 0;
    for(;;) {
        status = HAL_UART_Receive(huart, &pData[i], 1, Timeout);
        if(status != HAL_OK)
            return status;

        if((pData[i] == '\x08')||(pData[i] == '\x7F')) { // backspace
            if(i > 0) {
                status = HAL_UART_Transmit(huart, (uint8_t*)delete,
                                           sizeof(delete)-1, Timeout);
                if(status != HAL_OK)
                    return status;
                i--;
            }
            continue;
        }

        if((pData[i] == '\r') || (pData[i] == '\n')) {
            pData[i] = '\0';
            status = HAL_UART_Transmit(huart, (uint8_t*)newline,
                                       sizeof(newline)-1, Timeout);
            if(status != HAL_OK)
                return status;
            break;
        }

        // last character is reserved for '\0'
        if(i == (Size-1)) {
            continue; // buffer is full, ignore any input
        }

        status = HAL_UART_Transmit(huart, &pData[i], 1, Timeout);
        if(status != HAL_OK)
            return status;
        i++;
    }

    return HAL_OK;
}

void init() {
    /* do nothing */
}

void cmd_help() {
    UART_Printf("help              - show this message\r\n");
    UART_Printf("scan              - perform I2C scan\r\n");
    UART_Printf("dump              - read all registers\r\n");
    UART_Printf("read <reg>        - read given register value\r\n"
                "                    (e.g `read 0A`)\r\n");
    UART_Printf("write <reg> <val> - write <val> to register <reg>\r\n"
                "                    (e.g. `write 0A E1`)\r\n");
    UART_Printf("init              - initialize R820T2\r\n");
    UART_Printf("calibrate         - calibrate R820T2\r\n");
    UART_Printf("frequency <val>   - set frequency to <val>\r\n"
                "                    (e.g. `frequency 144000000`)\r\n");
    UART_Printf("bandwidth <val>   - Set IF bandwidth [0-15]\r\n");
    UART_Printf("lna_gain <val>    - Set gain of LNA [0-15]\r\n");
    UART_Printf("vga_gain <val>    - Set gain of VGA [0-15]\r\n");
    UART_Printf("mixer_gain <val>  - Set gain Mixer [0-15]\r\n");
    UART_Printf("lna_agc <val>     - Enable/disable LNA AGC [0-1]\r\n");
    UART_Printf("mixer_agc <val>   - Enable/disable Mixer AGC [0-1]\r\n");
}

void cmd_scan() {
    HAL_StatusTypeDef res;
    for(uint16_t i = 0; i < 128; i++) {
        res = HAL_I2C_IsDeviceReady(&hi2c1, i << 1, 1, 10);
        if(res == HAL_OK) {
            UART_Printf("0x%02X ", i);
        } else {
            UART_Printf(".");
        }
    }  
    UART_Printf("\r\n");
}

void cmd_dump() {
    uint8_t regs[R820T2_NUM_REGS];
    R820T2_read(0x00, regs, sizeof(regs));
    for(uint8_t i = 0; i < R820T2_NUM_REGS; i++) {
        UART_Printf("%02X ", regs[i]);
        if((i & 0x7) == 0x7) {
            UART_Printf("  ");
        }
        if((i & 0xF) == 0xF) {
            UART_Printf("\r\n");
        }
    }
}

void cmd_read(uint8_t reg) {
    if(reg >= R820T2_NUM_REGS) {
        UART_Printf("Out of bound: 0x00-0x%02X\r\n", R820T2_NUM_REGS);
        return;
    }

    uint8_t val = R820T2_read_reg(reg);
    UART_Printf("%02X\r\n", val);
}

void cmd_write(uint8_t reg, uint8_t val) {
    if(reg >= R820T2_NUM_REGS) {
        UART_Printf("Out of bound: 0x00-0x%02X\r\n", R820T2_NUM_REGS);
        return;
    }

    R820T2_write_reg(reg, val);
}

void cmd_init() {
    R820T2_init(); 
}

void cmd_calibrate() {
    int32_t res = R820T2_calibrate(); 
    if(res != 0) {
        UART_Printf("Calibration failed, res = %d\r\n", res);
    }
}

void cmd_frequency(uint32_t val) {
    if((val < 24000000) || (val > 1766000000)) {
        UART_Printf("Out of bound: 24000000-1766000000\r\n");
        return;
    }
    R820T2_set_frequency(val); 
}

void cmd_bandwidth(uint32_t val) {
    if(val > 15) {
        UART_Printf("Out of bound: 0-15\r\n");
        return;
    }

    R820T2_set_bandwidth(val); 
}

void cmd_lna_gain(uint32_t val) {
    if(val > 15) {
        UART_Printf("Out of bound: 0-15\r\n");
        return;
    }

    R820T2_set_lna_gain(val); 
}

void cmd_vga_gain(uint32_t val) {
    if(val > 15) {
        UART_Printf("Out of bound: 0-15\r\n");
        return;
    }

    R820T2_set_vga_gain(val); 
}

void cmd_mixer_gain(uint32_t val) {
    if(val > 15) {
        UART_Printf("Out of bound: 0-15\r\n");
        return;
    }

    R820T2_set_mixer_gain(val); 
}

void cmd_lna_agc(uint32_t val) {
    if(val > 1) {
        UART_Printf("Out of bound: 0-15\r\n");
        return;
    }

    R820T2_set_lna_agc(val); 
}

void cmd_mixer_agc(uint32_t val) {
    if(val > 1) {
        UART_Printf("Out of bound: 0-1\r\n");
        return;
    }

    R820T2_set_mixer_agc(val); 
}

void loop() 
	{
    unsigned int uint1, uint2;
    char cmd[128];
    UART_Printf("r820t2> ");
    UART_ReceiveString(&huart1, (uint8_t*)cmd, sizeof(cmd), HAL_MAX_DELAY);

    if(strcmp(cmd, "") == 0) {
        /* empty command - do nothing */
    } else if(strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if(strcmp(cmd, "scan") == 0) {
        cmd_scan();
    } else if(strcmp(cmd, "init") == 0) {
        cmd_init();
    } else if(strcmp(cmd, "calibrate") == 0) {
        cmd_calibrate();
    } else if(strcmp(cmd, "dump") == 0) {
        cmd_dump();
    } else if(sscanf(cmd, "frequency %u", &uint1) == 1) {
        cmd_frequency((uint32_t)uint1);
    } else if(sscanf(cmd, "bandwidth %u", &uint1) == 1) {
        cmd_bandwidth((uint32_t)uint1);
    } else if(sscanf(cmd, "lna_gain %u", &uint1) == 1) {
        cmd_lna_gain((uint32_t)uint1);
    } else if(sscanf(cmd, "vga_gain %u", &uint1) == 1) {
        cmd_vga_gain((uint32_t)uint1);
    } else if(sscanf(cmd, "mixer_gain %u", &uint1) == 1) {
        cmd_mixer_gain((uint32_t)uint1);
    } else if(sscanf(cmd, "lna_agc %u", &uint1) == 1) {
        cmd_lna_agc((uint32_t)uint1);
    } else if(sscanf(cmd, "mixer_agc %u", &uint1) == 1) {
        cmd_mixer_agc((uint32_t)uint1);
    } else if(sscanf(cmd, "read %02x", &uint1) == 1) {
        cmd_read((uint8_t)uint1);
    } else if(sscanf(cmd, "write %02x %02x", &uint1, &uint2) == 2) {
        cmd_write((uint8_t)uint1, (uint8_t)uint2);
    } else {
        UART_Printf("Unknown command, try `help`\r\n");
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
	HAL_ADCEx_Calibration_Start(&hadc1);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc, 6); // стартуем АЦП
	HAL_TIM_Base_Start(&htim3);
	
	uint32_t R820T2_freq = 860000000;
	uint32_t  timme = 0;
	uint32_t  timmeR = 0;
	
	HAL_UART_Transmit(&huart1, (uint8_t*)"Scaner_STM32F103CBT6_02\n", 6, 1000);

  __HAL_SPI_ENABLE(DISP_SPI_PTR);

  DISP_CS_UNSELECT;
  TOUCH_CS_UNSELECT; // это нужно только если есть тач

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ILI9341_Init(); // инициализация дисплея

  /////////////////////////// далее демонстрируются различные пользовательские функции ////////////////////////////
  ILI9341_Set_Rotation(SCREEN_VERTICAL_1); // установка ориентации экрана (варианты в файле ILI9341_GFX.h)

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ILI9341_Fill_Screen(MYFON); // заливка всего экрана цветом (цвета в файле ILI9341_GFX.h)

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /* вывод строк разными шрифтами (шрифты определены в файле fonts.h, а массивы шрифтов в файле fonts.c)
  первый и второй аргумент это начало координат (справа, сверху), четвёртый аргумент шрифт
  два последних аргумента это цвет шрифта и цвет фона шрифта */

//  ILI9341_WriteString(10, 10, "Hello World", Font_7x10, WHITE, MYFON); // можно передавать непосредственно текст
//  ILI9341_WriteString(20, 30, "Hello World", Font_11x18, WHITE, MYFON);
  ILI9341_WriteString(30, 60, "Hello World", Font_16x26, BLUE, DARKGREEN);

  char txt_buf[] = "Hello World";
  ILI9341_WriteString(40, 96, txt_buf, Font_16x26, RED, GREEN); // можно передавать массив

  HAL_Delay(1000);

  ILI9341_Fill_Screen(MYFON);
	
	  char buf[64] = {0,};

  uint8_t flag_press = 1;
  uint32_t time_press = 0;

  uint8_t flag_hold = 1;
  uint32_t timme_hold = 0;

  uint16_t x = 0;
  uint16_t y = 0;

		 R820T2_init(); 
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		
	//	loop();
		
//		if(flag)
//		{
//			flag = 0;
//			HAL_ADC_Stop_DMA(&hadc1); // это необязательно
//			snprintf(trans_str, 63, "ADC %d %d %d %d %d %d\n", (uint16_t)adc[0], (uint16_t)adc[1], (uint16_t)adc[2], (uint16_t)adc[3], (uint16_t)adc[4], (uint16_t)adc[5]);
//		//	snprintf(trans_str, 63, "ADC %d %d %d\n", (uint16_t)adc[3], (uint16_t)adc[4], (uint16_t)adc[5]);
//		//	HAL_UART_Transmit(&huart1, (uint8_t*)trans_str, strlen(trans_str), 1000);
////			adc[0] = 0;
////			adc[1] = 0;
////			adc[2] = 0;
////			adc[3] = 0;
////			adc[4] = 0;
////			adc[5] = 0;

//		
//			if(R820T2_freq == 860000000)
//			timme = HAL_GetTick();
//		
//	   	cmd_frequency(R820T2_freq);
//			R820T2_freq+=4000000;


//			if(R820T2_freq > 920000000)
//			{
//				timmeR = HAL_GetTick() - timme;
//				snprintf(trans_str, 128, "Time MS %lu \n" , timmeR);
//				timmeR = HAL_GetTick();
//			//	HAL_UART_Transmit(&huart1, (uint8_t*)trans_str, strlen(trans_str), 1000);
//				R820T2_freq = 860000000;
//			}

//				HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc, 6);
//		}
//				ILI9341_Fill_Screen(MYFON);
//	if(HAL_GPIO_ReadPin(IRQ_GPIO_Port, IRQ_Pin) == GPIO_PIN_RESET && flag_press) // если нажат тачскрин
//  {
//	  x = 0;
//	  y = 0;

//	  TOUCH_CS_UNSELECT;
//	  DISP_CS_UNSELECT;

//	  HAL_SPI_DeInit(DISP_SPI_PTR);
//	  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
//	  HAL_SPI_Init(DISP_SPI_PTR);

//	  if(ILI9341_TouchGetCoordinates(&x, &y))
//	  {
//		  flag_press = 0;

//		  //////// вывод координат в уарт для отладки ////////
//		  snprintf(buf, 64, "X = %d, Y = %d\n", x, y);
//		  HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 100);
//		  buf[strlen(buf) - 1] = '\0';
//		  //////////////////////////////////////////////////////
//	  }

//	  HAL_SPI_DeInit(DISP_SPI_PTR);
//	  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
//	  HAL_SPI_Init(DISP_SPI_PTR);

//	  __HAL_SPI_ENABLE(DISP_SPI_PTR);
//	  DISP_CS_SELECT;

//	  //////// вывод координат на экран для отладки ////////
//	  ILI9341_Fill_Screen(MYFON);
//	  ILI9341_WriteString(10, 120, buf, Font_11x18, WHITE, MYFON);
//	  //////////////////////////////////////////////////////

//	  if(x > 250 && x < 285 && y > 65 && y < 96) // если нажатие происходит в области этих координат
//	  {
//		  // что-то делаем
//	  }
//	  else if(x > 120 && x < 160 && y > 50 && y < 90) // если нажатие происходит в области этих координат
//	  {
//		  // что-то делаем
//	  }
//	  ///////////////// если нажатие происходит c удержанием кнопки ////////////////
//	  else if(x > 5 && x < 90 && y > 160 && y < 230 && flag_hold) // первая кнопка
//	  {
//		  flag_hold = 0;
//		  timme_hold = HAL_GetTick();
//		  // здесь ничего не делаем
//	  }
//	  else if(x > 100 && x < 200 && y > 160 && y < 230 && flag_hold) // вторая кнопка
//	  {
//		  flag_hold = 0;
//		  timme_hold = HAL_GetTick();
//		  // здесь ничего не делаем
//	  }


//	  ///////////////////////////
//	  time_press = HAL_GetTick();
//  }

//  if(!flag_press && (HAL_GetTick() - time_press) > 100) // задержка до следующего нажатия
//  {
//	  flag_press = 1;
//  }


//  //////////////////////////// удержание кнопки //////////////////////////////
//  if(!flag_hold && HAL_GPIO_ReadPin(IRQ_GPIO_Port, IRQ_Pin) != GPIO_PIN_RESET)
//  {
//	  flag_hold = 1;
//  }

//  if(!flag_hold && (HAL_GetTick() - timme_hold) > 2000) // 2 sek удержание кнопки
//  {
//	  if(HAL_GPIO_ReadPin(IRQ_GPIO_Port, IRQ_Pin) == GPIO_PIN_RESET)
//	  {
//		  if(x > 5 && x < 90 && y > 160 && y < 230) // первая кнопка
//		  {
//			  HAL_UART_Transmit(&huart1, (uint8_t*)"LONG PRESS_1\n", 13, 100); // отладка
//			  ILI9341_WriteString(10, 150, "LONG PRESS_1", Font_11x18, WHITE, MYFON); // отладка
//			  // что-то делаем
//		  }
//		  else if(x > 100 && x < 200 && y > 160 && y < 230) // вторая кнопка
//		  {
//			  HAL_UART_Transmit(&huart1, (uint8_t*)"LONG PRESS_2\n", 13, 100); // отладка
//			  ILI9341_WriteString(10, 150, "LONG PRESS_2", Font_11x18, WHITE, MYFON); // отладка
//			  // что-то делаем
//		  }
//	  }

//	  flag_hold = 1;
//  }
//		
//		
		 
 
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 6;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_13CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_6;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 720-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 10000-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, TFT_RST_Pin|TFT_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TOUCH_CS_Pin|LCD_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : TFT_RST_Pin TFT_DC_Pin */
  GPIO_InitStruct.Pin = TFT_RST_Pin|TFT_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : IRQ_Pin */
  GPIO_InitStruct.Pin = IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TOUCH_CS_Pin LCD_LED_Pin */
  GPIO_InitStruct.Pin = TOUCH_CS_Pin|LCD_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : TFT_CS_Pin */
  GPIO_InitStruct.Pin = TFT_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TFT_CS_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
