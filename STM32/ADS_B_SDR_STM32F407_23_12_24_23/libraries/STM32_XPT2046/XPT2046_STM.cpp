#include "XPT2046_STM.h"


#define READ_Y 0xD0  // Команда чтения координаты Y
#define READ_X 0x90  // Команда чтения координаты X



TS_Point::TS_Point(void) {
	x = y = z = 0;
}

TS_Point::TS_Point(int16_t x0, int16_t y0, int16_t z0) {
	x = x0;
	y = y0;
	z = z0;
}

bool TS_Point::operator==(TS_Point p1) {
	return  ((p1.x == x) && (p1.y == y) && (p1.z == z));
}

bool TS_Point::operator!=(TS_Point p1) {
	return  ((p1.x != x) || (p1.y != y) || (p1.z != z));
}

/* ======================================================================= */

XPT2046_STM::XPT2046_STM(int8_t _cs_pin, int8_t _irq_pin)
{
	_pin_cs = _cs_pin;
	_pin_irq = _irq_pin;
}


SPI_HandleTypeDef hspi2 = {};  // Указать номер SPI

void XPT2046_STM::begin()   // Настройка порта SPI2  
{
	// Настройка порта SPI2  с применением библиотеки настройки периферии HAL для STM32
	
	pinMode(_pin_cs, OUTPUT);
	digitalWrite(_pin_cs, HIGH);
	pinMode(_pin_irq, INPUT);
	digitalWrite(_pin_irq, HIGH);

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* SPI2 clock enable */
	__HAL_RCC_SPI2_CLK_ENABLE();  // Включить тактирование SPI2
	__HAL_RCC_GPIOB_CLK_ENABLE(); // Включить тактирование pin

	/**SPI2 GPIO Configuration
	PB12     ------> SPI2_NSS
	PB13     ------> SPI2_SCK
	PB14     ------> SPI2_MISO
	PB15     ------> SPI2_MOSI
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15; // Назначить pin для SPI2
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;                                      // Настроить на вывод
	GPIO_InitStruct.Pull = GPIO_NOPULL;                                          // Поддержку резисторов отключить
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                           // Скорость SPI 50 MHz
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;                                   // Альтернативная настройка pin для SPI2
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);                                      // включить порт B
	

	HAL_NVIC_SetPriority(SPI2_IRQn, 1, 0);                                       // Настройка приоритета прерывания для тачскрина (не применяем)
	HAL_NVIC_EnableIRQ(SPI2_IRQn);                                               // Настройка прерывания SPI для тачскрина (не применяем)

	// настройки режима SPI2
	hspi2.Instance = SPI2;                             
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_HARD_OUTPUT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.Init.CRCPolynomial = 10;
	HAL_SPI_Init(&hspi2);


}

TS_Point XPT2046_STM::getPoint()
{
	
	static const uint8_t cmd_read_x[] = { READ_X };
	static const uint8_t cmd_read_y[] = { READ_Y };
	static const uint8_t zeroes_tx[] = { 0x00, 0x00 };


	digitalWrite(_pin_cs, LOW);

	uint32_t avg_x = 0;
	uint32_t avg_y = 0;
	uint8_t samples = 50;
	uint8_t nsamples = 0;

	for (uint8_t i = 0; i < samples; i++) // Для получения стабильных результатов делаем несколько(samples) измерений.
	{
		if (!TouchPressed())
			break;

		nsamples++;
		// обмен данными с тачскрином по SPI2
		HAL_SPI_Transmit(&TOUCH_SPI_PORT, (uint8_t*)cmd_read_y, sizeof(cmd_read_y), HAL_MAX_DELAY);
		uint8_t y_raw[2];
		HAL_SPI_TransmitReceive(&TOUCH_SPI_PORT, (uint8_t*)zeroes_tx, y_raw, sizeof(y_raw), HAL_MAX_DELAY);

		HAL_SPI_Transmit(&TOUCH_SPI_PORT, (uint8_t*)cmd_read_x, sizeof(cmd_read_x), HAL_MAX_DELAY);
		uint8_t x_raw[2];
		HAL_SPI_TransmitReceive(&TOUCH_SPI_PORT, (uint8_t*)zeroes_tx, x_raw, sizeof(x_raw), HAL_MAX_DELAY);

		avg_x += (((uint16_t)x_raw[0]) << 8) | ((uint16_t)x_raw[1]);
		avg_y += (((uint16_t)y_raw[0]) << 8) | ((uint16_t)y_raw[1]);
	}
	

	digitalWrite(_pin_cs, HIGH);



	//if (nsamples < samples)
	//	return false;

	uint32_t raw_x = (avg_x / nsamples);

	if (raw_x < TOUCH_MIN_RAW_X) raw_x = TOUCH_MIN_RAW_X;
	if (raw_x > TOUCH_MAX_RAW_X) raw_x = TOUCH_MAX_RAW_X;

	uint32_t raw_y = (avg_y / samples);
	if (raw_y < TOUCH_MIN_RAW_Y) raw_y = TOUCH_MIN_RAW_Y;
	if (raw_y > TOUCH_MAX_RAW_Y) raw_y = TOUCH_MAX_RAW_Y;

	
	_point.x = (raw_x - TOUCH_MIN_RAW_X) * TOUCH_SCALE_X / (TOUCH_MAX_RAW_X - TOUCH_MIN_RAW_X);
	_point.y = (raw_y - TOUCH_MIN_RAW_Y) * TOUCH_SCALE_Y / (TOUCH_MAX_RAW_Y - TOUCH_MIN_RAW_Y);

	_point.x = (raw_x - TOUCH_MIN_RAW_X) * TOUCH_SCALE_X / (TOUCH_MAX_RAW_X - TOUCH_MIN_RAW_X);
	_point.y = (raw_y - TOUCH_MIN_RAW_Y) * TOUCH_SCALE_Y / (TOUCH_MAX_RAW_Y - TOUCH_MIN_RAW_Y);

	//_point.x = TOUCH_SCALE_X - _point.x;  // если инверсия экрана
//_point.x = raw_x;
//_point.y = raw_y;
	return _point;  // в _point результат тачскрина
}


// Определение нажатия на тачскрин.
bool XPT2046_STM:: TouchPressed()
{
	return HAL_GPIO_ReadPin(TOUCH_IRQ_GPIO_Port, TOUCH_IRQ_Pin) == GPIO_PIN_RESET;
}

// Не выяснил что за инфо z и зачем нужно. В прогамме не применяю.
bool XPT2046_STM::touched()
{
	//TODO: Add irq handler
	// z ~= 3-5 by default. 100-1000 on press.
	getPoint();
	return (_point.z > 100)? true : false;
}


